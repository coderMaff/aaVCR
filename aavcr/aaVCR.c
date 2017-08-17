/*

	aaVCR

	Air Attack - Multi recording program

	based on Fighter Ace Film Bandit by VMF215 Daggr


	Maf 05.08.2001 v0.0.1   Created
	Maf 04.09.2001 v0.0.2   Stoped aaVCR overwriting old aaVCR files, thanks puppet for spotting that
							Added option to open film directory for easy renaming
							Added help
							Added info ( about )
							Added q to quit
							Added quick launch and play of movie files 0-9
	Maf	04.09.2001 v0.0.3	Fixed bug in file skipping
							Added Bogeys GFX
	Maf 05.08.2003 v1.0.0	Implamented Mouse
							Got rid of windows borders
							Made trayable
							Added FileInfo function on F
							Tidied code up
							Added Explore film function, Gives you info about a film
*/

#define	STRICT

#define TIMER_TICK		1
#define TIMER_LAUNCH	2
#define AAEXEKEY		"SOFTWARE\\VR1\\Air Attack\\Game EXE"
#define AAINSTALLKEY	"SOFTWARE\\VR1\\Air Attack"
#define TRAY_ICON		(WM_APP+100)
#define VERSION			"v1.0.0"


#include <windows.h>
#include <windowsx.h>
#include "resource.h"
#include <stdio.h>

// ------------
//  Prototypes
// ------------

LRESULT CALLBACK			WndProc( HWND, UINT, WPARAM, LPARAM );
void 						MoveTick( HWND hwnd );
void 						RedrawScreen( HWND hwnd );
void						CheckFilm( HWND hwnd );
BOOL						Register( HINSTANCE	hInst );
HWND						Create(	HINSTANCE hInst,int nCmdShow );
void						Hide( BOOL, HWND );
void						Activate( HWND );
void						TrayIcon( HWND, DWORD, LPSTR  );
void						FileInfo( HWND );
void						ExploreFilm( HWND );

void						OnKeyDown(	HWND hwnd,	UINT vk, BOOL fDown, int cRepeat,UINT flags);
void						OnTimer(	HWND hwnd, UINT id );
BOOL						OnCreate(	HWND hwnd, CREATESTRUCT FAR * lpCreateStruct );
void						OnDestroy( HWND hwnd );
void						OnPaint(	HWND hwnd );
void						OnLmbClick(	HWND, WPARAM, LPARAM );
void						OnTrayIcon(	HWND, WPARAM, LPARAM );

// ------------------
//  Global Variables
// ------------------

		char		szAppName[] = "aaVCR";
		char		fActive = 0;											// Active 0 = false, 1 = true
		int			iFilms = 0;												// # of films backed up
static	HWND		MainWindow;												// Windowhandle
		char		lpIniFile[80];											// Name and path of ini file
		char		lpFileNameAppend[80];									// Append this onto the front of recordings
		char		lpAAEXE[1024];											// Air Attack EXE File
		char		lpAAFilmDir[1024];									// Air Attack Film Directory
		char		lpCaption[] = "aaVCR (c)2001-03 [TSQN]Kippercod";		// Window Caption
		HANDLE		hLogo;													// Logo handle
		HANDLE		hSwitch;												
		// Logo handle
		HANDLE		hTick[3];												// Moving spots handle
		HFONT		hFont;													// Font handle
static	HINSTANCE	hInst;													// App Instance
		int			key;													// Lastkeypress
static	int			x;
static	int			y;
static 	int			posX = 21;												// position of the tick counter
static  char		fDirection = 0;											// direction of travel for tick
		char		lpWindowText[6][80];									// Text to display in window
		BOOL		fHidden			= FALSE;	


// ---------------------
//  Program entry point
// ---------------------

int CALLBACK WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR  lpCmdLine,
                     int    nCmdShow)
{
	MSG			Msg;

	if( lstrlen(lpCmdLine) == 0 )
		lstrcpy(lpIniFile,".\\aaVCR.ini");
	else
		wsprintf(lpIniFile,".\\%s",lpCmdLine);

	if( !Register(hInstance))
	{
		char	m[50];

		wsprintf(m,"Failed to Register Window - Error %ld",GetLastError());
		MessageBox( NULL, m,"Error", MB_OK );

		return FALSE;
	}

	if(!Create(hInstance, nCmdShow))
	{
		MessageBox( NULL, "Failed to Create Window","Error", MB_OK );
		return FALSE;
	}

	while( GetMessage (&Msg, NULL, 0, 0 ))
	{
		TranslateMessage(&Msg);
		DispatchMessage(&Msg);
	}

	return Msg.wParam;
}


// ---------------------
//  Register the window
// ---------------------

BOOL Register(HINSTANCE hInstance)
{

	WNDCLASS W;

	memset( &W, 0, sizeof(W) );

	W.style			= 0;
	W.lpfnWndProc	= WndProc;
	W.hInstance		= hInstance;
	W.hIcon			= LoadIcon(	hInst, "IDI_ICON1");
	W.lpszClassName = szAppName;
	W.hbrBackground	= GetStockBrush( WHITE_BRUSH );
	W.hCursor		= LoadCursor( NULL, IDC_ARROW );

	return ( RegisterClass( &W ) != (ATOM)0 );
}

// ---------------
//  Create Window
// ---------------

HWND Create(HINSTANCE hInstance, int nCmdShow)
{
	HWND	hwnd;

	hInst = hInstance;

	x = ((GetSystemMetrics( SM_CXSCREEN ) - 400 ) / 2);
	y = ((GetSystemMetrics( SM_CYSCREEN ) - 200 ) / 2);

	hwnd = CreateWindowEx(	0,
							szAppName,
							lpCaption,
							WS_POPUP,
							x,
							y,
							600,
							200,
							NULL,
							NULL,
							hInstance,
							NULL );

	if( hwnd == NULL )
		return hwnd;

	ShowWindow( hwnd, nCmdShow );
	UpdateWindow( hwnd );

	TrayIcon( hwnd, NIM_ADD, "aaVCR" );

	return hwnd;

}

// ----------------------
//  The window procedure
// ----------------------

LRESULT CALLBACK  WndProc(	HWND hwnd,
							UINT Message,
							WPARAM wParam,
							LPARAM lParam )
{
	switch( Message )
	{
		HANDLE_MSG( hwnd, WM_CREATE, OnCreate );		// Message cracker from <windowsx.h>
		HANDLE_MSG( hwnd, WM_DESTROY, OnDestroy );
		HANDLE_MSG( hwnd, WM_PAINT, OnPaint );
		HANDLE_MSG( hwnd, WM_KEYDOWN, OnKeyDown );
		HANDLE_MSG( hwnd, WM_TIMER, OnTimer );
		
		case	TRAY_ICON	:	OnTrayIcon(hwnd, wParam, lParam );							
								return DefWindowProc( hwnd, Message, wParam, lParam );
								break;

		case	WM_LBUTTONDOWN	:	OnLmbClick(hwnd, wParam, lParam );							
									return DefWindowProc( hwnd, Message, wParam, lParam );
									break;
			
		default:
			return DefWindowProc( hwnd, Message, wParam, lParam );
	}
}

// ------------
//  TextToWord
// ------------

UINT TextToInt( char *Txt, int len )

{
	UINT	mul, total;
	int		i;

	mul		= 1;
	total	= 0;


	for	(i = len - 1; i	>= 0; i--)
	{
		total =	total +	((Txt[i] - '0')	* mul);
		mul	= mul *	10;
	}

	return total;
}


// ---------------
//  RedrawScreen
// ---------------

void RedrawScreen( HWND hwnd )
{

	RECT	r;

	r.left = 0;
	r.top = 0;
	r.right = 600;
	r.bottom = 200;

	InvalidateRect(hwnd,&r,FALSE); // Invalidate whole screen
}

// ---------------
//  Read Registry .. Read AA's settings from the registry
// ---------------

void Read_Registry( HWND hwnd )
{
	char			lpResult[3][256];
	char			m[1024];
	HANDLE			fh,hOpen;
	DWORD 			rc;
	DWORD			len;
	DWORD			dwType;
	char			buf[1024];
	WIN32_FIND_DATA FileData;
    HANDLE          hSearch;
    BOOL            fFinished = FALSE;
    char            Pattern[256];
	char			ci[10];
	int				i,ix;

	KillTimer( hwnd, TIMER_LAUNCH );	// Only wants to happen once

	fh = CreateFile(	lpIniFile,
						GENERIC_READ,
						0,
						NULL,
						OPEN_EXISTING,
						FILE_ATTRIBUTE_NORMAL,
						NULL );

	if ( fh == INVALID_HANDLE_VALUE )			// Cant find inifile
	{
		wsprintf(m,"Error - INI file does not exist ( %s ), or INI file not found in expected location",lpIniFile);
		MessageBox( NULL,m, "Error", MB_OK );
		PostQuitMessage(1);
		return;
	}

	CloseHandle(fh);

	GetPrivateProfileString( "aaVCR","name","NOT FOUND!",lpResult[0],sizeof(lpResult[0]),lpIniFile );
	GetPrivateProfileString( "aaVCR","launchaa","NOT FOUND!",lpResult[1],sizeof(lpResult[1]),lpIniFile );
	GetPrivateProfileString( "aaVCR","startactive","NOT FOUND!",lpResult[2],sizeof(lpResult[2]),lpIniFile );

	if ( (rc = RegOpenKeyEx(HKEY_LOCAL_MACHINE,AAINSTALLKEY, 0, KEY_READ, &hOpen)) == ERROR_SUCCESS)
    {
		/* Get aira.exe location from reg into lpAAEXE here */

		len = sizeof(buf);

		if ( RegQueryValueEx( hOpen, "Game EXE", 0, &dwType, (LPBYTE)buf, &len ) == ERROR_SUCCESS)
			wsprintf( lpAAEXE,"%s",buf );	

		buf[0] = 0x00; // Reset buf

		/* Get FilmDir Location from ref into lpAAFilmDir here */

		if ( RegQueryValueEx( hOpen, "Install Path", 0, &dwType, (LPBYTE)buf, &len ) == ERROR_SUCCESS)
			wsprintf( lpAAFilmDir,"%sFILM", buf );
		
		RegCloseKey( hOpen );

	}	
	else
	{
		MessageBox(hwnd,"Unable to find Air Attack in registry, are you sure its installed?\naaVCR will now quit","Error",MB_OK);
		OnDestroy(hwnd);
	}


	wsprintf(lpWindowText[0],"aira.exe = %s",lpAAEXE); // put it on screen
	wsprintf(lpWindowText[1],"FILM = %s",lpAAFilmDir);

	if ( lstrcmp( lpResult[0], "NOT FOUND!" ) == 0 )
	{
		lstrcpy(lpFileNameAppend,"aaVCR_File");
	} else
	{
		lstrcpy(lpFileNameAppend,lpResult[0]);
	}

	/* file dir thing, so we dont overwrite existing films */

	lstrcpy( Pattern, lpAAFilmDir );
    lstrcat( Pattern, "\\");
	lstrcat( Pattern, lpFileNameAppend );
	lstrcat( Pattern, "*.flm");

    hSearch = FindFirstFile( Pattern, &FileData);

    fFinished = (hSearch == INVALID_HANDLE_VALUE);

	i = 7777;

    while (!fFinished)
    {
		memset(ci,0x00,sizeof(ci));

		for( ix = lstrlen( lpFileNameAppend ); ix < lstrlen( FileData.cFileName ); ix++)
		{
			if ( FileData.cFileName[ix] == '.' )
				break;
		}

		ix = ix - lstrlen( lpFileNameAppend );

		memcpy(ci,FileData.cFileName + lstrlen( lpFileNameAppend),ix);

		i = TextToInt(ci,lstrlen(ci));

		if( i > iFilms )
			iFilms = i;

        if (!FindNextFile(hSearch, &FileData)) 
			fFinished = TRUE;
		
    }

	if ( i != 7777 )
		iFilms++;

	if ( iFilms > 0 )
		wsprintf(lpWindowText[4],"Starting at film %d",iFilms);

    FindClose(hSearch);

	/* end of file dir thing */

	if ( lstrcmp( lpResult[1], "TRUE" ) == 0 )
	{
		// all ok the execute aa
		WinExec( lpAAEXE, SW_SHOW );
	}

	if ( lstrcmp( lpResult[2], "TRUE" ) == 0 )
	{
		fActive = 1;
		SetTimer( hwnd,TIMER_TICK,200,NULL);			// Timer Tick 1/2 sec interval
	}

	RedrawScreen( hwnd );

}


// -------------------
// Activate
// -------------------

void Activate(HWND hwnd)
{
	if ( fActive == 1 )
	{
		KillTimer( hwnd, TIMER_TICK );
		fActive = 0;
		MoveTick( hwnd );
		wsprintf(lpWindowText[5],"Status = Stopped");
		RedrawScreen( hwnd );
	}
	else
	{
		SetTimer( hwnd,TIMER_TICK,200,NULL);			// Timer Tick 1/2 sec interval
		fActive = 1;
		wsprintf(lpWindowText[5],"Status = Started");
		RedrawScreen( hwnd );
	}
}

// -------------------
// ExploreFilm - Opens a replay file and does stuff with it
// -------------------

void ExploreFilm(HWND hwnd)
{
	FILE			*fh;
	OPENFILENAME	ofn;				// common dialog box structure
	char			szFile[260];       // buffer for filename	
	BOOL			f;
	int				k,j;
	char			m[1024];
	char			b[180];
	char			plane[128][80];

	ZeroMemory(&plane, sizeof(plane));

	// Build plane list

	wsprintf(plane[0x00],"Error Null");
	wsprintf(plane[0x01],"Early P38");
	wsprintf(plane[0x02],"Spitfire IX");
	wsprintf(plane[0x03],"Early Hurricane");
	wsprintf(plane[0x04],"LA5");
	wsprintf(plane[0x05],"Yak9");
	wsprintf(plane[0x06],"BF-109G-6");
	wsprintf(plane[0x07],"FW-190A-8");
	wsprintf(plane[0x08],"FW-190D-9");
	wsprintf(plane[0x09],"P51");
	wsprintf(plane[0x0A],"Hurricane");
	wsprintf(plane[0x0B],"Dunno! Tell kipp what plane this is");
	wsprintf(plane[0x0C],"109K");
	wsprintf(plane[0x0D],"LA7");
	wsprintf(plane[0x0E],"EP38");
	wsprintf(plane[0x0F],"Spitfire XIV");

	wsprintf(plane[0x14],"B17");

	wsprintf(plane[0x16],"Moskito");
	wsprintf(plane[0x17],"Lancaster");
	wsprintf(plane[0x18],"TU2");
	wsprintf(plane[0x19],"Junka");
	wsprintf(plane[0x1A],"JU_87");
	wsprintf(plane[0x1B],"GE Bomber");

	wsprintf(plane[0x50],"Flying Saucer");

	// Initialize OPENFILENAME
	ZeroMemory(&ofn, sizeof(OPENFILENAME));
	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = hwnd;
	ofn.lpstrFile = szFile;
	ofn.nMaxFile = sizeof(szFile);
	ofn.lpstrFilter = "Air Attack Replays\0*.flm\0";
	ofn.nFilterIndex = 1;
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = NULL;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

// Display the Open dialog box. 

	f = GetOpenFileName(&ofn);

	if(f)
	{
		fh=fopen(ofn.lpstrFile,"rb");

		if( fh == NULL )
		{
			// Cant open file
			MessageBox( hwnd, "Unable to open AirAttack EXE, fileinfo failed", "ExploreFilm", MB_OK );
			return;
		}	
//do stuff

		wsprintf(m,"Replay Analysis\n=============\n");

		j = fseek (fh, 0x40 , SEEK_SET);

		j = fread( b, 1, 80, fh);

		wsprintf(m,"Film Title : %s\n",b);

		j = fseek (fh, 0x30 , SEEK_SET);
		if( j == 0 )
		{
			j = fread( b, 1, 1, fh);

			switch(b[0])
			{
				case	0x17	:	wsprintf(m,"%sReplay Version : 0x17 - from KAA",m);
									break;

				case	0x19	:	wsprintf(m,"%sReplay Version : 0x19 - from FA 1.5 / WP AA / GCPRO AA",m);
									break;

				default			:	wsprintf(m,"%sReplay Version : Unknown!!! Please email this replay to kipp maff667@hotmail.com with info on which version of AA/FA created it!",m);					
			}				
		}


		wsprintf(m,"%s\nProtaganists:\n",m);

		j = 0;

		do
		{    
			k = fseek (fh, j , SEEK_SET);
			k = fread( b, 1, 80, fh);	
/* 

Examples

## = what number player they are
#1 = player number + 1
0x = either 02 03 or 04
PT = plane type
NN = Name text

0  1  2  3  4  5  6  7  8  9  10 11 12 13 14 15 16 17 18 19 20
in at start
00 00 ## 64 64 ## 03 03 ## 00 ## 00 ## 00 00 ## 00 #1 PT NN NN NN NN NN .... ( Targets Fladermouse )
00 00 ## 01 01 ## 02 02 ## 03 ## 03 ## 00 00 ## 00 #1 PT NN NN NN NN NN .... ( Anybody Recruiting )
00 00 ## 5E 5E ## 03 03 ## 03 01 00 ## 00 00 ## 00 #1 0F NN NN NN NN NN .... ( [Av]28_Casus_Belli )

joins
00 00 ## 64 64 ## 04 04 ## 03 ## 03 ## 00 00 ## PT NN NN NN NN NN .... ( Withabix )

*/

			if( 
				 ( b[00] == 0x00 ) &&
				 ( b[01] == 0x00 ) &&
				(( b[03] == 0x64 ) || ( b[3] == 0x01 ) || ( b[3] == 0x5E )) &&
				(( b[04] == 0x64 ) || ( b[4] == 0x01 ) || ( b[4] == 0x5E )) &&
				(( b[06] == 0x04 ) || ( b[6] == 0x03 ) || ( b[6] == 0x02 )) &&
				(( b[07] == 0x04 ) || ( b[7] == 0x03 ) || ( b[7] == 0x02 )) &&
				 ( b[13] == 0x00 ) &&
				 ( b[14] == 0x00 ) 

			  )
			{	
				if( b[16] == 0x00 ) // Joined at start of game
				{
					if( b[18] <= 0x50 ) // Throw away some of the dross
					{
						wsprintf(m,"%s %d)%s flying a %s\n",m,b[02],b+19,plane[b[18]]); 
					}
				}
				else // Joined during game
				{
					if( b[17] <= 0x50 ) // Throw away some of the dross
					{
						wsprintf(m,"%s %d)%s joined with a %s\n",m,b[02],b+17,plane[b[16]]); 
					}
				}
			}

			j++;
		}while( !feof( fh ) );

		fclose(fh);

		MessageBox(hwnd,m,"Replay file analysis",MB_OK);
	}

}

// -------------------
// FileInfo
// -------------------

void FileInfo(HWND hwnd)
{
	FILE		*fh;
	char		m[40000];
	char		b[80];
	int			size;
	long int	j;
	int			k;

	fh=fopen(lpAAEXE,"rb");

	if( fh == NULL )
	{
		// Cant open file
		MessageBox( hwnd, "Unable to open AirAttack EXE, fileinfo failed", "FileInfo", MB_OK );
		return;
	}	

	m[0] = 00;

	j = 0;

	size = fseek (fh, j , SEEK_SET);

	if( size != 0 )
		MessageBox( hwnd, "fseek bollocksed", "FileInfo", MB_OK );

	do
	{    
		size = fseek (fh, j , SEEK_SET);

		size = fread( b, 1, 80, fh);	

		if	(	
				( strncmp("AVI\\",b,4) == 0 )		||	
				( strncmp("BMP\\",b,4) == 0 )		||	
				( strncmp("DLG?\\",b,5) == 0 )		||
				( strncmp("EXPLOS\\",b,7) == 0 )	||	
				( strncmp("MDLG1\\",b,6) == 0 )		||
				( strncmp("MOBJECTS\\",b,9) == 0 )	||
				( strncmp("MPLANES\\",b,8) == 0 )	||	
				( strncmp("OBJECTS\\",b,8) == 0 )	||
				( strncmp("PLANES\\",b,7) == 0 )	||	
				( strncmp("SOUND\\",b,6) == 0 )		||
				( strncmp("SSDLG\\",b,6) == 0 )		||
				( strncmp("TXT\\",b,4) == 0 )		
			)
		{

			wsprintf(m,"%s\n0x%x %s",m,j,b);
			k++;
		}

		j++;
		
		if( k > 20 )
		{
			MessageBox( hwnd, m, "Stuff in your AIRA.exe", MB_OK );
			m[0] = 0x00;
			k = 0;
		}

	}
	while( !feof( fh ) );

	fclose(fh);

	
}

/* 
Name : Hide
Does : Hides / Unhides the interface of the program
*/

void Hide( BOOL fHide, HWND hwnd )
{
	if( fHide == TRUE )
	{
		ShowWindow( hwnd, SW_HIDE );
		fHidden = TRUE;
	}
	else
	{
		ShowWindow( hwnd, SW_SHOWNORMAL );				
		SetActiveWindow( hwnd );
		BringWindowToTop( hwnd );
		fHidden = FALSE;
	}			
}

/*
Name : TrayIcon
Does : handle the tray icon
*/

void TrayIcon( HWND hwnd,  DWORD dothis, LPSTR lpText )
{
	NOTIFYICONDATA tnd; 

	tnd.cbSize				= sizeof(NOTIFYICONDATA);     
	tnd.hWnd				= hwnd;     
	tnd.uID					= 1579385693;     
	tnd.uFlags				= NIF_MESSAGE|NIF_ICON|NIF_TIP;     
	tnd.hIcon				= LoadIcon(	hInst, "IDI_ICON1");
	tnd.uCallbackMessage	= TRAY_ICON;
	lstrcpyn(tnd.szTip, lpText, sizeof(tnd.szTip)); 	

	Shell_NotifyIcon( dothis, &tnd );
}


// --------------------------------
//	MoveTick move the progres pick one notch right or start at left
// --------------------------------
void MoveTick( HWND hwnd )
{
	RECT	r;

	if ( fDirection == 0 )
	{
		posX = posX + 12;
		if ( posX > 204 )
			fDirection = 1;
	} else
	{
		posX = posX - 12;
		if ( posX < 22 )
			fDirection = 0;
	}

	r.left = 0;
	r.top = 172;
	r.right = 401;
	r.bottom = 200;

	InvalidateRect(hwnd,&r,FALSE);	// For tick

	r.left = 540;
	r.top = 10;
	r.right = 600;
	r.bottom = 170;

	InvalidateRect(hwnd,&r,FALSE); // For switch

}

// --------------------------------
//	CheckFilm checks the file lock on a film and copies it if possible
// --------------------------------
void CheckFilm( HWND hwnd )
{
	HANDLE	fh;
	char	filename[1024];
	char	filename2[1024];

	wsprintf(filename,"%s\\TEMPFLM.$!#",lpAAFilmDir);		
	wsprintf(filename2,"%s\\%s%d.flm",lpAAFilmDir,lpFileNameAppend,iFilms);	
	
	fh = CreateFile(	filename,
						GENERIC_READ,
						0,
						NULL,
						OPEN_EXISTING,
						FILE_ATTRIBUTE_NORMAL,
						NULL );

	if ( fh == INVALID_HANDLE_VALUE )			// Cant find inifile
	{
		wsprintf(lpWindowText[5],"Status = Awaiting recording"); // put it on screen
		CloseHandle(fh);
	}
	else
	{
		CloseHandle(fh);
		wsprintf(lpWindowText[5],"Status = Recording found, awaiting finish"); // put it on screen

		if ( MoveFile(filename,filename2))
		{
			iFilms = iFilms + 1;
			wsprintf(lpWindowText[3],"Record %d Film(s)",iFilms); // put it on screen
			wsprintf(lpWindowText[4],"");
			wsprintf(lpWindowText[5],"Status = Recorded film"); // put it on screen
		}
	}

	RedrawScreen( hwnd );


	CloseHandle(fh);
}


// ---- On handlers

// -------------------
//  Handle WM_DESTROY
// -------------------

void OnDestroy( HWND hwnd)
{
	if( fActive == 1 )
		KillTimer( hwnd, TIMER_TICK );
	DeleteObject( hLogo );
	DeleteObject( hSwitch );
	DeleteObject( hFont );
	DeleteObject( hTick[0] );
	DeleteObject( hTick[1] );
	DeleteObject( hTick[2] );
	
	TrayIcon( hwnd, NIM_DELETE, "" );

	PostQuitMessage( 0 );
}

// ------------------
//  Handle WM_CREATE
// ------------------

BOOL OnCreate(	HWND hwnd,
						CREATESTRUCT FAR* lpCreateStruct )
{

	char	m[50];

	// GFX Inititalisation

	hLogo = LoadImage(	hInst,
						MAKEINTRESOURCE(102),
						IMAGE_BITMAP,
						600,
						200,
						LR_DEFAULTCOLOR );

	if ( !hLogo )
	{
		wsprintf(m,"Error Loading Bitmap %d - Instance = %lx",GetLastError(),hInst);
		MessageBox( hwnd, m,"Error",MB_OK);
		return FALSE;
	}

	hSwitch = LoadImage(	hInst,
							MAKEINTRESOURCE(IDB_SWITCH),
							IMAGE_BITMAP,
							99,
							26,
							LR_DEFAULTCOLOR );

	if ( !hSwitch )
	{
		wsprintf(m,"Error Loading Bitmap %d - Instance = %lx",GetLastError(),hInst);
		MessageBox( hwnd, m,"Error",MB_OK);
		return FALSE;
	}

	hTick[0] = LoadImage( 	hInst,
							MAKEINTRESOURCE(IDB_TICK0),
							IMAGE_BITMAP,
							12,
							13,
							LR_DEFAULTCOLOR );
	if ( !hTick[0] )
	{
		wsprintf(m,"Error Loading Bitmap %d - Instance = %lx",GetLastError(),hInst);
		MessageBox( hwnd, m,"Error",MB_OK);
		return FALSE;
	}

	hTick[1] = LoadImage( 	hInst,
							MAKEINTRESOURCE(IDB_TICK1),
							IMAGE_BITMAP,
							12,
							13,
							LR_DEFAULTCOLOR );
	if ( !hTick[1] )
	{
		wsprintf(m,"Error Loading Bitmap %d - Instance = %lx",GetLastError(),hInst);
		MessageBox( hwnd, m,"Error",MB_OK);
		return FALSE;
	}

	hTick[2] = LoadImage( 	hInst,
							MAKEINTRESOURCE(IDB_TICK2),
							IMAGE_BITMAP,
							12,
							13,
							LR_DEFAULTCOLOR );
	if ( !hTick[2] )
	{
		wsprintf(m,"Error Loading Bitmap %d - Instance = %lx",GetLastError(),hInst);
		MessageBox( hwnd, m,"Error",MB_OK);
		return FALSE;
	}

	hFont = CreateFont(	12,					// logical height of font
						6,					// logical average character width
						0,					// angle of escapement
						0,					// base-line orientation angle
						FW_REGULAR,			// font weight
						FALSE,				// italic attribute flag
						FALSE,				// underline attribute flag
						FALSE,				// strikeout attribute flag
						ANSI_CHARSET,		// character set identifier
						OUT_DEFAULT_PRECIS,	// output precision
						CLIP_MASK,			// clipping precision
						PROOF_QUALITY,		// output quality
						FF_SWISS,			// pitch and family
						NULL );				// pointer to typeface name string

	if ( !hFont )
	{
		wsprintf(m,"Error Loading Font %d - Instance = %lx",GetLastError(),hInst);
		MessageBox( hwnd, m,"Error",MB_OK);
		return FALSE;
	}

	
	SetTimer( hwnd,TIMER_LAUNCH,200,NULL);			// Timer Tick 1/2 sec interval	
	
	return TRUE;
}

// -----------------
//  Handle WM_PAINT
// -----------------

void OnPaint( HWND hwnd )
{
	int				x;
	HDC				PaintDC,BltDC,TickDC,SwitchDC;
	PAINTSTRUCT		PaintStruct;


	PaintDC		= BeginPaint( hwnd, &PaintStruct );
	BltDC		= CreateCompatibleDC( PaintDC );
	TickDC		= CreateCompatibleDC( PaintDC );
	SwitchDC	= CreateCompatibleDC( PaintDC );

	SelectObject( BltDC, hLogo );
	BitBlt( PaintDC, 0, 0, 600, 200, BltDC, 0, 0, SRCCOPY );

	SelectObject( TickDC, hTick[0] );
	BitBlt( PaintDC, posX, 33, 12, 13, TickDC, 0, 0, SRCCOPY );

	if ( fDirection == 0 )	// Traveling right
	{
		if( posX > 21 )
		{
			SelectObject( TickDC, hTick[1] );
			BitBlt( PaintDC, posX - 12, 33, 12, 13, TickDC, 0, 0, SRCCOPY );
		}
		if( posX > 33 )
		{
			SelectObject( TickDC, hTick[2] );
			BitBlt( PaintDC, posX - 24, 33, 12, 13, TickDC, 0, 0, SRCCOPY );
		}
	}
	else					// Traveling left
	{
		if( posX < 202 )
		{
			SelectObject( TickDC, hTick[1] );
			BitBlt( PaintDC, posX + 12, 33, 12, 13, TickDC, 0, 0, SRCCOPY );
		}
		if( posX < 192 )
		{
			SelectObject( TickDC, hTick[2] );
			BitBlt( PaintDC, posX + 24, 33, 12, 13, TickDC, 0, 0, SRCCOPY );
		}
	}

	if ( fActive == 1 )
	{
		SelectObject( SwitchDC, hSwitch );
		BitBlt( PaintDC, 250, 165, 349, 191, SwitchDC, 0, 0, SRCCOPY );
	}

	SelectObject( PaintDC, hFont );

	for( x = 0; x < 6; x++ )	
		TextOut( PaintDC, 
				40, 
				55 + ( x * 14 ), 
				lpWindowText[x], 
				lstrlen( lpWindowText[x] ));


	DeleteDC( TickDC );
	DeleteDC( BltDC );
	DeleteDC( PaintDC );
	DeleteDC( SwitchDC );
	EndPaint( hwnd, &PaintStruct );
}

// -------------------
//  Handle WM_KEYDOWN
// -------------------

void OnKeyDown(HWND hwnd, UINT vk, BOOL fDown, int cRepeat, UINT flags)
{

	char	m[1204];
	/*
	wsprintf(m,"%d",vk);

	MessageBox( hwnd, m,"kEYPRESS", MB_OK );	
	
	*/

	if( vk == 27 )				// ESC
		OnDestroy(hwnd);

	if(( vk >= 48 ) && ( vk <= 57 ))
	{
		wsprintf(m,"%s /Film_%s%d",lpAAEXE,lpFileNameAppend,vk - 48);		
		WinExec( m, SW_SHOW );
	}

	if (( vk == 65 ) || ( vk == 97 )) // a/A
	{
		Activate(hwnd);
	}

	if (( vk == 69 ) || ( vk == 94 )) // e/E
	{
		ExploreFilm(hwnd);
	}

	if (( vk == 70 ) || ( vk == 95 )) // f/F
	{
		FileInfo(hwnd);
	}

	if (( vk == 72 ) || ( vk == 104 )) // h
	{
		wsprintf(m,"aaVCR Help\n\n");
		lstrcat(m,"Keyboard controls:\n");
		lstrcat(m," a/A activate/de-activate\n");
		lstrcat(m," e/E explore film\n");
		lstrcat(m," f/F File Information (AIRA.EXE)\n");
		lstrcat(m," h/H help\n");
		lstrcat(m," i/I information\n");
		lstrcat(m," k/K take me to www.kardiak.co.uk\n");
		lstrcat(m," 0..9 Launch aa and view recording film 0-9\n");
		lstrcat(m," o/O Open AA Film dir so you can rename recordings\n");
		lstrcat(m," q/Q Quit\n");
		lstrcat(m," Esc Quit\n");
		lstrcat(m," + some hidden ones, or mebe not ;o)\n\n");
		lstrcat(m,"Default INI File settings ( aaVCR.ini ):\n");
		lstrcat(m," [aaVCR]\n");
		lstrcat(m," name=kipperfile\n");
		lstrcat(m," launchaa=FALSE\n");
		lstrcat(m," startactive=TRUE\n\n");
		lstrcat(m,"change name to rename your recordings different\n");
		lstrcat(m,"i.e. name=puppet will make puppet1.flm recordings\n");
		lstrcat(m,"startactive=TRUE or FALSE - obvious really!\n");
		lstrcat(m,"launchaa=FALSE or TRUE - Launch aa when aaVCR is run\n");
		MessageBox( hwnd, m, "Help", MB_OK );
	}

	if (( vk == 73 ) || ( vk == 105 )) // i
	{
		wsprintf(m,"aaVCR %s by Matthew Bushell in 2001-2003\n",VERSION);
		lstrcat(m," aka [TSQN]Kippercod - www.kardiak.co.uk\n");
		lstrcat(m," gfx by [TKFM]Bogeyman - www.kustard.co.uk\n\n");
		lstrcat(m," based on orignal work ( fabandit ) by VMF215 Daggr\n");
		lstrcat(m," requested by Dilly and Puppet\n");
		lstrcat(m," thanks to Bombz for GE AA Testing\n");
		lstrcat(m," somone whos name i forget for K AA Testing\n");
		lstrcat(m," thanks to DarkDave for AA Replay Information\n");
		MessageBox( hwnd, m, "About", MB_OK );
	}

	if (( vk == 75 ) || ( vk == 107 )) // k
	{
		wsprintf(m,"explorer http://www.kardiak.co.uk");
		WinExec(m,SW_SHOW);
	}
	
	if (( vk == 79 ) || ( vk == 111 )) // o/O
	{
		wsprintf(m,"explorer %s",lpAAFilmDir);
		WinExec(m,SW_SHOW);
	}

	if (( vk == 81 ) || ( vk == 113 )) // q/Q
		OnDestroy(hwnd);

}

// -----------------
//  Handle WM_TIMER
// -----------------

void OnTimer( HWND hwnd, UINT id )
{

	static char	fDevTeam 	= 	0;

	switch( id )
	{
		case TIMER_LAUNCH		:	KillTimer( hwnd, TIMER_LAUNCH );
									Read_Registry( hwnd );
									break;

		case TIMER_TICK			:	CheckFilm( hwnd );
									MoveTick( hwnd );
									break;

		default 				:	break;
	}

}


/*-----------------------------------------------------------

Name : OnLmbClick

Does : Handles click of left mouse button

------------------------------------------------------------*/

void OnLmbClick( HWND hwnd, WPARAM wParam, LPARAM lParam)
{
	int	xPos,yPos;

	xPos = LOWORD(lParam);  // horizontal position of cursor 
	yPos = HIWORD(lParam);  // vertical position of cursor 

// Minimize
	if(( xPos > 569) && ( yPos > 3 ) && ( xPos < 582 ) && ( yPos < 20 ))  Hide( TRUE, hwnd );		
// Quit
	else if(( xPos > 583) && ( yPos > 3 ) && ( xPos < 600 ) && ( yPos < 20 ))  OnDestroy( hwnd );		
// Start/Stop
	else if(( xPos > 248) && ( yPos > 163 ) && ( xPos < 350 ) && ( yPos < 190 ))  Activate( hwnd );		
	else
		SendMessage(hwnd, WM_NCLBUTTONDOWN, HTCAPTION,0);
}

/*-----------------------------------------------------------

Name : OnTrayIcon

Does : Handles tray events

------------------------------------------------------------*/

void OnTrayIcon( HWND hwnd, WPARAM wParam, LPARAM lParam)
{
	
	UINT			uMouseMsg;
	WINDOWPLACEMENT lpwndpl;	
	
	uMouseMsg = (UINT)lParam;

	if( uMouseMsg == WM_LBUTTONDOWN )
	{
		lpwndpl.length = sizeof(WINDOWPLACEMENT);
		if( GetWindowPlacement( hwnd, &lpwndpl ) )
		{
			Hide(!fHidden,hwnd);
		}		
	}
}


// ---------
//  The End
// ---------