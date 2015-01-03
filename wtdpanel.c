#include "uclock.h"

// Internal defines
#define FTYPE_NOTFOUND 0    // font does not exist
#define FTYPE_BITMAP   1    // font is a bitmap font (always non-Unicode)
#define FTYPE_UNICODE  2    // font is a Unicode-capable outline font
#define FTYPE_NOTUNI   3    // font is a non-Unicode outline font

#define PRF_NTLDATA_MAXZ    32  // max. length of a data item in PM_(Default)_National

// Internal function prototypes
void Paint_DefaultView( HWND hwnd, HPS hps, RECTL rcl, LONG clrBG, LONG clrFG, LONG clrBor, PWTDDATA pData );
void Paint_CompactView( HWND hwnd, HPS hps, RECTL rcl, LONG clrBG, LONG clrFG, LONG clrBor, PWTDDATA pdata );
BYTE GetFontType( HPS hps, PSZ pszFontFace, PFATTRS pfAttrs, LONG lCY, BOOL bH );
void SetFormatStrings( PWTDDATA pdata );
void FormatTime( HWND hwnd, PWTDDATA pdata, struct tm *time );
void FormatDate( HWND hwnd, PWTDDATA pdata, struct tm *time );


/* ------------------------------------------------------------------------- *
 * Window procedure for the WTDisplayPanel control.                          *
 * ------------------------------------------------------------------------- */
MRESULT EXPENTRY WTDisplayProc( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 )
{
    PCREATESTRUCT pCreate;              // pointer to WM_CREATE data
    PWTDCDATA   pinit;                  // pointer to initialization data
    PWTDDATA    pdata;                  // pointer to control data
    HPS         hps;                    // control's presentation space
    RECTL       rcl;                    // control's window area
    POINTL      ptl;                    // current drawing position
    POINTS      pts;                    // current pointer position
    CHAR        szEnv[ TZSTR_MAXZ+4 ] = {0};  // used to write TZ environment
    LONG        clrBG, clrFG, clrBor;   // current fore/back/border colours
    ULONG       ulid,
                flNewOpts,              // control options mask
                rc;
    struct tm  *ltime;                  // local time structure
    time_t      ctime;                  // passed time value
    UniChar    *puzPlaceName;           // passed place name
    PSZ         pszTZ,                  // passed TZ variable
                pszLocaleName;          // passed locale name
//FILE        *dbglog;

    switch( msg ) {

        case WM_CREATE:
            // initialize the private control data structure
            if ( !( pdata = (PWTDDATA) malloc( sizeof(WTDDATA) ))) return (MRESULT) TRUE;
            memset( pdata, 0, sizeof(WTDDATA) );
            WinSetWindowPtr( hwnd, 0, pdata );

            pCreate = (PCREATESTRUCT) mp2;

            // create a conversion object for the current codepage
            rc = UniCreateUconvObject( (UniChar *) L"@map=display,path=no,subuni=\\x003F", &(pdata->uconv) );
            if ( rc != ULS_SUCCESS ) return (MRESULT) TRUE;

            // get the control initialization data
            if ( mp1 ) {
                pinit = (PWTDCDATA) mp1;
                pdata->flOptions = pinit->flOptions;
                pdata->bSep = pinit->bSep;
                pdata->coordinates = pinit->coordinates;
                strncpy( pdata->szTZ, pinit->szTZ, TZSTR_MAXZ-1 );
                UniStrcpy( pdata->uzDesc, pinit->uzDesc );
                UniStrFromUcs( pdata->uconv, pdata->szDesc,
                               pinit->uzDesc, LOCDESC_MAXZ );                    ;
                UniStrcpy( pdata->uzTimeFmt, pinit->uzTimeFmt );
                UniStrcpy( pdata->uzDateFmt, pinit->uzDateFmt );
                if ( pinit->szLocale[0] ) {
                    rc = UniCreateLocaleObject( UNI_MBS_STRING_POINTER, pinit->szLocale, &(pdata->locale) );
                    if ( rc != ULS_SUCCESS )
                        rc = UniCreateLocaleObject( UNI_MBS_STRING_POINTER, "", &(pdata->locale) );
                    if ( rc != ULS_SUCCESS )
                        pdata->locale = NULL;
                } else {
                    rc = UniCreateLocaleObject( UNI_MBS_STRING_POINTER, "", &(pdata->locale) );
                    if ( rc != ULS_SUCCESS ) pdata->locale = NULL;
                }
            }
            else {
                // set a default locale
                rc = UniCreateLocaleObject( UNI_MBS_STRING_POINTER, "", &(pdata->locale) );
                if ( rc != ULS_SUCCESS) pdata->locale = NULL;
            }
            if ( !pdata->uzTimeFmt[0] ) {
                strncpy( pdata->szDesc, pCreate->pszText, LOCDESC_MAXZ-1 );
                UniStrToUcs( pdata->uconv, pdata->uzDesc, pdata->szDesc, LOCDESC_MAXZ );
            }
            if ( ! pdata->locale ) return (MRESULT) TRUE;
            SetFormatStrings( pdata );

            return (MRESULT) FALSE;


        case WM_DESTROY:
            if (( pdata = WinQueryWindowPtr( hwnd, 0 )) != NULL ) {
                if ( pdata->locale ) UniFreeLocaleObject( pdata->locale );
                if ( pdata->uconv  ) UniFreeUconvObject(  pdata->uconv  );
                free( pdata );
            }
            break;


        // pass this message up to the owner
        case WM_BEGINDRAG:
            pdata = WinQueryWindowPtr( hwnd, 0 );
            ptl.x = SHORT1FROMMP( mp1 );
            ptl.y = SHORT2FROMMP( mp1 );
            WinMapWindowPoints( hwnd, WinQueryWindow(hwnd, QW_OWNER), &ptl, 1 );
            pts.x = ptl.x;
            pts.y = ptl.y;
            WinPostMsg( WinQueryWindow(hwnd, QW_OWNER),
                        WM_BEGINDRAG, MPFROM2SHORT(pts.x, pts.y), mp2 );
            break;


        case WM_BUTTON1CLICK:
            pdata = WinQueryWindowPtr( hwnd, 0 );
            ptl.x = SHORT1FROMMP( mp1 );
            ptl.y = SHORT2FROMMP( mp1 );
            if ( pdata->flOptions & WTF_MODE_COMPACT ) {
                if ( WinPtInRect( WinQueryAnchorBlock(hwnd), &(pdata->rclDate), &ptl )) {
                    if ( pdata->flState & WTS_CVR_DATE )
                        pdata->flState &= ~WTS_CVR_DATE;
                    else
                        pdata->flState |= WTS_CVR_DATE;
                    // todo cycle through these and WTS_CVR_SUNTIME (and alt. date?)
                    WinInvalidateRect( hwnd, &(pdata->rclDate), FALSE );
                }
            }
            return (MRESULT) TRUE;


        // set the 'menu active' flag then pass this message up to the owner
        case WM_CONTEXTMENU:
            pdata = WinQueryWindowPtr( hwnd, 0 );
            pdata->flState |= WTS_GUI_MENUPOPUP;
            WinInvalidateRect(hwnd, NULL, FALSE);
            ptl.x = SHORT1FROMMP( mp1 );
            ptl.y = SHORT2FROMMP( mp1 );
            WinMapWindowPoints( hwnd, WinQueryWindow(hwnd, QW_OWNER), &ptl, 1 );
            pts.x = ptl.x;
            pts.y = ptl.y;
            WinPostMsg( WinQueryWindow(hwnd, QW_OWNER),
                        WM_CONTEXTMENU, MPFROM2SHORT(pts.x, pts.y), mp2 );
            break;


        // clear the 'menu active' flag
        case WM_MENUEND:
            pdata = WinQueryWindowPtr( hwnd, 0 );
            pdata->flState &= ~WTS_GUI_MENUPOPUP;
            WinInvalidateRect( hwnd, NULL, FALSE );
            break;


        case WM_PAINT:
            pdata = WinQueryWindowPtr( hwnd, 0 );
            hps = WinBeginPaint( hwnd, NULLHANDLE, NULLHANDLE );

            GpiSetLineType( hps, LINETYPE_DEFAULT );

            // switch to RGB colour mode
            GpiCreateLogColorTable( hps, LCOL_RESET, LCOLF_RGB, 0, 0, NULL );

            // get the current colours
            rc = WinQueryPresParam( hwnd, PP_FOREGROUNDCOLOR, PP_FOREGROUNDCOLORINDEX,
                                    &ulid, sizeof(clrFG), &clrFG, QPF_ID2COLORINDEX );
            if ( !rc ) clrFG = SYSCLR_WINDOWTEXT;

            rc = WinQueryPresParam( hwnd, PP_BACKGROUNDCOLOR, PP_BACKGROUNDCOLORINDEX,
                                    &ulid, sizeof(clrBG), &clrBG, QPF_ID2COLORINDEX );
            if ( !rc ) clrBG = SYSCLR_WINDOW;

            rc = WinQueryPresParam( hwnd, PP_BORDERCOLOR, 0, &ulid,
                                    sizeof(clrBor), &clrBor, 0 );
            if ( !rc ) clrBor = clrFG;

            WinQueryWindowRect( hwnd, &rcl );

            // paint the window contents according to the selected view mode
            GpiSetColor( hps, clrFG );
            if ( pdata->flOptions & WTF_MODE_LARGE )
                ;// todo
            else if ( pdata->flOptions & WTF_MODE_COMPACT )
                Paint_CompactView( hwnd, hps, rcl, clrBG, clrFG, clrBor, pdata );
            else
                Paint_DefaultView( hwnd, hps, rcl, clrBG, clrFG, clrBor, pdata );

            // paint the borders, if applicable
            GpiSetColor( hps, clrBor );
            if ( pdata->flOptions & WTF_BORDER_LEFT ) {
                ptl.x = 0; ptl.y = 0;
                GpiMove( hps, &ptl );
                ptl.x = 0; ptl.y = rcl.yTop-1;
                GpiLine( hps, &ptl );
                rcl.xLeft += 1;
            }
            if ( pdata->flOptions & WTF_BORDER_RIGHT ) {
                ptl.x = rcl.xRight-1; ptl.y = 0;
                GpiMove( hps, &ptl );
                ptl.x = rcl.xRight-1; ptl.y = rcl.yTop-1;
                GpiLine( hps, &ptl );
                rcl.xRight -= 1;
            }
            if ( pdata->flOptions & WTF_BORDER_BOTTOM ) {
                ptl.x = 0; ptl.y = 0;
                GpiMove( hps, &ptl );
                ptl.x = rcl.xRight-1; ptl.y = 0;
                GpiLine( hps, &ptl );
                rcl.yBottom += 1;
            }
            if ( pdata->flOptions & WTF_BORDER_TOP ) {
                ptl.x = 0; ptl.y = rcl.yTop-1;
                GpiMove( hps, &ptl );
                ptl.x = rcl.xRight-1; ptl.y = rcl.yTop-1;
                GpiLine( hps, &ptl );
                rcl.yTop -= 1;
            }

            // draw the popup menu indication border, if applicable
            if ( pdata->flState & WTS_GUI_MENUPOPUP ) {
                GpiSetColor( hps, clrFG );
                GpiSetLineType( hps, LINETYPE_DOT );
                ptl.x = 2; ptl.y = 2;
                GpiMove( hps, &ptl );
                ptl.x = rcl.xRight - 2; ptl.y = rcl.yTop - 2;
                GpiBox( hps, DRO_OUTLINE, &ptl, 10, 10 );
            }

            WinEndPaint( hps );
            return (MRESULT) 0;


        case WM_PRESPARAMCHANGED:
            switch ( (ULONG) mp1 ) {
                case PP_FONTNAMESIZE:
                case PP_BACKGROUNDCOLOR:
                case PP_BACKGROUNDCOLORINDEX:
                case PP_FOREGROUNDCOLOR:
                case PP_FOREGROUNDCOLORINDEX:
                case PP_BORDERCOLOR:
                    WinInvalidateRect(hwnd, NULL, FALSE);
                    break;
                case PP_SEPARATORCOLOR:
                    pdata = WinQueryWindowPtr( hwnd, 0 );
                    WinInvalidateRect( hwnd, &(pdata->rclDesc), FALSE );
                    break;
                default: break;
            }
            break;


        // --------------------------------------------------------------------
        // Custom messages defined for this control
        //

        /* .................................................................. *
         * WTD_SETDATETIME                                                    *
         *                                                                    *
         *   - mp1 (time_t)      - New calendar time value (in UTC)           *
         *   - mp2 (not used)                                                 *
         *                                                                    *
         * Update the current time in this clock display.  (Note: this        *
         * message will normally be sent multiple times per second, so we     *
         * should try and keep it as efficient as possible.)                  *
         *                                                                    *
         * .................................................................. */
        case WTD_SETDATETIME:   // mp1 == time_t
            pdata = WinQueryWindowPtr( hwnd, 0 );
            // if the time hasn't changed, do nothing
            if ( mp1 && (time_t) mp1 != pdata->timeval ) {
                ctime = (time_t) mp1;

                // calculate the local time for this timezone
                if ( pdata->szTZ[0] ) {
                    sprintf( szEnv, "TZ=%s", pdata->szTZ );
                    putenv( szEnv );
                    tzset();
                    ltime = localtime( &ctime );
                } else
                    ltime = gmtime( &ctime );

                // generate formatted time and date strings
                if ( pdata->locale && ltime ) {
                    FormatTime( hwnd, pdata, ltime );
                    FormatDate( hwnd, pdata, ltime );
/*
                    rc = (ULONG) UniStrftime( pdata->locale, uzFTime,
                                      TIMESTR_MAXZ-1, pdata->uzTimeFmt, ltime );
                    if ( UniStrcmp( uzFTime, pdata->uzTime ) != 0 ) {
                        UniStrcpy( pdata->uzTime, uzFTime );
                        if (( rc = UniStrFromUcs( pdata->uconv, pdata->szTime,
                                                  pdata->uzTime, TIMESTR_MAXZ )) != ULS_SUCCESS )
                            pdata->szTime[0] = 0;
                        WinInvalidateRect( hwnd, &(pdata->rclTime), FALSE );
                    }
                    rc = (ULONG) UniStrftime( pdata->locale, uzFDate,
                                      DATESTR_MAXZ-1, pdata->uzDateFmt, ltime );
                    if ( UniStrcmp( uzFDate, pdata->uzDate ) != 0 ) {
                        UniStrcpy( pdata->uzDate, uzFDate );
                        if (( rc = UniStrFromUcs( pdata->uconv, pdata->szDate,
                                                  pdata->uzDate, DATESTR_MAXZ )) != ULS_SUCCESS )
                            pdata->szDate[0] = 0;
                        WinInvalidateRect( hwnd, &(pdata->rclDate), FALSE );
                    }
*/
                }

            }
            return (MRESULT) 0;


        /* .................................................................. *
         * WTD_SETTIMEZONE                                                    *
         *                                                                    *
         *   - mp1 (PSZ)         - New timezone (formatted TZ variable)       *
         *   - mp2 (UniChar *)   - Human-readable timezone (or city) name     *
         *                                                                    *
         * Set or change the location & timezone used by this clock display.  *
         *                                                                    *
         * .................................................................. */
        case WTD_SETTIMEZONE:
            pdata = WinQueryWindowPtr( hwnd, 0 );
            pszTZ = (PSZ) mp1;
            puzPlaceName = (UniChar *) mp2;
            if ( pszTZ )
                strncpy( pdata->szTZ, pszTZ, TZSTR_MAXZ-1 );
            if ( puzPlaceName ) {
                UniStrncpy( pdata->uzDesc, puzPlaceName, LOCDESC_MAXZ-1 );
                if (( rc = UniStrFromUcs( pdata->uconv, pdata->szDesc,
                                          puzPlaceName, LOCDESC_MAXZ )) != ULS_SUCCESS )
                    pdata->szDesc[0] = 0;
            }
            WinInvalidateRect( hwnd, NULL, FALSE);
            return (MRESULT) 0;


        /* .................................................................. *
         * WTD_SETLOCALE                                                      *
         *                                                                    *
         *   - mp1 (PSZ)         - Locale name (as used by ULS)               *
         *   - mp2 (not used)                                                 *
         *                                                                    *
         * Set or change the formatting locale used for this clock display.   *
         * The locale is used to format the time/date strings for output; it  *
         * is a user preference, and does not necessarily correspond to the   *
         * timezone/location used by the clock itself.                        *
         *                                                                    *
         * .................................................................. */
        case WTD_SETLOCALE:
            pdata = WinQueryWindowPtr( hwnd, 0 );
            pszLocaleName = (PSZ) mp1;
            if ( pszLocaleName ) {
                if ( pdata->locale ) UniFreeLocaleObject( pdata->locale );
                rc = UniCreateLocaleObject( UNI_MBS_STRING_POINTER,
                                            pszLocaleName, &(pdata->locale) );

                // update the UniStrftime format strings
                SetFormatStrings( pdata );

                WinInvalidateRect( hwnd, NULL, FALSE);
            }
            return (MRESULT) 0;


        /* .................................................................. *
         * WTD_SETCOORDINATES                                                 *
         *                                                                    *
         *   - mp1 (USHORT/USHORT) - latitude coordinate (degrees/minutes)    *
         *   - mp2 (USHORT/USHORT) - longitude coordinate (degrees/minutes)   *
         *                                                                    *
         * Set or change the geographic coordinates for the current location. *
         * These are used to calculate day/night and sunrise/sunset times.    *
         *                                                                    *
         * .................................................................. */
        case WTD_SETCOORDINATES:
            pdata = WinQueryWindowPtr( hwnd, 0 );
            pdata->coordinates.sLatitude  = (SHORT) SHORT1FROMMP( mp1 );
            pdata->coordinates.bLaMin     = (BYTE)  SHORT2FROMMP( mp1 );
            pdata->coordinates.sLongitude = (SHORT) SHORT1FROMMP( mp2 );
            pdata->coordinates.bLoMin     = (BYTE)  SHORT2FROMMP( mp2 );
            // TODO calculate sunrise/sunset times
            return (MRESULT) 0;


        /* .................................................................. *
         * WTD_SETOPTIONS                                                     *
         *                                                                    *
         *   - mp1 (ULONG)  - Option flags (for flOptions field)              *
         *   - mp2 (not used)                                                 *
         *                                                                    *
         * Set the control options corresponding to the flOptions field in    *
         * WTDDATA.  This sets the entire flOptions field; any previously     *
         * set options will be cleared.  NOTE: if WTF_TIME_CUSTOM or          *
         * WTF_DATE_CUSTOM are set, it is also necessary to set the actual    *
         * custom format strings using WTD_SETFORMATS (below).                *
         *                                                                    *
         * .................................................................. */
        case WTD_SETOPTIONS:
            pdata = WinQueryWindowPtr( hwnd, 0 );
            flNewOpts = (ULONG) mp1;
            pdata->flOptions = flNewOpts;

            // update the UniStrftime format strings
            SetFormatStrings( pdata );

            WinInvalidateRect( hwnd, NULL, FALSE );
            return (MRESULT) 0;


        /* .................................................................. *
         * WTD_SETFORMATS                                                     *
         *                                                                    *
         *   - mp1 (UniChar *)  - New UniStrftime time format specifier       *
         *   - mp2 (UniChar *)  - New UniStrftime date format specifier       *
         *                                                                    *
         * Set the custom UniStrftime time/date formatting strings.  These    *
         * are ignored unless WTF_TIME_CUSTOM and/or WTF_DATE_CUSTOM are      *
         * set in the flOptions field in WTDDATA.  To set only one of the two *
         * strings, simply specify MPVOID (NULL) for the other.               *
         *                                                                    *
         * .................................................................. */
        case WTD_SETFORMATS:
            pdata = WinQueryWindowPtr( hwnd, 0 );
            if ( mp1 && ( pdata->flOptions & WTF_TIME_CUSTOM ))
                UniStrncpy( pdata->uzTimeFmt, (UniChar *) mp1, STRFT_MAXZ );
            if ( mp2 && ( pdata->flOptions & WTF_DATE_CUSTOM ))
                UniStrncpy( pdata->uzDateFmt, (UniChar *) mp2, STRFT_MAXZ );
            return (MRESULT) 0;


        /* .................................................................. *
         * WTD_SETSEPARATOR                                                   *
         *                                                                    *
         *   - mp1 (UCHAR) - New percentage width of description field        *
         *   - mp2 (not used)                                                 *
         *                                                                    *
         * Set the width of the description field in compact view, as a       *
         * percentage value of the total width.                               *
         *                                                                    *
         * .................................................................. */
        case WTD_SETSEPARATOR:
            pdata = WinQueryWindowPtr( hwnd, 0 );
            pdata->bSep = (UCHAR) mp1;

            WinInvalidateRect( hwnd, NULL, FALSE );
            return (MRESULT) 0;


        /* .................................................................. *
         * WTD_QUERYCONFIG                                                    *
         *                                                                    *
         *   - mp1 (PWTDCDATA)                                                *
         *   - mp2 (not used)                                                 *
         *                                                                    *
         * Get a summary of the panel's current configuration. This is        *
         * returned as (pointer to a) WTDCDATA structure, which can be used   *
         * to (re)create a new time panel with the same configuration.  Note  *
         * that this does not include font and colour settings, which must    *
         * be queried separately through the presentation parameters.         *
         *                                                                    *
         * Returns: BOOL                                                      *
         *  TRUE if the configuration was returned successfully.              *
         * .................................................................. */
        case WTD_QUERYCONFIG:
            if ( mp1 && ( pdata = WinQueryWindowPtr( hwnd, 0 )) != NULL ) {
                PSZ pszLocName;

                pinit = (PWTDCDATA) mp1;
                pinit->flOptions = pdata->flOptions;
                pinit->coordinates = pdata->coordinates;
                pinit->bSep = pdata->bSep;
                strcpy( pinit->szTZ, pdata->szTZ );
                UniStrcpy( pinit->uzDesc, pdata->uzDesc );
                UniStrcpy( pinit->uzTimeFmt, pdata->uzTimeFmt );
                UniStrcpy( pinit->uzDateFmt, pdata->uzDateFmt );
                if ( pdata->locale &&
                     ( UniQueryLocaleObject( pdata->locale, LC_TIME, UNI_MBS_STRING_POINTER,
                                             (PPVOID) &pszLocName ) == ULS_SUCCESS )        )
                {
                    strncpy( pinit->szLocale, pszLocName, ULS_LNAMEMAX );
                    UniFreeMem( pszLocName );
                }

                return (MRESULT) TRUE;
            }
            return (MRESULT) FALSE;

        /* .................................................................. *
         * WTD_QUERYOPTIONS                                                   *
         *                                                                    *
         *   - mp1 (not used)                                                 *
         *   - mp2 (not used)                                                 *
         *                                                                    *
         * Return the option flags (flOptions) for the current configuration. *
         * This is more convenient to use than WTD_QUERYCONFIG if only the    *
         * option flags specifically are required.                            *
         *                                                                    *
         * Returns: ULONG                                                     *
         *  Current value of the flOptions field in WTDDATA.                  *
         * .................................................................. */
        case WTD_QUERYOPTIONS:
            pdata = WinQueryWindowPtr( hwnd, 0 );
            return (MRESULT) pdata->flOptions;


    }

    return ( WinDefWindowProc( hwnd, msg, mp1, mp2 ));
}


/* ------------------------------------------------------------------------- *
 * Paint_DefaultView                                                         *
 *                                                                           *
 * Draw the contents of a panel in "default" (medium) view.                  *
 *                                                                           *
 * ------------------------------------------------------------------------- */
void Paint_DefaultView( HWND hwnd, HPS hps, RECTL rcl, LONG clrBG, LONG clrFG, LONG clrBor, PWTDDATA pdata )
{
    FATTRS      fontAttrs,              // current font attributes
                fontAttrs2;             // attributes of time font (if raster)
    FONTMETRICS fm;                     // current font metrics
    BYTE        fbType;                 // font type
    BOOL        bUnicode;               // OK to use Unicode encoding?
    SIZEF       sfCell;                 // character cell size
    LONG        cb,                     // length of current text, in bytes
                clrLine,                // separator line colour
                lCell,                  // desired character-cell height
                lHeight,                // dimensions of control rectangle (minus border)
                lTBHeight,              // height of the top & bottom display areas
                lTBOffset,              // baseline offset from centre of top and bottom areas
                lTmHeight,              // height of the the time display area
                lTmInset;               // left/right spacing around the time display area
    CHAR        szFont[ FACESIZE+4 ];   // current font pres.param
    PSZ         pszFontName;            // requested font family
    PCH         pchText;                // pointer to current output text
    POINTL      ptl;                    // current drawing position
    RECTL       rclTop,                 // area of top (description) region == pdata->rclDesc
                rclMiddle,              // area of middle region
                rclTime,                // area of time display in the middle region == pdata->rclTime
                rclBottom;              // area of bottom (date) region == pdata->rclDate
    ULONG       ulid,
                rc;


    lHeight = rcl.yTop - rcl.yBottom;
    lCell = (lHeight / 5) - 1;

    sfCell.cy = MAKEFIXED( lCell, 0 );
    sfCell.cx = sfCell.cy;

    // define the requested font and codepage
    memset( &fontAttrs, 0, sizeof( FATTRS ));
    fontAttrs.usRecordLength = sizeof( FATTRS );
    fontAttrs.fsType         = FATTR_TYPE_MBCS;
    fontAttrs.fsFontUse      = FATTR_FONTUSE_NOMIX;
    rc = WinQueryPresParam( hwnd, PP_FONTNAMESIZE, 0, NULL, sizeof(szFont), szFont, QPF_NOINHERIT );
    if ( rc ) {
        pszFontName = strchr( szFont, '.') + 1;
        strcpy( fontAttrs.szFacename, pszFontName );
        fbType = GetFontType( hps, pszFontName, &fontAttrs, lCell, TRUE );
        if ( fbType == FTYPE_NOTFOUND )
            rc = 0;
        else if ( fbType == FTYPE_UNICODE )
            bUnicode = TRUE;
        else
            bUnicode = FALSE;
    }
    if ( !rc ) {
        strcpy( fontAttrs.szFacename, "Times New Roman MT 30");
        bUnicode = TRUE;
    }
    fontAttrs.usCodePage = bUnicode ? UNICODE : 0;

    // set the required text size and make the font active
    if (( GpiCreateLogFont( hps, NULL, 1L, &fontAttrs )) == GPI_ERROR ) return;  // WinEndPaint first!!
    GpiSetCharBox( hps, &sfCell );
    GpiSetCharSet( hps, 1L );

    // we will draw the time string using a larger font size
    lCell = lHeight / 3;
    if ( fbType == FTYPE_BITMAP ) {
        // Bitmap fonts can't be scaled by increasing the charbox; we have to
        // go and find the specific font name + point size that fits within
        // our adjusted cell size.
        memset( &fontAttrs2, 0, sizeof( FATTRS ));
        fontAttrs2.usRecordLength = sizeof( FATTRS );
        fontAttrs2.fsType         = FATTR_TYPE_MBCS;
        fontAttrs2.fsFontUse      = FATTR_FONTUSE_NOMIX;
        strcpy( fontAttrs2.szFacename, pszFontName );
        if ( GetFontType( hps, pszFontName, &fontAttrs2, lCell, TRUE ) == FTYPE_BITMAP )
            GpiCreateLogFont( hps, NULL, 2L, &fontAttrs2 );
    }

    // get the font metrics so we can do positioning calculations
    GpiQueryFontMetrics( hps, sizeof(FONTMETRICS), &fm );

    lTBHeight = lHeight / 4;
    lTBOffset = max( (lTBHeight / 2) - (fm.lEmHeight / 2), fm.lMaxDescender + 1 );
    lTmHeight = rcl.yTop - rcl.yBottom - (lTBHeight * 2) - 2;
    lTmInset  = fm.lAveCharWidth * 2;   // TODO allow space for daylight indicator etc.

    // define the top region (the timezone/city description area)
    rclTop.xLeft = rcl.xLeft;
    rclTop.xRight = rcl.xRight;
    rclTop.yTop = rcl.yTop;
    rclTop.yBottom = rcl.yTop - lTBHeight;
    pdata->rclDesc = rclTop;

    // define the central region, and the time display area within that
    rclMiddle.xLeft = rcl.xLeft;
    rclMiddle.xRight = rcl.xRight;
    rclMiddle.yBottom = rcl.yBottom + lTBHeight + 1;
    rclMiddle.yTop = rclTop.yBottom - 1;

    rclTime.xLeft = rclMiddle.xLeft + lTmInset;
    rclTime.xRight = rclMiddle.xRight - lTmInset;
    rclTime.yBottom = rclMiddle.yBottom;
    rclTime.yTop = rclMiddle.yTop;
    pdata->rclTime = rclTime;

    // define the bottom region (the date display area)
    rclBottom.xLeft = rcl.xLeft;
    rclBottom.xRight = rcl.xRight;
    rclBottom.yBottom = rcl.yBottom;
    rclBottom.yTop = rcl.yBottom + lTBHeight;
    pdata->rclDate = rclBottom;

    GpiSetTextAlignment( hps, TA_LEFT, TA_BASE );

    // paint the background
    WinFillRect( hps, &rcl, clrBG );

    // draw the description string (if defined)
    if ( pdata->uzDesc[0] ) {
        pchText = bUnicode ? (PCH) pdata->uzDesc : (PCH) pdata->szDesc;
        cb = bUnicode ? UniStrlen(pdata->uzDesc) * 2 : strlen( pdata->szDesc );
        ptl.x = rclTop.xLeft + fm.lAveCharWidth;
        ptl.y = rclTop.yBottom + lTBOffset;
        GpiCharStringPosAt( hps, &ptl, &rclTop, CHS_CLIP, cb, pchText, NULL );
    }

    // draw the date string
    if ( pdata->uzDate[0] ) {
        pchText = bUnicode ? (PCH) pdata->uzDate : (PCH) pdata->szDate;
        cb = bUnicode ? UniStrlen(pdata->uzDate) * 2 : strlen( pdata->szDate );
        ptl.x = rclBottom.xLeft + fm.lAveCharWidth;
        ptl.y = rclBottom.yBottom + lTBOffset;
        GpiCharStringPosAt( hps, &ptl, &rclBottom, CHS_CLIP, cb, pchText, NULL );
    }

    // draw the time string
    if ( pdata->uzTime[0] ) {
        if ( fbType == FTYPE_BITMAP )
            GpiSetCharSet( hps, 2L );
        sfCell.cy = MAKEFIXED( lCell, 0 );
        sfCell.cx = sfCell.cy;
        GpiSetCharBox( hps, &sfCell );

        pchText = bUnicode ? (PCH) pdata->uzTime : (PCH) pdata->szTime;
        cb = bUnicode ? UniStrlen(pdata->uzTime) * 2 : strlen( pdata->szTime );
        ptl.x = rclTime.xLeft + ((rclTime.xRight - rclTime.xLeft) / 2);
        ptl.y = rclTime.yBottom + (lTmHeight / 2);
        GpiSetTextAlignment( hps, TA_CENTER, TA_HALF );
        GpiCharStringPosAt( hps, &ptl, &rclTime, CHS_CLIP, cb, pchText, NULL );
    }

    // draw the separator line for the top region
    rc = WinQueryPresParam( hwnd, PP_SEPARATORCOLOR, 0, &ulid,
                            sizeof(clrLine), &clrLine, QPF_NOINHERIT );
    if ( !rc ) clrLine = clrBor;
    GpiSetColor( hps, clrLine );
    ptl.x = rclTop.xLeft;
    ptl.y = rclTop.yBottom;
    GpiMove( hps, &ptl );
    ptl.x = rclTop.xRight - 1;
    GpiLine( hps, &ptl );

}


/* ------------------------------------------------------------------------- *
 * Paint_CompactView                                                         *
 *                                                                           *
 * Draw the contents of a panel in "compact" view.                           *
 *                                                                           *
 * ------------------------------------------------------------------------- */
void Paint_CompactView( HWND hwnd, HPS hps, RECTL rcl, LONG clrBG, LONG clrFG, LONG clrBor, PWTDDATA pdata )
{
    FATTRS      fontAttrs;              // current font attributes
    FONTMETRICS fm;                     // current font metrics
    BYTE        fbType;                 // font type
    BOOL        bUnicode;               // OK to use Unicode encoding?
    SIZEF       sfCell;                 // character cell size
    LONG        cb,                     // length of current text, in bytes
                clrLine,                // separator line colour
                lCellX, lCellY,         // used for cell calculation
                lCell,                  // desired character-cell height
                lHeight, lWidth,        // dimensions of control rectangle (minus border)
                lLWidth,                // width of the left display area
                lTxtOffset;             // baseline offset from centre of displayable areas
    BOOL        fCY = TRUE;             // use cell height for size calculation (vs width)?
    CHAR        szFont[ FACESIZE+4 ];   // current font pres.param
    PSZ         pszFontName;            // requested font family
    PCH         pchText;                // pointer to current output text
    POINTL      ptl;                    // current drawing position
    RECTL       rclLeft,                // area of top (description) region == pdata->rclDesc
                rclRight,               // area of middle region
                rclDateTime;            // clipping area of date/time display in the right region == pdata->rclTime
    ULONG       ulid,
                rc;


    lWidth = rcl.xRight - rcl.xLeft;
    lHeight = rcl.yTop - rcl.yBottom;
    if ( pdata->bSep > 0 )
        lLWidth = (lWidth * pdata->bSep) / 100;
    else
        lLWidth = (lWidth * 55) / 100;

    lCellY = (lHeight / 5) * 4;
/*
    if ( pdata->szDesc[0] )
        lCellX = lLWidth / strlen(pdata->szDesc);
    else
        lCellX = lCellY;
    fCY = (lCellX >= lCellY) ? TRUE: FALSE;
*/
    lCell = fCY ? lCellY: lCellX;


    sfCell.cy = MAKEFIXED( lCell, 0 );
    sfCell.cx = sfCell.cy;

    // define the requested font and codepage
    memset( &fontAttrs, 0, sizeof( FATTRS ));
    fontAttrs.usRecordLength = sizeof( FATTRS );
    fontAttrs.fsType         = FATTR_TYPE_MBCS;
    fontAttrs.fsFontUse      = FATTR_FONTUSE_NOMIX;
    rc = WinQueryPresParam( hwnd, PP_FONTNAMESIZE, 0, NULL, sizeof(szFont), szFont, QPF_NOINHERIT );
    if ( rc ) {
        pszFontName = strchr( szFont, '.') + 1;
        strcpy( fontAttrs.szFacename, pszFontName );
        fbType = GetFontType( hps, pszFontName, &fontAttrs, lCell, fCY );
        if ( fbType == FTYPE_NOTFOUND )
            rc = 0;
        else if ( fbType == FTYPE_UNICODE )
            bUnicode = TRUE;
        else
            bUnicode = FALSE;
    }
    if ( !rc ) {
        strcpy( fontAttrs.szFacename, "Times New Roman MT 30");
        bUnicode = TRUE;
    }
    fontAttrs.usCodePage = bUnicode ? UNICODE : 0;

    // set the required text size and make the font active
    if (( GpiCreateLogFont( hps, NULL, 1L, &fontAttrs )) == GPI_ERROR ) return;
    GpiSetCharBox( hps, &sfCell );
    GpiSetCharSet( hps, 1L );

    // get the font metrics so we can do positioning calculations
    GpiQueryFontMetrics( hps, sizeof(FONTMETRICS), &fm );

    lTxtOffset = max( (lHeight / 2) - (fm.lEmHeight / 2), fm.lMaxDescender + 1 );

    // define the left region (the timezone/city description area)
    rclLeft.xLeft = rcl.xLeft;
    rclLeft.xRight = rcl.xLeft + lLWidth;
    rclLeft.yTop = rcl.yTop;
    rclLeft.yBottom = rcl.yBottom;
    pdata->rclDesc = rclLeft;

    // define the right region (the date/time display area)
    rclRight.xLeft = rclLeft.xRight + 1;
    rclRight.xRight = rcl.xRight;
    rclRight.yBottom = rcl.yBottom;
    rclRight.yTop = rcl.yTop;

    rclDateTime.xLeft = rclRight.xLeft;
    rclDateTime.xRight = rclRight.xRight - (2 * fm.lAveCharWidth);
    rclDateTime.yBottom = rclRight.yBottom;
    rclDateTime.yTop = rclRight.yTop;
    pdata->rclTime = rclDateTime;
    pdata->rclDate = rclDateTime;

    GpiSetTextAlignment( hps, TA_LEFT, TA_BASE );

    // paint the background
    WinFillRect( hps, &rcl, clrBG );

    // draw the description string (if defined)
    if ( pdata->uzDesc[0] ) {
        pchText = bUnicode ? (PCH) pdata->uzDesc : (PCH) pdata->szDesc;
        cb = bUnicode ? UniStrlen(pdata->uzDesc) * 2 : strlen( pdata->szDesc );
        ptl.x = rclLeft.xLeft + fm.lAveCharWidth;
        ptl.y = rclLeft.yBottom + lTxtOffset;
        GpiCharStringPosAt( hps, &ptl, &rclLeft, CHS_CLIP, cb, pchText, NULL );
    }

    // draw the date string
    if (( pdata->flState & WTS_CVR_DATE ) && pdata->uzDate[0] ) {
        pchText = bUnicode ? (PCH) pdata->uzDate : (PCH) pdata->szDate;
        cb = bUnicode ? UniStrlen(pdata->uzDate) * 2 : strlen( pdata->szDate );
        ptl.x = rclRight.xLeft + fm.lAveCharWidth;
        ptl.y = rclRight.yBottom + lTxtOffset;
        GpiCharStringPosAt( hps, &ptl, &rclDateTime, CHS_CLIP, cb, pchText, NULL );
    }

    // draw the time string
    else if ( pdata->uzTime[0] ) {
        pchText = bUnicode ? (PCH) pdata->uzTime : (PCH) pdata->szTime;
        cb = bUnicode ? UniStrlen(pdata->uzTime) * 2 : strlen( pdata->szTime );
        ptl.x = rclRight.xLeft + fm.lAveCharWidth;
        ptl.y = rclRight.yBottom + lTxtOffset;
        GpiCharStringPosAt( hps, &ptl, &rclDateTime, CHS_CLIP, cb, pchText, NULL );
    }

    // draw the separator line
    rc = WinQueryPresParam( hwnd, PP_SEPARATORCOLOR, 0, &ulid,
                            sizeof(clrLine), &clrLine, QPF_NOINHERIT );
    if ( !rc ) clrLine = clrBor;
    GpiSetColor( hps, clrLine );
    ptl.x = rclLeft.xRight;
    ptl.y = rclLeft.yBottom;
    GpiMove( hps, &ptl );
    ptl.y = rclLeft.yTop;
    GpiLine( hps, &ptl );

}


/* ------------------------------------------------------------------------- *
 * GetFontType                                                               *
 *                                                                           *
 * This function does a couple of things.  First, it looks to see if the     *
 * requested font name corresponds to a bitmap (a.k.a. raster) font.  If so, *
 * it sets the required attributes (in the passed FATTRS object) to indicate *
 * the largest point size which fits within the requested character-cell     *
 * size, unless said cell size is too small for any of the available point   *
 * sizes.  In the latter case, if an outline version of the font exists,     *
 * indicate to use that instead; otherwise, return the smallest bitmap point *
 * size that is available.                                                   *
 *                                                                           *
 * If no matching bitmap font can be found, look for an outline version of   *
 * the font.  If the requested font name does not exist at all, then simply  *
 * return FTYPE_NOTFOUND.                                                    *
 *                                                                           *
 * The function also checks to see if the font is reported as a Unicode font *
 * (i.e. one that supports the UNICODE glyphlist).                           *
 *                                                                           *
 * ARGUMENTS:                                                                *
 *   HPS     hps        : Handle of the current presentation space.          *
 *   PSZ     pszFontFace: Face name of the font being requested.             *
 *   PFATTRS pfAttrs    : Address of a FATTRS object to receive the results. *
 *   LONG    lCell      : The requested cell size.                           *
 *   BOOL    bH         : TRUE: lCell represents cell height; FALSE: width   *
 *                                                                           *
 * RETURNS: BYTE                                                             *
 *   One of the following:                                                   *
 *   FTYPE_NOTFOUND: requested font name could not be found (does not exist) *
 *   FTYPE_BITMAP  : font is a bitmap font (which is always non-Unicode)     *
 *   FTYPE_UNICODE : font is a Unicode-capable outline font                  *
 *   FTYPE_NOTUNI  : font is a non-Unicode outline font                      *
 * ------------------------------------------------------------------------- */
BYTE GetFontType( HPS hps, PSZ pszFontFace, PFATTRS pfAttrs, LONG lCell, BOOL bH )
{
    PFONTMETRICS pfm;               // array of FONTMETRICS objects
    LONG         i,                 // loop index
                 lSmIdx    = -1,    // index of smallest available font
                 lLargest  = 0,     // max baseline ext of largest font
                 cFonts    = 0,     // number of fonts found
                 cCount    = 0;     // number of fonts to return
    BOOL         bOutline  = FALSE, // does font include an outline version?
                 bUnicode  = FALSE, // is font a Unicode font?
                 bFound    = FALSE; // did we find a suitable bitmap font?


    // Find the specific fonts which match the given face name
    cFonts = GpiQueryFonts( hps, QF_PUBLIC, pszFontFace,
                            &cCount, sizeof(FONTMETRICS), NULL );
    if ( cFonts < 1 ) return FTYPE_NOTFOUND;
    if (( pfm = (PFONTMETRICS) calloc( cFonts, sizeof(FONTMETRICS) )) == NULL )
        return FTYPE_NOTFOUND;
    GpiQueryFonts( hps, QF_PUBLIC, pszFontFace,
                   &cFonts, sizeof(FONTMETRICS), pfm );

    // Look for the largest bitmap font that fits within the requested height
    for ( i = 0; i < cFonts; i++ ) {
        if ( !( pfm[i].fsDefn & FM_DEFN_OUTLINE )) {
            if (( lSmIdx < 0 ) || ( bH? (pfm[i].lMaxBaselineExt < pfm[lSmIdx].lMaxBaselineExt):
                                        (pfm[i].lAveCharWidth < pfm[lSmIdx].lAveCharWidth) ))
                lSmIdx = i;
            if ( bH? (( pfm[i].lMaxBaselineExt <= lCell ) && ( pfm[i].lMaxBaselineExt > lLargest )):
                     (( pfm[i].lAveCharWidth <= lCell ) && ( pfm[i].lAveCharWidth > lLargest )) ) {
                lLargest = bH? pfm[i].lMaxBaselineExt: pfm[i].lAveCharWidth;
                bFound   = TRUE;
                strcpy( pfAttrs->szFacename, pfm[i].szFacename );
                pfAttrs->lMatch          = pfm[i].lMatch;
                pfAttrs->idRegistry      = pfm[i].idRegistry;
                pfAttrs->lMaxBaselineExt = pfm[i].lMaxBaselineExt;
                pfAttrs->lAveCharWidth   = pfm[i].lAveCharWidth;
            }
        } else {
            bOutline = TRUE;
            if ( pfm[i].fsType & FM_TYPE_UNICODE )
                bUnicode = TRUE;
        }
    }

    // If nothing fits within the requested cell size, use the smallest available
    if ( !bFound && !bOutline && lSmIdx >= 0 ) {
        bFound = TRUE;
        strcpy( pfAttrs->szFacename, pfm[lSmIdx].szFacename );
        pfAttrs->lMatch          = pfm[lSmIdx].lMatch;
        pfAttrs->idRegistry      = pfm[lSmIdx].idRegistry;
        pfAttrs->lMaxBaselineExt = pfm[lSmIdx].lMaxBaselineExt;
        pfAttrs->lAveCharWidth   = pfm[lSmIdx].lAveCharWidth;
    }

    free( pfm );
    if ( bUnicode ) return FTYPE_UNICODE;
    else if ( bOutline ) return FTYPE_NOTUNI;
    else return FTYPE_BITMAP;
}


/* ------------------------------------------------------------------------- *
 * SetFormatStrings                                                          *
 *                                                                           *
 * Initialize the current UniStrftime() format specifiers according to the   *
 * current locale and options.                                               *
 *                                                                           *
 * ------------------------------------------------------------------------- */
void SetFormatStrings( PWTDDATA pdata )
{
    UniChar *puzLOCI,           // queried locale item value
            *puzTSep,           // time separator from locale
            uzSecSpec[16],      // seconds format specifier
            *puz;               // string position pointer
    ULONG   cb, len, off,
            i, j;

    // Get the system (OS2.INI) time & date formatting defaults
    pdata->sysTimeFmt.iTime = PrfQueryProfileInt( HINI_USERPROFILE, "PM_Default_National", "iTime", 1 );
    PrfQueryProfileString( HINI_USERPROFILE, "PM_Default_National", "s1159", "AM", pdata->sysTimeFmt.szAM, PRF_NTLDATA_MAXZ );
    PrfQueryProfileString( HINI_USERPROFILE, "PM_Default_National", "s2359", "PM", pdata->sysTimeFmt.szPM, PRF_NTLDATA_MAXZ );
    PrfQueryProfileString( HINI_USERPROFILE, "PM_Default_National", "sTime", ":", pdata->sysTimeFmt.szTS, PRF_NTLDATA_MAXZ );
    pdata->sysTimeFmt.iDate = PrfQueryProfileInt( HINI_USERPROFILE, "PM_Default_National", "iDate", 2 );
    PrfQueryProfileString( HINI_USERPROFILE, "PM_Default_National", "sDate", "/", pdata->sysTimeFmt.szDS, PRF_NTLDATA_MAXZ );

    // Set the time format string
    if ( pdata->flOptions & WTF_TIME_ALT ) {
        // alternate time format string is requested, check locale for it
        if ( UniQueryLocaleItem( pdata->locale, LOCI_sAltTimeFormat, &puzLOCI ) == ULS_SUCCESS ) {
            if ( puzLOCI[0] == 0 )
                // no alt. time format defined for locale; use normal one instead
                UniStrcpy( pdata->uzTimeFmt, L"%X");
            else
                // We don't actually use the value of puzLOCI, just make sure
                // it exists.  If there is an alt. time format string, %EX will
                // map to it (whatever it may be).
                UniStrcpy( pdata->uzTimeFmt, L"%EX");
            UniFreeMem( puzLOCI );
        } else UniStrcpy( pdata->uzTimeFmt, L"%X");

    } else if ( pdata->flOptions & WTF_TIME_CUSTOM ) {
        // if the flags indicate a custom string but no custom string is set,
        // initialize it to the default
        if ( ! pdata->uzTimeFmt[0] ) UniStrcpy( pdata->uzTimeFmt, L"%X");

    } else if ( pdata->flOptions & WTF_TIME_SHORT ) {
        // WTF_TIME_SHORT only applies to normal and system time mode (and
        // system will handle it separately at formatting time).
        // Unfortunately, there is no regular locale item for "default short
        // time format", so we have to manually strip the seonds out of the
        // normal time format string.
        if (( UniQueryLocaleItem( pdata->locale, LOCI_sTimeFormat, &puzLOCI ) == ULS_SUCCESS ) &&
            ( UniQueryLocaleItem( pdata->locale, LOCI_sTime, &puzTSep ) == ULS_SUCCESS ))
        {
            // search for either ":%S" (where : is the time separator), or failing that simply "%S"
            UniStrncpy( uzSecSpec, puzTSep, 15 );
            UniStrncat( uzSecSpec, L"%S", 15 );
            cb = UniStrlen( uzSecSpec );
            puz = UniStrstr( puzLOCI, uzSecSpec );
            if ( !puz ) {
                cb = 2;
                puz = UniStrstr( puzLOCI, L"%S");
            }
            // now copy the string, skipping over the matched portion
            if ( puz ) {
                len = UniStrlen( puzLOCI );
                off = puz - puzLOCI;
                for ( i = 0, j = 0; i < len; i++ ) {
                    if ( i == off ) i += cb-1;
                    else pdata->uzTimeFmt[ j++ ] = puzLOCI[ i ];
                }
                pdata->uzTimeFmt[ j ] = 0;
            }
        }
        else UniStrcpy( pdata->uzTimeFmt, L"%R:%S");
    }
    else
        UniStrcpy( pdata->uzTimeFmt, L"%X");

    // In the case of WTF_TIME_SYSTEM, uzTimeFmt won't be used; we will be
    // formatting the time ourselves using the data from PM_Default_National

    // Set the date format string

    if ( pdata->flOptions & WTF_DATE_ALT ) {
        // alternate date format string is requested, check locale for it
        if ( UniQueryLocaleItem( pdata->locale, LOCI_sAltShortDate, &puzLOCI ) == ULS_SUCCESS ) {
            if ( !puzLOCI || puzLOCI[0] == 0 )
                // no alt. date format found; use normal one instead
                UniStrcpy( pdata->uzDateFmt, L"%x");
            else
                // like above, we can use %Ex instead of puzLOCI
                UniStrcpy( pdata->uzDateFmt, L"%Ex");
            UniFreeMem( puzLOCI );
        } else UniStrcpy( pdata->uzDateFmt, L"%x");

    } else if ( pdata->flOptions & WTF_DATE_CUSTOM ) {
        if ( ! pdata->uzDateFmt[0] ) UniStrcpy( pdata->uzDateFmt, L"%x");

    } else
        UniStrcpy( pdata->uzDateFmt, L"%x");

    // In the case of WTF_DATE_SYSTEM, uzDateFmt won't be used; we will be
    // formatting the date ourselves using the data from PM_Default_National

}


/* ------------------------------------------------------------------------- *
 * FormatTime                                                                *
 *                                                                           *
 * Generate a formatted time string.  Normally this simply calls UniStrftime *
 * with the current format specifier.  However, in the event that            *
 * WTF_TIME_SYSTEM is set, we will format the time ourselves using the       *
 * conventions previously obtained from OS2.INI PM_Default_National (which   *
 * indicates the current "system default" formatting conventions).           *
 *                                                                           *
 * ------------------------------------------------------------------------- */
void FormatTime( HWND hwnd, PWTDDATA pdata, struct tm *time )
{
    CHAR    szFTime[ TIMESTR_MAXZ ];
    UniChar uzFTime[ TIMESTR_MAXZ ];
    ULONG   rc;

    if ( pdata->flOptions & WTF_TIME_SYSTEM ) {
        if ( pdata->sysTimeFmt.iTime ) {                // 24-hour format
            if ( pdata->flOptions & WTF_TIME_SHORT )        // without seconds
                sprintf( szFTime, "%u%s%02u", time->tm_hour, pdata->sysTimeFmt.szTS,
                                              time->tm_min                           );
            else                                            // with seconds
                sprintf( szFTime, "%u%s%02u%s%02u", time->tm_hour, pdata->sysTimeFmt.szTS,
                                                    time->tm_min, pdata->sysTimeFmt.szTS,
                                                    time->tm_sec                          );
        }
        else {                                          // 12-hour format
            if ( pdata->flOptions & WTF_TIME_SHORT )        // without seconds
                sprintf( szFTime, "%u%s%02u %s", (time->tm_hour > 12) ?
                                                     time->tm_hour - 12 :
                                                     !time->tm_hour? 12: time->tm_hour,
                                                 pdata->sysTimeFmt.szTS,
                                                 time->tm_min,
                                                 (time->tm_hour > 11) ?
                                                     pdata->sysTimeFmt.szPM :
                                                     pdata->sysTimeFmt.szAM            );
            else                                            // with seconds
                sprintf( szFTime, "%u%s%02u%s%02u %s", (time->tm_hour > 12) ?
                                                           time->tm_hour - 12 :
                                                           time->tm_hour,
                                                       pdata->sysTimeFmt.szTS,
                                                       time->tm_min, pdata->sysTimeFmt.szTS,
                                                       time->tm_sec,
                                                       (time->tm_hour > 11) ?
                                                           pdata->sysTimeFmt.szPM :
                                                           pdata->sysTimeFmt.szAM            );
        }
        UniStrToUcs( pdata->uconv, uzFTime, szFTime, TIMESTR_MAXZ );
    }
    else
        rc = (ULONG) UniStrftime( pdata->locale, uzFTime,
                                  TIMESTR_MAXZ-1, pdata->uzTimeFmt, time );

    if ( UniStrcmp( uzFTime, pdata->uzTime ) != 0 ) {
        UniStrcpy( pdata->uzTime, uzFTime );
        if (( rc = UniStrFromUcs( pdata->uconv, pdata->szTime,
                                  pdata->uzTime, TIMESTR_MAXZ )) != ULS_SUCCESS )
            pdata->szTime[0] = 0;
        WinInvalidateRect( hwnd, &(pdata->rclTime), FALSE );
    }

}


/* ------------------------------------------------------------------------- *
 * FormatDate                                                                *
 *                                                                           *
 * Generate a formatted date string.  Normally this simply calls UniStrftime *
 * with the current format specifier.  However, in the event that            *
 * WTF_DATE_SYSTEM is set, we will format the date ourselves using the       *
 * conventions previously obtained from OS2.INI PM_Default_National (which   *
 * indicates the current "system default" formatting conventions).           *
 *                                                                           *
 * ------------------------------------------------------------------------- */
void FormatDate( HWND hwnd, PWTDDATA pdata, struct tm *time )
{
    CHAR    szFDate[ DATESTR_MAXZ ];
    UniChar uzFDate[ DATESTR_MAXZ ];
    ULONG   rc;

    if ( pdata->flOptions & WTF_DATE_SYSTEM ) {
        switch ( pdata->sysTimeFmt.iDate ) {
            case 0:                 // M-D-Y format
                sprintf( szFDate, "%02u%s%02u%s%u", 1 + time->tm_mon, pdata->sysTimeFmt.szDS,
                                                    time->tm_mday, pdata->sysTimeFmt.szDS,
                                                    1900 + time->tm_year );
                break;

            case 1:                 // D-M-Y format
                sprintf( szFDate, "%02u%s%02u%s%u", time->tm_mday, pdata->sysTimeFmt.szDS,
                                                    1 + time->tm_mon, pdata->sysTimeFmt.szDS,
                                                    1900 + time->tm_year );
                break;

            default:                // Y-M-D format
                sprintf( szFDate, "%u%s%02u%s%02u", 1900 + time->tm_year, pdata->sysTimeFmt.szDS,
                                                    1 + time->tm_mon, pdata->sysTimeFmt.szDS,
                                                    time->tm_mday );
                break;
        }
        UniStrToUcs( pdata->uconv, uzFDate, szFDate, DATESTR_MAXZ );
    }
    else
        rc = (ULONG) UniStrftime( pdata->locale, uzFDate,
                          DATESTR_MAXZ-1, pdata->uzDateFmt, time );

    if ( UniStrcmp( uzFDate, pdata->uzDate ) != 0 ) {
        UniStrcpy( pdata->uzDate, uzFDate );
        if (( rc = UniStrFromUcs( pdata->uconv, pdata->szDate,
                                  pdata->uzDate, DATESTR_MAXZ )) != ULS_SUCCESS )
            pdata->szDate[0] = 0;
        WinInvalidateRect( hwnd, &(pdata->rclDate), FALSE );
    }

}


/*
struct tm
   {
   int tm_sec;      // seconds after the minute [0-61]
   int tm_min;      // minutes after the hour [0-59]
   int tm_hour;     // hours since midnight [0-23]
   int tm_mday;     // day of the month [1-31]
   int tm_mon;      // months since January [0-11]
   int tm_year;     // years since 1900
   int tm_wday;     // days since Sunday [0-6]
   int tm_yday;     // days since January 1 [0-365]
   int tm_isdst;    // Daylight Saving Time flag
};
*/