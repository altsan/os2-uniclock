#include "uclock.h"
#include "ids.h"

// ----------------------------------------------------------------------------
// CONSTANTS

#define SZ_WINDOWCLASS          "UClockWindow"

#define HELP_FILE               "uclock.hlp"
#define INI_FILE                "uclock.ini"

// min. size of the program window (not used yet)
#define US_MIN_WIDTH            100
#define US_MIN_HEIGHT           60

#define LOCALE_BUF_MAX          4096    // maximum length of a locale list

// used to indicate an undefined colour presentation parameter
#define NO_COLOUR_PP            0xFF000000

// Maximum string length...
#define SZRES_MAXZ              256     // ...of a generic string resource

// Maximum number of clock-display windows supported
#define MAX_CLOCKS              12

// Profile (INI) file entries (these should not exceed 29 characters)
#define PRF_APP_POSITION        "Position"
#define PRF_KEY_LEFT            "Left"
#define PRF_KEY_BOTTOM          "Bottom"
#define PRF_KEY_WIDTH           "Width"
#define PRF_KEY_HEIGHT          "Height"

#define PRF_APP_PREFS           "Preferences"
#define PRF_KEY_FLAGS           "Flags"
#define PRF_KEY_COMPACTTH       "CompactThreshold"
#define PRF_KEY_DESCWIDTH       "DescWidth"
#define PRF_KEY_PERCOLUMN       "PerColumn"

#define PRF_APP_CLOCKDATA       "ClockData"
#define PRF_KEY_NUMCLOCKS       "Count"
#define PRF_KEY_PANEL           "WTDPanel"   // append ##
#define PRF_KEY_PRESPARAM       "WTDLook"    // append ##

// App-level style settings
#define APP_STYLE_TITLEBAR      0x1         // show titlebar
#define APP_STYLE_WTBORDERS     0x10        // show clock borders

PFNWP pfnRecProc;

// ----------------------------------------------------------------------------
// TYPEDEFS

// Used to group a single clock's presentation parameters together
typedef struct _Clock_PresParams {
    CHAR  szFont[ FACESIZE ];           // font
    ULONG clrBG,                        // background colour
          clrFG,                        // foreground (text) colour
          clrBor,                       // border colour
          clrSep;                       // separator line colour
} CLKSTYLE, *PCLKSTYLE;


// Used to pass data in the configuration dialog
typedef struct _Config_Data {
    HAB         hab;                        // anchor-block handle
    HMQ         hmq;                        // main message queue
    USHORT      usClocks;                   // number of clocks
    WTDCDATA    aClockData[ MAX_CLOCKS ];   // array of clock data
    CLKSTYLE    aClockStyle[ MAX_CLOCKS ];  // array of clock styles
    ULONG       ulPage1;                    // page ID
    USHORT      fsStyle;                    // global style flags
    USHORT      usCompactThreshold;         // when to auto-switch to compact view
    USHORT      usPerColumn;                // no. of clocks to show per column
    BYTE        bDescWidth;                 // % width of descriptions in compact view
    BOOL        bChanged;                   // was the configuration changed?
    UconvObject uconv;                      // Unicode text conversion object
} UCFGDATA, *PUCFGDATA;


// Used to pass data in the clock properties dialog
typedef struct _Clock_Properties {
    HAB         hab;                        // anchor-block handle
    HMQ         hmq;                        // main message queue
    USHORT      usClock;                    // current clock number
    WTDCDATA    clockData;                  // clock's data
    CLKSTYLE    clockStyle;                 // clock's fonts & colours
    ULONG       ulPage1,                    // page ID of clock setup page
                ulPage2;                    // page ID of clock style page
    BOOL        bChanged;                   // was the configuration changed?
    UconvObject uconv;                      // Unicode text conversion object
} UCLKPROP, *PUCLKPROP;


// Contains global application data
typedef struct _Global_Data {
    HAB     hab;                        // anchor-block handle
    HMQ     hmq;                        // main message queue
    HINI    hIni;                       // program INI file
    HWND    clocks[ MAX_CLOCKS ];       // array of clock-display controls
    HWND    hwndTB, hwndSM, hwndMM;     // titlebar and window control handles
    HWND    hwndPopup;                  // popup menu
    USHORT  usClocks;                   // number of clocks configured
    ULONG   timer;                      // refresh timer
    CHAR    szTZ[ TZSTR_MAXZ ];         // actual system TZ
    USHORT  usMsgClock;                 // clock which sent the latest message
    USHORT  fsStyle;                    // global style flags
    USHORT  usCompactThreshold;         // when to auto-switch to compact view
    USHORT  usPerColumn;                // no. of clocks to show per column
    BYTE    bDescWidth;                 // % width of descriptions in compact view
} UCLGLOBAL, *PUCLGLOBAL;


// ----------------------------------------------------------------------------
// FUNCTION PROTOTYPES

MRESULT EXPENTRY MainWndProc( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 );
void ResizeClocks( HWND hwnd );
MRESULT PaintClient( HWND hwnd );
MRESULT EXPENTRY AboutDlgProc( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 );
BOOL WindowSetup( HWND hwnd, HWND hwndClient );
void CentreWindow( HWND hwnd );
void ToggleTitleBar( PUCLGLOBAL pGlobal, HWND hwndFrame, BOOL fOn );
void OpenProfile( PUCLGLOBAL pGlobal );
BOOL LoadIniData( PVOID pData, USHORT cb, HINI hIni, PSZ pszApp, PSZ pszKey );
void SaveSettings( HWND hwnd );
void UpdateTime( HWND hwnd );
BOOL AddNewClock( HWND hwnd );
void DeleteClock( HWND hwnd, PUCLGLOBAL pGlobal, USHORT usClock );
void ConfigNotebook( HWND hwnd );
MRESULT EXPENTRY CfgDialogProc( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 );
BOOL CfgPopulateNotebook( HWND hwnd, PUCFGDATA pConfig );
MRESULT EXPENTRY CfgCommonPageProc( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 );
MRESULT EXPENTRY CfgClockPageProc( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 );
MRESULT EXPENTRY CfgPresPageProc( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 );
void CfgPopulateClockList( HWND hwnd, PUCFGDATA pConfig );
void CfgSettingsCommon( HWND hwnd, PUCFGDATA pConfig );
BOOL ClockNotebook( HWND hwnd, USHORT usNumber );
MRESULT EXPENTRY ClkDialogProc( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 );
BOOL ClkPopulateNotebook( HWND hwnd, PUCLKPROP pConfig );
MRESULT EXPENTRY ClkClockPageProc( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 );
MRESULT EXPENTRY ClkStylePageProc( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 );
BOOL ClkSettingsClock( HWND hwnd, PUCLKPROP pConfig );
BOOL ClkSettingsStyle( HWND hwnd, PUCLKPROP pConfig );



/* ------------------------------------------------------------------------- *
 * Main program (including initialization, message loop, and final cleanup)  *
 * ------------------------------------------------------------------------- */
int main( int argc, char *argv[] )
{
    UCLGLOBAL global;
    HAB       hab;                          // anchor block handle
    HMQ       hmq;                          // message queue handle
    HWND      hwndFrame,                    // window handle
              hwndClient,                   // client area handle
              hwndHelp;                     // help instance
    QMSG      qmsg;                         // message queue
    HELPINIT  helpInit;                     // help init structure
    CHAR      szRes[ SZRES_MAXZ ],          // buffer for string resources
              szError[ SZRES_MAXZ ] = "Error";
    ULONG     flStyle               = FCF_TITLEBAR | FCF_SYSMENU | FCF_MINMAX |
                                      FCF_SIZEBORDER | FCF_ICON | FCF_AUTOICON | FCF_ACCELTABLE |
                                      FCF_TASKLIST | FCF_NOBYTEALIGN;
    BOOL      fInitFailure = FALSE;
    PSZ       pszEnv;


    hab = WinInitialize( 0 );
    if ( hab == NULLHANDLE ) {
        sprintf( szError, "WinInitialize() failed.");
        fInitFailure = TRUE;
    }

    if ( ! fInitFailure ) {
        hmq = WinCreateMsgQueue( hab, 0 );
        if ( hmq == NULLHANDLE ) {
            sprintf( szError, "Unable to create message queue:\nWinGetLastError() = 0x%X\n", WinGetLastError(hab) );
            fInitFailure = TRUE;
        }
    }

    if (( ! fInitFailure ) &&
        ( ! WinRegisterClass( hab, WT_DISPLAY, WTDisplayProc, CS_SIZEREDRAW, sizeof(PWTDDATA) )))
    {
        sprintf( szError, "Failed to register class %s:\nWinGetLastError() = 0x%X\n", WT_DISPLAY, WinGetLastError(hab) );
        fInitFailure = TRUE;
    }
    if (( ! fInitFailure ) &&
        ( ! WinRegisterClass( hab, SZ_WINDOWCLASS, MainWndProc, CS_SIZEREDRAW, sizeof(PUCLGLOBAL) )))
    {
        sprintf( szError, "Failed to register class %s:\nWinGetLastError() = 0x%X\n", SZ_WINDOWCLASS, WinGetLastError(hab) );
        fInitFailure = TRUE;
    }

    if ( !fInitFailure ) {

        memset( &global, 0, sizeof(global) );
        global.hab      = hab;
        global.hmq      = hmq;
        global.usClocks = 0;
        global.usCompactThreshold = 50;
        global.bDescWidth = 55;

        pszEnv = getenv("TZ");
        if (pszEnv) strncpy( global.szTZ, pszEnv, TZSTR_MAXZ );

        hwndFrame = WinCreateStdWindow( HWND_DESKTOP, 0L, &flStyle,
                                        SZ_WINDOWCLASS, "UniClock", 0L,
                                        NULLHANDLE, ID_MAINPROGRAM, &hwndClient );
        if ( hwndFrame == NULLHANDLE ) {
            sprintf( szError, "Failed to create application window:\nWinGetLastError() = 0x%X\n", WinGetLastError(hab) );
            fInitFailure = TRUE;
        }
    }

    if ( fInitFailure ) {
        WinMessageBox( HWND_DESKTOP, HWND_DESKTOP, szError, "Program Initialization Error", 0, MB_CANCEL | MB_ERROR );
    } else {
        WinSetWindowPtr( hwndClient, 0, &global );
        global.hwndTB = WinWindowFromID( hwndFrame, FID_TITLEBAR );
        global.hwndSM = WinWindowFromID( hwndFrame, FID_SYSMENU );
        global.hwndMM = WinWindowFromID( hwndFrame, FID_MINMAX );

        // Initialize online help
        if ( ! WinLoadString( hab, 0, IDS_HELP_TITLE, SZRES_MAXZ-1, szRes ))
            sprintf( szRes, "Help");

        helpInit.cb                       = sizeof( HELPINIT );
        helpInit.pszTutorialName          = NULL;
        helpInit.phtHelpTable             = (PHELPTABLE) MAKELONG( ID_MAINPROGRAM, 0xFFFF );
        helpInit.hmodHelpTableModule      = 0;
        helpInit.hmodAccelActionBarModule = 0;
        helpInit.fShowPanelId             = 0;
        helpInit.idAccelTable             = 0;
        helpInit.idActionBar              = 0;
        helpInit.pszHelpWindowTitle       = szRes;
        helpInit.pszHelpLibraryName       = HELP_FILE;

        hwndHelp = WinCreateHelpInstance( hab, &helpInit );
        WinAssociateHelpInstance( hwndHelp, hwndFrame );

        // load the popup menu
        global.hwndPopup = WinLoadMenu( HWND_DESKTOP, 0, IDM_POPUP );

        // Open the INI file
        OpenProfile( &global );

        // Initialize and display the user interface
        WindowSetup( hwndFrame, hwndClient );

        // Now run the main program message loop
        while ( WinGetMsg( hab, &qmsg, 0, 0, 0 )) WinDispatchMsg( hab, &qmsg );

        WinStopTimer( hab, hwndClient, ID_TIMER );

    }

    // Clean up and exit
    WinDestroyWindow( hwndFrame );
    WinDestroyMsgQueue( hmq );
    WinTerminate( hab );

    return ( 0 );
}


/* ------------------------------------------------------------------------- *
 * Window procedure for the main client window.                              *
 * ------------------------------------------------------------------------- */
MRESULT EXPENTRY MainWndProc( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 )
{
    PUCLGLOBAL pGlobal;
    SWP        swp;
    RECTL      rcl;
    POINTL     ptl;
    USHORT     i;
    CHAR       szRes[ SZRES_MAXZ ],   // buffer for string resources
               szMsg[ SZRES_MAXZ ];   // buffer for popup message


    switch( msg ) {

        case WM_BEGINDRAG:
            WinSetFocus( HWND_DESKTOP, hwnd );
            WinSendMsg( WinQueryWindow( hwnd, QW_PARENT ), WM_TRACKFRAME, MPFROMSHORT(TF_MOVE), MPVOID );
            break;


        case WM_CREATE:
            return (MRESULT) FALSE;


        case WM_COMMAND:
            switch( SHORT1FROMMP( mp1 )) {

                case ID_CONFIG:
                    ConfigNotebook( hwnd );
                    //WinPostMsg( hwnd, WM_SAVEAPPLICATION, 0, 0 );
                    break;

                case ID_CLOCKCFG:
                    pGlobal = WinQueryWindowPtr( hwnd, 0 );
                    if ( pGlobal->usMsgClock < pGlobal->usClocks ) {
                        ClockNotebook( hwnd, pGlobal->usMsgClock );
                        //if (ClockNotebook( hwnd, pGlobal->usMsgClock ))
                            //WinPostMsg( hwnd, WM_SAVEAPPLICATION, 0, 0 );
                    }
                    break;

                case ID_CLOCKADD:
                    pGlobal = WinQueryWindowPtr( hwnd, 0 );
                    if (pGlobal->usClocks < MAX_CLOCKS)
                    {
                        AddNewClock( hwnd );
                        ResizeClocks( hwnd );
                        //WinPostMsg( hwnd, WM_SAVEAPPLICATION, 0, 0 );
                    }

                    break;

                case ID_CLOCKDEL:
                    pGlobal = WinQueryWindowPtr( hwnd, 0 );
                    if ( pGlobal->usMsgClock < pGlobal->usClocks ) {
                        if ( ! WinLoadString( pGlobal->hab, NULLHANDLE, IDS_PROMPT_DELETE, SZRES_MAXZ-1, szRes ))
                            sprintf( szRes, "Delete clock %%u?");
                        sprintf( szMsg, szRes, pGlobal->usMsgClock+1 );
                        if ( ! WinLoadString( pGlobal->hab, NULLHANDLE, IDS_PROMPT_CONFIRM, SZRES_MAXZ-1, szRes ))
                            sprintf( szRes, "Please Confirm");
                        if ( WinMessageBox( HWND_DESKTOP, hwnd, szMsg, szRes, 0,
                                            MB_YESNO | MB_QUERY | MB_MOVEABLE ) == MBID_YES )
                        {
                            DeleteClock( hwnd, pGlobal, pGlobal->usMsgClock );
                            ResizeClocks( hwnd );
                            //WinPostMsg( hwnd, WM_SAVEAPPLICATION, 0, 0 );
                        }
                    }
                    break;

                case ID_ABOUT:                  // Product information dialog
                    WinDlgBox( HWND_DESKTOP, hwnd, (PFNWP) AboutDlgProc, 0, IDD_ABOUT, NULL );
                    break;

                case ID_QUIT:                   // Exit the program
                    WinPostMsg( hwnd, WM_CLOSE, 0, 0 );
                    return (MRESULT) 0;

                default: break;

            } // end WM_COMMAND messages
            return (MRESULT) 0;


        case WM_CONTEXTMENU:
            pGlobal = WinQueryWindowPtr( hwnd, 0 );
            ptl.x = SHORT1FROMMP( mp1 );
            ptl.y = SHORT2FROMMP( mp1 );

            // We need to remember which clock panel generated the popup event
            pGlobal->usMsgClock = 0xFFFF;
            for ( i = 0; i < pGlobal->usClocks; i++ ) {
                if ( pGlobal->clocks[i] && WinQueryWindowPos( pGlobal->clocks[i], &swp )) {
                    rcl.xLeft   = swp.x;
                    rcl.yBottom = swp.y;
                    rcl.xRight  = rcl.xLeft + swp.cx;
                    rcl.yTop    = rcl.yBottom + swp.cy;
                    if ( WinPtInRect( pGlobal->hab, &rcl, &ptl )) {
                        pGlobal->usMsgClock = i;
                        break;
                    }
                }
            }

            // Now show the popup menu
            WinMapWindowPoints( hwnd, HWND_DESKTOP, &ptl, 1 );
            WinPopupMenu( HWND_DESKTOP, hwnd, pGlobal->hwndPopup, ptl.x, ptl.y, 0,
                          PU_HCONSTRAIN | PU_VCONSTRAIN | PU_KEYBOARD | PU_MOUSEBUTTON1 );
            break;


        case WM_CONTROL:
            switch( SHORT1FROMMP( mp1 )) {
            } // end WM_CONTROL messages
            break;

        case WM_MENUEND:
            pGlobal = WinQueryWindowPtr( hwnd, 0 );
            if ( pGlobal->usMsgClock < pGlobal->usClocks )
                WinPostMsg( pGlobal->clocks[pGlobal->usMsgClock], WM_MENUEND, mp1, mp2 );
            break;

        case WM_SIZE:
            ResizeClocks( hwnd );
            break;

        case WM_PAINT:
            PaintClient( hwnd );
            break;

        case WM_TIMER:
            if ( SHORT1FROMMP(mp1) == ID_TIMER ) {
                UpdateTime( hwnd );
            }
            break;

        case WM_SAVEAPPLICATION:
            // Save the current settings to the INI file
            SaveSettings( hwnd );
            break;

        case WM_CLOSE:
            SaveSettings( hwnd );
            WinPostMsg( hwnd, WM_QUIT, 0, 0 );
            return (MRESULT) 0;


    } // end event handlers

    return ( WinDefWindowProc( hwnd, msg, mp1, mp2 ));

}


/* ------------------------------------------------------------------------- *
 * WindowSetup                                                               *
 *                                                                           *
 * Initialize, arrange, and position the application and user interface.     *
 *                                                                           *
 * ARGUMENTS:                                                                *
 *   HWND hwnd       : Handle of the main application frame.                 *
 *   HWND hwndClient : Handle of the application client area.                *
 *                                                                           *
 * RETURNS: BOOL                                                             *
 *   TRUE: success                                                           *
 *   FALSE: an error occured                                                 *
 * ------------------------------------------------------------------------- */
BOOL WindowSetup( HWND hwnd, HWND hwndClient )
{
    PUCLGLOBAL  pGlobal;
    WTDCDATA    wtInit;                // initialization data for clock panels
    CLKSTYLE    savedPP;               // presentation parameters from INI
    UconvObject uconv;                 // Unicode conversion object
    UniChar     uzRes[ SZRES_MAXZ ];   // buffer converted UCS-2 strings
    CHAR        szPrfKey[ 32 ],        // INI key name
                szRes[ SZRES_MAXZ ],   // buffer for string resources
                szFont[ FACESIZE+4 ];  // current font pres.param
    HPOINTER    hicon;                 // main application icon
    LONG        x  = 0,                // window position values (from INI)
                y  = 0,
                cx = 0,
                cy = 0;
    ULONG       clrB = 0x00404040,     // default border & separator colours
                clrS = 0x00A0A0A0,
                ulVal;
    USHORT      i,
                usVal;
    BYTE        bVal;
    BOOL        fLook,                 // appearance data loaded from INI successfully?
                fData;                 // clock settings loaded from INI successfully?


    pGlobal = WinQueryWindowPtr( hwndClient, 0 );

    if ( LoadIniData( &ulVal, sizeof(ULONG), pGlobal->hIni, PRF_APP_PREFS, PRF_KEY_FLAGS ))
        pGlobal->fsStyle = ulVal;
    if ( LoadIniData( &bVal, sizeof(CHAR), pGlobal->hIni, PRF_APP_PREFS, PRF_KEY_DESCWIDTH ))
        pGlobal->bDescWidth = bVal;
    if ( LoadIniData( &usVal, sizeof(USHORT), pGlobal->hIni, PRF_APP_PREFS, PRF_KEY_COMPACTTH ))
        pGlobal->usCompactThreshold = usVal;
    if ( LoadIniData( &usVal, sizeof(USHORT), pGlobal->hIni, PRF_APP_PREFS, PRF_KEY_PERCOLUMN ))
        pGlobal->usPerColumn = usVal;

    LoadIniData( &(pGlobal->usClocks), sizeof(USHORT), pGlobal->hIni, PRF_APP_CLOCKDATA, PRF_KEY_NUMCLOCKS );
    if (pGlobal->usClocks > MAX_CLOCKS)
        pGlobal->usClocks = MAX_CLOCKS;

    UniCreateUconvObject( (UniChar *) L"@map=display,path=no", &(uconv) );

    if ( !pGlobal->usClocks ) {
        // No saved clocks found in INI; create a default configuration
        pGlobal->usClocks = 2;

        if ( ! ( uconv &&
               ( WinLoadString( pGlobal->hab, NULLHANDLE, IDS_LOC_UTC, SZRES_MAXZ-1, szRes )) &&
               ( UniStrToUcs( uconv, uzRes, szRes, SZRES_MAXZ ) == ULS_SUCCESS )))
            UniStrcpy( uzRes, L"UTC");

        pGlobal->clocks[0] = WinCreateWindow( hwndClient, WT_DISPLAY, "", 0L,
                                              0, 0, 0, 0, hwndClient, HWND_TOP, FIRST_CLOCK, NULL, NULL );
        WinSendMsg( pGlobal->clocks[0], WTD_SETTIMEZONE, MPFROMP("UTC0"), MPFROMP(uzRes));
        WinSendMsg( pGlobal->clocks[0], WTD_SETOPTIONS,  MPFROMLONG( WTF_BORDER_FULL ), MPVOID );
        WinSetPresParam( pGlobal->clocks[0], PP_SEPARATORCOLOR, sizeof(ULONG), &clrS );
        WinSetPresParam( pGlobal->clocks[0], PP_BORDERCOLOR, sizeof(ULONG), &clrB );
        WinSetPresParam( pGlobal->clocks[0], PP_FONTNAMESIZE, 11, "9.WarpSans");

        if ( ! ( uconv &&
               ( WinLoadString( pGlobal->hab, NULLHANDLE, IDS_LOC_DEFAULT, SZRES_MAXZ-1, szRes )) &&
               ( UniStrToUcs( uconv, uzRes, szRes, SZRES_MAXZ ) == ULS_SUCCESS )))
            UniStrcpy( uzRes, L"(Current Location)");

        pGlobal->clocks[1] = WinCreateWindow( hwndClient, WT_DISPLAY, "", 0L,
                                              0, 0, 0, 0, hwndClient, HWND_TOP, FIRST_CLOCK+1, NULL, NULL );
        WinSendMsg( pGlobal->clocks[1], WTD_SETTIMEZONE, MPFROMP(pGlobal->szTZ), MPFROMP(uzRes));
        WinSendMsg( pGlobal->clocks[1], WTD_SETOPTIONS,  MPFROMLONG( WTF_BORDER_FULL & ~WTF_BORDER_BOTTOM ), MPVOID );
        WinSetPresParam( pGlobal->clocks[1], PP_SEPARATORCOLOR, sizeof(ULONG), &clrS );
        WinSetPresParam( pGlobal->clocks[1], PP_BORDERCOLOR, sizeof(ULONG), &clrB );
        WinSetPresParam( pGlobal->clocks[1], PP_FONTNAMESIZE, 11, "9.WarpSans");

/*
        // for testing -----
        pGlobal->usClocks = 3;
        wtInit.cb = sizeof( WTDCDATA );
        wtInit.flOptions = WTF_DATE_CUSTOM;
        sprintf( wtInit.szTZ, "JST-9");
        sprintf( wtInit.szLocale, "ja_JP");
        UniStrcpy( wtInit.uzDesc, L"Japan");
        UniStrcpy( wtInit.uzDateFmt, L"%Ex (%EC)");
        pGlobal->clocks[0] = WinCreateWindow( hwndClient, WT_DISPLAY, "", 0L,
                                            0, 0, 0, 0, hwndClient, HWND_TOP, FIRST_CLOCK, &wtInit, NULL );
        WinSetPresParam( pGlobal->clocks[0], PP_BORDERCOLOR, sizeof(ULONG), &clrB );
        WinSetPresParam( pGlobal->clocks[0], PP_SEPARATORCOLOR, sizeof(ULONG), &clrS );

        wtInit.flOptions = WTF_TIME_CUSTOM | WTF_BORDER_TOP | WTF_BORDER_BOTTOM;
        sprintf( wtInit.szTZ, "GMT0BST,3,-1,0,3600,10,-1,0,7200,3600");
        sprintf( wtInit.szLocale, "en_CA");
        UniStrcpy( wtInit.uzDesc, L"United Kingdom");
        UniStrcpy( wtInit.uzTimeFmt, L"%I:%M %p");
        pGlobal->clocks[1] = WinCreateWindow( hwndClient, WT_DISPLAY, "", 0L,
                                            0, 0, 0, 0, hwndClient, HWND_TOP, FIRST_CLOCK+1, &wtInit, NULL );
        WinSetPresParam( pGlobal->clocks[1], PP_BORDERCOLOR, sizeof(ULONG), &clrB );
        WinSetPresParam( pGlobal->clocks[1], PP_SEPARATORCOLOR, sizeof(ULONG), &clrS );

        wtInit.flOptions = 0;
        sprintf( wtInit.szTZ, "EST5EDT,3,2,0,7200,11,1,0,7200,3600");
        sprintf( wtInit.szLocale, "");
        UniStrcpy( wtInit.uzDesc, L"Canada (Eastern)");
        pGlobal->clocks[2] = WinCreateWindow( hwndClient, WT_DISPLAY, "", 0L,
                                            0, 0, 0, 0, hwndClient, HWND_TOP, FIRST_CLOCK+2, &wtInit, NULL );
        WinSetPresParam( pGlobal->clocks[2], PP_BORDERCOLOR, sizeof(ULONG), &clrB );
        WinSetPresParam( pGlobal->clocks[2], PP_SEPARATORCOLOR, sizeof(ULONG), &clrS );
        // for testing -----
*/
    }
    else for ( i = 0; i < pGlobal->usClocks; i++ ) {
        // Load each clock's configuration from the INI

        sprintf( szPrfKey, "%s%02d", PRF_KEY_PANEL, i );
        fData = LoadIniData( &wtInit, sizeof(wtInit), pGlobal->hIni, PRF_APP_CLOCKDATA, szPrfKey );
        if ( !fData ) {
            memset( &wtInit, 0, sizeof( wtInit ));
            if ( ! ( uconv &&
                   ( WinLoadString( pGlobal->hab, NULLHANDLE, IDS_LOC_DEFAULT, SZRES_MAXZ-1, szRes )) &&
                   ( UniStrToUcs( uconv, wtInit.uzDesc, szRes, SZRES_MAXZ ) == ULS_SUCCESS )))
                UniStrcpy( wtInit.uzDesc, L"(Current Location)");
            wtInit.cb = sizeof( WTDCDATA );
            wtInit.flOptions = WTF_DATE_SYSTEM | WTF_TIME_SYSTEM;
            sprintf( wtInit.szTZ, pGlobal->szTZ );
            UniStrcpy( wtInit.uzDateFmt, L"%x");
            UniStrcpy( wtInit.uzTimeFmt, L"%X");
        }
        sprintf( szPrfKey, "%s%02d", PRF_KEY_PRESPARAM, i );
        fLook = LoadIniData( &savedPP, sizeof(savedPP), pGlobal->hIni, PRF_APP_CLOCKDATA, szPrfKey );
        wtInit.flOptions |= WTF_BORDER_FULL;
        if ( i ) wtInit.flOptions &= ~WTF_BORDER_BOTTOM;
        pGlobal->clocks[i] = WinCreateWindow( hwndClient, WT_DISPLAY, "", 0L,
                                              0, 0, 0, 0, hwndClient, HWND_TOP, FIRST_CLOCK+i, &wtInit, NULL );
        if ( ! pGlobal->clocks[i] ) continue;

        if ( fLook ) {
            if ( savedPP.clrFG != NO_COLOUR_PP )
                WinSetPresParam( pGlobal->clocks[i], PP_FOREGROUNDCOLOR, sizeof(ULONG), &(savedPP.clrFG) );
            if ( savedPP.clrBG != NO_COLOUR_PP )
                WinSetPresParam( pGlobal->clocks[i], PP_BACKGROUNDCOLOR, sizeof(ULONG), &(savedPP.clrBG) );
            if ( savedPP.clrBor != NO_COLOUR_PP )
                WinSetPresParam( pGlobal->clocks[i], PP_BORDERCOLOR, sizeof(ULONG), &(savedPP.clrBor) );
            if ( savedPP.clrSep != NO_COLOUR_PP )
                WinSetPresParam( pGlobal->clocks[i], PP_SEPARATORCOLOR, sizeof(ULONG), &(savedPP.clrSep) );
            if ( savedPP.szFont[0] ) {
                sprintf( szFont, "10.%s", savedPP.szFont );
                WinSetPresParam( pGlobal->clocks[i], PP_FONTNAMESIZE, strlen(szFont)+1, szFont );
            }
        }
    }

    // Hide the titlebar if appropriate
    ToggleTitleBar( pGlobal, WinQueryWindow( hwnd, QW_PARENT ), FALSE );

    // Set the window mini-icon
    hicon = WinLoadPointer( HWND_DESKTOP, 0, ID_MAINPROGRAM );
    WinSendMsg( hwnd, WM_SETICON, MPFROMP(hicon), MPVOID );

    // Start the clock(s)
    UpdateTime( hwndClient );
    pGlobal->timer = WinStartTimer( pGlobal->hab, hwndClient, ID_TIMER, 100 );

    // Check for saved position & size settings
    LoadIniData( &x,  sizeof(LONG), pGlobal->hIni, PRF_APP_POSITION, PRF_KEY_LEFT );
    LoadIniData( &y,  sizeof(LONG), pGlobal->hIni, PRF_APP_POSITION, PRF_KEY_BOTTOM );
    LoadIniData( &cx, sizeof(LONG), pGlobal->hIni, PRF_APP_POSITION, PRF_KEY_WIDTH );
    LoadIniData( &cy, sizeof(LONG), pGlobal->hIni, PRF_APP_POSITION, PRF_KEY_HEIGHT );

    if ( !(x && y && cx && cy )) {
        LONG scr_width, scr_height;
        scr_width  = WinQuerySysValue( HWND_DESKTOP, SV_CXSCREEN );
        scr_height = WinQuerySysValue( HWND_DESKTOP, SV_CYSCREEN );
        cx = 250;
        cy = 175;
        x = ( scr_width - cx ) / 2;
        y = ( scr_height - cy ) / 2;
    }
    WinSetWindowPos( hwnd, HWND_TOP, x, y, cx, cy, SWP_SHOW | SWP_MOVE | SWP_SIZE | SWP_ACTIVATE );

    if ( uconv ) UniFreeUconvObject( uconv );

    return TRUE;
}


/* ------------------------------------------------------------------------- *
 * ResizeClocks                                                              *
 *                                                                           *
 * Handles resizing the main client window or adding/removing clocks.        *
 * ------------------------------------------------------------------------- */
void ResizeClocks( HWND hwnd )
{
    PUCLGLOBAL pGlobal;
    RECTL      rcl;
    USHORT     i;
    ULONG      flOpts;
    LONG       x, y, cx, cy, cy2;

    pGlobal = WinQueryWindowPtr( hwnd, 0 );
    if (pGlobal && pGlobal->usClocks)
    {
        WinQueryWindowRect( hwnd, &rcl );

        x  = rcl.xLeft;
        y  = rcl.yBottom;
        cx = rcl.xRight;
        cy2 = (rcl.yTop - y) / pGlobal->usClocks;
        cy = cy2 + ((rcl.yTop - y) % pGlobal->usClocks);

        for ( i = 0; i < pGlobal->usClocks; i++ ) {
            if ( ! pGlobal->clocks[i] ) break;
            flOpts = (ULONG) WinSendMsg( pGlobal->clocks[i], WTD_QUERYOPTIONS, MPVOID, MPVOID );
            if ( cy < pGlobal->usCompactThreshold )
                flOpts |= WTF_MODE_COMPACT;
            else
                flOpts &= ~WTF_MODE_COMPACT;
            WinSendMsg( pGlobal->clocks[i], WTD_SETOPTIONS, MPFROMLONG(flOpts), MPVOID );
            WinSetWindowPos( pGlobal->clocks[i], HWND_TOP, x, y, cx, cy, SWP_SIZE | SWP_MOVE | SWP_SHOW );
            y += cy;
            cy = cy2;
        }
    }
}


/* ------------------------------------------------------------------------- *
 * PaintClient                                                               *
 *                                                                           *
 * Handles (re)painting the main client window.                              *
 * ------------------------------------------------------------------------- */
MRESULT PaintClient( HWND hwnd )
{
    PUCLGLOBAL pGlobal;
    HPS        hps;
    RECTL      rcl;

    hps = WinBeginPaint( hwnd, NULLHANDLE, NULLHANDLE );
    if (hps)
    {
        pGlobal = WinQueryWindowPtr( hwnd, 0 );
        if (pGlobal && (pGlobal->usClocks == 0))
        {
            WinQueryWindowRect( hwnd, &rcl );
            WinFillRect( hps, &rcl, SYSCLR_DIALOGBACKGROUND );
        }

        WinEndPaint( hps );
    }

    return ( 0 );
}


/* ------------------------------------------------------------------------- *
 * CentreWindow                                                              *
 *                                                                           *
 * Centres the given window on the screen.                                   *
 *                                                                           *
 * ARGUMENTS:                                                                *
 *   HWND hwnd: Handle of the window to be centred.                          *
 *                                                                           *
 * RETURNS: N/A                                                              *
 * ------------------------------------------------------------------------- */
void CentreWindow( HWND hwnd )
{
    LONG scr_width, scr_height;
    LONG x, y;
    SWP wp;

    scr_width = WinQuerySysValue( HWND_DESKTOP, SV_CXSCREEN );
    scr_height = WinQuerySysValue( HWND_DESKTOP, SV_CYSCREEN );

    if ( WinQueryWindowPos( hwnd, &wp )) {
        x = ( scr_width - wp.cx ) / 2;
        y = ( scr_height - wp.cy ) / 2;
        WinSetWindowPos( hwnd, HWND_TOP, x, y, wp.cx, wp.cy, SWP_SHOW | SWP_MOVE | SWP_ACTIVATE );
    }

}


/* ------------------------------------------------------------------------- *
 * ToggleTitleBar                                                            *
 *                                                                           *
 * Turns the frame window titlebar on or off.                                *
 *                                                                           *
 * ARGUMENTS:                                                                *
 *   PUCLGLOBAL pGlobal: Global window data                                  *
 *   HWND hwndFrame: Handle of the frame window.                             *
 *   BOOL fOn      : Indicates new state of titlebar.                        *
 *                                                                           *
 * RETURNS: N/A                                                              *
 * ------------------------------------------------------------------------- */
void ToggleTitleBar( PUCLGLOBAL pGlobal, HWND hwndFrame, BOOL fOn )
{
    ULONG flTBFlags = FCF_TITLEBAR | FCF_SYSMENU | FCF_MINMAX;

    if ( fOn ) {
        WinSetParent( pGlobal->hwndTB, hwndFrame, FALSE );
        WinSetParent( pGlobal->hwndSM, hwndFrame, FALSE );
        WinSetParent( pGlobal->hwndMM, hwndFrame, FALSE );
        WinSendMsg( pGlobal->hwndTB, TBM_SETHILITE, MPFROMSHORT(TRUE), 0L );
    }
    else {
        WinSetParent( pGlobal->hwndTB, HWND_OBJECT, FALSE );
        WinSetParent( pGlobal->hwndSM, HWND_OBJECT, FALSE );
        WinSetParent( pGlobal->hwndMM, HWND_OBJECT, FALSE );
    }
    WinSendMsg( hwndFrame, WM_UPDATEFRAME, (MPARAM) flTBFlags, 0L );
}


/* ------------------------------------------------------------------------- *
 * OpenProfile                                                               *
 *                                                                           *
 * Figure out where our INI file should be located (in the same directory as *
 * the program executable) and open it.                                      *
 *                                                                           *
 * ARGUMENTS:                                                                *
 *     PUCLGLOBAL pGlobal: pointer to global data structure                  *
 *                                                                           *
 * RETURNS: N/A                                                              *
 * ------------------------------------------------------------------------- */
void OpenProfile( PUCLGLOBAL pGlobal )
{
    PPIB   ppib;
    CHAR   szIni[ CCHMAXPATH ] = {0};
    PSZ    c;

    // Query the current executable path
    DosGetInfoBlocks( NULL, &ppib );
    strncpy( szIni, ppib->pib_pchcmd, CCHMAXPATH-1 );

    // Now change the filename portion to point to our own INI file
    if (( c = strrchr( szIni, '\\') + 1 ) != NULL ) {
        memset( c, 0, strlen(c) );
        strncat( szIni, INI_FILE, CCHMAXPATH - 1 );
    } else {
        sprintf( szIni, INI_FILE );
    }

    pGlobal->hIni = PrfOpenProfile( pGlobal->hab, szIni );
}


/* ------------------------------------------------------------------------- *
 * LoadIniData                                                               *
 *                                                                           *
 * Retrieve a value from the program INI file and writes it into the buffer  *
 * provided.  If the size of the INI data does not match the provided buffer *
 * size, no action is taken (this is done to avoid version mismatches        *
 * between saved INI data and updated program data structures).              *
 *                                                                           *
 * ARGUMENTS:                                                                *
 *     PVOID  pData : Buffer where the data will be written.                 *
 *     USHORT cb    : Size of pData, in bytes                                *
 *     HINI   hIni  : Handle to the INI file.                                *
 *     PSZ    pszApp: INI application name.                                  *
 *     PSZ    pszKey: INI key name.                                          *
 *                                                                           *
 * RETURNS: BOOL                                                             *
 *     TRUE if the data was read, FALSE otherwise.                           *
 * ------------------------------------------------------------------------- */
BOOL LoadIniData( PVOID pData, USHORT cb, HINI hIni, PSZ pszApp, PSZ pszKey )
{
    BOOL  fOK;
    ULONG ulBytes;

    fOK = PrfQueryProfileSize( hIni, pszApp, pszKey, &ulBytes );
    if ( fOK && ( ulBytes > 0 ) && ( ulBytes == cb )) {
        PrfQueryProfileData( hIni, pszApp, pszKey, pData, &ulBytes );
        return TRUE;
    }
    return FALSE;
}


/* ------------------------------------------------------------------------- *
 * SaveSettings                                                              *
 *                                                                           *
 * Saves various settings to the INI file.  Called on program exit.          *
 *                                                                           *
 * ARGUMENTS:                                                                *
 *   HWND hwnd: Client window handle.                                        *
 *                                                                           *
 * RETURNS: N/A                                                              *
 * ------------------------------------------------------------------------- */
void SaveSettings( HWND hwnd )
{
    PUCLGLOBAL pGlobal;              // global data
    WTDCDATA wtdconfig;              // clock configuration data
    CLKSTYLE clkstyle;               // written presentation parameters
    CHAR     szPrfKey[ 32 ],         // INI key name
             szFont[ FACESIZE+4 ];   // current font pres.param
    SWP      wp;                     // window size/position
    PSZ      psz;                    // pointer to string offset
    USHORT   i;
    ULONG    ulid,                   // required by WinQueryPresParam, not otherwise used
             clr;                    // current colour pres. param


    pGlobal = WinQueryWindowPtr( hwnd, 0 );

    // Save the window position
    if ( WinQueryWindowPos( WinQueryWindow(hwnd, QW_PARENT), &wp )) {
        PrfWriteProfileData( pGlobal->hIni, PRF_APP_POSITION, PRF_KEY_LEFT,   &(wp.x),  sizeof(wp.x) );
        PrfWriteProfileData( pGlobal->hIni, PRF_APP_POSITION, PRF_KEY_BOTTOM, &(wp.y),  sizeof(wp.y) );
        PrfWriteProfileData( pGlobal->hIni, PRF_APP_POSITION, PRF_KEY_WIDTH,  &(wp.cx), sizeof(wp.cx) );
        PrfWriteProfileData( pGlobal->hIni, PRF_APP_POSITION, PRF_KEY_HEIGHT, &(wp.cy), sizeof(wp.cy) );
    }

    // Save the global options
    PrfWriteProfileData( pGlobal->hIni, PRF_APP_PREFS, PRF_KEY_FLAGS,
                         &(pGlobal->fsStyle), sizeof(pGlobal->fsStyle) );
    PrfWriteProfileData( pGlobal->hIni, PRF_APP_PREFS, PRF_KEY_DESCWIDTH,
                         &(pGlobal->bDescWidth), sizeof(pGlobal->bDescWidth) );
    PrfWriteProfileData( pGlobal->hIni, PRF_APP_PREFS, PRF_KEY_COMPACTTH,
                         &(pGlobal->usCompactThreshold), sizeof(pGlobal->usCompactThreshold) );
    PrfWriteProfileData( pGlobal->hIni, PRF_APP_PREFS, PRF_KEY_PERCOLUMN,
                         &(pGlobal->usPerColumn), sizeof(pGlobal->usPerColumn) );

    PrfWriteProfileData( pGlobal->hIni, PRF_APP_CLOCKDATA, PRF_KEY_NUMCLOCKS,
                         &(pGlobal->usClocks), sizeof(pGlobal->usClocks) );

    // Save the current settings of each clock
    for ( i = 0; i < pGlobal->usClocks; i++ ) {
        if ( ! pGlobal->clocks[i] ) continue;
        memset( &clkstyle, 0, sizeof(CLKSTYLE) );
        memset( &wtdconfig, 0, sizeof(WTDCDATA) );

        // Save the presentation parameters (colours & font)

        if ( WinQueryPresParam( pGlobal->clocks[i], PP_FOREGROUNDCOLOR, PP_FOREGROUNDCOLORINDEX, &ulid, sizeof(clr), &clr, QPF_ID2COLORINDEX ))
            clkstyle.clrFG = clr;
        else
            clkstyle.clrFG = NO_COLOUR_PP;

        if ( WinQueryPresParam( pGlobal->clocks[i], PP_BACKGROUNDCOLOR, PP_BACKGROUNDCOLORINDEX, &ulid, sizeof(clr), &clr, QPF_ID2COLORINDEX ))
            clkstyle.clrBG = clr;
        else
            clkstyle.clrBG = NO_COLOUR_PP;

        if ( WinQueryPresParam( pGlobal->clocks[i], PP_BORDERCOLOR, 0, &ulid, sizeof(clr), &clr, 0 ))
            clkstyle.clrBor = clr;
        else
            clkstyle.clrBor = NO_COLOUR_PP;

        if ( WinQueryPresParam( pGlobal->clocks[i], PP_SEPARATORCOLOR, 0, &ulid, sizeof(clr), &clr, QPF_NOINHERIT ))
            clkstyle.clrSep = clr;
        else
            clkstyle.clrSep = NO_COLOUR_PP;

        if ( WinQueryPresParam( pGlobal->clocks[i], PP_FONTNAMESIZE, 0, NULL, sizeof(szFont), szFont, QPF_NOINHERIT ))
        {
            psz = strchr( szFont, '.') + 1;
            strncpy( clkstyle.szFont, psz, FACESIZE );
        }

        sprintf( szPrfKey, "%s%02d", PRF_KEY_PRESPARAM, i );
        PrfWriteProfileData( pGlobal->hIni, PRF_APP_CLOCKDATA, szPrfKey, &(clkstyle), sizeof(clkstyle) );

        // Save the setup options (via a WTDCDATA structure)
        wtdconfig.cb = sizeof(WTDCDATA);
        if ( WinSendMsg( pGlobal->clocks[i], WTD_QUERYCONFIG, MPFROMP(&wtdconfig), MPVOID ))
        {
            sprintf( szPrfKey, "%s%02d", PRF_KEY_PANEL, i );
            PrfWriteProfileData( pGlobal->hIni, PRF_APP_CLOCKDATA, szPrfKey, &(wtdconfig), sizeof(wtdconfig) );
        } else {
            ErrorPopup("Failed to get clock data.");    //TODO NLS
        }

    }
    // TODO delete any leftover entries from deleted panels

    PrfCloseProfile( pGlobal->hIni );
}


/* ------------------------------------------------------------------------- *
 * AddNewClock                                                               *
 *                                                                           *
 * Add a new clock panel (local timezone and default font/colours).  Once    *
 * created, the configuration dialog is opened for that clock.  If the       *
 * dialog is cancelled, the new clock is deleted; otherwise, it is added to  *
 * the global array of running clocks.                                       *
 *                                                                           *
 * ARGUMENTS:                                                                *
 *   HWND hwnd: Client window handle.                                        *
 *                                                                           *
 * RETURNS: BOOL                                                             *
 *   TRUE if clock was created; FALSE otherwise                              *
 * ------------------------------------------------------------------------- */
BOOL AddNewClock( HWND hwnd )
{
    PUCLGLOBAL  pGlobal;              // global data
    UconvObject uconv;                // Unicode conversion object
    HWND        hwndClock;            // new clock hwnd
    ULONG       flOptions,            // clock options
                clrB = 0x00404040,
                clrS = 0x00A0A0A0;
    USHORT      usNew;                // new clock number
    CHAR        szRes[ SZRES_MAXZ ];   // buffer for string resources
    UniChar     uzRes[ SZRES_MAXZ ];   // buffer converted UCS-2 strings

    pGlobal = WinQueryWindowPtr( hwnd, 0 );
    usNew = pGlobal->usClocks;

    // create new clock with default settings (local time, default colours)
    hwndClock = WinCreateWindow( hwnd, WT_DISPLAY, "", 0L, 0, 0, 0, 0, hwnd,
                                 HWND_TOP, FIRST_CLOCK + usNew, NULL, NULL );
    if ( !hwndClock ) return FALSE;

    pGlobal->clocks[ usNew ] = hwndClock;

    UniCreateUconvObject( (UniChar *) L"@map=display,path=no", &(uconv) );
    if ( ! ( uconv &&
           ( WinLoadString( pGlobal->hab, NULLHANDLE, IDS_LOC_DEFAULT, SZRES_MAXZ-1, szRes )) &&
           ( UniStrToUcs( uconv, uzRes, szRes, SZRES_MAXZ ) == ULS_SUCCESS )))
        UniStrcpy( uzRes, L"UTC");

    // only clock 0 should have a bottom border
    flOptions = WTF_BORDER_FULL;
    if ( usNew ) flOptions &= ~WTF_BORDER_BOTTOM;

    // set default properties
    WinSendMsg( hwndClock, WTD_SETTIMEZONE, MPFROMP(pGlobal->szTZ), MPFROMP(uzRes));
    WinSendMsg( hwndClock, WTD_SETOPTIONS,  MPFROMLONG(flOptions), MPVOID );
    WinSetPresParam( hwndClock, PP_SEPARATORCOLOR, sizeof(ULONG), &clrS );
    WinSetPresParam( hwndClock, PP_BORDERCOLOR, sizeof(ULONG), &clrB );
    WinSetPresParam( hwndClock, PP_FONTNAMESIZE, 11, "9.WarpSans");

#if 0
    pGlobal->usClocks++;
    WinInvalidateRect( hwnd, NULL, TRUE );
#else
    // now show the properties dialog
    if ( ClockNotebook( hwnd, usNew ) ) {
        pGlobal->usClocks++;
    }
    else {
        // delete the new clock if the user cancelled
        WinDestroyWindow( pGlobal->clocks[usNew] );
        pGlobal->clocks[usNew] = NULLHANDLE;
    }
#endif

    if ( uconv ) UniFreeUconvObject( uconv );
    return TRUE;
}


/* ------------------------------------------------------------------------- *
 * DeleteClock                                                               *
 *                                                                           *
 * Delete a clock panel.                      .                              *
 * ------------------------------------------------------------------------- */
void DeleteClock( HWND hwnd, PUCLGLOBAL pGlobal, USHORT usDelete )
{
    HWND   clocks_tmp[ MAX_CLOCKS ];
    USHORT i, j;

    // remove the deleted clock from the array of clock HWNDs
    memcpy( &clocks_tmp, &(pGlobal->clocks), sizeof(pGlobal->clocks) );
    memset( &(pGlobal->clocks), 0, sizeof(pGlobal->clocks) );
    for ( i = 0, j = 0; i < pGlobal->usClocks; i++ ) {
        if ( i != usDelete )
            pGlobal->clocks[ j++ ] = clocks_tmp[ i ];
    }

    pGlobal->usClocks--;
    WinDestroyWindow( clocks_tmp[usDelete] );

    // The bottom-most clock (clock 0, and only 0) should have a full border.
    // So if we deleted that clock, we change the border on the new clock 0.
    if ( usDelete == 0 )
        WinSendMsg( pGlobal->clocks[0], WTD_SETOPTIONS,
                    MPFROMLONG( WTF_BORDER_FULL ), MPVOID );

}


/* ------------------------------------------------------------------------- *
 * UpdateTime                                                                *
 *                                                                           *
 * Updates the time in every clock panel.                                    *
 * ------------------------------------------------------------------------- */
void UpdateTime( HWND hwnd )
{
    static time_t oldtime;
    PUCLGLOBAL pGlobal;     // global data
    time_t     newtime;
    CHAR       szEnv[ TZSTR_MAXZ+3 ];
    USHORT     i;

    pGlobal = WinQueryWindowPtr( hwnd, 0 );

    if ( pGlobal->szTZ[0] ) {
        sprintf( szEnv, "TZ=%s", pGlobal->szTZ );
        putenv( szEnv );
    }

    tzset();
    newtime = time( NULL );
    if ( newtime == oldtime ) return;
    oldtime = newtime;

    for ( i = 0; i < pGlobal->usClocks; i++ ) {
        if ( ! pGlobal->clocks[i] ) break;
        WinSendMsg( pGlobal->clocks[i], WTD_SETDATETIME, MPFROMLONG(newtime), MPVOID );
    }
}


/* ------------------------------------------------------------------------- *
 * AboutDlgProc                                                              *
 *                                                                           *
 * Dialog procedure for the product information dialog.                      *
 * ------------------------------------------------------------------------- */
MRESULT EXPENTRY AboutDlgProc( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 )
{
    switch ( msg ) {

        case WM_INITDLG:
            CentreWindow( hwnd );
            break;

        default: break;
    }

    return WinDefDlgProc( hwnd, msg, mp1, mp2 );
}


/* ------------------------------------------------------------------------- *
 * ConfigNotebook                                                            *
 *                                                                           *
 * Initialize and open the configuration notebook dialog.                    *
 * ------------------------------------------------------------------------- */
void ConfigNotebook( HWND hwnd )
{
    UCFGDATA   config;
    PUCLGLOBAL pGlobal;
    PSZ        psz;
    USHORT     i;
    ULONG      rc,
               ulid,
               clr;
    UCHAR      szFont[ FACESIZE + 4 ];


    pGlobal = WinQueryWindowPtr( hwnd, 0 );

    memset( &config, 0, sizeof(UCFGDATA) );
    config.hab = pGlobal->hab;
    config.hmq = pGlobal->hmq;
    config.usClocks = pGlobal->usClocks;
    config.fsStyle = pGlobal->fsStyle;
    config.usCompactThreshold = pGlobal->usCompactThreshold;
    config.bDescWidth = pGlobal->bDescWidth;
    config.usPerColumn = pGlobal->usPerColumn;

    rc = UniCreateUconvObject( (UniChar *) L"@map=display,path=no", &(config.uconv) );
    if ( rc != ULS_SUCCESS) config.uconv = NULL;

    // Query the current settings of each clock
    for ( i = 0; i < pGlobal->usClocks; i++ ) {
        if ( ! pGlobal->clocks[i] ) continue;

        // colours & fonts
        if ( WinQueryPresParam( pGlobal->clocks[i], PP_FOREGROUNDCOLOR,
                                PP_FOREGROUNDCOLORINDEX, &ulid,
                                sizeof(clr), &clr, QPF_ID2COLORINDEX ))
            config.aClockStyle[ i ].clrFG = clr;
        else
            config.aClockStyle[ i ].clrFG = NO_COLOUR_PP;

        if ( WinQueryPresParam( pGlobal->clocks[i], PP_BACKGROUNDCOLOR,
                                PP_BACKGROUNDCOLORINDEX, &ulid,
                                sizeof(clr), &clr, QPF_ID2COLORINDEX ))
            config.aClockStyle[ i ].clrBG = clr;
        else
            config.aClockStyle[ i ].clrBG = NO_COLOUR_PP;

        if ( WinQueryPresParam( pGlobal->clocks[i], PP_BORDERCOLOR,
                                0, &ulid, sizeof(clr), &clr, 0 ))
            config.aClockStyle[ i ].clrBor = clr;
        else
            config.aClockStyle[ i ].clrBor = NO_COLOUR_PP;

        if ( WinQueryPresParam( pGlobal->clocks[i], PP_SEPARATORCOLOR,
                                0, &ulid, sizeof(clr), &clr, QPF_NOINHERIT ))
            config.aClockStyle[ i ].clrSep = clr;
        else
            config.aClockStyle[ i ].clrSep = NO_COLOUR_PP;

        if ( WinQueryPresParam( pGlobal->clocks[i], PP_FONTNAMESIZE, 0,
                                NULL, sizeof(szFont), szFont, QPF_NOINHERIT ))
        {
            psz = strchr( szFont, '.') + 1;
            strncpy( config.aClockStyle[ i ].szFont, psz, FACESIZE );
        }

        // configurable settings
        config.aClockData[ i ].cb = sizeof(WTDCDATA);
        WinSendMsg( pGlobal->clocks[i], WTD_QUERYCONFIG, MPFROMP(&(config.aClockData[i])), MPVOID );
    }

    WinDlgBox( HWND_DESKTOP, hwnd, (PFNWP) CfgDialogProc, 0, IDD_CONFIG, &config );

    if ( config.bChanged ) {
        // Apply changes
        if (( config.fsStyle & APP_STYLE_TITLEBAR ) && !( pGlobal->fsStyle & APP_STYLE_TITLEBAR ))
            ToggleTitleBar( pGlobal, WinQueryWindow( hwnd, QW_PARENT ), TRUE );
        else if ( !( config.fsStyle & APP_STYLE_TITLEBAR ) && ( pGlobal->fsStyle & APP_STYLE_TITLEBAR ))
            ToggleTitleBar( pGlobal, WinQueryWindow( hwnd, QW_PARENT ), FALSE );
        pGlobal->fsStyle = config.fsStyle;
        pGlobal->bDescWidth = config.bDescWidth;
        pGlobal->usPerColumn = config.usPerColumn;
        pGlobal->usCompactThreshold = config.usCompactThreshold;
        for ( i = 0; i < pGlobal->usClocks; i++ ) {
            WinSendMsg( pGlobal->clocks[i], WTD_SETSEPARATOR,
                        MPFROMCHAR(pGlobal->bDescWidth), 0L );
        }
        ResizeClocks( hwnd );
    }

    UniFreeUconvObject( config.uconv );
}


/* ------------------------------------------------------------------------- *
 * CfgDialogProc                                                             *
 *                                                                           *
 * Dialog procedure for the application configuration dialog.                *
 * ------------------------------------------------------------------------- */
MRESULT EXPENTRY CfgDialogProc( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 )
{
    static PUCFGDATA pConfig;
    HPOINTER  hicon;
    HWND      hwndPage;

    switch ( msg ) {

        case WM_INITDLG:
            hicon = WinLoadPointer( HWND_DESKTOP, 0, ID_MAINPROGRAM );
            WinSendMsg( hwnd, WM_SETICON, (MPARAM) hicon, MPVOID );

            pConfig = (PUCFGDATA) mp2;

            if ( ! CfgPopulateNotebook( hwnd, pConfig ) ) {
                ErrorPopup("Error populating notebook.");   // TODO NLS
                return (MRESULT) TRUE;
            }
            CentreWindow( hwnd );
            return (MRESULT) FALSE;


        case WM_COMMAND:
            switch( SHORT1FROMMP( mp1 )) {

                case DID_OK:
                    hwndPage = (HWND) WinSendDlgItemMsg( hwnd, IDD_CFGBOOK, BKM_QUERYPAGEWINDOWHWND, MPFROMLONG(pConfig->ulPage1), 0 );
                    if ( hwndPage && hwndPage != BOOKERR_INVALID_PARAMETERS )
                        CfgSettingsCommon( hwndPage, pConfig );
                    pConfig->bChanged = TRUE;
                    break;

                default: break;
            } // end WM_COMMAND messages
            break;


        case WM_CONTROL:
            switch( SHORT1FROMMP( mp1 )) {
            } // end WM_CONTROL messages
            break;


        default: break;
    }

    return WinDefDlgProc( hwnd, msg, mp1, mp2 );
}


/* ------------------------------------------------------------------------- *
 * CfgPopulateNotebook()                                                     *
 *                                                                           *
 * Creates the pages (dialogs) of the configuration notebook.                *
 *                                                                           *
 * ARGUMENTS:                                                                *
 *     HWND hwnd        : HWND of the properties dialog.                     *
 *     PUCFGDATA pConfig: Pointer to common configuration data.              *
 *                                                                           *
 * RETURNS: BOOL                                                             *
 *     FALSE if an error occurred.  TRUE otherwise.                          *
 * ------------------------------------------------------------------------- */
BOOL CfgPopulateNotebook( HWND hwnd, PUCFGDATA pConfig )
{
    ULONG          pageId;
    HWND           hwndPage,
                   hwndBook;
    NOTEBOOKBUTTON bookButtons[ 3 ];
    USHORT         fsCommon = BKA_AUTOPAGESIZE | BKA_STATUSTEXTON;
    CHAR           szOK[ SZRES_MAXZ ],
                   szCancel[ SZRES_MAXZ ],
                   szHelp[ SZRES_MAXZ ];


    hwndBook = WinWindowFromID( hwnd, IDD_CFGBOOK );

    if ( ! WinLoadString( pConfig->hab, 0, IDS_BTN_OK, SZRES_MAXZ-1, szOK ))
        sprintf( szOK, "OK");
    if ( ! WinLoadString( pConfig->hab, 0, IDS_BTN_CANCEL, SZRES_MAXZ-1, szCancel ))
        sprintf( szOK, "Cancel");
    if ( ! WinLoadString( pConfig->hab, 0, IDS_BTN_HELP, SZRES_MAXZ-1, szHelp ))
        sprintf( szOK, "~Help");

    // create common buttons
    bookButtons[0].pszText  = szOK;
    bookButtons[0].idButton = DID_OK;
    bookButtons[0].flStyle  = BS_AUTOSIZE | BS_DEFAULT;
    bookButtons[1].pszText  = szCancel;
    bookButtons[1].idButton = DID_CANCEL;
    bookButtons[1].flStyle  = BS_AUTOSIZE;
    bookButtons[2].pszText  = szHelp;
    bookButtons[2].idButton = DID_HELP;
    bookButtons[2].flStyle  = BS_AUTOSIZE | BS_HELP;
    WinSendMsg( hwndBook, BKM_SETNOTEBOOKBUTTONS, MPFROMLONG( 3 ), MPFROMP( bookButtons ));

    // this should make the notebook tabs usable if the program is run on Warp 3
    WinSendMsg( hwndBook, BKM_SETDIMENSIONS, MPFROM2SHORT( 75, 22 ), MPFROMSHORT( BKA_MAJORTAB ));

    // add the common settings page
    hwndPage = WinLoadDlg( hwndBook, hwndBook, (PFNWP) CfgCommonPageProc, 0, IDD_CFGCOMMON, pConfig );
    pageId = (ULONG) WinSendMsg( hwndBook, BKM_INSERTPAGE, NULL,
                                 MPFROM2SHORT( fsCommon | BKA_MAJOR, BKA_FIRST ));
    if ( ! pageId ) return FALSE;
    if ( ! WinSendMsg( hwndBook, BKM_SETTABTEXT, MPFROMLONG( pageId ), MPFROMP("~Common")))                     // TODO NLS
        return FALSE;
    if ( ! WinSendMsg( hwndBook, BKM_SETPAGEWINDOWHWND, MPFROMLONG( pageId ), MPFROMP( hwndPage )))
        return FALSE;
    pConfig->ulPage1 = pageId;

    return TRUE;
}


/* ------------------------------------------------------------------------- *
 * CfgCommonPageProc                                                         *
 *                                                                           *
 * Dialog procedure for the common settings page.                            *
 * ------------------------------------------------------------------------- */
MRESULT EXPENTRY CfgCommonPageProc( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 )
{
    static PUCFGDATA pConfig;

    switch ( msg ) {

        case WM_INITDLG:
            pConfig = (PUCFGDATA) mp2;
            if ( !pConfig ) return (MRESULT) TRUE;
            CfgPopulateClockList( hwnd, pConfig );

            if ( pConfig->fsStyle & APP_STYLE_TITLEBAR )
                WinSendDlgItemMsg( hwnd, IDD_TITLEBAR, BM_SETCHECK, MPFROMSHORT(1), MPVOID );
            if ( pConfig->fsStyle & APP_STYLE_WTBORDERS )
                WinSendDlgItemMsg( hwnd, IDD_BORDERS, BM_SETCHECK, MPFROMSHORT(1), MPVOID );

            WinSendDlgItemMsg( hwnd, IDD_VIEWSWITCH, SPBM_SETLIMITS, MPFROMLONG(500), MPFROMLONG(0) );
            WinSendDlgItemMsg( hwnd, IDD_VIEWSWITCH, SPBM_SETCURRENTVALUE,
                               MPFROMLONG(pConfig->usCompactThreshold), MPVOID );
            WinSendDlgItemMsg( hwnd, IDD_DESCWIDTH, SPBM_SETLIMITS, MPFROMLONG(100), MPFROMLONG(0) );
            WinSendDlgItemMsg( hwnd, IDD_DESCWIDTH, SPBM_SETCURRENTVALUE,
                               MPFROMLONG(pConfig->bDescWidth), MPVOID );
            WinSendDlgItemMsg( hwnd, IDD_PERCOLUMN, SPBM_SETLIMITS, MPFROMLONG(12), MPFROMLONG(0) );
            WinSendDlgItemMsg( hwnd, IDD_PERCOLUMN, SPBM_SETCURRENTVALUE,
                               MPFROMLONG(pConfig->usPerColumn), MPVOID );

            break;


        case WM_COMMAND:
            switch( SHORT1FROMMP( mp1 )) {

                // common buttons
                case DID_OK:
                    WinPostMsg( WinQueryWindow(hwnd, QW_OWNER), WM_COMMAND, mp1, mp2 );
                    break;


                case DID_CANCEL:
                    WinPostMsg( WinQueryWindow(hwnd, QW_OWNER), WM_COMMAND, mp1, mp2 );
                    break;

                default: break;
            }
            // end of WM_COMMAND messages
            return (MRESULT) 0;


        default: break;
    }

    return WinDefDlgProc( hwnd, msg, mp1, mp2 );
}


/* ------------------------------------------------------------------------- *
 * CfgPopulateClockList                                                      *
 *                                                                           *
 * Fill the list of currently-configured clocks.                             *
 * ------------------------------------------------------------------------- */
void CfgPopulateClockList( HWND hwnd, PUCFGDATA pConfig )
{
    USHORT i;
    UCHAR  szDesc[ LOCDESC_MAXZ ];
    PSZ    psz;

    /* This is a temporary (and crude) stopover until I can actually implement
     * a proper city/timezone database with descriptions.
     */

    for ( i = 0; i < pConfig->usClocks; i++ ) {
        //sprintf( szDesc, "Clock %u (", i+1 );
        UniStrFromUcs( pConfig->uconv, szDesc, pConfig->aClockData[i].uzDesc, LOCDESC_MAXZ );
        strncat( szDesc, " (", LOCDESC_MAXZ );
        strncat( szDesc, pConfig->aClockData[i].szTZ, LOCDESC_MAXZ );
        psz = strchr( szDesc, ',');
        if ( psz ) {
            *psz = '\0';
            strncat( szDesc, "...", LOCDESC_MAXZ );
        }
        strncat( szDesc, ")", LOCDESC_MAXZ );
        WinSendDlgItemMsg( hwnd, IDD_CLOCKLIST, LM_INSERTITEM,
                           MPFROMSHORT(LIT_END), MPFROMP(szDesc) );
    }

}


/* ------------------------------------------------------------------------- *
 * CfgSettingsCommon                                                         *
 *                                                                           *
 * Get configuration changes on the common settings page.                    *
 * ------------------------------------------------------------------------- */
void CfgSettingsCommon( HWND hwnd, PUCFGDATA pConfig )
{
    ULONG ul;

    if ( (USHORT) WinSendDlgItemMsg( hwnd, IDD_TITLEBAR, BM_QUERYCHECK, 0L, 0L ) == 1 )
        pConfig->fsStyle |= APP_STYLE_TITLEBAR;
    else
        pConfig->fsStyle &= ~APP_STYLE_TITLEBAR;
    if ( (USHORT) WinSendDlgItemMsg( hwnd, IDD_BORDERS, BM_QUERYCHECK, 0L, 0L ) == 1 )
        pConfig->fsStyle |= APP_STYLE_WTBORDERS;
    else
        pConfig->fsStyle &= ~APP_STYLE_WTBORDERS;

    if ( WinSendDlgItemMsg( hwnd, IDD_VIEWSWITCH, SPBM_QUERYVALUE,
                            MPFROMP(&ul), MPFROM2SHORT(SPBQ_UPDATEIFVALID, 0)))
        pConfig->usCompactThreshold = (USHORT) ul;
    if ( WinSendDlgItemMsg( hwnd, IDD_DESCWIDTH, SPBM_QUERYVALUE,
                            MPFROMP(&ul), MPFROM2SHORT(SPBQ_UPDATEIFVALID, 0)))
        pConfig->bDescWidth = (BYTE) ul;
    if ( WinSendDlgItemMsg( hwnd, IDD_PERCOLUMN, SPBM_QUERYVALUE,
                            MPFROMP(&ul), MPFROM2SHORT(SPBQ_UPDATEIFVALID, 0)))
        pConfig->usPerColumn = (USHORT) ul;
}


/* ------------------------------------------------------------------------- *
 * ClockNotebook                                                             *
 *                                                                           *
 * Initialize and open the clock-panel configuration notebook dialog.        *
 *                                                                           *
 * ARGUMENTS:                                                                *
 *     HWND hwnd      : HWND of the application client.                      *
 *     USHORT usNumber: Number of the clock being configured.                *
 *                                                                           *
 * RETURNS: BOOL                                                             *
 *     TRUE if the configuration was changed; FALSE if it was not changed,   *
 *     or if an error occurred.                                              *
 * ------------------------------------------------------------------------- */
BOOL ClockNotebook( HWND hwnd, USHORT usNumber )
{
    UCLKPROP   props;
    PUCLGLOBAL pGlobal;
    PSZ        psz;
    ULONG      rc,
               ulid,
               flBorders,
               clr;
    UCHAR      szFont[ FACESIZE + 4 ];
    MPARAM     mp1, mp2;

    pGlobal = WinQueryWindowPtr( hwnd, 0 );

    memset( &props, 0, sizeof(props) );
    props.hab = pGlobal->hab;
    props.hmq = pGlobal->hmq;
    props.usClock = usNumber;
    props.bChanged = FALSE;

    rc = UniCreateUconvObject( (UniChar *) L"@map=display,path=no", &(props.uconv) );
    if ( rc != ULS_SUCCESS) props.uconv = NULL;

    // Query the settings of the current clock
    if ( ! pGlobal->clocks[usNumber] ) return FALSE;

    // colours & fonts
    if ( WinQueryPresParam( pGlobal->clocks[usNumber], PP_FOREGROUNDCOLOR,
                            PP_FOREGROUNDCOLORINDEX, &ulid,
                            sizeof(clr), &clr, QPF_ID2COLORINDEX ))
        props.clockStyle.clrFG = clr;
    else
        props.clockStyle.clrFG = NO_COLOUR_PP;

    if ( WinQueryPresParam( pGlobal->clocks[usNumber], PP_BACKGROUNDCOLOR,
                            PP_BACKGROUNDCOLORINDEX, &ulid,
                            sizeof(clr), &clr, QPF_ID2COLORINDEX ))
        props.clockStyle.clrBG = clr;
    else
        props.clockStyle.clrBG = NO_COLOUR_PP;

    if ( WinQueryPresParam( pGlobal->clocks[usNumber], PP_BORDERCOLOR,
                            0, &ulid, sizeof(clr), &clr, 0 ))
        props.clockStyle.clrBor = clr;
    else
        props.clockStyle.clrBor = NO_COLOUR_PP;

    if ( WinQueryPresParam( pGlobal->clocks[usNumber], PP_SEPARATORCOLOR,
                            0, &ulid, sizeof(clr), &clr, QPF_NOINHERIT ))
        props.clockStyle.clrSep = clr;
    else
        props.clockStyle.clrSep = NO_COLOUR_PP;

    if ( WinQueryPresParam( pGlobal->clocks[usNumber], PP_FONTNAMESIZE, 0,
                            NULL, sizeof(szFont), szFont, QPF_NOINHERIT ))
    {
        psz = strchr( szFont, '.') + 1;
        strncpy( props.clockStyle.szFont, psz, FACESIZE );
    }

    // configurable settings
    props.clockData.cb = sizeof(WTDCDATA);
    WinSendMsg( pGlobal->clocks[usNumber], WTD_QUERYCONFIG, MPFROMP(&(props.clockData)), MPVOID );

    flBorders = props.clockData.flOptions & 0xF0;   // preserve the current border settings

    // show the configuration dialog
    WinDlgBox( HWND_DESKTOP, hwnd, (PFNWP) ClkDialogProc, 0, IDD_CONFIG, &props );

    // dialog has closed, now update the settings
    if ( props.bChanged ) {

        // configuration settings

        WinSendMsg( pGlobal->clocks[usNumber], WTD_SETTIMEZONE,
                    MPFROMP(props.clockData.szTZ), MPFROMP(props.clockData.uzDesc));
        WinSendMsg( pGlobal->clocks[usNumber], WTD_SETOPTIONS,
                    MPFROMLONG(props.clockData.flOptions | flBorders), MPVOID );
        WinSendMsg( pGlobal->clocks[usNumber], WTD_SETLOCALE,
                    MPFROMP(props.clockData.szLocale), MPVOID );

        mp1 = ( props.clockData.flOptions & WTF_TIME_CUSTOM ) ?
              MPFROMP( props.clockData.uzTimeFmt ) : MPVOID;
        mp2 = ( props.clockData.flOptions & WTF_DATE_CUSTOM ) ?
              MPFROMP( props.clockData.uzDateFmt ) : MPVOID;
        if ( mp1 || mp2 )
            WinSendMsg( pGlobal->clocks[usNumber], WTD_SETFORMATS, mp1, mp2 );

        // appearance settings

        if ( props.clockStyle.clrFG != NO_COLOUR_PP )
            WinSetPresParam( pGlobal->clocks[usNumber], PP_FOREGROUNDCOLOR, sizeof(ULONG), &(props.clockStyle.clrFG) );
        if ( props.clockStyle.clrBG != NO_COLOUR_PP )
            WinSetPresParam( pGlobal->clocks[usNumber], PP_BACKGROUNDCOLOR, sizeof(ULONG), &(props.clockStyle.clrBG) );
        if ( props.clockStyle.clrBor != NO_COLOUR_PP )
            WinSetPresParam( pGlobal->clocks[usNumber], PP_BORDERCOLOR, sizeof(ULONG), &(props.clockStyle.clrBor) );
        if ( props.clockStyle.clrSep != NO_COLOUR_PP )
            WinSetPresParam( pGlobal->clocks[usNumber], PP_SEPARATORCOLOR, sizeof(ULONG), &(props.clockStyle.clrSep) );
        if ( props.clockStyle.szFont[0] ) {
            sprintf( szFont, "10.%s", props.clockStyle.szFont );
            WinSetPresParam( pGlobal->clocks[usNumber], PP_FONTNAMESIZE, strlen(szFont)+1, szFont );
        }

    }

    UniFreeUconvObject( props.uconv );
    return ( props.bChanged );
}


/* ------------------------------------------------------------------------- *
 * ClkDialogProc                                                             *
 *                                                                           *
 * Dialog procedure for the clock properties dialog.                         *
 * ------------------------------------------------------------------------- */
MRESULT EXPENTRY ClkDialogProc( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 )
{
    static PUCLKPROP pConfig;
    HPOINTER  hicon;
    HWND      hwndPage;
    UCHAR     szTitle[ SZRES_MAXZ ];

    switch ( msg ) {

        case WM_INITDLG:
            hicon = WinLoadPointer( HWND_DESKTOP, 0, ID_MAINPROGRAM );
            WinSendMsg( hwnd, WM_SETICON, (MPARAM) hicon, MPVOID );

            pConfig = (PUCLKPROP) mp2;

            sprintf( szTitle, "Clock %d - Properties", pConfig->usClock+1 );     // TODO NLS
            WinSetWindowText( hwnd, szTitle );

            if ( ! ClkPopulateNotebook( hwnd, pConfig ) ) {
                ErrorPopup("Error populating notebook.");   // TODO NLS
                return (MRESULT) TRUE;
            }
            CentreWindow( hwnd );
            return (MRESULT) FALSE;


        case WM_COMMAND:
            switch( SHORT1FROMMP( mp1 )) {

                case DID_OK:
                    hwndPage = (HWND) WinSendDlgItemMsg( hwnd, IDD_CFGBOOK, BKM_QUERYPAGEWINDOWHWND, MPFROMLONG(pConfig->ulPage1), 0 );
                    if ( hwndPage && hwndPage != BOOKERR_INVALID_PARAMETERS )
                        ClkSettingsClock( hwndPage, pConfig );
                    hwndPage = (HWND) WinSendDlgItemMsg( hwnd, IDD_CFGBOOK, BKM_QUERYPAGEWINDOWHWND, MPFROMLONG(pConfig->ulPage2), 0 );
                    if ( hwndPage && hwndPage != BOOKERR_INVALID_PARAMETERS )
                        ClkSettingsStyle( hwndPage, pConfig );
                    pConfig->bChanged = TRUE;
                    break;

                default: break;
            } // end WM_COMMAND messages
            break;


        case WM_CONTROL:
            switch( SHORT1FROMMP( mp1 )) {
            } // end WM_CONTROL messages
            break;


        default: break;
    }

    return WinDefDlgProc( hwnd, msg, mp1, mp2 );
}


/* ------------------------------------------------------------------------- *
 * ClkPopulateNotebook()                                                     *
 *                                                                           *
 * Creates the pages (dialogs) of the clock properties notebook.             *
 *                                                                           *
 * ARGUMENTS:                                                                *
 *     HWND hwnd         : HWND of the properties dialog.                    *
 *     PUCLKPROP pConfig: Pointer to clock properties data.                  *
 *                                                                           *
 * RETURNS: BOOL                                                             *
 *     FALSE if an error occurred.  TRUE otherwise.                          *
 * ------------------------------------------------------------------------- */
BOOL ClkPopulateNotebook( HWND hwnd, PUCLKPROP pConfig )
{
    ULONG          pageId;
    HWND           hwndPage,
                   hwndBook;
    NOTEBOOKBUTTON bookButtons[ 3 ];
    USHORT         fsCommon = BKA_AUTOPAGESIZE | BKA_STATUSTEXTON;
    CHAR           szOK[ SZRES_MAXZ ],
                   szCancel[ SZRES_MAXZ ],
                   szHelp[ SZRES_MAXZ ];


    hwndBook = WinWindowFromID( hwnd, IDD_CFGBOOK );
    if ( ! WinLoadString( pConfig->hab, 0, IDS_BTN_OK, SZRES_MAXZ-1, szOK ))
        sprintf( szOK, "OK");
    if ( ! WinLoadString( pConfig->hab, 0, IDS_BTN_CANCEL, SZRES_MAXZ-1, szCancel ))
        sprintf( szOK, "Cancel");
    if ( ! WinLoadString( pConfig->hab, 0, IDS_BTN_HELP, SZRES_MAXZ-1, szHelp ))
        sprintf( szOK, "~Help");

    // create common buttons
    bookButtons[0].pszText  = szOK;
    bookButtons[0].idButton = DID_OK;
    bookButtons[0].flStyle  = BS_AUTOSIZE | BS_DEFAULT;
    bookButtons[1].pszText  = szCancel;
    bookButtons[1].idButton = DID_CANCEL;
    bookButtons[1].flStyle  = BS_AUTOSIZE;
    bookButtons[2].pszText  = szHelp;
    bookButtons[2].idButton = DID_HELP;
    bookButtons[2].flStyle  = BS_AUTOSIZE | BS_HELP;
    WinSendMsg( hwndBook, BKM_SETNOTEBOOKBUTTONS, MPFROMLONG( 3 ), MPFROMP( bookButtons ));

    // this should make the notebook tabs usable if the program is run on Warp 3
    WinSendMsg( hwndBook, BKM_SETDIMENSIONS, MPFROM2SHORT( 75, 22 ), MPFROMSHORT( BKA_MAJORTAB ));

    // add the clock configuration pages
    hwndPage = WinLoadDlg( hwndBook, hwndBook, (PFNWP) ClkClockPageProc, 0, IDD_CFGCLOCKS, pConfig );
    pageId = (ULONG) WinSendMsg( hwndBook, BKM_INSERTPAGE, NULL,
                                 MPFROM2SHORT( fsCommon | BKA_MAJOR, BKA_FIRST ));
    if ( !pageId ) return FALSE;
    if ( !WinSendMsg( hwndBook, BKM_SETTABTEXT, MPFROMLONG(pageId), MPFROMP("~Clock Setup")))      // TODO NLS
        return FALSE;
    if ( !WinSendMsg( hwndBook, BKM_SETPAGEWINDOWHWND,
                      MPFROMLONG( pageId ), MPFROMP( hwndPage )))
        return FALSE;
    pConfig->ulPage1 = pageId;

    hwndPage = WinLoadDlg( hwndBook, hwndBook, (PFNWP) ClkStylePageProc, 0, IDD_CFGPRES, pConfig );
    pageId = (ULONG) WinSendMsg( hwndBook, BKM_INSERTPAGE, NULL,
                                 MPFROM2SHORT( fsCommon | BKA_MAJOR, BKA_LAST ));
    if ( !pageId ) return FALSE;
    if ( !WinSendMsg( hwndBook, BKM_SETTABTEXT, MPFROMLONG(pageId), MPFROMP("~Appearance")))      // TODO NLS
        return FALSE;
    if ( !WinSendMsg( hwndBook, BKM_SETPAGEWINDOWHWND,
                      MPFROMLONG( pageId ), MPFROMP( hwndPage )))
        return FALSE;
    pConfig->ulPage2 = pageId;

    return TRUE;
}


/* ------------------------------------------------------------------------- *
 * ClkClockPageProc                                                          *
 *                                                                           *
 * Dialog procedure for the clock setup configuration page.                  *
 * ------------------------------------------------------------------------- */
MRESULT EXPENTRY ClkClockPageProc( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 )
{
    static PUCLKPROP pConfig;
    UniChar *puzLocales,
            *puz;
    HWND    hwndLB;
    SHORT   sIdx;
    USHORT  offset;
    UCHAR   szLocale[ ULS_LNAMEMAX + 1 ],
            szFormat[ STRFT_MAXZ ] = {0},
            szName[ LOCDESC_MAXZ ] = {0},
            szSysDef[ SZRES_MAXZ ],
            szLocDef[ SZRES_MAXZ ],
            szCustom[ SZRES_MAXZ ];

    switch ( msg ) {

        case WM_INITDLG:
            pConfig = (PUCLKPROP) mp2;
            if ( !pConfig ) return (MRESULT) TRUE;

            puzLocales = (UniChar *) calloc( LOCALE_BUF_MAX, sizeof(UniChar) );
            if ( puzLocales ) {
                if ( UniQueryLocaleList( UNI_USER_LOCALES | UNI_SYSTEM_LOCALES,
                                         puzLocales, LOCALE_BUF_MAX ) == ULS_SUCCESS )
                {
                    offset = 0;
                    while ( (offset < LOCALE_BUF_MAX) && (*(puzLocales+offset)) ) {
                        puz = puzLocales + offset;
                        sprintf( szLocale, "%ls", puz );
                        WinSendDlgItemMsg( hwnd, IDD_LOCALES, LM_INSERTITEM,
                                           MPFROMSHORT(LIT_SORTASCENDING), MPFROMP(szLocale) );
                        offset += UniStrlen( puz ) + 1;
                    }
                }
                free( puzLocales );
            }

            if ( ! WinLoadString( pConfig->hab, 0, IDS_FORMAT_SYSDEF, SZRES_MAXZ-1, szSysDef ))
                sprintf( szSysDef, "System default");
            if ( ! WinLoadString( pConfig->hab, 0, IDS_FORMAT_LOCDEF, SZRES_MAXZ-1, szLocDef ))
                sprintf( szLocDef, "Locale default");
            if ( ! WinLoadString( pConfig->hab, 0, IDS_FORMAT_CUSTOM, SZRES_MAXZ-1, szCustom ))
                sprintf( szCustom, "Custom string");
            WinSendDlgItemMsg( hwnd, IDD_TIMEFMT,  LM_INSERTITEM, MPFROMSHORT(LIT_END), MPFROMP(szSysDef) );
            WinSendDlgItemMsg( hwnd, IDD_TIMEFMT,  LM_INSERTITEM, MPFROMSHORT(LIT_END), MPFROMP(szLocDef) );
            WinSendDlgItemMsg( hwnd, IDD_TIMEFMT,  LM_INSERTITEM, MPFROMSHORT(LIT_END), MPFROMP(szCustom) );
            WinSendDlgItemMsg( hwnd, IDD_TIMEFMT2, LM_INSERTITEM, MPFROMSHORT(LIT_END), MPFROMP(szSysDef) );
            WinSendDlgItemMsg( hwnd, IDD_TIMEFMT2, LM_INSERTITEM, MPFROMSHORT(LIT_END), MPFROMP(szLocDef) );
            WinSendDlgItemMsg( hwnd, IDD_TIMEFMT2, LM_INSERTITEM, MPFROMSHORT(LIT_END), MPFROMP(szCustom) );
            WinSendDlgItemMsg( hwnd, IDD_DATEFMT,  LM_INSERTITEM, MPFROMSHORT(LIT_END), MPFROMP(szSysDef) );
            WinSendDlgItemMsg( hwnd, IDD_DATEFMT,  LM_INSERTITEM, MPFROMSHORT(LIT_END), MPFROMP(szLocDef) );
            WinSendDlgItemMsg( hwnd, IDD_DATEFMT,  LM_INSERTITEM, MPFROMSHORT(LIT_END), MPFROMP(szCustom) );
            WinSendDlgItemMsg( hwnd, IDD_DATEFMT2, LM_INSERTITEM, MPFROMSHORT(LIT_END), MPFROMP(szSysDef) );
            WinSendDlgItemMsg( hwnd, IDD_DATEFMT2, LM_INSERTITEM, MPFROMSHORT(LIT_END), MPFROMP(szLocDef) );
            WinSendDlgItemMsg( hwnd, IDD_DATEFMT2, LM_INSERTITEM, MPFROMSHORT(LIT_END), MPFROMP(szCustom) );

            // Select the current values
            WinSetDlgItemText( hwnd, IDD_TZDISPLAY, pConfig->clockData.szTZ );

            // description field
            // (temporary until we can implement a proper city/location database)
            if ( pConfig->uconv &&
                ( UniStrFromUcs( pConfig->uconv, szName,
                                 pConfig->clockData.uzDesc, LOCDESC_MAXZ ) == ULS_SUCCESS ))
                WinSetDlgItemText( hwnd, IDD_CITYLIST, szName );

            // formatting locale
            sIdx = (SHORT) WinSendDlgItemMsg( hwnd, IDD_LOCALES, LM_SEARCHSTRING,
                                              MPFROM2SHORT(LSS_CASESENSITIVE,LIT_FIRST),
                                              MPFROMP(pConfig->clockData.szLocale) );
            if ( sIdx != LIT_ERROR )
                WinSendDlgItemMsg( hwnd, IDD_LOCALES, LM_SELECTITEM,
                                   MPFROMSHORT(sIdx), MPFROMSHORT(TRUE) );

            // time format controls (primary)
            if ( pConfig->uconv &&
                ( UniStrFromUcs( pConfig->uconv, szFormat,
                                 pConfig->clockData.uzTimeFmt, STRFT_MAXZ ) == ULS_SUCCESS ))
                WinSetDlgItemText( hwnd, IDD_TIMESTR, szFormat );

            if ( pConfig->clockData.flOptions & WTF_TIME_SYSTEM )
                sIdx = 0;
            else if ( pConfig->clockData.flOptions & WTF_TIME_CUSTOM )
                sIdx = 2;
            else sIdx = 1;
            WinSendDlgItemMsg( hwnd, IDD_TIMEFMT, LM_SELECTITEM,
                               MPFROMSHORT(sIdx), MPFROMSHORT(TRUE) );

            if ( sIdx < 2 && pConfig->clockData.flOptions & WTF_TIME_SHORT )
                WinSendDlgItemMsg( hwnd, IDD_TIMESHORT, BM_SETCHECK, MPFROMSHORT(1), MPVOID );

            // time format controls (alternate)
            if ( pConfig->clockData.flOptions & WTF_ATIME_SYSTEM )
                sIdx = 0;
            else if ( pConfig->clockData.flOptions & WTF_ATIME_CUSTOM )
                sIdx = 2;
            else sIdx = 1;
            WinSendDlgItemMsg( hwnd, IDD_TIMEFMT2, LM_SELECTITEM,
                               MPFROMSHORT(sIdx), MPFROMSHORT(TRUE) );

            if ( sIdx < 2 && pConfig->clockData.flOptions & WTF_ATIME_SHORT )
                WinSendDlgItemMsg( hwnd, IDD_TIMESHORT2, BM_SETCHECK, MPFROMSHORT(1), MPVOID );

            // date format controls (primary)
            if ( pConfig->uconv &&
                 ( UniStrFromUcs( pConfig->uconv, szFormat,
                                  pConfig->clockData.uzDateFmt, STRFT_MAXZ ) == ULS_SUCCESS ))
                WinSetDlgItemText( hwnd, IDD_DATESTR, szFormat );

            if ( pConfig->clockData.flOptions & WTF_DATE_SYSTEM )
                sIdx = 0;
            else if ( pConfig->clockData.flOptions & WTF_DATE_CUSTOM )
                sIdx = 2;
            else sIdx = 1;
            WinSendDlgItemMsg( hwnd, IDD_DATEFMT, LM_SELECTITEM,
                               MPFROMSHORT(sIdx), MPFROMSHORT(TRUE) );

            // date format controls (secondary)
            if ( pConfig->clockData.flOptions & WTF_LONGDATE_SYSTEM )
                sIdx = 0;
            else if ( pConfig->clockData.flOptions & WTF_LONGDATE_CUSTOM )
                sIdx = 2;
            else sIdx = 1;
            WinSendDlgItemMsg( hwnd, IDD_DATEFMT2, LM_SELECTITEM,
                               MPFROMSHORT(sIdx), MPFROMSHORT(TRUE) );

            return (MRESULT) FALSE;


        case WM_COMMAND:
            switch( SHORT1FROMMP( mp1 )) {

                // common buttons
                case DID_OK:
                    WinPostMsg( WinQueryWindow(hwnd, QW_OWNER), WM_COMMAND, mp1, mp2 );
                    break;

                case DID_CANCEL:
                    WinPostMsg( WinQueryWindow(hwnd, QW_OWNER), WM_COMMAND, mp1, mp2 );
                    break;

                default: break;
            }
            // end of WM_COMMAND messages
            return (MRESULT) 0;


        case WM_CONTROL:
            switch( SHORT1FROMMP( mp1 )) {

                case IDD_TIMEFMT:
                    if ( SHORT2FROMMP(mp1) == LN_SELECT ) {
                        hwndLB = (HWND) mp2;
                        sIdx = (SHORT) WinSendMsg( hwndLB, LM_QUERYSELECTION, MPFROMSHORT(LIT_FIRST), 0 );
                        WinShowWindow( WinWindowFromID(hwnd, IDD_TIMESHORT), (sIdx < 2)? TRUE: FALSE );
                        WinShowWindow( WinWindowFromID(hwnd, IDD_TIMESTR), (sIdx == 2)? TRUE: FALSE );
                    }
                    break;

                case IDD_TIMEFMT2:
                    if ( SHORT2FROMMP(mp1) == LN_SELECT ) {
                        hwndLB = (HWND) mp2;
                        sIdx = (SHORT) WinSendMsg( hwndLB, LM_QUERYSELECTION, MPFROMSHORT(LIT_FIRST), 0 );
                        WinShowWindow( WinWindowFromID(hwnd, IDD_TIMESHORT2), (sIdx < 2)? TRUE: FALSE );
                        WinShowWindow( WinWindowFromID(hwnd, IDD_TIMESTR2), (sIdx == 2)? TRUE: FALSE );
                    }
                    break;

                case IDD_DATEFMT:
                    if ( SHORT2FROMMP(mp1) == LN_SELECT ) {
                        hwndLB = (HWND) mp2;
                        sIdx = (SHORT) WinSendMsg( hwndLB, LM_QUERYSELECTION, MPFROMSHORT(LIT_FIRST), 0 );
                        WinShowWindow( WinWindowFromID(hwnd, IDD_DATESTR), (sIdx == 2)? TRUE: FALSE );
                    }
                    break;

                case IDD_DATEFMT2:
                    if ( SHORT2FROMMP(mp1) == LN_SELECT ) {
                        hwndLB = (HWND) mp2;
                        sIdx = (SHORT) WinSendMsg( hwndLB, LM_QUERYSELECTION, MPFROMSHORT(LIT_FIRST), 0 );
                        WinShowWindow( WinWindowFromID(hwnd, IDD_DATESTR2), (sIdx == 2)? TRUE: FALSE );
                    }
                    break;

            } // end WM_CONTROL messages
            break;


        default: break;
    }

    return WinDefDlgProc( hwnd, msg, mp1, mp2 );
}


/* ------------------------------------------------------------------------- *
 * ClkStylePageProc                                                          *
 *                                                                           *
 * Dialog procedure for the clock panels appearance & pres.params page.      *
 * ------------------------------------------------------------------------- */
MRESULT EXPENTRY ClkStylePageProc( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 )
{
    static PUCLKPROP pConfig;
    UCHAR szFontPP[ FACESIZE+4 ];
    ULONG clr;

    switch ( msg ) {

        case WM_INITDLG:
            pConfig = (PUCLKPROP) mp2;
            if ( !pConfig ) return (MRESULT) TRUE;

            clr = pConfig->clockStyle.clrBG;
            if ( clr == NO_COLOUR_PP ) {
                clr = SYSCLR_WINDOW;
                WinSetPresParam( WinWindowFromID(hwnd, IDD_BACKGROUND),
                                 PP_BACKGROUNDCOLORINDEX, sizeof(ULONG), &clr );
            } else
                WinSetPresParam( WinWindowFromID(hwnd, IDD_BACKGROUND),
                                 PP_BACKGROUNDCOLOR, sizeof(ULONG), &clr );
            clr = pConfig->clockStyle.clrFG;
            if ( clr == NO_COLOUR_PP ) {
                clr = SYSCLR_WINDOWTEXT;
                WinSetPresParam( WinWindowFromID(hwnd, IDD_FOREGROUND),
                                 PP_BACKGROUNDCOLORINDEX, sizeof(ULONG), &clr );
            } else
                WinSetPresParam( WinWindowFromID(hwnd, IDD_FOREGROUND),
                                 PP_BACKGROUNDCOLOR, sizeof(ULONG), &clr );
            clr = pConfig->clockStyle.clrBor;
            if ( clr == NO_COLOUR_PP ) {
                clr = SYSCLR_WINDOWTEXT;
                WinSetPresParam( WinWindowFromID(hwnd, IDD_BORDER),
                                 PP_BACKGROUNDCOLORINDEX, sizeof(ULONG), &clr );
            } else
                WinSetPresParam( WinWindowFromID(hwnd, IDD_BORDER),
                                 PP_BACKGROUNDCOLOR, sizeof(ULONG), &clr );
            clr = pConfig->clockStyle.clrSep;
            if ( clr == NO_COLOUR_PP ) {
                clr = SYSCLR_WINDOWTEXT;
                WinSetPresParam( WinWindowFromID(hwnd, IDD_SEPARATOR),
                                 PP_BACKGROUNDCOLORINDEX, sizeof(ULONG), &clr );
            } else
                WinSetPresParam( WinWindowFromID(hwnd, IDD_SEPARATOR),
                                 PP_BACKGROUNDCOLOR, sizeof(ULONG), &clr );

            if ( pConfig->clockStyle.szFont[0] ) {
                if ( !strncmp( pConfig->clockStyle.szFont, "WarpSans", 8 ))
                    sprintf( szFontPP, "9.%s", pConfig->clockStyle.szFont );
                else
                    sprintf( szFontPP, "10.%s", pConfig->clockStyle.szFont );
                WinSetPresParam( WinWindowFromID(hwnd, IDD_DISPLAYFONT),
                                 PP_FONTNAMESIZE, strlen(szFontPP)+1, szFontPP );
            }
            return (MRESULT) FALSE;


        case WM_COMMAND:
            switch( SHORT1FROMMP( mp1 )) {

                // common buttons
                case DID_OK:
                    WinPostMsg( WinQueryWindow(hwnd, QW_OWNER), WM_COMMAND, mp1, mp2 );
                    break;

                case DID_CANCEL:
                    WinPostMsg( WinQueryWindow(hwnd, QW_OWNER), WM_COMMAND, mp1, mp2 );
                    break;

                default: break;
            }
            // end of WM_COMMAND messages
            return (MRESULT) 0;

        default: break;
    }

    return WinDefDlgProc( hwnd, msg, mp1, mp2 );
}


/* ------------------------------------------------------------------------- *
 * ClkSettingsClock                                                          *
 *                                                                           *
 * Get configuration changes on the clock properties page.                   *
 * ------------------------------------------------------------------------- */
BOOL ClkSettingsClock( HWND hwnd, PUCLKPROP pConfig )
{
    WTDCDATA settings;                  // new settings
    SHORT    sIdx;                      // listbox index
    UCHAR    szDesc[ LOCDESC_MAXZ ]  = {0},
             szTimeFmt[ STRFT_MAXZ ] = {0},
             szDateFmt[ STRFT_MAXZ ] = {0};


    memset( &settings, 0, sizeof(WTDCDATA) );
    settings.cb = sizeof( WTDCDATA );

    // options mask

    sIdx = (SHORT) WinSendDlgItemMsg( hwnd, IDD_TIMEFMT, LM_QUERYSELECTION,
                                      MPFROMSHORT(LIT_FIRST), 0 );
    switch ( sIdx ) {
        case 0:  settings.flOptions |= WTF_TIME_SYSTEM; break;
//        case 2:  settings.flOptions |= WTF_TIME_ALT;    break;
        case 2:  settings.flOptions |= WTF_TIME_CUSTOM; break;
        default: break;
    }
    if ( sIdx < 2 && WinQueryButtonCheckstate( hwnd, IDD_TIMESHORT ))
        settings.flOptions |= WTF_TIME_SHORT;

    sIdx = (SHORT) WinSendDlgItemMsg( hwnd, IDD_DATEFMT, LM_QUERYSELECTION,
                                      MPFROMSHORT(LIT_FIRST), 0 );
    switch ( sIdx ) {
        case 0:  settings.flOptions |= WTF_DATE_SYSTEM; break;
//        case 2:  settings.flOptions |= WTF_DATE_ALT;    break;
        case 2:  settings.flOptions |= WTF_DATE_CUSTOM; break;
        default: break;
    }

    // save the current view mode flag
    settings.flOptions |= ( pConfig->clockData.flOptions & 0xF );

    // (the program will determine the border settings automatically;
    // other option flags haven't been implemented yet)


    // timezone string
    WinQueryDlgItemText( hwnd, IDD_TZDISPLAY, TZSTR_MAXZ, settings.szTZ );

    // description
    WinQueryDlgItemText( hwnd, IDD_CITYLIST, LOCDESC_MAXZ, szDesc );
    UniStrToUcs( pConfig->uconv, settings.uzDesc, szDesc, LOCDESC_MAXZ );

    // formatting locale
    sIdx = (SHORT) WinSendDlgItemMsg( hwnd, IDD_LOCALES, LM_QUERYSELECTION,
                                      MPFROMSHORT(LIT_FIRST), 0 );
    if ( sIdx != LIT_NONE )
        WinSendDlgItemMsg( hwnd, IDD_LOCALES, LM_QUERYITEMTEXT,
                           MPFROM2SHORT(sIdx, ULS_LNAMEMAX + 1), settings.szLocale );

    // custom format strings, if applicable

    if ( settings.flOptions & WTF_TIME_CUSTOM ) {
        WinQueryDlgItemText( hwnd, IDD_TIMESTR, STRFT_MAXZ, szTimeFmt );
        UniStrToUcs( pConfig->uconv, settings.uzTimeFmt, szTimeFmt, STRFT_MAXZ );
    }
    else UniStrcpy( settings.uzTimeFmt, pConfig->clockData.uzTimeFmt );

    if ( settings.flOptions & WTF_DATE_CUSTOM ) {
        WinQueryDlgItemText( hwnd, IDD_DATESTR, STRFT_MAXZ, szDateFmt );
        UniStrToUcs( pConfig->uconv, settings.uzDateFmt, szDateFmt, STRFT_MAXZ );
    }
    else UniStrcpy( settings.uzDateFmt, pConfig->clockData.uzDateFmt );

    memcpy( &(pConfig->clockData), &settings, sizeof(WTDCDATA) );
    return TRUE;
}


/* ------------------------------------------------------------------------- *
 * ClkSettingsStyle                                                          *
 *                                                                           *
 * Get configuration changes on the clock style page.                        *
 * ------------------------------------------------------------------------- */
BOOL ClkSettingsStyle( HWND hwnd, PUCLKPROP pConfig )
{
    CLKSTYLE styles;                 // new presentation parameters
    ULONG    ulid,
             clr;
    UCHAR    szFont[ FACESIZE+4 ];
    PSZ      psz;

    memset( &styles, 0, sizeof(CLKSTYLE) );

    // colour presentation parameters
    if ( WinQueryPresParam( WinWindowFromID(hwnd, IDD_BACKGROUND),
                            PP_BACKGROUNDCOLOR, PP_BACKGROUNDCOLORINDEX,
                            &ulid, sizeof(clr), &clr, QPF_ID2COLORINDEX ))
        styles.clrBG = clr;
    else
        styles.clrBG = NO_COLOUR_PP;

    if ( WinQueryPresParam( WinWindowFromID(hwnd, IDD_FOREGROUND),
                            PP_BACKGROUNDCOLOR, PP_BACKGROUNDCOLORINDEX,
                            &ulid, sizeof(clr), &clr, QPF_ID2COLORINDEX ))
        styles.clrFG = clr;
    else
        styles.clrFG = NO_COLOUR_PP;

    if ( WinQueryPresParam( WinWindowFromID(hwnd, IDD_BORDER),
                            PP_BACKGROUNDCOLOR, PP_BACKGROUNDCOLORINDEX,
                            &ulid, sizeof(clr), &clr, QPF_ID2COLORINDEX ))
        styles.clrBor = clr;
    else
        styles.clrBor = NO_COLOUR_PP;

    if ( WinQueryPresParam( WinWindowFromID(hwnd, IDD_SEPARATOR),
                            PP_BACKGROUNDCOLOR, PP_BACKGROUNDCOLORINDEX,
                            &ulid, sizeof(clr), &clr, QPF_ID2COLORINDEX ))
        styles.clrSep = clr;
    else
        styles.clrSep = NO_COLOUR_PP;

    if ( WinQueryPresParam( WinWindowFromID(hwnd, IDD_DISPLAYFONT),
                            PP_FONTNAMESIZE, 0, NULL,
                            sizeof(szFont), szFont, QPF_NOINHERIT ))
    {
        psz = strchr( szFont, '.') + 1;
        strncpy( styles.szFont, psz, FACESIZE );
    }

    memcpy( &(pConfig->clockStyle), &styles, sizeof(CLKSTYLE) );
    return TRUE;
}


