// uclock.h
//
// This file contains shared definitions used by both the main application
// window (uclock.c) and the custom control "WTDisplayPanel" (wtdisplay.c).
//

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

// ----------------------------------------------------------------------------
// MACROS

// Handy message box for errors and debug messages
#define ErrorPopup( text ) \
    WinMessageBox( HWND_DESKTOP, HWND_DESKTOP, text, "Error", 0, MB_OK | MB_ERROR )



// ----------------------------------------------------------------------------
// CONSTANTS


// GENERAL CONSTANTS
//

#define SZ_VERSION              "0.4"
#define SZ_COPYRIGHT            "2015-2019"


// The Unicode UCS-2 codepage
#define UNICODE                 1200


// Maximum string length...
#define CPNAME_MAXZ             12      // ...of a ULS codepage name
#define CPSPEC_MAXZ             32      // ...of a ULS codepage specifier
#define LOCDESC_MAXZ            128     // ...of a place name/description
#define TIMESTR_MAXZ            64      // ...of a formatted time string
#define DATESTR_MAXZ            128     // ...of a formatted date string
#define SUNTIMES_MAXZ           180     // ...of a combined sunrise/sunset string
#define TZSTR_MAXZ              64      // ...of a TZ variable string
#define STRFT_MAXZ              64      // ...of a (Uni)strftime format string
#define PRF_NTLDATA_MAXZ        32      // ...of a data item in PM_National
#define COUNTRY_MAXZ            64      // ...of a country name
#define REGION_MAXZ             64      // ...of a timezone/region/city name



// DEFINITIONS FOR THE WT_DISPLAY CONTROL
//

// Class name of the world-time display control
#define WT_DISPLAY              "WTDisplayPanel"


// Custom messages for world-time display control
#define WTD_SETTIMEZONE         ( WM_USER + 101 )
#define WTD_SETDATETIME         ( WM_USER + 102 )
#define WTD_SETLOCALE           ( WM_USER + 103 )
#define WTD_SETCOORDINATES      ( WM_USER + 104 )
#define WTD_SETOPTIONS          ( WM_USER + 105 )
#define WTD_SETFORMATS          ( WM_USER + 106 )
#define WTD_SETCOUNTRYZONE      ( WM_USER + 107 )
#define WTD_SETWEATHER          ( WM_USER + 108 )
#define WTD_SETSEPARATOR        ( WM_USER + 109 )
#define WTD_QUERYCONFIG         ( WM_USER + 110 )
#define WTD_QUERYOPTIONS        ( WM_USER + 111 )
#define WTD_SETINDICATORS       ( WM_USER + 112 )


// Custom presentation parameters for the world-time display control
#define PP_SEPARATORCOLOR       ( PP_USER + 1 )


/* Values used by the flOptions field in WTDDATA.  Each nybble represents a
 * different category of (mostly mutually-exclusive) options:
 *
 * 0x00000000
 *   ³³³³³³³À display mode/size (0 == default size)
 *   ³³³³³³ÀÄ border lines      (0 == no border)
 *   ³³³³³ÀÄÄ render style      (0 == digital clock, default font, GPI rendering)
 *   ³³³³ÀÄÄÄ place settings    (0 == no location info available)
 *   ³³³ÀÄÄÄÄ time display      (0 == use locale's standard time format)
 *   ³³ÀÄÄÄÄÄ TBD
 *   ³ÀÄÄÄÄÄÄ date display      (0 == use locale's standard date format)
 *   ÀÄÄÄÄÄÄÄ long date display (0 == use locale's long date format)
 */
// * == not yet implemented (for future use)
#define WTF_MODE_LARGE          0x01        // * extra-large display mode
#define WTF_MODE_COMPACT        0x02        // compact display mode
#define WTF_MODE_TINY           0x04        // * extra-compact display mode
#define WTF_BORDER_LEFT         0x10        // draw a line on the left edge
#define WTF_BORDER_RIGHT        0x20        // draw a line on the right edge
#define WTF_BORDER_TOP          0x40        // draw a line on the top edge
#define WTF_BORDER_BOTTOM       0x80        // draw a line on the bottom edge
#define WTF_BORDER_FULL         0xF0        // draw all four borders
//#define WTF_CLOCK_BMP1          0x100       // * use style-1 bitmaps for clock numbers
//#define WTF_CLOCK_BMP2          0x200       // * use style-2 bitmaps for clock numbers
#define WTF_SPECIAL_CAIRO       0x400       // * use Cairo for rendering instead of standard GPI
#define WTF_SPECIAL_ANALOG      0x800       // * use an analog-style clock
#define WTF_PLACE_HAVECOORD     0x1000      // we have geographic coordinates for this clock
#define WTF_PLACE_HAVECITY      0x2000      // * we have a city name for this clock
#define WTF_PLACE_HAVEWEATHER   0x4000      // * we can determine current weather for this clock
#define WTF_TIME_ALT            0x10000     // use locale's alternate time format
#define WTF_TIME_SYSTEM         0x20000     // use system default time format
#define WTF_TIME_CUSTOM         0x40000     // use a customized time format
#define WTF_TIME_SHORT          0x80000     // don't show seconds in time (ignored for custom & alt)
#define WTF_DATE_ALT            0x1000000   // use locale's alternate date format
#define WTF_DATE_SYSTEM         0x2000000   // use system default date format
#define WTF_DATE_CUSTOM         0x4000000   // use a customized date format
#define WTF_LONGDATE_ALT        0x10000000  // * use locale's alternate long date format
#define WTF_LONGDATE_SYSTEM     0x20000000  // * use system default long date format
#define WTF_LONGDATE_CUSTOM     0x40000000  // * use a customized long date format


/* Values used by the flState field in WTDDATA.  Some of these are used to
 * indicate the current display state of fields which can be clicked to
 * cycle through various data.  Others indicate miscellaneous GUI states
 * (e.g. keyboard focus, popup menu active, etc.).
 *
 * 0x00000000
 *   ³³³³³³³À description/top area         (0 == show city name)
 *   ³³³³³³ÀÄ time/middle area             (0 == show (normal) time)
 *   ³³³³³ÀÄÄ date/bottom area             (0 == show (normal) date)
 *   ³³³³ÀÄÄÄ right area in compact view   (0 == show (normal) time)
 *   ³³³ÀÄÄÄÄ misc. indicators             (0 == none)
 *   ³³ÀÄÄÄÄÄ TBD
 *   ³ÀÄÄÄÄÄÄ GUI state indicators         (0 == default)
 *   ÀÄÄÄÄÄÄÄ GUI state indicators         (0 == default)
 *
 */
// * == not yet implemented
#define WTS_TOP_TZNAME          0x1         // * show TZ description text in top area
#define WTS_TOP_TZVAR           0x2         // * show TZ variable in top area
#define WTS_TOP_COORDINATES     0x4         // * show geographic coordinates in top area
//#define WTS_MID_ALTTIME         0x10        // show user's alternate time in middle area
//#define WTS_BOT_ALTDATE         0x100       // show user's alternate date in bottom area
#define WTS_BOT_SUNTIME         0x200       // show sunrise/sunset times in bottom area
#define WTS_CVR_DATE            0x1000      // show (normal) date in compact view right
#define WTS_CVR_SUNTIME         0x2000      // show sunrise/sunset times in compact view right
//#define WTS_MIS_WEATHER         0x10000     // * display weather icon
#define WTS_GUI_HILITE          0x1000000   // draw focus (highlighted) state on panel
#define WTS_GUI_MENUPOPUP       0x2000000   // draw "active popup menu" border on panel
#define WTS_GUI_FOCUS1          0x10000000  // draw focus for first sub-area (left or top)
#define WTS_GUI_FOCUS2          0x20000000  // draw focus for second sub-area (mid-left)
#define WTS_GUI_FOCUS3          0x40000000  // draw focus for third sub-area (mid-right)
#define WTS_GUI_FOCUS4          0x80000000  // draw focus for fourth sub-area (right or bottom)
#define WTS_GUI_FOCUSALL        0xFF000000  // shorthand for all focus bits

// ----------------------------------------------------------------------------
// TYPEDEFS

// system default time & date formatting data from OS2.INI PM_(Default_)National
typedef struct _PM_National_Data {
    BYTE        iTime;                    // time format (0=12hr, 1=24hr)
    BYTE        iDate;                    // date format (0:MDY, 1:DMY, 2:YMD)
    CHAR        szAM[ PRF_NTLDATA_MAXZ ]; // AM string
    CHAR        szPM[ PRF_NTLDATA_MAXZ ]; // PM string
    CHAR        szTS[ PRF_NTLDATA_MAXZ ]; // time separator (e.g. ":")
    CHAR        szDS[ PRF_NTLDATA_MAXZ ]; // date separator (e.g. "/")
} NATIONFMT, *PNATIONFMT;


// Geographical coordinates (latitude & longitude)
// Don't bother with seconds - even at its widest point, one minute is less
// than 2 km, and that should be more than precise enough for our purposes.
typedef struct _Geo_Coordinates {
    SHORT       sLatitude;          // degrees latitude (-90 <-> 90)
    BYTE        bLaMin;             // minutes latitude (0-60)
    SHORT       sLongitude;         // degrees longitude (-180 <-> 180)
    BYTE        bLoMin;             // minutes longitude (0-60)
} GEOCOORD, *PGEOCOORD;


// Configurable data for the world-time display, used by WM_CREATE/WM_QUERYCONFIG
// (and also for saving clock settings to the INI file).
typedef struct _WTDisplay_CData {
    USHORT        cb;                       // size of this data structure
    ULONG         flOptions;                // option settings
    CHAR          szTZ[ TZSTR_MAXZ ];       // timezone (TZ string)
    CHAR          szLocale[ ULS_LNAMEMAX ]; // formatting locale name
    GEOCOORD      coordinates;              // geographic coordinates
    UniChar       uzDesc[ LOCDESC_MAXZ ];   // timezone/place description
    UniChar       uzTimeFmt[ STRFT_MAXZ ];  // UniStrftime time format string
    UniChar       uzDateFmt[ STRFT_MAXZ ];  // UniStrftime date format string
    BYTE          bSep;                     // width of description area as % of total width
    UniChar       uzTZCtry[ COUNTRY_MAXZ ]; // last selected timezone country
    UniChar       uzTZRegn[ REGION_MAXZ ];  // last selected timezone region
} WTDCDATA, *PWTDCDATA;

// Stub version of the above, used for fallback if we have a size mismatch
// with the saved INI data; these fields should always be present regardless
// of program version.
typedef struct _WTDisplay_CStub {
    USHORT        cb;                       // size of this data structure
    ULONG         flOptions;                // option settings
    CHAR          szTZ[ TZSTR_MAXZ ];       // timezone (TZ string)
    CHAR          szLocale[ ULS_LNAMEMAX ]; // formatting locale name
    GEOCOORD      coordinates;              // geographic coordinates
    UniChar       uzDesc[ LOCDESC_MAXZ ];   // timezone/place description
    UniChar       uzTimeFmt[ STRFT_MAXZ ];  // UniStrftime time format string
    UniChar       uzDateFmt[ STRFT_MAXZ ];  // UniStrftime date format string
} WTDCSTUB, *PWTDCSTUB;

// Full control data for the world-time display
typedef struct _WTDisplay_Data {
    time_t        timeval;                  // current time (UTC)
    time_t        tm_sunrise;               // sunrise time (UTC)
    time_t        tm_sunset;                // sunset time (UTC)
    ULONG         flOptions;                // option settings
    ULONG         flState;                  // state flags
    LocaleObject  locale;                   // formatting locale
    UconvObject   uconv;                    // codepage conversion object
    GEOCOORD      coordinates;              // geographic coordinates
    NATIONFMT     sysTimeFmt;               // system default time format data
    CHAR          szTZ[ TZSTR_MAXZ ];       // timezone (TZ string)
    CHAR          szDesc[ LOCDESC_MAXZ ];   // timezone/place description (codepage version)
    CHAR          szTime[ TIMESTR_MAXZ ];   // formatted time string      (codepage version)
    CHAR          szDate[ DATESTR_MAXZ ];   // formatted date string      (codepage version)
    CHAR          szSunR[ TIMESTR_MAXZ ];   // formatted sunrise time     (codepage version)
    CHAR          szSunS[ TIMESTR_MAXZ ];   // formatted sunset time      (codepage version)
    UniChar       uzDesc[ LOCDESC_MAXZ ];   // timezone/place description (Unicode version)
    UniChar       uzTime[ TIMESTR_MAXZ ];   // formatted time string      (Unicode version)
    UniChar       uzDate[ DATESTR_MAXZ ];   // formatted date string      (Unicode version)
    UniChar       uzSunR[ TIMESTR_MAXZ ];   // formatted sunrise time     (Unicode version)
    UniChar       uzSunS[ TIMESTR_MAXZ ];   // formatted sunset time      (Unicode version)
    UniChar       uzTimeFmt[ STRFT_MAXZ ];  // UniStrftime time format string
    UniChar       uzDateFmt[ STRFT_MAXZ ];  // UniStrftime date format string
    UniChar       uzTZCtry[ COUNTRY_MAXZ ]; // last selected timezone country
    UniChar       uzTZRegn[ REGION_MAXZ ];  // last selected timezone region
    BOOL          fUnicode;                 // using Unicode font?
    FATTRS        faNormal;                 // attributes of normal display font
    FATTRS        faTime;                   // attributes of time display font
    BYTE          bSep;                     // width of description area as % of total width
    RECTL         rclDesc;                  // defines the description area
    RECTL         rclTime;                  // defines the time display area
    RECTL         rclDate;                  // defines the date display area
    HPOINTER      hptrDay;                  // daytime indicator icon
    HPOINTER      hptrNight;                // nighttime indicator icon
} WTDDATA, *PWTDDATA;



// ----------------------------------------------------------------------------
// FUNCTION PROTOTYPES

MRESULT EXPENTRY WTDisplayProc( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 );

