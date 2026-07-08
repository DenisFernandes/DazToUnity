#pragma once
#include "dzbasicdialog.h"
#include <QtGui/qcombobox.h>
#include <QtCore/qsettings.h>
#include <DzBridgeDialog.h>

class QPushButton;
class QLineEdit;
class QCheckBox;
class QComboBox;
class QGroupBox;
class QLabel;
class QWidget;
class DzUnityForkAction;

class UnitTest_DzUnityDialog;

#include "dzbridge.h"

class DzUnityForkDialog : public DZ_BRIDGE_NAMESPACE::DzBridgeDialog{
	friend DzUnityForkAction;
	Q_OBJECT
	Q_PROPERTY(QWidget* assetsFolderEdit READ getAssetsFolderEdit)
public:
	Q_INVOKABLE QLineEdit* getAssetsFolderEdit() { return assetsFolderEdit; }

	/** Constructor **/
	 DzUnityForkDialog(QWidget *parent=nullptr);

	/** Destructor **/
	virtual ~DzUnityForkDialog() {}

	Q_INVOKABLE void resetToDefaults() override;
	Q_INVOKABLE bool loadSavedSettings() override;

protected slots:
	void HandleSelectAssetsFolderButton();
	void HandleInstallUnityFilesCheckBoxChange(int state);
	void HandleAssetFolderChanged(const QString& directoryName);
	void HandleAssetTypeComboChange(int state) override;
	void HandleTargetPluginInstallerButton() override;
	void HandleOpenIntermediateFolderButton(QString sFolderPath = "") override;
	void HandleAssetTypeComboChange(const QString& assetType) override;

protected:
	QLineEdit* projectEdit;
	QPushButton* projectButton;
	QLineEdit* assetsFolderEdit;
	QPushButton* assetsFolderButton;

	QLabel* installOrOverwriteUnityFilesLabel;
	QCheckBox* installUnityFilesCheckBox;

	bool IsValidProjectFolder(QString sProjectFolderPath);
	virtual void setDisabled(bool) override;

#ifdef UNITTEST_DZBRIDGE
	friend class UnitTest_DzUnityDialog;
#endif
};
