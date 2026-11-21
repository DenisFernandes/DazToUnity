#pragma once

#include <QtCore/qlist.h>
#include <QtCore/qmap.h>
#include <QtCore/qset.h>
#include <QtCore/qstringlist.h>
#include <QtCore/qtemporaryfile.h>
#include <QtCore/qvector.h>
#include <fbxsdk.h>

class DzNode;
class DzNumericProperty;

class DzUnityGeograftMorphs
{
public:
	DzUnityGeograftMorphs(DzNode* Figure, const QMap<QString, QString>& SelectedMorphs, const QString& Destination);
	~DzUnityGeograftMorphs();

	bool isEmpty() const { return m_presets.isEmpty(); }
	bool capture();
	QString fbxPath() const { return m_stagedFbx.fileName(); }
	QString nativeMorphRules(const QString& Rules, bool& HasNativeMorphs) const;
	bool publish(FbxScene* Scene, const QStringList& MainMeshNames);
	bool published() const { return m_published; }
	QString error() const { return m_error; }
	QString destination() const { return m_destination; }

private:
	struct MeshSample
	{
		QVector<FbxVector4> Points;
		QVector<QVector<int> > Faces;
	};

	struct Preset
	{
		QString Name;
		DzNumericProperty* Control;
		double OriginalValue;
		MeshSample Target;
	};

	void addPreset(const QString& Name, DzNumericProperty* Control, const QMap<QString, QString>& SelectedMorphs);
	bool sampleMesh(const QString& Path, MeshSample& Sample);
	bool fail(const QString& Error);
	bool matchesTopology(FbxMesh* Mesh) const;

	DzNode* m_figure;
	QList<Preset> m_presets;
	QSet<DzNode*> m_grafts;
	QMap<DzNumericProperty*, double> m_resolution;
	QMap<DzNode*, bool> m_visibility;
	MeshSample m_neutral;
	QString m_destination;
	QTemporaryFile m_stagedFbx;
	QString m_error;
	bool m_scenePrepared;
	bool m_published;
};
