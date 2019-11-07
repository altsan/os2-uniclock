#include "uclock.h"
#include "sunriset.h"

// Internal defines
#define FTYPE_NOTFOUND 0    // font does not exist
#define FTYPE_BITMAP   1    // font is a bitmap font (always non-Unicode)
#define FTYPE_UNICODE  2    // font is a Unicode-capable outline font
#define FTYPE_NOTUNI   3    // font is a non-Unicode outline font

#define PRF_NTLDATA_MAXZ    32  // max. length of a data item in PM_(Default)_National

// convert coordinates from (degrees, minutes, seconds) into single decimal values
#define DECIMAL_DEGREES( deg, min, sec )    ( (double)( deg + min/60 + sec/3600 ))

// round a time_t value to midnight at the start of that day
#define ROUND_TO_MIDNIGHT( time )           ( floor(time / 86400) * 86400 )

// boolean check for whether the given time is daytime.
#define IS_DAYTIME( now, sunrise, sunset )    (( now > sunrise ) && ( now < sunset ))

// Internal function prototypes
void Paint_DefaultView( HWND hwnd, HPS hps, RECTL rcl, LONG clrBG, LONG clrFG, LONG clrBor, PWTDDATA pData );
void Paint_CompactView( HWND hwnd, HPS hps, RECTL rcl, LONG clrBG, LONG clrFG, LONG clrBor, PWTDDATA pdata );
BYTE GetFontType( HPS hps, PSZ pszFontFace, PFATTRS pfAttrs, LONG lCY, BOOL bH );
void SetFormatStrings( PWTDDATA pdata );
BOOL FormatTime( HWND hwnd, PWTDDATA pdata, struct tm *time, UniChar *puzTime, PSZ pszTime );
BOOL FormatDate( HWND hwnd, PWTDDATA pdata, struct tm *time );
void SetSunTimes( HWND hwnd, PWTDDATA pdata );
void DrawSunriseIndicator( HPS hps );
void DrawSunsetIndicator( HPS hps );
void DrawDayIndicatorSmall( HPS hps );
void DrawDayIndicatorLarge( HPS hps );
void DrawNightIndicatorSmall( HPS hps );
void DrawNightIndicatorLarge( HPS hps );


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
                    // cycle to the next view
                    if ( pdata->flState & WTS_CVR_DATE ) {
                        // currently showing date - cycle to time
                        pdata->flState &= ~WTS_CVR_DATE;
                        //pdata->flState |= WTS_CVR_SUNTIME;
                        //pdata->flState &= ~WTS_CVR_WEATHER;   // for future use
                    }
                    else if ( pdata->flState & WTS_CVR_SUNTIME ) {
                        // currently showing sunrise/sunset - not yet implemented
                        pdata->flState &= ~WTS_CVR_DATE;
                        //pdata->flState &= ~WTS_CVR_SUNTIME;
                        //pdata->flState |= WTS_CVR_WEATHER;    // for future use
                    }
                    else {
                        // currently showing time (default) - cycle to date
                        pdata->flState |= WTS_CVR_DATE;
                        //pdata->flState &= ~WTS_CVR_SUNTIME;
                        //pdata->flState &= ~WTS_CVR_WEATHER;   // for future use
                    }
                    WinInvalidateRect( hwnd, &(pdata->rclDate), FALSE );
                }
            }
            else {      // default (medium) view
                if ( WinPtInRect( WinQueryAnchorBlock(hwnd), &(pdata->rclDate), &ptl )) {
                    // date (bottom) area - cycle to the next view
                    if ( pdata->flState & WTS_BOT_SUNTIME ) {
                        // currently showing sunrise/sunset time, cycle to date
                        pdata->flState &= ~WTS_BOT_SUNTIME;
                    }
                    else {
                        // currently showing date, cycle to sunrise/sunset
                        if ( pdata->flOptions & WTF_PLACE_HAVECOORD )
                            pdata->flState |= WTS_BOT_SUNTIME;
                    }
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
                pdata->timeval = ctime;

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
                    if ( FormatTime( hwnd, pdata, ltime, pdata->uzTime, pdata->szTime ))
                        WinInvalidateRect( hwnd, &(pdata->rclTime), FALSE );
                    if ( FormatDate( hwnd, pdata, ltime )) {
                        WinInvalidateRect( hwnd, &(pdata->rclDate), FALSE );
                        SetSunTimes( hwnd, pdata );
                    }
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
         *   - mp1 (SHORT/SHORT) - latitude coordinate (degrees/minutes)      *
         *   - mp2 (SHORT/SHORT) - longitude coordinate (degrees/minutes)     *
         *                                                                    *
         * Set or change the geographic coordinates for the current location. *
         * These are used to calculate day/night and sunrise/sunset times.    *
         * Degrees must be -90 to 90 for latitude, -180 to 180 for longitude. *
         * Minutes must be 0 to 60.                                           *
         *                                                                    *
         * Set mp1 to 0xFFFFFFFF to unset the coordinates.                    *
         *                                                                    *
         * .................................................................. */
        case WTD_SETCOORDINATES:
            pdata = WinQueryWindowPtr( hwnd, 0 );
            if ( (ULONG)mp1 == 0xFFFFFFFF ) {
                pdata->coordinates.sLatitude  = 0;
                pdata->coordinates.bLaMin     = 0;
                pdata->coordinates.sLongitude = 0;
                pdata->coordinates.bLoMin     = 0;
                pdata->flOptions &= ~WTF_PLACE_HAVECOORD;
            }
            else {
                // TODO validate the values
                pdata->coordinates.sLatitude  = (SHORT) SHORT1FROMMP( mp1 );
                pdata->coordinates.bLaMin     = (BYTE)  SHORT2FROMMP( mp1 );
                pdata->coordinates.sLongitude = (SHORT) SHORT1FROMMP( mp2 );
                pdata->coordinates.bLoMin     = (BYTE)  SHORT2FROMMP( mp2 );
                pdata->flOptions |= WTF_PLACE_HAVECOORD;
            }
            /* There's no need to update the sunrise/sunset times here, as
             * it will be done the next time WTD_SETDATETIME is called
             * (which normally happens several times a second).
             */
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
            SetSunTimes( hwnd, pdata );

            WinInvalidateRect( hwnd, NULL, FALSE );
            return (MRESULT) 0;


        /* .................................................................. *
         * WTD_SETFORMATS                                                     *
         *                                                                    *
         *   - mp1 (UniChar *)  - New UniStrftime time format specifier       *
         *   - mp2 (UniChar *)  - New UniStrftime date format specifier       *
         *                                                                    *
         * Set the custom UniStrftime formatting strings for primary date and *
         * time display.  These strings are only set if WTF_TIME_CUSTOM       *
         * and/or WTF_DATE_CUSTOM are set in the WTDATA flOptions field.  To  *
         * set only one of the two strings, simply specify MPVOID (NULL) for  *
         * the other and it will not be changed.                              *
         *                                                                    *
         * .................................................................. */
        case WTD_SETFORMATS:
            pdata = WinQueryWindowPtr( hwnd, 0 );
            if ( mp1 && ( pdata->flOptions & WTF_TIME_CUSTOM ))
                UniStrncpy( pdata->uzTimeFmt, (UniChar *) mp1, STRFT_MAXZ );
            if ( mp2 && ( pdata->flOptions & WTF_DATE_CUSTOM ))
                UniStrncpy( pdata->uzDateFmt, (UniChar *) mp2, STRFT_MAXZ );
            SetSunTimes( hwnd, pdata );
            return (MRESULT) 0;


        /* .................................................................. *
         * WTD_SETALTFORMATS                                                  *
         *                                                                    *
         *   - mp1 (UniChar *)  - New UniStrftime time format specifier       *
         *   - mp2 (UniChar *)  - New UniStrftime date format specifier       *
         *                                                                    *
         * Set the custom UniStrftime formatting strings for alternate date   *
         * and time display.  These strings are only set if WTF_ATIME_CUSTOM  *
         * and/or WTF_ADATE_CUSTOM are set in the WTDATA flOptions field.  To *
         * set only one of the two strings, simply specify MPVOID (NULL) for  *
         * the other and it will not be changed.                              *
         *                                                                    *
         * .................................................................. *
        case WTD_SETALTFORMATS:
            pdata = WinQueryWindowPtr( hwnd, 0 );
            if ( mp1 && ( pdata->flOptions & WTF_ATIME_CUSTOM ))
                UniStrncpy( pdata->uzTimeFmt2, (UniChar *) mp1, STRFT_MAXZ );
            if ( mp2 && ( pdata->flOptions & WTF_ADATE_CUSTOM ))
                UniStrncpy( pdata->uzDateFmt2, (UniChar *) mp2, STRFT_MAXZ );
            return (MRESULT) 0;
        */

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
         * WTD_SETINDICATORS                                                  *
         *                                                                    *
         *   - mp1 (HPTR) - Pointer to daytime indicator icon data            *
         *   - mp2 (HPTR) - Pointer to nighttime indicator icon data          *
         *                                                                    *
         * Set the icons used for the day/night indicators.  These must be    *
         * created by the parent application, probably from resources.  If    *
         * the application ever destroys these icons, it must call this       *
         * message again with both handles set to NULL.                       *
         *                                                                    *
         * .................................................................. */
        case WTD_SETINDICATORS:
            pdata = WinQueryWindowPtr( hwnd, 0 );
            if ( mp1 ) pdata->hptrDay = (HPOINTER) mp1;
            if ( mp2 ) pdata->hptrNight = (HPOINTER) mp2;
            WinInvalidateRect( hwnd, &(pdata->rclDate), FALSE );
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
    if (( GpiCreateLogFont( hps, NULL, 1L, &fontAttrs )) == GPI_ERROR ) return;
    GpiSetCharBox( hps, &sfCell );
    GpiSetCharSet( hps, 1L );

    // we will draw the time string using a larger font size
    lCell = lHeight / 3;
    if ( fbType == FTYPE_BITMAP ) {
        /* Bitmap fonts can't be scaled by increasing the charbox; we have to
         * go and find the specific font name + point size that fits within
         * our adjusted cell size.
         */
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
    lTmInset  = max( fm.lEmInc / 2, 6 );

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
        ptl.x = rclTop.xLeft + lTmInset;
        ptl.y = rclTop.yBottom + lTBOffset;
        GpiCharStringPosAt( hps, &ptl, &rclTop, CHS_CLIP, cb, pchText, NULL );
    }

    // draw the bottom area text
    if ( pdata->uzDate[0] ) {
        if ( pdata->flState & WTS_BOT_SUNTIME ) {
            // sunrise/sunset times
            ptl.x = rclBottom.xLeft + lTmInset;
            ptl.y = rclBottom.yBottom + 20;
            GpiMove( hps, &ptl );
            DrawSunriseIndicator( hps );
            pchText = bUnicode ? (PCH) pdata->uzSunR : (PCH) pdata->szSunR;
            cb = bUnicode ? UniStrlen(pdata->uzSunR) * 2 : strlen( pdata->szSunR );
            ptl.x += 20;
            ptl.y = rclBottom.yBottom + lTBOffset;
            GpiCharStringPosAt( hps, &ptl, &rclBottom, CHS_CLIP, cb, pchText, NULL );
            GpiQueryCurrentPosition( hps, &ptl );
            ptl.x += 3 * lTmInset;
            ptl.y = rclBottom.yBottom + 20;
            GpiMove( hps, &ptl );
            DrawSunsetIndicator( hps );
            pchText = bUnicode ? (PCH) pdata->uzSunS : (PCH) pdata->szSunS;
            cb = bUnicode ? UniStrlen(pdata->uzSunS) * 2 : strlen( pdata->szSunS );
            ptl.x += 20;
            ptl.y = rclBottom.yBottom + lTBOffset;
            GpiCharStringPosAt( hps, &ptl, &rclBottom, CHS_CLIP, cb, pchText, NULL );
        }
        else {
            // date string
            pchText = bUnicode ? (PCH) pdata->uzDate : (PCH) pdata->szDate;
            cb = bUnicode ? UniStrlen(pdata->uzDate) * 2 : strlen( pdata->szDate );
            ptl.x = rclBottom.xLeft + lTmInset;
            ptl.y = rclBottom.yBottom + lTBOffset;
            GpiCharStringPosAt( hps, &ptl, &rclBottom, CHS_CLIP, cb, pchText, NULL );
        }
    }

    // draw the day/night indicator, if applicable
    if ( pdata->flOptions & WTF_PLACE_HAVECOORD ) {
        ptl.x = rclMiddle.xRight - 40;
        if ( IS_DAYTIME( pdata->timeval, pdata->tm_sunrise, pdata->tm_sunset )) {
            if ( pdata->hptrDay ) {
                ptl.y = rclMiddle.yBottom + (lTmHeight / 2) - 20;
                WinDrawPointer( hps, ptl.x, ptl.y, pdata->hptrDay, DP_NORMAL );
                //rclTime.xRight -= ( 40 - lTmInset );
            }
            else {
                ptl.y = rclMiddle.yBottom + (lTmHeight / 2) + 16;
                GpiMove( hps, &ptl );
                DrawDayIndicatorLarge( hps );
                //rclTime.xRight -= ( 32 - lTmInset );
            }
        }
        else {
            if ( pdata->hptrNight ) {
                ptl.y = rclMiddle.yBottom + (lTmHeight / 2) - 20;
                WinDrawPointer( hps, ptl.x, ptl.y, pdata->hptrNight, DP_NORMAL );
                //rclTime.xRight -= ( 40 - lTmInset );
            }
            else {
                ptl.y = rclMiddle.yBottom + (lTmHeight / 2) + 16;
                GpiMove( hps, &ptl );
                DrawNightIndicatorLarge( hps );
                //rclTime.xRight -= ( 32 - lTmInset );
            }
        }
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
                lTxtInset,              // default left text margin
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

    lTxtOffset = 1 + max( (lHeight / 2) - (fm.lEmHeight / 2), fm.lMaxDescender + 1 );
    lTxtInset = max( fm.lEmInc / 2, 6 );

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
    rclDateTime.xRight = rclRight.xRight - 22;
    rclDateTime.yBottom = rclRight.yBottom;
    rclDateTime.yTop = rclRight.yTop;
    pdata->rclTime = rclDateTime;
    pdata->rclDate = rclDateTime;

    GpiSetTextAlignment( hps, TA_LEFT, TA_BASE );

    // paint the background
    WinFillRect( hps, &rcl, clrBG );

    // draw the day/night indicator, if applicable
    if ( pdata->flOptions & WTF_PLACE_HAVECOORD ) {
        ptl.x = rclRight.xRight - 21;
        if ( IS_DAYTIME( pdata->timeval, pdata->tm_sunrise, pdata->tm_sunset )) {
            if ( pdata->hptrDay ) {
                ptl.y = max( 1, rclRight.yBottom + (lHeight / 2) - 10 );
                WinDrawPointer( hps, ptl.x, ptl.y, pdata->hptrDay, DP_NORMAL | DP_MINI );
            }
            else {
                ptl.y = rclRight.yBottom + (lHeight / 2) + 8;
                GpiMove( hps, &ptl );
                DrawDayIndicatorSmall( hps );
            }
        }
        else {
            if ( pdata->hptrNight ) {
                ptl.y = max( 1, rclRight.yBottom + (lHeight / 2) - 10 );
                WinDrawPointer( hps, ptl.x, ptl.y, pdata->hptrNight, DP_NORMAL | DP_MINI );
            }
            else {
                ptl.y = rclRight.yBottom + (lHeight / 2) + 8;
                GpiMove( hps, &ptl );
                DrawNightIndicatorSmall( hps );
            }
        }
    }

    // draw the description string (if defined)
    if ( pdata->uzDesc[0] ) {
        pchText = bUnicode ? (PCH) pdata->uzDesc : (PCH) pdata->szDesc;
        cb = bUnicode ? UniStrlen(pdata->uzDesc) * 2 : strlen( pdata->szDesc );
        ptl.x = rclLeft.xLeft + lTxtInset;
        ptl.y = rclLeft.yBottom + lTxtOffset;
        GpiCharStringPosAt( hps, &ptl, &rclLeft, CHS_CLIP, cb, pchText, NULL );
    }

    // draw the date string
    if (( pdata->flState & WTS_CVR_DATE ) && pdata->uzDate[0] ) {
        pchText = bUnicode ? (PCH) pdata->uzDate : (PCH) pdata->szDate;
        cb = bUnicode ? UniStrlen(pdata->uzDate) * 2 : strlen( pdata->szDate );
        ptl.x = rclRight.xLeft + lTxtInset;
        ptl.y = rclRight.yBottom + lTxtOffset;
        GpiCharStringPosAt( hps, &ptl, &rclDateTime, CHS_CLIP, cb, pchText, NULL );
    }

    // draw the time string
    else if ( pdata->uzTime[0] ) {
        pchText = bUnicode ? (PCH) pdata->uzTime : (PCH) pdata->szTime;
        cb = bUnicode ? UniStrlen(pdata->uzTime) * 2 : strlen( pdata->szTime );
        ptl.x = rclRight.xLeft + lTxtInset;
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

    }
    else if ( pdata->flOptions & WTF_TIME_CUSTOM ) {
        // if the flags indicate a custom string but no custom string is set,
        // initialize it to the default
        if ( ! pdata->uzTimeFmt[0] ) UniStrcpy( pdata->uzTimeFmt, L"%X");

    }
    else if ( pdata->flOptions & WTF_TIME_SHORT ) {
        // WTF_TIME_SHORT only applies to normal and system time mode (and
        // system will handle it separately at formatting time).
        // Unfortunately, there is no regular locale item for "default short
        // time format", so we have to manually strip the seconds out of the
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
        }
        else UniStrcpy( pdata->uzDateFmt, L"%x");

    }
    else if ( pdata->flOptions & WTF_DATE_CUSTOM ) {
        if ( ! pdata->uzDateFmt[0] ) UniStrcpy( pdata->uzDateFmt, L"%x");

    }
    else
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
 * RETURNS:                                                                  *
 * TRUE if the displayable time has changed (and thus should be redrawn);    *
 * FALSE otherwise.                                                          *
 * ------------------------------------------------------------------------- */
BOOL FormatTime( HWND hwnd, PWTDDATA pdata, struct tm *time, UniChar *puzTime, PSZ pszTime )
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

    if ( UniStrcmp( uzFTime, puzTime ) != 0 ) {
        UniStrcpy( puzTime, uzFTime );
        if (( rc = UniStrFromUcs( pdata->uconv, pszTime,
                                  puzTime, TIMESTR_MAXZ )) != ULS_SUCCESS )
            pszTime[0] = 0;
        return TRUE;
    }
    return FALSE;
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
 * RETURNS:                                                                  *
 * TRUE if the date has changed (meaning the display should be redrawn);     *
 * FALSE otherwise.                                                          *
 * ------------------------------------------------------------------------- */
BOOL FormatDate( HWND hwnd, PWTDDATA pdata, struct tm *time )
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
        return TRUE;
    }
    return FALSE;
}


/* ------------------------------------------------------------------------- *
 * SetSunTimes                                                               *
 *                                                                           *
 * Calculates the sunrise/sunset times.                                      *
 *                                                                           *
 * ------------------------------------------------------------------------- */
void SetSunTimes( HWND hwnd, PWTDDATA pdata )
{
    double     lon, lat,        // geographic coordinate values
               rise, set;       // sunrise/sunset times (in hours UTC)
    CHAR       szEnv[ TZSTR_MAXZ+4 ] = {0};
    time_t     utime,           // calendar time (UTC)
               midnight;        // calendar time of local midnight
    struct tm *ltime;           // broken-down local time
    BOOL       fHasTZ = FALSE;  // is a TZ variable defined for this clock?

    if ( !( pdata->flOptions & WTF_PLACE_HAVECOORD )) {
        pdata->tm_sunrise = 0;
        pdata->tm_sunset = 0;
        pdata->uzSunS[0] = 0;
        pdata->szSunS[0] = 0;
        pdata->uzSunR[0] = 0;
        pdata->szSunR[0] = 0;
        return;
    }

    if ( pdata->szTZ[0] ) {
        sprintf( szEnv, "TZ=%s", pdata->szTZ );
        putenv( szEnv );
        tzset();
        fHasTZ = TRUE;
    }
    utime = pdata->timeval;
    ltime = fHasTZ? localtime( &utime ): gmtime( &utime );
    if ( !ltime ) return;

    // Get the coordinates into the expected format and calculate the sun times
    lat = DECIMAL_DEGREES( pdata->coordinates.sLatitude, pdata->coordinates.bLaMin, 0 );
    lon = DECIMAL_DEGREES( pdata->coordinates.sLongitude, pdata->coordinates.bLoMin, 0 );
    sun_rise_set( 1900 + ltime->tm_year, 1 + ltime->tm_mon, ltime->tm_mday,
                  lon, lat, &rise, &set );

    /* Get the calendar time of midnight (at day's start).  Note we have to
     * factor in the zone offset (defined in IBM C runtime variable _timezone),
     * in case local time falls in a different day than UTC.
     * (This must be done in two steps or the macro will return wrong results.)
     */
    midnight = pdata->timeval - _timezone;
    midnight = ROUND_TO_MIDNIGHT( midnight );

//    _PmpfF(("(%s) Rise %f, Set: %f, Zone offset: %d, midnight: %f", pdata->szDesc, rise, set, _timezone, midnight ));

    // Convert sunrise/sunset from decimal hours to full calendar time
    pdata->tm_sunrise = midnight + floor( rise * 3600 );
    pdata->tm_sunset  = midnight + floor( set * 3600 );

    // These calendar times are all in UTC, of course.
    // Convert sunrise into local time and generate the formatted string
    utime = pdata->tm_sunrise;
    ltime = fHasTZ? localtime( &utime ): gmtime( &utime );
    FormatTime( hwnd, pdata, ltime, pdata->uzSunR, pdata->szSunR );

    // And the same for sunset
    utime = pdata->tm_sunset;
    ltime = fHasTZ? localtime( &utime ): gmtime( &utime );
    FormatTime( hwnd, pdata, ltime, pdata->uzSunS, pdata->szSunS );

    _PmpfF(("(%s) sunrise: %s, sunset: %s (currently: %s)", pdata->szDesc, pdata->szSunR, pdata->szSunS, pdata->szTime ));
    _PmpfF(("     sunrise: %f, sunset: %f (currently %f)", pdata->tm_sunrise, pdata->tm_sunset, pdata->timeval ));

    // Force a redraw of the indicator area
    if ( pdata->flOptions & WTF_MODE_COMPACT )
        WinInvalidateRect( hwnd, &(pdata->rclDate), FALSE );
    else
        WinInvalidateRect( hwnd, &(pdata->rclTime), FALSE );
}


/* FOR QUICK REFERENCE:
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


/* ------------------------------------------------------------------------- *
 * DrawSunriseIndicator                                                      *
 * ------------------------------------------------------------------------- */
void DrawSunriseIndicator( HPS hps )
{
    static BYTE abImage[] = { 0x00, 0x00,
                              0x00, 0x00,
                              0x00, 0x00,
                              0x00, 0x00,
                              0x02, 0x00,       // ......1. ........
                              0x02, 0x00,       // ......1. ........
                              0x42, 0x10,       // .1....1. ...1....
                              0x2F, 0xA0,       // ..1.1111 1.1.....
                              0x1F, 0xC0,       // ...11111 11......
                              0x3F, 0xE0,       // ..111111 111.....
                              0x3F, 0xE0,       // ..111111 111.....
                              0xFF, 0xF8,       // 11111111 11111...
                              0x00, 0x00,
                              0x00, 0x00,
                              0x00, 0x00,
                              0x00, 0x00
                             };
    SIZEL sizl;

    sizl.cx = 16;
    sizl.cy = 16;
    GpiImage( hps, 0L, &sizl, sizeof( abImage ), abImage );
}


/* ------------------------------------------------------------------------- *
 * DrawSunsetIndicator                                                       *
 * ------------------------------------------------------------------------- */
void DrawSunsetIndicator( HPS hps )
{
    static BYTE abImage[] = { 0x00, 0x00,
                              0x00, 0x00,
                              0x00, 0x00,
                              0x00, 0x00,
                              0x00, 0x00,
                              0x00, 0x00,
                              0xFF, 0xF8,       // 11111111 11111...
                              0x3F, 0xE0,       // ..111111 111.....
                              0x3F, 0xE0,       // ..111111 111.....
                              0x1F, 0xC0,       // ...11111 11......
                              0x2F, 0xA0,       // ..1.1111 1.1.....
                              0x42, 0x10,       // .1....1. ...1....
                              0x02, 0x00,       // ......1. ........
                              0x02, 0x00,       // ......1. ........
                              0x00, 0x00,
                              0x00, 0x00,
                             };
    SIZEL sizl;

    sizl.cx = 16;
    sizl.cy = 16;
    GpiImage( hps, 0L, &sizl, sizeof( abImage ), abImage );
}


/* ------------------------------------------------------------------------- *
 * DrawDayIndicatorSmall                                                     *
 * ------------------------------------------------------------------------- */
void DrawDayIndicatorSmall( HPS hps )
{
    static BYTE abImage[] = { 0x00, 0x00,
                              0x00, 0x00,
                              0x00, 0x00,
                              0x01, 0x00,   // .......1 ........
                              0x01, 0x00,   // .......1 ........
                              0x13, 0x90,   // ...1..11 1..1....
                              0x0F, 0xE0,   // ....1111 111.....
                              0x0F, 0xE0,   // ....1111 111.....
                              0x7F, 0xFC,   // .1111111 111111..
                              0x0F, 0xE0,   // ....1111 111.....
                              0x0F, 0xE0,   // ....1111 111.....
                              0x13, 0x90,   // ...1..11 1..1....
                              0x01, 0x00,   // .......1 ........
                              0x01, 0x00,   // .......1 ........
                              0x00, 0x00,
                              0x00, 0x00
                             };
    SIZEL sizl;
    LONG  lFG;

    sizl.cx = 16;
    sizl.cy = 16;
    lFG = GpiQueryColor( hps );
    GpiSetColor( hps, lFG & ~0x333333 );
    GpiImage( hps, 0L, &sizl, sizeof( abImage ), abImage );
    GpiSetColor( hps, lFG );
}


/* ------------------------------------------------------------------------- *
 * DrawDayIndicatorLarge                                                     *
 * ------------------------------------------------------------------------- */
void DrawDayIndicatorLarge( HPS hps )
{
    static BYTE abImage[] = { 0x00, 0x00, 0x00, 0x00,
                              0x00, 0x00, 0x00, 0x00,
                              0x00, 0x00, 0x00, 0x00,
                              0x00, 0x00, 0x00, 0x00,
                              0x00, 0x00, 0x00, 0x00,
                              0x00, 0x01, 0x00, 0x00,   // ........ .......1 ........ ........
                              0x00, 0x01, 0x00, 0x00,   // ........ .......1 ........ ........
                              0x00, 0x01, 0x00, 0x00,   // ........ .......1 ........ ........
                              0x00, 0x01, 0x00, 0x00,   // ........ .......1 ........ ........
                              0x02, 0x01, 0x00, 0x80,   // ......1. .......1 ........ 1.......
                              0x01, 0x07, 0xC1, 0x00,   // .......1 .....111 11.....1 ........
                              0x00, 0x9F, 0xF2, 0x00,   // ........ 1..11111 1111..1. ........
                              0x00, 0x3F, 0xF8, 0x00,   // ........ ..111111 11111... ........
                              0x00, 0x7F, 0xFC, 0x00,   // ........ .1111111 111111.. ........
                              0x00, 0xFF, 0xFE, 0x00,   // ........ 11111111 1111111. ........
                              0x00, 0xFF, 0xFE, 0x00,   // ........ 11111111 1111111. ........
                              0x01, 0xFF, 0xFF, 0x00,   // .......1 11111111 11111111 ........
                              0x01, 0xFF, 0xFF, 0x00,   // .......1 11111111 11111111 ........
                              0x3F, 0xFF, 0xFF, 0xF8,   // ..111111 11111111 11111111 11111...
                              0x01, 0xFF, 0xFF, 0x00,   // .......1 11111111 11111111 ........
                              0x01, 0xFF, 0xFF, 0x00,   // .......1 11111111 11111111 ........
                              0x00, 0xFF, 0xFE, 0x00,   // ........ 11111111 1111111. ........
                              0x00, 0xFF, 0xFE, 0x00,   // ........ 11111111 1111111. ........
                              0x00, 0x7F, 0xFC, 0x00,   // ........ .1111111 111111.. ........
                              0x00, 0x3F, 0xF8, 0x00,   // ........ ..111111 11111... ........
                              0x00, 0x9F, 0xF2, 0x00,   // ........ 1..11111 1111..1. ........
                              0x01, 0x07, 0xC1, 0x00,   // .......1 .....111 11.....1 ........
                              0x02, 0x01, 0x00, 0x80,   // ......1. .......1 ........ 1.......
                              0x00, 0x01, 0x00, 0x00,   // ........ .......1 ........ ........
                              0x00, 0x01, 0x00, 0x00,   // ........ .......1 ........ ........
                              0x00, 0x01, 0x00, 0x00,   // ........ .......1 ........ ........
                              0x00, 0x01, 0x00, 0x00    // ........ .......1 ........ ........
                             };
    SIZEL sizl;
    LONG  lFG;

    sizl.cx = 32;
    sizl.cy = 32;
    lFG = GpiQueryColor( hps );
    GpiSetColor( hps, lFG & ~0x666666 );
    GpiImage( hps, 0L, &sizl, sizeof( abImage ), abImage );
    GpiSetColor( hps, lFG );
}


/* ------------------------------------------------------------------------- *
 * DrawNightIndicatorSmall                                                   *
 * ------------------------------------------------------------------------- */
void DrawNightIndicatorSmall( HPS hps )
{
    static BYTE abImage[] = { 0x00, 0x00,
                              0x00, 0x00,
                              0x00, 0x00,
                              0x03, 0x80,   // ......11 1.......
                              0x20, 0xE0,   // ..1..... 111.....
                              0x00, 0xF0,   // ........ 1111....
                              0x00, 0x70,   // ........ .111....
                              0x00, 0x78,   // ........ .1111...
                              0x00, 0x78,   // ........ .1111...
                              0x00, 0xF8,   // ........ 11111...
                              0x31, 0xF0,   // ..11...1 1111....
                              0x1F, 0xF0,   // ...11111 1111....
                              0x0F, 0xE0,   // ....1111 111.....
                              0x03, 0x80,   // ......11 1.......
                              0x00, 0x00,
                              0x00, 0x00
                             };
    SIZEL sizl;
    LONG  lFG;

    sizl.cx = 16;
    sizl.cy = 16;
    lFG = GpiQueryColor( hps );
    GpiSetColor( hps, lFG & ~0x333333 );
    GpiImage( hps, 0L, &sizl, sizeof( abImage ), abImage );
    GpiSetColor( hps, lFG );
}


/* ------------------------------------------------------------------------- *
 * DrawNightIndicatorLarge                                                   *
 * ------------------------------------------------------------------------- */
void DrawNightIndicatorLarge( HPS hps )
{
    static BYTE abImage[] = { 0x00, 0x00, 0x00, 0x00,
                              0x00, 0x00, 0x00, 0x00,
                              0x00, 0x00, 0x00, 0x00,
                              0x00, 0x00, 0x00, 0x00,
                              0x00, 0x00, 0x00, 0x00,
                              0x00, 0x00, 0x00, 0x40,   // ........ ........ ........ .1......
                              0x00, 0x0F, 0x80, 0x00,   // ........ ....1111 1....... ........
                              0x02, 0x03, 0xF0, 0x00,   // ......1. ......11 1111.... ........
                              0x05, 0x01, 0xFC, 0x00,   // .....1.1 .......1 111111.. ........
                              0x02, 0x00, 0xFE, 0x00,   // ......1. ........ 1111111. ........
                              0x00, 0x00, 0x7F, 0x00,   // ........ ........ .1111111 ........
                              0x00, 0x00, 0x3F, 0x00,   // ........ ........ ..111111 ........
                              0x00, 0x00, 0x3F, 0x80,   // ........ ........ ..111111 1.......
                              0x00, 0x00, 0x1F, 0x80,   // ........ ........ ...11111 1.......
                              0x00, 0x00, 0x1F, 0xC0,   // ........ ........ ...11111 11......
                              0x00, 0x00, 0x1F, 0xC0,   // ........ ........ ...11111 11......
                              0x00, 0x00, 0x1F, 0xC0,   // ........ ........ ...11111 11......
                              0x00, 0x00, 0x1F, 0xC0,   // ........ ........ ...11111 11......
                              0x00, 0x00, 0x3F, 0xC0,   // ........ ........ ..111111 11......
                              0x08, 0x00, 0x3F, 0xC0,   // ....1... ........ ..111111 11......
                              0x0C, 0x00, 0x7F, 0x80,   // ....11.. ........ .1111111 1.......
                              0x07, 0x01, 0xFF, 0x80,   // .....111 .......1 11111111 1.......
                              0x07, 0xFF, 0xFF, 0x00,   // .....111 11111111 11111111 ........
                              0x03, 0xFF, 0xFF, 0x00,   // ......11 11111111 11111111 ........
                              0x01, 0xFF, 0xFE, 0x00,   // .......1 11111111 1111111. ........
                              0x00, 0xFF, 0xFC, 0x00,   // ........ 11111111 111111.. ........
                              0x00, 0x3F, 0xF0, 0x00,   // ........ ..111111 1111.... ........
                              0x00, 0x0F, 0xC0, 0x10,   // ........ ....1111 11...... ...1....
                              0x00, 0x00, 0x00, 0x00,
                              0x00, 0x00, 0x00, 0x00,
                              0x00, 0x00, 0x00, 0x00,
                              0x00, 0x00, 0x00, 0x00
                            };
    SIZEL sizl;
    LONG  lFG;

    sizl.cx = 32;
    sizl.cy = 32;
    lFG = GpiQueryColor( hps );
    GpiSetColor( hps, lFG & ~0x666666 );
    GpiImage( hps, 0L, &sizl, sizeof( abImage ), abImage );
    GpiSetColor( hps, lFG );
}

