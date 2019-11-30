/****************************************************************************
 * uclock.c                                                                 *
 *                                                                          *
 * Contains the Universal Clock main program and functions related to the   *
 * basic application window/logic.                                          *
 *                                                                          *
 ****************************************************************************
 *  This program is free software; you can redistribute it and/or modify    *
 *  it under the terms of the GNU General Public License as published by    *
 *  the Free Software Foundation; either version 2 of the License, or       *
 *  (at your option) any later version.                                     *
 *                                                                          *
 *  This program is distributed in the hope that it will be useful,         *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of          *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 *  GNU General Public License for more details.                            *
 *                                                                          *
 *  You should have received a copy of the GNU General Public License       *
 *  along with this program; if not, write to the Free Software             *
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA                *
 *  02111-1307  USA                                                         *
 *                                                                          *
 ****************************************************************************/
#define INCL_DOSERRORS
#define INCL_DOSMISC
#define INCL_DOSRESOURCES
#define INCL_DOSMODULEMGR
#define INCL_DOSPROCESS
#define INCL_GPI
#define INCL_WIN
#define INCL_WINPOINTERS
#include <os2.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <uconv.h>
#include <unidef.h>

#include <PMPRINTF.H>

#include "wtdpanel.h"
#include "uclock.h"
#include "ids.h"


// ----------------------------------------------------------------------------
// CONSTANTS

/* ------------------------------------------------------------------------- *
 * Main program (including initialization, message loop, and final cleanup)  *
 * ------------------------------------------------------------------------- */
int main( int argc, char *argv[] )
{
    UCLGLOBAL global;
    HAB       hab;                          // anchor block handle
    HMQ       hmq;                          // message queue handle
    HLIB      hlib;
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


    // Presentation Manager program initialization
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

    // Register the custom clock panel class
    if (( ! fInitFailure ) &&
        ( ! WinRegisterClass( hab, WT_DISPLAY, WTDisplayProc, CS_SIZEREDRAW, sizeof(PWTDDATA) )))
    {
        sprintf( szError, "Failed to register class %s:\nWinGetLastError() = 0x%X\n", WT_DISPLAY, WinGetLastError(hab) );
        fInitFailure = TRUE;
    }
    // And the UniClock main window class
    if (( ! fInitFailure ) &&
        ( ! WinRegisterClass( hab, SZ_WINDOWCLASS, MainWndProc, CS_SIZEREDRAW, sizeof(PUCLGLOBAL) )))
    {
        sprintf( szError, "Failed to register class %s:\nWinGetLastError() = 0x%X\n", SZ_WINDOWCLASS, WinGetLastError(hab) );
        fInitFailure = TRUE;
    }

    if ( !fInitFailure ) {
        // Get a handle to WPConfig (for the colour wheel support)
        hlib = WinLoadLibrary( hab, "WPCONFIG.DLL" );

        // Initialize global program data
        memset( &global, 0, sizeof(global) );
        global.hab      = hab;
        global.hmq      = hmq;
        global.usClocks = 0;
        global.usCompactThreshold = 50;
        global.bDescWidth = 55;

        // Get system environment variables
        pszEnv = getenv("TZ");
        if (pszEnv) strncpy( global.szTZ, pszEnv, TZSTR_MAXZ-1 );

        pszEnv = getenv("LANG");
        if (pszEnv) strncpy( global.szLoc, pszEnv, ULS_LNAMEMAX );

        // Create the program window
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
        // Basic window setup

        // Save pointer to the global data struct in a window word
        WinSetWindowPtr( hwndClient, 0, &global );

        // Get various handles to the frame controls
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

        // Create the popup menu
        global.hwndPopup = WinLoadMenu( HWND_DESKTOP, 0, IDM_POPUP );

        // Open the INI file, if there is one
        OpenProfile( &global );

        // Initialize and display the user interface
        WindowSetup( hwndFrame, hwndClient );

        // Now run the main program message loop
        while ( WinGetMsg( hab, &qmsg, 0, 0, 0 )) WinDispatchMsg( hab, &qmsg );

        // Stop the clock timer
        WinStopTimer( hab, hwndClient, ID_TIMER );

        // Free handle to WPConfig
        WinDeleteLibrary( hab, hlib );
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

        // Mouse drag (either button) - move the window
        case WM_BEGINDRAG:
            WinSetFocus( HWND_DESKTOP, hwnd );
            WinSendMsg( WinQueryWindow( hwnd, QW_PARENT ), WM_TRACKFRAME, MPFROMSHORT(TF_MOVE), MPVOID );
            break;


        // Do nothing here (initial setup is called from main after this returns)
        case WM_CREATE:
            return (MRESULT) FALSE;


        case WM_COMMAND:
            switch( SHORT1FROMMP( mp1 )) {

                case ID_CONFIG:
                    ConfigNotebook( hwnd );
                    //  WinPostMsg( hwnd, WM_SAVEAPPLICATION, 0, 0 );
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
            // Not used at present
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
            // This causes some kind of race condition, so we don't call it now
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
                szRes2[ 32 ],          // ditto
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

    pGlobal->hptrDay = WinLoadPointer( HWND_DESKTOP, 0L, ICON_DAY );
    pGlobal->hptrNight = WinLoadPointer( HWND_DESKTOP, 0L, ICON_NIGHT );

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
        WinSendMsg( pGlobal->clocks[0], WTD_SETLOCALE, MPFROMP("univ"), MPVOID );
        WinSendMsg( pGlobal->clocks[0], WTD_SETOPTIONS, MPFROMLONG( WTF_BORDER_FULL ), MPVOID );
//        WinSendMsg( pGlobal->clocks[0], WTD_SETINDICATORS, MPFROMP( pGlobal->hptrDay ), MPFROMP( pGlobal->hptrNight ));
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
        WinSendMsg( pGlobal->clocks[1], WTD_SETLOCALE, MPFROMP(pGlobal->szLoc), MPVOID );
        WinSendMsg( pGlobal->clocks[1], WTD_SETOPTIONS, MPFROMLONG( WTF_BORDER_FULL & ~WTF_BORDER_BOTTOM ), MPVOID );
//        WinSendMsg( pGlobal->clocks[1], WTD_SETINDICATORS, MPFROMP( pGlobal->hptrDay ), MPFROMP( pGlobal->hptrNight ));
        WinSetPresParam( pGlobal->clocks[1], PP_SEPARATORCOLOR, sizeof(ULONG), &clrS );
        WinSetPresParam( pGlobal->clocks[1], PP_BORDERCOLOR, sizeof(ULONG), &clrB );
        WinSetPresParam( pGlobal->clocks[1], PP_FONTNAMESIZE, 11, "9.WarpSans");
    }
    else for ( i = 0; i < pGlobal->usClocks; i++ ) {
        // Load each clock's configuration from the INI
        sprintf( szPrfKey, "%s%02d", PRF_KEY_PANEL, i );
        fData = LoadIniData( &wtInit, sizeof(wtInit), pGlobal->hIni, PRF_APP_CLOCKDATA, szPrfKey );
        if ( !fData ) {
            // Saved INI data did not match the structure size!
            if ( i == 0 ) {
                // Warn the user about this (but only for the first clock)
                WinLoadString( pGlobal->hab, NULLHANDLE, IDS_ERROR_INI_TITLE, 31, szRes2 );
                WinLoadString( pGlobal->hab, NULLHANDLE, IDS_ERROR_CLOCKDATA, SZRES_MAXZ-1, szRes );
                if ( WinMessageBox( HWND_DESKTOP, hwnd, szRes, szRes2, 0,
                                    MB_OKCANCEL | MB_WARNING | MB_MOVEABLE ) != MBID_OK )
                {
                    WinPostMsg( hwnd, WM_QUIT, 0L, 0L );
                    return FALSE;
                }
            }
            memset( &wtInit, 0, sizeof( wtInit ));
            // First, attempt a recovery by loading the stub data (up to uzDateFmt)
            // which should be the same in all program versions...
            if ( PrfQueryProfileSize( pGlobal->hIni, PRF_APP_CLOCKDATA, szPrfKey, &ulVal ) &&
                 ( ulVal >= sizeof( WTDCSTUB )))
            {
                ulVal = sizeof( WTDCSTUB );
                PrfQueryProfileData( pGlobal->hIni, PRF_APP_CLOCKDATA, szPrfKey, &wtInit, &ulVal );
            }
            else {
                // Hmm, that failed too. Maybe a corrupt INI file?
                // We can't do anything except revert to default clock settings.
                if ( ! ( uconv &&
                       ( WinLoadString( pGlobal->hab, NULLHANDLE, IDS_LOC_DEFAULT, SZRES_MAXZ-1, szRes )) &&
                       ( UniStrToUcs( uconv, wtInit.uzDesc, szRes, SZRES_MAXZ ) == ULS_SUCCESS )))
                    UniStrcpy( wtInit.uzDesc, L"(Current Location)");
                wtInit.flOptions = WTF_DATE_SYSTEM | WTF_TIME_SYSTEM;
                sprintf( wtInit.szTZ, pGlobal->szTZ );
                UniStrcpy( wtInit.uzDateFmt, L"%x");
                UniStrcpy( wtInit.uzTimeFmt, L"%X");
            }
            wtInit.bSep = pGlobal->bDescWidth;
            wtInit.cb = sizeof( WTDCDATA );
        }
        sprintf( szPrfKey, "%s%02d", PRF_KEY_PRESPARAM, i );
        fLook = LoadIniData( &savedPP, sizeof(savedPP), pGlobal->hIni, PRF_APP_CLOCKDATA, szPrfKey );
        pGlobal->clocks[i] = WinCreateWindow( hwndClient, WT_DISPLAY, "", 0L,
                                              0, 0, 0, 0, hwndClient, HWND_TOP, FIRST_CLOCK+i, &wtInit, NULL );
        if ( ! pGlobal->clocks[i] ) continue;

//        WinSendMsg( pGlobal->clocks[i], WTD_SETINDICATORS, MPFROMP( pGlobal->hptrDay ), MPFROMP( pGlobal->hptrNight ));
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

    pGlobal->usCols = (pGlobal->usPerColumn && pGlobal->usClocks) ?
                        (USHORT)ceil((float)pGlobal->usClocks / pGlobal->usPerColumn ) :
                        1;
    UpdateClockBorders( pGlobal );

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
    USHORT     i, ccount;
    ULONG      flOpts;
    LONG       x, y, cx, cy;

    pGlobal = WinQueryWindowPtr( hwnd, 0 );
    if (pGlobal && pGlobal->usClocks)
    {
        WinQueryWindowRect( hwnd, &rcl );

        x  = rcl.xLeft;
        y  = rcl.yBottom;
        cx = rcl.xRight / pGlobal->usCols;
        cy = (rcl.yTop - y) / (pGlobal->usPerColumn? pGlobal->usPerColumn: pGlobal->usClocks);

        ccount = 0;
        for ( i = 0; i < pGlobal->usClocks; i++ ) {
            ccount++;
            if ( ! pGlobal->clocks[i] ) break;
            flOpts = (ULONG) WinSendMsg( pGlobal->clocks[i], WTD_QUERYOPTIONS, MPVOID, MPVOID );
            if ( cy < pGlobal->usCompactThreshold )
                flOpts |= WTF_MODE_COMPACT;
            else
                flOpts &= ~WTF_MODE_COMPACT;
            WinSendMsg( pGlobal->clocks[i], WTD_SETOPTIONS, MPFROMLONG(flOpts), MPVOID );
            WinSetWindowPos( pGlobal->clocks[i], HWND_TOP, x, y, cx, cy, SWP_SIZE | SWP_MOVE | SWP_SHOW );
            if ( pGlobal->usPerColumn && ( ccount >= pGlobal->usPerColumn )) {
                ccount = 0;
                x += cx;
                y = rcl.yBottom;
            }
            else
                y += cy;
        }
    }
}


/* ------------------------------------------------------------------------- *
 * UpdateClockBorders                                                        *
 *                                                                           *
 * ------------------------------------------------------------------------- */
void UpdateClockBorders( PUCLGLOBAL pGlobal )
{
    LONG   flOpts;
    USHORT i, ccount;

    for ( i = 0, ccount = 0; i < pGlobal->usClocks; i++, ccount++ ) {
        if ( !pGlobal->clocks[i] ) break;
        flOpts = (ULONG) WinSendMsg( pGlobal->clocks[i], WTD_QUERYOPTIONS, 0, 0 );
        flOpts |= WTF_BORDER_FULL;
        // Turn off the bottom border for all but the bottom-most clock in each column
        if ( pGlobal->usPerColumn && ( ccount >= pGlobal->usPerColumn ))
            ccount = 0;
        if ( ccount )
            flOpts &= ~WTF_BORDER_BOTTOM;
        WinSendMsg( pGlobal->clocks[i], WTD_SETOPTIONS, MPFROMLONG(flOpts), 0 );
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
    RECTL      rcl, rclx;
    SWP        swp;
    USHORT     i;

    hps = WinBeginPaint( hwnd, NULLHANDLE, NULLHANDLE );
    if (hps)
    {
        pGlobal = WinQueryWindowPtr( hwnd, 0 );
        if (pGlobal /* && (pGlobal->usClocks == 0) */)
        {
            WinQueryWindowRect( hwnd, &rcl );
            for ( i = 0; i < pGlobal->usClocks; i++ ) {
                if ( WinQueryWindowPos( pGlobal->clocks[i], &swp )) {
                    rclx.xLeft = swp.x + 1;
                    rclx.xRight = swp.x + swp.cx - 1;
                    rclx.yBottom = swp.y + 1;
                    rclx.yTop = swp.y + swp.cy - 1;
                    GpiExcludeClipRectangle( hps, &rclx );
                }
            }
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
 * MoveListItem                                                              *
 *                                                                           *
 * Move a list item up or down (by one) within a listbox control. This       *
 * function does not perform any bounds check; the caller must do that.      *
 *                                                                           *
 * ARGUMENTS:                                                                *
 *   HWND  hwndList: Handle of the listbox control.                          *
 *   SHORT sItem   : Index of the list item to move.                         *
 *   BOOL  fMoveUp : If TRUE, item will be moved up by one position;         *
 *                   if FALSE, item will be moved down by one position.      *
 *                                                                           *
 * RETURNS: SHORT                                                            *
 *   The new item index (return code from LM_INSERTITEM).                    *
 * ------------------------------------------------------------------------- */
SHORT MoveListItem( HWND hwndList, SHORT sItem, BOOL fMoveUp )
{
    UCHAR szDesc[ LOCDESC_MAXZ ];
    ULONG ul;

    WinSendMsg( hwndList, LM_QUERYITEMTEXT,
                MPFROM2SHORT( sItem, LOCDESC_MAXZ-1 ), MPFROMP(szDesc) );
    ul = (ULONG) WinSendMsg( hwndList, LM_QUERYITEMHANDLE,
                             MPFROMSHORT(sItem), 0 );
    WinEnableWindowUpdate( hwndList, FALSE );
    WinSendMsg( hwndList, LM_DELETEITEM, MPFROMSHORT(sItem), 0 );
    if ( fMoveUp )
        sItem--;
    else
        sItem++;
    sItem = (SHORT) WinSendMsg( hwndList, LM_INSERTITEM,
                                MPFROMSHORT(sItem), MPFROMP(szDesc) );
    WinSendMsg( hwndList, LM_SETITEMHANDLE,
                MPFROMSHORT(sItem), MPFROMLONG((LONG)ul) );
    WinShowWindow( hwndList, TRUE );
    return ( sItem );
}


/* ------------------------------------------------------------------------- *
 * ErrorMessage                                                              *
 *                                                                           *
 * ARGUMENTS:                                                                *
 *   HWND   hwnd: Handle of the message-box owner.                           *
 *   USHORT usID: Resource ID of the error string to display.                *
 *                                                                           *
 * RETURNS: N/A                                                              *
 * ------------------------------------------------------------------------- */
void ErrorMessage( HWND hwnd, USHORT usID )
{
    HAB  hab;
    CHAR szRes1[ SZRES_MAXZ ],
         szRes2[ 20 ];  // just need enough for the "Error" string in any language

    if ( hwnd == NULLHANDLE )
        hwnd = HWND_DESKTOP;

    hab = WinQueryAnchorBlock( hwnd );
    if ( !WinLoadString( hab, NULLHANDLE, usID, sizeof( szRes1 ) - 1, szRes1 ))
        sprintf( szRes1, "Error %d\n(Failed to load string resource)", usID );
    if ( !WinLoadString( hab, NULLHANDLE, IDS_ERROR_TITLE, sizeof( szRes2 ) - 1, szRes2 ))
        strcpy( szRes2, "Error");
    WinMessageBox( HWND_DESKTOP, hwnd, szRes1, szRes2, 0, MB_OK | MB_ERROR );
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
            ErrorMessage( hwnd, IDS_ERROR_CLKDATA );
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

    flOptions = 0;
/*  UpdateClockBorders() should take care of this now
    // only bottom-row clocks should have a bottom border
    flOptions = WTF_BORDER_FULL;
    if ( usNew ) flOptions &= ~WTF_BORDER_BOTTOM;
*/

//    WinSendMsg( hwndClock, WTD_SETINDICATORS, MPFROMP(pGlobal->hptrDay), MPFROMP(pGlobal->hptrNight));

    // set default properties
    WinSendMsg( hwndClock, WTD_SETTIMEZONE, MPFROMP(pGlobal->szTZ), MPFROMP(uzRes));
    WinSendMsg( hwndClock, WTD_SETLOCALE, MPFROMP(pGlobal->szLoc), 0L );
    WinSendMsg( hwndClock, WTD_SETOPTIONS, MPFROMLONG(flOptions), 0L );
    WinSendMsg( hwndClock, WTD_SETSEPARATOR, MPFROMCHAR(pGlobal->bDescWidth), 0L );
    WinSetPresParam( hwndClock, PP_SEPARATORCOLOR, sizeof(ULONG), &clrS );
    WinSetPresParam( hwndClock, PP_BORDERCOLOR, sizeof(ULONG), &clrB );
    WinSetPresParam( hwndClock, PP_FONTNAMESIZE, 11, "9.WarpSans");

    // now show the properties dialog
    if ( ClockNotebook( hwnd, usNew ) ) {
        // new clock was accepted, so add it to the GUI
        pGlobal->usClocks++;
        pGlobal->usCols = pGlobal->usPerColumn ?
                            (USHORT)ceil((float)pGlobal->usClocks / pGlobal->usPerColumn ) :
                            1;
        UpdateClockBorders( pGlobal );
    }
    else {
        // delete the new clock if the user cancelled
        WinDestroyWindow( pGlobal->clocks[usNew] );
        pGlobal->clocks[usNew] = NULLHANDLE;
    }

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
    pGlobal->usCols = (pGlobal->usPerColumn && pGlobal->usClocks) ?
                        (USHORT)ceil((float)pGlobal->usClocks / pGlobal->usPerColumn ) :
                        1;

/*
    // The bottom-most clock (clock 0, and only 0) should have a full border.
    // So if we deleted that clock, we change the border on the new clock 0.
    if ( usDelete == 0 )
        WinSendMsg( pGlobal->clocks[0], WTD_SETOPTIONS,
                    MPFROMLONG( WTF_BORDER_FULL ), MPVOID );
*/
    UpdateClockBorders( pGlobal );
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
    HAB  hab;
    CHAR szBuffer[ SZRES_MAXZ ],
         szText[ SZRES_MAXZ ];

    switch ( msg ) {

        case WM_INITDLG:
            hab = WinQueryAnchorBlock( hwnd );
            if ( WinLoadString( hab, 0, IDS_ABOUT_VERSION, SZRES_MAXZ-1, szBuffer ))
                sprintf( szText, szBuffer, SZ_VERSION );
            else
                sprintf( szText, "V%s", SZ_VERSION );
            WinSetDlgItemText( hwnd, IDD_VERSION, szText );

            if ( WinLoadString( hab, 0, IDS_ABOUT_COPYRIGHT, SZRES_MAXZ-1, szBuffer ))
                sprintf( szText, szBuffer, SZ_COPYRIGHT );
            else
                sprintf( szText, "(C) %s Alex Taylor", SZ_COPYRIGHT );
            WinSetDlgItemText( hwnd, IDD_COPYRIGHT, szText );

            CentreWindow( hwnd );
            break;

        default: break;
    }

    return WinDefDlgProc( hwnd, msg, mp1, mp2 );
}


/* ------------------------------------------------------------------------- *
 * ClrDlgProc                                                                *
 *                                                                           *
 * Dialog procedure for the colour-selection dialog.  Adapted from the       *
 * following (public domain) sample code by Paul Ratcliffe:                  *
 * http://home.clara.net/orac/files/os2/pmcolour.zip                         *
 * ------------------------------------------------------------------------- */
MRESULT EXPENTRY ClrDlgProc( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 )
{
    CWDATA  *cwdata;
    HWND    hwndSpin;
    ULONG   newval;

    cwdata = (CWDATA *) WinQueryWindowULong( hwnd, QWL_USER );

    switch( msg )
    {
        case WM_INITDLG:
            if (!(cwdata = malloc(sizeof(CWDATA))) ||
                !mp2 || !(cwdata->rgb = &(((CWPARAM *) mp2)->rgb)))
            {
                WinPostMsg( hwnd, WM_CLOSE, MPVOID, MPVOID );
                break;
            }
            WinSetWindowULong( hwnd, QWL_USER, (ULONG) cwdata );
            cwdata->rgbold = *cwdata->rgb;
            cwdata->hwndCol = WinWindowFromID( hwnd, ID_WHEEL );
            cwdata->hwndSpinR = WinWindowFromID( hwnd, ID_SPINR );
            cwdata->hwndSpinG = WinWindowFromID( hwnd, ID_SPING );
            cwdata->hwndSpinB = WinWindowFromID( hwnd, ID_SPINB );
            cwdata->updatectl = FALSE;
            cwdata->pfCancel = &(((CWPARAM *) mp2)->fCancel);
            WinSendMsg( cwdata->hwndSpinR, SPBM_SETLIMITS,
                        MPFROMSHORT(255), MPFROMSHORT(0) );
            WinSendMsg( cwdata->hwndSpinG, SPBM_SETLIMITS,
                        MPFROMSHORT(255), MPFROMSHORT(0) );
            WinSendMsg( cwdata->hwndSpinB, SPBM_SETLIMITS,
                        MPFROMSHORT(255), MPFROMSHORT(0) );
            WinSendMsg( hwnd, CWN_CHANGE,
                        MPFROMLONG(*((ULONG *) cwdata->rgb) & 0xFFFFFF), MPVOID );
            WinSendMsg( cwdata->hwndCol, CWM_SETCOLOUR,
                        MPFROMLONG(*((ULONG *) cwdata->rgb) & 0xFFFFFF), MPVOID) ;
            break;

        case WM_COMMAND:
            switch(SHORT1FROMMP(mp1))
            {
                case ID_UNDO:
                    WinSendMsg( hwnd, CWN_CHANGE,
                                MPFROMLONG(*((ULONG *) &cwdata->rgbold) & 0xFFFFFF),
                                MPVOID );
                    WinSendMsg( cwdata->hwndCol, CWM_SETCOLOUR,
                                MPFROMLONG(*((ULONG *) &cwdata->rgbold) & 0xFFFFFF),
                                MPVOID );
                    return ((MPARAM) 0);

                case DID_CANCEL:
                    WinSendMsg( hwnd, CWN_CHANGE,
                                MPFROMLONG(*((ULONG *) &cwdata->rgbold) & 0xFFFFFF),
                                MPVOID );
                    WinSendMsg( cwdata->hwndCol, CWM_SETCOLOUR,
                                MPFROMLONG(*((ULONG *) &cwdata->rgbold) & 0xFFFFFF),
                                MPVOID );
                    *(cwdata->pfCancel) = TRUE;
                    break;
            }
            break;

        case CWN_CHANGE:
            *cwdata->rgb = *((RGB *) &mp1);
            cwdata->updatectl = FALSE;
            WinSendMsg( cwdata->hwndSpinR, SPBM_SETCURRENTVALUE,
                        MPFROMSHORT(cwdata->rgb->bRed), MPVOID );
            WinSendMsg( cwdata->hwndSpinG, SPBM_SETCURRENTVALUE,
                        MPFROMSHORT(cwdata->rgb->bGreen), MPVOID );
            WinSendMsg( cwdata->hwndSpinB, SPBM_SETCURRENTVALUE,
                        MPFROMSHORT(cwdata->rgb->bBlue), MPVOID );
            cwdata->updatectl = TRUE;
            break;

        case WM_CONTROL:
            switch(SHORT1FROMMP(mp1))
            {
                case ID_SPINR:
                case ID_SPING:
                case ID_SPINB:
                    hwndSpin = WinWindowFromID( hwnd, SHORT1FROMMP(mp1) );
                    switch(SHORT2FROMMP(mp1))
                    {
                        case SPBN_CHANGE:
                        case SPBN_KILLFOCUS:
                            if (cwdata->updatectl)
                            {
                                WinSendMsg( hwndSpin, SPBM_QUERYVALUE,
                                            (MPARAM) &newval,
                                            MPFROM2SHORT(0, SPBQ_ALWAYSUPDATE) );
                                switch(SHORT1FROMMP(mp1))
                                {
                                    case ID_SPINR:
                                    cwdata->rgb->bRed = (BYTE) newval;
                                    break;

                                    case ID_SPING:
                                    cwdata->rgb->bGreen = (BYTE) newval;
                                    break;

                                    case ID_SPINB:
                                    cwdata->rgb->bBlue = (BYTE) newval;
                                }

                                WinSendMsg( cwdata->hwndCol, CWM_SETCOLOUR,
                                            MPFROMLONG(*((ULONG *) cwdata->rgb) &
                                            0xFFFFFF), MPVOID );
                            }
                        default: break;
                    }
                default: break;
            }
            break;

        case WM_DESTROY:
            free( cwdata );
    }

    return WinDefDlgProc( hwnd, msg, mp1, mp2 );
}


/* ------------------------------------------------------------------------- *
 * SelectFont                                                                *
 *                                                                           *
 * Pop up a font selection dialog.                                           *
 *                                                                           *
 * Parameters:                                                               *
 *   HWND   hwnd   : handle of the current window.                           *
 *   PSZ    pszFace: pointer to buffer containing current font name.         *
 *   USHORT cbBuf  : size of the buffer pointed to by 'pszFace'.             *
 *                                                                           *
 * RETURNS: BOOL                                                             *
 * ------------------------------------------------------------------------- */
BOOL SelectFont( HWND hwnd, PSZ pszFace, USHORT cbBuf )
{
    FONTDLG     fontdlg = {0};
    FONTMETRICS fm      = {0};
    LONG        lQuery  = 0;
    CHAR        szName[ FACESIZE ];
    HWND        hwndFD;
    HPS         hps;

    hps = WinGetPS( hwnd );
    strncpy( szName, pszFace, FACESIZE-1 );

    // Get the metrics of the current font (we'll want to know the weight class)
    lQuery = 1;
    GpiQueryFonts( hps, QF_PUBLIC, pszFace, &lQuery, sizeof(fm), &fm );

    fontdlg.cbSize         = sizeof( FONTDLG );
    fontdlg.hpsScreen      = hps;
    fontdlg.pszTitle       = NULL;
    fontdlg.pszPreview     = NULL;
    fontdlg.pfnDlgProc     = NULL;
    fontdlg.pszFamilyname  = szName;
    fontdlg.usFamilyBufLen = sizeof( szName );
    fontdlg.fxPointSize    = ( fm.fsDefn & FM_DEFN_OUTLINE ) ?
                                MAKEFIXED( 10, 0 ) :
                                ( fm.sNominalPointSize / 10 );
    fontdlg.usWeight       = (USHORT) fm.usWeightClass;
    fontdlg.clrFore        = SYSCLR_WINDOWTEXT;
    fontdlg.clrBack        = SYSCLR_WINDOW;
    fontdlg.fl             = FNTS_CENTER | FNTS_CUSTOM;
    fontdlg.flStyle        = 0;
    fontdlg.flType         = ( fm.fsSelection & FM_SEL_ITALIC ) ? FTYPE_ITALIC : 0;
    fontdlg.usDlgId        = IDD_FONTDLG;
    fontdlg.hMod           = NULLHANDLE;

    hwndFD = WinFontDlg( HWND_DESKTOP, hwnd, &fontdlg );
    WinReleasePS( hps );
    if (( hwndFD ) && ( fontdlg.lReturn == DID_OK )) {
        strncpy( pszFace, fontdlg.fAttrs.szFacename, cbBuf-1 );
        return TRUE;
    }
    return FALSE;
}


/* ------------------------------------------------------------------------- *
 * SelectColour                                                              *
 *                                                                           *
 * Pop up a colour selection dialog.                                         *
 *                                                                           *
 * Parameters:                                                               *
 *   HWND hwnd   : handle of the current window.                             *
 *   HWND hwndCtl: handle of the control whose colour is being updated.      *
 *                                                                           *
 * RETURNS: BOOL                                                             *
 * ------------------------------------------------------------------------- */
void SelectColour( HWND hwnd, HWND hwndCtl )
{
    CWPARAM cwp;
    ULONG   clr;

    cwp.cb = sizeof(CWPARAM);
    cwp.fCancel = FALSE;
    if ( WinQueryPresParam( hwndCtl, PP_BACKGROUNDCOLOR,
                            PP_BACKGROUNDCOLORINDEX, NULL,
                            sizeof(clr), &clr, QPF_ID2COLORINDEX ))
    {
        cwp.rgb.bRed   = RGBVAL_RED( clr );
        cwp.rgb.bGreen = RGBVAL_GREEN( clr );
        cwp.rgb.bBlue  = RGBVAL_BLUE( clr );
        WinDlgBox( HWND_DESKTOP, hwnd, ClrDlgProc,
                   NULLHANDLE, ID_CLRDLG, &cwp );

        if ( !cwp.fCancel ) {
            clr = RGB2LONG( cwp.rgb.bRed, cwp.rgb.bGreen, cwp.rgb.bBlue );
            WinSetPresParam( hwndCtl, PP_BACKGROUNDCOLOR, sizeof(clr), &clr );
        }
    }

}



