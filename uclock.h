/****************************************************************************
 * uclock.h                                                                 *
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


// ----------------------------------------------------------------------------
// MACROS

// Handy message box for errors and debug messages
#define ErrorPopup( text ) \
    WinMessageBox( HWND_DESKTOP, HWND_DESKTOP, text, "Error", 0, MB_OK | MB_ERROR )


// ----------------------------------------------------------------------------
// CONSTANTS

#define SZ_VERSION              "0.5"
#define SZ_COPYRIGHT            "2015-2019"

#define SZ_WINDOWCLASS          "UClockWindow"

#define HELP_FILE               "uclock.hlp"
#define INI_FILE                "uclock.ini"
#define ZONE_FILE               "ZONEINFO"

// min. size of the program window (not used yet)
//#define US_MIN_WIDTH            100
//#define US_MIN_HEIGHT           60

#define LOCALE_BUF_MAX          4096    // maximum length of a locale list
#define PROFILE_MAXZ            32      // maximum length of INI file name (without path)
#define TZDATA_MAXZ             128     // maximum length of a TZ profile item
#define ISO6709_MAXZ            16      // maximum length of an ISO 6709 coordinates string
#define SZRES_MAXZ              256     // maximum length of a generic string resource

// used to indicate an undefined colour presentation parameter
#define NO_COLOUR_PP            0xFF000000

// Used by the colour dialog
#define CWN_CHANGE              0x601
#define CWM_SETCOLOUR           0x602

// upper limit of the switch-to-compact-view threshold (in pixels)
#define MAX_THRESHOLD           500

// Maximum number of clock-display windows supported
#define MAX_CLOCKS              12

// Maximum number of clock-display windows allowed in one column
#define MAX_PER_COLUMN          MAX_CLOCKS

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
// MACROS

// Get separate RGB values from a long
#define RGBVAL_RED(l)           ((BYTE)((l >> 16) & 0xFF))
#define RGBVAL_GREEN(l)         ((BYTE)((l >>  8) & 0xFF))
#define RGBVAL_BLUE(l)          ((BYTE)(l & 0xFF))

// Combine separate RGB values into a long
#define RGB2LONG(r,g,b)         ((LONG)((r << 16) | (g << 8) | b ))


// ----------------------------------------------------------------------------
// TYPEDEFS

// Used by the PM colour-wheel control (on the colour-selection dialog)
typedef struct CWPARAM {
    USHORT  cb;
    RGB     rgb;
    BOOL    fCancel;
} CWPARAM;

typedef struct CWDATA {
    USHORT  updatectl;
    HWND    hwndCol;
    HWND    hwndSpinR;
    HWND    hwndSpinG;
    HWND    hwndSpinB;
    RGB     *rgb;
    RGB     rgbold;
    BOOL    *pfCancel;
} CWDATA;


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
    USHORT      aClockOrder[ MAX_CLOCKS ];  // new clock order (array of indices)
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


// Used to pass data in the timezone selection dialog
typedef struct _TZ_Properties {
    HAB         hab;                        // anchor-block handle
    HMQ         hmq;                        // main message queue
    CHAR        achDesc[ LOCDESC_MAXZ ];    // current description
    CHAR        achTZ[ TZSTR_MAXZ ];        // TZ variable string
    CHAR        achCtry[ COUNTRY_MAXZ ];    // current country selection
    CHAR        achRegn[ REGION_MAXZ ];     // current region/city selection
    GEOCOORD    coordinates;                // geographic coordinates
    UconvObject uconv;                      // Unicode (UCS-2) conversion object
    UconvObject uconv1208;                  // UTF-8 conversion object
} TZPROP, *PTZPROP;


// Contains global application data
typedef struct _Global_Data {
    HAB      hab;                       // anchor-block handle
    HMQ      hmq;                       // main message queue
    HINI     hIni;                      // program INI file
    HPOINTER hptrDay;                   // day indicator icon
    HPOINTER hptrNight;                 // night indicator icon

    HWND    clocks[ MAX_CLOCKS ];       // array of clock-display controls
    HWND    hwndTB, hwndSM, hwndMM;     // titlebar and window control handles
    HWND    hwndPopup;                  // popup menu
    USHORT  usClocks;                   // number of clocks configured
    ULONG   timer;                      // refresh timer
    CHAR    szTZ[ TZSTR_MAXZ ];         // actual system TZ
    CHAR    szLoc[ ULS_LNAMEMAX+1 ];    // actual system locale
    USHORT  usMsgClock;                 // clock which sent the latest message
    USHORT  fsStyle;                    // global style flags
    USHORT  usCompactThreshold;         // when to auto-switch to compact view
    USHORT  usPerColumn;                // no. of clocks to show per column
    USHORT  usCols;                     // number of columns required
    BYTE    bDescWidth;                 // % width of descriptions in compact view
} UCLGLOBAL, *PUCLGLOBAL;


// ----------------------------------------------------------------------------
// FUNCTION PROTOTYPES

// application logic
MRESULT EXPENTRY MainWndProc( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 );
void             ResizeClocks( HWND hwnd );
MRESULT          PaintClient( HWND hwnd );
MRESULT EXPENTRY AboutDlgProc( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 );
MRESULT EXPENTRY ClrDlgProc( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 );
BOOL             WindowSetup( HWND hwnd, HWND hwndClient );
void             OpenProfile( PUCLGLOBAL pGlobal );
void             ToggleTitleBar( PUCLGLOBAL pGlobal, HWND hwndFrame, BOOL fOn );
void             SaveSettings( HWND hwnd );
void             UpdateTime( HWND hwnd );
BOOL             AddNewClock( HWND hwnd );
void             DeleteClock( HWND hwnd, PUCLGLOBAL pGlobal, USHORT usClock );
void             UpdateClockBorders( PUCLGLOBAL pGlobal );

// application configuration
void             ConfigNotebook( HWND hwnd );
MRESULT EXPENTRY CfgDialogProc( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 );
BOOL             CfgPopulateNotebook( HWND hwnd, PUCFGDATA pConfig );
MRESULT EXPENTRY CfgCommonPageProc( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 );
void             CfgSettingsCommon( HWND hwnd, PUCFGDATA pConfig );
void             CfgPopulateClockList( HWND hwnd, PUCFGDATA pConfig );

// clock panel configuration
BOOL             ClockNotebook( HWND hwnd, USHORT usNumber );
MRESULT EXPENTRY ClkDialogProc( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 );
BOOL             ClkPopulateNotebook( HWND hwnd, PUCLKPROP pConfig );
MRESULT EXPENTRY ClkClockPageProc( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 );
MRESULT EXPENTRY ClkStylePageProc( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 );
BOOL             ClkSettingsClock( HWND hwnd, PUCLKPROP pConfig );
BOOL             ClkSettingsStyle( HWND hwnd, PUCLKPROP pConfig );
MRESULT EXPENTRY TZDlgProc( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 );
void             TZPopulateCountryZones( HWND hwnd, HINI hTZDB, PSZ pszCtry, PTZPROP pProps );
void             SelectTimeZone( HWND hwnd, PUCLKPROP pConfig );
BOOL             ParseTZCoordinates( PSZ pszCoord, PGEOCOORD pGC );
void             UnParseTZCoordinates( PSZ pszCoord, GEOCOORD coordinates );

// utils
void             CentreWindow( HWND hwnd );
void             ErrorMessage( HWND hwnd, USHORT usID );
BOOL             LoadIniData( PVOID pData, USHORT cb, HINI hIni, PSZ pszApp, PSZ pszKey );
SHORT            MoveListItem( HWND hwndList, SHORT sItem, BOOL fMoveUp );
BOOL             SelectFont( HWND hwnd, PSZ pszFace, USHORT cbBuf );
void             SelectColour( HWND hwnd, HWND hwndCtl );


