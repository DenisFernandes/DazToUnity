#include "DzUnityGeograftMorphs.h"

#include <QtCore/qdir.h>
#include <QtCore/qtextstream.h>
#include <dzapp.h>
#include <dzenumproperty.h>
#include <dzexporter.h>
#include <dzexportmgr.h>
#include <dzfacetshape.h>
#include <dzfigure.h>
#include <dzfileiosettings.h>
#include <dzfloatproperty.h>
#include <dzmorph.h>
#include <dzobject.h>
#include <dzscene.h>

#include "OpenFBXInterface.h"

#include <cmath>
#include <cstdio>
#ifdef Q_OS_WIN
#include <windows.h>
#endif

namespace
{
	const double ControlTolerance = 0.000001;
	const double PositionTolerance = 0.00001;

	bool replaceFile(const QString& Source, const QString& Destination)
	{
#ifdef Q_OS_WIN
		const QString NativeSource = QDir::toNativeSeparators(Source);
		const QString NativeDestination = QDir::toNativeSeparators(Destination);
		return MoveFileExW(reinterpret_cast<LPCWSTR>(NativeSource.utf16()),
			reinterpret_cast<LPCWSTR>(NativeDestination.utf16()), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != 0;
#else
		return std::rename(QFile::encodeName(Source).constData(), QFile::encodeName(Destination).constData()) == 0;
#endif
	}
}

DzUnityGeograftMorphs::DzUnityGeograftMorphs(DzNode* Figure, const QMap<QString, QString>& SelectedMorphs, const QString& Destination)
	: m_figure(Figure), m_destination(Destination), m_stagedFbx(QDir::tempPath() + "/DazUnityGraft-XXXXXX.fbx"),
	  m_scenePrepared(false), m_published(false)
{
	if (Figure == nullptr || SelectedMorphs.isEmpty())
		return;

	// DAZ projects ordinary figure morphs onto fitted grafts. The original figure owns those controls.
	QMap<QString, QString> GraftSelection = SelectedMorphs;
	DzObject* FigureObject = Figure->getObject();
	for (auto Item = SelectedMorphs.constBegin(); Item != SelectedMorphs.constEnd(); ++Item)
	{
		if (qobject_cast<DzNumericProperty*>(Figure->findProperty(Item.key(), false)) != nullptr ||
			(FigureObject != nullptr && qobject_cast<DzMorph*>(FigureObject->findModifier(Item.key())) != nullptr))
			GraftSelection.remove(Item.key());
	}

	DzNodeList Descendants;
	Figure->getNodeChildren(Descendants, true);
	foreach (DzNode* Node, Descendants)
	{
		DzFigure* Graft = qobject_cast<DzFigure*>(Node);
		if (Graft == nullptr || !Graft->isGraftFollowing() || Graft->getFollowTarget() != Figure || !Node->isVisible())
			continue;

		m_grafts.insert(Node);
		for (int Index = 0; Index < Node->getNumProperties(); ++Index)
		{
			DzProperty* Property = Node->getProperty(Index);
			addPreset(Property->getName(), qobject_cast<DzNumericProperty*>(Property), GraftSelection);
		}
		DzObject* Object = Node->getObject();
		if (Object == nullptr)
			continue;
		for (int Index = 0; Index < Object->getNumModifiers(); ++Index)
		{
			DzMorph* Morph = qobject_cast<DzMorph*>(Object->getModifier(Index));
			if (Morph != nullptr)
				addPreset(Morph->getName(), Morph->getValueChannel(), GraftSelection);
		}
	}
}

DzUnityGeograftMorphs::~DzUnityGeograftMorphs()
{
	if (m_scenePrepared)
	{
		foreach (const Preset& Item, m_presets)
			Item.Control->setDoubleValue(Item.OriginalValue);
		for (auto Item = m_resolution.constBegin(); Item != m_resolution.constEnd(); ++Item)
			Item.key()->setDoubleValue(Item.value());
	}
	for (auto Item = m_visibility.constBegin(); Item != m_visibility.constEnd(); ++Item)
		Item.key()->setVisible(Item.value());
}

void DzUnityGeograftMorphs::addPreset(const QString& Name, DzNumericProperty* Control, const QMap<QString, QString>& SelectedMorphs)
{
	if (Control == nullptr || !SelectedMorphs.contains(Name))
		return;
	foreach (const Preset& Item, m_presets)
	{
		if (Item.Control == Control)
			return;
		if (Item.Name == Name)
		{
			fail(QString("The selected graft morph '%1' occurs on more than one control.").arg(Name));
			return;
		}
	}
	Preset Item;
	Item.Name = Name;
	Item.Control = Control;
	Item.OriginalValue = Control->getDoubleValue();
	m_presets.append(Item);
}

bool DzUnityGeograftMorphs::capture()
{
	if (!m_error.isEmpty())
		return false;
	if (!m_stagedFbx.open())
		return fail("Could not create a temporary FBX.");
	m_stagedFbx.close();

	DzNodeList Descendants;
	m_figure->getNodeChildren(Descendants, true);
	QMap<DzNode*, bool> HiddenForSampling;
	DzNodeListIterator Nodes = dzScene->nodeListIterator();
	while (Nodes.hasNext())
	{
		DzNode* Node = Nodes.next();
		DzObject* Object = Node->getObject();
		if (Object == nullptr)
			continue;
		if (Node != m_figure && !Descendants.contains(Node))
		{
			if (Node->isVisible())
			{
				m_visibility.insert(Node, true);
				Node->setVisible(false);
			}
			continue;
		}
		if (Node != m_figure && !m_grafts.contains(Node) && Node->isVisible())
		{
			HiddenForSampling.insert(Node, true);
			Node->setVisible(false);
		}
		DzFacetShape* Shape = qobject_cast<DzFacetShape*>(Object->getCurrentShape());
		DzNumericProperty* Lod = Shape == nullptr ? nullptr : Shape->getLODControl();
		if (Lod != nullptr)
			m_resolution.insert(Lod, Lod->getDoubleValue());
	}

	m_scenePrepared = true;
	for (auto Item = m_resolution.constBegin(); Item != m_resolution.constEnd(); ++Item)
		Item.key()->setDoubleValue(0.0);

	QTemporaryFile Obj(QDir::tempPath() + "/DazUnityGraft-XXXXXX.obj");
	bool Captured = Obj.open();
	Obj.close();
	if (!Captured)
		fail("Could not create a temporary OBJ.");
	if (Captured)
	{
		foreach (const Preset& Item, m_presets)
			Item.Control->setDoubleValue(0.0);
		foreach (const Preset& Item, m_presets)
		{
			if (qAbs(Item.Control->getDoubleValue()) > ControlTolerance)
				Captured = fail(QString("Could not zero the graft morph '%1'.").arg(Item.Name));
		}
	}
	if (Captured)
		Captured = sampleMesh(Obj.fileName(), m_neutral);
	for (int Index = 0; Captured && Index < m_presets.size(); ++Index)
	{
		Preset& Item = m_presets[Index];
		Item.Control->setDoubleValue(1.0);
		if (qAbs(Item.Control->getDoubleValue() - 1.0) > ControlTolerance)
			Captured = fail(QString("Could not set the graft morph '%1' to 100%.").arg(Item.Name));
		else
			Captured = sampleMesh(Obj.fileName(), Item.Target);
		Item.Control->setDoubleValue(0.0);
		if (Captured && (Item.Target.Faces != m_neutral.Faces || Item.Target.Points.size() != m_neutral.Points.size()))
			Captured = fail(QString("The graft morph '%1' changed mesh topology.").arg(Item.Name));
	}
	for (auto Item = HiddenForSampling.constBegin(); Item != HiddenForSampling.constEnd(); ++Item)
		Item.key()->setVisible(Item.value());
	return Captured;
}

bool DzUnityGeograftMorphs::sampleMesh(const QString& Path, MeshSample& Sample)
{
	DzExporter* Exporter = dzApp->getExportMgr()->findExporterByClassName("DzObjExporter");
	if (Exporter == nullptr)
		return fail("DAZ Studio's OBJ exporter is unavailable.");

	DzFileIOSettings Options;
	Exporter->getDefaultOptions(&Options);
	Options.setIntValue("RunSilent", 1);
	Options.setFloatValue("Scale", 1.0);
	Options.setIntValue("FloatPrecision", 8);
	Options.setStringValue("LatAxis", "X");
	Options.setStringValue("VertAxis", "Y");
	Options.setStringValue("DepthAxis", "Z");
	Options.setBoolValue("InvertLat", false);
	Options.setBoolValue("InvertVert", false);
	Options.setBoolValue("InvertDepth", false);
	Options.setBoolValue("IgnoreInvisible", true);
	Options.setBoolValue("RemoveUnusedVerts", true);
	Options.setBoolValue("SelectedOnly", false);
	Options.setBoolValue("SelectedRootsOnly", false);
	Options.setBoolValue("PrimaryRootOnly", false);
	Options.setBoolValue("WeldSeams", false);
	Options.setBoolValue("TriangulateNgons", false);
	Options.setBoolValue("WriteF", true);
	Options.setBoolValue("WriteVT", false);
	Options.setBoolValue("WriteVN", false);
	Options.setBoolValue("WriteMtllib", false);
	Options.setBoolValue("CollectMaps", false);
	Options.setBoolValue("ConvertMaps", false);
	const DzError Result = Exporter->writeFile(Path, &Options);
	Exporter->deleteLater();
	if (Result != DZ_NO_ERROR)
		return fail(QString("DAZ Studio could not sample the graft mesh (OBJ error %1).").arg(Result));

	QFile File(Path);
	if (!File.open(QIODevice::ReadOnly | QIODevice::Text))
		return fail("Could not read the temporary OBJ.");
	QTextStream Text(&File);
	while (!Text.atEnd())
	{
		const QString Line = Text.readLine();
		if (!Line.startsWith("v ") && !Line.startsWith("f "))
			continue;
		const QStringList Tokens = Line.simplified().split(' ');
		if (Tokens.first() == "v")
		{
			FbxVector4 Point;
			for (int Axis = 0; Axis < 3; ++Axis)
			{
				bool Valid = false;
				Point[Axis] = Tokens.value(Axis + 1).toDouble(&Valid);
				if (!Valid || !std::isfinite(Point[Axis]))
					return fail("An OBJ sample contains an invalid vertex.");
			}
			Point[3] = 1.0;
			Sample.Points.append(Point);
		}
		else
		{
			QVector<int> Face;
			for (int Index = 1; Index < Tokens.size(); ++Index)
			{
				bool Valid = false;
				const int Vertex = Tokens[Index].section('/', 0, 0).toInt(&Valid);
				if (!Valid || Vertex <= 0 || Vertex > Sample.Points.size())
					return fail("An OBJ sample contains an invalid face index.");
				Face.append(Vertex - 1);
			}
			Sample.Faces.append(Face);
		}
	}
	if (Sample.Points.isEmpty() || Sample.Faces.isEmpty())
		return fail("DAZ Studio produced an empty graft mesh sample.");
	return true;
}

QString DzUnityGeograftMorphs::nativeMorphRules(const QString& Rules, bool& HasNativeMorphs) const
{
	QSet<QString> GraftNames;
	foreach (const Preset& Item, m_presets)
		GraftNames.insert(Item.Name);
	QStringList Filtered;
	foreach (const Preset& Item, m_presets)
		Filtered << Item.Name << "2";
	HasNativeMorphs = false;
	const QStringList Pairs = Rules.split('\n');
	for (int Index = 0; Index + 1 < Pairs.size(); Index += 2)
	{
		if (GraftNames.contains(Pairs[Index]))
			continue;
		Filtered << Pairs[Index] << Pairs[Index + 1];
		if (Pairs[Index + 1] == "1")
			HasNativeMorphs = true;
	}
	return Filtered.join("\n");
}

bool DzUnityGeograftMorphs::matchesTopology(FbxMesh* Mesh) const
{
	if (Mesh == nullptr || Mesh->GetDeformerCount(FbxDeformer::eSkin) == 0 ||
		Mesh->GetControlPointsCount() != m_neutral.Points.size() || Mesh->GetPolygonCount() != m_neutral.Faces.size())
		return false;
	for (int Face = 0; Face < Mesh->GetPolygonCount(); ++Face)
	{
		const QVector<int>& SampleFace = m_neutral.Faces[Face];
		if (Mesh->GetPolygonSize(Face) != SampleFace.size())
			return false;
		for (int Vertex = 0; Vertex < SampleFace.size(); ++Vertex)
		{
			if (Mesh->GetPolygonVertex(Face, Vertex) != SampleFace[Vertex])
				return false;
		}
	}
	return true;
}

bool DzUnityGeograftMorphs::publish(FbxScene* Scene, const QStringList& MainMeshNames)
{
	QList<FbxMesh*> Matches;
	QList<FbxMesh*> NamedMatches;
	for (int Index = 0; Index < Scene->GetGeometryCount(); ++Index)
	{
		FbxMesh* Mesh = FbxCast<FbxMesh>(Scene->GetGeometry(Index));
		if (matchesTopology(Mesh) && Mesh->GetNode() != nullptr)
		{
			Matches.append(Mesh);
			if (MainMeshNames.contains(QString::fromUtf8(Mesh->GetName())) ||
				MainMeshNames.contains(QString::fromUtf8(Mesh->GetNode()->GetName())))
				NamedMatches.append(Mesh);
		}
	}
	if (!NamedMatches.isEmpty())
		Matches = NamedMatches;
	if (Matches.size() != 1)
		return fail("Could not match the merged OBJ topology to one skinned FBX mesh.");

	FbxMesh* Mesh = Matches.first();
	FbxNode* Node = Mesh->GetNode();
	const FbxAMatrix Geometry(Node->GetGeometricTranslation(FbxNode::eSourcePivot),
		Node->GetGeometricRotation(FbxNode::eSourcePivot), Node->GetGeometricScaling(FbxNode::eSourcePivot));
	const FbxAMatrix ObjectToWorld = Node->EvaluateGlobalTransform() * Geometry;
	const FbxAMatrix WorldToObject = ObjectToWorld.Inverse();
	const double CentimetersPerUnit = Scene->GetGlobalSettings().GetSystemUnit().GetScaleFactor();
	if (!std::isfinite(CentimetersPerUnit) || CentimetersPerUnit <= 0.0)
		return fail("The FBX uses an invalid unit scale.");
	auto toLocal = [&](const FbxVector4& Point) {
		return WorldToObject.MultT(FbxVector4(Point[0] / CentimetersPerUnit, Point[1] / CentimetersPerUnit,
			Point[2] / CentimetersPerUnit, 1.0));
	};
	const FbxVector4* Points = Mesh->GetControlPoints();
	QVector<FbxVector4> NeutralLocal;
	NeutralLocal.reserve(m_neutral.Points.size());
	for (int Index = 0; Index < m_neutral.Points.size(); ++Index)
	{
		const FbxVector4 Neutral = toLocal(m_neutral.Points[Index]);
		// Some DAZ export settings smooth the base vertices. Require the samples and FBX to still agree in space.
		if ((ObjectToWorld.MultT(Points[Index]) - ObjectToWorld.MultT(Neutral)).Length() * CentimetersPerUnit > 2.0)
			return fail("The neutral OBJ and FBX vertices differ in orientation or pose.");
		NeutralLocal.append(Neutral);
	}

	FbxBlendShape* BlendShape = FbxBlendShape::Create(Scene, "Geograft Morphs");
	foreach (const Preset& Item, m_presets)
	{
		const QByteArray Name = Item.Name.toUtf8();
		FbxShape* Shape = FbxShape::Create(Scene, Name.constData());
		Shape->InitControlPoints(m_neutral.Points.size());
		bool HasDeformation = false;
		for (int Index = 0; Index < m_neutral.Points.size(); ++Index)
		{
			const FbxVector4 Delta = toLocal(Item.Target.Points[Index]) - NeutralLocal[Index];
			FbxVector4 Position = Points[Index];
			for (int Axis = 0; Axis < 3; ++Axis)
				Position[Axis] += Delta[Axis];
			HasDeformation = HasDeformation || Delta.Length() > PositionTolerance;
			Shape->SetControlPointAt(Position, Index);
		}
		if (!HasDeformation)
			return fail(QString("The selected graft morph '%1' did not deform the mesh.").arg(Item.Name));

		FbxBlendShapeChannel* Channel = FbxBlendShapeChannel::Create(Scene, Name.constData());
		if (!Channel->AddTargetShape(Shape, 100.0) || !BlendShape->AddBlendShapeChannel(Channel))
			return fail(QString("Could not attach the graft morph '%1' to the FBX.").arg(Item.Name));
		Channel->DeformPercent.Set(Item.OriginalValue * 100.0);
	}
	if (Mesh->AddDeformer(BlendShape) < 0 || !OpenFBXInterface::GetInterface()->SaveScene(Scene, fbxPath()))
		return fail("Could not write the graft blend shapes to the FBX.");

	// Publish a completed file atomically, even when the temporary directory is on a different drive.
	QString PendingPath;
	{
		QTemporaryFile Output(m_destination + ".XXXXXX.tmp");
		if (!Output.open())
			return fail("Could not stage the completed FBX beside its destination.");
		QFile Stage(fbxPath());
		if (!Stage.open(QIODevice::ReadOnly))
			return fail("Could not read the completed temporary FBX.");
		while (!Stage.atEnd())
		{
			const QByteArray Block = Stage.read(1024 * 1024);
			if (Block.isEmpty() || Output.write(Block) != Block.size())
				return fail("Could not copy the completed FBX to its destination.");
		}
		if (!Output.flush())
			return fail("Could not flush the completed FBX to disk.");
		PendingPath = Output.fileName();
		Output.setAutoRemove(false);
	}
	if (!replaceFile(PendingPath, m_destination))
	{
		QFile::remove(PendingPath);
		return fail("Could not replace the destination FBX.");
	}
	m_published = true;
	dzApp->log(QString("DazToUnity Fork: Exported %1 graft blend shapes.").arg(m_presets.size()));
	return true;
}

bool DzUnityGeograftMorphs::fail(const QString& Error)
{
	m_error = Error;
	return false;
}
