/*****************************************************************/
/*																 */
/*  This file is part of the RoboMX Qeury API                    */
/*  The query API is used by RoboMX plugins to query information */
/*  about chatrooms, users and settings                          */
/*																 */
/*  This is provided 'AS-IS' wihtout any warranty                */
/*                                                               */ 
/*****************************************************************/


#ifndef __RMX_QAPI_H__
#define __RMX_QAPI_H__

#define WM_QAPI WM_APP+6

///////////////////////////////////
// Returns the RoboMX version
#define RMX_GETVERSION 0

///////////////////////////////////
// Get info about the channellist
// DWORD dwNum = (DWORD)QueryInformation(RMX_GETNUMXXXXX, 0, 0)
#define RMX_GETNUMCHANNELS			100
#define RMX_GETNUMFILTERCHANNELS	101
#define RMX_GETNUMWINMX				102
#define RMX_GETNUMRCMS				103
#define RMX_GETNUMROSE				104
#define RMX_GETNUMMCMA				105
#define RMX_GETNUMMCS				106
#define RMX_GETNUMWCS				107
#define RMX_GETNUMWPNUSERS			108
#define RMX_GETNUMFXSERV			109

//make robomx reload the channellist
//
// clear the channellist, then load the channels
// BOOL bSuccess = (BOOL)QueryInformation(RMX_LOADCHANNELS, 1, 0);
//
// update the list without clearing
// BOOL bSuccess = (BOOL)QueryInformation(RMX_LOADCHANNELS, 0, 0);
//
// returns TRUE on success
#define RMX_LOADCHANNELS			110

// toggle the channellist:
// close the list:
// int nPrevStatus = QueryInformation(RMX_LISTSTATUS, 0, 0);
// open the list:
// int nPrevStatus = QueryInformation(RMX_LISTSTATUS, 1, 0);
// check if list is open or closed
// int nPrevStauts = QueryInformation(RMX_LISTSTATUS, 2, 0);
// returns 0 if the list was previously closed, returns 1 if the list was open
#define RMX_LISTSTATUS				111

///////////////////////////////////
// Get info about a chatroom

// Get Average lag in milliseconds
// DWORD dwLag = (DWORD)QueryInformation(RMX_GETAVERAGELAG, dwRoomID, 0)
#define RMX_GETAVERAGELAG           200

// Get Number of total messages recieved in room
// DWORD dwNum = (DWORD)QueryInformation(RMX_GETRECVMSGS, dwRoomID, 0)
#define RMX_GETRECVMSGS            201

// Get Number of total messages sent to room
// DWORD dwNum = (DWORD)QueryInformation(RMX_GETSENTMSGS, dwRoomID, 0)
#define RMX_GETSENTMSGS	            202


///////////////////////////////////
// Get configuration entry

// Get name of the configuraiton file
// in max_len = 400
// char* buffer = new char[400]
// int nLen = (int)QueryInformation(RMX_GET_CONFIG, max_len, buffer);
// returns the length of the string. If the buffer is not big enough to hold the string
// nothing is copied
// If lParam is NULL it returns only the length of the string
#define RMX_GET_CONFIG				300

// Get colorref
// COLORREF cr = (COLORREF)QueryInformation(RMX_CFG_COLOR_, 0, 0)
#define RMX_CFG_COLOR_BG			301
#define RMX_CFG_COLOR_NAME			302
#define RMX_CFG_COLOR_MSG			303
#define RMX_CFG_COLOR_ACTION		304
#define RMX_CFG_COLOR_TOPIC			305
#define RMX_CFG_COLOR_MOTD			306
#define RMX_CFG_COLOR_OPMSG			307
#define RMX_CFG_COLOR_ENTER			308
#define RMX_CFG_COLOR_PART			309
#define RMX_CFG_COLOR_LINK			310
#define RMX_CFG_COLOR_ERROR			311
#define RMX_CFG_COLOR_PENDING		312
#define RMX_CFG_COLOR_OK			313



#endif //__RMX_QAPI_H__