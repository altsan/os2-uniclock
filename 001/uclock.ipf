:userdoc.

.*****************************************************************************
:h1 x=left y=bottom width=100% height=100% res=001.Introduction
:p.Universal Clock is a multi-timezone &osq.world clock&csq. application for
OS/2.

:p.Universal Clock allows you to configure multiple clock panels, each one
representing a timezone of your choice. You can also associate each clock panel
with specific geographic coordinates, which enables a day/night indicator
for that location. Each clock panel may also use its own font, colours,
and formatting preferences.

:p.See :link reftype=hd res=100.Using Universal Clock:elink. for more
information.

.*****************************************************************************
:h1 x=left y=bottom width=100% height=100% res=100.Using Universal Clock
:p.The Universal Clock application consists of up to 12 clock panels, which
can be arranged vertically, horizontally, or in a grid.

:p.You can add, delete, or modify these clock panels according to your
preferences.

:p.Each clock panel has two possible &osq.view&csq. modes&colon.
:ul compact.
:li.:link reftype=hd res=120.Default view:elink.
:li.:link reftype=hd res=130.Compact view:elink.
:eul.
:p.The current view is selected automatically depending on the
window size&colon. if the Universal Clock window is reduced below a certain
size (which can be customized in the program settings), the clock panels will
automatically switch over to compact view.

:p.See also&colon.
:ul compact.
:li.:link reftype=hd res=110.The initial configuration:elink.
:li.:link reftype=hd res=140.Adding and removing clocks:elink.
:li.:link reftype=hd res=150.Clock properties:elink.
:li.:link reftype=hd res=160.Program settings:elink.
:eul.

.* ---------------------------------------------------------------------------
:h2 x=left y=bottom width=100% height=100% res=110.The initial configuration
:p.:artwork name='en_default.bmp'.

:p.The first time you run Universal Clock, it will start up with a default
initial configuration. This consists of two clock panels&colon.
&osq.(Local Time)&csq., which is configured with the current system
timezone; and &osq.(Coordinated Universal Time)&csq., which shows standard
UTC time.

:p.Both of these clocks will initially use your system's default window colours
and locale conventions, and the WarpSans font (if available). You can customize
or delete either of these clocks, and/or add new clocks.

:p.You can also configure the global program settings, which control the
behaviour of the Universal Clock program as a whole.

:p.See&colon.
:ul compact.
:li.:link reftype=hd res=140.Adding and removing clocks:elink.
:li.:link reftype=hd res=150.Clock properties:elink.
:li.:link reftype=hd res=160.Program settings:elink.
:eul.


.* ---------------------------------------------------------------------------
:h2 x=left y=bottom width=100% height=100% res=120.Default view
:p.:artwork name='en_medium.bmp'.

:p.The default clock view consists of three display areas&colon.
a description area at the top, the time display area in the middle, and an
additional information area at the bottom.

:p.:hp5.Description Area:ehp5.
:p.The description area shows the title which has been assigned to this
clock. This can be configured in the clock settings.

:p.A thin horizontal line separates the description area from the rest of
the clock panel.

:p.:hp5.Time Display Area:ehp5.
:p.The time area displays the current time in the timezone for which this
clock has been configured. The exact format of the time display depends on
the clock's localization settings.

:p.If geographic coordinates are enabled for this clock, then a graphical
day/night indicator will also be shown in this area, to the right of the time
display. This indicates whether it is currently daytime or night-time in the
configured location.

:p.:hp5.Bottom Area:ehp5.
:p.The bottom area normally shows the current date in the configured timezone.
Like the time display, the date format depends on the clock's localization
settings.

:p.If geographic coordinates are enabled for this clock, then clicking on the
text in this area (or pressing Space while this area is selected) will toggle
the display between showing the current date, and showing the current date's
sunrise and sunset times in the configured location.


.* ---------------------------------------------------------------------------
:h2 x=left y=bottom width=100% height=100% res=130.Compact view
:p.:artwork name='en_compact.bmp'.

:p.The compact clock view consists of left and right display areas, separated
by a thin vertical line.

:p.:hp5.Left Area:ehp5.
:p.The left display area shows the title which has been assigned to this
clock. This can be configured in the clock settings.

:p.:hp5.Right Area:ehp5.
:p.The right display area shows either the current time, or the current
date, in this clock's timezone. Clicking anywhere in the right display area
(or pressing Space while this area is selected) will switch between the two.

:p.The exact time and date formats will depend, as always, on the clock's
localization settings.

:p.If geographic coordinates are enabled for this clock, then a graphical
day/night indicator will also be shown in this area, to the right of the time
or date. This indicates whether it is currently daytime or night-time in the
configured location.


.* ---------------------------------------------------------------------------
:h2 x=left y=bottom width=100% height=100% res=140.Adding and removing clocks
:p.To add a new clock panel, click mouse button 2 on any existing clock panel,
and choose :hp1.Add clock:ehp1. from the popup menu. The
:link reftype=hd res=150.clock properties:elink. notebook will open.
Configure the desired settings for the new clock, and select :hp1.OK:ehp1.
when done. The new clock panel will be added to the Universal Clock window.

:p.To remove an existing clock, click mouse button 2 on the clock panel you
want to delete, and select :hp1.Delete clock:ehp1. from the popup menu. You
will be prompted for confirmation; select :hp1.Yes:ehp1. and the clock panel
will be removed from Universal Clock.

:nt.If you remove all clock panels and then close Universal Clock, the next
time you start the program it will revert to the
:link reftype=hd res=110.initial configuration:elink.:ent.


.* ---------------------------------------------------------------------------
:h2 x=left y=bottom width=100% height=100% res=150.Clock properties
:p.Each clock panel has its own configuration settings. To configure a clock,
click mouse button 2 on that clock, and select :hp1.Clock properties:ehp1.
from the popup menu. This brings up the clock properties notebook.

:p.This notebook has two pages&colon. :link reftype=hd res=151.Clock
setup:elink. and :link reftype=hd res=152.Appearance:elink..

.* ...........................................................................
:h3 x=left y=bottom width=100% height=100% res=151.Clock setup
:p.This page controls the clock settings.
:dl break=fit tsize=20.

:dt.Description
:dd.A short, free-form name describing what time this clock represents. This
will be displayed in the clock panel along with the time/date information.

:dt.Timezone
:dd.This is the formatted timezone specifier that determines the time shown
in this clock. If you know the desired specifier, you can enter it manually;
otherwise, clicking the &osq.&per.&per.&per.&csq. button will bring up the
:link reftype=hd res=153.timezone selection dialog:elink., which allows you
to select a timezone from a list of geographic locations.

:dt.Coordinates
:dd.Selecting this checkbox will enable geographic coordinates for this
clock. The coordinates themselves are selected along with the city in
the timezone selection dialog. Enabling geographic coordinates will in turn
enable sunrise/sunset times and the day/night indicator for this clock.

:dt.Locale
:dd.The locale affects the time and date formatting preferences for this
clock (see the next two items). The default is the current system or
process locale (a.k.a. the %LANG% setting). This is a cosmetic preference
and has no relation to the timezone settings for this clock.

:dt.Time format
:dd.This setting controls the formatting of the time display for this
clock. You can select from four basic format styles&colon.
:dl tsize=25.
:dt.System default
:dd.Formats the time according to the system-wide &osq.PM_National&csq.
conventions for Presentation Manager applications.
:dt.Locale default
:dd.Formats the time according to the defaults for the locale selected
under :hp1.Locale:ehp1. (see above).
:dt.Locale alternative
:dd.Some locales (mostly those with non-Latin writing systems) define
an &osq.alternative&csq. format for time display, usually one that uses
native characters and symbols. This option formats the time according to
that format, if the locale (as selected under :hp1.Locale:ehp1.) defines
it. If the selected locale does not define an alternative time format,
then this option is the same as &osq.Locale default&csq..
:dt.Custom string
:dd.This option allows you to enter a custom time format string using
:link reftype=fn refid=strftime.strftime:elink. syntax.
:edl.
:p.For the &osq.System default&csq. and &osq.Locale default&csq.
format styles, you have the option of disabling the seconds display.
(Due to the extreme variability of the &osq.Locale alternative&csq.
style, it does not support this option.)
For the &osq.Custom string&csq. style, display of seconds is controlled
by the specified format string.

:dt.Date format
:dd.This setting controls the formatting of the date display for this
clock. You can select from four basic format styles&colon.
:dl tsize=25.
:dt.System default
:dd.Formats the date according to the system-wide &osq.PM_National&csq.
conventions for Presentation Manager applications.
:dt.Locale default
:dd.Formats the date according to the defaults for the locale selected
under :hp1.Locale:ehp1. (see above).
:dt.Locale alternative
:dd.This option formats the date according to the selected locale's
&osq.alternative&csq. date format, if it defines one. If it does not, then
this option is the same as &osq.Locale default&csq..
:dt.Custom string
:dd.This option allows you to enter a custom date format string using
:link reftype=fn refid=strftime.strftime:elink. syntax.
:edl.
:edl.

.* ...........................................................................
:h3 x=left y=bottom width=100% height=100% res=152.Appearance
:p.This page controls the clock's colour and font settings.

:dl break=fit tsize=20.

:dt.Font
:dd.Controls the display font used by this clock. You can drag a font
into the preview area from the OS/2 Font Palette, or select a font using
the :hp1.Font:ehp1. button.
:p.The font size is determined automatically by the size of the clock panel
(in both standard and compact views). However, keep in mind that bitmap
fonts are not infinitely scalable, so selecting a bitmap font will limit the
extent to which the font size can change.
:p.If you select a :link reftype=fn refid=unicode_font.Unicode-capable
font:elink., the panel text will automatically be rendered in Unicode rather
than the current codepage. This is generally desirable if you use descriptions
or formatting strings that contain characters not normally supported by your
system codepage.

:dt.Colours
:dd.Controls various colours used by this clock. For each colour setting,
you can drag a colour into the preview area from one the OS/2 colour palettes,
or use the buttons to select a colour.

.* Background
.* Foreground
.* Borders
.* Separator line
:edl.


:fn id=strftime.
:p.The format string syntax is as per the :hp2.UniStrftime:ehp2. function in
the Universal Language Support API. It is summarized in brief below.

:p.The format string contains two types of objects&colon. literal characters
and substitution symbols (also known as conversion specifiers). Substitution
symbols consist of a &percent. (percent sign), followed by one or two characters
which indicate what value will be inserted into the text at this position.
The available substitution symbols are&colon.
:dl tsize=7.
:dt.&percent.a
:dd.The locale's abbreviated weekday name (e.g. Sun).
:dt.&percent.A
:dd.The locale's full weekday name (e.g. Sunday).
:dt.&percent.b
:dd.The locale's abbreviated month name (e.g. Jan).
:dt.&percent.B
:dd.The locale's full month name (e.g. January).
:dt.&percent.c
:dd.The locale's full date and time format.
:dt.&percent.C
:dd.The century as a decimal number (00-99).
:dt.&percent.d
:dd.The day of the month as a decimal number (01 to 31).
:dt.&percent.D
:dd.The date in &percent.m/&percent.d/&percent.y format (for example, 01/31/94).
:dt.&percent.e
:dd.The day of the month as a decimal number ( 1 to 31). A single digit is
preceded by a space.
:dt.&percent.h
:dd.Same as &percent.b.
:dt.&percent.H
:dd.The 24-hour-clock hour as a decimal number (00 to 23).
:dt.&percent.I
:dd.The 12-hour-clock hour as a decimal number (01 to 12).
:dt.&percent.j
:dd.The day of the year as a decimal number (001 to 366).
:dt.&percent.m
:dd.The month of the year as a decimal number (01 to 12).
:dt.&percent.M
:dd.The minute of the hour as a decimal number (00 to 59).
:dt.&percent.n
:dd.Specifies a new-line character.
:dt.&percent.p
:dd.The locale's AM or PM string.
:dt.&percent.r
:dd.The time in AM/PM notation (equivalent to "&percent.I&colon.&percent.M&colon.&percent.S&percent.p").
:dt.&percent.R
:dd.The time in 24-hour-clock notation without seconds (equivalent to "&percent.H&colon.&percent.M").
:dt.&percent.S
:dd.The second of the minute as a decimal number (00 to 61).
:dt.&percent.t
:dd.Specifies a tab character.
:dt.&percent.T
:dd.The time in 24-hour-clock notation, with seconds (equivalent to "&percent.H&colon.&percent.M&colon.&percent.S").
:dt.&percent.u
:dd.The day of the week as a decimal number (1 to 7), where 1 represents Monday.
:dt.&percent.U
:dd.The week of the year as a decimal number (00 to 53), where the week starts on Sunday.
:dt.&percent.V
:dd.The week of the year as a decimal number (01 to 53), where the week starts on Monday.
If the week containing 1 January has four or more days in the new year, then it is
considered week 1; otherwise, it is week 53 of the previous year, and the next week is week 1.
:dt.&percent.w
:dd.The day of the week as a decimal number (0 to 6), where 0 represents Sunday.
:dt.&percent.W
:dd.The week of the year as a decimal number (00 to 53), where the week starts on Monday.
All days in a new year preceding the first Sunday are considered to be week 0.
:dt.&percent.x
:dd.The locale's full date format.
:dt.&percent.X
:dd.The locale's full time format.
:dt.&percent.y
:dd.The year of the century (00 to 99).
:dt.&percent.Y
:dd.The year, including century, as a decimal number (e.g. 1994).
:dt.&percent.Z
:dd.The time-zone name or abbreviation if one can be determined (e.g. EST). Omitted
if time zone information does not exist.
:dt.&percent.&percent.
:dd.Specifies a &percent. (percent) sign.
:edl.

:p.Some conversion specifiers can be modified by the E or O modifier characters
to indicate that an alternative format or specification should be used rather
than the one normally used by the unmodified conversion specifier. If the
alternative format or specification does not exist for the current locale, the
behavior will be as if the unmodified conversion specification were used.
:dl tsize=7.
:dt.&percent.Ec
:dd.Locale's alternative date and time representation.
:dt.&percent.EC
:dd.The name of the base year (period) in the locale's alternative representation.
:dt.&percent.Ex
:dd.The locale's alternative date representation.
:dt.&percent.EX
:dd.The locale's alternative time representation.
:dt.&percent.Ey
:dd.The offset in years from &percent.EC in the locale's alternative representation.
:dt.&percent.EY
:dd.The full alternative year representation.
:dt.&percent.Od
:dd.The day of the month using the locale's alternative numeric symbols; filled as
needed with leading zeros if there is any alternative symbol for zero, otherwise with
leading spaces.
:dt.&percent.Oe
:dd.The day of the month in the locale's alternative numeric symbols, filled as needed
with leading spaces.
:dt.&percent.OH
:dd.The hour in 24-hour-clock notation, using the locale's alternative numeric symbols.
:dt.&percent.OI
:dd.The hour in 12-hour-clock notation, using the locale's alternative numeric symbols.
:dt.&percent.Om
:dd.The month using the locale's alternative numeric symbols.
:dt.&percent.OM
:dd.The minutes using the locale's alternative numeric symbols.
:dt.&percent.OS
:dd.The seconds using the locale's alternative numeric symbols.
:dt.&percent.Ou
:dd.The weekday of the year (where Monday is 1) using the locale's alternative numeric symbols.
:dt.&percent.OU
:dd.The week number of the year (where Sunday is the first day of the week) in the locale's
alternative numeric symbols.
:dt.&percent.OV
:dd.The week number of the year (where Monday is the first day of the week) in the locale's
alternative numeric symbols. If the week containing 1 January has four or more days in the
new year, then it is considered week 1; otherwise, it is week 53 of the previous year, and
the next week is week 1.
:dt.&percent.Ow
:dd.The number of the weekday (where Sunday is 0) in the locale's alternative numeric symbols.
:dt.&percent.OW
:dd.The week number of the year (where Monday is the first day of the week) in the locale's
alternative numeric symbols.
:dt.&percent.Oy
:dd.The year (offset from &percent.C) in the locale's alternative representation and using the
locale's alternative numeric symbols.
:edl.
:efn.

:fn id=unicode_font.
:p.&osq.Unicode-capable&csq. means that the font contains a Unicode
character mapping table (cmap). It does not imply anything in particular
about how many Unicode characters the font actually supports.
:p.Note that most TrueType (and OpenType) fonts are Unicode-capable, whereas
bitmap fonts and Postscript (Type 1) fonts are not.
:efn.

.* ...........................................................................
:h3 x=left y=bottom width=100% height=100% res=153.Timezone selector
:p.This dialog allows you to select a timezone by country and region.
:dl tsize=20.
:dt.Select by country
:dd.Select the country which contains the timezone you want.
:dt.Zone
:dd.Select the zone name within the selected country. (Zones are named
following the conventions of the open source :hp1.tzdata:ehp1. distribution,
in the form "Continent/[Region/]City". Select the city closest to the
location that you want to represent.)
:dt.Coordinates
:dd.Specifies the geographic coordinates (latitude and longitude) of the
selected location. When you select a zone, the predefined coordinates for
that zone are populated; you can then modify them as needed.
:dt.TZ value
:dd.This field shows the timezone specifier, also known as the TZ value.
When you select a zone, the predefined TZ value for that zone is populated
in this field; you can then modify it if necessary.
:edl.
:p.Select :hp1.OK:ehp1. to accept the specified TZ value and coordinates for
the current clock.


.* ---------------------------------------------------------------------------
:h2 x=left y=bottom width=100% height=100% res=160.Program settings
:p.This dialog allows you to configure global program settings.
:dl tsize=30.
:dt.Show title bar
:dd.Enables or disables the standard window title bar for the Universal Clock
application.
:dt.Use compact view below
:dd.Sets the size threshold below which clock panels are drawn in compact
rather than standard view. The specified value is the size of an individual
clock panel, in pixels.
:dt.Compact description width
:dd.Specifies where the vertical separator line dividing the description and
time/date display areas will be drawn in compact view. This is given as a
percentage of the clock panel width.
:dt.Clock panel arrangement
:dd.This allows you to control the arrangement of clock panels into columns.
By default (value 0), all clocks are arranged in a single column, with the
first clock at the bottom. When clocks are arranged in multiple columns, the
first clock will appear on the bottom left.
:p.The list box shows a summary of all currently-defined clock panels. Here
you can change the order of the clock panels. Clocks are shown in the same
order in which they would appear in a single-column view, with clock 1 at
the bottom and the highest-numbered clock at the top. The up and down
pushbuttons will move the currently-selected clock up or down in the list.
:edl.


.*****************************************************************************
:h1 x=left y=bottom width=100% height=100% res=200.Keys
:p.The main window supports the following keyboard commands&colon.

:dl tsize=15 break=fit.

:dt.F1
:dd.Show general help.

:dt.F3
:dd.Exit UniClock.

:dt.Left / Up
:dd.Select the previous (or last) display area within the clock panel that
currently has keyboard focus. If no area is currently selected, the last
area in the current view is selected.
:p.Text in the currently-selected display area will appear underlined.

:dt.Right / Down
:dd.Select the next display area within the clock panel that currently
has keyboard focus. If no area is currently selected, the first area in the
current view (which is normally the description area) is selected.

:dt.Shift+F10
:dd.Show popup context menu.

:dt.Shift+Tab
:dd.Switch keyboard focus to the previous clock panel.

:dt.Space
:dd.Cycle the display mode of the currently-selected display area.

:dt.Tab
:dd.Switch keyboard focus to the next clock panel.

:edl.


.*****************************************************************************
:h1 x=left y=bottom width=100% height=100% res=990.Notices
:p.:hp2.Universal Clock for OS/2:ehp2.
.br
Copyright (C) 2015-2019 Alexander Taylor
:p.Includes the public domain 'sunriset' module by Paul Schlyter.

:p.This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

:p.This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

:p.You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

:p.See :link reftype=hd refid=license.the following section:elink. for the
full text of the GNU General Public License.

:p.Universal Clock source code repository&colon. https&colon.//github.com/altsan/os2-uniclock


.* ------------------------------------------------------------------------
:h2 x=left y=bottom width=100% height=100% id=license res=991.GNU General Public License
.im ..\gplv2.ipf

:euserdoc.
