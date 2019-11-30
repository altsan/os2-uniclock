/****************************************************************************
 * cfg_clk.c                                                                *
 *                                                                          *
 * Configuration dialogs/notebook pages and related logic for implementing  *
 * individual clock panel settings.                                         *
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
        WinSendMsg( pGlobal->clocks[usNumber], WTD_SETCOUNTRYZONE,
                    MPFROMP(props.clockData.uzTZCtry), MPFROMP(props.clockData.uzTZRegn));
        if ( props.clockData.flOptions & WTF_PLACE_HAVECOORD )
            WinSendMsg( pGlobal->clocks[usNumber], WTD_SETCOORDINATES,
                        MPFROM2SHORT(props.clockData.coordinates.sLatitude,
                                     (SHORT)props.clockData.coordinates.bLaMin),
                        MPFROM2SHORT(props.clockData.coordinates.sLongitude,
                                     (SHORT)props.clockData.coordinates.bLoMin)
                      );
        else
            WinSendMsg( pGlobal->clocks[usNumber], WTD_SETCOORDINATES,
                        MPFROMLONG( 0xFFFFFFFF ), MPFROMLONG( 0xFFFFFFFF ));

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
    PUCLKPROP pConfig;
    HPOINTER  hicon;
    HWND      hwndPage;
    UCHAR     szTitle[ SZRES_MAXZ ];

    switch ( msg ) {

        case WM_INITDLG:
            hicon = WinLoadPointer( HWND_DESKTOP, 0, ID_MAINPROGRAM );
            WinSendMsg( hwnd, WM_SETICON, (MPARAM) hicon, MPVOID );

            pConfig = (PUCLKPROP) mp2;
            WinSetWindowPtr( hwnd, 0, pConfig );

            sprintf( szTitle, "Clock %d - Properties", pConfig->usClock+1 );     // TODO NLS
            WinSetWindowText( hwnd, szTitle );

            if ( ! ClkPopulateNotebook( hwnd, pConfig ) ) {
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
            pConfig = WinQueryWindowPtr( hwnd, 0 );
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
            szCoord[ ISO6709_MAXZ ],
            szSysDef[ SZRES_MAXZ ],
            szLocDef[ SZRES_MAXZ ],
            szLocAlt[ SZRES_MAXZ ],
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
                                           MPFROMSHORT(LIT_SORTASCENDING),
                                           MPFROMP(szLocale) );
                        offset += UniStrlen( puz ) + 1;
                    }
                }
                free( puzLocales );
            }

            if ( ! WinLoadString( pConfig->hab, 0, IDS_FORMAT_SYSDEF, SZRES_MAXZ-1, szSysDef ))
                sprintf( szSysDef, "System default");
            if ( ! WinLoadString( pConfig->hab, 0, IDS_FORMAT_LOCDEF, SZRES_MAXZ-1, szLocDef ))
                sprintf( szLocDef, "Locale default");
            if ( ! WinLoadString( pConfig->hab, 0, IDS_FORMAT_LOCALT, SZRES_MAXZ-1, szLocAlt ))
                sprintf( szLocAlt, "Locale alternative or default");
            if ( ! WinLoadString( pConfig->hab, 0, IDS_FORMAT_CUSTOM, SZRES_MAXZ-1, szCustom ))
                sprintf( szCustom, "Custom string");
            WinSendDlgItemMsg( hwnd, IDD_TIMEFMT,  LM_INSERTITEM, MPFROMSHORT(LIT_END), MPFROMP(szSysDef) );
            WinSendDlgItemMsg( hwnd, IDD_TIMEFMT,  LM_INSERTITEM, MPFROMSHORT(LIT_END), MPFROMP(szLocDef) );
            WinSendDlgItemMsg( hwnd, IDD_TIMEFMT,  LM_INSERTITEM, MPFROMSHORT(LIT_END), MPFROMP(szLocAlt) );
            WinSendDlgItemMsg( hwnd, IDD_TIMEFMT,  LM_INSERTITEM, MPFROMSHORT(LIT_END), MPFROMP(szCustom) );
            WinSendDlgItemMsg( hwnd, IDD_DATEFMT,  LM_INSERTITEM, MPFROMSHORT(LIT_END), MPFROMP(szSysDef) );
            WinSendDlgItemMsg( hwnd, IDD_DATEFMT,  LM_INSERTITEM, MPFROMSHORT(LIT_END), MPFROMP(szLocDef) );
            WinSendDlgItemMsg( hwnd, IDD_DATEFMT,  LM_INSERTITEM, MPFROMSHORT(LIT_END), MPFROMP(szLocAlt) );
            WinSendDlgItemMsg( hwnd, IDD_DATEFMT,  LM_INSERTITEM, MPFROMSHORT(LIT_END), MPFROMP(szCustom) );

            // Set the current values
            WinSetDlgItemText( hwnd, IDD_TZDISPLAY, pConfig->clockData.szTZ );
            UnParseTZCoordinates( szCoord, pConfig->clockData.coordinates );
            WinSetDlgItemText( hwnd, IDD_COORDINATES, szCoord );
            if ( pConfig->clockData.flOptions & WTF_PLACE_HAVECOORD ) {
                WinCheckButton( hwnd, IDD_USECOORDINATES, TRUE );
                WinEnableControl( hwnd, IDD_COORDINATES, TRUE );
            }

            // description field
            if ( pConfig->uconv &&
                ( UniStrFromUcs( pConfig->uconv, szName,
                                 pConfig->clockData.uzDesc, LOCDESC_MAXZ ) == ULS_SUCCESS ))
                WinSetDlgItemText( hwnd, IDD_CITYLIST, szName );

            // formatting locale
            sIdx = (SHORT) WinSendDlgItemMsg( hwnd, IDD_LOCALES, LM_SEARCHSTRING,
                                              MPFROM2SHORT(0,LIT_FIRST),
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
            else if ( pConfig->clockData.flOptions & WTF_TIME_ALT )
                sIdx = 2;
            else if ( pConfig->clockData.flOptions & WTF_TIME_CUSTOM )
                sIdx = 3;
            else
                sIdx = 1;
            WinSendDlgItemMsg( hwnd, IDD_TIMEFMT, LM_SELECTITEM,
                               MPFROMSHORT(sIdx), MPFROMSHORT(TRUE) );

            if ( sIdx < 2 && pConfig->clockData.flOptions & WTF_TIME_SHORT )
                WinSendDlgItemMsg( hwnd, IDD_TIMESHORT, BM_SETCHECK, MPFROMSHORT(1), MPVOID );

            // date format controls (primary)
            if ( pConfig->uconv &&
                 ( UniStrFromUcs( pConfig->uconv, szFormat,
                                  pConfig->clockData.uzDateFmt, STRFT_MAXZ ) == ULS_SUCCESS ))
                WinSetDlgItemText( hwnd, IDD_DATESTR, szFormat );

            if ( pConfig->clockData.flOptions & WTF_DATE_SYSTEM )
                sIdx = 0;
            else if ( pConfig->clockData.flOptions & WTF_DATE_ALT )
                sIdx = 2;
            else if ( pConfig->clockData.flOptions & WTF_DATE_CUSTOM )
                sIdx = 3;
            else
                sIdx = 1;
            WinSendDlgItemMsg( hwnd, IDD_DATEFMT, LM_SELECTITEM,
                               MPFROMSHORT(sIdx), MPFROMSHORT(TRUE) );
            return (MRESULT) FALSE;


        case WM_COMMAND:
            switch( SHORT1FROMMP( mp1 )) {

                case IDD_TZSELECT:
                    SelectTimeZone( hwnd, pConfig );
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


        case WM_CONTROL:
            switch( SHORT1FROMMP( mp1 )) {

                case IDD_TIMEFMT:
                    if ( SHORT2FROMMP(mp1) == LN_SELECT ) {
                        hwndLB = (HWND) mp2;
                        sIdx = (SHORT) WinSendMsg( hwndLB, LM_QUERYSELECTION, MPFROMSHORT(LIT_FIRST), 0 );
                        WinShowWindow( WinWindowFromID(hwnd, IDD_TIMESHORT), (sIdx < 2)? TRUE: FALSE );
                        WinShowWindow( WinWindowFromID(hwnd, IDD_TIMESTR), (sIdx == 3)? TRUE: FALSE );
                    }
                    break;

                case IDD_DATEFMT:
                    if ( SHORT2FROMMP(mp1) == LN_SELECT ) {
                        hwndLB = (HWND) mp2;
                        sIdx = (SHORT) WinSendMsg( hwndLB, LM_QUERYSELECTION, MPFROMSHORT(LIT_FIRST), 0 );
                        WinShowWindow( WinWindowFromID(hwnd, IDD_DATESTR), (sIdx == 3)? TRUE: FALSE );
                    }
                    break;

                case IDD_USECOORDINATES:
                    WinEnableControl( hwnd, IDD_COORDINATES,
                                      (BOOL)WinQueryButtonCheckstate( hwnd, IDD_USECOORDINATES ));
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
    UCHAR  szFontPP[ FACESIZE+4 ];
    ULONG  clr;

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

                case IDD_DISPLAYFONT_BTN:
                    if ( WinQueryPresParam( WinWindowFromID(hwnd, IDD_DISPLAYFONT),
                                            PP_FONTNAMESIZE, 0, NULL,
                                            sizeof(szFontPP), szFontPP, QPF_NOINHERIT ))
                    {
                        PSZ psz = strchr( szFontPP, '.') + 1;
                        if ( SelectFont( hwnd, psz,
                                         sizeof(szFontPP) - ( strlen(szFontPP) - strlen(psz) )))
                        {
                            WinSetPresParam( WinWindowFromID( hwnd, IDD_DISPLAYFONT ),
                                             PP_FONTNAMESIZE, strlen(szFontPP)+1, szFontPP );
                        }
                    }
                    break;

                case IDD_BACKGROUND_BTN:
                    SelectColour( hwnd, WinWindowFromID( hwnd, IDD_BACKGROUND ));
                    break;

                case IDD_FOREGROUND_BTN:
                    SelectColour( hwnd, WinWindowFromID( hwnd, IDD_FOREGROUND ));
                    break;

                case IDD_BORDER_BTN:
                    SelectColour( hwnd, WinWindowFromID( hwnd, IDD_BORDER ));
                    break;

                case IDD_SEPARATOR_BTN:
                    SelectColour( hwnd, WinWindowFromID( hwnd, IDD_SEPARATOR ));
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
        // case 1 is the locale default, which uses bitmask 0
        case 2:  settings.flOptions |= WTF_TIME_ALT;    break;
        case 3:  settings.flOptions |= WTF_TIME_CUSTOM; break;
        default: break;
    }
    if ( sIdx < 2 && WinQueryButtonCheckstate( hwnd, IDD_TIMESHORT ))
        settings.flOptions |= WTF_TIME_SHORT;

    sIdx = (SHORT) WinSendDlgItemMsg( hwnd, IDD_DATEFMT, LM_QUERYSELECTION,
                                      MPFROMSHORT(LIT_FIRST), 0 );
    switch ( sIdx ) {
        case 0:  settings.flOptions |= WTF_DATE_SYSTEM; break;
        // case 1 is the locale default, which uses bitmask 0
        case 2:  settings.flOptions |= WTF_DATE_ALT;    break;
        case 3:  settings.flOptions |= WTF_DATE_CUSTOM; break;
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

    // country/region names are stored in the clockData structure,
    // so save those before we overwrite it
    UniStrcpy( settings.uzTZCtry, pConfig->clockData.uzTZCtry );
    UniStrcpy( settings.uzTZRegn, pConfig->clockData.uzTZRegn );

    // geographic coordinates
    memcpy( &(settings.coordinates), &(pConfig->clockData.coordinates), sizeof(GEOCOORD) );
    if ( WinQueryButtonCheckstate( hwnd, IDD_USECOORDINATES ) != 0 )
        settings.flOptions |= WTF_PLACE_HAVECOORD;
    else
        settings.flOptions &= ~WTF_PLACE_HAVECOORD;

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

    // font presentation parameter
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


/* ------------------------------------------------------------------------- *
 * TZDlgProc                                                                 *
 *                                                                           *
 * Dialog procedure for the TZ configuration dialog.                         *
 * ------------------------------------------------------------------------- */
MRESULT EXPENTRY TZDlgProc( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 )
{
    static HINI    hTZDB = NULLHANDLE;
    static PTZPROP pProps;

    ULONG    cb,
             len;
    CHAR     achLang[ 4 ] = {0},
             achDotLang[ 5 ] = {0},
             achProfile[ PROFILE_MAXZ ] = {0},
             achCountry[ COUNTRY_MAXZ ] = {0};
    UniChar  aucCountry[ COUNTRY_MAXZ ] = {0};
    PUCHAR   pbuf;
    LONG     lVal;
    SHORT    sIdx,
             sCount;
    PSZ      pszData, pszDataCopy,
             pszTZ,
             pszCoord;
    GEOCOORD gc;
    BOOL     fGotDB = FALSE;


    switch ( msg ) {

        case WM_INITDLG:
            pProps = (PTZPROP) mp2;     // pointer to configuration structure

            // Set the spinbutton bounds and default values
            WinSendDlgItemMsg( hwnd, IDD_TZLAT_DEGS, SPBM_SETLIMITS,
                               MPFROMLONG( 90 ), MPFROMLONG( -90 ));
            WinSendDlgItemMsg( hwnd, IDD_TZLAT_MINS, SPBM_SETLIMITS,
                               MPFROMLONG( 60 ), MPFROMLONG( 0 ));
            WinSendDlgItemMsg( hwnd, IDD_TZLONG_DEGS, SPBM_SETLIMITS,
                               MPFROMLONG( 180 ), MPFROMLONG( -180 ));
            WinSendDlgItemMsg( hwnd, IDD_TZLONG_MINS, SPBM_SETLIMITS,
                               MPFROMLONG( 60 ), MPFROMLONG( 0 ));

            // Open the TZ profile and get all application names
            if ( ! WinLoadString( pProps->hab, 0, IDS_LANG, sizeof(achLang)-1, achLang ))
                strcpy( achLang, "EN");
            sprintf( achDotLang, ".%s", achLang );
            sprintf( achProfile, "%s.%s", ZONE_FILE, achLang );
            hTZDB = PrfOpenProfile( pProps->hab, achProfile );
            if ( hTZDB &&
                 PrfQueryProfileSize( hTZDB, NULL, NULL, &cb ) && cb )
            {
                // TODO move this into a function, ugh
                pbuf = (PUCHAR) calloc( cb, sizeof(BYTE) );
                if ( pbuf ) {
                    if ( PrfQueryProfileData( hTZDB, NULL, NULL, pbuf, &cb ) && cb )
                    {
                        PSZ pszApp = (PSZ) pbuf;    // Current application name

                        fGotDB = TRUE;
                        do {
                            // See if this is a country code (two-letter acronym)
                            if ((( len = strlen( pszApp )) == 2 ) &&
                                ( strchr( pszApp, '/') == NULL ))
                            {
                                /* Query the full country name from the .<language> key
                                 * (note we pass pszApp as the default, so if the query
                                 * fails, the country name will simply be the country code)
                                 */
                                PrfQueryProfileString( hTZDB, pszApp, achDotLang, pszApp,
                                                       (PVOID) achCountry, COUNTRY_MAXZ );

                                // Convert country name from UTF-8 to the current codepage
                                if ( pProps->uconv && pProps->uconv1208 &&
                                     !UniStrToUcs( pProps->uconv1208, aucCountry, achCountry, COUNTRY_MAXZ ))
                                {
                                    // If this fails, achCountry will retain the original string
                                    UniStrFromUcs( pProps->uconv, achCountry, aucCountry, COUNTRY_MAXZ );
                                }

                                // Add country name to listbox
                                sIdx = (SHORT) WinSendDlgItemMsg( hwnd, IDD_TZCOUNTRY, LM_INSERTITEM,
                                                                  MPFROMSHORT( LIT_SORTASCENDING ), MPFROMP( achCountry ));
                                // Set the list item handle to the country code
                                WinSendDlgItemMsg( hwnd, IDD_TZCOUNTRY, LM_SETITEMHANDLE,
                                                   MPFROMSHORT( sIdx ), MPFROMP( strdup( pszApp )));
                             }
                            pszApp += len + 1;
                        } while ( len );
                    }
                    free( pbuf );
                }
            }
            else {
                ErrorMessage( hwnd, IDS_ERROR_ZONEINFO );
            }

            // Try to select the saved city, if any
            if ( fGotDB ) {
                sIdx = LIT_NONE;
                if ( pProps->achCtry[0] )
                    sIdx = (SHORT) \
                               WinSendDlgItemMsg( hwnd, IDD_TZCOUNTRY, LM_SEARCHSTRING,
                                                  MPFROM2SHORT( LSS_PREFIX, LIT_FIRST ),
                                                  MPFROMP( pProps->achCtry ));
                // Failing that, do a search on the description
                if (( sIdx == LIT_NONE ) || ( sIdx == LIT_ERROR ))
                    sIdx = (SHORT) \
                               WinSendDlgItemMsg( hwnd, IDD_TZCOUNTRY, LM_SEARCHSTRING,
                                                  MPFROM2SHORT( LSS_PREFIX, LIT_FIRST ),
                                                  MPFROMP( pProps->achDesc ));
                WinSendDlgItemMsg( hwnd, IDD_TZCOUNTRY, LM_SELECTITEM,
                                   MPFROMSHORT( sIdx ), MPFROMSHORT( TRUE ));
            }

            /* Set the current TZ string value (we do this after selecting the
             * default country+zone so that the selection event doesn't reset
             * the value to the default for that zone - in case it's been
             * customized).
             */
            WinSetDlgItemText( hwnd, IDD_TZVALUE, pProps->achTZ );

            // Ditto the coordinates
            // TODO only if WTF_PLACE_HAVECOORD is 0 (otherwise do above)
            WinSendDlgItemMsg( hwnd, IDD_TZLAT_DEGS, SPBM_SETCURRENTVALUE,
                               MPFROMLONG( (LONG)(pProps->coordinates.sLatitude) ), 0L );
            WinSendDlgItemMsg( hwnd, IDD_TZLAT_MINS, SPBM_SETCURRENTVALUE,
                               MPFROMLONG( (LONG)(pProps->coordinates.bLaMin) ), 0L );
            WinSendDlgItemMsg( hwnd, IDD_TZLONG_DEGS, SPBM_SETCURRENTVALUE,
                               MPFROMLONG( (LONG)(pProps->coordinates.sLongitude) ), 0L );
            WinSendDlgItemMsg( hwnd, IDD_TZLONG_MINS, SPBM_SETCURRENTVALUE,
                               MPFROMLONG( (LONG)(pProps->coordinates.bLoMin) ), 0L );

            CentreWindow( hwnd );
            break;


        case WM_COMMAND:
            switch( SHORT1FROMMP( mp1 )) {
                case DID_OK:
                    WinQueryDlgItemText( hwnd, IDD_TZVALUE, LOCDESC_MAXZ, pProps->achTZ );
                    WinQueryDlgItemText( hwnd, IDD_TZCOUNTRY, COUNTRY_MAXZ, pProps->achCtry );
                    WinQueryDlgItemText( hwnd, IDD_TZNAME, REGION_MAXZ, pProps->achRegn );

                    WinSendDlgItemMsg( hwnd, IDD_TZLAT_DEGS, SPBM_QUERYVALUE,
                                       MPFROMP( &lVal ), MPFROM2SHORT( 0, SPBQ_UPDATEIFVALID ));
                    pProps->coordinates.sLatitude = lVal;
                    WinSendDlgItemMsg( hwnd, IDD_TZLAT_MINS, SPBM_QUERYVALUE,
                                       MPFROMP( &lVal ), MPFROM2SHORT( 0, SPBQ_UPDATEIFVALID ));
                    pProps->coordinates.bLaMin = lVal;
                    WinSendDlgItemMsg( hwnd, IDD_TZLONG_DEGS, SPBM_QUERYVALUE,
                                       MPFROMP( &lVal ), MPFROM2SHORT( 0, SPBQ_UPDATEIFVALID ));
                    pProps->coordinates.sLongitude = lVal;
                    WinSendDlgItemMsg( hwnd, IDD_TZLONG_MINS, SPBM_QUERYVALUE,
                                       MPFROMP( &lVal ), MPFROM2SHORT( 0, SPBQ_UPDATEIFVALID ));
                    pProps->coordinates.bLoMin = lVal;
                    break;
            }
            break;
            // WM_COMMAND

        case WM_CONTROL:
            switch(SHORT1FROMMP(mp1))
            {

                case IDD_TZCOUNTRY:
                    // New country selected
                    if ( SHORT2FROMMP(mp1) == LN_SELECT ) {

                        // Clear the timezone list
                        sCount = (SHORT) WinSendDlgItemMsg( hwnd, IDD_TZNAME,
                                                            LM_QUERYITEMCOUNT,
                                                            0L, 0L );
                        for ( sIdx = 0; sIdx < sCount; sIdx++ ) {
                            pszData = (PSZ) WinSendDlgItemMsg( hwnd, IDD_TZNAME,
                                                               LM_QUERYITEMHANDLE,
                                                               MPFROMSHORT( sIdx ),
                                                               0L );
                            if ( pszData ) free( pszData );
                        }
                        WinSendDlgItemMsg( hwnd, IDD_TZNAME, LM_DELETEALL, 0L, 0L );

                        // Get the new country value
                        sIdx = (SHORT) WinSendDlgItemMsg( hwnd, IDD_TZCOUNTRY,
                                                          LM_QUERYSELECTION,
                                                          MPFROMSHORT( LIT_FIRST ),
                                                          0L );
                        pszData = (PSZ) WinSendDlgItemMsg( hwnd, IDD_TZCOUNTRY,
                                                           LM_QUERYITEMHANDLE,
                                                           MPFROMSHORT( sIdx ),
                                                           0L );
                        if ( pszData ) {
                            // Repopulate the timezone list for the new country
                            TZPopulateCountryZones( hwnd, hTZDB, pszData, pProps );

                            // Try to select the saved region, if any
                            if ( pProps->achRegn[0] )
                                sIdx = (SHORT) \
                                           WinSendDlgItemMsg( hwnd, IDD_TZNAME, LM_SEARCHSTRING,
                                                              MPFROM2SHORT( LSS_PREFIX, LIT_FIRST ),
                                                              MPFROMP( pProps->achRegn ));
                            // Failing that, do a substring search on the description
                            if (( sIdx == LIT_NONE ) || ( sIdx == LIT_ERROR ))
                                sIdx = (SHORT) \
                                           WinSendDlgItemMsg( hwnd, IDD_TZNAME, LM_SEARCHSTRING,
                                                              MPFROM2SHORT( LSS_SUBSTRING, LIT_FIRST ),
                                                              MPFROMP( pProps->achDesc ));
                            WinSendDlgItemMsg( hwnd, IDD_TZNAME, LM_SELECTITEM,
                                               MPFROMSHORT( sIdx ), MPFROMSHORT( TRUE ));
                        }
                    }
                    break;
                    // IDD_TZCOUNTRY


                case IDD_TZNAME:
                    // New timezone selected
                    if ( SHORT2FROMMP(mp1) == LN_SELECT ) {
                        // Get the selected timezone and populate the data fields
                        sIdx = (SHORT) WinSendDlgItemMsg( hwnd, IDD_TZNAME, LM_QUERYSELECTION,
                                                          MPFROMSHORT( LIT_FIRST ), 0L );
                        pszData = (PSZ) WinSendDlgItemMsg( hwnd, IDD_TZNAME, LM_QUERYITEMHANDLE,
                                                           MPFROMSHORT( sIdx ), 0L);
                        if ( pszData && (( pszDataCopy = strdup( pszData )) != NULL )) {
                            _PmpfF(("Item handle data: %s", pszDataCopy ));
                            pszTZ = strtok( pszDataCopy, "\t");
                            pszCoord = strtok( NULL, "\t");
                            memset( &gc, 0, sizeof( gc ));
                            if ( pszCoord )
                                ParseTZCoordinates( pszCoord, &gc );
                            else
                                _PmpfF((" --> Unable to parse coordinates!"));
                            // set coordinate values
                            WinSendDlgItemMsg( hwnd, IDD_TZLAT_DEGS, SPBM_SETCURRENTVALUE,
                                               MPFROMLONG( (LONG)gc.sLatitude ), 0L );
                            WinSendDlgItemMsg( hwnd, IDD_TZLAT_MINS, SPBM_SETCURRENTVALUE,
                                               MPFROMLONG( (LONG)gc.bLaMin ), 0L );
                            WinSendDlgItemMsg( hwnd, IDD_TZLONG_DEGS, SPBM_SETCURRENTVALUE,
                                               MPFROMLONG( (LONG)gc.sLongitude ), 0L );
                            WinSendDlgItemMsg( hwnd, IDD_TZLONG_MINS, SPBM_SETCURRENTVALUE,
                                               MPFROMLONG( (LONG)gc.bLoMin ), 0L );
                            // set TZ value
                            WinSetDlgItemText( hwnd, IDD_TZVALUE, pszTZ );
                            free( pszDataCopy );
                        }
                    }
                    break;
                    // IDD_TZNAME

                default: break;
            }
            break;


        case WM_DESTROY:
            // Close the zone info db
            if ( hTZDB )
                PrfCloseProfile( hTZDB );

            // Free the country list itemdata values
            sCount = (SHORT) WinSendDlgItemMsg( hwnd, IDD_TZCOUNTRY,
                                                LM_QUERYITEMCOUNT, 0L, 0L );
            for ( sIdx = 0; sIdx < sCount; sIdx++ ) {
                pszData = (PSZ) WinSendDlgItemMsg( hwnd, IDD_TZCOUNTRY,
                                                   LM_QUERYITEMHANDLE,
                                                   MPFROMSHORT( sIdx ), 0L );
                if ( pszData ) free( pszData );
            }

            // Free the timezone list itemdata values
            sCount = (SHORT) WinSendDlgItemMsg( hwnd, IDD_TZNAME,
                                                LM_QUERYITEMCOUNT, 0L, 0L );
            for ( sIdx = 0; sIdx < sCount; sIdx++ ) {
                pszData = (PSZ) WinSendDlgItemMsg( hwnd, IDD_TZNAME,
                                                   LM_QUERYITEMHANDLE,
                                                   MPFROMSHORT( sIdx ), 0L );
                if ( pszData ) free( pszData );
            }

            break;

        default: break;
    }

    return WinDefDlgProc( hwnd, msg, mp1, mp2 );
}


/* ------------------------------------------------------------------------- *
 * SelectTimeZone                                                            *
 *                                                                           *
 * Pop up the timezone selection dialog.                                     *
 *                                                                           *
 * Parameters:                                                               *
 *   HWND hwnd        : handle of the current window.                        *
 *   PUCLKPROP pConfig: pointer to the current clock properties.             *
 *                                                                           *
 * RETURNS: N/A                                                              *
 * ------------------------------------------------------------------------- */
void SelectTimeZone( HWND hwnd, PUCLKPROP pConfig )
{
    TZPROP tzp = {0};
    CHAR   achCoord[ ISO6709_MAXZ ] = {0};

    // Populate the settings structure for passing data to/from the dialog
    tzp.hab = pConfig->hab;
    tzp.hmq = pConfig->hmq;
    tzp.uconv = pConfig->uconv;
    tzp.achCtry[0] = '\0';
    tzp.achRegn[0] = '\0';

    memcpy( &(tzp.coordinates), &(pConfig->clockData.coordinates),
            sizeof(GEOCOORD) );

    if ( UniCreateUconvObject( (UniChar *) L"IBM-1208@map=display,path=no",
                               &(tzp.uconv1208) ) != 0 )
        tzp.uconv1208 = NULL;

    // Get the current values
    WinQueryDlgItemText( hwnd, IDD_CITYLIST, LOCDESC_MAXZ, tzp.achDesc );
    WinQueryDlgItemText( hwnd, IDD_TZDISPLAY, LOCDESC_MAXZ, tzp.achTZ );

    if ( pConfig->clockData.uzTZCtry[0] )
        UniStrFromUcs( tzp.uconv, tzp.achCtry,
                       pConfig->clockData.uzTZCtry, sizeof(tzp.achCtry)-1 );
    if ( pConfig->clockData.uzTZRegn[0] )
        UniStrFromUcs( tzp.uconv, tzp.achRegn,
                       pConfig->clockData.uzTZRegn, sizeof(tzp.achRegn)-1 );

    // Show & process the dialog
    WinDlgBox( HWND_DESKTOP, hwnd, (PFNWP) TZDlgProc, 0, IDD_TIMEZONE, &tzp );

    // Update the settings
    WinSetDlgItemText( hwnd, IDD_TZDISPLAY, tzp.achTZ );
    memcpy( &(pConfig->clockData.coordinates), &(tzp.coordinates),
            sizeof(GEOCOORD) );
    UnParseTZCoordinates( achCoord, pConfig->clockData.coordinates );
    WinSetDlgItemText( hwnd, IDD_COORDINATES, achCoord );

    if ( tzp.achCtry[0] )
        UniStrToUcs( tzp.uconv, pConfig->clockData.uzTZCtry,
                     tzp.achCtry, COUNTRY_MAXZ );
    if ( tzp.achRegn[0] )
        UniStrToUcs( tzp.uconv, pConfig->clockData.uzTZRegn,
                     tzp.achRegn, REGION_MAXZ );

    if ( tzp.uconv1208 ) UniFreeUconvObject( tzp.uconv1208 );
}


/* ------------------------------------------------------------------------- *
 * TZPopulateCountryZones                                                    *
 *                                                                           *
 *                                                                           *
 * Parameters:                                                               *
 *   HWND     hwnd   : handle of the current window.                         *
 *   HINI     hTZDB  : handle to already-open TZ data INI file.              *
 *   PSZ      pszCtry: country code.                                         *
 *   PTZPROPS pProps : pointer to custom window data                         *
 *                                                                           *
 * RETURNS: N/A                                                              *
 * ------------------------------------------------------------------------- */
void TZPopulateCountryZones( HWND hwnd, HINI hTZDB, PSZ pszCtry, PTZPROP pProps )
{
    PUCHAR  pkeys;
    CHAR    achZone[ REGION_MAXZ ],
            achTZ[ TZSTR_MAXZ ];
    UniChar aucZone[ REGION_MAXZ ];
    PSZ     pszName,
            pszValue;
    ULONG   cb,
            len;
    SHORT   sIdx;

    if ( hTZDB == NULLHANDLE ) return;

    // Get all keys in pszCtry
    if ( PrfQueryProfileSize( hTZDB, pszCtry, NULL, &cb ) && cb ) {
        pkeys = (PUCHAR) calloc( cb, sizeof(BYTE) );
        if ( pkeys ) {
            if ( PrfQueryProfileData( hTZDB, pszCtry, NULL, pkeys, &cb ) && cb )
            {
                // TODO This should really go into a separate function...
                PSZ pszKey = (PSZ) pkeys;
                do {
                    len = strlen( pszKey );
                    // Query the value of each key that doesn't start with '.'
                    if ( len && ( *pszKey != '.')) {
                        pszName = pszKey;

                        // Now 'pszName' should contain the tzdata zone name.

                        /* -- For now, just use the original tzdata zone names.
                        // Get the translated & localized region name (achZone)
                        PrfQueryProfileString( hTZDB, pszCtry, pszName, "Unknown",
                                               (PVOID) achZone, REGION_MAXZ );
                        */

                        // Skip past any leading '*' (we don't use this flag)
                        if (( len > 1 ) && ( pszName[0] == '*')) {
                            pszName++;
                        }
                        strncpy( achZone, pszName, sizeof(achZone)-1 );

                        /* Next, get the TZ string and geo coordinates for this zone.
                         * These are found in a top-level application under the name
                         * of the tzdata zone.
                         */

                        // Get the TZ variable associated with this zone
                        PrfQueryProfileString( hTZDB, pszName, "TZ", "",
                                               (PVOID) achTZ, TZSTR_MAXZ );

                        // Filter out zones with no TZ variable
                        if ( strlen( achZone ) == 0 ) continue;

                        // Zone name is UTF-8, so convert it to the current codepage
                        if ( pProps->uconv && pProps->uconv1208 &&
                             !UniStrToUcs( pProps->uconv1208, aucZone, achZone, REGION_MAXZ ))
                        {
                            // If this fails, achZone will retain the original string
                            UniStrFromUcs( pProps->uconv, achZone, aucZone, REGION_MAXZ );
                        }

                        // Add zone name to listbox
                        /* TODO: If we ever switch to using the localized region
                         * names, then what we should do is construct a list of
                         * (unique) names to keep. Then (after the loop), if the
                         * list is non-empty we populate the name dropdown;
                         * otherwise disable it.
                         */
                        sIdx = (SHORT) \
                                WinSendDlgItemMsg( hwnd, IDD_TZNAME, LM_INSERTITEM,
                                                   MPFROMSHORT( LIT_SORTASCENDING ),
                                                   MPFROMP( achZone ));

                        // Allocate a string for the item data. This will be
                        // freed when the list is cleared (in TZDlgProc).
                        pszValue = malloc( TZDATA_MAXZ );

                        // Save the TZ variable string and get the coordinates
                        strncpy( pszValue, achTZ, TZSTR_MAXZ );
                        PrfQueryProfileString( hTZDB, pszName, "Coordinates", "+0+0",
                                               (PVOID) achZone, sizeof(achZone)-1 );

                        // Concatenate the two together separated by tab
                        if (( strlen( pszValue ) + strlen( achZone ) + 1 ) < TZDATA_MAXZ ) {
                            strcat( pszValue, "\t");
                            strncat( pszValue, achZone, TZDATA_MAXZ-1 );
                        }

                        // Save the TZ+coordinates string in the item data handle
                        WinSendDlgItemMsg( hwnd, IDD_TZNAME, LM_SETITEMHANDLE,
                                           MPFROMSHORT( sIdx ),
                                           MPFROMP( pszValue ));
                    }
                    pszKey += len + 1;
                } while ( len );

                WinSendDlgItemMsg( hwnd, IDD_TZNAME, LM_SELECTITEM,
                                   MPFROMSHORT( 0 ), MPFROMSHORT( TRUE ));

            }
            free( pkeys );
        }
    }
}


/* ------------------------------------------------------------------------- *
 * ParseTZCoordinates                                                        *
 *                                                                           *
 * Parse geographic coordinates in the ISO 6709 format used by the tzdata    *
 * database.                                                                 *
 *                                                                           *
 * Parameters:                                                               *
 *   PSZ       pszCoord: String containing the ISO 6709 coordinates.     (I) *
 *   PGEOCOORD pGCB    : UClocl geographic coordinates structure.        (O) *
 *                                                                           *
 * RETURNS: BOOL                                                             *
 *   TRUE if parsing was successful, FALSE otherwise.                        *
 * ------------------------------------------------------------------------- */
BOOL ParseTZCoordinates( PSZ pszCoord, PGEOCOORD pGC )
{
    PSZ p = pszCoord;
    int iDeg;
    int iMin;

    if ( !p || ( *p == 0 )) return FALSE;

    // Format is [+|-]DDMM[+|-]DDDMM or [+|-]DDMMSS[+|-]DDDMMSS
    if (( *p != '+') && ( *p != '-')) return FALSE;
    if ( sscanf( p, "%3d%2d", &iDeg, &iMin ) != 2 ) return FALSE;

    pGC->sLatitude = (SHORT) iDeg;
    pGC->bLaMin = (BYTE) iMin;
    do {
        p++;
    } while ( *p && ( *p != '+') && ( *p != '-') && (( p - pszCoord ) <= 16 ));
    if ( *p == 0 ) return FALSE;
    if ( sscanf( p, "%4d%2d", &iDeg, &iMin ) != 2 ) return FALSE;

    pGC->sLongitude = (SHORT) iDeg;
    pGC->bLoMin = (BYTE) iMin;
    return TRUE;
}


/* ------------------------------------------------------------------------- *
 * UnParseTZCoordinates                                                      *
 *                                                                           *
 * Generate geographic coordinates in ISO 6709 format from the contents of a *
 * GEOCOORD structure.                                                       *
 *                                                                           *
 * Parameters:                                                               *
 *   PSZ      pszCoord   : String buffer to contain the ISO 6709 coordinates.*
 *                         must be at least 16 bytes (preallocated).     (I) *
 *   GEOCOORD coordinates: UClock geographic coordinates structure.      (O) *
 *                                                                           *
 * RETURNS: BOOL                                                             *
 *   TRUE if parsing was successful, FALSE otherwise.                        *
 * ------------------------------------------------------------------------- */
void UnParseTZCoordinates( PSZ pszCoord, GEOCOORD coordinates )
{
    int iLaDeg = coordinates.sLatitude,
        iLaMin = coordinates.bLaMin,
        iLoDeg = coordinates.sLongitude,
        iLoMin = coordinates.bLoMin;

    sprintf( pszCoord, "%c%02d%02d%c%03d%02d",
             ( iLaDeg < 0 )? '-': '+', abs( iLaDeg ), iLaMin,
             ( iLoDeg < 0 )? '-': '+', abs( iLoDeg ), iLoMin );
}

