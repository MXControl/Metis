/*
** Copyright (C) 2003-2006 Bender
**  
** Metis is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
** 
** Metis is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
** 
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software 
** Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

#if !defined(AFX_PLUGIN_H__B188A380_735B_4FFE_BD75_3F5B00775250__INCLUDED_)
#define AFX_PLUGIN_H__B188A380_735B_4FFE_BD75_3F5B00775250__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <winsock2.h>

#include "RoboEx.h"

#include "Ini.h"
#include <queue>
#include <vector>
#include <Afxmt.h>
#include <Afxtempl.h>
#include "types.h"

typedef struct TAG_ROOMDATA{

	CString strRoomName;
	CString strServer;
	CString strVersion;
	DWORD   dwID;
	BOOL    bIsBotActivated;
	CUserMap* pUserMap;
	HWND    hChat;

} ROOMDATA, *PROOMDATA;

typedef struct TAG_EXT_CMD{

	CRoboEx *pPlugin;
	PCOMMAND pCmd;
	CString strNickname;
	CString strMessage;
	DWORD dwID;
} EXT_CMD, *PEXT_CMD;

enum CMD_TAG_TYPE{

	RENAME = 0,
	TIMER,
	LEAVE,
	ENTER,
	JOIN,
	CMD,
	OP,
	PM	
};

class TiXmlNode;

class CMetis : public CRoboEx
{
public:

	BOOL SubclassChat(HWND hWnd);
	static LRESULT CALLBACK SubChatProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	DWORD GetID(HWND hChat);

	BOOL IsAdmin(CString &strUser);
	void UpdateAutoMessageInterval(unsigned int* pTick);
	void InputMyMessage(DWORD dwID, CString strMsg);
	BOOL ValidateUsergroups();
	BOOL ValidateCommand(CString& strFile, PCOMMAND pCmd, UINT nType);

	void RunCommand(PEXT_CMD pExtCmd);
	CString Operator(int nType, CString lValue, CString rValue, CString nValue);
	CMetis();
	virtual ~CMetis();

	// Overides of virtual members in MXChatPlugin
	void Init(void);
	void Quit();

	void OnPart(DWORD dwID, PMX_USERINFO pUserInfo);
	void OnMessage(DWORD dwID, CString* pUser, CString* pMsg, BOOL bIsAction);
	void OnOpMessage(DWORD dwID, CString* pUser, CString* pMsg);
    void OnSetUserName(CString strNewUsername);
	void OnJoin(DWORD dwID, PMX_USERINFO pUserInfo);
	void OnJoinChannel(DWORD dwID, CString strChannel, CUserMap* pUserMap, CString strServerString, CString strServerVersion);
	void OnRenameChannel(DWORD dwID, CString strNewname);
	void OnRename(DWORD dwID, PMX_USERINFO pOld, PMX_USERINFO pNew);
	void OnLeaveChannel(DWORD dwID);
	void OnInputHook(DWORD dwID, CString *pString);
	void OnPrivateMsg(CString* pSender, CString* pMessage, DWORD dwNode, WORD wPort);
	void OnPrepareMenu(DWORD dwID, BOOL bIsUser, HMENU hMenu);
	void OnMenuCommand(UINT nCmd, DWORD dwID, CString strName);
	void OnReloadCfg();
	// Called when the channelbar (bIsUser = FALSE)
	// or userlisttooltip (bIsUser = TRUE) is about to be displayed
	// pTool is a pointer to the CString object containing the tooltip text
	virtual void OnToolTipPrepare(DWORD dwID, BOOL bIsUser, CString* pTool); 

	HINSTANCE m_hInstance;
	HICON     m_hIcon;
	HMENU     m_hMetis;
	HMENU	  m_hFiles;
protected:

	void ReplaceHostname(CString& strString);

	// TRIVIA
	int IncreaseScore(CString& strUser, int nPoints, PINT nDone);
	int GetScore(CString& strUser);
	void InitTrivia();
	void ClearGame();
	void Trivia(DWORD dwID, CString strName, CString strMessage);
	CString m_strTScore;
	CString m_strTNoAnswer;
	CString m_strTCorrect;
	CString m_strTSkip;
	CString m_strTNext;
	CString m_strTStart;
	CArray<PTRIVIA, PTRIVIA> m_aTrivia;
	CArray<PTRIVIA, PTRIVIA> m_aGame;
	PTRIVIA m_pCurrent;
	BOOL   m_bTrivia;
	static UINT TriviaThread(PVOID pVoid);
	CEvent m_eTrivia;
	int m_nCurrent;
	int m_nRoundEnd;
	int m_nRoundCurrent;
	DWORD m_dwTriviaID;
	DWORD m_nTriviaTime;
	// TRIVIA END

	void ClearRandom();
	// Attributes

	//global settings
	CString m_strLast;      // last line said by the bpt
	CString	m_strBotname;	// Name of Bot (Read from MXChat.ini)
	CString	m_strBadLan;    // Bad language response
	CString m_strPreKick;   // Message posted befor Penalty is executed
	BOOL	m_bBadWord;     // Activate Badlanguage feature
	int		m_nAutoKick;    // Number of warnings before autokick
	BOOL	m_bAutoKick;    // enable autokick
	BOOL	m_bExclude;     // exclude users from autokick
	int     m_nFlood;       // default wait time between messages (in milliseconds)
	BOOL    m_bParam;	    // Security option Parameter
    BOOL    m_bNoNick;		// Securtiy option nickname

	UINT	m_nBotTicRem;         // Helper for Automessage
	int		m_nSecs;	          // Interval for Automessage in seconds
	int		m_nBlockTime;		  // Userflood control ban time in seconds
	int		m_nBlockRange;		  // Userflood control time frame in seconds
	int		m_nBlockMessageCount; // Userflood control number of allowed message per time frame
	CString m_strSeperator;       // Seperator for %NICK%
	CString m_strEditor;		  // Default editor for xml files
	CString	m_strBadLanRedirect;  // Message for Badlanguage redirect command
	int		m_nDefaultFlood;      // Default flood protection
	int		m_nDefCmdType;		  // Default command type
	BOOL	m_bMiniTray;		  // Minimize to tray or not

	CStringArray m_aWarnedNicks;
	CStringArray m_aIncludeFiles;
	CStringArray m_aParseErrors;


	std::queue<BOT_MESSAGE> m_qMessageQueue;  // Message queue of bot for output
	CArray<USER_INFO, USER_INFO> m_aUserInfo; // Array holding userflood names
	
	BOOL		m_bStopOut;			// Stop bot
	BOOL		m_bStopCmdThreads;  // Stop all cmd threads
	CEvent		m_eThreadFinished;	// Event to signal that bot is halted
	CEvent      m_eTimerThread;
	DWORD		m_dwPMID;

	CArray<PBADWORD, PBADWORD> m_aBadWords; // Array holding all badwords
	CArray<PCOMMAND, PCOMMAND> m_aCommands; // Array holding all commands
	CArray<PCOMMAND, PCOMMAND> m_aOpCommands; // Array holding all operator commands (WinMX 3.53 only)
	CArray<PCOMMAND, PCOMMAND> m_aEnter;    // Array holding all enter messages
	CArray<PCOMMAND, PCOMMAND> m_aLeave;    // Array holding all leave messages
	CArray<PCOMMAND, PCOMMAND> m_aPM;       // Array holding all PM commands
	CArray<ROOMDATA, ROOMDATA> m_aRooms;    // Array holding all rooms the bot is in
	
	// This maps half id names to Rawnames
	CMap<CString, LPCTSTR, CString, CString&> m_aHalfID;
	// This maps no-id names to Rawnames
	CMap<CString, LPCTSTR, CString, CString&> m_aNoID;

	COMMAND cRename;      // rename command
	COMMAND cAuto;        // auto command
	COMMAND cOnJoinRoom;  // called when bot is entering a room (even if bot is off)

	//CArray<PUSER_GROUP, PUSER_GROUP> m_aUserGroups;  // Array holding all usergroups
	CMap<CString, LPCTSTR, PUSER_GROUP, PUSER_GROUP> m_mUserGroups;
	
	int			 m_vWarnedCount[255];
	CMap<CString, LPCSTR, CString, CString&> m_mUserVar;
	
	int			 m_nExecError;
	ULONG		 m_nExecCode;

	DWORD	m_nBotStartTick; // Systeuptime in milliseconds when bot was started
	DWORD	m_nBotTic;        
	CString m_strBotTic; // for %USERVAR[n]% support.... :rolleyes:

	CIni    m_Ini;
	CEvent  m_eInitDone;
	CMenu   *m_pMenu;

	BOOL m_bUpdate;
	BOOL m_bBeep;

protected:
	void LoadToTray(HWND hWnd, UINT	 uCallbackMessage, CString sInfoTitle, CString sInfo, CString sTip,  int uTimeout, HICON icon, DWORD dwIcon);
	BOOL SetIconAndTitleForBalloonTip(CToolTipCtrl *pToolTipCtrl, int tti_ICON, CString title );
	CToolTipCtrl m_tToolTip;
	NOTIFYICONDATA m_nIconData;

	void OnEnterLeaveUser(DWORD dwID, PMX_USERINFO pUserInfo, BOOL bEnter = TRUE);
	// methods
	static UINT OutThread(PVOID pVoid);
	static UINT AsyncCmd(PVOID pVoid);
	static UINT StartupThread(PVOID pVoid);
	static UINT OnTimerThread(PVOID pVoid);
	// Load Settings from MXChat.ini
	void LoadSettings();

	// Load settings from a Metis command file
	BOOL ParseIniFile(CString strFile = "MXC.xml", BOOL bIncluded = FALSE);
	inline BOOL ParseCommand(TiXmlNode* pNode, PCOMMAND pCmd, CString strFilename, UINT nCmdType = CMD);

	// Input methods
	void Bot(DWORD dwID, CString strNickname, CString strMessage, BOOL bOp = FALSE);
	void RunCommand(PCOMMAND pCmd, DWORD dwID, CString strNickname, CString strMessage);

	// Output Helper functions

	// Print next message from bot queue
	void PrintNextMessage();

	// Print auto message
	void PrintAutoMessage();

	// Print help
	void PrintHelp(DWORD dwID);

	// Helper functions for variables
	CString GetMyDate(BOOL bLong);
	CString GetBotUptime();
	CString GetSystemUptime();
	CString GetInetBeats();
	CString GetMyLocalTime();
	CString GetRawName(DWORD dwID, CString strNickname);
	CString GetName(CString strNickname);
	CString GetWinampSong();
	void    ReplaceUservars(CString &rString);
	//void    SaveUserVar(int nIndex, CString strData);
	void    SaveUserVar(CString strKey, CString strData);
	CString TranslateIP(DWORD dwIP);
	void    ReplaceVars(CString&strString, CString strName, DWORD dwID);
	CString GetFiles(DWORD dwID, CString strName);
	CString GetLineType(DWORD dwID, CString strName);
	
	// Room management
	void		RemoveRoom(DWORD dwID);
	BOOL		AddRoom(DWORD dwID, CString strName, CUserMap* pUserMap, CString strServer, CString strVer);
	ROOMDATA&	GetRoom(DWORD dwID);
	CString		GetRoomName(DWORD dwID);
	//DWORD		GetID(HWND hChat);

	COLORREF    m_crBg;
	COLORREF    m_crOK;
	COLORREF    m_crErr;
	COLORREF    m_crPend;

	// Userright management
	BOOL	    GroupApplies(CString strGroup, CString strNick);
	PUSER_GROUP GetUserGroup(CString strGroup);
	BOOL        NickApplies(CString strGroup, CString strNick,  BOOL bCase);
	CString     GetNick(CString strName);
	BOOL        IsUserBlocked(CString strNickname);
	void	    RemoveWarnedUser(CString strUser);
	void	    IncreaseWarnedCount(CString strUser);
	int		    GetWarnedCount(CString strUser);
	BOOL     	IsUserWarned(CString strUser);

	// Other helperfunctions
	BOOL MatchesCondition(int nCondition, CString strValue, CString strRValue, DWORD dwID, CString strName, CString strIP, CString strParameter = "", CString strTrigger = "");
	BOOL ContainsStringExact(CString strLine, CString strSubString, BOOL bCase = TRUE, BOOL bExactMatch = FALSE);
	void MyShellExecute(CString strCmd, BOOL bWait = FALSE);
	void WriteMyEchoText(DWORD dwID, LPCTSTR strText, COLORREF tc = RGB(0, 150, 0));

public:
	CString GetIP(DWORD dwID, CString strName);
	CString GetHostname(DWORD dwID, CString strName);
	void SendPMTo(CString strUser, CString strMessage);
	CString ReadFromFile(CString strFile, CString strMode, CString strLine);
	BOOL WriteToFile(CString strFile, CString strMode, CString strData);
	void FinishRound(DWORD dwID, CString strWinner);
	//void FixColorName(CString& strName, BOOL bRobo);
	static BOOL GetExtWinampData(CString& rData);
	CString GetHalfName(CString strRawname);
	static const CString FormatInt(int nNUm);
	void ParseFinishErrors(void);
	BOOL HandleCommandOutput(DWORD dwID, PCOMMAND pCmd, const CString strNickname, const CString strMessage, TRIGGER t, const CString strParameter);
	CString ReadFromWeb(CString strURL, CString strMode, CString strLine);
	CString FormatIdleTime(DWORD dwTick);
	CString FormatStayTime(DWORD dwTick);
	CString FormatUserlevel(WORD wLevel);
};



#endif // !defined(AFX_PLUGIN_H__B188A380_735B_4FFE_BD75_3F5B00775250__INCLUDED_)
