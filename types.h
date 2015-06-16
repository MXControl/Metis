#ifndef __TYPES_H_INCLUDED__
#define __TYPES_H_INCLUDED__

typedef struct TAG_USERGROUP{

	CString	strGroup;
	CString strUsers;
	BOOL bCase;
	CString strFile;
	int     nLine;
} USER_GROUP, *PUSER_GROUP;

typedef struct TAG_TRIGGER{

	CString strTrigger;
	CString strNames;
	CString strGroup;
	BOOL	bExact;
	BOOL	bCase;
	CString strLValue;
	CString strRValue;
	int     nCondition; // -1 = none, 0 = not equal, 1 = equal
} TRIGGER, *PTRIGGER, BADWORD, *PBADWORD;

#define strBadWord strTrigger 


typedef struct TAG_RESPONSE{

	CString strResponse;
	int     nDelay;
	int		nCmdType;
	int     nData;
	CString strData;
	CString strMode;
	CString strLValue;
	CString strRValue;
	int     nCondition; // -1 = none, 0 = not equal, 1 = equal
	int     nOperator;  // -1 = none, 0 = -, 1 = +, 2 = *, 3 = /, 4 = %
	CString strLop;
	CString strRop;
	CString strNop;
} RESPONSE, *PRESPONSE;

// Command mode
enum CMD_MODES{

	CMD_MODE_BLOCK = 0,
	CMD_MODE_THREAD
};

// Command types
enum CMD_TYPES{

	CMD_TYPE_NORMAL = 1,
	CMD_TYPE_RANDOM,
	CMD_TYPE_SCRIPT,
};

// Output types
enum OUT_TYPES{
	OUT_NORMAL = 1,
	OUT_BOT  = 2,
	OUT_EXEC,
	OUT_PUSH,
	OUT_POP,
	OUT_BREAK,
	OUT_GOTO,
	OUT_SLEEP,
	OUT_ECHO,
	OUT_TOOLTIP,
	OUT_CHANNELTOOL,
	OUT_PM,
	OUT_FILE,
	OUT_TRIGGER
};

// Conditions
enum CONDITIONS{

	CON_DONTUSE = -1,
	CON_NOTEQUAL = 0,
	CON_EQUAL,
	CON_SMALLER,
	CON_GREATER,
	CON_FIND,
	CON_NOTFIND,
	CON_FINDNC,
	CON_NOTFINDNC,
	CON_OR,
	CON_AND,
	CON_ISNUM,
	CON_MAX = CON_ISNUM
};

// Operators
enum OPERATORS{

	OP_DONT_USE = -1,
	OP_MINUS = 0,
	OP_MINUSMINUS,
	OP_PLUS,
	OP_PLUSPLUS,
	OP_TIMES,
	OP_DIVIDE,
	OP_MODULO,
	OP_POWER,
	OP_OR,
	OP_AND,
	OP_NOT,
	OP_SMALLER,
	OP_EQUAL,
	OP_NOTEQUAL,
	OP_GREATER,
	OP_STR_LEN,
	OP_STR_TRIMM,
	OP_STR_LOWER,
	OP_STR_UPPER,
	OP_STR_LEFT,
	OP_STR_RIGHT,
	OP_STR_FIND,
	OP_STR_REVERSE_FIND,
	OP_STR_APPEND,
	OP_STR_REMOVE,
	OP_STR_REPLACE,
	OP_READ_FILE,
	OP_READ_WEB,
	OP_MAX = OP_READ_WEB
};

typedef struct TAG_COMMAND{
	
	CString strFile; // filename the command was loaded from
	int	    nLine;   // line in the file
	CArray<TRIGGER, TRIGGER> aTrigger;
	CArray<RESPONSE, RESPONSE> aResponses;
	CString strNames;
	CString strGroup;
	int  nIn;
	int  nNum;
	int  nCmdType; 
	BOOL bCase;
	int  nFlood;
	int  nLast;
	int  nMode;  // blocking / non blocking
	CString strLValue;
	CString strRValue;
	int     nCondition; // -1 = none, 0 = not equal, 1 = equal
} COMMAND, *PCOMMAND;


typedef struct TAG_BOT_MESSAGE{

	CString strName;
	CString strTrigger;
	CString strResponse;
	CString strData;
	DWORD   dwChannelID;
	int     nDelay;
	UINT	nData;
	UINT    nType;
} BOT_MESSAGE, *PBOT_MESSAGE;

typedef struct TAG_USERINFO{

	CString strName;
	int nCount;
	DWORD nFirst;
	DWORD nBlockStart;
	int   nTriviaScore;
} USER_INFO, *PUSER_INFO;

typedef struct TAG_TRIVIA{
	
	DWORD nPoints;
	DWORD nTime;
	CString strTrigger;
	CStringArray aResponse;
} TRIVIA, *PTRIVIA;


#endif // __TYPES_H_INCLUDED__
