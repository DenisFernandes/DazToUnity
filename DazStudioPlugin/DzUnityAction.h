#pragma once
#include <dzaction.h>
#include <dznode.h>
#include <dzjsonwriter.h>
#include <QtCore/qfile.h>
#include <QtCore/qtextstream.h>
#include <QtCore/qlist.h>
#include <QtCore/qstringlist.h>
#include <DzBridgeAction.h>
#include "DzUnityDialog.h"

class UnitTest_DzUnityAction;

#include "dzbridge.h"

class DzUnityForkAction : public DZ_BRIDGE_NAMESPACE::DzBridgeAction {
	 Q_OBJECT
	 Q_PROPERTY(bool InstallUnityFiles READ getInstallUnityFiles WRITE setInstallUnityFiles)
	 Q_PROPERTY(bool ExportStrandHairAlembic READ getExportStrandHairAlembic WRITE setExportStrandHairAlembic)
	 Q_PROPERTY(bool ExportStrandHairBlenderAlembic READ getExportStrandHairBlenderAlembic WRITE setExportStrandHairBlenderAlembic)
	 Q_PROPERTY(QString StrandHairBlenderExecutable READ getStrandHairBlenderExecutable WRITE setStrandHairBlenderExecutable)
	 Q_PROPERTY(bool RunStrandHairBlenderBake READ getRunStrandHairBlenderBake WRITE setRunStrandHairBlenderBake)
public:
	DzUnityForkAction();

	void setInstallUnityFiles(bool arg) { m_bInstallUnityFiles = arg; }
	bool getInstallUnityFiles() { return m_bInstallUnityFiles; }
	void setExportStrandHairAlembic(bool arg) { m_bExportStrandHairAlembic = arg; }
	bool getExportStrandHairAlembic() { return m_bExportStrandHairAlembic; }
	void setExportStrandHairBlenderAlembic(bool arg) { m_bExportStrandHairAlembic = arg; }
	bool getExportStrandHairBlenderAlembic() { return m_bExportStrandHairAlembic; }
	void setStrandHairBlenderExecutable(QString arg) { m_sStrandHairBlenderExecutable = arg; }
	QString getStrandHairBlenderExecutable() { return m_sStrandHairBlenderExecutable; }
	void setRunStrandHairBlenderBake(bool arg) { m_bRunStrandHairBlenderBake = arg; }
	bool getRunStrandHairBlenderBake() { return m_bRunStrandHairBlenderBake; }

protected:
	struct HairAssetRecord
	{
		QString NodeName;
		QString NodeLabel;
		QString SourceClass;
		QString AlembicFile;
		QString RelativePath;
		QString SourceFile;
		QString JobFile;
		QString ResultFile;
		QString SchemaSummary;
		QString ExportMode;
		int FrameStart;
		int FrameEnd;
		QString ParentHint;
		QString ExportStatus;
		QString Warning;
	};

	 bool m_bInstallUnityFiles;
	 bool m_bExportStrandHairAlembic;
	 bool m_bRunStrandHairBlenderBake;
	 QString m_sStrandHairBlenderExecutable;
	 QList<HairAssetRecord> m_aHairAssets;

	 void executeAction();
	 Q_INVOKABLE bool createUI();
	 void exportNode(DzNode* Node) override;
	 Q_INVOKABLE void writeConfiguration();
	 Q_INVOKABLE void setExportOptions(DzFileIOSettings& ExportOptions);
	 Q_INVOKABLE QString createUnityFiles(bool replace = true);
	 QString readGuiRootFolder();
	 void discoverStrandHairNodes(DzNode* Node, QList<DzNode*>& HairNodes);
	 bool isDforceHairNode(DzNode* Node);
	 bool hasDforceSignal(DzNode* Node);
	 bool hasHairSignal(DzNode* Node);
	 bool containsHairKeyword(const QString& Value);
	 QString sanitizeExportName(QString Value);
	 QString getParentHint(DzNode* Node);
	 QString compactProcessOutput(const QString& Output);
	 void setNodeAndDescendantsVisible(DzNode* Node, bool Visible);
	 QString jsonEscape(const QString& Value);
	 QString defaultBlenderExecutable();
	 QString resolveBlenderExecutable();
	 QString materializeBlenderBakeScript(QString& Warning);
	 QString findAlembicInspector();
	 QStringList getDazContentDirectories();
	 QString findDiffeomorphicExportScript();
	 bool exportDiffeomorphicDbz(const QString& DbzFile, QString& Warning);
	 bool writeBlenderHairJob(const HairAssetRecord& Record, const QString& SourceDuf, const QString& SourceDbz, QString& Warning);
	 QString readJsonStringMember(const QString& JsonText, const QString& MemberName);
	 bool readJsonBoolMember(const QString& JsonText, const QString& MemberName);
	 void runBlenderHairBake(HairAssetRecord& Record);
	 void exportStrandHairAlembic(DzNode* RootNode);
	 HairAssetRecord exportStrandHairNode(DzNode* HairNode);
	 void writeHairAssets(DzJsonWriter& writer);

#ifdef UNITTEST_DZBRIDGE
	friend class UnitTest_DzUnityAction;
#endif

};
