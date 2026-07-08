#include "dzplugin.h"
#include "dzapp.h"

#include "version.h"
#include "DzUnityAction.h"
#include "DzUnityDialog.h"

#include "dzbridge.h"

CPP_PLUGIN_DEFINITION("DazToUnity Fork Bridge");

DZ_PLUGIN_AUTHOR("Daz 3D, Inc");

DZ_PLUGIN_VERSION(PLUGIN_MAJOR, PLUGIN_MINOR, PLUGIN_REV, PLUGIN_BUILD);

#ifdef _DEBUG
DZ_PLUGIN_DESCRIPTION(QString(
	"<b>Pre-Release DazToUnity Fork Bridge v%1.%2.%3.%4 </b><br>\
<a href = \"https://github.com/daz3d/DazToUnity\">Github</a><br><br>"
).arg(PLUGIN_MAJOR).arg(PLUGIN_MINOR).arg(PLUGIN_REV).arg(PLUGIN_BUILD));
#else
DZ_PLUGIN_DESCRIPTION(QString(
"This fork plugin provides the ability to send assets to Unity. \
Documentation and source code are available on <a href = \"https://github.com/daz3d/DazToUnity\">Github</a>.<br>"
));
#endif

DZ_PLUGIN_CLASS_GUID(DzUnityForkAction, 6ddc54ab-57a9-48be-91fe-a543e0e46ddf);
NEW_PLUGIN_CUSTOM_CLASS_GUID(DzUnityForkDialog, d2f74bf9-13e5-4e47-9106-404cef286768);

#ifdef UNITTEST_DZBRIDGE

#include "UnitTest_DzUnityAction.h"
#include "UnitTest_DzUnityDialog.h"

DZ_PLUGIN_CLASS_GUID(UnitTest_DzUnityAction, 366c16df-48b0-4a27-846e-7f2856a6ee9d);
DZ_PLUGIN_CLASS_GUID(UnitTest_DzUnityDialog, 5cc19f96-c620-4052-9b9f-027c9208ca8f);

#endif
