#pragma once

#define API_VERSION 0x2000L

// this struct is for internal usage only
typedef struct TAG_EXT_MSG_STRUCT{

	LPCSTR lpszMsg;
	WPARAM wParam;
	LPARAM lParam;
	LPCSTR lpszParam;
} EXT_MSG_STRUCT;


// user info struct
typedef struct TAG_MXUSERINFO{

	CString strUser;
	CString strNodeIP;
	CString strRealIP;   // only seen by admins/ops in winmx 3.52 and later
	CString strHostname; //	only seen by admins/ops in winmx 3.52 and later
	WORD	wUserLever;  // operator / uservoice level, only winmx 3.52 and later
	DWORD   dwNodeIP;
	WORD    wNodePort;
	WORD    wLineType;
	DWORD	dwNumFiles;
	DWORD   dwStayTime;
	CTime	cJoinTime;
	DWORD   dwIdleTime;
	DWORD   dwNumMsg;
	CString strLastMsg;
} MX_USERINFO, *PMX_USERINFO;

//typedef  CArray<MX_USERINFO, MX_USERINFO> CUserArray;
typedef CMap<CString, LPCTSTR, MX_USERINFO, MX_USERINFO&> CUserMap;

// CRoboEx command target
class AFX_EXT_CLASS CRoboEx : public CObject
{
	
//Public Methods
public:
	CRoboEx();
	virtual ~CRoboEx();

	virtual void Init();
	virtual void Quit(void);
	
	// Called when client joins a channel
	// Note: pUserArray is a pointer to a CArray containing
	// all users in the channel. Please do not modify/remove or add data
	// in the array. Consider it as 'Read only!' You might crash RoboMX
	// if you modify it. You have been warned.
	// warning: modified syntax since API Version 0x1003L. No backward compability
	virtual void OnJoinChannel(DWORD dwID, CString strChannel, CUserMap* pUserMap, CString strServerString, CString strServerVersion);

	// Called in WinMX 3.53 rooms when the channel name changes
	// new since API VERSION 0x1001L
	virtual void OnRenameChannel(DWORD dwID, CString strNewname);

	// Called when your own username changes, either because of
	// Userinteraction or if the client-id changes
	// new since API Version 0x1002L
	virtual void OnSetUserName(CString strNewUsername);

	// Called when client leaves a channel
	virtual void OnLeaveChannel(DWORD dwID);

	// Called when a user joins a channel
	virtual void OnJoin(DWORD dwID, PMX_USERINFO pUser);

	// Called when a user in the channel changes an attribute
	virtual void OnRename(DWORD dwID, PMX_USERINFO pOld, PMX_USERINFO pNew);

	// Called when a user leaves the channel
	virtual void OnPart(DWORD dwID, PMX_USERINFO pUser);

	// Called when a user writes a message or action in the channel
	virtual void OnMessage(DWORD dwID, CString* pUser, CString* pMsg, BOOL bIsAction);

	// Called when a operator writes a message in the channel and the user has permission to see it
	virtual void OnOpMessage(DWORD dwID, CString* pUser, CString* pMsg);

	// Called when the user types something into the client
	virtual void OnInputHook(DWORD dwID, CString* pInput);

	// Called when a new PM arrives.
	virtual void OnPrivateMsg(CString* pSender, CString* pMessage, DWORD dwNode, WORD wPort);

	// Called when the channelbar (bIsUser = FALSE)
	// or userlisttooltip (bIsUser = TRUE) is about to be displayed
	// pTool is a pointer to the CString object containing the tooltip text
	virtual void OnToolTipPrepare(DWORD dwID, BOOL bIsUser, CString* pTool); 

	// Called when a context menu (either chat- or usermenu) is
	// about to be displayed. If bIsUser is TRUE then the userlist menu
	// is about to be display, otherwise the chatmenu.
	// hMenu is a handle to the menu to be displayed
	virtual void OnPrepareMenu(DWORD dwID, BOOL bIsUser, HMENU hMenu);
	
	// Called when a user selects an item from a menu. nCmd is the command identifer, 
	// dwID is the ID of the channel. this is -1 if the command is not channel related
	// if the command is called from a userlist menu strName contains the selected username
	virtual void OnMenuCommand(UINT nCmd, DWORD dwID, CString strName);

	// Called when RoboMX reloads settings from RoboMX.ini
	// (when user hits "Apply" in the Settings dialog of RoboMX i.e.
	virtual void OnReloadCfg();

	// Called when user clicks on Configure button in options dialog
	virtual void Configure(void);

	// Call this to write text on the chat window. Do not override this method
	void WriteEchoText(DWORD dwID, CString strText, COLORREF crText, COLORREF crBg);

	// Call this method to write text on the system console of the chat. Do not override this method!
	void WriteSystemEchoText(DWORD dwID, CString strMsg, COLORREF crText, COLORREF crBg);

	// Call this method to display a tooltip in the RoboMX Systray area
	// (Note: This only works on Windows 2000/XP and on 98/ME if IE6 is installed)
	// Timeout: 1 to 10 seconds, dwIcon: 1 = Icon Information, 2 = Warning, 3 = Error
	void DisplayToolTip(CString strMessage, UINT uTimeout, DWORD dwIcon);

	// Call this method to display a tooltip in the channelbar
	// dwID is the ID of the channel, strMessage the text to display in the tooltip :)
	void DisplayChannelToolTip(DWORD dwID, CString strMessage);

	// Call this method to Input text into the chat. Do not override this method
	void InputMessage(DWORD dwID, CString strMsg);

	// Call this methot to send a PM to a user <strName>
	// dwTargetNode and wTargetPort must be the node of the recipient
	// (get from the userarray ;)
	void SendPrivateMsg(DWORD dwTargetNode, WORD wTargetPort, CString strName, CString strMsg);

	// Call this method to query information from RoboMX
	// Have a look in qapi.h for more details
	LRESULT QueryInformation(UINT uQueryID, WPARAM wParam, LPARAM lParam);
// Public Attributes
public: 

	// String containing the name of the plugin
	CString m_strName;
	// String containing the Author of the plugin
	CString m_strAuthor;
	// String containing a description what the plugin does
	CString m_strDescription;
};

typedef CRoboEx*	(*REGISTERPLUGIN)(PLONG);

AFX_EXT_API CRoboEx* RegisterExtension(PLONG pVersion);


