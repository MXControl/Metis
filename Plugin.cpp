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

#include "stdafx.h"

#include <Shlwapi.h>
#include <winbase.h>
#include <afxinet.h>
#include <math.h>

#include "Plugin.h"
#include "wa_ipc.h"
#include "rmx_qapi.h"
#include "resource.h"
#include "MemMap.h"


#include "ErrorDlg.h"
#include "SpyDlg.h"

#include "tinyxml.h"
#include ".\plugin.h"
#include "Tokenizer.h"
#include <tchar.h>


#define ID_MENUBASE  40600

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

#ifndef NIF_INFO
# pragma message("==========================================================================")
# pragma message(" You need the lates Platform SDK to compile Metis.")
# pragma message(" You can download the SDK from")
# pragma message(" http://www.microsoft.com/msdownload/platformsdk/sdkupdate/.")
# pragma message("==========================================================================")
#endif

#ifdef _DEBUG
# define STR_VERSION     "2.83 (DEBUG)"
# define STR_VERSION_DLG "Metis 2.83 (DEBUG)"
# pragma message("==========================================================================")
# pragma message("= Compiling Metis DEBUG (VERSION 2.83)                                   =")
# pragma message("==========================================================================")
#else
# define STR_VERSION     "2.83"
# define STR_VERSION_DLG "Metis 2.83"
# pragma message("==========================================================================")
# pragma message("= Compiling Metis RELEASE (VERSION 2.83)                                 =")
# pragma message("==========================================================================")
#endif

BOOL g_bNewVersion = FALSE;
CString g_strWd;
BOOL g_bMini = FALSE;

#define WM_UPDATE_INFO		WM_USER+1

#define DEBUG_TRACE

const char* line_types[] = {"Unknown", "14.4K", "28.8K", "33.3K", "56K", 
                            "64K ISDN", "128K ISDN", "Cable", "DSL", "T1", "T3"};

#define NUM_LINES 11
#define MAX_LENGTH 2048

extern CMetis g_rPlugin;

// Constructor
CMetis::CMetis()
{

	m_nBotTic	 = 0;
	m_strBotname = _T("Anonymous000");
	m_strBadLan  = _T("");
	m_bBadWord   = TRUE;
	m_nBotTicRem = 0;
	m_nAutoKick  = 3;
	m_bAutoKick  = TRUE;
	m_bExclude   = FALSE;
	cAuto.nFlood = 0;
	m_nBlockTime = 0;
	m_bParam     = TRUE;
    m_bNoNick    = FALSE;

	m_nBlockRange        = 0;
	m_nBlockMessageCount = 0;
	m_strSeperator       = "._ ";
	m_bStopCmdThreads = FALSE;
	m_strEditor = "notepad.exe";

	m_nDefaultFlood = 60;
	m_nDefCmdType = CMD_TYPE_NORMAL;
	m_bMiniTray = TRUE;
	m_bUpdate = TRUE;
	m_bBeep = TRUE;
	m_bTrivia = FALSE;

	m_dwPMID = 0;

	m_strName.Format("Metis Chatbot %s", STR_VERSION);
	m_strAuthor		 = "Bender (Modified by Pri)";
	m_strDescription = "Metis Chatbot for WinMX";

	m_nRoundEnd			= -1;
	m_nRoundCurrent		= 0;
	m_strBotTic			= "0";
	m_crBg				= 0;
	m_crOK				= 0;
	m_crErr				= 0;
	m_crPend			= 0;
}

// destroctur
CMetis::~CMetis()
{

}

UINT CMetis::StartupThread(PVOID pVoid)
{

	ASSERT(pVoid);
	CMetis* pPlugin = (CMetis*)pVoid;
	
	// Wait until Metis left Initialize()
	WaitForSingleObject(pPlugin->m_eInitDone, INFINITE);
	pPlugin->m_eInitDone.ResetEvent();
	
	SleepEx(5000, TRUE);


#ifndef _DEBUG
	CInternetSession	is;
	CString				strTmp, strNewVersion;
	
	CString strURL;
 
	strURL = "http://metis.mxpulse.com/version.info";

	try{

		CHttpFile* pFile = (CHttpFile*) is.OpenURL(strURL);
		//some ISPs interfear with a proxy to display adds when the first page is loaded
		//so we close and opem the file again
		pFile->Close();
		pFile = 0;
		pFile = (CHttpFile*) is.OpenURL(strURL);
		
		if(pFile != NULL){

			pFile->ReadString(strNewVersion);
		}

		is.Close();
	}
	catch(CInternetException* pEx){
		
		TCHAR   szCause[255];
		CString strFormatted;

		pEx->GetErrorMessage(szCause, 255);
		strFormatted.Format("Error during Update Query: %s\n", szCause);
		TRACE(strFormatted);
	}

	if(strNewVersion != STR_VERSION_DLG){

		g_bNewVersion = TRUE;
		strTmp.Format("There is a new version of Metis available!\n\n -> You are running %s\n -> The newest version is %s\n\nVisit http://renmx.com to download the latest version",
			STR_VERSION_DLG, strNewVersion);
		pPlugin->DisplayToolTip(strTmp, 30, 1);
	}

	if(g_bNewVersion){

		int nTimer = 0;
		while(!pPlugin->m_bStopOut && (nTimer <= 30)){

			nTimer++;
			Sleep(1000);
		}
		g_bNewVersion = FALSE;
	}
#endif
	// Signal Startup Thread is finished
	pPlugin->m_eInitDone.SetEvent();

	return 0;
}

// Initialze plugin
void CMetis::Init(void)
{

	TCHAR szBuffer[_MAX_PATH]; 
	::GetModuleFileName(m_hInstance, szBuffer, _MAX_PATH);
	g_strWd.Format("%s", szBuffer);
	g_strWd = g_strWd.Left(g_strWd.ReverseFind('\\')) + "\\Plugins";

   	ClearRandom();
	LoadSettings();

	// Check if the RoboMX version is compatible with Metis
	DWORD dwRoboVer = (DWORD)QueryInformation(RMX_GETVERSION, 0, 0);

	if(dwRoboVer < 208){

		CString strError;
		strError.Format(_T("Fatal error!\n")
			            _T("Metis detected that you are running RoboMX %d.%02d\n")
						_T("but Metis requires RoboMX version 2.08 or newer to\n")
						_T("work properly. You can obtain a new RoboMX version\n")
						_T("from http://mxcontrol.sf.net.\n")
						_T("Metis will not load."),
						dwRoboVer / 100,
						dwRoboVer % 100);

		AfxMessageBox(strError, MB_ICONSTOP);
		
		m_eInitDone.SetEvent();
		m_eTrivia.SetEvent();
		m_eThreadFinished.SetEvent();
		m_eTimerThread.SetEvent();
	}
	else{
	
		BOOL bReturn = TRUE;

		m_aParseErrors.RemoveAll();

		bReturn &= ParseIniFile();
		for(int j = 0; j < m_aIncludeFiles.GetSize(); j++){

			bReturn &= ParseIniFile(m_aIncludeFiles[j], TRUE);
		}

		ValidateUsergroups();

		m_bStopOut = FALSE;
		
		// Initialize Random number generator
		srand(GetTickCount());
		
		DEBUG_TRACE("Firing off Startup Thread");
		m_eInitDone.ResetEvent();
		if(m_bUpdate){

			AfxBeginThread(StartupThread, (void*)this, THREAD_PRIORITY_NORMAL);
		}

		DEBUG_TRACE("Firing off Output- and Worker-Threads");
		m_eThreadFinished.ResetEvent();
		m_eTimerThread.ResetEvent();
		AfxBeginThread(OutThread, (void*)this, THREAD_PRIORITY_NORMAL);
		AfxBeginThread(OnTimerThread, (void*)this, THREAD_PRIORITY_NORMAL);

		m_nBotStartTick = GetTickCount();
		m_nBotTic = GetTickCount();
		
		m_eInitDone.SetEvent();
		m_eTrivia.SetEvent();

		ParseFinishErrors();

	} // End if dwRoboVer < 208
	
	// Query colors
	m_crBg				= (COLORREF)QueryInformation(RMX_CFG_COLOR_BG, 0, 0);
	m_crOK				= (COLORREF)QueryInformation(RMX_CFG_COLOR_OK, 0, 0);
	m_crErr				= (COLORREF)QueryInformation(RMX_CFG_COLOR_ERROR, 0, 0);
	m_crPend			= (COLORREF)QueryInformation(RMX_CFG_COLOR_PENDING, 0, 0);
	
	DEBUG_TRACE("Leaving call Initialize()"); 
}

void CMetis::OnReloadCfg()
{

	m_crBg				= (COLORREF)QueryInformation(RMX_CFG_COLOR_BG, 0, 0);
	m_crOK				= (COLORREF)QueryInformation(RMX_CFG_COLOR_OK, 0, 0);
	m_crErr				= (COLORREF)QueryInformation(RMX_CFG_COLOR_ERROR, 0, 0);
	m_crPend			= (COLORREF)QueryInformation(RMX_CFG_COLOR_PENDING, 0, 0);
}

// Terminate the plugin
void CMetis::Quit()
{

	// Halt all command threads
	m_bStopCmdThreads = TRUE;
	SleepEx(1000, FALSE);
	
	// stop output and trivia
	m_bStopOut = TRUE;
	m_bTrivia  = FALSE;

	WaitForSingleObject(m_eTrivia.m_hObject, INFINITE);
	WaitForSingleObject(m_eInitDone.m_hObject, INFINITE);
	WaitForSingleObject(m_eThreadFinished.m_hObject, INFINITE);
	WaitForSingleObject(m_eTimerThread.m_hObject, INFINITE);
	
	// Clean up
	m_mUserVar.RemoveAll();
	ClearRandom();
}

// Hook into all commands entered in the editline
void CMetis::OnInputHook(DWORD dwID, CString *pString)
{

	CString strCmd = *pString;

	strCmd.MakeLower();
	strCmd.TrimLeft();
	strCmd.TrimRight();

	// load config
	if(strCmd.Find("/mxc load", 0) >= 0)
	{

		strCmd.Replace("/mxc load", "");
		strCmd.TrimLeft();
		strCmd.TrimRight();
		
		m_bStopCmdThreads = TRUE;
		WriteEchoText(dwID, ">> Metis: Stopping all user threads. Please stand by...\n", m_crPend, m_crBg);
		SleepEx(1000, FALSE);

		if(m_bTrivia){

			m_bTrivia = FALSE;
			WaitForSingleObject(m_eTrivia, INFINITE);
			WriteEchoText(dwID, ">> Metis: Trivia stopped\n", m_crErr, m_crBg);
			ClearGame();
		}

		m_aIncludeFiles.SetSize(0);
		ClearRandom();
		LoadSettings();
		
		BOOL bResult = TRUE;
		m_aParseErrors.RemoveAll();
		if((bResult = ParseIniFile(strCmd.IsEmpty() ? "MXC.xml" : strCmd)) == TRUE){

			BOOL bResult2 = TRUE;
			CString strOut;
			strOut.Format("Loaded %s\n", (strCmd.IsEmpty() ? _T("MXC.xml") : strCmd));
			WriteMyEchoText(dwID, strOut, m_crPend);

			for(int i = 0; i < m_aIncludeFiles.GetSize(); i++){

				if((bResult2 = ParseIniFile(m_aIncludeFiles[i], TRUE)) == TRUE){

					strOut.Format("Loaded %s\n", m_aIncludeFiles[i]);
					WriteMyEchoText(dwID, strOut, m_crPend);
				}
				else{

					if(bResult2 == 2){
					
						strOut.Format(">> Metis: Warning in %s. Possible inconsistent commands detected...\n", m_aIncludeFiles[i]);
						WriteMyEchoText(dwID, strOut, m_crErr);
					}
					else{

						strOut.Format(">> Metis: Error in %s. Invalid syntax or file not found.\n", m_aIncludeFiles[i]);
						WriteMyEchoText(dwID, strOut, m_crErr);
					}
				}
			}
			
			ParseFinishErrors();
			WriteEchoText(dwID, ">> Metis: Configuration file reparsed\n", m_crOK, m_crBg);
		}
		else{

			if(bResult == 2){

				CString strOut;
				strOut.Format(">> Metis: Warning in \"%s\" Possible inconsistent commands detected...\n",(strCmd.IsEmpty() ? "MXC.xml" : strCmd));
				WriteMyEchoText(dwID, strOut, m_crErr);
			}
			else{

				CString strOut;
				strOut.Format(">> Metis: Error in \"%s\" Invalid syntax or file not found.\n\n",(strCmd.IsEmpty() ? "MXC.xml" : strCmd));
				WriteMyEchoText(dwID, strOut, m_crErr);
			}
		}

		if(!ValidateUsergroups()){

			WriteMyEchoText(dwID, ">> Metis: Warning usergroup inconsistency detected...\n", m_crErr);
		}
		
		pString->Empty();
		m_bStopCmdThreads = FALSE;
	}

	// edit config (manually)
	else if(strCmd.Find("/mxc edit", 0) >= 0)
	{

		strCmd.Replace("/mxc edit", "");
		strCmd.TrimLeft();
		strCmd.TrimRight();

		if(strCmd.IsEmpty())
			::ShellExecute(NULL, "open", m_strEditor,  "\"" + g_strWd + "\\MXC.xml\"", g_strWd, SW_SHOW);
		else
			::ShellExecute(NULL, "open", m_strEditor,  "\"" + g_strWd + "\\" + strCmd + "\"", g_strWd, SW_SHOW);
			

		WriteEchoText(dwID, ">> Metis: Type /mxc load when you are done editing the command file.\n", m_crPend, m_crBg);
		pString->Empty();
	}
	// print out the help message
	else if(strCmd == "/mxc stats"){

		 CString strStats;

		 WriteEchoText(dwID, "Statistics:\nConfiguration files loaded via <include .../>:\n", m_crOK, m_crBg);
		 for(int i = 0; i < m_aIncludeFiles.GetSize(); i++){

			 strStats.Format("+ %s\n", m_aIncludeFiles[i]);
			 WriteEchoText(dwID, strStats, m_crPend, m_crBg);
		 }
		 WriteEchoText(dwID, "End of include files.\n", m_crOK, m_crBg);
		 strStats.Format("Number of files loaded: %02d\n\n", m_aIncludeFiles.GetSize()+1);
		 WriteEchoText(dwID, strStats, m_crPend, m_crBg);
		
		 strStats.Format("Number of scripts:\n"
						 "+ <OnJoinRoom> %04d\n"
						 "+ <OnEnter> %04d\n"
						 "+ <OnLeave> %04d\n"
						 "+ <OnTimer> %04d\n"
						 "+ <OnRename> %04d\n"
						 "+ <OnOpMessage> %04d\n"
						 "+ <command> %04d\n"
						 "+ <OnPM> %04d\n"
						 "+ <badword> %04d\n"
						 "+ <trivia>  %04d\n",
						 cOnJoinRoom.aResponses.GetSize() ? 1 : 0,
						 m_aEnter.GetSize(),
						 m_aLeave.GetSize(),
						 cAuto.nFlood == 0 ? 0 : 1,
						 cRename.aResponses.GetSize() ? 1 : 0,
						 m_aOpCommands.GetSize(),
						 m_aCommands.GetSize(),
						 m_aPM.GetSize(),
						 m_aBadWords.GetSize(),
						 m_aTrivia.GetSize());
		 WriteEchoText(dwID, strStats, m_crPend, m_crBg);
		pString->Empty();
	}
	else if(strCmd == "/mxc spy"){

		CSpyDlg dlg;
		dlg.m_pVars = &m_mUserVar;

		dlg.DoModal();

		pString->Empty();
	}
	else if(strCmd == "/mxc help")
	{

		PrintHelp(dwID);
		pString->Empty();
	}
	else if(strCmd == "/mxc run"){
    
		if(GetRoom(dwID).bIsBotActivated){

			strCmd = "/mxc stop";
		}
		else{

			strCmd = "/mxc start";
		}
	}
	else if(strCmd == "/mxc trivia"){

		if(m_bTrivia == TRUE){

			strCmd = "/mxc trivia stop";
		}
		else{

			strCmd = "/mxc trivia start";
		}
	}
	
	if(strCmd == "/mxc start"){

		CString strOut;

		if(GetRoom(dwID).bIsBotActivated){

			strOut.Format(">> Metis: Bot already running in channel %s (0x%X)\n", GetRoom(dwID).strRoomName, dwID);
			WriteEchoText(dwID, strOut, m_crErr, m_crBg);
		}
		else{

			POSITION pos = GetRoom(dwID).pUserMap->GetStartPosition();
			CString strUser;
			MX_USERINFO user;
			while(pos){


				GetRoom(dwID).pUserMap->GetNextAssoc(pos, strUser, user);
				TRACE("Building name-map for %s\n", user.strUser);
				CString strTmp = GetHalfName(user.strUser);
				m_aHalfID.SetAt(strTmp, user.strUser);

				strTmp = GetName(user.strUser);
				m_aNoID.SetAt(strTmp, user.strUser);
			}

			GetRoom(dwID).bIsBotActivated = TRUE;
			strOut.Format(">> Metis: Bot activated in channel %s (0x%X)\n", GetRoom(dwID).strRoomName, dwID);
			WriteEchoText(dwID, strOut, m_crOK,  m_crBg);
			if(m_dwPMID == 0){
				
				m_dwPMID = dwID;
				strOut.Format(">> Metis: Accepting PM's in channel %s (0x%X)\n", GetRoom(dwID).strRoomName, dwID);
				WriteEchoText(dwID, strOut, m_crOK, m_crBg);
			} 
			if(m_bBeep){

				Beep(1500, 100);
				Beep(2000, 100);
				Beep (2800, 100);
			}
		}
		pString->Empty();
	}
	else if(strCmd == "/mxc stop"){

		CString strOut;
		if(!GetRoom(dwID).bIsBotActivated){

			strOut.Format(">> Metis: Bot is not  running in channel %s (0x%X)\n", GetRoom(dwID).strRoomName, dwID);
			WriteEchoText(dwID, strOut, m_crErr,  m_crBg);
		}
		else{

			GetRoom(dwID).bIsBotActivated = FALSE;
			strOut.Format(">> Metis: Bot stopped in channel %s (0x%X)\n", GetRoom(dwID).strRoomName, dwID);
			WriteEchoText(dwID, strOut, m_crOK,  m_crBg);
			if(m_dwPMID == dwID){
				
				m_dwPMID = 0;
				WriteEchoText(dwID, ">> Metis: No default room for PM's\n", m_crErr, m_crBg);
			} 
			if(m_bBeep){

				Beep(2800, 100);
				Beep(2000, 100);
				Beep(1500, 100);
			}
		}
		pString->Empty();
	}
	else if(strCmd == "/mxc trivia start"){

		CString strOut;
		if(!GetRoom(dwID).bIsBotActivated){

			strOut.Format(">> Metis: Bot is not  running in channel %s (0x%X)\nType \"/mxc start\" first\n", GetRoom(dwID).strRoomName, dwID);
			WriteEchoText(dwID, strOut, m_crErr,  m_crBg);
		}
		else{

			if(m_bTrivia){

				strOut.Format(">> Metis: Trivia is already running in channel %s (0x%X)\n", GetRoom(m_dwTriviaID).strRoomName, m_dwTriviaID);
	 			WriteEchoText(dwID, strOut, m_crErr,  m_crBg);
			}
			else{
				
				if(m_aTrivia.GetSize() == 0){
					
					WriteEchoText(dwID, ">> Metis: No questions for Trivia found... Please add questions first!\n", m_crErr, m_crBg);
					pString->Empty();
					return;
				}

				m_dwTriviaID = dwID;
				WriteEchoText(dwID, ">> Metis: Randomizing questions. Stand by...\n", m_crPend, m_crBg);
				InitTrivia();
				m_eTrivia.ResetEvent();
				m_bTrivia = TRUE;
				InputMessage(dwID, m_strTStart);
				AfxBeginThread(TriviaThread, (LPVOID)this, THREAD_PRIORITY_NORMAL);
				WriteEchoText(dwID, ">> Metis: Trivia started\n", m_crOK, m_crBg);
				WriteEchoText(dwID, ">> NOTE: A more advanced Trivia bot as the one integrated into Metis can be found on http://mxcontrol.sf.net\n", m_crOK, m_crBg);
			}
		}
		pString->Empty();
	}
	else if(strCmd == "/mxc trivia stop"){

		CString strOut;
		if(!GetRoom(dwID).bIsBotActivated){

			strOut.Format(">> Metis: Bot is not  running in channel %s (0x%X)\nType \"/mxc start\" first\n", GetRoom(dwID).strRoomName, dwID);
			WriteEchoText(dwID, strOut, m_crErr,  m_crBg);
		}
		else{

			if(m_bTrivia){

				if(m_dwTriviaID == dwID){

					m_dwTriviaID = 0;
					WriteEchoText(dwID, ">> Metis: Shutting down Trivia game. Stand by...\n", m_crPend, m_crBg);
					m_bTrivia = FALSE;
					WaitForSingleObject(m_eTrivia, INFINITE);
					ClearGame();
					WriteEchoText(dwID, ">> Metis: Trivia stopped\n", m_crOK, m_crBg);
				}
				else{

					strOut.Format(">> Metis: Trivia is not running in this channel but in %s (0x%X)\n", GetRoom(m_dwTriviaID).strRoomName, m_dwTriviaID);
	 				WriteEchoText(dwID, strOut, m_crErr, m_crBg);
				}
			}
			else{
				
				strOut.Format(">> Metis: Trivia is not running in channel %s (0x%X)\n", GetRoom(dwID).strRoomName, dwID);
	 			WriteEchoText(dwID, strOut, m_crErr,  m_crBg);
			}
		}
		pString->Empty();
	}
}

// Called when user enteres a channel
void CMetis::OnJoin(DWORD dwID, PMX_USERINFO pUserInfo)
{

	// Build Half and No-ID usernames :-)
	CString strTmp = GetHalfName(pUserInfo->strUser);
	m_aHalfID.SetAt(strTmp, pUserInfo->strUser);

	strTmp = GetName(pUserInfo->strUser);
	m_aNoID.SetAt(strTmp, pUserInfo->strUser);

	if(!m_aRooms.GetSize()) return;
	if(!GetRoom(dwID).bIsBotActivated) return;
	
	OnEnterLeaveUser(dwID, pUserInfo);
}


void CMetis::OnPrivateMsg(CString* pSender, CString* pMessage, DWORD dwNode, WORD wPort)
{

	if(!m_aRooms.GetSize()) return;

	CString strNickname = *pSender;
	CString strMessage  = *pMessage;
	*pSender = "";
	*pMessage = "";

	// Nickname is emtpy, ignore input :-P
	if(strNickname.IsEmpty()){
	
		DEBUG_TRACE("Leaving call Bot(). Reason: Nickname is empty.\n");
		return;
	}
	// Message is emtpy, ignore input :-P
	if(strMessage.IsEmpty()){
		
		DEBUG_TRACE("Leaving call Bot(). Reason: Message is empty.\n");
		return;
	}
	
	// if this is a message printed by the bot, ignore it still :-P
	if(strNickname == m_strBotname){
		
		DEBUG_TRACE("Leaving call Bot(). Reason: Message was posted with my nickname.\n");
		return;
	}

	CString strTmp, strTmp2;

	strNickname.Replace("*", "&lowast;");
	strNickname.Replace("$", "&sect;");
	strNickname.Replace("%", "&perc;");
	strMessage.Replace("*", "&lowast;");
	strMessage.Replace("$", "&sect;");
	strMessage.Replace("%", "&perc;");
    
	// loop through all <OnPM>
	for(int i = 0; i < m_aPM.GetSize(); i++){


		PCOMMAND pCmd = m_aPM.GetAt(i);
		if(pCmd->nMode == CMD_MODE_BLOCK){

			RunCommand(pCmd, m_dwPMID, strNickname, strMessage);
		}
		else{

			PEXT_CMD pExtCmd = new EXT_CMD;
			pExtCmd->pCmd = pCmd;
			pExtCmd->dwID = m_dwPMID;
			pExtCmd->strMessage = strMessage;
			pExtCmd->strNickname = strNickname;
			pExtCmd->pPlugin = this;
			AfxBeginThread(AsyncCmd, (LPVOID)pExtCmd, THREAD_PRIORITY_NORMAL, 0, 0, 0);
		}
	}
}

// Called on new message
void CMetis::OnMessage(DWORD dwID, CString* pUser, CString* pMsg, BOOL bIsAction)
{

	if(!m_aRooms.GetSize()) return;
	if(!GetRoom(dwID).bIsBotActivated) return;

	Bot(dwID, *pUser, *pMsg);
	if(m_bTrivia){

		Trivia(dwID, *pUser, *pMsg);
	}
}

void CMetis::OnOpMessage(DWORD dwID, CString* pUser, CString* pMsg)
{

	if(!m_aRooms.GetSize()) return;
	if(!GetRoom(dwID).bIsBotActivated) return;

	CString strMsg = *pMsg, strName = *pUser;
	if(strMsg.GetLength()){

		if(strMsg[0] == 0x03){

			strMsg = strMsg.Mid(2);
		}
	}
	if(strName.GetLength()){

		if(strName[0] == 0x03){

			int nEnd = strName.GetLength() - 6;
			if(nEnd > 0)
				strName = strName.Mid(3, nEnd);
		}
	}

	Bot(dwID, strName, strMsg, TRUE);

	if(m_bTrivia){

		Trivia(dwID, strName, strMsg);
	}
}

void CMetis::OnSetUserName(CString strNewUsername)
{

	CString strChange;
	strChange.Format("Botname changed from %s to %s\n", m_strBotname, strNewUsername);
	m_strBotname = strNewUsername;

	for(int i = 0; i < m_aRooms.GetSize(); i++){

		WriteEchoText(m_aRooms[i].dwID, strChange, m_crPend, m_crBg);
	}
}

// Called when a user changes name or node ip
void CMetis::OnRename(DWORD dwID, PMX_USERINFO pOld, PMX_USERINFO pNew)
{

	// Update Half and No-ID usernames :-)
	CString strTmp = GetHalfName(pOld->strUser);
	m_aHalfID.RemoveKey(strTmp);

	strTmp = GetName(pOld->strUser);
	m_aNoID.RemoveKey(strTmp);
	
	strTmp = GetHalfName(pNew->strUser);
	m_aHalfID.SetAt(strTmp, pNew->strUser);

	strTmp = GetName(pNew->strUser);
	m_aNoID.SetAt(strTmp, pNew->strUser);
	// End update half and no-id usernames :-)


	if(!m_aRooms.GetSize()) return;
	if(!GetRoom(dwID).bIsBotActivated || (cRename.nIn != -1)) return;

	CString strOldname = pOld->strUser, strNewname = pNew->strUser;

	if(GetHalfName(strOldname) == GetHalfName(strNewname)) return;

	strOldname.Replace("*", "&lowast;");
	strOldname.Replace("%", "&perc;");
	strOldname.Replace("$", "&sect;");
	strNewname.Replace("*", "&lowast;");
	strNewname.Replace("%", "&perc;");
	strNewname.Replace("$", "&sect;");
	

	if(!MatchesCondition(cRename.nCondition, cRename.strLValue, cRename.strRValue, dwID, strNewname, pNew->strRealIP)) return;
	if((cRename.nCmdType == CMD_TYPE_RANDOM) || (cRename.nCmdType == CMD_TYPE_NORMAL)){

		int nRes = (cRename.nCmdType == CMD_TYPE_RANDOM ? rand() % cRename.nNum : 0);
		if(!MatchesCondition(cRename.aResponses[nRes].nCondition, cRename.aResponses[nRes].strLValue, cRename.aResponses[nRes].strRValue,dwID, strNewname, pNew->strRealIP)) return;

		strTmp = cRename.aResponses[nRes].strResponse;

		strTmp.Replace("%OLDNAME%", GetName(strOldname));
		strTmp.Replace("%NEWNAME%", GetName(strNewname));
		strTmp.Replace("%OLDRAWNAME%", strOldname);
		strTmp.Replace("%NEWRAWNAME%", strNewname);
		strTmp.Replace("%IP%", pNew->strRealIP);
		
		if(cRename.aResponses[nRes].nCmdType == OUT_PUSH){

			if(cRename.aResponses[nRes].nOperator == -1){

				ReplaceVars(strTmp, strNewname, dwID);
				ReplaceUservars(strTmp);
				SaveUserVar(cRename.aResponses[nRes].strData, strTmp);
			}
			else{
				
				CString strRValue = cRename.aResponses[nRes].strRop;
				CString strLValue = cRename.aResponses[nRes].strLop;
				CString strNValue = cRename.aResponses[nRes].strNop;

				ReplaceVars(strNValue, strNewname, dwID);
				ReplaceUservars(strNValue);
				ReplaceVars(strLValue, strNewname, dwID);
				ReplaceUservars(strLValue);
				ReplaceVars(strRValue, strNewname, dwID);
				ReplaceUservars(strRValue);

				SaveUserVar(cRename.aResponses[nRes].strData, Operator(cRename.aResponses[nRes].nOperator, strLValue, strRValue, strNValue));
			}
		}
		else if(cRename.aResponses[nRes].nCmdType == OUT_POP){

			m_mUserVar.RemoveKey(cRename.aResponses[nRes].strData);
		}
		else if(cRename.aResponses[nRes].nCmdType == OUT_FILE){

			CString strData     = cRename.aResponses[nRes].strData;
			CString strMode     = cRename.aResponses[nRes].strMode;
			CString strResponse = strTmp;

			ReplaceVars(strData, strNewname, dwID);
			ReplaceUservars(strData);
			
			ReplaceVars(strMode, strNewname, dwID);
			ReplaceUservars(strMode);
			
			ReplaceVars(strResponse, strNewname, dwID);
			ReplaceUservars(strResponse);
			
			WriteToFile(strData, strMode, strResponse);
		}
		else{

			BOT_MESSAGE b;
			b.dwChannelID = dwID;
			b.strName     = strNewname;
			b.strTrigger  = strNewname;
			b.strResponse = strTmp;
			b.nDelay      = cRename.aResponses[nRes].nDelay;
			b.nData		  = cRename.aResponses[nRes].nData;
			b.nType       = cRename.aResponses[nRes].nCmdType;
			b.strData	  = cRename.aResponses[nRes].strData;
			ReplaceVars(b.strData, strNewname, dwID);
			ReplaceUservars(b.strData);
			
			ReplaceVars(b.strResponse, strNewname, dwID);
			ReplaceUservars(b.strResponse);

			m_qMessageQueue.push(b);
		}
	}
	else if(cRename.nCmdType == CMD_TYPE_SCRIPT){

		for(int k = 0; k < cRename.nNum; k++){

			if(!MatchesCondition(cRename.aResponses[k].nCondition, cRename.aResponses[k].strLValue, cRename.aResponses[k].strRValue,dwID, strNewname, pNew->strRealIP)) continue;
			strTmp = cRename.aResponses[k].strResponse;
			strTmp.Replace("%OLDNAME%", GetName(strOldname));
			strTmp.Replace("%NEWNAME%", GetName(strNewname));
			strTmp.Replace("%OLDRAWNAME%", strOldname);
			strTmp.Replace("%NEWRAWNAME%", strNewname);
			strTmp.Replace("%IP%", pNew->strRealIP);

			if(cRename.aResponses[k].nCmdType == OUT_BREAK){

				break;
			}
			else if(cRename.aResponses[k].nCmdType == OUT_GOTO){

				int nGoto = k + cRename.aResponses[k].nData;
				if((nGoto >= 0) && (nGoto < cRename.nNum)){

					k = nGoto-1; // make it one smaller, because the for statement will 
								 // increment it again
					continue;
				}
				else{

					WriteMyEchoText(dwID, "Error in Command. Goto statement out of bound\n");
					break;
				}
			}
			else if(cRename.aResponses[k].nCmdType == OUT_PUSH){

				if(cRename.aResponses[k].nOperator == -1){

					ReplaceVars(strTmp, strNewname, dwID);
					ReplaceUservars(strTmp);
					SaveUserVar(cRename.aResponses[k].strData, strTmp);
				}
				else{
					
					CString strRValue = cRename.aResponses[k].strRop;
					CString strLValue = cRename.aResponses[k].strLop;
					CString strNValue = cRename.aResponses[k].strNop;

					ReplaceVars(strNValue, strNewname, dwID);
					ReplaceUservars(strNValue);
					ReplaceVars(strLValue, strNewname, dwID);
					ReplaceUservars(strLValue);
					ReplaceVars(strRValue, strNewname, dwID);
					ReplaceUservars(strRValue);

					SaveUserVar(cRename.aResponses[k].strData, Operator(cRename.aResponses[k].nOperator, strLValue, strRValue, strNValue));
				}
			}
			else if(cRename.aResponses[k].nCmdType == OUT_POP){

				m_mUserVar.RemoveKey(cRename.aResponses[k].strData);
			}
			else if(cRename.aResponses[k].nCmdType == OUT_FILE){

				CString strData     = cRename.aResponses[k].strData;
				CString strMode     = cRename.aResponses[k].strMode;
				CString strResponse = strTmp;

				ReplaceVars(strData, strNewname, dwID);
				ReplaceUservars(strData);
				
				ReplaceVars(strMode, strNewname, dwID);
				ReplaceUservars(strMode);
				
				ReplaceVars(strResponse, strNewname, dwID);
				ReplaceUservars(strResponse);
				
				WriteToFile(strData, strMode, strResponse);
			}
			else{

				BOT_MESSAGE b; // sigh, structs need a constructor :P
				b.dwChannelID = dwID;
				b.strName     = strNewname;
				b.strTrigger  = strNewname;
				b.strResponse = strTmp;
				b.nDelay      = cRename.aResponses[k].nDelay;
				b.nData		  = cRename.aResponses[k].nData;
				b.nType       = cRename.aResponses[k].nCmdType;
				b.strData	  = cRename.aResponses[k].strData;

				ReplaceVars(b.strData, strNewname, dwID);
				ReplaceUservars(b.strData);
				
				ReplaceVars(b.strResponse, strNewname, dwID);
				ReplaceUservars(b.strResponse);

				m_qMessageQueue.push(b);
			}
		}
	}
}

// called when a user enters a channel
void CMetis::OnJoinChannel(DWORD dwID, CString strChannel, CUserMap* pUserMap, CString strServerString, CString strServerVersion)
{

	TRACE("ROOM ID = %X\n", dwID);

	BOOL bExists = AddRoom(dwID, strChannel, pUserMap, strServerString, strServerVersion);

	POSITION pos = pUserMap->GetStartPosition();
	CString strUser;
	MX_USERINFO user;
	while(pos){


		pUserMap->GetNextAssoc(pos, strUser, user);
		TRACE("Building name-map for %s\n", user.strUser);
		CString strTmp = GetHalfName(user.strUser);
		m_aHalfID.SetAt(strTmp, user.strUser);

		strTmp = GetName(user.strUser);
		m_aNoID.SetAt(strTmp, user.strUser);
	}

	LoadSettings();
	PrintHelp(dwID);
	
	// Execute on Join Room Command
	if(!m_aRooms.GetSize()) return;
	if(cOnJoinRoom.nIn != -1) return;

	CString strTmp;
	
	if(!MatchesCondition(cOnJoinRoom.nCondition, cOnJoinRoom.strLValue, cOnJoinRoom.strRValue, dwID, "onjoinroom", "")) return;
	if((cOnJoinRoom.nCmdType == CMD_TYPE_RANDOM) || (cOnJoinRoom.nCmdType == CMD_TYPE_NORMAL)){

		int nRes = (cOnJoinRoom.nCmdType == CMD_TYPE_RANDOM ? rand() % cOnJoinRoom.nNum : 0);
		if(!MatchesCondition(cOnJoinRoom.aResponses[nRes].nCondition, cOnJoinRoom.aResponses[nRes].strLValue, cOnJoinRoom.aResponses[nRes].strRValue,dwID, "onjoinroom", "")) return;

		strTmp = cOnJoinRoom.aResponses[nRes].strResponse;
		
		if(cOnJoinRoom.aResponses[nRes].nCmdType == OUT_PUSH){

			if(cOnJoinRoom.aResponses[nRes].nOperator == -1){

				ReplaceVars(strTmp, "onjoinroom", dwID);
				ReplaceUservars(strTmp);
				SaveUserVar(cOnJoinRoom.aResponses[nRes].strData, strTmp);
			}
			else{
				
				CString strRValue = cOnJoinRoom.aResponses[nRes].strRop;
				CString strLValue = cOnJoinRoom.aResponses[nRes].strLop;
				CString strNValue = cOnJoinRoom.aResponses[nRes].strNop;

				ReplaceVars(strNValue, "onjoinroom", dwID);
				ReplaceUservars(strNValue);
				ReplaceVars(strLValue, "onjoinroom", dwID);
				ReplaceUservars(strLValue);
				ReplaceVars(strRValue, "onjoinroom", dwID);
				ReplaceUservars(strRValue);

				SaveUserVar(cOnJoinRoom.aResponses[nRes].strData, Operator(cOnJoinRoom.aResponses[nRes].nOperator, strLValue, strRValue, strNValue));
			}
		}
		else if(cOnJoinRoom.aResponses[nRes].nCmdType == OUT_POP){

			m_mUserVar.RemoveKey(cOnJoinRoom.aResponses[nRes].strData);
		}
		else if(cOnJoinRoom.aResponses[nRes].nCmdType == OUT_FILE){

			CString strData     = cOnJoinRoom.aResponses[nRes].strData;
			CString strMode     = cOnJoinRoom.aResponses[nRes].strMode;
			CString strResponse = strTmp;

			ReplaceVars(strData, "onjoinroom", dwID);
			ReplaceUservars(strData);
			
			ReplaceVars(strMode, "onjoinroom", dwID);
			ReplaceUservars(strMode);
			
			ReplaceVars(strResponse, "onjoinroom", dwID);
			ReplaceUservars(strResponse);
			
			WriteToFile(strData, strMode, strResponse);
		}
		else{

			BOT_MESSAGE b;
			b.dwChannelID = dwID;
			b.strName     = "onjoinroom";
			b.strTrigger  = "onjoinroom";
			b.strResponse = strTmp;
			b.nDelay      = cOnJoinRoom.aResponses[nRes].nDelay;
			b.nData		  = cOnJoinRoom.aResponses[nRes].nData;
			b.nType       = cOnJoinRoom.aResponses[nRes].nCmdType;
			b.strData	  = cOnJoinRoom.aResponses[nRes].strData;
			
			ReplaceVars(b.strData, b.strName, dwID);
			ReplaceUservars(b.strData);
			
			ReplaceVars(b.strResponse, b.strName, dwID);
			ReplaceUservars(b.strResponse);

			m_qMessageQueue.push(b);
		}
	}
	else if(cOnJoinRoom.nCmdType == CMD_TYPE_SCRIPT){

		for(int k = 0; k < cOnJoinRoom.nNum; k++){

			if(!MatchesCondition(cOnJoinRoom.aResponses[k].nCondition, cOnJoinRoom.aResponses[k].strLValue, cOnJoinRoom.aResponses[k].strRValue,dwID, "onjoinroom", "")) continue;
			strTmp = cOnJoinRoom.aResponses[k].strResponse;

			if(cOnJoinRoom.aResponses[k].nCmdType == OUT_BREAK){

				break;
			}
			else if(cOnJoinRoom.aResponses[k].nCmdType == OUT_GOTO){

				int nGoto = k + cOnJoinRoom.aResponses[k].nData;
				if((nGoto >= 0) && (nGoto < cOnJoinRoom.nNum)){

					k = nGoto-1; // make it one smaller, because the for statement will 
								 // increment it again
					continue;
				}
				else{

					WriteMyEchoText(dwID, "Error in Command. Goto statement out of bound\n");
					break;
				}
			}
			else if(cOnJoinRoom.aResponses[k].nCmdType == OUT_PUSH){

				if(cOnJoinRoom.aResponses[k].nOperator == -1){

					ReplaceVars(strTmp, "onjoinroom", dwID);
					ReplaceUservars(strTmp);
					SaveUserVar(cOnJoinRoom.aResponses[k].strData, strTmp);
				}
				else{
					
					CString strRValue = cOnJoinRoom.aResponses[k].strRop;
					CString strLValue = cOnJoinRoom.aResponses[k].strLop;
					CString strNValue = cOnJoinRoom.aResponses[k].strNop;

					ReplaceVars(strNValue, "onjoinroom", dwID);
					ReplaceUservars(strNValue);
					ReplaceVars(strLValue, "onjoinroom", dwID);
					ReplaceUservars(strLValue);
					ReplaceVars(strRValue, "onjoinroom", dwID);
					ReplaceUservars(strRValue);

					SaveUserVar(cOnJoinRoom.aResponses[k].strData, Operator(cOnJoinRoom.aResponses[k].nOperator, strLValue, strRValue, strNValue));
				}
			}
			else if(cOnJoinRoom.aResponses[k].nCmdType == OUT_POP){

				m_mUserVar.RemoveKey(cOnJoinRoom.aResponses[k].strData);
			}
			else if(cOnJoinRoom.aResponses[k].nCmdType == OUT_FILE){

				CString strData     = cOnJoinRoom.aResponses[k].strData;
				CString strMode     = cOnJoinRoom.aResponses[k].strMode;
				CString strResponse = strTmp;

				ReplaceVars(strData, "onjoinroom", dwID);
				ReplaceUservars(strData);
				
				ReplaceVars(strMode, "onjoinroom", dwID);
				ReplaceUservars(strMode);
				
				ReplaceVars(strResponse, "onjoinroom", dwID);
				ReplaceUservars(strResponse);
				
				WriteToFile(strData, strMode, strResponse);
			}
			else{

				BOT_MESSAGE b;
				b.dwChannelID = dwID;
				b.strName     = "onjoinroom";
				b.strTrigger  = "onjoinroom";
				b.strResponse = strTmp;
				b.nDelay      = cOnJoinRoom.aResponses[k].nDelay;
				b.nData		  = cOnJoinRoom.aResponses[k].nData;
				b.nType       = cOnJoinRoom.aResponses[k].nCmdType;
				b.strData	  = cOnJoinRoom.aResponses[k].strData;

				ReplaceVars(b.strData, b.strName, dwID);
				ReplaceUservars(b.strData);
				
				ReplaceVars(b.strResponse, b.strName, dwID);
				ReplaceUservars(b.strResponse);

				m_qMessageQueue.push(b);
			}
		}
	}
}

void CMetis::OnRenameChannel(DWORD dwID, CString strNewname)
{

	if(!m_aRooms.GetSize()) return;

	// Update channelname
	GetRoom(dwID).strRoomName = strNewname;
}

// called when a user leaves the channel
void CMetis::OnPart(DWORD dwID, PMX_USERINFO pUserInfo)
{

	CString strTmp = GetHalfName(pUserInfo->strUser);
	m_aHalfID.RemoveKey(strTmp);

	strTmp = GetName(pUserInfo->strUser);
	m_aNoID.RemoveKey(strTmp);

	if(!m_aRooms.GetSize()) return;
	if(!GetRoom(dwID).bIsBotActivated) return;
		
	OnEnterLeaveUser(dwID, pUserInfo, FALSE);
}

// called when the Bot leaves a channel
void CMetis::OnLeaveChannel(DWORD dwID)
{

	RemoveRoom(dwID);
}

//////////////////////////////////////////////////////////////////////////////////
//  
//  Bot management functions
//  start
//

// load settings from MXChat.ini
void CMetis::LoadSettings()
{

	CString strParent = g_strWd.Left(g_strWd.ReverseFind('\\'));

	m_Ini.SetIniFileName(strParent + "\\RoboMX.ini");
	m_strBotname = m_Ini.GetValue("UserInfo", "Nickname", "Metis.2.8_000_12345");
	m_crBg = m_Ini.GetValue("Colors", "Background", RGB(255,255,255));
}

// Parse a command from the given xml node
// <command></command> <-- this node is passed as pNode
BOOL CMetis::ParseCommand(TiXmlNode* pNode, PCOMMAND pCmd, CString strFilename, UINT nCmdType)
{

	CString strCmdType;
	switch(nCmdType){

		case RENAME: 
			strCmdType = "<OnRename>";
			break;
		case TIMER:
			strCmdType = "<OnTimer>";
			break;
		case LEAVE: 
			strCmdType = "<OnPart>";
			break;
		case ENTER: 
			strCmdType = "<OnEnter>";
			break;
		case JOIN: 
			strCmdType = "<OnJoinRoom>";
			break;
		case CMD: 
			strCmdType = "<command>";
			break;
		case OP:
			strCmdType = "<OnOpMessage>";
			break;
		case PM:
			strCmdType = "<OnPM>";
			break;
		default:
			strCmdType = "unknown";
	}

	BOOL bReturn			= TRUE;
	TiXmlNode* childNode	= NULL;
	CString strTmp;

	pCmd->strFile = strFilename;
	pCmd->nLine = pNode->Row();

	strTmp = pNode->ToElement()->Attribute("type");
	strTmp.MakeLower();
	if(strTmp == "random") pCmd->nCmdType = CMD_TYPE_RANDOM;
	else if(strTmp == "script") pCmd->nCmdType = CMD_TYPE_SCRIPT;
	else if(strTmp == "normal") pCmd->nCmdType = CMD_TYPE_NORMAL;
	else pCmd->nCmdType = m_nDefCmdType;

	pNode->ToElement()->Attribute("case", &pCmd->bCase);

	strTmp = pNode->ToElement()->Attribute("condition");
	if((strTmp == "!=")  || (strTmp == "0")) pCmd->nCondition = CON_NOTEQUAL;
	else if((strTmp == "==")  || (strTmp == "1")) pCmd->nCondition = CON_EQUAL;
	else if((strTmp == "<")   || (strTmp == "2")) pCmd->nCondition = CON_SMALLER;
	else if((strTmp == ">")   || (strTmp == "3")) pCmd->nCondition = CON_GREATER;
	else if((strTmp == "?")   || (strTmp == "4")) pCmd->nCondition = CON_FIND;
	else if((strTmp == "!?")  || (strTmp == "5")) pCmd->nCondition = CON_NOTFIND;
	else if((strTmp == "_?")  || (strTmp == "6")) pCmd->nCondition = CON_FINDNC;
	else if((strTmp == "!_?") || (strTmp == "7")) pCmd->nCondition = CON_NOTFINDNC;
	else if((strTmp == "||") || (strTmp == "8")) pCmd->nCondition = CON_OR;
	else if((strTmp == "&&") || (strTmp == "9")) pCmd->nCondition = CON_AND;
	else if((strTmp == "digit") || (strTmp == "10")) pCmd->nCondition = CON_ISNUM;
	else pCmd->nCondition = CON_DONTUSE;

	if((pCmd->nCondition < CON_DONTUSE) || (pCmd->nCondition > CON_MAX)) pCmd->nCondition = CON_DONTUSE;
	
	pCmd->strLValue = pNode->ToElement()->Attribute("lvalue");
	pCmd->strRValue = pNode->ToElement()->Attribute("rvalue");

	pCmd->strGroup  = pNode->ToElement()->Attribute("usergroup");
	pCmd->strNames  = pNode->ToElement()->Attribute("users");
	pCmd->strNames.Replace(";" , ";|;");
	pCmd->strNames.Replace("lowast;", "&lowast;");
	pCmd->strNames.Replace("sect;", "&sect;");
	pCmd->strNames.Replace("perc;", "&perc;");
	pCmd->strNames.Replace("amp;", "&amp;");
	// fix special charaters
	pCmd->strNames.Replace("$", "&sect;");
	pCmd->strNames.Replace("%", "&perct;");


	pCmd->nLast = 0;

	if(nCmdType == TIMER){

		m_strBotTic = pNode->ToElement()->Attribute("interval");
	}

	// The following attributes and nodes are exclusive to
	// <command> <OnOpMessage> and <OnPM>
	if(nCmdType >= CMD){

		// flood protection
		pNode->ToElement()->Attribute("flood", &pCmd->nFlood);

		// mode (blocking vs thread)
		strTmp = pNode->ToElement()->Attribute("mode");
		if(strTmp == "thread") pCmd->nMode = CMD_MODE_THREAD;
		else pCmd->nMode = CMD_MODE_BLOCK;

		// get all triggers
		while((nCmdType >= CMD) && ((childNode = pNode->IterateChildren("in", childNode)) != NULL)){

			TRIGGER t;
			t.strNames = childNode->ToElement()->Attribute("users");
			t.strNames.Replace(";" , ";|;");
			t.strNames.Replace("lowast;", "&lowast;");
			t.strNames.Replace("sect;", "&sect;");
			t.strNames.Replace("perc;", "&perc;");
			t.strNames.Replace("amp;", "&amp;");
			// fix special charaters
			t.strNames.Replace("$", "&sect;");
			t.strNames.Replace("%", "&perct;");

			t.strGroup = childNode->ToElement()->Attribute("usergroup");

			childNode->ToElement()->Attribute("case", &t.bCase);
			
			strTmp = childNode->ToElement()->Attribute("type");
			if(strTmp.CompareNoCase("exact") == 0){

				t.bExact = TRUE;
			}
			else{

				t.bExact = FALSE;
			}

			strTmp = childNode->ToElement()->Attribute("condition");
			if((strTmp == "!=")  || (strTmp == "0")) t.nCondition = CON_NOTEQUAL;
			else if((strTmp == "==")  || (strTmp == "1")) t.nCondition = CON_EQUAL;
			else if((strTmp == "<")   || (strTmp == "2")) t.nCondition = CON_SMALLER;
			else if((strTmp == ">")   || (strTmp == "3")) t.nCondition = CON_GREATER;
			else if((strTmp == "?")   || (strTmp == "4")) t.nCondition = CON_FIND;
			else if((strTmp == "!?")  || (strTmp == "5")) t.nCondition = CON_NOTFIND;
			else if((strTmp == "_?")  || (strTmp == "6")) t.nCondition = CON_FINDNC;
			else if((strTmp == "!_?") || (strTmp == "7")) t.nCondition = CON_NOTFINDNC;
			else if((strTmp == "||") || (strTmp == "8")) t.nCondition = CON_OR;
			else if((strTmp == "&&") || (strTmp == "9")) t.nCondition = CON_AND;
			else if((strTmp == "digit") || (strTmp == "10")) t.nCondition = CON_ISNUM;
			else t.nCondition = CON_DONTUSE;

			if((t.nCondition < CON_DONTUSE) || (t.nCondition > CON_MAX)) t.nCondition = CON_DONTUSE;
			
			t.strLValue = childNode->ToElement()->Attribute("lvalue");
			t.strRValue = childNode->ToElement()->Attribute("rvalue");

			if(childNode->FirstChild() != NULL){

				t.strTrigger = childNode->FirstChild()->ToText()->Value();
				t.strTrigger.Replace("lowast;", "&lowast;");
				t.strTrigger.Replace("sect;", "&sect;");
				t.strTrigger.Replace("perc;", "&perc;");
				t.strTrigger.Replace("amp;", "&amp;");
			}
			pCmd->aTrigger.Add(t);

		}
	}
	else{

		pCmd->nFlood = 0;
		pCmd->nMode = CMD_MODE_BLOCK;
	}
	
	

	// Get all outputs
	while((childNode = pNode->IterateChildren("out", childNode)) != NULL){

		RESPONSE r;
		
		childNode->ToElement()->Attribute("delay", &r.nDelay);

		strTmp = childNode->ToElement()->Attribute("type");
		if(strTmp == "exec") r.nCmdType = OUT_EXEC;
		else if(strTmp == "control") r.nCmdType = OUT_BOT;
		else if(strTmp == "push")    r.nCmdType = OUT_PUSH;
		else if(strTmp == "pop")     r.nCmdType = OUT_POP;
		else if(strTmp == "break")   r.nCmdType = OUT_BREAK;
		else if(strTmp == "goto")    r.nCmdType = OUT_GOTO;
		else if(strTmp == "sleep")   r.nCmdType = OUT_SLEEP;
		else if(strTmp == "echo")    r.nCmdType = OUT_ECHO;
		else if(strTmp == "tooltip") r.nCmdType = OUT_TOOLTIP;
		else if(strTmp == "channeltool") r.nCmdType = OUT_CHANNELTOOL;
		else if(strTmp == "pm") r.nCmdType = OUT_PM;
		else if(strTmp == "file") r.nCmdType = OUT_FILE;
		else if(strTmp == "self") r.nCmdType = OUT_TRIGGER;
		else r.nCmdType = OUT_NORMAL;
		
		r.strMode = childNode->ToElement()->Attribute("mode");

		if((r.nCmdType != OUT_PM) && (r.nCmdType != OUT_FILE) && (r.nCmdType != OUT_POP) && (r.nCmdType != OUT_PUSH)){

			childNode->ToElement()->Attribute("extdata", &r.nData);
		}
		else{

			r.strData = childNode->ToElement()->Attribute("extdata");
		}

		strTmp = childNode->ToElement()->Attribute("condition");
		if((strTmp == "!=")  || (strTmp == "0")) r.nCondition = CON_NOTEQUAL;
		else if((strTmp == "==")  || (strTmp == "1")) r.nCondition = CON_EQUAL;
		else if((strTmp == "<")   || (strTmp == "2")) r.nCondition = CON_SMALLER;
		else if((strTmp == ">")   || (strTmp == "3")) r.nCondition = CON_GREATER;
		else if((strTmp == "?")   || (strTmp == "4")) r.nCondition = CON_FIND;
		else if((strTmp == "!?")  || (strTmp == "5")) r.nCondition = CON_NOTFIND;
		else if((strTmp == "_?")  || (strTmp == "6")) r.nCondition = CON_FINDNC;
		else if((strTmp == "!_?") || (strTmp == "7")) r.nCondition = CON_NOTFINDNC;
		else if((strTmp == "||") || (strTmp == "8"))  r.nCondition = CON_OR;
		else if((strTmp == "&&") || (strTmp == "9"))  r.nCondition = CON_AND;
		else if((strTmp == "digit") || (strTmp == "10")) r.nCondition = CON_ISNUM;
		else r.nCondition = CON_DONTUSE;

		if((r.nCondition < CON_DONTUSE) || (r.nCondition > CON_MAX)) r.nCondition = CON_DONTUSE;

		r.strLValue = childNode->ToElement()->Attribute("lvalue");
		r.strRValue = childNode->ToElement()->Attribute("rvalue");

		TiXmlNode* opNode = childNode->FirstChild("operator");
		if(opNode != NULL){

			strTmp = opNode->ToElement()->Attribute("type");
			if(strTmp == "-") r.nOperator = OP_MINUS;
			else if(strTmp == "+") r.nOperator = OP_PLUS;
			else if(strTmp == "--") r.nOperator = OP_MINUSMINUS;
			else if(strTmp == "++") r.nOperator = OP_PLUSPLUS;
			else if(strTmp == "*") r.nOperator = OP_TIMES;
			else if(strTmp == "/") r.nOperator = OP_DIVIDE;
			else if(strTmp == "%") r.nOperator = OP_MODULO;
			else if(strTmp == "^") r.nOperator = OP_POWER;
			else if(strTmp == "||") r.nOperator = OP_OR;
			else if(strTmp == "aa") r.nOperator = OP_AND;
			else if(strTmp == "!") r.nOperator = OP_NOT;
			else if(strTmp == "<") r.nOperator = OP_SMALLER;
			else if(strTmp == "==") r.nOperator = OP_EQUAL;
			else if(strTmp == "!=") r.nOperator = OP_NOTEQUAL;
			else if(strTmp == ">") r.nOperator = OP_GREATER;
			else if(strTmp == "strlen") r.nOperator = OP_STR_LEN;
			else if(strTmp == "strtrm") r.nOperator = OP_STR_TRIMM;
			else if(strTmp == "strlwr") r.nOperator = OP_STR_LOWER;
			else if(strTmp == "strupr") r.nOperator = OP_STR_UPPER;
			else if(strTmp == "strleft") r.nOperator = OP_STR_LEFT;
			else if(strTmp == "strright") r.nOperator= OP_STR_RIGHT;
			else if(strTmp == "strfind") r.nOperator= OP_STR_FIND;
			else if(strTmp == "strrfind") r.nOperator= OP_STR_REVERSE_FIND;
			else if(strTmp == "strcat") r.nOperator= OP_STR_APPEND;
			else if(strTmp == "strrem") r.nOperator= OP_STR_REMOVE;
			else if(strTmp == "strrep") r.nOperator = OP_STR_REPLACE;
			else if(strTmp == "readfile") r.nOperator = OP_READ_FILE;
			else if(strTmp == "readweb") r.nOperator = OP_READ_WEB;
			else{
			
				CString strWarning;
				strWarning.Format("Warning in %s near line %d:\nUnknown operator type %s\nWill default to +",
					strFilename, pCmd->nLine, strTmp);
				//DisplayToolTip(strWarning, 1, NIIF_WARNING);
				m_aParseErrors.Add(strWarning);
				r.nOperator = OP_PLUS; // +
			}

			r.strLop = opNode->ToElement()->Attribute("lvalue");
			r.strRop = opNode->ToElement()->Attribute("rvalue");
			r.strNop = opNode->ToElement()->Attribute("nvalue");
		}
		else{

			TiXmlNode *rNode = childNode->FirstChild();
			if(rNode != NULL){

				r.strResponse = rNode->ToText()->Value();
			}
			r.nOperator = -1;
		}
		pCmd->aResponses.Add(r);
	}
	pCmd->nIn  = (int)pCmd->aTrigger.GetSize();
	pCmd->nNum = (int)pCmd->aResponses.GetSize();
	if((nCmdType >= CMD) && (pCmd->nIn == 0)){

		CString strError;
		strError.Format("Error:  A %s in file %s  line %d does not have any <in> tags!\n", strCmdType, strFilename, pCmd->nLine);
		//AfxMessageBox(strError, MB_ICONEXCLAMATION);
		m_aParseErrors.Add(strError);
	}
	if(pCmd->nNum == 0){

		CString strError;
		strError.Format("Error:  A %s in file %s line %d does not have any <out> tags!\n", strCmdType, strFilename, pCmd->nLine);
		//AfxMessageBox(strError, MB_ICONEXCLAMATION);
		m_aParseErrors.Add(strError);
	}
	else{

		if(!ValidateCommand(strFilename, pCmd, nCmdType)){

			bReturn = 2;
		}
	}

	return bReturn;
}

// load commads and settings from a metis xml file
BOOL CMetis::ParseIniFile(CString strFilename, BOOL bIncluded)
{
	
	BOOL bReturn = TRUE;
	CString strIniFile = g_strWd + "\\" + strFilename;
	CString strTmp;

	TiXmlDocument doc(strIniFile);
	
	if(doc.LoadFile() == false){

		CString strError;
		if(doc.ErrorRow() == 0){

			strError.Format("Error in %s: %s", strIniFile, doc.ErrorDesc());
		}
		else{

			strError.Format("Error in %s line %d: %s", strFilename, doc.ErrorRow(), doc.ErrorDesc());
		}
		//AfxMessageBox(strError, MB_ICONSTOP);
		m_aParseErrors.Add(strError);
		return FALSE;
	}

	//int i;
	//XNodes childs, subchilds;
	TiXmlNode* mainNode = 0;
	//mainNode = doc.FirstChild("config");
	
	if(doc.FirstChild("config") == NULL){

		CString strError;
		strError.Format("Fatal Error! File %s misses <config> ... </config> supernode!",
			strFilename);
		m_aParseErrors.Add(strError);
		return FALSE;
	}

	while((mainNode = doc.IterateChildren("config", mainNode)) != NULL){

		TiXmlNode* subNode = 0;
		TiXmlNode* childNode = 0;
		std::string strBuffer;

		if(mainNode == NULL) return FALSE;
		
		if(!bIncluded){

			///////////////////////////////////
			// Get basic settings
			///////////////////////////////////

			// Beep system speakers
			subNode = mainNode->FirstChild("EnableBeep");
			if(subNode != 0){

				subNode->ToElement()->Attribute("value", &m_bBeep);
				if((m_bBeep > 1) || (m_bBeep < 0)) m_bBeep = TRUE;
			}
			else m_bBeep = TRUE;

			// check for updates on startup
			subNode = mainNode->FirstChild("EnableUpdateCheck");
			if(subNode != 0){

				subNode->ToElement()->Attribute("value", &m_bUpdate);
				if((m_bUpdate > 1) || (m_bUpdate < 0)) m_bUpdate = TRUE;
			}
			else m_bUpdate = TRUE;

			// Default time to wait until a command can be triggered again
			subNode = mainNode->FirstChild("DefaultCmdFloodProt");
			if(subNode != 0){

				subNode->ToElement()->Attribute("value", &m_nDefaultFlood);
				if((m_nDefaultFlood > 1) || (m_nDefaultFlood < 0)) m_nDefaultFlood = 60;
			}
			else m_nDefaultFlood = 60;

			// Default command type
			subNode = mainNode->FirstChild("DefaultCmdType");
			if(subNode != 0){

				strBuffer = subNode->ToElement()->Attribute("value");

				if(strBuffer.compare("normal") == 0) m_nDefCmdType = CMD_TYPE_NORMAL;
				else if(strBuffer.compare("script") == 0) m_nDefCmdType = CMD_TYPE_SCRIPT;
				else if(strBuffer.compare("random") == 0) m_nDefCmdType = CMD_TYPE_RANDOM;
				else m_nDefCmdType = CMD_TYPE_NORMAL;

			}
			else m_nDefCmdType = CMD_TYPE_NORMAL;

			// AutoKick
			subNode = mainNode->FirstChild("AutoKick");
			if(subNode == 0){
				
				// due to error in old documentation and examples
				// try this also
				subNode = mainNode->FirstChild("Autokick");      
			}

			if(subNode != 0){

				subNode->ToElement()->Attribute("enable", &m_bAutoKick);
				if((m_bAutoKick < 0) || (m_bAutoKick > 1)) m_bAutoKick = FALSE;

				subNode->ToElement()->Attribute("warning", &m_nAutoKick);
				if(m_nAutoKick < 0) m_nAutoKick = 3;

				subNode->ToElement()->Attribute("warnings", &m_nAutoKick);
				if(m_nAutoKick < 0) m_nAutoKick = 3;
			}
			else{
				
				m_bAutoKick = FALSE;
				m_nAutoKick = 3;
			}

			// Badword Penalty
			subNode = mainNode->FirstChild("BadwordPenalty");
			if(subNode != 0){
	            
				subNode->ToElement()->Attribute("enable", &m_bBadWord);
				if((m_bBadWord < 0) || (m_bBadWord > 1)) m_bBadWord = 0;

				subNode->ToElement()->Attribute("exclude", &m_bExclude);
				if((m_bExclude < 0) || (m_bExclude > 1)) m_bExclude = 0;
			}
			else{

				m_bBadWord = FALSE;
				m_bExclude = FALSE;
			}

			// Bot flood control
			subNode = mainNode->FirstChild("BotFloodControl");
			if(subNode != 0){

				subNode->ToElement()->Attribute("value", &m_nFlood);
				if(m_nFlood < 0) m_nFlood = 2000;
			}
			else m_nFlood = 2000;

			// User level flood control
			subNode = mainNode->FirstChild("UserFloodControl");
			if(subNode != 0){
	            
				subNode->ToElement()->Attribute("BanTime", &m_nBlockTime);
				if(m_bBadWord < 0) m_nBlockTime = 240*1000;

				subNode->ToElement()->Attribute("TimeFrame", &m_nBlockRange);
				if(m_nBlockRange < 0) m_nBlockRange = 60*1000;

				subNode->ToElement()->Attribute("MaxTriggerPerFrame", &m_nBlockMessageCount);
				if(m_nBlockMessageCount < 0)m_nBlockMessageCount = 3;
			}
			else{

				m_nBlockRange = 240*1000;
				m_nBlockMessageCount = 3;
				m_nBlockRange = 60*1000;
			}

			// nick seperator
			subNode = mainNode->FirstChild("NickSeperator");
			if(subNode != 0){

				m_strSeperator = subNode->ToElement()->Attribute("value");


			}
			else m_strSeperator = "._ ";

			subNode = mainNode->FirstChild(_T("BadLangWarning"));
			if(subNode != 0){

				m_strBadLan = subNode->ToElement()->Attribute("value");
			}
			else{

				m_strBadLan = "I'd watch that tone of voice, %NAME%, if I was you.";
			}

			subNode = mainNode->FirstChild(_T("BadLangPreKick"));
			if(subNode != 0){

				m_strPreKick = subNode->ToElement()->Attribute("value");
			}
			else{

				m_strPreKick = "You have been warned, %NAME%. Final words are not permitted. Bye.";
			}

			subNode = mainNode->FirstChild(_T("RedirectCommand"));
			if(subNode != 0){

				m_strBadLanRedirect = subNode->ToElement()->Attribute("value");
			}
			else{

				m_strBadLanRedirect = "/kickban %RAWNAME% 15";
			}
			
			subNode = mainNode->FirstChild(_T("ConfigEditor"));
			if(subNode != 0){

				m_strEditor = subNode->ToElement()->Attribute("value");
				if(m_strEditor.IsEmpty()){

					m_strEditor = "notepad.exe";
				}
			}
			else{

				m_strEditor = "notepad.exe";
			}

			subNode = mainNode->FirstChild(_T("SecureParam"));
			if(subNode != 0){

				subNode->ToElement()->Attribute("value", &m_bParam);
				if(m_bParam < 0) m_bParam = TRUE;
			}
			else{

				m_bParam = TRUE;
			}
			
			subNode = mainNode->FirstChild(_T("SecureNickname"));
			if(subNode != 0){

				subNode->ToElement()->Attribute("value", &m_bNoNick);
				if(m_bNoNick < 0) m_bNoNick = FALSE;
			}
			else{

				m_bParam = FALSE;
			}

			
			// Deprecated
			subNode = mainNode->FirstChild(_T("MaxUserVars"));
			if(subNode != 0){

				DisplayToolTip("Deprecated Element <MaxUserVars> found. This element is no longer required and can be removed from your MXC.xml", 10000, NIF_INFO);
			}

			// NEW 2.1 Trivia Strings :-)
			subNode = mainNode->FirstChild(_T("TriviaStartMessage"));
			if(subNode != 0){

				m_strTStart = subNode->ToElement()->Attribute("value");
			}
			else{

				m_strTStart = "Trivia Commands: SCORE - Display userscore, RANKING - Display top 3 ranking, SKIP - Skip question (Admins only), RESET [Score Limit] - Reset Trivia game. [Score Limit] points needed to win a round. (Admins only)";
			}

			subNode = mainNode->FirstChild(_T("TriviaScoreMsg"));
			if(subNode != 0){

				m_strTScore = subNode->ToElement()->Attribute("value");
			}
			else{

				m_strTScore = "User score: %SCORE% points";
			}

			subNode = mainNode->FirstChild(_T("TriviaNoAnswer"));
			if(subNode != 0){

				m_strTNoAnswer = subNode->ToElement()->Attribute("value");
			}
			else{

				m_strTNoAnswer = "Question \"%QUES%\" was not answered.  The corrrect answer is \"%ANS%\"";
			}

			subNode = mainNode->FirstChild(_T("TriviaCorrect"));
			if(subNode != 0){

				m_strTCorrect = subNode->ToElement()->Attribute("value");
			}
			else{

				m_strTCorrect = "%NAME% answered correct (%ANS%)";
			}

			subNode = mainNode->FirstChild(_T("TriviaSkip"));
			if(subNode != 0){

				m_strTSkip = subNode->ToElement()->Attribute("value");
			}
			else{

				m_strTSkip = "question skipped";
			}

			subNode = mainNode->FirstChild(_T("TriviaNext"));
			if(subNode != 0){

				m_strTNext = subNode->ToElement()->Attribute("value");
			}
			else{

				m_strTNext = "Trivia : %QUES%";
			}

			subNode = mainNode->FirstChild(_T("TriviaDefaultTime"));
			if(subNode != 0){

				subNode->ToElement()->Attribute("value", (int*)&m_nTriviaTime);
				if(m_nTriviaTime <= 0) m_nTriviaTime = 30;
			}
			else{

				m_nTriviaTime = 30;
			}
		}

		
		/////////////////////////////////
		// Get all include files
		////////////////////////////////
		subNode = 0;
		while((subNode = mainNode->IterateChildren("include", subNode)) != NULL){

			CString strInc = subNode->ToElement()->Attribute("file");
			
			if(m_aIncludeFiles.GetSize() == 0){

				m_aIncludeFiles.Add(strInc);
				continue;
			}

			// Dont include file twice ;)
			for(int j = 0; j < m_aIncludeFiles.GetSize(); j++){

				if(m_aIncludeFiles[j] == strInc) break;
			}
			if(j >= m_aIncludeFiles.GetSize()){

				m_aIncludeFiles.Add(strInc);
			}

		}

		/////////////////////////////////
		// Due to an error in the old 
		// help files also Include
		////////////////////////////////
		subNode = 0;
		while((subNode = mainNode->IterateChildren("Include", subNode)) != NULL){

			CString strInc = subNode->ToElement()->Attribute("file");
			
			if(m_aIncludeFiles.GetSize() == 0){

				m_aIncludeFiles.Add(strInc);
				continue;
			}

			// Dont include file twice ;)
			for(int j = 0; j < m_aIncludeFiles.GetSize(); j++){

				if(m_aIncludeFiles[j] == strInc) break;
			}
			if(j >= m_aIncludeFiles.GetSize()){

				m_aIncludeFiles.Add(strInc);
			}

		}
		  
		/////////////////////////////////
		// Get all usergroups if any
		////////////////////////////////
		subNode = 0;
		while((subNode = mainNode->IterateChildren("usergroup", subNode)) != NULL){

			PUSER_GROUP pUg = new USER_GROUP;

			pUg->strGroup = subNode->ToElement()->Attribute("name");
			if(pUg->strGroup.IsEmpty()){

				CString strError;
				strError.Format("Error in file %s\nUsergroup name must not be empty!\n\nCorrect format is <usergroup name=\"groupname\">\n", 
					strFilename);
				//AfxMessageBox(strError, MB_ICONEXCLAMATION);
				m_aParseErrors.Add(strError);
				delete pUg;
				continue;
			}
			if(pUg->strGroup.Find("!", 0) == 0){

				CString strError;
				strError.Format("Error in file %s\nUsergroup name must not contain exclamation mark. ! is a reserved character for the logical operator NOT (!, i.e. to exclude a usergroup from a command)!\n\nCorrect format is <usergroup name=\"groupname\">\n",
					strFilename);
				//AfxMessageBox(strError, MB_ICONEXCLAMATION);
				m_aParseErrors.Add(strError);
				delete pUg;
				continue;
			}
			if(GetUserGroup(pUg->strGroup) != NULL){

				PUSER_GROUP temp = GetUserGroup(pUg->strGroup);
				CString strError;
				strError.Format("Error in file %s\nUsergroup with name %s already defined in file %s line %d. This occurance will be ignored.\n",
					strFilename, temp->strFile, temp->nLine);
				//AfxMessageBox(strError, MB_ICONEXCLAMATION);
				m_aParseErrors.Add(strError);
				delete pUg;
				continue;
			}

			subNode->ToElement()->Attribute("case", &pUg->bCase);
			if(pUg->bCase < 0) pUg->bCase = FALSE;
			else if(pUg->bCase > 0) pUg->bCase = TRUE;

			pUg->strFile = strFilename;
			pUg->nLine   = subNode->Row();

			CString strTmp;
			if(subNode->FirstChild() != NULL){
				
				strTmp = subNode->FirstChild()->ToText()->Value();
			}

			// fix broken entities (& removed by tinyxml)
			strTmp.Replace("lowast;", "&lowast;");
			strTmp.Replace("sect;", "&sect;");
			strTmp.Replace("perc;", "&perc;");
			strTmp.Replace("amp;", "&amp;");
			// fix special charaters
			strTmp.Replace("$", "&sect;");
			strTmp.Replace("%", "&perct;");

			// parsing help :P
 			strTmp.Replace(" ", ";|;");

			pUg->strUsers = strTmp;
			m_mUserGroups.SetAt(pUg->strGroup, pUg);
		}
				 
		/////////////////////////////////
		// Get badwords
		////////////////////////////////
		subNode = 0;
		while((subNode = mainNode->IterateChildren("badword", subNode)) != NULL){

			PBADWORD pB = new BADWORD;
			pB->strNames  = subNode->ToElement()->Attribute("users");
			pB->strNames.Replace(";" , ";|;");
 			// fix broken entities (& removed by tinyxml)
			pB->strNames.Replace("lowast;", "&lowast;");
			pB->strNames.Replace("sect;", "&sect;");
			pB->strNames.Replace("perc;", "&perc;");
			// fix special charaters
			pB->strNames.Replace("$", "&sect;");
			pB->strNames.Replace("%", "&perct;");

			pB->strGroup = subNode->ToElement()->Attribute("usergroup");
			subNode->ToElement()->Attribute("case", &pB->bCase);
			if(pB->bCase < 0) pB->bCase = FALSE;

			strTmp = subNode->ToElement()->Attribute("condition");
			if((strTmp == "!=")  || (strTmp == "0")) pB->nCondition = CON_NOTEQUAL;
			else if((strTmp == "==")  || (strTmp == "1")) pB->nCondition = CON_EQUAL;
			else if((strTmp == "<")   || (strTmp == "2")) pB->nCondition = CON_SMALLER;
			else if((strTmp == ">")   || (strTmp == "3")) pB->nCondition = CON_GREATER;
			else if((strTmp == "?")   || (strTmp == "4")) pB->nCondition = CON_FIND;
			else if((strTmp == "!?")  || (strTmp == "5")) pB->nCondition = CON_NOTFIND;
			else if((strTmp == "_?")  || (strTmp == "6")) pB->nCondition = CON_FINDNC;
			else if((strTmp == "!_?") || (strTmp == "7")) pB->nCondition = CON_NOTFINDNC;
			else pB->nCondition = CON_DONTUSE;

			if((pB->nCondition < CON_DONTUSE) || (pB->nCondition > CON_MAX)) pB->nCondition = CON_DONTUSE;

			pB->strLValue = subNode->ToElement()->Attribute("lvalue");
			pB->strRValue = subNode->ToElement()->Attribute("rvalue");

			if(subNode->FirstChild() != NULL){

				strTmp = subNode->FirstChild()->ToText()->Value();
			}
			pB->strTrigger = strTmp;

			m_aBadWords.Add(pB);
		}
			

		/////////////////////////////////
		// Get OnJoinRoom
		////////////////////////////////
		subNode = 0;
		subNode = mainNode->FirstChild(_T("OnJoinRoom"));
		if(subNode != NULL)
		{

			bReturn = ParseCommand(subNode, &cOnJoinRoom, strFilename, JOIN);
			cOnJoinRoom.nIn  = -1;
		}
		else{

			if(!bIncluded){
				cOnJoinRoom.nIn = -1;
				cOnJoinRoom.nNum = -1;

			}
		}

		/////////////////////////////////
		// Get OnRename
		////////////////////////////////
		subNode = 0;
		subNode = mainNode->FirstChild(_T("OnRename"));
		if(subNode != NULL)
		{


			bReturn = ParseCommand(subNode, &cRename, strFilename, RENAME);
			cRename.nFlood	= 0;
			cRename.nLast	= 0;
			cRename.nIn		= -1;
		}
		else{

			if(!bIncluded){
				cRename.nIn = -1;
				cRename.nNum = -1;
			}
		}

				
		/////////////////////////////////
		// Get OnTimer
		////////////////////////////////
		subNode = 0;
		subNode = mainNode->FirstChild(_T("OnTimer"));
		if(subNode != NULL)
		{

			bReturn = ParseCommand(subNode, &cAuto, strFilename, TIMER);

			cAuto.nIn  = -1;
		}
		
		COMMAND* cmd = NULL;
		subNode = 0;

		/////////////////////////////////
		// Get all on enter commands
		////////////////////////////////
		while((subNode = mainNode->IterateChildren("OnEnter", subNode)) != NULL){

			cmd = new COMMAND;

			bReturn = ParseCommand(subNode, cmd, strFilename, ENTER);
			m_aEnter.Add(cmd);
		}
				
		subNode = 0;
		/////////////////////////////////
		// Get all on leave commands
		////////////////////////////////
		while((subNode = mainNode->IterateChildren("OnLeave", subNode)) != NULL){
	  		
			cmd = new COMMAND;
			bReturn = ParseCommand(subNode, cmd, strFilename, LEAVE);
			m_aLeave.Add(cmd);
		}
			   
		subNode = 0;
		/////////////////////////////////
		// Get all other commands
		////////////////////////////////
		while((subNode = mainNode->IterateChildren("command", subNode)) != NULL){

			cmd = new COMMAND;
			bReturn = ParseCommand(subNode, cmd, strFilename, CMD);
			m_aCommands.Add(cmd);
		}
		
		subNode = 0;
		/////////////////////////////////
		// Get all PM commands
		////////////////////////////////
		while((subNode = mainNode->IterateChildren("OnPM", subNode)) != NULL){

			cmd = new COMMAND;
			bReturn = ParseCommand(subNode, cmd, strFilename, PM);
			m_aPM.Add(cmd);
		}
		
		subNode = 0;
		/////////////////////////////////
		// Get all operator commands
		////////////////////////////////
		while((subNode = mainNode->IterateChildren("OnOpMessage", subNode)) != NULL){

			cmd = new COMMAND;
			bReturn = ParseCommand(subNode, cmd, strFilename, OP);
			m_aOpCommands.Add(cmd);
		}

		subNode = 0;
		/////////////////////////////////////////////////////
		// Get All Trivia questions
		////////////////////////////////////////////////////
		while((subNode = mainNode->IterateChildren("Trivia", subNode)) != NULL){

			PTRIVIA pT = new TRIVIA;
			
			subNode->ToElement()->Attribute("score", (int*)&pT->nPoints);
			if(pT->nPoints <= 0) pT->nPoints = 1;

			subNode->ToElement()->Attribute("time", (int*)&pT->nTime);
			if(pT->nTime <= 0) pT->nTime = m_nTriviaTime;

			childNode = subNode->FirstChild("q");

			if(childNode != NULL){

				if(childNode->FirstChild() != NULL){

					pT->strTrigger = childNode->FirstChild()->ToText()->Value();
				}
			}
			else{

				CString strError;
				strError.Format("Error:  A <Trivia> in file %s line %d does not have any <q> tags!\n", strFilename, subNode->Row());
				AfxMessageBox(strError, MB_ICONEXCLAMATION);
				continue;
			}

			while((childNode = subNode->IterateChildren("a", childNode)) != NULL){

				if(childNode->FirstChild() != NULL){
		
					pT->aResponse.Add(childNode->FirstChild()->ToText()->Value());
				}
			}
			if(pT->aResponse.GetSize() == 0){

				CString strError;
				strError.Format("Error:  A <Trivia> in file %s line %d does not have any <a> tags!\n", strFilename, subNode->Row());
				AfxMessageBox(strError, MB_ICONEXCLAMATION);
				continue;
			}
			m_aTrivia.Add(pT);
		}					  
	}// end iterate throug <config>

	return bReturn;
}

// Thread for trivia game :-)
UINT CMetis::TriviaThread(PVOID pParam)
{

	CMetis *pPlugin = (CMetis*)pParam;
	
	int nStartTic = 0;
	CString strTmp;

	while(pPlugin->m_bTrivia){

		// No current question so pop ask the next one :-)
		if(pPlugin->m_pCurrent == NULL){

			// take care it does not loop out of scope!
			if(pPlugin->m_nCurrent >= pPlugin->m_aGame.GetSize()){

				pPlugin->m_nCurrent = 0;
			}
			// Get next question
			pPlugin->m_pCurrent = pPlugin->m_aGame.GetAt(pPlugin->m_nCurrent++);

			BOT_MESSAGE b;
			b.nType = OUT_NORMAL;
			b.nData = 0;
			b.nDelay = pPlugin->m_nFlood;
			b.dwChannelID = pPlugin->m_dwTriviaID;
			b.strTrigger = "trivia";
			b.strName = pPlugin->m_strBotname;
			b.strResponse = pPlugin->m_strTNext;
			b.strResponse.Replace("%QUES%", pPlugin->m_pCurrent->strTrigger);
			strTmp.Format("%d", pPlugin->m_pCurrent->nTime);
			b.strResponse.Replace("%TIME%",	strTmp);
			strTmp.Format("%d", pPlugin->m_pCurrent->nPoints);
			b.strResponse.Replace("%POINTS%", strTmp);

			pPlugin->ReplaceVars(b.strData, b.strName, pPlugin->m_dwTriviaID);
			pPlugin->ReplaceUservars(b.strData);
			
			pPlugin->ReplaceVars(b.strResponse, b.strName, pPlugin->m_dwTriviaID);
			pPlugin->ReplaceUservars(b.strResponse);

			pPlugin->m_qMessageQueue.push(b);

			nStartTic = GetTickCount();
		}
		else if((GetTickCount() - nStartTic) > pPlugin->m_pCurrent->nTime*1000){

			//ran out of time...
			BOT_MESSAGE b;
			b.nType = OUT_NORMAL;
			b.nDelay = pPlugin->m_nFlood;
			b.nData = 0;
			b.strResponse = pPlugin->m_strTNoAnswer;
			b.strResponse.Replace("%ANS%", pPlugin->m_pCurrent->aResponse[0]);
			b.strResponse.Replace("%QUES%", pPlugin->m_pCurrent->strTrigger);
			strTmp.Format("%d", pPlugin->m_pCurrent->nTime);
			b.strResponse.Replace("%TIME%",	strTmp);
			strTmp.Format("%d", pPlugin->m_pCurrent->nPoints);
			b.strResponse.Replace("%POINTS%", strTmp);
			
			pPlugin->m_pCurrent = NULL;

			b.dwChannelID = pPlugin->m_dwTriviaID;
			b.strTrigger = "trivia";
			b.strName = pPlugin->m_strBotname;

			pPlugin->ReplaceVars(b.strData, b.strName,  pPlugin->m_dwTriviaID);
			pPlugin->ReplaceUservars(b.strData);
			
			pPlugin->ReplaceVars(b.strResponse, b.strName,  pPlugin->m_dwTriviaID);
			pPlugin->ReplaceUservars(b.strResponse);

			pPlugin->m_qMessageQueue.push(b);

		}
		Sleep(100);
	}
	pPlugin->m_eTrivia.SetEvent();
	return 0;
}

UINT CMetis::OnTimerThread(PVOID pParams)
{

	CMetis *pPlugin = (CMetis*)pParams;

	TRACE("Entering OnTimerThread...\n");

	register unsigned int dwSeconds  = 0;
	register unsigned int dwInterval = 0;

	while(!pPlugin->m_bStopOut){

		dwSeconds+=100;
		pPlugin->UpdateAutoMessageInterval(&dwInterval);

		if((dwSeconds >= dwInterval) && dwInterval != 0){

			pPlugin->PrintAutoMessage();
			dwSeconds = 0;
		}

		SleepEx(100, FALSE);
	}

	pPlugin->m_eTimerThread.SetEvent();
	TRACE("Leaving OnTimer Thread...\n");
	return 0;
}

// Main output thread for output queue handling
UINT CMetis::OutThread(PVOID pParams)
{

	CMetis *pPlugin = (CMetis*)pParams;
	CString strTmp;
	int nSleep = 20;
	while(!pPlugin->m_bStopOut){

		if(pPlugin->m_qMessageQueue.empty()){

			SleepEx(nSleep, FALSE);
		}
		else{

			SleepEx(pPlugin->m_qMessageQueue.front().nDelay, FALSE);

		}
		pPlugin->PrintNextMessage();
	}
	
	pPlugin->m_eThreadFinished.SetEvent();
	
	TRACE("OutThread Exited\n");
	return 0;
}



// Print next botmessage from output queue
void CMetis::PrintNextMessage()
{

	if(m_qMessageQueue.empty()){

		return;
	}
	BOT_MESSAGE b = m_qMessageQueue.front();
	m_qMessageQueue.pop();

	DEBUG_TRACE("In Call PrintNextMessage() [ChannelID=%d]", b.dwChannelID);
	DEBUG_TRACE(" -- Type=%d, Output=%s", b.nType, b.strResponse);
	DEBUG_TRACE(" -- Remaining messages in queue: %d\n", m_qMessageQueue.size());
	
	//ReplaceUservars(b.strResponse);
	b.strName.Replace("&lowast;", "*");
	b.strName.Replace("&amp;", "\"");
	b.strName.Replace("&perc;", "%");
	b.strName.Replace("&sect;", "$");
	b.strName.Replace("lowast;", "*");
	b.strName.Replace("amp;", "\"");
	b.strName.Replace("perc;", "%");
	b.strName.Replace("sect;", "$");
	b.strResponse.Replace("%TRIGGER%", b.strTrigger);
	b.strResponse.Replace("&lowast;", "*");
	b.strResponse.Replace("&perc;", "%");
	b.strResponse.Replace("&amp;", "\"");
	b.strResponse.Replace("&sect;", "$");
	b.strResponse.Replace("lowast;", "*");
	b.strResponse.Replace("perc;", "%");
	b.strResponse.Replace("amp;", "\"");
	b.strResponse.Replace("sect;", "$");

	//ReplaceVars(b.strResponse, b.strName, b.dwChannelID);

	TRACE("> %s\n", b.strResponse);

	switch(b.nType){

	case OUT_EXEC:
		MyShellExecute(b.strResponse, b.nData);
		break;
	case OUT_BOT:
		m_strLast = b.strResponse;
		OnInputHook(b.dwChannelID, &b.strResponse);
		break;
	case OUT_NORMAL:
		if(b.strResponse.GetLength() > 400){

			//b.strResponse = b.strResponse.Left(400);
			b.strResponse.Truncate(400);
		}
		m_strLast = b.strResponse;
		InputMyMessage(b.dwChannelID, b.strResponse);
		break;
	case OUT_ECHO:
		if(b.strResponse.GetLength() > 1000){

			//b.strResponse = b.strResponse.Left(400);
			b.strResponse.Truncate(1000);
		}
		b.strResponse.Replace("\\n", "\n");
		WriteMyEchoText(b.dwChannelID, b.strResponse);
		break;
	case OUT_TOOLTIP:
		b.strResponse.Replace("\\n", "\n");
		DisplayToolTip(b.strResponse, 1, 1);
		break;
	case OUT_CHANNELTOOL:
		b.strResponse.Replace("\\n", "\n");
		DisplayChannelToolTip(b.dwChannelID, b.strResponse);
		break;
	case OUT_PM:
		//ReplaceUservars(b.strData);
		//ReplaceVars(b.strData, b.strName, m_dwPMID);
 		if(b.strResponse.GetLength() > 400){

			//b.strResponse = b.strResponse.Left(400);
			b.strResponse.Truncate(400);
		}
		m_strLast = b.strResponse;
		SendPMTo(b.strData, b.strResponse);
		break;
	case OUT_TRIGGER:
		//ReplaceUservars(b.strData);
		//ReplaceVars(b.strResponse, b.strName, m_dwPMID);
		Bot(m_dwPMID, "_BOT_SELF_TRIGGER_", b.strResponse, FALSE);
		break;
	default:
		ASSERT(FALSE);
	}
}

// save data in a uservariable
//void CMetis::SaveUserVar(int nIndex, CString strData)
void CMetis::SaveUserVar(CString strKey, CString strData)
{

	strData.Replace("*", "&lowast;");
	strData.Replace("%", "&perc;");
	strData.Replace("$", "&sect;");
	m_mUserVar.SetAt(strKey, strData);
}

// Get Time in internetbeats
CString CMetis::GetInetBeats()
{

	// one beat = 1 minute and 26.4 seconds
	//          = 86.4 seconds
	CString strBeats;
	int nBeats = 0; 
	SYSTEMTIME time;

	GetSystemTime(&time);
	if(time.wHour < 23)	
		time.wHour+=1;
	else
		time.wHour=0;

	
	nBeats = time.wHour*60*60 + time.wMinute*60 + time.wSecond;
	nBeats = (int)(nBeats / 86.4);

	strBeats.Format("@%03d", nBeats);
	return strBeats;
}

CString CMetis::GetMyDate(BOOL bLong)
{

	CString strDate;
	SYSTEMTIME time;

	int n = GetLocaleInfo(LOCALE_SYSTEM_DEFAULT, bLong? LOCALE_SLONGDATE : LOCALE_SSHORTDATE, 0, 0);

	if(n != 0){

		char *szFormat = new char[n];
		ZeroMemory(szFormat, n);

		n = GetLocaleInfo(LOCALE_SYSTEM_DEFAULT, bLong? LOCALE_SLONGDATE : LOCALE_SSHORTDATE, szFormat, n);

		GetLocalTime(&time);
		
		n = GetDateFormat(LOCALE_SYSTEM_DEFAULT, 0, &time, szFormat, 0, 0); 
		
		if(n != 0){

			char *szDate = new char[n];
			ZeroMemory(szDate, n);

			n = GetDateFormat(LOCALE_SYSTEM_DEFAULT, 0, &time, szFormat, szDate, n);
			
			strDate = szDate;
			
			delete szDate;
			szDate   = 0;
		}

		delete szFormat;
		szFormat = 0;
	}
	return strDate;
}
// Get Localtime
CString CMetis::GetMyLocalTime()
{

	CString strTime;
	SYSTEMTIME time;
		
	GetLocalTime(&time);
	

	strTime.Format("%02d:%02d.%02d", time.wHour, time.wMinute, time.wSecond);

	return strTime;
}

// Get Systemuptime
CString CMetis::GetSystemUptime()
{

	CString strUptime;
	DWORD nMS = GetTickCount();
	int nSec = nMS / 1000;
	int nMin = nMS / 60000;
	int nHour = nMS / 3600000;

	strUptime.Format("%02d:%02d:%02d.%03d", nHour, nMin - nHour*60, nSec - nMin*60, nMS-nSec*1000);

	return strUptime;
}

// Get bot running time
CString CMetis::GetBotUptime()
{

	CString strUptime;
	DWORD nMS = GetTickCount() - m_nBotStartTick;
	int nSec = nMS / 1000;
	int nMin = nMS / 60000;
	int nHour = nMS / 3600000;

	strUptime.Format("%02d:%02d:%02d.%03d", nHour, nMin - nHour*60, nSec - nMin*60, nMS-nSec*1000);

	return strUptime;
}

// GEt song currently playing in winamp 2.x/5.x
CString CMetis::GetWinampSong()
{

	CString strWinamp = "Winamp is not active";
	HWND hwndWinamp = ::FindWindow("Winamp v1.x",NULL);
	if(hwndWinamp != NULL){

		TCHAR *buff = new TCHAR[250];
		::GetWindowText(hwndWinamp, buff, 250);
		strWinamp = buff;
		strWinamp = strWinamp.Mid(strWinamp.Find(" ", 0)+1, strWinamp.Find(" - Winamp") - strWinamp.Find(" ", 0)-1);
		delete buff;
		buff = NULL;
	}

	return strWinamp;
}

CString CMetis::GetHalfName(CString strRawname)
{

	CString strName;
	int nLen = strRawname.GetLength();
	// Bender's.coffee.|_|D..000_50583
	if((nLen-6) <= 2){

		strName = strRawname;
	}
	else{

		if(strRawname[nLen-6] == '_'){

			strName = strRawname.Left(nLen-6);
		}
		else{

			strName = strRawname;
		}
	}
	return strName;
}

// GetName returns the Name with the 9 digit userid removed
CString CMetis::GetName(CString strRawname)
{
	
	CString strName;
	int nLen = strRawname.GetLength();
	// Bender's.coffee.|_|D..000_50583
	if((nLen-9) <= 2){

		strName = strRawname;
	}
	else{

		if(strRawname[nLen-6] == '_'){

			strName = strRawname.Left(nLen-9);
		}
		else{

			strName = strRawname;
		}
	}
	return strName;
}


CString CMetis::GetIP(DWORD dwID, CString strName)
{

	CString strIP;

	CUserMap* pUserMap = GetRoom(dwID).pUserMap;
	
	if(pUserMap == NULL){
		
		ASSERT(FALSE);
		return strIP;
	}


	//for(int i = 0; i < pUserMap->GetSize(); i++){
	MX_USERINFO user;
	if(pUserMap->Lookup(strName, user)){

		/*if(pUserMap->GetAt(i).strUser.Find(strName, 0) == 0){
			
			strIP = pUserMap->GetAt(i).strRealIP;
			break;			
		}*/
		strIP = user.strRealIP;
	}
	//if(i >= pUserMap->GetSize()){
	else{

		strIP = "0.0.0.0";
	}
	return strIP;
}


CString CMetis::GetHostname(DWORD dwID, CString strName)
{

	CString strHost;

	CUserMap* pUserMap = GetRoom(dwID).pUserMap;
	
	if(pUserMap == NULL){
		
		ASSERT(FALSE);
		return strHost;
	}

	/*for(int i = 0; i < pUserMap->GetSize(); i++){

		if(pUserMap->GetAt(i).strUser.Find(strName, 0) == 0){
			
			strHost = pUserMap->GetAt(i).strHostname;
			break;			
		}
	}
	if(i >= pUserMap->GetSize()){*/
	
	MX_USERINFO user;
	if(pUserMap->Lookup(strName, user)){

		strHost = user.strHostname;
	}
	else{

		strHost = "unknown host";
	}
	return strHost;
}

// GetRawname return username with ID in given channel
CString CMetis::GetRawName(DWORD dwID, CString strNickname)
{
	
	CString strRawname = strNickname;

	if(m_aHalfID.Lookup(strNickname, strRawname) == FALSE){

		if(m_aNoID.Lookup(strNickname, strRawname) == FALSE){

			CUserMap* pUserMap = GetRoom(dwID).pUserMap;
	
			if(pUserMap == NULL){
				
				ASSERT(FALSE);
				strRawname = strNickname;
				return strRawname; // emergency exit ;)
			}
			MX_USERINFO user;
			if(pUserMap->Lookup(strNickname, user)){

				strRawname = user.strUser;
			}
		}
	}

	return strRawname;
}

// Get Connectión type of user in channel ID
CString CMetis::GetLineType(DWORD dwID, CString strName)
{

	CString strRawname, strLine = "Unknown";

	CUserMap* pUserMap = GetRoom(dwID).pUserMap;
	
	if(pUserMap == NULL){
		
		ASSERT(FALSE);
		return strLine;
	}

	/*for(int i = 0; i < pUserMap->GetSize(); i++){

		strRawname = pUserMap->GetAt(i).strUser;
		WORD wLine = pUserMap->GetAt(i).wLineType;
		if((wLine >= 0) && (wLine < NUM_LINES))
			strLine = line_types[wLine];
		else 
			strLine = line_types[0];
		if(strRawname.Find(strName, 0) == 0) break;	
	}*/
	MX_USERINFO user;
	if(pUserMap->Lookup(strName, user)){

		if((user.wLineType >= 0) && (user.wLineType < NUM_LINES)){

			strLine = line_types[user.wLineType];
		}
		else{

			strLine = line_types[0];
		}
	}
	
	return strLine;
}


// Get Number of files shared of user
CString CMetis::GetFiles(DWORD dwID, CString strName)
{

	CString strRawname, strFiles = "0";

	CUserMap* pUserMap = GetRoom(dwID).pUserMap;
	
	if(pUserMap == NULL){
		
		ASSERT(FALSE);
		return strFiles;
	}

	/*for(int i = 0; i < pUserMap->GetSize(); i++){

		strRawname = pUserMap->GetAt(i).strUser;
		strFiles.Format("%d", pUserMap->GetAt(i).dwNumFiles);
		if(strRawname.Find(strName, 0) == 0) break;	
	}*/
	MX_USERINFO user;
	if(pUserMap->Lookup(strName, user)){

		strFiles.Format("%d", user.dwNumFiles);
	}

	return strFiles;
}

// Remove warned status of a user
void CMetis::RemoveWarnedUser(CString strUser)
{

	for(int i = 0; i < m_aWarnedNicks.GetSize(); i++){

		if(strUser == m_aWarnedNicks.GetAt(i)){

			m_aWarnedNicks.RemoveAt(i, 1);
			return;
		}
	}
}

// Incruse warning level of a user
void CMetis::IncreaseWarnedCount(CString strUser)
{

	for(int i = 0; i < m_aWarnedNicks.GetSize(); i++){

		if(strUser == m_aWarnedNicks.GetAt(i)){

			if(i >= 255){
				return;
			}
			int nValue = m_vWarnedCount[i];
			m_vWarnedCount[i] = ++nValue;
			
			return; 
		}
	}
}

// Get the warned count of a user
int CMetis::GetWarnedCount(CString strUser)
{

	for(int i = 0; i < m_aWarnedNicks.GetSize(); i++){

		if(strUser == m_aWarnedNicks.GetAt(i)){

			if(i >= 255){
				return 0;
			}

			int nCnt = m_vWarnedCount[i];
			
			return nCnt ;
		}
	}

	return 0;
}


// Check if a user is warned
BOOL CMetis::IsUserWarned(CString strUser)
{

	for(int i = 0; i < m_aWarnedNicks.GetSize(); i++){

		if(strUser == m_aWarnedNicks.GetAt(i)){

			return TRUE;
		}
	}

	return FALSE;
}

void CMetis::FinishRound(DWORD dwID, CString strWinner)
{

	if(!m_aUserInfo.GetSize()) return;

	USER_INFO u1, u2, u3;

	u1.nTriviaScore = -1;
	u1.strName = "";
	for(int i = 0; i < m_aUserInfo.GetSize(); i++){

		if(m_aUserInfo[i].nTriviaScore > u1.nTriviaScore){

			TRACE("1> %s %d\n", m_aUserInfo[i].strName, m_aUserInfo[i].nTriviaScore);
			u1 = m_aUserInfo[i];
		}
	}

	if(m_aUserInfo.GetSize() > 1){

		u2.nTriviaScore = -1;
		u2.strName = "";
		for(i = 0; i < m_aUserInfo.GetSize(); i++){

			if((m_aUserInfo[i].nTriviaScore > u2.nTriviaScore) &&
				(m_aUserInfo[i].nTriviaScore <= u1.nTriviaScore) &&
				(m_aUserInfo[i].strName != u1.strName)){

				TRACE("2> %s %d\n", m_aUserInfo[i].strName, m_aUserInfo[i].nTriviaScore);
				u2 = m_aUserInfo[i];
			}
		}
	}
	if(m_aUserInfo.GetSize() > 2){


		u3.nTriviaScore = -1;
		u3.strName = "";
		for(i = 0; i < m_aUserInfo.GetSize(); i++){

			if((m_aUserInfo[i].nTriviaScore > u3.nTriviaScore) &&
				(m_aUserInfo[i].nTriviaScore <= u2.nTriviaScore) &&
				(m_aUserInfo[i].strName != u2.strName) &&
				(m_aUserInfo[i].strName != u1.strName)){

				TRACE("3> %s %d\n", m_aUserInfo[i].strName, m_aUserInfo[i].nTriviaScore);
				u3 = m_aUserInfo[i];
			}
		}
	}

	BOT_MESSAGE b;
	b.nType = OUT_NORMAL;
	b.nData = 0;
	b.nDelay = m_nFlood;
	b.strName = strWinner;
	b.dwChannelID = dwID;
	b.strTrigger = "ROUND-FINISHED";

	b.strResponse.Format("%s won this round:", strWinner);
	m_qMessageQueue.push(b);

	b.strResponse.Format("  Place 1: %s (%02d points)", u1.strName, u1.nTriviaScore);
	m_qMessageQueue.push(b);

	if(m_aUserInfo.GetSize() > 1){

		b.strResponse.Format("  Place 2: %s (%02d points)", u2.strName, u2.nTriviaScore);
		m_qMessageQueue.push(b);
	}

	if(m_aUserInfo.GetSize() > 2){

		b.strResponse.Format("  Place 3: %s (%02d points)", u3.strName, u3.nTriviaScore);
		m_qMessageQueue.push(b);
	}

	for(int i = 0; i <  m_aUserInfo.GetSize(); i++){

		m_aUserInfo[i].nTriviaScore = 0;
	}

	b.strResponse	= "Resetting game and starting new round.";
	m_qMessageQueue.push(b);
}

// Check for trivia answers
void CMetis::Trivia(DWORD dwID, CString strNickname, CString strMessage)
{

	// Nickname is emtpy, ignore input :-P
	if(strNickname.IsEmpty()){
	
		return;
	}
	// Message is emtpy, ignore input :-P
	if(strMessage.IsEmpty()){
		
		return;
	}
	
	// if this is a message printed by the bot, ignore it still :-P
	if((GetRawName(dwID, strNickname) == m_strBotname) || (m_strBotname.Find(GetName(strNickname), 0) == 0)){
		
		return;
	}

	// If this is a message printed by the bot via #nickname .... ignore it too :-P
	if((m_strLast.Find(strMessage, 0) >= NULL) && (m_strLast.Find(strNickname, 0) >= 0) && (m_strLast.Find("#private", 0) < 0)){
		
		return;
	}	

	CString strTmp;

	if(strMessage == "SCORE"){

		strTmp.Format("%d", GetScore(strNickname));

		BOT_MESSAGE b;
		b.nType = OUT_NORMAL;
		b.nData = -1;
		b.nDelay = m_nFlood;
		b.dwChannelID = dwID;
		b.strName = strNickname;
		b.strTrigger = "SCORE";
		b.strResponse = m_strTScore;
		b.strResponse.Replace("%SCORE%", strTmp);
		ReplaceVars(b.strData, strNickname, dwID);
		ReplaceUservars(b.strData);
		
		ReplaceVars(b.strResponse, strNickname, dwID);
		ReplaceUservars(b.strResponse);
		m_qMessageQueue.push(b);
		return;		
	}
	
	if(strMessage == "RANKING"){

		if(!m_aUserInfo.GetSize()) return;

		USER_INFO u1, u2, u3;

		u1.nTriviaScore = -1;
		u1.strName = "";
		for(int i = 0; i < m_aUserInfo.GetSize(); i++){

			if(m_aUserInfo[i].nTriviaScore > u1.nTriviaScore){

				TRACE("1> %s %d\n", m_aUserInfo[i].strName, m_aUserInfo[i].nTriviaScore);
				u1 = m_aUserInfo[i];
			}
		}

		if(m_aUserInfo.GetSize() > 1){

			u2.nTriviaScore = -1;
			u2.strName = "";
			for(i = 0; i < m_aUserInfo.GetSize(); i++){

				if((m_aUserInfo[i].nTriviaScore > u2.nTriviaScore) &&
					(m_aUserInfo[i].nTriviaScore <= u1.nTriviaScore) &&
					(m_aUserInfo[i].strName != u1.strName)){

					TRACE("2> %s %d\n", m_aUserInfo[i].strName, m_aUserInfo[i].nTriviaScore);
					u2 = m_aUserInfo[i];
				}
			}
		}
		if(m_aUserInfo.GetSize() > 2){


			u3.nTriviaScore = -1;
			u3.strName = "";
			for(i = 0; i < m_aUserInfo.GetSize(); i++){

				if((m_aUserInfo[i].nTriviaScore > u3.nTriviaScore) &&
					(m_aUserInfo[i].nTriviaScore <= u2.nTriviaScore) &&
					(m_aUserInfo[i].strName != u2.strName) &&
					(m_aUserInfo[i].strName != u1.strName)){

					TRACE("3> %s %d\n", m_aUserInfo[i].strName, m_aUserInfo[i].nTriviaScore);
					u3 = m_aUserInfo[i];
				}
			}
		}

		BOT_MESSAGE b;
		b.nType = OUT_NORMAL;
		b.nData = 0;
		b.nDelay = m_nFlood;
		b.strName = strNickname;
		b.dwChannelID = dwID;
		b.strTrigger = "RANKING";

		b.strResponse = "Trivia UserRanking:";
		m_qMessageQueue.push(b);

		b.strResponse.Format("  Place 1: %s (%02d points)", u1.strName, u1.nTriviaScore);
		m_qMessageQueue.push(b);

		if(m_aUserInfo.GetSize() > 1){

			b.strResponse.Format("  Place 2: %s (%02d points)", u2.strName, u2.nTriviaScore);
			m_qMessageQueue.push(b);
		}

		if(m_aUserInfo.GetSize() > 2){

			b.strResponse.Format("  Place 3: %s (%02d points)", u3.strName, u3.nTriviaScore);
			m_qMessageQueue.push(b);
		}
		return;		
	}

	if((strMessage == "SKIP") && IsAdmin(strNickname)){


		CString strTmp;
		BOT_MESSAGE b;

		b.nType = OUT_NORMAL;
		b.nData = 0;
		b.nDelay = m_nFlood;
		b.strResponse = m_strTSkip;
		b.strResponse.Replace("%QUES%", m_pCurrent->strTrigger);
		b.strResponse.Replace("%ANS%", m_pCurrent->aResponse[0]);
		strTmp.Format("%d", m_pCurrent->nTime);
		b.strResponse.Replace("%TIME%",	strTmp);
		strTmp.Format("%d", m_pCurrent->nPoints);
		b.strResponse.Replace("%POINTS%", strTmp);
		b.dwChannelID = dwID;
		b.strName = strNickname;
		b.strTrigger = "SCORE";
		ReplaceVars(b.strData, strNickname, dwID);
		ReplaceUservars(b.strData);
		
		ReplaceVars(b.strResponse, strNickname, dwID);
		ReplaceUservars(b.strResponse);
		m_qMessageQueue.push(b);

		m_pCurrent = NULL;
		return;		
	}

	if((strMessage.Find("RESET") == 0) && IsAdmin(strNickname)){

		for(int i = 0; i <  m_aUserInfo.GetSize(); i++){
	
			m_aUserInfo[i].nTriviaScore = 0;
		}

		CString strTmp = strMessage.Mid(6);
		strTmp.TrimLeft(); strTmp.TrimRight();

		if(strTmp.IsEmpty()){

			m_nRoundEnd = -1;
		}
		else{

			m_nRoundEnd		= atoi(strTmp);
		}

		BOT_MESSAGE b;
		b.nType			= OUT_NORMAL;
		b.nData			= 0;
		b.dwChannelID	= dwID;
		b.nDelay		= 0;
		b.strTrigger	= "RESET";
		b.strName		= strNickname;
		if(m_nRoundEnd != -1){

			b.strResponse.Format("Trivia score reset. Starting new round: %d points wins", m_nRoundEnd);
		}
		else{

			b.strResponse	= "Trivia score reset.";
		}
		m_qMessageQueue.push(b);
		return;		
	}

	if(m_pCurrent == NULL){


		return;
	}

	for(int i = 0; i < m_pCurrent->aResponse.GetSize(); i++){

		if(m_pCurrent == 0) break;
		if(ContainsStringExact(strMessage, m_pCurrent->aResponse[i], FALSE) >= 0){
		
			int nDone = 0;
			BOT_MESSAGE b;
			b.nType = OUT_NORMAL;
			b.nData = 0;
			b.nDelay = m_nFlood;
			b.dwChannelID = dwID;
			b.strName = strNickname;
			b.strTrigger = "SCORE";
			b.strResponse = m_strTCorrect;
			strTmp.Format("%d", IncreaseScore(strNickname, m_pCurrent->nPoints, &nDone));
			b.strResponse.Replace("%SCORE%", strTmp);
			b.strResponse.Replace("%ANS%", m_pCurrent->aResponse[i]);
			b.strResponse.Replace("%QUES%", m_pCurrent->strTrigger);
			strTmp.Format("%d", m_pCurrent->nTime);
			b.strResponse.Replace("%TIME%",	strTmp);
			strTmp.Format("%d", m_pCurrent->nPoints);
			b.strResponse.Replace("%POINTS%", strTmp);
			ReplaceVars(b.strData, strNickname, dwID);
			ReplaceUservars(b.strData);
			ReplaceVars(b.strResponse, strNickname, dwID);
			ReplaceUservars(b.strResponse);
			m_qMessageQueue.push(b);
			if(nDone != 0){

				FinishRound(dwID, strNickname);
			}
			m_pCurrent = NULL;
			break;
		}
	}
}

// Handles command output
// returns FALSE if output is not run due to not-matching condition
// return TRUE if run successfull
BOOL CMetis::HandleCommandOutput(DWORD dwID, PCOMMAND pCmd, const CString strNickname, const CString strMessage, TRIGGER t, const CString strParameter)
{


	CString strTmp;

	///////////////////////////////////////////////////////////////
	// NON-CMD_TYPE_SCRIPT   S T A R T
	//
	if(pCmd->nCmdType != CMD_TYPE_SCRIPT){

		// Get response index
		int nRes = (pCmd->nCmdType == CMD_TYPE_RANDOM ? rand() % pCmd->nNum : 0);

		// Get response string
		strTmp = pCmd->aResponses[nRes].strResponse;
		strTmp.Replace("%PARAM%", strParameter);
		strTmp.Replace("%PARAMETER%", strParameter);

		// Check if condition for trigger and response apply
		if(
			!MatchesCondition(pCmd->aResponses[nRes].nCondition, pCmd->aResponses[nRes].strLValue, pCmd->aResponses[nRes].strRValue, dwID, strNickname, "", strParameter, strMessage) ||
			!MatchesCondition(t.nCondition, t.strLValue, t.strRValue, dwID, strNickname, "", strParameter, strMessage)
			){

			return FALSE;
		}

		// If output is OUT_PUSH dump output emmediatly to uservars
		if(pCmd->aResponses[nRes].nCmdType == OUT_PUSH){

			if(pCmd->aResponses[nRes].nOperator == -1){

				ReplaceVars(strTmp, strNickname, dwID);
				ReplaceUservars(strTmp);
				SaveUserVar(pCmd->aResponses[nRes].strData, strTmp);
			}
			else{
				
				CString strRValue = pCmd->aResponses[nRes].strRop;
				CString strLValue = pCmd->aResponses[nRes].strLop;
				CString strNValue = pCmd->aResponses[nRes].strNop;

				ReplaceVars(strNValue, strNickname, dwID);
				ReplaceUservars(strNValue);
				ReplaceVars(strLValue, strNickname, dwID);
				ReplaceUservars(strLValue);
				strLValue.Replace("%PARAMETER%", strParameter);
				strLValue.Replace("%PARAM%", strParameter);
				strLValue.Replace("%TRIGGER%", strMessage);
				ReplaceVars(strRValue, strNickname, dwID);
				ReplaceUservars(strRValue);
				strRValue.Replace("%PARAMETER%", strParameter);
				strRValue.Replace("%PARAM%", strParameter);
				strRValue.Replace("%TRIGGER%", strMessage);

				SaveUserVar(pCmd->aResponses[nRes].strData, Operator(pCmd->aResponses[nRes].nOperator, strLValue, strRValue, strNValue));
			}
		}
		// clear a variable
		else if(pCmd->aResponses[nRes].nCmdType == OUT_POP){

			CString strClear = pCmd->aResponses[nRes].strData;

			m_mUserVar.RemoveKey(strClear);
		}

		else if(pCmd->aResponses[nRes].nCmdType == OUT_FILE){

			CString strData     = pCmd->aResponses[nRes].strData;
			CString strMode   = pCmd->aResponses[nRes].strMode;
			CString strResponse = pCmd->aResponses[nRes].strResponse;

			ReplaceVars(strData, strNickname, dwID);
			ReplaceUservars(strData);
			strData.Replace("%PARAMETER%", strParameter);
			strData.Replace("%PARAM%", strParameter);
			strData.Replace("%TRIGGER%", strMessage);
			
			ReplaceVars(strMode, strNickname, dwID);
			ReplaceUservars(strMode);
			strMode.Replace("%PARAMETER%", strParameter);
			strMode.Replace("%PARAM%", strParameter);
			strMode.Replace("%TRIGGER%", strMessage);
			
			ReplaceVars(strResponse, strNickname, dwID);
			ReplaceUservars(strResponse);
			strResponse.Replace("%PARAMETER%", strParameter);
			strResponse.Replace("%PARAM%", strParameter);
			strResponse.Replace("%TRIGGER%", strMessage);
			
			WriteToFile(strData, strMode, strResponse);
		}
		// else push it into the output queue
		else{

			BOT_MESSAGE b;
			b.dwChannelID = dwID;
			b.strName	  = strNickname;
			b.strTrigger  = strMessage;
			b.strResponse = strTmp;
			b.nDelay      = pCmd->aResponses[nRes].nDelay;
			b.nData		  = pCmd->aResponses[nRes].nData;
			b.nType       = pCmd->aResponses[nRes].nCmdType;
			b.strData	  = pCmd->aResponses[nRes].strData;
			
			ReplaceVars(b.strData, strNickname, dwID);
			ReplaceUservars(b.strData);
			b.strData.Replace("%PARAMETER%", strParameter);
			b.strData.Replace("%PARAM%", strParameter);
			b.strData.Replace("%TRIGGER%", strMessage);
			
			ReplaceVars(b.strResponse, strNickname, dwID);
			ReplaceUservars(b.strResponse);
			b.strResponse.Replace("%PARAMETER%", strParameter);
			b.strResponse.Replace("%PARAM%", strParameter);
			b.strResponse.Replace("%TRIGGER%", strMessage);
			
			DEBUG_TRACE("In Call RunCommand() Pushing output into queue:\n -- %s", strTmp);

			m_qMessageQueue.push(b);
		}
	}
	// NON-CMD_TYPE_SCRIPT   E N D
	//
	///////////////////////////////////////////////////////////////

	///////////////////////////////////////////////////////////////
	// CMD_TYPE_SCRIPT   S T A R T
	//
	else{

		// loop through all responses
		for(int k = 0; (k < pCmd->nNum) && !m_bStopCmdThreads; k++){

			// Get response string
			strTmp = pCmd->aResponses[k].strResponse;
			strTmp.Replace("%PARAM%", strParameter);
			strTmp.Replace("%PARAMETER%", strParameter);

			if(
				!MatchesCondition(pCmd->aResponses[k].nCondition, pCmd->aResponses[k].strLValue, pCmd->aResponses[k].strRValue, dwID, strNickname, "", strParameter, strMessage) ||
				!MatchesCondition(t.nCondition, t.strLValue, t.strRValue, dwID, strNickname, "", strParameter, strMessage)
				){

				continue;
			}

			if(pCmd->aResponses[k].nCmdType == OUT_BREAK){

				break;
			}
			else if(pCmd->aResponses[k].nCmdType == OUT_GOTO){

				int nGoto = k + pCmd->aResponses[k].nData;
				if((nGoto >= 0) && (nGoto < pCmd->nNum)){

					k = nGoto-1; // make it one smaller, because the for statement will 
								    // increment it again
					continue;
				}
				else{

					WriteMyEchoText(dwID, "Error in Command. Goto statement out of bound\n");
					break;
				}
			}
			else if(pCmd->aResponses[k].nCmdType == OUT_SLEEP){

				TRACE("Sleeping...\n");
				Sleep(pCmd->aResponses[k].nData);

				continue;
			}
			else if(pCmd->aResponses[k].nCmdType == OUT_PUSH){

				if(pCmd->aResponses[k].nOperator == -1){

					ReplaceVars(strTmp, strNickname, dwID);
					ReplaceUservars(strTmp);
					strTmp.Replace("%TRIGGER%", strMessage);

					SaveUserVar(pCmd->aResponses[k].strData, strTmp);
				}
				else{
					
					CString strRValue = pCmd->aResponses[k].strRop;
					CString strLValue = pCmd->aResponses[k].strLop;
					CString strNValue = pCmd->aResponses[k].strNop;

					ReplaceVars(strNValue, strNickname, dwID);
					ReplaceUservars(strNValue);
					ReplaceVars(strLValue, strNickname, dwID);
					ReplaceUservars(strLValue);
					strLValue.Replace("%PARAMETER%", strParameter);
					strLValue.Replace("%PARAM%", strParameter);
					strLValue.Replace("%TRIGGER%", strMessage);
					ReplaceVars(strRValue, strNickname, dwID);
					ReplaceUservars(strRValue);
					strRValue.Replace("%PARAMETER%", strParameter);
					strRValue.Replace("%PARAM%", strParameter);
					strRValue.Replace("%TRIGGER%", strMessage);

					SaveUserVar(pCmd->aResponses[k].strData, Operator(pCmd->aResponses[k].nOperator, strLValue, strRValue, strNValue));
				}
			}
			else if(pCmd->aResponses[k].nCmdType == OUT_POP){

				m_mUserVar.RemoveKey(pCmd->aResponses[k].strData);
			}
			else if(pCmd->aResponses[k].nCmdType == OUT_FILE){

				CString strData     = pCmd->aResponses[k].strData;
				CString strMode   = pCmd->aResponses[k].strMode;
				CString strResponse = pCmd->aResponses[k].strResponse;

				ReplaceVars(strData, strNickname, dwID);
				ReplaceUservars(strData);
				strData.Replace("%PARAMETER%", strParameter);
				strData.Replace("%PARAM%", strParameter);
				strData.Replace("%TRIGGER%", strMessage);
				
				ReplaceVars(strMode, strNickname, dwID);
				ReplaceUservars(strMode);
				strMode.Replace("%PARAMETER%", strParameter);
				strMode.Replace("%PARAM%", strParameter);
				strMode.Replace("%TRIGGER%", strMessage);
				
				ReplaceVars(strResponse, strNickname, dwID);
				ReplaceUservars(strResponse);
				strResponse.Replace("%PARAMETER%", strParameter);
				strResponse.Replace("%PARAM%", strParameter);
				strResponse.Replace("%TRIGGER%", strMessage);
				
				WriteToFile(strData, strMode, strResponse);
			}
			else{

				BOT_MESSAGE b;
				b.dwChannelID = dwID;
				b.strName	  = strNickname;
				b.strTrigger  = strMessage;
				b.strResponse = strTmp;
				b.nDelay	  = pCmd->aResponses[k].nDelay;
				b.nData		  = pCmd->aResponses[k].nData;
				b.nType       = pCmd->aResponses[k].nCmdType;
				b.strData	  = pCmd->aResponses[k].strData;

				ReplaceVars(b.strData, strNickname, dwID);
				ReplaceUservars(b.strData);
				b.strData.Replace("%PARAMETER%", strParameter);
				b.strData.Replace("%PARAM%", strParameter);
				b.strData.Replace("%TRIGGER%", strMessage);
				
				ReplaceVars(b.strResponse, strNickname, dwID);
				ReplaceUservars(b.strResponse);
				b.strResponse.Replace("%PARAMETER%", strParameter);
				b.strResponse.Replace("%PARAM%", strParameter);
				b.strResponse.Replace("%TRIGGER%", strMessage);

				DEBUG_TRACE("In Call RunCommand() Pushing output into queue:\n -- %s", strTmp);
				m_qMessageQueue.push(b);
				Sleep(10);
			}
		}

	}
	//
	// CMD_TYPE_SCRIPT   E N D
	///////////////////////////////////////////////////////////////

	return TRUE;
}

// run a command (<command>)
void CMetis::RunCommand(PCOMMAND pCmd, DWORD dwID, CString strNickname, CString strMessage)
{

	// if command condition does not skip rest of loop
	if(!MatchesCondition(pCmd->nCondition, pCmd->strLValue, pCmd->strRValue, dwID, strNickname, "", strMessage)) return;

	CString strTmp, strParameter;
	// loop through all triggers of the command
	for(int j = 0; (j < pCmd->aTrigger.GetSize()) && !m_bStopCmdThreads; j++){

		TRIGGER t = pCmd->aTrigger.GetAt(j);
		strTmp = t.strTrigger;

		// replace all uservariables in trigger.
		ReplaceUservars(strTmp);

		// Get Position of $PARAM...
		strTmp.Replace("%PARAM%", "%PARAMETER%");
		int nParamPosTrigger = strTmp.Find("%PARAMETER%", 0);
		int nTriggerPosMsg = 0;

		strTmp.Replace("%PARAMETER%", "*");
		
		// Check if input matches this trigger
		if((nTriggerPosMsg = ContainsStringExact(strMessage, strTmp, t.bCase, t.bExact)) >= 0){
			
			nParamPosTrigger += nTriggerPosMsg;

			// check if user is blocked. return when he is.
			if(IsUserBlocked(strNickname)) return;

			// Get Rights management for this command and the trigger
			PUSER_GROUP p1 = GetUserGroup(pCmd->strGroup);
			PUSER_GROUP p2 = GetUserGroup(t.strGroup);
			
			// if one of these are none empty check if the user applies to a group
			if(!pCmd->strNames.IsEmpty() || !pCmd->strGroup.IsEmpty() || 
				!t.strNames.IsEmpty() || !t.strGroup.IsEmpty()){

				if(!NickApplies(pCmd->strNames, strNickname, pCmd->bCase) &&
				!NickApplies(t.strNames, strNickname, t.bCase) &&
				!GroupApplies(t.strGroup, strNickname) &&
				!GroupApplies(pCmd->strGroup, strNickname)) continue;
			}


			// Check for flood protection
			if((GetTickCount() - (DWORD)pCmd->nLast) > (DWORD)pCmd->nFlood*1000) pCmd->nLast = GetTickCount();
			else continue; 

			// Get the Parameter :-)
			strParameter = strMessage.Mid(nParamPosTrigger >= 0 ? nParamPosTrigger : 0);
			if(m_bParam){
				
				strParameter.Replace("#admin", "admin");
				strParameter.Replace("#user", "user");
			}
			// Handle the output now, then break out of the loop
			if(HandleCommandOutput(dwID, pCmd, strNickname, strMessage, t, strParameter)){

				break;
			}
			else{

				continue;
			}
		}
	}
}

// run a command (<command>)
void CMetis::RunCommand(PEXT_CMD pExtCmd)
{
	
	ASSERT(pExtCmd != NULL);
	RunCommand(pExtCmd->pCmd, pExtCmd->dwID, pExtCmd->strNickname, pExtCmd->strMessage);
}

UINT CMetis::AsyncCmd(PVOID pVoid)
{

	ASSERT(pVoid != NULL);
	PEXT_CMD pExt = (PEXT_CMD)pVoid;
	CMetis* pPlugin = (CMetis*)pExt->pPlugin;
	
	if(pPlugin->m_bStopCmdThreads) return 0;

	//TRACE("Entering Async Cmd %s\n", pExt->pCmd->aTrigger[0].strTrigger);
	pPlugin->RunCommand(pExt);
	//TRACE("Leaving Async Cmd %s\n", pExt->pCmd->aTrigger[0].strTrigger);
	
	pExt->dwID = 0;
	pExt->pCmd = 0;
	pExt->pCmd = 0;
	pExt->strMessage = "";
	pExt->strNickname = "";
	
	delete pExt;
	pExt = 0;

	return 0;
}

// Main input processing for message/action
void CMetis::Bot(DWORD dwID, CString strNickname, CString strMessage, BOOL bOp)
{
	

	// Nickname is emtpy, ignore input :-P
	if(strNickname.IsEmpty() && !bOp){
	
		DEBUG_TRACE("Leaving call Bot(). Reason: Nickname is empty.\n");
		return;
	}
	// Message is emtpy, ignore input :-P
	if(strMessage.IsEmpty()){
		
		DEBUG_TRACE("Leaving call Bot(). Reason: Message is empty.\n");
		return;
	}
	
	// if this is a message printed by the bot, ignore it still :-P
	if((GetRawName(dwID, strNickname) == m_strBotname) || (m_strBotname.Find(GetName(strNickname), 0) == 0)){
		
		DEBUG_TRACE("Leaving call Bot(). Reason: Message was posted with my nickname.\n");
		return;
	}

	// If this is a message printed by the bot via #nickname .... ignore it too :-P
	if((m_strLast.Find(strMessage, 0) >= NULL) && (m_strLast.Find(strNickname, 0) >= 0) && (m_strLast.Find("#private", 0) < 0)){
		
		DEBUG_TRACE("Leaving call Bot(). Reason: Message was posted by myself.");
		DEBUG_TRACE("  -- " + m_strLast + "\n");
		return;
	}	
	
	// Security option ignore all #nickname #message
	if(m_bNoNick){

		DEBUG_TRACE("Leaving call Bot(). Reason: SecureNickname applies.\n");
		if(GetRawName(dwID, strNickname) == GetName(strNickname)) return;
	}

	CString strTmp, strTmp2;

	strNickname.Replace("*", "&lowast;");
	strNickname.Replace("$", "&sect;");
	strNickname.Replace("%", "&perc;");
	strMessage.Replace("*", "&lowast;");
	strMessage.Replace("$", "&sect;");
	strMessage.Replace("%", "&perc;");

	// Badword
	for(int i = 0; (i < m_aBadWords.GetSize()) && m_bBadWord && !bOp; i++){


		PBADWORD pB = m_aBadWords.GetAt(i);
		
		// Check if trigger applies and if condition is valid
		if(
			(ContainsStringExact(strMessage, pB->strTrigger, pB->bCase) >= 0) && 
			MatchesCondition(pB->nCondition, pB->strLValue, pB->strRValue, dwID, strNickname, "", strMessage)
		   ){

			// check wether user/-groups are to be excluded....
			if(m_bExclude){

				PUSER_GROUP pUg = GetUserGroup(pB->strGroup);
				if(pUg != NULL){

					if(NickApplies(pUg->strUsers, strNickname, pUg->bCase)) continue;
				}
				if(NickApplies(pB->strNames, strNickname,pB->bCase)) continue;
			}
			
			// check if user is already warned
			if(!IsUserWarned(strNickname)){

				// if he is not, add him to the list
				m_vWarnedCount[m_aWarnedNicks.Add(strNickname)] = 1;
			}
			//else{

			// user is warned
			int nWarnedCnt = GetWarnedCount(strNickname);

			// user has exceeded maximum number of warnings
			if(nWarnedCnt >= m_nAutoKick){
				
				// kick him
				BOT_MESSAGE b;
				b.dwChannelID = dwID;
				b.strName	  = strNickname;
				b.strResponse = m_strPreKick;
				b.strTrigger  = m_strPreKick;
				b.nDelay      = m_nFlood;
				b.nData	      = -1;
				b.nType       = CMD_TYPE_NORMAL;

				ReplaceVars(b.strData, strNickname, dwID);
				ReplaceUservars(b.strData);
				
				ReplaceVars(b.strResponse, strNickname, dwID);
				ReplaceUservars(b.strResponse);

				m_qMessageQueue.push(b);
				Sleep(20);

				b.dwChannelID = dwID;
				b.strName	  = strNickname;
				b.strResponse = m_strBadLanRedirect;
				b.strTrigger  = m_strBadLanRedirect;
				b.nDelay      = m_nFlood;
				b.nData	      = -1;
				b.nType       = CMD_TYPE_NORMAL;
				
				ReplaceVars(b.strData, strNickname, dwID);
				ReplaceUservars(b.strData);
				
				ReplaceVars(b.strResponse, strNickname, dwID);
				ReplaceUservars(b.strResponse);

				m_qMessageQueue.push(b);
				RemoveWarnedUser(strNickname);
				break;
			}
			else{

				// warn user another time
				IncreaseWarnedCount(strNickname);
			}
			//} // is warned user

			// managed warned count and he wasnt kicked so write a warning
			BOT_MESSAGE b;
			b.dwChannelID = dwID;
			b.strName	  = strNickname;
			b.strTrigger  = strMessage;
			b.strResponse = m_strBadLan;
			b.nDelay      = m_nFlood;
			b.nData       = -1;
			b.nType       = CMD_TYPE_NORMAL;

			ReplaceVars(b.strData, strNickname, dwID);
			ReplaceUservars(b.strData);
			
			ReplaceVars(b.strResponse, strNickname, dwID);
			ReplaceUservars(b.strResponse);

			m_qMessageQueue.push(b);
		}
	}


	// loop through all <command>
	for(i = 0; i < m_aCommands.GetSize() && !bOp; i++){


		PCOMMAND pCmd = m_aCommands.GetAt(i);
		if(pCmd->nMode == CMD_MODE_BLOCK){

			RunCommand(pCmd, dwID, strNickname, strMessage);
		}
		else{

			PEXT_CMD pExtCmd = new EXT_CMD;
			pExtCmd->pCmd = pCmd;
			pExtCmd->dwID = dwID;
			pExtCmd->strMessage = strMessage;
			pExtCmd->strNickname = strNickname;
			pExtCmd->pPlugin = this;
			AfxBeginThread(AsyncCmd, (LPVOID)pExtCmd, THREAD_PRIORITY_NORMAL, 0, 0, 0);
		}
	}

	// loop through all <OnOpMessage>
	for(i = 0; i < m_aOpCommands.GetSize() && bOp; i++){


		PCOMMAND pCmd = m_aOpCommands.GetAt(i);
		if(pCmd->nMode == CMD_MODE_BLOCK){

			RunCommand(pCmd, dwID, strNickname, strMessage);
		}
		else{

			PEXT_CMD pExtCmd = new EXT_CMD;
			pExtCmd->pCmd = pCmd;
			pExtCmd->dwID = dwID;
			pExtCmd->strMessage = strMessage;
			pExtCmd->strNickname = strNickname;
			pExtCmd->pPlugin = this;
			AfxBeginThread(AsyncCmd, (LPVOID)pExtCmd, THREAD_PRIORITY_NORMAL, 0, 0, 0);
		}
	}

}

// Handle Enter / Leave user commands
void CMetis::OnEnterLeaveUser(DWORD dwID, PMX_USERINFO pUserInfo, BOOL bEnter)
{

	if(pUserInfo <= 0){

		//WriteEchoText(dwID, "Fatal Error in call OnEnterLeaveUser(...): Pointer to PMX_USERIFNO structure is invalid!\n", RGB(255,0,0), 0);
		return;
	}
	CString strTmp, strNickname = pUserInfo->strUser;
	strNickname.Replace("*", "&lowast;");
	strNickname.Replace("$", "&sect;");
	strNickname.Replace("%", "&perc;");

	if(strNickname.IsEmpty()) return;

	// loop through all commands
	int nMax = (int)(bEnter ? m_aEnter.GetSize() : m_aLeave.GetSize());
	for(int i = 0; i < nMax; i++){

		PCOMMAND pCmd = (bEnter? m_aEnter.GetAt(i) : m_aLeave.GetAt(i));
		
		if(!MatchesCondition(pCmd->nCondition, pCmd->strLValue, pCmd->strRValue, dwID, strNickname, pUserInfo->strRealIP)) continue;

		PUSER_GROUP p1 = GetUserGroup(pCmd->strGroup);
		
		// if one of these are none empty check if the user applies to a group
		if(!pCmd->strNames.IsEmpty() || !pCmd->strGroup.IsEmpty()){

			if(!NickApplies(pCmd->strNames, strNickname, pCmd->bCase) &&
			!GroupApplies(pCmd->strGroup, strNickname)) continue;
		}

		if(pCmd->nCmdType != CMD_TYPE_SCRIPT){

			int nRes = (pCmd->nCmdType == CMD_TYPE_RANDOM ? rand() % pCmd->nNum : 0);

			if(!MatchesCondition(pCmd->aResponses[nRes].nCondition, pCmd->aResponses[nRes].strLValue, pCmd->aResponses[nRes].strRValue, dwID, strNickname, pUserInfo->strRealIP)) continue;

			strTmp = pCmd->aResponses[nRes].strResponse;

			char num[10];
			itoa(pUserInfo->dwNumFiles, (char*)num, 10);
			strTmp.Replace("%FILES%", num);

			if((pUserInfo->wLineType >= 0) && (pUserInfo->wLineType < NUM_LINES)){
				
				strTmp.Replace("%LINE%", line_types[pUserInfo->wLineType]);
			}
			else{

				strTmp.Replace("%LINES%", "N/A");
			}

			strTmp.Replace("%IP%", pUserInfo->strRealIP);
			strTmp.Replace("%RAWNAME%", strNickname);

			if(pCmd->aResponses[nRes].nCmdType == OUT_PUSH){

				if(pCmd->aResponses[nRes].nOperator == -1){

					ReplaceVars(strTmp, strNickname, dwID);
					ReplaceUservars(strTmp);
					SaveUserVar(pCmd->aResponses[nRes].strData, strTmp);
				}
				else{
					
					CString strRValue = pCmd->aResponses[nRes].strRop;
					CString strLValue = pCmd->aResponses[nRes].strLop;
					CString strNValue = pCmd->aResponses[nRes].strNop;

					ReplaceVars(strNValue, strNickname, dwID);
					ReplaceUservars(strNValue);
					ReplaceVars(strLValue, strNickname, dwID);
					ReplaceUservars(strLValue);
					ReplaceVars(strRValue, strNickname, dwID);
					ReplaceUservars(strRValue);

					SaveUserVar(pCmd->aResponses[nRes].strData, Operator(pCmd->aResponses[nRes].nOperator, strLValue, strRValue, strNValue));
				}
			}
			else if(pCmd->aResponses[nRes].nCmdType == OUT_POP){

				m_mUserVar.RemoveKey(pCmd->aResponses[nRes].strData);
			}
			else if(pCmd->aResponses[nRes].nCmdType == OUT_FILE){

				CString strData     = pCmd->aResponses[nRes].strData;
				CString strMode   = pCmd->aResponses[nRes].strMode;
				CString strResponse = pCmd->aResponses[nRes].strResponse;

				ReplaceVars(strData, strNickname, dwID);
				ReplaceUservars(strData);
				
				ReplaceVars(strMode, strNickname, dwID);
				ReplaceUservars(strMode);
				
				ReplaceVars(strResponse, strNickname, dwID);
				ReplaceUservars(strResponse);
				
				WriteToFile(strData, strMode, strResponse);
			}
			else{

				BOT_MESSAGE b;
				b.dwChannelID = dwID;
				b.strName	  = strNickname;
				b.strTrigger  = "OnEnter";
				b.strResponse = strTmp;
				b.nDelay      = pCmd->aResponses[nRes].nDelay;
				b.nData		  = pCmd->aResponses[nRes].nData;
				b.nType       = pCmd->aResponses[nRes].nCmdType;
				b.strData	  = pCmd->aResponses[nRes].strData;
				
				ReplaceVars(b.strData, strNickname, dwID);
				ReplaceUservars(b.strData);
				
				ReplaceVars(b.strResponse, strNickname, dwID);
				ReplaceUservars(b.strResponse);

				m_qMessageQueue.push(b);
			}
		}
		else{

			for(int k = 0; k < pCmd->nNum; k++){

				if(!MatchesCondition(pCmd->aResponses[k].nCondition, pCmd->aResponses[k].strLValue, pCmd->aResponses[k].strRValue,dwID, strNickname, pUserInfo->strRealIP)) continue;
				strTmp = pCmd->aResponses[k].strResponse;

				char num[10];
				itoa(pUserInfo->dwNumFiles, (char*)num, 10);
				strTmp.Replace("%FILES%", num);
				if((pUserInfo->wLineType >= 0) && (pUserInfo->wLineType < NUM_LINES)){

					strTmp.Replace("%LINE%", line_types[pUserInfo->wLineType]);
				}
				else{

					strTmp.Replace("%LINE%", "N/A");
				}
				strTmp.Replace("%IP%", pUserInfo->strRealIP);
				strTmp.Replace("%RAWNAME%", strNickname);

				if(pCmd->aResponses[k].nCmdType == OUT_BREAK){

					break;
				}
				else if(pCmd->aResponses[k].nCmdType == OUT_GOTO){

					int nGoto = k + pCmd->aResponses[k].nData;
					if((nGoto >= 0) && (nGoto < pCmd->nNum)){

						k = nGoto-1; // make it one smaller, because the for statement will 
								     // increment it again
						continue;
					}
					else{

						WriteMyEchoText(dwID, "Error in Command. Goto statement out of bound\n");
						break;
					}
				}
				else if(pCmd->aResponses[k].nCmdType == OUT_PUSH){

					if(pCmd->aResponses[k].nOperator == -1){

						ReplaceVars(strTmp, strNickname, dwID);
						ReplaceUservars(strTmp);
						SaveUserVar(pCmd->aResponses[k].strData, strTmp);
					}
					else{
						
						CString strRValue = pCmd->aResponses[k].strRop;
						CString strLValue = pCmd->aResponses[k].strLop;
						CString strNValue = pCmd->aResponses[k].strNop;

						ReplaceVars(strNValue, strNickname, dwID);
						ReplaceUservars(strNValue);
						ReplaceVars(strLValue, strNickname, dwID);
						ReplaceUservars(strLValue);
						ReplaceVars(strRValue, strNickname, dwID);
						ReplaceUservars(strRValue);

						SaveUserVar(pCmd->aResponses[k].strData, Operator(pCmd->aResponses[k].nOperator, strLValue, strRValue, strNValue));
					}
				}
				else if(pCmd->aResponses[k].nCmdType == OUT_POP){

					m_mUserVar.RemoveKey(pCmd->aResponses[k].strData);
				}
				else if(pCmd->aResponses[k].nCmdType == OUT_FILE){

					CString strData     = pCmd->aResponses[k].strData;
					CString strMode   = pCmd->aResponses[k].strMode;
					CString strResponse = pCmd->aResponses[k].strResponse;

					ReplaceVars(strData, strNickname, dwID);
					ReplaceUservars(strData);
					
					ReplaceVars(strMode, strNickname, dwID);
					ReplaceUservars(strMode);
					
					ReplaceVars(strResponse, strNickname, dwID);
					ReplaceUservars(strResponse);
					
					WriteToFile(strData, strMode, strResponse);
				}
				else{

					BOT_MESSAGE b;
					b.dwChannelID = dwID;
					b.strName	  = strNickname;
					b.strTrigger  = "OnEnter";
					b.strResponse = strTmp;
					b.nDelay	  = pCmd->aResponses[k].nDelay;
					b.nData		  = pCmd->aResponses[k].nData;
					b.nType       = pCmd->aResponses[k].nCmdType;
					b.strData	  = pCmd->aResponses[k].strData;

					ReplaceVars(b.strData, strNickname, dwID);
					ReplaceUservars(b.strData);
					
					ReplaceVars(b.strResponse, strNickname, dwID);
					ReplaceUservars(b.strResponse);

					m_qMessageQueue.push(b);
				}
			}
		}
	}
}

// Print next automessagte
void CMetis::PrintAutoMessage()
{

	TRACE("Entering PrintAuto\n");
	if(m_aRooms.GetSize() && (cAuto.nNum > 0)){
	
		TRACE("There is a OnTimer command...\n");
		CString strTmp;

		if((cAuto.nCmdType == CMD_TYPE_RANDOM) || (cAuto.nCmdType == CMD_TYPE_NORMAL)){

			TRACE("Type is %d (random/normal handler)\n", cAuto.nCmdType);
			int nRes = (cAuto.nCmdType == CMD_TYPE_RANDOM ? rand() % cAuto.nNum : 0);
			strTmp = cAuto.aResponses[nRes].strResponse;

			BOT_MESSAGE b;
			b.strName	  = m_strBotname;
			b.strTrigger  =  "automessage";
			b.strResponse = strTmp;
			b.nDelay      = cAuto.aResponses[nRes].nDelay;
			b.nData		  = cAuto.aResponses[nRes].nData;
			b.nType       = cAuto.aResponses[nRes].nCmdType;
			b.strData	  = cAuto.aResponses[nRes].strData;

			for(int i = 0; i < m_aRooms.GetSize(); i++){

				if(!m_aRooms[i].bIsBotActivated) continue;
				if(!MatchesCondition(cAuto.nCondition, cAuto.strLValue, cAuto.strRValue, m_aRooms[i].dwID, m_strBotname, "")) continue;
				if(!MatchesCondition(cAuto.aResponses[nRes].nCondition, cAuto.aResponses[nRes].strLValue, cAuto.aResponses[nRes].strRValue, m_aRooms[i].dwID, m_strBotname, "")) continue;

				if(cAuto.aResponses[nRes].nCmdType == OUT_PUSH){

					if(cAuto.aResponses[nRes].nOperator == -1){

						ReplaceVars(strTmp, "automessage", m_aRooms[i].dwID);
						ReplaceUservars(strTmp);
						SaveUserVar(cAuto.aResponses[nRes].strData, strTmp);
					}
					else{
						
						CString strRValue = cAuto.aResponses[nRes].strRop;
						CString strLValue = cAuto.aResponses[nRes].strLop;
						CString strNValue = cAuto.aResponses[nRes].strNop;

						ReplaceVars(strNValue, "automessage", m_aRooms[i].dwID);
						ReplaceUservars(strNValue);
						ReplaceVars(strLValue, "automessage", m_aRooms[i].dwID);
						ReplaceUservars(strLValue);
						ReplaceVars(strRValue, "automessage", m_aRooms[i].dwID);
						ReplaceUservars(strRValue);

						SaveUserVar(cAuto.aResponses[nRes].strData, Operator(cAuto.aResponses[nRes].nOperator, strLValue, strRValue, strNValue));
					}
				}
				else if(cAuto.aResponses[nRes].nCmdType == OUT_POP){

					m_mUserVar.RemoveKey(cAuto.aResponses[nRes].strData);
				}
				else if(cAuto.aResponses[nRes].nCmdType == OUT_FILE){

					CString strData     = cAuto.aResponses[nRes].strData;
					CString strMode     = cAuto.aResponses[nRes].strMode;
					CString strResponse = cAuto.aResponses[nRes].strResponse;

					ReplaceVars(strData, "automessage", m_aRooms[i].dwID);
					ReplaceUservars(strData);
					
					ReplaceVars(strMode, "automessage", m_aRooms[i].dwID);
					ReplaceUservars(strMode);
					
					ReplaceVars(strResponse, "automessage", m_aRooms[i].dwID);
					ReplaceUservars(strResponse);
					
					WriteToFile(strData, strMode, strResponse);
				}
				else{

					b.dwChannelID = m_aRooms[i].dwID;

					ReplaceVars(b.strData, b.strName, m_aRooms[i].dwID);
					ReplaceUservars(b.strData);
					
					ReplaceVars(b.strResponse, b.strName, m_aRooms[i].dwID);
					ReplaceUservars(b.strResponse);

					m_qMessageQueue.push(b);
				}
			}

		}
		else if(cAuto.nCmdType == CMD_TYPE_SCRIPT){

			TRACE("Type is %d (script handler)\n", cAuto.nCmdType);
			for(int k = 0; k < cAuto.nNum; k++){

				TRACE("<out> %d of %d\n", k, cAuto.nNum);
				strTmp = cAuto.aResponses[k].strResponse;
				int nPos = 0;

				BOT_MESSAGE b;
				b.strName	  = m_strBotname;
				b.strTrigger  = "automessage";
				b.strResponse = strTmp;
				b.nDelay      = cAuto.aResponses[k].nDelay;
				b.nData		  = cAuto.aResponses[k].nData;
				b.nType       = cAuto.aResponses[k].nCmdType;
				b.strData	  = cAuto.aResponses[k].strData;

				for(int i = 0; i < m_aRooms.GetSize(); i++){

					TRACE("Handling for room 0x%X\n", m_aRooms[i].dwID);
					if(!m_aRooms[i].bIsBotActivated) continue;
					if(!MatchesCondition(cAuto.nCondition, cAuto.strLValue, cAuto.strRValue, m_aRooms[i].dwID, m_strBotname,"")) continue;
					if(!MatchesCondition(cAuto.aResponses[k].nCondition, cAuto.aResponses[k].strLValue, cAuto.aResponses[k].strRValue, m_aRooms[i].dwID, m_strBotname,"")) continue;

					if(cAuto.aResponses[k].nCmdType == OUT_BREAK){

						break;
					}
					else if(cAuto.aResponses[k].nCmdType == OUT_GOTO){

						int nGoto = k + cAuto.aResponses[k].nData;
						if((nGoto >= 0) && (nGoto < cAuto.nNum)){

							k = nGoto-1; // make it one smaller, because the for statement will 
								         // increment it again
							continue;
						}
						else{

							WriteMyEchoText(m_aRooms[i].dwID, "Error in Command. Goto statement out of bound\n");
							break;
						}
					}
					else if(cAuto.aResponses[k].nCmdType == OUT_PUSH){

						if(cAuto.aResponses[k].nOperator == -1){

							ReplaceVars(strTmp, "automessage", m_aRooms[i].dwID);
							ReplaceUservars(strTmp);
							SaveUserVar(cAuto.aResponses[k].strData, strTmp);
						}
						else{
							
							CString strRValue = cAuto.aResponses[k].strRop;
							CString strLValue = cAuto.aResponses[k].strLop;
							CString strNValue = cAuto.aResponses[k].strNop;

							ReplaceVars(strNValue, "automessage", m_aRooms[i].dwID);
							ReplaceUservars(strNValue);
							ReplaceVars(strLValue, "automessage", m_aRooms[i].dwID);
							ReplaceUservars(strLValue);
							ReplaceVars(strRValue, "automessage", m_aRooms[i].dwID);
							ReplaceUservars(strRValue);

							SaveUserVar(cAuto.aResponses[k].strData, Operator(cAuto.aResponses[k].nOperator, strLValue, strRValue, strNValue));
						}
					}
					else if(cAuto.aResponses[k].nCmdType == OUT_POP){

						m_mUserVar.RemoveKey(cAuto.aResponses[k].strData);
					}
					else if(cAuto.aResponses[k].nCmdType == OUT_FILE){

						CString strData     = cAuto.aResponses[k].strData;
						CString strMode     = cAuto.aResponses[k].strMode;
						CString strResponse = cAuto.aResponses[k].strResponse;

						ReplaceVars(strData, "automessage", m_aRooms[i].dwID);
						ReplaceUservars(strData);
						
						ReplaceVars(strMode, "automessage", m_aRooms[i].dwID);
						ReplaceUservars(strMode);
						
						ReplaceVars(strResponse, "automessage", m_aRooms[i].dwID);
						ReplaceUservars(strResponse);
						
						WriteToFile(strData, strMode, strResponse);
					}
					else{

						TRACE("Printing for room 0x%X\n", m_aRooms[i].dwID);
						b.dwChannelID = m_aRooms[i].dwID;
						ReplaceVars(b.strData, b.strName, m_aRooms[i].dwID);
						ReplaceUservars(b.strData);
						
						ReplaceVars(b.strResponse, b.strName, m_aRooms[i].dwID);
						ReplaceUservars(b.strResponse);
						m_qMessageQueue.push(b);
					}
				}
			}
		}
	}
	TRACE("Laeving PrintAuto\n");
}


// Print out the help on screen
void CMetis::PrintHelp(DWORD dwID)
{

	//WriteEchoText(dwID, "<!> Middle-Button or Right-Button Mouse click will show Metis menu <!>\n", m_crErr, m_crBg);
	CString strVer;
	strVer.Format("%s © 2003-2006 by Bender. All rights reserved.\n", STR_VERSION_DLG);
	WriteEchoText(dwID, strVer, m_crOK, m_crBg);
	WriteEchoText(dwID, "Using TinyXML © 2000 - 2002 by Lee Thomason (www.sourceforge.net/projects/tinyxml)\n\n", m_crOK, m_crBg);
	WriteEchoText(dwID, "Available commands to control this Bot:\n", m_crOK, m_crBg);
	WriteEchoText(dwID, "/mxc stats    - some statistics (loaded commands etc.)\n", m_crPend, m_crBg);
	WriteEchoText(dwID, "/mxc spy     - displays uservaribales currently in memory\n", m_crPend, m_crBg);
	WriteEchoText(dwID, "/mxc edit     - Edit MXC.xml file in default editor\n", m_crPend, m_crBg);
	WriteEchoText(dwID, "/mxc edit <file>     - Edit <file> file in default editor\n", m_crPend, m_crBg);
	WriteEchoText(dwID, "/mxc load     - Reload configuration file MXC.xml after editing.\n", m_crPend, m_crBg);
	WriteEchoText(dwID, "/mxc load <file>     - Load commands from <file> configuration file.\n", m_crPend, m_crBg);
	WriteEchoText(dwID, "/mxc start     - Start the bot in this room\n", m_crPend, m_crBg);
	WriteEchoText(dwID, "/mxc stop     - Stop the bot in this room\n", m_crPend, m_crBg);
	WriteEchoText(dwID, "/mxc trivia start    - Run a trivia game in this room\n", m_crPend, m_crBg);
	WriteEchoText(dwID, "/mxc trivia stop    - Stop a trivia game in this room\n", m_crPend, m_crBg);
	WriteEchoText(dwID, "/mxc help     - print help\n", m_crPend, m_crBg);
	WriteEchoText(dwID, "<!> Middle-Button or Right-Button Mouse click will show Metis menu <!>\n", m_crErr, m_crBg);
	WriteEchoText(dwID, "<!> Type '/mxc run' to start the bot <!>\n\n", m_crErr, m_crBg);
}

// Check if command condition apply
BOOL CMetis::MatchesCondition(int nCondition, CString strLValue, CString strRValue, DWORD dwID, CString strName, CString strIP, CString strParameter, CString strTrigger)
{

	if(nCondition == CON_DONTUSE){

		return TRUE;
	}
	// Replace all variables
	ReplaceVars(strLValue, strName, dwID);
	ReplaceVars(strRValue, strName, dwID);
	ReplaceUservars(strLValue);
	ReplaceUservars(strRValue);
	strLValue.Replace("%PARAMETER%", strParameter);
	strRValue.Replace("%PARAMETER%", strParameter);
	strLValue.Replace("%PARAM%", strParameter);
	strRValue.Replace("%PARAM%", strParameter);
	strLValue.Replace("%IP%", strIP);
	strRValue.Replace("%IP%", strIP);
	strLValue.Replace("%TRIGGER%", strTrigger);
	strRValue.Replace("%TRIGGER%", strTrigger);
	// fix broken entities (& removed by tinyxml)
	strRValue.Replace("lowast;", "&lowast;");
	strRValue.Replace("sect;", "&sect;");
	strRValue.Replace("perc;", "&perc;");
	strLValue.Replace("lowast;", "&lowast;");
	strLValue.Replace("sect;", "&sect;");
	strLValue.Replace("perc;", "&perc;");
	// fix special charaters
	strRValue.Replace("$", "&sect;");
	strRValue.Replace("%", "&perct;");
	strLValue.Replace("$", "&sect;");
	strLValue.Replace("%", "&perct;");

	BOOL bReturn = FALSE;
	int nA = 0, nB = 0;

	switch(nCondition){

	case CON_NOTEQUAL: // no equal
		bReturn = strLValue != strRValue;
		break;
	case CON_EQUAL: // equal
		bReturn = strLValue == strRValue;
		break;
	case CON_SMALLER: // smaller
		nA = atoi(strLValue);
		nB = atoi(strRValue);
		bReturn = nA < nB;
		break;
	case CON_GREATER: // greater
		nA = atoi(strLValue);
		nB = atoi(strRValue);
		bReturn = nA > nB;
		break;
	case CON_FIND: // contains
		bReturn = strLValue.Find(strRValue,0) >= 0;
		break;
	case CON_NOTFIND: // contains not
		bReturn = strLValue.Find(strRValue,0) < 0;
		break;
	case CON_FINDNC: // contains (no case)
		strLValue.MakeLower();
		strRValue.MakeLower();
		bReturn = strLValue.Find(strRValue,0) >= 0;
		break;
	case CON_NOTFINDNC: // contains not (no case)
		strLValue.MakeLower();
		strRValue.MakeLower();
		bReturn = strLValue.Find(strRValue,0) < 0;
		break;
	case CON_OR: //non exculisve or
		nA = atoi(strLValue);
		nB = atoi(strRValue);
		bReturn = nA || nB;
		break;
	case CON_AND: // and
		nA = atoi(strLValue);
		nB = atoi(strRValue);
		bReturn = nA && nB;
		break;
	default:
		ASSERT(FALSE);
	}

	DEBUG_TRACE("Condition: %d lvalue=%s rvalue=%s -> result=%d", nCondition, strLValue, strRValue, bReturn);
	return bReturn;
}

// Check if a string contains a substring with wildcard search
// return -1 if not found and 0 based index if found
int CMetis::ContainsStringExact(CString strLine, CString strSubString, BOOL bCase, BOOL bExactMatch)
{
	
    if(strLine.IsEmpty()) return FALSE;
	if(strSubString.IsEmpty()) return TRUE;
	int nReturn = -1;
	BOOL bFound = FALSE;
	if(!bCase){

		strLine.MakeLower();
		strSubString.MakeLower();
	}
	if(bExactMatch){

		if(strLine == strSubString){

			return 0;
		}
		else{

			return -1;
		}
	}

	CString strSearch = strSubString;
	strSearch.Remove('*');
	
	int nIndex = -1;
	BOOL bFront, bEnd;

	if((nIndex = strLine.Find(strSearch, 0)) >= 0){

		//Check if there is a space or ( at front and end of searched item
		bEnd = (nIndex + strSearch.GetLength() >= strLine.GetLength() ? TRUE : strLine.GetAt(strSearch.GetLength() + nIndex) == ' ');
		bFront = (nIndex > 0 ? strLine.GetAt(nIndex-1) == ' ' : TRUE);

		if((strSubString.GetAt(0) == '*') && (strSubString.GetAt(strSubString.GetLength()-1) == '*')){

			bFound = TRUE;
		}
		else if((strSubString.GetAt(0) == '*') && (strSubString.GetAt(strSubString.GetLength()-1) != '*')){

			bFound = bEnd;
		}
		else if((strSubString.GetAt(0) != '*') && (strSubString.GetAt(strSubString.GetLength()-1) == '*')){

			bFound = bFront;
		}
		else{

			bFound = bFront && bEnd;
		}
		if(bFound){
			
			nReturn = nIndex;
			DEBUG_TRACE(" -- In Call ContainsStringExact():\n  ---- %s, found in %s at %d", strSubString, strLine, nIndex);
		}
	}
	return nReturn;
}

// Check if a user is blocked (badword penalty)
BOOL CMetis::IsUserBlocked(CString strNickname)
{
	BOOL bReturn = FALSE;
	BOOL bFound = FALSE;
	if(strNickname == m_strBotname) return TRUE;

	for(int i = 0; i < m_aUserInfo.GetSize(); i++){

		if(m_aUserInfo.GetAt(i).strName == strNickname){

			bFound = TRUE;
			break;
		} // end ... == nickname
	}// end for


	// User has not been posting yet
	if(!bFound){

		USER_INFO u;
		u.nCount      = 1;
		u.nFirst      = GetTickCount();
		u.strName     = strNickname;
		u.nBlockStart = -1;
		u.nTriviaScore = 0;
		m_aUserInfo.Add(u);
	}
	else{

		// User is already blocked
		if(m_aUserInfo.GetAt(i).nBlockStart != -1){

			if((GetTickCount() - m_aUserInfo.GetAt(i).nBlockStart) <= (UINT)m_nBlockTime){
				
				// Blocked and still in blocktime
				TRACE(strNickname + " is blocked\n");
				bReturn = TRUE;
			}
			else{

				// Blocked but exceeded blocktime so release him
				TRACE(strNickname + " is now unblocked\n");
				DEBUG_TRACE("> User Flood Protection: %s is now unblocked", strNickname);
				USER_INFO u = m_aUserInfo.GetAt(i);
				u.nCount = 1;
				u.nFirst = GetTickCount();
				u.nBlockStart = -1;
				m_aUserInfo.SetAt(i, u);
				bReturn = FALSE;
			}
		}
		// User is not yet blocked
		else{
			
			if((GetTickCount() - m_aUserInfo.GetAt(i).nFirst) <= (UINT)m_nBlockRange){

				// User has not exceeded blockrange so push him one up
				TRACE(strNickname + " pushed a notch\n");
				USER_INFO u = m_aUserInfo.GetAt(i);
				u.nCount++;
				m_aUserInfo.SetAt(i, u);
				bReturn = FALSE;
			}
			else{

				// enough time between posts
				USER_INFO u = m_aUserInfo.GetAt(i);
				u.nBlockStart = -1;
				u.nCount = 1;
				u.nFirst = GetTickCount();
				m_aUserInfo.SetAt(i, u);
				bReturn = FALSE;
			}
			if(m_aUserInfo.GetAt(i).nCount >= m_nBlockMessageCount){

				DEBUG_TRACE("> User Flood Protection: %s is now blocked", strNickname);
				USER_INFO u = m_aUserInfo.GetAt(i);
				u.nBlockStart = GetTickCount();
				m_aUserInfo.SetAt(i, u);
				bReturn = TRUE;
			}
		}
	}

	return bReturn;
}

void CMetis::ClearGame()
{

	m_aGame.SetSize(0);
	m_bTrivia = FALSE;
	m_pCurrent = NULL;
	m_nCurrent = 0;
	m_nRoundEnd			= -1;
	m_nRoundCurrent		= 0;

}

// Clean up. delete entries of all arrays
void CMetis::ClearRandom()
{

	while(m_aCommands.GetSize()){

		PCOMMAND pCmd = m_aCommands.GetAt(m_aCommands.GetSize()-1);
		m_aCommands.RemoveAt(m_aCommands.GetSize()-1);
		pCmd->aResponses.RemoveAll();
		pCmd->aTrigger.RemoveAll();
		delete pCmd;
		pCmd = NULL;
	}
	while(m_aOpCommands.GetSize()){

		PCOMMAND pCmd = m_aOpCommands.GetAt(m_aOpCommands.GetSize()-1);
		m_aOpCommands.RemoveAt(m_aOpCommands.GetSize()-1);
		pCmd->aResponses.RemoveAll();
		pCmd->aTrigger.RemoveAll();
		delete pCmd;
		pCmd = NULL;
	}
	while(m_aPM.GetSize()){

		PCOMMAND pCmd = m_aPM.GetAt(m_aPM.GetSize()-1);
		m_aPM.RemoveAt(m_aPM.GetSize()-1);
		pCmd->aResponses.RemoveAll();
		pCmd->aTrigger.RemoveAll();
		delete pCmd;
		pCmd = NULL;
	}
	while(m_aEnter.GetSize()){

		PCOMMAND pCmd = m_aEnter.GetAt(m_aEnter.GetSize()-1);
		m_aEnter.RemoveAt(m_aEnter.GetSize()-1);
		pCmd->aResponses.RemoveAll();
		pCmd->aTrigger.RemoveAll();
		delete pCmd;
		pCmd = NULL;
	}
	while(m_aLeave.GetSize()){

		PCOMMAND pCmd = m_aLeave.GetAt(m_aLeave.GetSize()-1);
		m_aLeave.RemoveAt(m_aLeave.GetSize()-1);
		pCmd->aResponses.RemoveAll();
		pCmd->aTrigger.RemoveAll();
		delete pCmd;
		pCmd = NULL;
	}
	while(m_aBadWords.GetSize()){

		PBADWORD pCmd = m_aBadWords.GetAt(m_aBadWords.GetSize()-1);
		m_aBadWords.RemoveAt(m_aBadWords.GetSize()-1);
		delete pCmd;
		pCmd = NULL;
	}
	POSITION pos = m_mUserGroups.GetStartPosition();
	while(pos){

		PUSER_GROUP pCmd; CString strTmp;
		m_mUserGroups.GetNextAssoc(pos, strTmp, pCmd);
		m_mUserGroups.RemoveKey(strTmp);
		delete pCmd;
		pCmd = NULL;
	}
	while(m_aTrivia.GetSize()){

		PTRIVIA pCmd = m_aTrivia.GetAt(m_aTrivia.GetSize()-1);
		m_aTrivia.RemoveAt(m_aTrivia.GetSize()-1);
		pCmd->aResponse.RemoveAll();
		pCmd->strTrigger.Empty();
		delete pCmd;
		pCmd = NULL;
	}

	m_aGame.SetSize(0);
	cOnJoinRoom.aResponses.RemoveAll();
	cOnJoinRoom.nIn  = -1;
	cRename.aResponses.RemoveAll();
	cRename.nIn  = -1;
	cAuto.nIn = -1;
	cAuto.aResponses.RemoveAll();
}

// Get Nickname
CString CMetis::GetNick(CString strName)
{

	int nEnd = 0;

	for(int i = 0; i < m_strSeperator.GetLength(); i++){

		nEnd = strName.Find(m_strSeperator[i]);
		if(nEnd > NULL){

			return strName.Left(nEnd);
		}
	}
	return strName;
}

// Check if user is part of a nickname collection assigned to a command
BOOL CMetis::NickApplies(CString strGroup, CString strNick, BOOL bCase)
{

	BOOL bReturn = FALSE;
	CString strTmp;
	int nStart = 0, nEnd = 0;
	
	if(strGroup.IsEmpty()) return FALSE;
	strGroup += ";|;";

	while((nEnd = strGroup.Find(";|;", nStart)) > NULL){

		strTmp = strGroup.Mid(nStart, nEnd - nStart);
		nStart = nEnd+3;
		if(ContainsStringExact(strNick, strTmp, bCase) >= 0){

			bReturn = TRUE;
			break;
		}
	}

	return bReturn;
}

// Check if user is part of group assigned to a command
BOOL CMetis::GroupApplies(CString strGroup, CString strNick)
{

	if(strGroup.IsEmpty()) return FALSE;
	
	BOOL bExclude = strGroup[0] == '!';
	if(bExclude){

		strGroup = strGroup.Mid(1);
	}

	PUSER_GROUP pUg = GetUserGroup(strGroup);
	
	if(pUg == NULL){
	
		return FALSE;
	}
	if(pUg->strUsers.IsEmpty()){

		return FALSE;
	}

	BOOL bReturn = NickApplies(pUg->strUsers, strNick, pUg->bCase);
	if(bExclude) bReturn = !bReturn;

	return bReturn;
}

// Get usergroup with given name
PUSER_GROUP CMetis::GetUserGroup(CString strGroup)
{

	/*for(int i = 0; i < m_aUserGroups.GetSize(); i++){


		if(m_aUserGroups[i]->strGroup == strGroup){

			return m_aUserGroups[i];
		}
	}*/
	PUSER_GROUP group = NULL;
	if(m_mUserGroups.Lookup(strGroup, group)){

		return group;
	}
	return NULL;
}

// Get room name assigned to ID
CString CMetis::GetRoomName(DWORD dwID)
{

	for(int i = 0; i < m_aRooms.GetSize(); i++){

		if(m_aRooms[i].dwID == dwID) return m_aRooms[i].strRoomName;
	}
	return "";
}

// Add a room
BOOL CMetis::AddRoom(DWORD dwID, CString strName, CUserMap* pUserMap, CString strServer, CString strVer)
{

	for(int i = 0; i < m_aRooms.GetSize(); i++){

		if(m_aRooms[i].dwID == dwID){
			
			m_aRooms[i].strRoomName = strName;
			m_aRooms[i].strServer   = strServer;
			m_aRooms[i].strVersion  = strVer;
			m_aRooms[i].pUserMap  = pUserMap;
			return TRUE;
		}
	}

	HWND hTmp   = ::GetNextWindow((HWND)dwID, GW_HWNDFIRST|GW_CHILD);
	HWND hFirst = hTmp;
	while(::GetDlgCtrlID(hTmp) != 1038){

		TRACE("ID=%04d\n", ::GetDlgCtrlID(hTmp) != 1038);
		hTmp = ::GetNextWindow(hTmp, GW_HWNDNEXT);
		if(hTmp == hFirst){

			DisplayToolTip("Fatal: Metis could not subclass Channelwindow\n", 1000, NIIF_ERROR);
			break;
		}
	}

	ROOMDATA r;
	r.dwID = dwID;
	r.strRoomName		= strName;
	r.pUserMap			= pUserMap;
	r.bIsBotActivated	= FALSE;
	r.hChat				= hTmp;
	r.strVersion		= strVer;
	r.strServer			= strServer;
	SubclassChat(hTmp);
	m_aRooms.Add(r);
	Beep(2800, 100);
	Beep(2000, 100);
	Beep(1500, 100);
	
	return FALSE;
}

// Get ID of channel by Window Handle of chatroom
DWORD CMetis::GetID(HWND hChat)
{

	for(int i = 0; i < m_aRooms.GetSize(); i++){

		if(m_aRooms[i].hChat == hChat){

			return m_aRooms[i].dwID;
		}
	}
	return 0;
}	 

// Get room assigned to ID
ROOMDATA& CMetis::GetRoom(DWORD dwID)
{

	for(int i = 0; i < m_aRooms.GetSize(); i++){

		if(m_aRooms[i].dwID == dwID){

			break;
		}
	}
	return m_aRooms[i];
}

// remove room assigned to ID
void CMetis::RemoveRoom(DWORD dwID)
{

	for(int i = 0; i < m_aRooms.GetSize(); i++){

		if(m_aRooms[i].dwID == dwID){

			m_aRooms.RemoveAt(i);
			return;
		}
	}
}

// Replace all user variables
void CMetis::ReplaceUservars(CString &rString)
{

	CString strKey, strValue;
	
	// handle old style ... *sigh*
	while(rString.Find(_T("%USERVAR["),0) != -1){

		int nStart = rString.Find(_T("%USERVAR["),0)+9;
		strKey = rString.Mid(nStart, rString.Find(_T("]"), nStart)-nStart);

		if(m_mUserVar.Lookup(strKey, strValue)){

			if(rString.Replace("%USERVAR[" + strKey + "]%", strValue) == 0){

				DisplayToolTip("There appears to be a malformed Uservariable in this string: \n" +
					           rString, 10000, 3);
				return;
			}
		}
		else if(rString.Replace("%USERVAR[" + strKey + "]%", "") == 0){

			return;
		}   
	}

	// handle new style :-)
	int nStart = 0, nEnd = -1;
	while((nStart = rString.Find(_T("$"), nStart)) != -1){

		//nStart = rString.Find(_T("$"),0)+1;
		strKey = rString.Mid(nStart+1, rString.Find(_T("$"), nStart+1)-nStart-1);
		nStart+=1;
		if(m_mUserVar.Lookup(strKey, strValue)){

			if(rString.Replace("$" + strKey + "$", strValue) == 0){

				DisplayToolTip("There appears to be a malformed Uservariable in this string: \n" +
					           rString, 10000, 3);
				return;
			}
		}
		// uservariable is empty...
        else if(rString.Replace("$" + strKey + "$", "") == 0){

			return;
		}
	}
}


void CMetis::ReplaceHostname(CString &strString)
{

	if(strString.Find("%USERVAR[", 0) >= 0){

		ReplaceUservars(strString);
	}
	while(strString.Find(_T("%HOSTNAME["),0) != -1){

		int nStart = strString.Find(_T("%HOSTNAME["),0)+10;
		
		CString strIP = strString.Mid(nStart, strString.Find(_T("]"), nStart)-nStart);
		
		CString strHost;
		HOSTENT *host = NULL;
		if(inet_addr(strIP)==INADDR_NONE){

			DEBUG_TRACE("Not a valid IP address: %s", strIP);
		}
		else{

			unsigned long addr = inet_addr(strIP);
			host=gethostbyaddr((char*)&addr,sizeof(addr),AF_INET);
		}
		if(host == NULL){

			DEBUG_TRACE("Could not resolve hostname of %s", strIP);
			strHost = strIP;
		}
		else{

			strHost = host->h_name;
			DEBUG_TRACE("Hostname: %s %s", host->h_name);

		}

		strString.Replace("%HOSTNAME[" + strIP + "]%", strHost);
	}
}

BOOL CMetis::GetExtWinampData(CString& rData)
{

	BOOL bReturn = FALSE;
	
	HWND hwndExt = ::FindWindow(NULL, "ExtWinampReportv1.1");
	
	if(hwndExt != NULL){ // found the window of our plugin
	
		if(::SendMessage(hwndExt, WM_UPDATE_INFO, 0, 0) == 3){ 
			
			// successfully asked to update the tag info

			CMemMapFile mmf;

			// open memory file with the tag
			if(mmf.MapExistingMemory(_T("ROBOMXREPORT"), (MAX_LENGTH+1)*sizeof(TCHAR))){

				TCHAR tcText[MAX_LENGTH+1];
				_tcscpy(tcText, _T(""));
				
				LPVOID lpData = mmf.Open();
				if(lpData){

					// we successfully got a pointer to the 
					// content of the memory file
					_tcscpy(tcText, (TCHAR*)lpData);
					rData = tcText;
					bReturn = TRUE;
				}
				
				// close the memory file again
				mmf.Close();
			}
		}
	}
	return bReturn;
}

// Replace all variables
void CMetis::ReplaceVars(CString &strString, CString strName, DWORD dwID)
{

	if(strString.Find("%",0) < 0) return;

	strName.Replace("&lowast;", "*");
	strName.Replace("&sect;", "$");
	strName.Replace("&perc;", "%");
	// Replace identifiers
	// %RAWNAME[strName]% 
	CString strRawname;
	if(m_aHalfID.Lookup(strName, strRawname) == FALSE){

		if(m_aNoID.Lookup(strName, strRawname) == FALSE){

			strRawname = strName;
		}
	}
	while(strString.Find(_T("%RAWNAME["),0) != -1){

		int nStart = strString.Find(_T("%RAWNAME["),0)+9;
		
		CString strTmp = strString.Mid(nStart, strString.Find(_T("]"), nStart)-nStart);
		
		strString.Replace("%RAWNAME[" + strTmp + "]%", GetRawName(dwID, strTmp));
		
		if(strTmp.Find("%RAWNAME", 0) >= 0) break;
	}
	
	CString strNum;
	
	// %RANDOMNUM%
	strNum.Format("%d", rand() % 2000);
	strString.Replace(_T("%RANDOMNUM%"), strNum);

	// %RANDOMNUM[nMax]%
	while(strString.Find(_T("%RANDOMNUM["),0) != -1){

		int nStart = strString.Find(_T("%RANDOMNUM["),0)+11;
		CString strTmp = strString.Mid(nStart, strString.Find(_T("]"), nStart)-nStart);
		int nMax = atoi(strTmp);	
		CString strNum;
		strNum.Format("%d", rand() % nMax);
		strString.Replace("%RANDOMNUM[" + strTmp + "]%", strNum);
	}
	

	strNum.Format("%d",  GetRoom(dwID).pUserMap->GetSize());
	strString.Replace(_T("%NUMUSERS%"), strNum);

	strNum.Format("%d",  m_nExecError);
	strString.Replace(_T("%EXEC_ERROR%"), strNum);
	strNum.Format("%d",  m_nExecCode);
	strString.Replace(_T("%OUT_EXEC_CODE%"), strNum);
	strNum.Format("%d", GetTickCount());
	strString.Replace(_T("%TICKCOUNT%"), strNum);
	
	strString.Replace("%HOSTNAME%", GetHostname(dwID, strRawname));

	if(strString.Find("%HOSTNAME", 0) >= 0){

		ReplaceHostname(strString);
	}

	// Winamp stuff:
	if(strString.Find("%WA-", 0) >= 0){

		HWND hwndWinamp = ::FindWindow("Winamp v1.x",NULL);
		int nIsVideoPlaying = (int)::SendMessage(hwndWinamp, WM_WA_IPC, 0, IPC_IS_PLAYING_VIDEO);

		CString strArtist, strFile, strMode, strVidRes, strSong, strIsVideo, strVersion, strPlayTime, strTotalTime, strRemTime, strSampleRate, strBitrate, strNumChannels, strStatus = "not running", strAlbum, strYear, strGenre, strTrack, strComment;
		if(hwndWinamp != NULL){

			CString strTmp, strData;


			// Get Song and Artist playing in
			// Winamp. This is for videoplayback
			// and in case the advanced lookup fails
			strTmp = GetWinampSong();
			
			int nIndex = strTmp.Find("-", 0);
			if(nIndex > 0){

				strArtist	= strTmp.Left(nIndex);
				strArtist.TrimRight();
				strArtist.TrimLeft();
				strSong		= strTmp.Mid(nIndex+1);
				strSong.TrimLeft();
				strSong.TrimRight();
			}
			

			if(nIsVideoPlaying <= 1){

				// If Winamp does not play a video, get 
				// all the ID3-/ Ogg-Tag entries from 
				// our Winamp Plugin :-)
				BOOL bSuccess = GetExtWinampData(strTmp);
				
				if(bSuccess){ // Was successfull

					CTokenizer token(strTmp, "\n");
					if(token.Next(strTmp)){
				
						strTmp.TrimRight();
						strTmp.TrimLeft();
						if(strTmp.GetLength())   // If this entry is OK copy it, 
							strArtist =  strTmp; // Otherwise we use the old value...
					}
					if(token.Next(strTmp)){
				
						strTmp.TrimRight();
						strTmp.TrimLeft();
						if(strTmp.GetLength())  // If this entry is OK copy it, 
							strSong = strTmp;   // Otherwise we use the old value...
					}
					if(token.Next(strTmp)){
				
						strTmp.TrimRight();
						strAlbum = strTmp;
					}
					if(token.Next(strTmp)){
				
						strTmp.TrimRight();
						strYear = strTmp;
					}
					if(token.Next(strTmp)){
				
						strTmp.TrimRight();
						strGenre = strTmp;
					}
					if(token.Next(strTmp)){
				
						strTmp.TrimRight();
						strTrack = strTmp;
					}
					if(token.Next(strTmp)){
				
						strTmp.TrimRight();
						strFile = strTmp;
					}
					if(token.Next(strTmp)){
				
						strTmp.TrimRight();
						strComment = strTmp;
					}
				}

				strVersion.Format("%x", ::SendMessage(hwndWinamp, WM_WA_IPC, 0, IPC_GETVERSION));
				strVersion.SetAt(1, '.');

				int nTotal = 0, nRem = 0, nEla = 0;
				nTotal = (int)::SendMessage(hwndWinamp, WM_WA_IPC, 1, IPC_GETOUTPUTTIME);
				strTotalTime.Format("%02d:%02d", nTotal/60, nTotal%60);

				nEla= (int)::SendMessage(hwndWinamp, WM_WA_IPC, 0, IPC_GETOUTPUTTIME) / 1000;
				strPlayTime.Format("%02d:%02d", nEla/60, nEla%60);
				
				nRem = nTotal - nEla;
				strRemTime.Format("%02d:%02d", nRem/60, nRem%60);

				strSampleRate.Format("%d", ::SendMessage(hwndWinamp, WM_WA_IPC, 0, IPC_GETINFO));
				strBitrate.Format("%d", ::SendMessage(hwndWinamp, WM_WA_IPC, 1, IPC_GETINFO));
				strNumChannels.Format("%d", ::SendMessage(hwndWinamp, WM_WA_IPC, 2, IPC_GETINFO));

				switch(::SendMessage(hwndWinamp, WM_WA_IPC, 0, IPC_ISPLAYING)){

				case 1: strStatus = "playing";
					break;
				case 3: strStatus = "paused";
					break;
				default: strStatus = "stopped";
				}

				switch(nIsVideoPlaying){

				case 0: strIsVideo.Format("%d", 0);
					break;
				case 1: strIsVideo.Format("%d", 1);
					break;
				default : strIsVideo.Format("%d", 2);
				}

				if(nIsVideoPlaying > 1){

					strMode  = "Video";
					int nRes = (int)::SendMessage(hwndWinamp, WM_WA_IPC, 3, IPC_GETINFO);
					strVidRes.Format("%dx%d", LOWORD(nRes), HIWORD(nRes));
				}
				else{

					strMode		= "Audio";
					strVidRes	= "";
				}
			}
			strString.Replace(_T("%WA-ARTIST%"), strArtist);
			strString.Replace(_T("%WA-SONG%"), strSong);
			strString.Replace(_T("%WA-ALBUM%"), strAlbum);
			strString.Replace(_T("%WA-TRACK%"), strTrack);
			strString.Replace(_T("%WA-YEAR%"), strYear);
			strString.Replace(_T("%WA-GENRE%"), strGenre);
			strString.Replace(_T("%WA-COMMENT%"), strComment);
			strString.Replace(_T("%WA-VERSION%"), strVersion);
			strString.Replace(_T("%WA-ELATIME%"), strPlayTime);
			strString.Replace(_T("%WA-REMTIME%"), strRemTime);
			strString.Replace(_T("%WA-TOTALTIME%"), strTotalTime);
			strString.Replace(_T("%WA-SAMPLERATE%"), strSampleRate);
			strString.Replace(_T("%WA-BITRATE%"), strBitrate);
			strString.Replace(_T("%WA-CHANNELS%"), strNumChannels);
			strString.Replace(_T("%WA-STATUS%"), strStatus);
			strString.Replace(_T("%WA-ISVIDEO%"), strIsVideo);
			strString.Replace(_T("%WA-VIDRES%"), strVidRes);
			strString.Replace(_T("%WA-FILE%"), strFile);
			strString.Replace(_T("%WA-MODE%"), strMode);
		}// end if winamp != 0

	}
	
	// any RMX-Query-API variables?
	if(strString.Find("%RMX-", 0) >= 0){

		DWORD dwNum = (DWORD)QueryInformation(RMX_GETNUMCHANNELS, 0, 0);
		strString.Replace("%RMX-CH-NUMCHANNELS%", FormatInt(dwNum));
		dwNum = (DWORD)QueryInformation(RMX_GETNUMFILTERCHANNELS, 0, 0);
		strString.Replace("%RMX-CH-NUMFILTERCHANNELS%", FormatInt(dwNum));
		dwNum = (DWORD)QueryInformation(RMX_GETNUMWINMX, 0, 0);
		strString.Replace("%RMX-CH-NUMWINMX%", FormatInt(dwNum));
		dwNum = (DWORD)QueryInformation(RMX_GETNUMRCMS, 0, 0);
		strString.Replace("%RMX-CH-NUMRCMS%", FormatInt(dwNum));
		dwNum = (DWORD)QueryInformation(RMX_GETNUMROSE, 0, 0);
		strString.Replace("%RMX-CH-NUMROSE%", FormatInt(dwNum));
		dwNum = (DWORD)QueryInformation(RMX_GETNUMMCMA, 0, 0);
		strString.Replace("%RMX-CH-NUMMCMA%", FormatInt(dwNum));
		dwNum = (DWORD)QueryInformation(RMX_GETNUMMCS, 0, 0);
		strString.Replace("%RMX-CH-NUMMCS%", FormatInt(dwNum));
		dwNum = (DWORD)QueryInformation(RMX_GETNUMWCS, 0, 0);
		strString.Replace("%RMX-CH-NUMWCS%", FormatInt(dwNum));
		dwNum = (DWORD)QueryInformation(RMX_GETNUMFXSERV, 0, 0);
		strString.Replace("%RMX-CH-NUMFXSERV%", FormatInt(dwNum));
		dwNum = (DWORD)QueryInformation(RMX_GETNUMWPNUSERS, 0, 0);
		strString.Replace("%RMX-CH-NUMWPNUSERS%", FormatInt(dwNum));

		dwNum = (DWORD)QueryInformation(RMX_GETAVERAGELAG, dwID, 0);
		strString.Replace("%RMX-RM-AVERAGELAG%", FormatInt(dwNum));
		dwNum = (DWORD)QueryInformation(RMX_GETRECVMSGS, dwID, 0);
		strString.Replace("%RMX-RM-RECVMSGS%", FormatInt(dwNum));
		dwNum = (DWORD)QueryInformation(RMX_GETSENTMSGS, dwID, 0);
		strString.Replace("%RMX-RM-SENTMSGS%", FormatInt(dwNum));
	}

	strString.Replace(_T("%IP%"), GetIP(dwID, strRawname));
	strString.Replace(_T("%FILES%"), GetFiles(dwID, strRawname));
	strString.Replace(_T("%LINE%"), GetLineType(dwID, strRawname));
	strString.Replace(_T("%VERSION%"), STR_VERSION);
	strString.Replace(_T("%LOCALTIME%"), GetMyLocalTime());
	
	MX_USERINFO user;
	if(GetRoom(dwID).pUserMap->Lookup(strRawname, user)){

		strString.Replace(_T("%JOINTIME%"), user.cJoinTime.Format(_T("%H:%M:%S, %m/%d/%y")));
		strString.Replace(_T("%IDLETIME%"), FormatIdleTime(user.dwIdleTime));
		strString.Replace(_T("%STAYTIME%"), FormatStayTime(user.dwStayTime));
		strString.Replace(_T("%USERMODE%"), FormatUserlevel(user.wUserLever));
		strString.Replace(_T("%LASTMSG%"), user.strLastMsg);
	}
	
	strString.Replace(_T("%DATE%"), GetMyDate(FALSE));
	strString.Replace(_T("%LONGDATE%"), GetMyDate(TRUE));

	SYSTEMTIME time;
	GetLocalTime(&time);
	strNum.Format("%d", time.wDayOfWeek);
	strString.Replace(_T("%DAY-OF-WEEK%"), strNum);
	strNum.Format("%d", time.wDay);
	strString.Replace(_T("%DAY%"), strNum);
	strNum.Format("%d", time.wYear);
	strString.Replace(_T("%YEAR%"), strNum);
	strNum.Format("%d", time.wMonth);
	strString.Replace(_T("%MONTH%"), strNum);
	strNum.Format("%d", time.wHour);
	strString.Replace(_T("%HOUR%"), strNum);
	strNum.Format("%d", time.wMinute);
	strString.Replace(_T("%MINUTE%"), strNum);
	strNum.Format("%d", time.wSecond);
	strString.Replace(_T("%SECOND%"), strNum);
	strNum.Format("%d", time.wMilliseconds);
	strString.Replace(_T("%MILLISECOND%"), strNum);



	strString.Replace(_T("%INETBEATS%"), GetInetBeats());
	strString.Replace(_T("%SYSTEMUPTIME%"), GetSystemUptime());
	strString.Replace(_T("%BOTUPTIME%"), GetBotUptime());
	strString.Replace(_T("%BOTNAME%"), GetName(m_strBotname));
	strString.Replace(_T("%BOTRAWNAME%"), GetRawName(dwID, m_strBotname));
	strString.Replace(_T("%ROOMNAME%"), GetRoomName(dwID));
	strString.Replace(_T("%SERVER%"), GetRoom(dwID).strServer);
	strString.Replace(_T("%SERVERVERSION%"), GetRoom(dwID).strVersion);
	strString.Replace(_T("%NAME%"), GetName(strName));
	
	strString.Replace(_T("%RAWNAME%"), GetRawName(dwID, strName));
	strString.Replace(_T("%NICK%"), GetNick(strName));
}

// hexstring -> integer
int axtoi(char *hexStg)
{

  int n = 0;         // position in string
  int m = 0;         // position in digit[] to shift
  int count;         // loop index
  int intValue = 0;  // integer value of hex string
  int digit[5];      // hold values to convert
  while (n < 2) {
     if (hexStg[n]=='\0')
        break;
     if (hexStg[n] > 0x29 && hexStg[n] < 0x40 ) //if 0 to 9
        digit[n] = hexStg[n] & 0x0f;            //convert to int
     else if (hexStg[n] >='a' && hexStg[n] <= 'f') //if a to f
        digit[n] = (hexStg[n] & 0x0f) + 9;      //convert to int
     else if (hexStg[n] >='A' && hexStg[n] <= 'F') //if A to F
        digit[n] = (hexStg[n] & 0x0f) + 9;      //convert to int
     else break;
    n++;
  }
  count = n;
  m = n - 1;
  n = 0;
  while(n < count) {
     // digit[n] is value of hex digit at position n
     // (m << 2) is the number of positions to shift
     // OR the bits into return value
     intValue = intValue | (digit[n] << (m << 2));
     m--;   // adjust the position to set
     n++;   // next digit to process
  }

  return (intValue);
}

// translate non-dotted reversed IP to dotted IP
CString CMetis::TranslateIP(DWORD dwIP)
{

	// 109.253.0.64 <-> 64.253.109.115
	if(dwIP == 0) return "0.0.0.0";
	CString strIP;
	strIP.Format("%X", dwIP);
	
	int nA = 0, nB = 0, nC = 0, nD = 0;
	
	nA = axtoi((LPSTR)(LPCSTR)strIP.Mid(0,2));
	nB = axtoi((LPSTR)(LPCSTR)strIP.Mid(2,2));
	nC = axtoi((LPSTR)(LPCSTR)strIP.Mid(4,2));
	nD = axtoi((LPSTR)(LPCSTR)strIP.Mid(6,2));
	

	strIP.Format("%d.%d.%d.%d", nD, nC, nB, nA);
	return strIP;

}

void CMetis::WriteMyEchoText(DWORD dwID, LPCTSTR strText, COLORREF tc)
{

	WriteEchoText(dwID, strText, tc, m_crBg);
}


// Shell Execute with parameter handling
void CMetis::MyShellExecute(CString strCmd, BOOL bWait)
{

	ReplaceUservars(strCmd);

	HANDLE hProcess = NULL;
	PROCESS_INFORMATION processInfo;
	STARTUPINFO startupInfo;
	::ZeroMemory(&startupInfo, sizeof(startupInfo));
	startupInfo.cb = sizeof(startupInfo);

	//if(::CreateProcess((LPTSTR)(LPCTSTR)strExec, (LPTSTR)(LPCTSTR)strParam, 
	if(::CreateProcess(NULL, (LPTSTR)(LPCTSTR)strCmd,
					   NULL,  // process security
					   NULL,  // thread security
					   FALSE, // no inheritance
					   0,     // no startup flags
					   NULL,  // no special environment
					   NULL,  // default startup directory
					   &startupInfo,
					   &processInfo))
	{ // success 
	
		m_nExecError = 0;
		hProcess = processInfo.hProcess;

		if(bWait){

			::WaitForSingleObject(hProcess, INFINITE);
			::GetExitCodeProcess(hProcess, &m_nExecCode);
			::CloseHandle(hProcess);
		}
		else{

			m_nExecCode = 0;
			::CloseHandle(hProcess);
		}
	} 
	else{ // error

		m_nExecError = GetLastError();
		m_nExecCode = 0;
	}
}

CString CMetis::Operator(int nType, CString lValue, CString rValue, CString nValue)
{
	
	CString strResult;
	
	int nResult = 0;
	int nLValue = 0;
	int nRValue = 0;
	if(nType < 15){

		nLValue = atoi(lValue);
		nRValue = atoi(rValue);
	}

	switch(nType){

	case OP_MINUS: // minus
		nResult = nLValue - nRValue;
		break;
	case OP_PLUS: // plus
		nResult = nLValue + nRValue;
		break;
	case OP_MINUSMINUS:
		nResult = nLValue - 1;
		break;
	case OP_PLUSPLUS:
		nResult = nLValue + 1;
		break;
	case OP_TIMES: // times
		nResult = nLValue * nRValue;
		break;
	case OP_DIVIDE: // divide
		if(nRValue != 0)
			nResult = nLValue / nRValue;
		else
			return "Error: Division by zero";
		break;
	case OP_MODULO: // modulo
		if(nRValue != 0)
			nResult = nLValue % nRValue;
		else
			return "Error: Division by zero";
		break;
	case OP_POWER: // power
		nResult = (int)pow(nLValue, nRValue);
		break;
	case OP_OR: // non exclusive or
		nResult = nLValue || nRValue;
		break;
	case OP_AND: // and
		nResult = nLValue && nRValue;
		break;
	case OP_NOT: // logical not
		nResult = !nLValue;
		break;
	case OP_SMALLER: // smaller
		nResult = nLValue < nRValue;
		break;
	case OP_EQUAL: // equal
		nResult = nLValue == nRValue;
		break;
	case OP_NOTEQUAL: // not equal
		nResult = nLValue != nRValue;
		break;
	case OP_GREATER: // greater
		nResult = nLValue > nRValue;
		break;
	case OP_STR_LEN: // strlen
		nResult = lValue.GetLength();
		break;
	case OP_STR_TRIMM: // strtrm
	case OP_STR_LEFT: // strleft
		if(atoi(rValue) <= lValue.GetLength()){

			strResult = lValue.Left(atoi(rValue));
		}
		else strResult = lValue;
		break;
	case OP_STR_LOWER: // strlwr
		strResult = lValue;
		strResult.MakeLower();
		break;
	case OP_STR_UPPER: // strupr
		strResult = lValue;
		strResult.MakeUpper();
		break;
	case OP_STR_RIGHT: // strright
		if(atoi(rValue) < lValue.GetLength()){

			strResult = lValue.Mid(atoi(rValue));
		}
		else strResult = lValue;
		break;
	case OP_STR_FIND: // strfind
		nResult = lValue.Find(rValue, 0);
		strResult.Format("%d", nResult);
		break;
	case OP_STR_REVERSE_FIND: // strrfind
		nResult = lValue.ReverseFind(rValue.GetAt(0));
		strResult.Format("%d", nResult);
		break;
	case OP_STR_APPEND:
		strResult = lValue + rValue;
		break;
	case OP_STR_REMOVE:
		strResult = lValue;
		strResult.Replace(rValue, "");
		break;
	case OP_STR_REPLACE:
		strResult = nValue;
		strResult.Replace(lValue, rValue);
		break;
	case OP_READ_FILE:
		strResult = ReadFromFile(nValue, lValue, rValue);
		break;
	case OP_READ_WEB:
		strResult = ReadFromWeb(nValue, lValue, rValue);
		break;
	default: // error
		ASSERT(FALSE);
		break;
	}
	
	if(nType <= OP_STR_LEN){

		strResult.Format("%d", nResult);
	}
	return strResult;
}

BOOL CMetis::SubclassChat(HWND hWnd)
{

	WNDPROC OldWndProc = (WNDPROC)::SetWindowLong(hWnd, GWL_WNDPROC, (LONG)SubChatProc);
	if(OldWndProc && ((LRESULT)OldWndProc != (LRESULT)SubChatProc)){

		::SetWindowLong(hWnd, GWL_USERDATA, (LONG)OldWndProc);
		return TRUE;
	}
	else return FALSE;
}


LRESULT CALLBACK CMetis::SubChatProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{

	WNDPROC OldWndProc = (WNDPROC)::GetWindowLong(hwnd, GWL_USERDATA);
	if(!OldWndProc)return FALSE;
	
	::SetCursor(::LoadCursor(NULL, IDC_ARROW));
	if(uMsg == WM_MBUTTONDOWN){

		CMenu *popup = new CMenu;
		CMenu *files = new CMenu;
		BOOL bReturn = TRUE;
		if(popup->CreatePopupMenu() && files->CreatePopupMenu()){

			CFileFind finder;
			int nFile = 0;
			BOOL bResult = finder.FindFile(g_strWd + "\\*.xml");
			while(bResult){

				bResult = finder.FindNextFile();
				files->AppendMenu(MF_STRING, 20000+nFile++, finder.GetFileName());
			}

			popup->AppendMenu(MF_STRING, 10024, "Start Bot");
			popup->AppendMenu(MF_STRING, 10025, "Stop Bot");
			popup->AppendMenu(MF_SEPARATOR, 0, "");
			popup->AppendMenu(MF_STRING, 10026, "Start Trivia");
			popup->AppendMenu(MF_STRING, 10027, "Stop Trivia");
			popup->AppendMenu(MF_SEPARATOR, 0, "");
			popup->AppendMenu(MF_STRING, 10028, "Edit MXC.xml");
			popup->AppendMenu(MF_POPUP, (UINT_PTR)files->m_hMenu, "Edit...");
			popup->AppendMenu(MF_STRING, 10029, "Reload MXC.xml");
			popup->AppendMenu(MF_STRING, 10030, "Print statisics");
			popup->AppendMenu(MF_STRING, 10033,    "Variable Spy");
			popup->AppendMenu(MF_SEPARATOR, 0, "");
			popup->AppendMenu(MF_STRING, 10031, "Help");
			popup->AppendMenu(MF_STRING, 10032, "Visit Metis Homepage");

			POINT curPos;
			::GetCursorPos(&curPos);
			popup->TrackPopupMenu(TPM_RIGHTALIGN | TPM_RIGHTBUTTON | TPM_LEFTBUTTON, curPos.x, curPos.y, CWnd::FromHandle(hwnd), NULL);
			popup->DestroyMenu();
			files->DestroyMenu();
		}
		delete files;
		files = NULL;
		delete popup;
		popup = NULL;
		return TRUE;
	}
	
	else if((uMsg == WM_COMMAND) && (wParam >= 10024) && (wParam <= 10032)){

		DWORD dwID = 0;
		switch(wParam){

		case 10024: //Start
			g_rPlugin.InputMyMessage(g_rPlugin.GetID(hwnd), "/mxc start");
			break;
		case 10025: // Stop
			g_rPlugin.InputMyMessage(g_rPlugin.GetID(hwnd), "/mxc stop");
			break;
		case 10026: // /Start Trivia
			g_rPlugin.InputMyMessage(g_rPlugin.GetID(hwnd), "/mxc trivia start");
			break;
		case 10027: // Stop Trivia
			g_rPlugin.InputMyMessage(g_rPlugin.GetID(hwnd), "/mxc trivia stop");
			break;
		case 10028: // Edit
			g_rPlugin.InputMyMessage(g_rPlugin.GetID(hwnd), "/mxc edit");
			break;
		case 10029: // Load
			g_rPlugin.InputMyMessage(g_rPlugin.GetID(hwnd), "/mxc load");
			break;
		case 10030: // stats
			g_rPlugin.InputMyMessage(g_rPlugin.GetID(hwnd), "/mxc stats");
			break;
		case 10031: // Help
			::ShellExecute(0, "open", g_strWd + "\\docs\\Metis.chm", 0, 0, SW_SHOW);
			break;
		case 10032: // Home
			::ShellExecute(0, "open", "http://mxcontrol.sourceforge.net", 0, 0, SW_SHOW);
			break;
		case 10033: // Spy
			g_rPlugin.InputMyMessage(g_rPlugin.GetID(hwnd), "/mxc spy");
			break;
		default:
			ASSERT(FALSE);
		}
		return TRUE;
	}

	else if((uMsg == WM_COMMAND) && (wParam >= 20000)){


		CFileFind finder;
		int nFile = 20000;
		BOOL bResult = finder.FindFile(g_strWd + "\\*.xml");
		while(bResult){

			bResult = finder.FindNextFile();
			if(nFile++ == wParam){

				g_rPlugin.InputMyMessage(g_rPlugin.GetID(hwnd), "/mxc edit " + finder.GetFileName());
			}

		}
	}
	return ::CallWindowProc(OldWndProc, hwnd, uMsg, wParam, lParam);
}

void CMetis::OnPrepareMenu(DWORD dwID, BOOL bIsUser, HMENU hMenu)
{

	if(!bIsUser){

		m_hMetis = ::CreatePopupMenu();
		m_hFiles = ::CreatePopupMenu();
		if(m_hMetis && m_hFiles){

			int nFile = 0;
			CFileFind finder;
			BOOL bResult = finder.FindFile(g_strWd + "\\*.xml");
			while(bResult){

				bResult = finder.FindNextFile();
				::AppendMenu(m_hFiles, MF_STRING, ID_MENUBASE+20+nFile++, finder.GetFileName());
			}

			::AppendMenu(m_hMetis, MF_STRING, ID_MENUBASE,	"Start Bot");
			::AppendMenu(m_hMetis, MF_STRING, ID_MENUBASE+1,	"Stop Bot");
			::AppendMenu(m_hMetis, MF_SEPARATOR, 0, (LPCTSTR)0);
			::AppendMenu(m_hMetis, MF_STRING, ID_MENUBASE+2,	"Start Metis-Trivia");
			::AppendMenu(m_hMetis, MF_STRING, ID_MENUBASE+3,	"Stop Metis-Trivia");
			::AppendMenu(m_hMetis, MF_SEPARATOR, 0, (LPCTSTR)0);
			::AppendMenu(m_hMetis, MF_STRING, ID_MENUBASE+4,	"Edit MXC.xml");
			::AppendMenu(m_hMetis, MF_POPUP, (UINT_PTR)m_hFiles, "Edit...");
			::AppendMenu(m_hMetis, MF_STRING, ID_MENUBASE+5,	"Reload MXC.xml");
			::AppendMenu(m_hMetis, MF_STRING, ID_MENUBASE+6,	"Print statisics");
			::AppendMenu(m_hMetis, MF_STRING, ID_MENUBASE+9,    "Variable Spy");
			::AppendMenu(m_hMetis, MF_SEPARATOR, 0, (LPCTSTR)0);
			::AppendMenu(m_hMetis, MF_STRING, ID_MENUBASE+7,	"Help");
			::AppendMenu(m_hMetis, MF_STRING, ID_MENUBASE+8,	"Visit Metis Homepage");

			::AppendMenu(hMenu, MF_SEPARATOR, 0, (LPCTSTR)0);
			::AppendMenu(hMenu, MF_POPUP, (UINT_PTR)m_hMetis, "Metis");
		}
	}
}

// Called when a user selects an item from a menu. nCmd is the command identifer, 
// dwID is the ID of the channel. this is -1 if the command is not channel related
void CMetis::OnMenuCommand(UINT nCmd, DWORD dwID, CString strName)
{

	if((dwID > 0) && (nCmd >= ID_MENUBASE) && (nCmd <= ID_MENUBASE+10)){

		switch(nCmd){

		case ID_MENUBASE: //Start
			g_rPlugin.InputMyMessage(dwID, "/mxc start");
			break;
		case ID_MENUBASE+1: // Stop
			g_rPlugin.InputMyMessage(dwID, "/mxc stop");
			break;
		case ID_MENUBASE+2: // /Start Trivia
			g_rPlugin.InputMyMessage(dwID, "/mxc trivia start");
			break;
		case ID_MENUBASE+3: // Stop Trivia
			g_rPlugin.InputMyMessage(dwID, "/mxc trivia stop");
			break;
		case ID_MENUBASE+4: // Edit
			g_rPlugin.InputMyMessage(dwID, "/mxc edit");
			break;
		case ID_MENUBASE+5: // Load
			g_rPlugin.InputMyMessage(dwID, "/mxc load");
			break;
		case ID_MENUBASE+6: // stats
			g_rPlugin.InputMyMessage(dwID, "/mxc stats");
			break;
		case ID_MENUBASE+7: // Help
			::ShellExecute(0, "open", g_strWd + "\\docs\\Metis.chm", 0, 0, SW_SHOW);
			break;
		case ID_MENUBASE+8: // Home
			::ShellExecute(0, "open", "http://mxcontrol.sourceforge.net", 0, 0, SW_SHOW);
			break;
		case ID_MENUBASE+9: // Spy
			g_rPlugin.InputMyMessage(dwID, "/mxc spy");
			break;
		default:
			ASSERT(FALSE);
		}
	}

	else if((dwID > 0) && (nCmd >= ID_MENUBASE+20) && (nCmd <= ID_MENUBASE+70)){


		CFileFind finder;
		int nFile = ID_MENUBASE+20;
		BOOL bResult = finder.FindFile(g_strWd + "\\*.xml");
		while(bResult){

			bResult = finder.FindNextFile();
			if(nFile++ == nCmd){

				g_rPlugin.InputMyMessage(dwID, "/mxc edit " + finder.GetFileName());
			}
		}
	}
}


BOOL CMetis::ValidateCommand(CString& strFile, PCOMMAND pCmd, UINT nType)
{

	BOOL bIsValid = TRUE;

	
	CString strCmd, strError;
	
	if(nType == CMD){
		
		strCmd = pCmd->aTrigger[0].strTrigger;
	}
	else if(nType == RENAME){
		
		strCmd = "<OnRename>";
	}
	else if(nType == TIMER){ 
		
		strCmd = "<OnTimer>";
	}
	else if(nType == ENTER){ 
		
		strCmd = "<OnEnter>";
	}
	else if(nType == LEAVE){ 
		
		strCmd = "<OnLeave>";
	}
	else if(nType == OP){

		strCmd = "<OnOpMessage>";
	}
	else if(nType == PM){

		strCmd = "<OnPM>";
	}
	else if(nType == JOIN){

		strCmd = "<OnJoinRoom>";
	}
	else{ 
		
		strCmd = "error";
	}

	// Check conditions of command
	if(pCmd->nCondition != CON_DONTUSE){

		if(pCmd->strLValue.IsEmpty() && pCmd->strLValue.IsEmpty()){

			strError.Format("Warning: Inconsistent condition detected in a command in file %s line %d\n"
				            "\tCommand: %s\n"
							"\tCondition is %d but both left and right value (lValaue and rValue) are empty\n"
							"\tIs this really intended?",
							strFile, pCmd->nLine,
							strCmd,
							pCmd->nCondition
					 );
			
			//AfxMessageBox(strError, MB_ICONEXCLAMATION);
			m_aParseErrors.Add(strError);
			bIsValid = FALSE;
		}
	}
	if((nType < CMD) && (pCmd->nMode == CMD_MODE_THREAD)){

		strError.Format("Warning: invalid attribute in file %s near line %d for %s\n"
			            "\tattribute 'mode' is only supported for <command>, <OnPM> and <OnOpMessage>",
						strFile, pCmd->nLine, strCmd);

		//AfxMessageBox(strError, MB_ICONEXCLAMATION);
		m_aParseErrors.Add(strError);
		bIsValid = FALSE;
	}

	if((nType >= CMD) && (pCmd->nFlood < 0)){

		strError.Format("Warning invalid value for attribute 'flood' in file %s near line %d on command %s\n"
			            "\tflood attribute must be greater or equal to zero but is %d",
						strFile, pCmd->nLine, strCmd, pCmd->nFlood);

		//AfxMessageBox(strError, MB_ICONEXCLAMATION);
		m_aParseErrors.Add(strError);
		bIsValid = FALSE;
	}

	// Check trigger
	if(nType >= CMD){

		for(int i = 0; i < pCmd->nIn; i++){

			if(pCmd->aTrigger[i].nCondition != CON_DONTUSE){

				if(pCmd->aTrigger[i].strLValue.IsEmpty() && pCmd->aTrigger[i].strRValue.IsEmpty()){

					strError.Format("Warning: Inconsistent condition detected in a command in file %s line %d\n"
									"\tTrigger: %s\n"
									"\tCondition is %d but both left and right value (lValaue and rValue) are empty\n"
									"\tIs this really intended?",
									strFile, pCmd->nLine,
									pCmd->aTrigger[i].strTrigger,
									pCmd->aTrigger[i].nCondition
							 );
					
					//AfxMessageBox(strError, MB_ICONEXCLAMATION);
					m_aParseErrors.Add(strError);
					bIsValid = FALSE;
				}
			}
		}
	}

	// Check responses
	//  condition
	for(int i = 0; i < pCmd->nNum; i++){

		if(pCmd->aResponses[i].nCondition != CON_DONTUSE){

			if(pCmd->aResponses[i].strLValue.IsEmpty() && pCmd->aResponses[i].strRValue.IsEmpty()){

				strError.Format("Warning: Inconsistent condition detected in a command in file %s line %d\n"
								"\tResponse: %s\n"
								"\tCondition is %d but both left and right value (lValaue and rValue) are empty\n"
								"\tIs this really intended?",
								strFile, pCmd->nLine,
								pCmd->aResponses[i].strResponse,
								pCmd->aResponses[i].nCondition
						 );
				
				m_aParseErrors.Add(strError);
				//AfxMessageBox(strError, MB_ICONEXCLAMATION);
				bIsValid = FALSE;
			}
		}
	}

	// operators
	for(i = 0; i < pCmd->nNum; i++){

		if(pCmd->aResponses[i].nOperator != OP_DONT_USE){

			int nOp = pCmd->aResponses[i].nOperator;
			
			if(((nOp == OP_NOT) || (nOp == OP_STR_LEN) || (nOp == OP_STR_UPPER) || (nOp == OP_STR_LOWER)) 
				&& pCmd->aResponses[i].strLop.IsEmpty()){

				CString strOp;
				switch(nOp){

				case OP_NOT:
					strOp = "! (not)";
					break;
				case OP_STR_LEN:
					strOp = "strlen";
					break;
				case OP_STR_UPPER:
					strOp = "struppr";
					break;
				case OP_STR_LOWER:
					strOp = "strlwr";
					break;
				case OP_MINUSMINUS:
					strOp = "--";
					break;
				case OP_PLUSPLUS:
					strOp = "++";
					break;
				}
				strError.Format("Warning: Inconsistent operator detected in file %s line %d\n"
						        "\tCommand: %s\n"
								"\tOperator: %s\n" 
								"\tRequires lValue, but lValue is empty",
								strFile, pCmd->nLine, strCmd, strOp);
				//AfxMessageBox(strError, MB_ICONEXCLAMATION);
				m_aParseErrors.Add(strError);
				bIsValid = FALSE;
			}
			else if((nOp == OP_STR_REPLACE) && (pCmd->aResponses[i].strLop.IsEmpty() || pCmd->aResponses[i].strRop.IsEmpty() ||pCmd->aResponses[i].strNop.IsEmpty())){

				strError.Format("Warning: Inconsistent operator detected in file %s line %d\n"
					            "\tCommand: %s"
								"\tOperator strrep requires lvalue, rvalue and nvalue to be set!",
								strFile, pCmd->nLine, strCmd);
				//AfxMessageBox(strError, MB_ICONEXCLAMATION);
				m_aParseErrors.Add(strError);

				bIsValid = FALSE;
			}
			else if((nOp == OP_READ_FILE) && (pCmd->aResponses[i].strNop.IsEmpty() || pCmd->aResponses[i].strLop.IsEmpty())){

				strError.Format("Warning: Inconsistent operator detected in file %s line %d\n"
					            "\tCommand: %s"
								"\tOperator readfile requires nvalue and lvalue to be set!",
								strFile, pCmd->nLine, strCmd);
				//AfxMessageBox(strError, MB_ICONEXCLAMATION);
				m_aParseErrors.Add(strError);

				bIsValid = FALSE;
			}
			else if(pCmd->aResponses[i].strLop.IsEmpty() && pCmd->aResponses[i].strRop.IsEmpty()){

				strError.Format("Warning: Inconsistent operator detected in file %s line %d\n"
					            "\tCommand: %s"
								"\tOperator %d is present but both lvalue and rvalue are empty",
								strFile, pCmd->nLine, strCmd, nOp);
				//AfxMessageBox(strError, MB_ICONEXCLAMATION);
				m_aParseErrors.Add(strError);
				bIsValid = FALSE;

			}
		}
	}

	return bIsValid;
}

BOOL CMetis::ValidateUsergroups()
{

	CString strGroup, strError;
	BOOL bReturn = TRUE;

	for(int i = 0; i < m_aCommands.GetSize(); i++){

		strGroup = m_aCommands[i]->strGroup;
		
		if(!strGroup.IsEmpty()){
			
			if(strGroup[0] == '!') strGroup = strGroup.Mid(1);
			if(GetUserGroup(strGroup) == NULL){

				strError.Format("Warning in file %s: line %d\n"
					            "\tCommand %s\n\tis limited to non-existing usergroup \"%s\"",
					            m_aCommands[i]->strFile, m_aCommands[i]->nLine,
								m_aCommands[i]->aTrigger[0].strTrigger,
								m_aCommands[i]->strGroup
					     );
				//AfxMessageBox(strError, MB_ICONEXCLAMATION);
				m_aParseErrors.Add(strError);
				bReturn = FALSE;

			}
		}
		for(int j = 0; j < m_aCommands[i]->nIn; j++){

			strGroup = m_aCommands[i]->aTrigger[j].strGroup;

			if(!strGroup.IsEmpty()){

				if(strGroup[0] == '!') strGroup = strGroup.Mid(1);
				if(GetUserGroup(strGroup) == NULL){

					strError.Format("Warning in file %s\n\tTrigger %s\n\tis limited to non-existing usergroup \"%s\"",
									m_aCommands[j]->strFile,
									m_aCommands[i]->aTrigger[j].strTrigger,
									m_aCommands[i]->aTrigger[j].strGroup
							 );
					//AfxMessageBox(strError, MB_ICONEXCLAMATION);
					m_aParseErrors.Add(strError);
					bReturn = FALSE;

				}
			}
		}
	}


	// Enter commands
	for(i = 0; i < m_aEnter.GetSize(); i++){

		strGroup = m_aEnter[i]->strGroup;
		if(!strGroup.IsEmpty()){

			if(strGroup[0] == '!') strGroup = strGroup.Mid(1);
			if(GetUserGroup(strGroup) == NULL){

				strError.Format("Warning in file %s:\n\tCommand %s\n\tis limited to non-existing usergroup \"%s\"",
					            m_aEnter[i]->strFile,
								"<OnEnter>",
								m_aEnter[i]->strGroup
					     );
				//AfxMessageBox(strError, MB_ICONEXCLAMATION);
				m_aParseErrors.Add(strError);
				bReturn = FALSE;

			}
		}
	}

	// leave
	for(i = 0; i < m_aLeave.GetSize(); i++){

		strGroup = m_aLeave[i]->strGroup;
		if(!strGroup.IsEmpty()){

			if(strGroup[0] == '!') strGroup = strGroup.Mid(1);
			if(GetUserGroup(strGroup) == NULL){

				strError.Format("Warning in file %s:\n\tCommand %s\n\tis limited to non-existing usergroup \"%s\"",
					            m_aLeave[i]->strFile,
								"<OnLeave>",
								m_aLeave[i]->strGroup
					     );
				//AfxMessageBox(strError, MB_ICONEXCLAMATION);
				m_aParseErrors.Add(strError);
				bReturn = FALSE;

			}
		}
	}

	strGroup = cRename.strGroup;
	if(!strGroup.IsEmpty()){

		if(strGroup[0] == '!') strGroup = strGroup.Mid(1);
		if(GetUserGroup(strGroup) == NULL){

			strError.Format("Warning in file %s:\n\tCommand %s\n\tis limited to non-existing usergroup \"%s\"",
					        cRename.strFile,
							"<OnRename>",
							cRename.strGroup
					 );
			//AfxMessageBox(strError, MB_ICONEXCLAMATION);
			m_aParseErrors.Add(strError);
			bReturn = FALSE;

		}
	}

	strGroup = cAuto.strGroup;
	if(!strGroup.IsEmpty()){

		if(strGroup[0] == '!') strGroup = strGroup.Mid(1);
		if(GetUserGroup(strGroup) == NULL){

			strError.Format("Warning in file %s:\n\tCommand %s\n\tis limited to non-existing usergroup \"%s\"",
					        cAuto.strFile,
							"<OnTimer>",
							cAuto.strGroup
					 );
			//AfxMessageBox(strError, MB_ICONEXCLAMATION);
			m_aParseErrors.Add(strError);
			bReturn = FALSE;

		}
	}
	return bReturn;
}

void CMetis::InputMyMessage(DWORD dwID, CString strMsg)
{

	//strMsg.Replace(" ", " ");
	InputMessage(dwID, strMsg);
}

BOOL IsNumber(LPCTSTR pszText)
{

	//ASSERT_VALID_STRING(pszText);

	for(int i = 0; i < lstrlen(pszText); i++){

		if(!_istdigit(pszText[i])){

			return FALSE;
		}
	}

	return TRUE;
}

void CMetis::UpdateAutoMessageInterval(unsigned int* pTick)
{

	CString strTmp = m_strBotTic;
	ReplaceUservars(strTmp);
	if(!IsNumber(strTmp)){

		CString strMsg;
		strMsg.Format("Warning:\ninterval attribute of <OnTimer>\nis set to non-digit value '%s'.\nTimer can not be processed.\n",
			strTmp);
		DisplayToolTip(strMsg, 10000, NIIF_WARNING);
	}
	else{

		*pTick = atoi(strTmp) * 1000; // interval is set in seconds, but handled in milliseconds :P
	}
}

// Init trivia questions...
void CMetis::InitTrivia()
{

   	CArray<PTRIVIA, PTRIVIA> aTmp;

	for(int i = 0; i < m_aTrivia.GetSize(); i++){

		aTmp.Add(m_aTrivia.GetAt(i));
	}

	int nIndex = 0;
	//randomize order...
	while(aTmp.GetSize()){

		nIndex = rand() % (int)aTmp.GetSize();
		m_aGame.Add(aTmp.GetAt(nIndex));
		aTmp.RemoveAt(nIndex);
	}
	m_nCurrent = 0;
	m_nRoundEnd			= -1;
	m_nRoundCurrent		= 0;
}

int CMetis::GetScore(CString &strUser)
{

	for(int i = 0; i < m_aUserInfo.GetSize(); i++){

		if(m_aUserInfo[i].strName.Find(strUser, 0) == 0){

			return m_aUserInfo[i].nTriviaScore;
		}
	}
	return 0;
}

int CMetis::IncreaseScore(CString &strUser, int nPoints, PINT nDone)
{

	ASSERT(nDone);

	int nResult = -1;

	for(int i = 0; i < m_aUserInfo.GetSize(); i++){

		if(m_aUserInfo[i].strName.Find(strUser, 0) == 0){

			m_aUserInfo[i].nTriviaScore += nPoints;
			nResult =  m_aUserInfo[i].nTriviaScore;
			break;
		}
	}

	if(nResult == -1){

		USER_INFO u;
		u.nTriviaScore = nPoints;
		u.strName = strUser;
		u.nCount      = 0;
		u.nFirst      = 0;
		u.nBlockStart = -1;
		m_aUserInfo.Add(u);
		nResult = nPoints;
	}

	if(m_nRoundEnd != -1){

		*nDone = nResult >= m_nRoundEnd;
	}
	else{

		*nDone = 0;
	}
	
	return nResult;
}

BOOL CMetis::IsAdmin(CString &strUser)
{

	return GroupApplies("Trivia", strUser);
}

void CMetis::SendPMTo(CString strUser, CString strMessage)
{

	DWORD dwIP  = 0;
	WORD  wPort = 0;

	if(m_dwPMID == 0) return;

	MX_USERINFO user;
	for(int i = 0; i < GetRoom(m_dwPMID).pUserMap->GetSize(); i++){
 
		//if(GetRoom(m_dwPMID).pUserMap->GetAt(i).strUser.Compare(strUser) == 0){
		if(GetRoom(m_dwPMID).pUserMap->Lookup(strUser, user)){

			dwIP  = user.dwNodeIP;
			wPort = user.wNodePort;
			break;
		}
	}

	if((dwIP == 0) ||(wPort == 0)) return;

	SendPrivateMsg(dwIP, wPort, strUser, strMessage);
}

CString CMetis::ReadFromFile(CString strFile, CString strMode, CString strLine)
{

	CStdioFile file; 
	CString strBuffer;
	BOOL  bResult = FALSE;

	if(strLine.IsEmpty() && strMode == "c" ) strLine = "-1";
	if(strLine.IsEmpty() && strMode == "l" ) strLine = "1";

	TRY{

		bResult = file.Open(strFile, CFile::modeRead|CFile::typeText);
		if(bResult){

			CString strTmp;
			int nCnt  = 0;
			int nRead = atoi(strLine);
			while(file.ReadString(strTmp)){

				nCnt++;
				if((strMode == "c") && nRead == -1){

					// read complete file
					strBuffer += strTmp;
				}
				else if((strMode == "c") && (nRead >= nCnt)){

					// read complete file until line nRead
					strBuffer += strTmp;
				}
				else if((strMode == "c") && (nRead < nCnt)){

					// bounce out
					break;
				}
				else if((strMode == "l") && (nRead == nCnt)){

					// read only line nRead from file
					strBuffer = strTmp;
					break;
				}
			}  // while(file.ReadString(strTmp))
		file.Close();
		}
	}
	CATCH(CFileException, e){

	}END_CATCH;

	strBuffer.Replace("\n", "\\n");
	strBuffer.Replace("\t", "\\t");
	strBuffer.Replace("\r", "\\r");
	return strBuffer;
}

BOOL CMetis::WriteToFile(CString strFile, CString strMode, CString strData)
{
	
	BOOL bResult = TRUE;

	CStdioFile file; 
	CString strBuffer;

	strData.Replace("\\n", "\n");
	strData.Replace("\\t", "\t");
	strData.Replace("\\r", "\r");

	TRY{

		UINT uFlags = 0;
		if(strMode == "a") // a = append
			uFlags = CFile::modeWrite|CFile::modeCreate|CFile::modeNoTruncate|CFile::typeText;
		else // truncate file
			uFlags = CFile::modeWrite|CFile::modeCreate|CFile::typeText;

		bResult = file.Open(strFile, uFlags);
		if(bResult){
		
			if(strMode == "a"){
				
				file.Seek(0, CFile::end);
			}
			file.WriteString(strData);
			file.Flush();
			file.Close();
		}
	}
	CATCH(CFileException, e){

		bResult = FALSE;
	}END_CATCH;
	return bResult;
}


const CString CMetis::FormatInt(int nNum)
{
	CString strNum;
	strNum.Format("%d", nNum);
	return strNum;
}

// Called when the channelbar (bIsUser = FALSE)
// or userlisttooltip (bIsUser = TRUE) is about to be displayed
// pTool is a pointer to the CString object containing the tooltip text
void CMetis::OnToolTipPrepare(DWORD dwID, BOOL bIsUser, CString* pTool)
{

	if(dwID > 0){
	
		CString strMetisTool;
		strMetisTool.Format(
						"<br><table cellpadding=\"2\" cellspacing=\"2\" align=\"left\">"
						"<tr>"
						"<td><table cellpadding=\"0\" cellspacing=\"0\" align=\"left\"><tr><td bgcolor=\"#0000cc\"><font color=\"white\"><b>%s</b></font></td></tr></table>"
						"<b>Metis Status:</b><t><t>%s<br>"
						"<b>Metis Trivia: </b><t><t>%s%<br>"
						"<hr><br>"
						"<b>Files loaded:</b><t>%d<br>"
						"</td>"
						"</tr>"
						"</table>",
						STR_VERSION_DLG,
						GetRoom(dwID).bIsBotActivated ? "Running" : "Stopped",
						m_dwTriviaID == dwID ? "Running" : "Stopped",
						m_aIncludeFiles.GetSize()+1
					);
		
		pTool->Append(strMetisTool);
	}
}


void CMetis::ParseFinishErrors(void)
{

	if(m_aParseErrors.GetSize()){

		CErrorDlg dlg;
		
		for(int n = 0; n < m_aParseErrors.GetSize(); n++){

            dlg.m_strErrors += m_aParseErrors[n] + _T("\n");
		}

		dlg.m_strErrors.Replace(_T("\n"), _T("\r\n"));
		dlg.DoModal();
	}
}


CString CMetis::ReadFromWeb(CString strURL, CString strMode, CString strLine)
{
	

	CInternetSession is;
	
	CHttpFile *file = NULL; 
	CString strBuffer;
	CString strHeaders = "User-Agent: Metis/2.83";
	BOOL  bResult = FALSE;

	if(strLine.IsEmpty() && strMode == "c" ) strLine = "-1";
	if(strLine.IsEmpty() && strMode == "l" ) strLine = "1";

	try{

		//bResult = file.Open(strFile, CFile::modeRead|CFile::typeText);

		file = (CHttpFile*)is.OpenURL(strURL, 1, 1, strHeaders, strHeaders.GetLength());
		if(file){

			CString strTmp;
			int nCnt  = 0;
			int nRead = atoi(strLine);
			while(file->ReadString(strTmp)){

				nCnt++;
				if((strMode == "c") && nRead == -1){

					// read complete file
					strBuffer += strTmp;
				}
				else if((strMode == "c") && (nRead >= nCnt)){

					// read complete file until line nRead
					strBuffer += strTmp;
				}
				else if((strMode == "c") && (nRead < nCnt)){

					// bounce out
					break;
				}
				else if((strMode == "l") && (nRead == nCnt)){

					// read only line nRead from file
					strBuffer = strTmp;
					break;
				}
			}  // while(file.ReadString(strTmp))
		file->Close();
		}
	}
	catch(CInternetException* e){

		TCHAR   szCause[255];
		CString strFormatted;

		e->GetErrorMessage(szCause, 255);
		strBuffer.Format("Error: %s\n", szCause);
	}

	strBuffer.Replace("\n", "\\n");
	strBuffer.Replace("\t", "\\t");
	strBuffer.Replace("\r", "\\r");
	strBuffer.Replace("\\r\\n\\n", ".");
	return strBuffer;
}

CString CMetis::FormatIdleTime(DWORD dwTick)
{
	
	DWORD dwIdleTime = (GetTickCount() - dwTick)/1000;


	CString strIdleTime;
	strIdleTime.Format("%02d:%02d", dwIdleTime/60, dwIdleTime%60);
	
	return strIdleTime;
}

CString CMetis::FormatStayTime(DWORD dwTick)
{
	
	DWORD dwMinutes = GetTickCount() - dwTick;
	
	CString strTime;
	strTime.Format("%d", dwMinutes / 1000 / 60);
	
	return strTime;
}

CString CMetis::FormatUserlevel(WORD wLevel)
{
	
	CString strLevel;
	switch(wLevel){

		case 1:
			strLevel = _T("@");
			break;
		case 2:
			strLevel = _T("+");
			break;
		default:
			strLevel = _T("");
	}
	
	return strLevel;
}