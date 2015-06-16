// RoboMXPlugin.cpp : Defines the initialization routines for the DLL.
//

#include "stdafx.h"
#include <afxdllx.h>

#include "Plugin.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

static AFX_EXTENSION_MODULE RoboMXPluginDLL = { NULL, NULL };


//////////////////
// This is your (only!) Plugin object
/////////////////
CMetis g_rPlugin; 

extern "C" int APIENTRY
DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
	// Remove this if you use lpReserved
	UNREFERENCED_PARAMETER(lpReserved);

	if (dwReason == DLL_PROCESS_ATTACH)
	{
		TRACE0("RoboMXPlugin.DLL Initializing!\n");
		
		// Extension DLL one-time initialization
		if (!AfxInitExtensionModule(RoboMXPluginDLL, hInstance))
			return 0;

		// Insert this DLL into the resource chain
		// NOTE: If this Extension DLL is being implicitly linked to by
		//  an MFC Regular DLL (such as an ActiveX Control)
		//  instead of an MFC application, then you will want to
		//  remove this line from DllMain and put it in a separate
		//  function exported from this Extension DLL.  The Regular DLL
		//  that uses this Extension DLL should then explicitly call that
		//  function to initialize this Extension DLL.  Otherwise,
		//  the CDynLinkLibrary object will not be attached to the
		//  Regular DLL's resource chain, and serious problems will
		//  result.

		new CDynLinkLibrary(RoboMXPluginDLL);

	}
	else if (dwReason == DLL_PROCESS_DETACH)
	{
		TRACE0("RoboMXPlugin.DLL Terminating!\n");

		// Terminate the library before destructors are called
		AfxTermExtensionModule(RoboMXPluginDLL);
	}
	return 1;   // ok
}

// RoboMX calls this function when the Plugin-DLL was loaded
// to get the API-Object.
// pVer is a pointer to a variable that recieves the version
// of the API you use in the plugin
CRoboEx* RegisterExtensionEx(PLONG pVer)
{

	*pVer = API_VERSION;
	return &g_rPlugin;   // return our global API Object
}

