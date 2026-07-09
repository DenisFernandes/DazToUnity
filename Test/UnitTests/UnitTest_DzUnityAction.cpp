#ifdef UNITTEST_DZBRIDGE

#include "UnitTest_DzUnityAction.h"
#include "DzUnityAction.h"


UnitTest_DzUnityAction::UnitTest_DzUnityAction()
{
	m_testObject = (QObject*) new DzUnityForkAction();
}

bool UnitTest_DzUnityAction::runUnitTests()
{
	RUNTEST(_DzBridgeUnityAction);
	RUNTEST(setInstallUnityFiles);
	RUNTEST(getInstallUnityFiles);
	RUNTEST(setExportStrandHairAlembic);
	RUNTEST(getExportStrandHairAlembic);
	RUNTEST(setExportStrandHairBlenderAlembic);
	RUNTEST(getExportStrandHairBlenderAlembic);
	RUNTEST(setStrandHairBlenderExecutable);
	RUNTEST(getStrandHairBlenderExecutable);
	RUNTEST(setRunStrandHairBlenderBake);
	RUNTEST(getRunStrandHairBlenderBake);
	RUNTEST(executeAction);
	RUNTEST(createUI);
	RUNTEST(writeConfiguration);
	RUNTEST(setExportOptions);
	RUNTEST(createUnityFiles);
	RUNTEST(readGuiRootFolder);

	return true;
}

bool UnitTest_DzUnityAction::_DzBridgeUnityAction(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	TRY_METHODCALL(new DzUnityForkAction());
	return bResult;
}

bool UnitTest_DzUnityAction::setInstallUnityFiles(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	TRY_METHODCALL(qobject_cast<DzUnityForkAction*>(m_testObject)->setInstallUnityFiles(false));
	return bResult;
}

bool UnitTest_DzUnityAction::getInstallUnityFiles(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	TRY_METHODCALL(qobject_cast<DzUnityForkAction*>(m_testObject)->getInstallUnityFiles());
	return bResult;
}

bool UnitTest_DzUnityAction::setExportStrandHairAlembic(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	TRY_METHODCALL(qobject_cast<DzUnityForkAction*>(m_testObject)->setExportStrandHairAlembic(false));
	TRY_METHODCALL(qobject_cast<DzUnityForkAction*>(m_testObject)->setExportStrandHairAlembic(true));
	return bResult;
}

bool UnitTest_DzUnityAction::getExportStrandHairAlembic(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	TRY_METHODCALL(qobject_cast<DzUnityForkAction*>(m_testObject)->getExportStrandHairAlembic());
	return bResult;
}

bool UnitTest_DzUnityAction::setExportStrandHairBlenderAlembic(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	TRY_METHODCALL(qobject_cast<DzUnityForkAction*>(m_testObject)->setExportStrandHairBlenderAlembic(false));
	TRY_METHODCALL(qobject_cast<DzUnityForkAction*>(m_testObject)->setExportStrandHairBlenderAlembic(true));
	return bResult;
}

bool UnitTest_DzUnityAction::getExportStrandHairBlenderAlembic(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	TRY_METHODCALL(qobject_cast<DzUnityForkAction*>(m_testObject)->getExportStrandHairBlenderAlembic());
	return bResult;
}

bool UnitTest_DzUnityAction::setStrandHairBlenderExecutable(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	TRY_METHODCALL(qobject_cast<DzUnityForkAction*>(m_testObject)->setStrandHairBlenderExecutable("C:/Program Files/Blender Foundation/Blender 4.4/blender.exe"));
	return bResult;
}

bool UnitTest_DzUnityAction::getStrandHairBlenderExecutable(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	TRY_METHODCALL(qobject_cast<DzUnityForkAction*>(m_testObject)->getStrandHairBlenderExecutable());
	return bResult;
}

bool UnitTest_DzUnityAction::setRunStrandHairBlenderBake(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	TRY_METHODCALL(qobject_cast<DzUnityForkAction*>(m_testObject)->setRunStrandHairBlenderBake(false));
	TRY_METHODCALL(qobject_cast<DzUnityForkAction*>(m_testObject)->setRunStrandHairBlenderBake(true));
	return bResult;
}

bool UnitTest_DzUnityAction::getRunStrandHairBlenderBake(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	TRY_METHODCALL(qobject_cast<DzUnityForkAction*>(m_testObject)->getRunStrandHairBlenderBake());
	return bResult;
}

bool UnitTest_DzUnityAction::executeAction(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	TRY_METHODCALL(qobject_cast<DzUnityForkAction*>(m_testObject)->executeAction());
	return bResult;
}

bool UnitTest_DzUnityAction::createUI(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	TRY_METHODCALL(qobject_cast<DzUnityForkAction*>(m_testObject)->createUI());
	return bResult;
}

bool UnitTest_DzUnityAction::writeConfiguration(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	TRY_METHODCALL(qobject_cast<DzUnityForkAction*>(m_testObject)->writeConfiguration());
	return bResult;
}

bool UnitTest_DzUnityAction::setExportOptions(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	DzFileIOSettings arg;
	TRY_METHODCALL(qobject_cast<DzUnityForkAction*>(m_testObject)->setExportOptions(arg));
	return bResult;
}

bool UnitTest_DzUnityAction::createUnityFiles(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	TRY_METHODCALL(qobject_cast<DzUnityForkAction*>(m_testObject)->createUnityFiles());
	return bResult;
}

bool UnitTest_DzUnityAction::readGuiRootFolder(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	TRY_METHODCALL(qobject_cast<DzUnityForkAction*>(m_testObject)->readGuiRootFolder());
	return bResult;
}


#include "moc_UnitTest_DzUnityAction.cpp"

#endif
