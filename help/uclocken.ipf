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
:p.The Universal Clock application consists of up to 16 clock panels, which
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
size (which can be customized in the global settings), the clock panels will
automatically switch over to compact view.

:p.See also&colon.
:ul compact.
:li.:link reftype=hd res=110.The initial configuration:elink.
:li.:link reftype=hd res=140.Adding and removing clocks:elink.
:li.:link reftype=hd res=150.Clock configuration:elink.
:li.:link reftype=hd res=160.Global configuration:elink.
:eul.

.* ---------------------------------------------------------------------------
:h2 x=left y=bottom width=100% height=100% res=110.The initial configuration
:p.The first time you run Universal Clock, it will start up with a default
initial configuration. This consists of two clock panels&colon.
:ul compact.
:li.&osq.(Local Time)&csq., which is configured with the current system
timezone;
:li.&osq.(Coordinated Universal Time)&csq., which shows standard UTC time.
:eul.

:p.:artwork name='en_default.bmp' align='center'.

:p.Both clocks will use your system's default window colours and locale
conventions, and the WarpSans font (if available).

:p.You can customize or delete either of these clocks, and/or add new clocks.
(Note that you cannot delete the last clock panel &endash. there must be at
least one.)

:p.You can also set global configuration options, which control the behaviour
of the Universal Clock program as a whole.

:p.See&colon.
:ul compact.
:li.:link reftype=hd res=140.Adding and removing clocks:elink.
:li.:link reftype=hd res=150.Clock configuration:elink.
:li.:link reftype=hd res=160.Global configuration:elink.
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

:p.If geographic coordinates are set and enabled for this clock, then a
graphical day/night indicator will also be shown in this area, to the
right of the time display. This indicates whether it is currently daytime
or night-time in the configured location.

:p.:hp5.Bottom Area:ehp5.
:p.The bottom area normally shows the current date in the configured timezone.
Like the time display, the date format depends on the clock's localization
settings.

:p.If geographic coordinates are set and enabled for this clock, then clicking
on the text in this area will toggle the display between showing the current
date, and showing the current date's sunrise and sunset times in the
configured location.

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
will switch between the two.

:p.The exact time and date formats will depend, as always, on the clock's
localization settings.

:p.If geographic coordinates are set and enabled for this clock, then a
graphical day/night indicator will also be shown in this area, to the
right of the time or date. This indicates whether it is currently daytime
or night-time in the configured location.


.* ---------------------------------------------------------------------------
:h2 x=left y=bottom width=100% height=100% res=140.Adding and removing clocks
:p.To add a new clock panel, click mouse button 2 on any existing clock panel,
and choose :hp1.Add clock:ehp1. from the popup menu. The
:link reftype=hd res=150.clock configuration:elink. notebook will open.
Configure the desired settings for the new clock, and select :hp1.OK:ehp1.
when done. The new clock panel will be added to the Universal Clock window.

:p.To remove an existing clock, click mouse button 2 on the clock panel you
want to delete, and select :hp1.Delete clock:ehp1. from the popup menu. You
will be prompted for confirmation; select :hp1.Yes:ehp1. and the clock panel
will be removed from Universal Clock.


.* ---------------------------------------------------------------------------
:h2 x=left y=bottom width=100% height=100% res=150.Clock configuration
:p.


.* ---------------------------------------------------------------------------
:h2 x=left y=bottom width=100% height=100% res=160.Global configuration
:p.


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

:euserdoc.
