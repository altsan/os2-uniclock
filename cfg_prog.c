/****************************************************************************
 * cfg_prog.c                                                               *
 *                                                                          *
 * Configuration dialogs/notebook pages and related logic for implementing  *
 * the global program settings.                                             *
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


/* ------------------------------------------------------------------------- *
 * ConfigNotebook                                                            *
 *                                                                           *
 * Initialize and open the configuration notebook dialog.                    *
 * ------------------------------------------------------------------------- */
void ConfigNotebook( HWND hwnd )
{
    UCFGDATA   config;
    PUCLGLOBAL pGlobal;
    HWND       clocks_tmp[ MAX_CLOCKS ];
    BOOL       fReorder;
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
        config.aClockOrder[ i ] = i;
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

        fReorder = FALSE;
        for ( i = 0; i < pGlobal->usClocks; i++ ) {
            if ( config.aClockOrder[ i ] != i ) fReorder = TRUE;
            clocks_tmp[ i ] = pGlobal->clocks[ config.aClockOrder[ i ]];
        }
        if ( fReorder ) {
            for ( i = 0; i < pGlobal->usClocks; i++ ) {
                pGlobal->clocks[ i ] = clocks_tmp[ i ];
            }
        }

        pGlobal->fsStyle = config.fsStyle;
        pGlobal->bDescWidth = config.bDescWidth;
        pGlobal->usPerColumn = config.usPerColumn;
        pGlobal->usCols = (pGlobal->usPerColumn && pGlobal->usClocks) ?
                            (USHORT)ceil((float)pGlobal->usClocks / pGlobal->usPerColumn ) :
                            1;
        pGlobal->usCompactThreshold = config.usCompactThreshold;
        for ( i = 0; i < pGlobal->usClocks; i++ ) {
            WinSendMsg( pGlobal->clocks[i], WTD_SETSEPARATOR,
                        MPFROMCHAR(pGlobal->bDescWidth), 0L );
        }
        UpdateClockBorders( pGlobal );
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
    PUCFGDATA pConfig;
    HPOINTER  hicon;
    HWND      hwndPage;

    switch ( msg ) {

        case WM_INITDLG:
            hicon = WinLoadPointer( HWND_DESKTOP, 0, ID_MAINPROGRAM );
            WinSendMsg( hwnd, WM_SETICON, (MPARAM) hicon, MPVOID );

            pConfig = (PUCFGDATA) mp2;
            WinSetWindowPtr( hwnd, 0, pConfig );

            if ( ! CfgPopulateNotebook( hwnd, pConfig ) ) {
                ErrorMessage( hwnd, IDS_ERROR_NOTEBOOK );
                return (MRESULT) TRUE;
            }
            CentreWindow( hwnd );
            return (MRESULT) FALSE;


        case WM_COMMAND:
            pConfig = WinQueryWindowPtr( hwnd, 0 );
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
            pConfig = WinQueryWindowPtr( hwnd, 0 );
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
    SHORT  sIdx, sCount;

    switch ( msg ) {

        case WM_INITDLG:
            pConfig = (PUCFGDATA) mp2;
            if ( !pConfig ) return (MRESULT) TRUE;
            CfgPopulateClockList( hwnd, pConfig );

            if ( pConfig->fsStyle & APP_STYLE_TITLEBAR )
                WinSendDlgItemMsg( hwnd, IDD_TITLEBAR, BM_SETCHECK, MPFROMSHORT(1), MPVOID );
            if ( pConfig->fsStyle & APP_STYLE_WTBORDERS )
                WinSendDlgItemMsg( hwnd, IDD_BORDERS, BM_SETCHECK, MPFROMSHORT(1), MPVOID );

            WinSendDlgItemMsg( hwnd, IDD_VIEWSWITCH, SPBM_SETLIMITS,
                               MPFROMLONG(MAX_THRESHOLD), MPFROMLONG(0) );
            WinSendDlgItemMsg( hwnd, IDD_VIEWSWITCH, SPBM_SETCURRENTVALUE,
                               MPFROMLONG(pConfig->usCompactThreshold), MPVOID );
            WinSendDlgItemMsg( hwnd, IDD_DESCWIDTH, SPBM_SETLIMITS,
                               MPFROMLONG(100), MPFROMLONG(0) );
            WinSendDlgItemMsg( hwnd, IDD_DESCWIDTH, SPBM_SETCURRENTVALUE,
                               MPFROMLONG(pConfig->bDescWidth), MPVOID );
            WinSendDlgItemMsg( hwnd, IDD_PERCOLUMN, SPBM_SETLIMITS,
                               MPFROMLONG(MAX_PER_COLUMN), MPFROMLONG(0) );
            WinSendDlgItemMsg( hwnd, IDD_PERCOLUMN, SPBM_SETCURRENTVALUE,
                               MPFROMLONG(pConfig->usPerColumn), MPVOID );

            break;


        case WM_COMMAND:
            switch( SHORT1FROMMP( mp1 )) {

                case IDD_CLOCKUP:
                    sIdx = (SHORT) WinSendDlgItemMsg( hwnd, IDD_CLOCKLIST,
                                                      LM_QUERYSELECTION,
                                                      MPFROMSHORT(LIT_CURSOR), 0 );
                    if (( sIdx != LIT_NONE ) && ( sIdx > 0 )) {
                        // move list item up by one
                        MoveListItem( WinWindowFromID( hwnd, IDD_CLOCKLIST ), sIdx, TRUE );
                    }
                    break;


                case IDD_CLOCKDOWN:
                    sIdx = (SHORT) WinSendDlgItemMsg( hwnd, IDD_CLOCKLIST,
                                                      LM_QUERYSELECTION,
                                                      MPFROMSHORT(LIT_CURSOR), 0 );
                    sCount = (SHORT) WinSendDlgItemMsg( hwnd, IDD_CLOCKLIST,
                                                        LM_QUERYITEMCOUNT, 0, 0 );
                    if (( sIdx != LIT_NONE ) && ( sIdx+1 < sCount )) {
                        // move list item down by one
                        MoveListItem( WinWindowFromID( hwnd, IDD_CLOCKLIST ), sIdx, FALSE );
                    }
                    break;


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
    SHORT  sIdx;
    UCHAR  szDesc[ LOCDESC_MAXZ ];
    PSZ    psz;

    for ( i = pConfig->usClocks; i > 0; i-- ) {
        UniStrFromUcs( pConfig->uconv, szDesc, pConfig->aClockData[i-1].uzDesc, LOCDESC_MAXZ );
        strncat( szDesc, " (", LOCDESC_MAXZ );
        strncat( szDesc, pConfig->aClockData[i-1].szTZ, LOCDESC_MAXZ );
        psz = strchr( szDesc, ',');
        if ( psz ) {
            *psz = '\0';
            strncat( szDesc, "...", LOCDESC_MAXZ );
        }
        strncat( szDesc, ")", LOCDESC_MAXZ );
        sIdx = (SHORT) WinSendDlgItemMsg( hwnd, IDD_CLOCKLIST, LM_INSERTITEM,
                                          MPFROMSHORT(LIT_END), MPFROMP(szDesc) );
        WinSendDlgItemMsg( hwnd, IDD_CLOCKLIST, LM_SETITEMHANDLE,
                           MPFROMSHORT(sIdx), MPFROMLONG((LONG)i-1) );
    }
}


/* ------------------------------------------------------------------------- *
 * CfgSettingsCommon                                                         *
 *                                                                           *
 * Get configuration changes on the common settings page.                    *
 * ------------------------------------------------------------------------- */
void CfgSettingsCommon( HWND hwnd, PUCFGDATA pConfig )
{
    SHORT aNewOrder[ MAX_CLOCKS ];
    SHORT count, i, j;
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

    // Reorder the list of clocks as needed
    count = (SHORT) WinSendDlgItemMsg( hwnd, IDD_CLOCKLIST, LM_QUERYITEMCOUNT, 0L, 0L );
    if ( !count ) return;
    for ( i = 0; i < count; i++ ) {
        ul = (ULONG) WinSendDlgItemMsg( hwnd, IDD_CLOCKLIST, LM_QUERYITEMHANDLE, MPFROMSHORT(i), 0 );
        if ( ul >= MAX_CLOCKS ) return; // simple sanity check
        aNewOrder[ i ] = (SHORT) ul;
    }
    // The list orders the clocks in reverse order, so...
    for ( i = 0; i < count; i++ ) {
        j = count - i - 1;
        pConfig->aClockOrder[ i ] = aNewOrder[ j ];
    }
}


