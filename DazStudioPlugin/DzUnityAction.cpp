#include <QtGui/qcheckbox.h>
#include <QtGui/QMessageBox>
#include <QtNetwork/qudpsocket.h>
#include <QtNetwork/qabstractsocket.h>
#include <QCryptographicHash>
#include <QtCore/qcoreapplication.h>
#include <QtCore/qdir.h>
#include <QtCore/qfileinfo.h>
#include <QtCore/qpair.h>
#include <QtCore/qprocess.h>
#include <QtCore/qvariant.h>
#include <QtGui/qdesktopservices.h>

#include <dzapp.h>
#include <dzcontentmgr.h>
#include <dzfileiosettings.h>
#include <dzscene.h>
#include <dzscript.h>
#include <dzmainwindow.h>
#include <dzmaterial.h>
#include <dzshape.h>
#include <dzproperty.h>
#include <dzobject.h>
#include <dzpresentation.h>
#include <dznumericproperty.h>
#include <dzimageproperty.h>
#include <dzcolorproperty.h>
#include <dpcimages.h>

#include "QtCore/qmetaobject.h"
#include "dzmodifier.h"
#include "dzgeometry.h"
#include "dzweightmap.h"
#include "dzfacetshape.h"
#include "dzfacetmesh.h"
#include "dzfacegroup.h"
#include "dzprogress.h"

#include "DzUnityAction.h"
#include "DzUnityDialog.h"
#include "DzBridgeMorphSelectionDialog.h"
#include "DzBridgeSubdivisionDialog.h"

#ifdef WIN32
#include <shellapi.h>
#endif

#include "dzbridge.h"

DzUnityForkAction::DzUnityForkAction() :
	DzBridgeAction(tr("DazToUnity Fork"), tr("Send the selected node to Unity with DazToUnity Fork."))
{
	m_nNonInteractiveMode = 0;
	m_sAssetType = QString("SkeletalMesh");
	m_bInstallUnityFiles = false;
	m_bExportStrandHairAlembic = false;
	m_bRunStrandHairBlenderBake = true;
	m_sStrandHairBlenderExecutable = defaultBlenderExecutable();
	//Setup Icon
	QString iconName = "icon";
	QPixmap basePixmap = QPixmap::fromImage(getEmbeddedImage(iconName.toLatin1()));
	QIcon icon;
	icon.addPixmap(basePixmap, QIcon::Normal, QIcon::Off);
	QAction::setIcon(icon);

}

bool DzUnityForkAction::createUI()
{
	// Check if the main window has been created yet.
	// If it hasn't, alert the user and exit early.
	DzMainWindow* mw = dzApp->getInterface();
	if (!mw)
	{
		if (m_nNonInteractiveMode == 0) QMessageBox::warning(0, tr("Error"),
			tr("The main window has not been created yet."), QMessageBox::Ok);

		return false;
	}

	// m_subdivisionDialog creation REQUIRES valid Character or Prop selected
	if (dzScene->getNumSelectedNodes() != 1)
	{
		if (m_nNonInteractiveMode == 0) QMessageBox::warning(0, tr("Error"),
			tr("Please select one Character or Prop to send."), QMessageBox::Ok);

		return false;
	}

	 // Create the dialog
	if (!m_bridgeDialog)
	{
		m_bridgeDialog = new DzUnityForkDialog(mw);
	}
	else
	{
		DzUnityForkDialog* unityDialog = qobject_cast<DzUnityForkDialog*>(m_bridgeDialog);
		if (unityDialog)
		{
			unityDialog->resetToDefaults();
			unityDialog->loadSavedSettings();
		}
	}

	if (!m_subdivisionDialog) m_subdivisionDialog = DZ_BRIDGE_NAMESPACE::DzBridgeSubdivisionDialog::Get(m_bridgeDialog);
	if (!m_morphSelectionDialog) m_morphSelectionDialog = DZ_BRIDGE_NAMESPACE::DzBridgeMorphSelectionDialog::Get(m_bridgeDialog);

	return true;
}

void DzUnityForkAction::executeAction()
{
	// CreateUI() disabled for debugging -- 2022-Feb-25
	/*
		 // Create and show the dialog. If the user cancels, exit early,
		 // otherwise continue on and do the thing that required modal
		 // input from the user.
		 if (createUI() == false)
			 return;
	*/

	// Check if the main window has been created yet.
	// If it hasn't, alert the user and exit early.
	DzMainWindow* mw = dzApp->getInterface();
	if (!mw)
	{
		if (m_nNonInteractiveMode == 0)
		{
			QMessageBox::warning(0, tr("Error"),
				tr("The main window has not been created yet."), QMessageBox::Ok);
		}
		return;
	}

	// Create and show the dialog. If the user cancels, exit early,
	// otherwise continue on and do the thing that required modal
	// input from the user.
	if (dzScene->getNumSelectedNodes() != 1)
	{
		DzNodeList rootNodes = buildRootNodeList();
		if (rootNodes.length() == 1)
		{
			dzScene->setPrimarySelection(rootNodes[0]);
		}
		else if (rootNodes.length() > 1)
		{
			if (m_nNonInteractiveMode == 0)
			{
				QMessageBox::warning(0, tr("Error"),
					tr("Please select one Character or Prop to send."), QMessageBox::Ok);
			}
		}
	}

	// Create the dialog
	if (m_bridgeDialog == nullptr)
	{
		m_bridgeDialog = new DzUnityForkDialog(mw);
	}
	else
	{
		if (m_nNonInteractiveMode == 0)
		{
			m_bridgeDialog->resetToDefaults();
			m_bridgeDialog->loadSavedSettings();
		}
	}

	// Prepare member variables when not using GUI
	if (m_nNonInteractiveMode == 1)
	{
//		if (m_sRootFolder != "") m_bridgeDialog->getIntermediateFolderEdit()->setText(m_sRootFolder);

		if (m_aMorphListOverride.isEmpty() == false)
		{
			m_bEnableMorphs = true;
			m_sMorphSelectionRule = m_aMorphListOverride.join("\n1\n");
			m_sMorphSelectionRule += "\n1\n.CTRLVS\n2\nAnything\n0";
			if (m_morphSelectionDialog == nullptr)
			{
				m_morphSelectionDialog = DZ_BRIDGE_NAMESPACE::DzBridgeMorphSelectionDialog::Get(m_bridgeDialog);
			}
			m_mMorphNameToLabel.clear();
			foreach(QString morphName, m_aMorphListOverride)
			{
				QString label = m_morphSelectionDialog->GetMorphLabelFromName(morphName);
				m_mMorphNameToLabel.insert(morphName, label);
			}
		}
		else
		{
			m_bEnableMorphs = false;
			m_sMorphSelectionRule = "";
			m_mMorphNameToLabel.clear();
		}

	}

	// If the Accept button was pressed, start the export
	int dlgResult = -1;
	if (m_nNonInteractiveMode == 0)
	{
		dlgResult = m_bridgeDialog->exec();
	}
	if (m_nNonInteractiveMode == 1 || dlgResult == QDialog::Accepted)
	{
		// DB 2021-10-11: Progress Bar
		DzProgress* exportProgress = new DzProgress("Sending to Unity...", 10);

		// Read Common GUI values
		readGui(m_bridgeDialog);

		// Read Custom GUI values
		DzUnityForkDialog* unityDialog = qobject_cast<DzUnityForkDialog*>(m_bridgeDialog);
		if (unityDialog)
		{
			m_bInstallUnityFiles = unityDialog->installUnityFilesCheckBox->isChecked();
			if (m_nNonInteractiveMode == 0)
			{
				m_bExportStrandHairAlembic = unityDialog->exportStrandHairAlembicCheckBox->isChecked();
				m_sStrandHairBlenderExecutable = unityDialog->strandHairBlenderExecutableEdit->text();
				m_bRunStrandHairBlenderBake = unityDialog->runStrandHairBlenderBakeCheckBox->isChecked();
			}
		}
		// custom animation filename correction for Unity
		if (m_sAssetType == "Animation")
		{
			if (m_nNonInteractiveMode == 0)
			{
				// correct CharacterFolder
				m_sExportSubfolder = m_sAssetName.left(m_sAssetName.indexOf("@"));
				m_sDestinationPath = m_sRootFolder + "/" + m_sExportSubfolder + "/";
				// correct animation filename
				m_sDestinationFBX = m_sDestinationPath + m_sAssetName + ".fbx";
			}
		}

		//Create Daz3D folder if it doesn't exist
		QDir dir;
		dir.mkpath(m_sRootFolder);
		exportProgress->step();

		exportHD(exportProgress);

		// DB 2021-10-11: Progress Bar
		exportProgress->finish();

		// DB 2021-09-02: messagebox "Export Complete"
		if (m_nNonInteractiveMode == 0)
		{
			if (m_bInstallUnityFiles)
			{
				QMessageBox::information(0, "DazToUnity Fork Bridge",
					tr("Export phase from Daz Studio complete. Please switch to Unity to continue.\n\n\
If Unity Import dialog does not appear, then please double-click the \"DazToUnity HDRP\" UnityPackage \
file located in the Assets\\Daz3D\\Support\\ folder of your Unity Project."), QMessageBox::Ok);
				QString destPath = createUnityFiles(true);
#ifdef WIN32
				ShellExecute(0, 0, destPath.toLocal8Bit().data(), 0, 0, SW_SHOW);
#endif
			}
			else
			{
				QMessageBox::information(0, "DazToUnity Fork Bridge",
					tr("Export phase from Daz Studio complete. Please switch to Unity to begin Import phase."), QMessageBox::Ok);
			}
		}

	}
}

QString DzUnityForkAction::createUnityFiles(bool replace)
{
	if (!m_bInstallUnityFiles)
		return "";

	QString destinationFolder = m_sRootFolder + "/Support";
	QDir dir;
	dir.mkpath(destinationFolder);

	QString srcPathHDRP = ":/DazBridgeUnity/2019-hdrp.unitypackage";
	QFile srcFileHDRP(srcPathHDRP);
	QString destPathHDRP = destinationFolder + "/DazToUnity HDRP.unitypackage";
	this->copyFile(&srcFileHDRP, &destPathHDRP, replace);
	srcFileHDRP.close();

	QString srcPathURP = ":/DazBridgeUnity/2019-urp.unitypackage";
	QFile srcFileURP(srcPathURP);
	QString destPathURP = destinationFolder + "/DazToUnity URP.unitypackage";
	this->copyFile(&srcFileURP, &destPathURP, replace);
	srcFileURP.close();

	QString srcPathStandard = ":/DazBridgeUnity/2019-builtin.unitypackage";
	QFile srcFileStandard(srcPathStandard);
	QString destPathStandard = destinationFolder + "/DazToUnity Standard Shader.unitypackage";
	this->copyFile(&srcFileStandard, &destPathStandard, replace);
	srcFileStandard.close();


	return destPathHDRP;
}

void DzUnityForkAction::exportNode(DzNode* Node)
{
	if (Node == nullptr)
	{
		DZ_BRIDGE_NAMESPACE::DzBridgeAction::exportNode(Node);
		return;
	}

	if (!m_bExportingBaseMesh)
	{
		m_aHairAssets.clear();
	}

	bool bIsMeshAsset = (m_sAssetType == "SkeletalMesh" || m_sAssetType == "StaticMesh");
	if (m_bExportStrandHairAlembic && bIsMeshAsset && !m_bExportingBaseMesh)
	{
		exportStrandHairAlembic(Node);
	}

	DZ_BRIDGE_NAMESPACE::DzBridgeAction::exportNode(Node);
}

void DzUnityForkAction::discoverStrandHairNodes(DzNode* Node, QList<DzNode*>& HairNodes)
{
	if (Node == nullptr)
		return;

	if (isDforceHairNode(Node))
	{
		HairNodes.append(Node);
	}

	DzNodeListIterator Iterator = Node->nodeChildrenIterator();
	while (Iterator.hasNext())
	{
		DzNode* Child = Iterator.next();
		discoverStrandHairNodes(Child, HairNodes);
	}
}

bool DzUnityForkAction::isDforceHairNode(DzNode* Node)
{
	return hasDforceSignal(Node) && hasHairSignal(Node);
}

bool DzUnityForkAction::hasDforceSignal(DzNode* Node)
{
	if (Node == nullptr)
		return false;

	DzObject* Object = Node->getObject();
	if (Object == nullptr)
		return false;

	for (int index = 0; index < Object->getNumModifiers(); index++)
	{
		DzModifier* modifier = Object->getModifier(index);
		if (modifier == nullptr)
			continue;

		QString className = modifier->className().toLower();
		QString name = modifier->getName().toLower();
		if (className.contains("dforce") || name.contains("dforce"))
		{
			return true;
		}
	}

	return false;
}

bool DzUnityForkAction::hasHairSignal(DzNode* Node)
{
	if (Node == nullptr)
		return false;

	if (containsHairKeyword(Node->getName()) || containsHairKeyword(Node->getLabel()) || containsHairKeyword(Node->className()))
		return true;

	DzPresentation* presentation = Node->getPresentation();
	if (presentation && containsHairKeyword(presentation->getType()))
		return true;

	DzObject* Object = Node->getObject();
	if (Object)
	{
		if (containsHairKeyword(Object->getName()) || containsHairKeyword(Object->className()))
			return true;

		DzShape* Shape = Object->getCurrentShape();
		if (Shape)
		{
			if (containsHairKeyword(Shape->getName()) || containsHairKeyword(Shape->className()))
				return true;

			for (int i = 0; i < Shape->getNumMaterials(); i++)
			{
				DzMaterial* Material = Shape->getMaterial(i);
				if (Material == nullptr)
					continue;

				if (containsHairKeyword(Material->getName()) || containsHairKeyword(Material->getMaterialName()) || containsHairKeyword(Material->className()))
					return true;
			}
		}
	}

	return false;
}

bool DzUnityForkAction::containsHairKeyword(const QString& Value)
{
	QString value = Value.toLower();
	return value.contains("hair")
		|| value.contains("strand")
		|| value.contains("beard")
		|| value.contains("moustache")
		|| value.contains("mustache")
		|| value.contains("blended dual lobe")
		|| value.contains("dual lobe")
		|| value.contains("littlefox")
		|| value.contains("little fox")
		|| value.contains("oot hair")
		|| value.contains("oothair");
}

QString DzUnityForkAction::sanitizeExportName(QString Value)
{
	QString cleaned = Value.replace(QRegExp("[^A-Za-z0-9_]"), "_");
	while (cleaned.contains("__"))
	{
		cleaned.replace("__", "_");
	}
	cleaned = cleaned.trimmed();
	if (cleaned.isEmpty())
	{
		cleaned = "Hair";
	}
	return cleaned;
}

QString DzUnityForkAction::getParentHint(DzNode* Node)
{
	DzNode* current = Node;
	while (current)
	{
		if (current->getName().toLower().contains("head") || current->getLabel().toLower().contains("head"))
		{
			return current->getName();
		}
		current = current->getNodeParent();
	}

	return "head";
}

QString DzUnityForkAction::compactProcessOutput(const QString& Output)
{
	QString compact = Output;
	compact.replace("\r", " ");
	compact.replace("\n", " ");
	compact = compact.simplified();
	if (compact.length() > 800)
	{
		compact = compact.left(800) + "...";
	}
	return compact;
}

void DzUnityForkAction::setNodeAndDescendantsVisible(DzNode* Node, bool Visible)
{
	if (Node == nullptr)
		return;

	Node->setVisible(Visible);
	DzNodeListIterator Iterator = Node->nodeChildrenIterator();
	while (Iterator.hasNext())
	{
		DzNode* Child = Iterator.next();
		setNodeAndDescendantsVisible(Child, Visible);
	}
}

static void appendHairWarning(QString& Warning, const QString& Addition)
{
	if (Addition.isEmpty())
		return;

	if (!Warning.isEmpty())
		Warning += " ";
	Warning += Addition;
}

QString DzUnityForkAction::jsonEscape(const QString& Value)
{
	QString escaped = Value;
	escaped.replace("\\", "\\\\");
	escaped.replace("\"", "\\\"");
	escaped.replace("\r", "\\r");
	escaped.replace("\n", "\\n");
	escaped.replace("\t", "\\t");
	return escaped;
}

QString DzUnityForkAction::defaultBlenderExecutable()
{
	QStringList candidates;
	QString envBlender = QString::fromLocal8Bit(qgetenv("DAZ_TO_UNITY_BLENDER_EXE").constData()).trimmed();
	if (!envBlender.isEmpty())
		candidates << envBlender;

	candidates << "C:/Program Files/Blender Foundation/Blender 4.4/blender.exe"
		<< "C:/Program Files/Blender Foundation/Blender 5.1/blender.exe"
		<< "C:/Program Files/Blender Foundation/Blender 4.2/blender.exe";

	foreach(QString candidate, candidates)
	{
		QFileInfo fileInfo(QDir::cleanPath(candidate));
		if (fileInfo.exists() && fileInfo.isFile())
			return fileInfo.absoluteFilePath();
	}

	return candidates.isEmpty() ? QString() : candidates.first();
}

QString DzUnityForkAction::resolveBlenderExecutable()
{
	QStringList candidates;
	QString envBlender = QString::fromLocal8Bit(qgetenv("DAZ_TO_UNITY_BLENDER_EXE").constData()).trimmed();
	if (!envBlender.isEmpty())
		candidates << envBlender;
	if (!m_sStrandHairBlenderExecutable.trimmed().isEmpty())
		candidates << m_sStrandHairBlenderExecutable.trimmed();
	candidates << defaultBlenderExecutable();

	foreach(QString candidate, candidates)
	{
		QFileInfo fileInfo(QDir::cleanPath(candidate));
		if (fileInfo.exists() && fileInfo.isFile())
			return fileInfo.absoluteFilePath();
	}

	return candidates.isEmpty() ? QString() : QDir::cleanPath(candidates.first());
}

QString DzUnityForkAction::materializeBlenderBakeScript(QString& Warning)
{
	QString envScript = QString::fromLocal8Bit(qgetenv("DAZ_TO_UNITY_BLENDER_BAKE_SCRIPT").constData()).trimmed();
	if (!envScript.isEmpty())
	{
		QFileInfo envScriptInfo(QDir::cleanPath(envScript));
		if (envScriptInfo.exists() && envScriptInfo.isFile())
			return envScriptInfo.absoluteFilePath();

		appendHairWarning(Warning, "DAZ_TO_UNITY_BLENDER_BAKE_SCRIPT was set, but the file was not found: " + envScript);
	}

	QString hairDir = m_sDestinationPath;
	if (!hairDir.endsWith("/") && !hairDir.endsWith("\\"))
		hairDir += "/";
	hairDir += "Hair";
	QDir().mkpath(hairDir);

	QString materializedScript = QDir(hairDir).absoluteFilePath("dtu_blender_curve_bake.py");
	QFile::remove(materializedScript);
	if (QFile::copy(":/DazBridgeUnity/dtu_blender_curve_bake.py", materializedScript))
		return materializedScript;

	QStringList candidates;
	QString appPath = QCoreApplication::applicationDirPath();
	candidates << QDir::current().absoluteFilePath("Tools/Blender/dtu_blender_curve_bake.py")
		<< QDir(appPath).absoluteFilePath("Tools/Blender/dtu_blender_curve_bake.py")
		<< QDir(appPath).absoluteFilePath("../Tools/Blender/dtu_blender_curve_bake.py")
		<< QDir(appPath).absoluteFilePath("plugins/Tools/Blender/dtu_blender_curve_bake.py");

	foreach(QString candidate, candidates)
	{
		QFileInfo scriptInfo(QDir::cleanPath(candidate));
		if (scriptInfo.exists() && scriptInfo.isFile())
			return scriptInfo.absoluteFilePath();
	}

	Warning = "Could not materialize or find dtu_blender_curve_bake.py. Set DAZ_TO_UNITY_BLENDER_BAKE_SCRIPT to the tracked script path.";
	return QString();
}

QString DzUnityForkAction::findAlembicInspector()
{
	QStringList candidates;
	QString envInspector = QString::fromLocal8Bit(qgetenv("DAZ_TO_UNITY_ABCLS").constData()).trimmed();
	if (!envInspector.isEmpty())
		candidates << envInspector;

	QString pluginsPath = dzApp ? dzApp->getPluginsPath() : QString();
	QString homePath = dzApp ? dzApp->getHomePath() : QString();
	QString appPath = QCoreApplication::applicationDirPath();

	QStringList basePaths;
	basePaths << pluginsPath
		<< appPath
		<< QDir(pluginsPath).absoluteFilePath("DazToUnityFork")
		<< QDir(pluginsPath).absoluteFilePath("DazToUnity")
		<< QDir(appPath).absoluteFilePath("tools/alembic")
		<< QDir(homePath).absoluteFilePath("tools/alembic")
		<< "D:/SDKs/vcpkg/installed/x64-windows/tools/alembic";

	foreach(QString basePath, basePaths)
	{
		if (!basePath.isEmpty())
			candidates << QDir(basePath).absoluteFilePath("abcls.exe");
	}

	QString pathEnv = QString::fromLocal8Bit(qgetenv("PATH").constData());
	foreach(QString pathPart, pathEnv.split(";", QString::SkipEmptyParts))
	{
		candidates << QDir(pathPart.trimmed()).absoluteFilePath("abcls.exe");
	}

	foreach(QString candidate, candidates)
	{
		QFileInfo inspectorInfo(QDir::cleanPath(candidate));
		if (inspectorInfo.exists() && inspectorInfo.isFile())
			return inspectorInfo.absoluteFilePath();
	}

	return QString();
}

QStringList DzUnityForkAction::getDazContentDirectories()
{
	QStringList contentDirs;
	DzContentMgr* contentMgr = dzApp ? dzApp->getContentMgr() : nullptr;
	if (contentMgr)
	{
		for (int i = 0; i < contentMgr->getNumContentDirectories(); i++)
		{
			QString contentDir = contentMgr->getContentDirectoryPath(i);
			if (!contentDir.isEmpty())
				contentDirs << QDir::cleanPath(contentDir);
		}
	}

	QString documentsPath = QDesktopServices::storageLocation(QDesktopServices::DocumentsLocation);
	if (!documentsPath.isEmpty())
		contentDirs << QDir(documentsPath).absoluteFilePath("DAZ 3D/Studio/My Library");
	contentDirs << "C:/Users/Public/Documents/My DAZ 3D Library";

	QStringList uniqueDirs;
	foreach(QString contentDir, contentDirs)
	{
		contentDir = QDir::cleanPath(contentDir);
		if (!contentDir.isEmpty() && !uniqueDirs.contains(contentDir, Qt::CaseInsensitive))
			uniqueDirs << contentDir;
	}
	return uniqueDirs;
}

QString DzUnityForkAction::findDiffeomorphicExportScript()
{
	foreach(QString contentDir, getDazContentDirectories())
	{
		QString candidate = QDir(contentDir).absoluteFilePath("Scripts/Diffeomorphic/export_to_blender.dsa");
		QFileInfo scriptInfo(candidate);
		if (scriptInfo.exists() && scriptInfo.isFile())
			return scriptInfo.absoluteFilePath();
	}

	QString envScript = QString::fromLocal8Bit(qgetenv("DAZ_TO_UNITY_DIFFEOMORPHIC_EXPORT_SCRIPT").constData()).trimmed();
	if (!envScript.isEmpty())
	{
		QFileInfo scriptInfo(QDir::cleanPath(envScript));
		if (scriptInfo.exists() && scriptInfo.isFile())
			return scriptInfo.absoluteFilePath();
	}

	return QString();
}

bool DzUnityForkAction::exportDiffeomorphicDbz(const QString& DbzFile, QString& Warning)
{
	QString scriptPath = findDiffeomorphicExportScript();
	if (scriptPath.isEmpty())
	{
		Warning = "Diffeomorphic Daz export script was not found. Install it with Tools/Blender/install-diffeomorphic.ps1, or continue with the saved .duf fallback.";
		return false;
	}

	QFile::remove(DbzFile);
	DzScript script;
	if (!script.loadFromFile(scriptPath))
	{
		Warning = "Could not load Diffeomorphic Daz export script: " + scriptPath + ". " + script.errorMessage();
		return false;
	}

	QVariantList args;
	args << DbzFile;
	if (!script.call("exportToBlender", args))
	{
		Warning = "Diffeomorphic exportToBlender() failed: " + script.errorMessage();
		if (script.errorLine() > 0)
			Warning += QString(" line %1").arg(script.errorLine());
		QFileInfo dbzInfo(DbzFile);
		if (!dbzInfo.exists() || dbzInfo.size() <= 0)
			return false;

		Warning += ". A non-empty .dbz was still created, so the Blender curve bake will try to use it.";
		return true;
	}

	QFileInfo dbzInfo(DbzFile);
	if (!dbzInfo.exists() || dbzInfo.size() <= 0)
	{
		Warning = "Diffeomorphic exportToBlender() did not create a non-empty .dbz file: " + DbzFile;
		return false;
	}

	return true;
}

bool DzUnityForkAction::writeBlenderHairJob(const HairAssetRecord& Record, const QString& SourceDuf, const QString& SourceDbz, QString& Warning)
{
	QFile jobFile(Record.JobFile);
	if (!jobFile.open(QIODevice::WriteOnly | QIODevice::Text))
	{
		Warning = "Could not write Blender hair bake job: " + Record.JobFile + ". " + jobFile.errorString();
		return false;
	}

	QTextStream stream(&jobFile);
	stream.setCodec("UTF-8");
	stream << "{\n";
	stream << "  \"Version\": 1,\n";
	stream << "  \"AssetName\": \"" << jsonEscape(m_sAssetName) << "\",\n";
	stream << "  \"HairNodeName\": \"" << jsonEscape(Record.NodeName) << "\",\n";
	stream << "  \"HairNodeLabel\": \"" << jsonEscape(Record.NodeLabel) << "\",\n";
	stream << "  \"OutputAlembic\": \"" << jsonEscape(Record.AlembicFile) << "\",\n";
	stream << "  \"ResultJson\": \"" << jsonEscape(Record.ResultFile) << "\",\n";
	if (!SourceDuf.isEmpty())
		stream << "  \"SourceDuf\": \"" << jsonEscape(SourceDuf) << "\",\n";
	if (!SourceDbz.isEmpty())
		stream << "  \"SourceDbz\": \"" << jsonEscape(SourceDbz) << "\",\n";
	stream << "  \"Frame\": " << Record.FrameStart << ",\n";
	stream << "  \"PreferGuides\": true,\n";
	stream << "  \"ContentDirs\": [\n";
	QStringList contentDirs = getDazContentDirectories();
	for (int i = 0; i < contentDirs.count(); i++)
	{
		stream << "    \"" << jsonEscape(contentDirs[i]) << "\"";
		if (i + 1 < contentDirs.count())
			stream << ",";
		stream << "\n";
	}
	stream << "  ],\n";
	stream << "  \"AbcInspectExe\": \"" << jsonEscape(findAlembicInspector()) << "\"\n";
	stream << "}\n";
	jobFile.close();
	return true;
}

QString DzUnityForkAction::readJsonStringMember(const QString& JsonText, const QString& MemberName)
{
	QString needle = "\"" + MemberName + "\"";
	int memberIndex = JsonText.indexOf(needle);
	if (memberIndex < 0)
		return QString();
	int colonIndex = JsonText.indexOf(":", memberIndex + needle.length());
	if (colonIndex < 0)
		return QString();
	int quoteIndex = JsonText.indexOf("\"", colonIndex + 1);
	if (quoteIndex < 0)
		return QString();

	QString value;
	bool escaping = false;
	for (int i = quoteIndex + 1; i < JsonText.length(); i++)
	{
		QChar ch = JsonText.at(i);
		if (escaping)
		{
			if (ch == 'n')
				value += "\n";
			else if (ch == 'r')
				value += "\r";
			else if (ch == 't')
				value += "\t";
			else
				value += ch;
			escaping = false;
			continue;
		}
		if (ch == '\\')
		{
			escaping = true;
			continue;
		}
		if (ch == '"')
			break;
		value += ch;
	}
	return value;
}

bool DzUnityForkAction::readJsonBoolMember(const QString& JsonText, const QString& MemberName)
{
	QString needle = "\"" + MemberName + "\"";
	int memberIndex = JsonText.indexOf(needle);
	if (memberIndex < 0)
		return false;
	int colonIndex = JsonText.indexOf(":", memberIndex + needle.length());
	if (colonIndex < 0)
		return false;
	QString tail = JsonText.mid(colonIndex + 1).trimmed();
	return tail.startsWith("true", Qt::CaseInsensitive);
}

void DzUnityForkAction::runBlenderHairBake(HairAssetRecord& Record)
{
	if (!m_bRunStrandHairBlenderBake)
	{
		Record.ExportStatus = "PendingExternalBake";
		appendHairWarning(Record.Warning, "Blender bake job was written but not run because Run Strand Hair Blender Bake is disabled.");
		return;
	}

	QString blenderExe = resolveBlenderExecutable();
	QFileInfo blenderInfo(blenderExe);
	if (!blenderInfo.exists() || !blenderInfo.isFile())
	{
		Record.ExportStatus = "Skipped";
		appendHairWarning(Record.Warning, "Blender executable was not found: " + blenderExe + ". Set the UI path or DAZ_TO_UNITY_BLENDER_EXE.");
		return;
	}

	QString scriptWarning;
	QString bakeScript = materializeBlenderBakeScript(scriptWarning);
	if (bakeScript.isEmpty())
	{
		Record.ExportStatus = "Failed";
		appendHairWarning(Record.Warning, scriptWarning);
		return;
	}

	QStringList args;
	args << "--background" << "--python" << bakeScript << "--" << Record.JobFile;

	QProcess process;
	process.setWorkingDirectory(QFileInfo(Record.JobFile).absolutePath());
	process.start(blenderExe, args);
	if (!process.waitForStarted(30000))
	{
		Record.ExportStatus = "Failed";
		appendHairWarning(Record.Warning, "Failed to start Blender: " + blenderExe + ". " + process.errorString());
		return;
	}

	if (!process.waitForFinished(1200000))
	{
		process.kill();
		process.waitForFinished(30000);
		Record.ExportStatus = "Failed";
		appendHairWarning(Record.Warning, "Blender strand hair curve bake timed out.");
		return;
	}

	QString processOutput = compactProcessOutput(QString::fromLocal8Bit(process.readAllStandardOutput()) + QString::fromLocal8Bit(process.readAllStandardError()));
	QFile resultFile(Record.ResultFile);
	if (!resultFile.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		Record.ExportStatus = "Failed";
		appendHairWarning(Record.Warning, "Blender did not write a result JSON file: " + Record.ResultFile + ". " + processOutput);
		return;
	}

	QString resultJson = QString::fromUtf8(resultFile.readAll());
	resultFile.close();
	Record.SchemaSummary = readJsonStringMember(resultJson, "SchemaSummary");
	bool success = readJsonBoolMember(resultJson, "Success");
	bool containsCurve = readJsonBoolMember(resultJson, "ContainsCurveV2");
	QFileInfo alembicInfo(Record.AlembicFile);
	if (process.exitStatus() == QProcess::NormalExit && process.exitCode() == 0 && success && containsCurve && alembicInfo.exists() && alembicInfo.size() > 0)
	{
		Record.ExportStatus = "Exported";
		dzApp->log("DazToUnity Fork: Exported strand hair through Blender curves: " + Record.AlembicFile);
		return;
	}

	Record.ExportStatus = "Failed";
	QString resultError = readJsonStringMember(resultJson, "Error");
	if (resultError.isEmpty())
		resultError = "Blender finished without verified AbcGeom_Curve_v2 output.";
	appendHairWarning(Record.Warning, resultError + " " + processOutput);
}

void DzUnityForkAction::exportStrandHairAlembic(DzNode* RootNode)
{
	QList<DzNode*> HairNodes;
	discoverStrandHairNodes(RootNode, HairNodes);

	if (HairNodes.isEmpty())
	{
		dzApp->log("DazToUnity Fork: Blender strand hair export enabled, no dForce strand-based hair candidates found.");
		return;
	}

	QDir dir;
	dir.mkpath(m_sDestinationPath + "/Hair");

	foreach(DzNode* HairNode, HairNodes)
	{
		HairAssetRecord record = exportStrandHairNode(HairNode);
		m_aHairAssets.append(record);
	}
}

DzUnityForkAction::HairAssetRecord DzUnityForkAction::exportStrandHairNode(DzNode* HairNode)
{
	HairAssetRecord record;
	record.NodeName = HairNode ? HairNode->getName() : "";
	record.NodeLabel = HairNode ? HairNode->getLabel() : "";
	record.SourceClass = HairNode ? HairNode->className() : "";
	record.ExportMode = "ExternalBlenderCurveBake";
	record.FrameStart = dzScene ? dzScene->getFrame() : 0;
	record.FrameEnd = record.FrameStart;
	record.ParentHint = getParentHint(HairNode);
	record.ExportStatus = "Failed";
	record.Warning = "";

	QString assetName = sanitizeExportName(m_sExportFilename.isEmpty() ? m_sAssetName : m_sExportFilename);
	QString hairName = sanitizeExportName(record.NodeName.isEmpty() ? record.NodeLabel : record.NodeName);
	record.RelativePath = "Hair/" + assetName + "_" + hairName + ".abc";
	QString destinationPath = m_sDestinationPath;
	if (!destinationPath.endsWith("/") && !destinationPath.endsWith("\\"))
	{
		destinationPath += "/";
	}
	record.AlembicFile = destinationPath + record.RelativePath;
	record.AlembicFile.replace("\\", "/");
	record.RelativePath.replace("\\", "/");
	record.JobFile = destinationPath + "Hair/" + assetName + "_" + hairName + ".blender-hair-job.json";
	record.ResultFile = destinationPath + "Hair/" + assetName + "_" + hairName + ".blender-hair-result.json";
	record.JobFile.replace("\\", "/");
	record.ResultFile.replace("\\", "/");

	if (HairNode == nullptr)
	{
		record.Warning = "Hair node was null.";
		return record;
	}

	bool savedTo = false;
	QString sceneFile = dzScene ? dzScene->getFilename(&savedTo) : QString();
	sceneFile.replace("\\", "/");

	QString dbzFile = destinationPath + "Hair/" + assetName + "_" + hairName + ".dbz";
	dbzFile.replace("\\", "/");
	QString sourceDbz;
	QString dbzWarning;
	if (exportDiffeomorphicDbz(dbzFile, dbzWarning))
	{
		sourceDbz = dbzFile;
		if (!dbzWarning.isEmpty())
			appendHairWarning(record.Warning, dbzWarning);
	}
	else
	{
		appendHairWarning(record.Warning, dbzWarning);
	}

	QString sourceDuf = sceneFile;
	record.SourceFile = sourceDbz.isEmpty() ? sourceDuf : sourceDbz;
	if (record.SourceFile.isEmpty())
	{
		record.ExportStatus = "UnsupportedSource";
		appendHairWarning(record.Warning, "No saved Daz scene file was available for Blender/Diffeomorphic import. Save the scene or install/run the Diffeomorphic .dbz export script.");
		return record;
	}

	QString jobWarning;
	if (!writeBlenderHairJob(record, sourceDuf, sourceDbz, jobWarning))
	{
		record.ExportStatus = "Failed";
		appendHairWarning(record.Warning, jobWarning);
		return record;
	}

	runBlenderHairBake(record);
	if (!record.Warning.isEmpty())
	{
		dzApp->log("DazToUnity Fork: " + record.Warning + " Node: " + record.NodeLabel);
	}

	return record;
}

void DzUnityForkAction::writeHairAssets(DzJsonWriter& writer)
{
	if (!m_bExportStrandHairAlembic && m_aHairAssets.isEmpty())
		return;

	writer.startMemberArray("HairAssets", true);
	foreach(HairAssetRecord record, m_aHairAssets)
	{
		writer.startObject(true);
		writer.addMember("Version", 1);
		writer.addMember("Node Name", record.NodeName);
		writer.addMember("Node Label", record.NodeLabel);
		writer.addMember("Source Class", record.SourceClass);
		writer.addMember("Alembic File", record.AlembicFile);
		writer.addMember("Relative Path", record.RelativePath);
		writer.addMember("Source File", record.SourceFile);
		writer.addMember("Job File", record.JobFile);
		writer.addMember("Result File", record.ResultFile);
		writer.addMember("Schema Summary", record.SchemaSummary);
		writer.addMember("Export Mode", record.ExportMode);
		writer.addMember("Frame Start", record.FrameStart);
		writer.addMember("Frame End", record.FrameEnd);
		writer.addMember("Parent Hint", record.ParentHint);
		writer.addMember("Export Status", record.ExportStatus);
		writer.addMember("Warning", record.Warning);
		writer.finishObject();
	}
	writer.finishArray();
}

void DzUnityForkAction::writeConfiguration()
{
	QString DTUfilename = m_sDestinationPath + m_sExportFilename + ".dtu";
	QFile DTUfile(DTUfilename);
	DTUfile.open(QIODevice::WriteOnly);
	DzJsonWriter writer(&DTUfile);
	writer.startObject(true);

	writeDTUHeader(writer);

	if (m_sAssetType.toLower().contains("mesh") || m_sAssetType == "Animation")
	{
		writeAllMaterials(m_pSelectedNode, writer);
		writeAllMorphs(writer);

		// DB, 2022-June-17: Daz To Unified Bridge Format support
		writeMorphLinks(writer);
		writeMorphNames(writer);
		DzBoneList aBoneList = getAllBones(m_pSelectedNode);
		writeSkeletonData(m_pSelectedNode, writer);
		writeHeadTailData(m_pSelectedNode, writer);
		writeJointOrientation(aBoneList, writer);
		writeLimitData(aBoneList, writer);
		writePoseData(m_pSelectedNode, writer, true);

		writeAllSubdivisions(writer);
		writeAllDforceInfo(m_pSelectedNode, writer);
		if (m_sAssetType == "SkeletalMesh" || m_sAssetType == "StaticMesh")
		{
			writeHairAssets(writer);
		}
	}

	if (m_sAssetType == "Pose")
	{
		writeAllPoses(writer);
	}

	if (m_sAssetType == "Environment")
	{
		writeEnvironment(writer);
	}

	writer.finishObject();
	DTUfile.close();
}

// Setup custom FBX export options
void DzUnityForkAction::setExportOptions(DzFileIOSettings& ExportOptions)
{
	ExportOptions.setBoolValue("doEmbed", false);
	ExportOptions.setBoolValue("doDiffuseOpacity", false);
	ExportOptions.setBoolValue("doCopyTextures", false);

}

QString DzUnityForkAction::readGuiRootFolder()
{
	QString rootFolder = QDesktopServices::storageLocation(QDesktopServices::DocumentsLocation) + QDir::separator() + "DazToUnityFork";

	if (m_bridgeDialog)
	{
		QLineEdit* assetsFolderEdit = nullptr;
		DzUnityForkDialog* unityDialog = qobject_cast<DzUnityForkDialog*>(m_bridgeDialog);

		if (unityDialog)
			assetsFolderEdit = unityDialog->getAssetsFolderEdit();

		if (assetsFolderEdit)
			rootFolder = assetsFolderEdit->text().replace("\\", "/") + "/Daz3D";
	}
	return rootFolder;
}

#include "moc_DzUnityAction.cpp"
