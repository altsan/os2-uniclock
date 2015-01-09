/* MAKEDB.CMD
 *
 * Generate ZONEINFO.DAT (OS/2 profile db containing timezone information) from
 * the open source tzdata database files.
 *
 * ZONEINFO.DAT takes the following structure (in OS/2 profile format):
 *
 * YY (where 'YY' is a two-letter ISO country code) - application
 *   - .EN      (full country name in English, from the tzdata db) - key
 *   - .XX      (country name in language XX, not implemented) - key
 *   - zone     (where 'zone' is a timezone name in or overlapping this country)
 *              = TZ-string\tcoordinates(null)
 *   - zone     (etc.)
 *   - ...
 *
 * zone (where 'zone' is a timezone name in English) - application
 *   - .XX          (timezone name in language XX', not implemented)
 *   - TZ           (TZ string, duplicated from above)
 *   - Coordinates  (coordinates, duplicated from above)
*/

CALL RxFuncAdd 'SysLoadFuncs', 'REXXUTIL', 'SysLoadFuncs'
CALL SysLoadFuncs

cn_file = 'iso3166.tab'
zn_file = 'zone1970.tab'
profile = 'ZONEINFO.DAT'

IF STREAM( profile, 'C', 'QUERY EXISTS') <> '' THEN
    CALL SysFileDelete profile

/* Read the country list */
CALL LINEIN cn_file, 1, 0
DO WHILE LINES( cn_file )
    l = STRIP( LINEIN( cn_file ))
    IF l == '' THEN ITERATE
    IF LEFT( l, 1 ) == '#' THEN ITERATE
    PARSE VAR l ccode '09'x cname
    CALL SysIni profile, ccode, '.EN', cname || '00'x
END
CALL STREAM cn_file, 'C', 'CLOSE'


/* Read the zone list */
zcount = 0
CALL LINEIN zn_file, 1, 0
DO WHILE LINES( zn_file )
    l = STRIP( LINEIN( zn_file ))
    IF l == '' THEN ITERATE
    IF LEFT( l, 1 ) == '#' THEN ITERATE
    PARSE VAR l countries '09'x coords '09'x zname '09'x .
    IF zname == '' THEN ITERATE
    zcount = zcount + 1
    zones.!names.zcount = zname
    cclist = TRANSLATE( countries, ' ', ',')
    zones.zname.!countries.0 = WORDS( cclist )
    zones.zname.!coordinates = coords
    zones.zname.!rules.0 = 0
    zones.zname.!offset = '0'
    DO i = 1 TO zones.zname.!countries.0
        ccode = WORD( cclist, i )
        zones.zname.!countries.i = ccode
    END
END
CALL STREAM zn_file, 'C', 'CLOSE'
zones.!names.0 = zcount

CALL ReadRulesFile 'africa'
CALL ReadRulesFile 'antarctica'
CALL ReadRulesFile 'asia'
CALL ReadRulesFile 'australasia'
CALL ReadRulesFile 'europe'
CALL ReadRulesFile 'northamerica'
CALL ReadRulesFile 'southamerica'

DO i = 1 TO zones.!names.0
    zone_name = zones.!names.i
    IF zones.zone_name.!rules.0 == 0 THEN ITERATE
    zone_offset = zones.zone_name.!offset
    zone_symbol = zones.zone_name.!name
    zone_geo    = zones.zone_name.!coordinates
    letter1 = ''
    letter2 = ''
    month1  = ''
    day1    = ''
    hour1   = ''
    save1   = ''
    month2  = ''
    day2    = ''
    hour2   = ''
    save2   = ''
    IF zones.zone_name.!rules.0 > 0 THEN DO
        rule_name = zones.zone_name.!rules.1
        IF SYMBOL('rules.rule_name.0') == 'VAR' THEN DO
            IF rules.rule_name.0 >= 2 THEN DO
                PARSE VAR rules.rule_name.1 month1 day1 hour1 save1 letter1 .
                PARSE VAR rules.rule_name.2 month2 day2 hour2 save2 letter2 .
            END
        END
    END

    /* Make sure the return-to-standard-time rule (in which saveX is 0 or '-')
     * gets passed as the second set of parameters
     */
    IF (save1 == '0') | ( save1 == '-') THEN
        tzvar = MakeTZString( zone_symbol, zone_offset, letter2, letter1, month2, day2, hour2, save2, month1, day1, hour1 )
    ELSE
        tzvar = MakeTZString( zone_symbol, zone_offset, letter1, letter2, month1, day1, hour1, save1, month2, day2, hour2 )

    /* SAY zone_name tzvar */
    CALL SysIni profile, zone_name, 'TZ', tzvar || '00'x
    CALL SysIni profile, zone_name, 'Coordinates', zone_geo || '00'x
    DO j = 1 TO zones.zone_name.!countries.0
        CALL SysIni profile, zones.zone_name.!countries.j, zone_name, tzvar || '09'x || zone_geo || '00'x
    END
END


RETURN 0



ReadRulesFile: PROCEDURE EXPOSE zones. rules.
    PARSE ARG r_file

    in_zdef = ''
    CALL LINEIN r_file, 1, 0
    DO WHILE LINES( r_file )
        l = STRIP( TRANSLATE( LINEIN( r_file ), ' ', '09'x ))
        IF l == '' THEN ITERATE
        IF LEFT( l, 1 ) == '#' THEN ITERATE
        IF LEFT( l, 4 ) == 'Rule' THEN DO
            in_zdef = ''
            PARSE VAR l 'Rule' r_name r_from r_to r_type r_in r_on r_at r_save r_letter
            IF LEFT( r_to, 3 ) == 'max' THEN DO
                IF SYMBOL('rules.r_name.0') == 'LIT' THEN
                    j = 1
                ELSE
                    j = rules.r_name.0 + 1
                rules.r_name.j = r_in r_on r_at r_save r_letter
                rules.r_name.0 = j
            END
        END
        ELSE IF LEFT( l, 4 ) == 'Link' THEN DO
            in_zdef = ''
            ITERATE
        END
        ELSE IF LEFT( l, 4 ) == 'Zone' THEN DO
            PARSE VAR l 'Zone' z_name z_offset z_rule z_format z_unty .
            in_zdef = z_name
            IF z_unty == '' THEN DO
                IF SYMBOL('zones.z_name.!rules.0') == 'LIT' THEN
                    j = 1
                ELSE
                    j = zones.z_name.!rules.0 + 1
                zones.z_name.!rules.j = z_rule
                zones.z_name.!rules.0 = j
                zones.z_name.!name = z_format
                zones.z_name.!offset = z_offset
                in_zdef = ''
            END
        END
        ELSE if in_zdef <> '' THEN DO
            PARSE VAR l z_offset z_rule z_format z_unty .
            IF z_unty == '' THEN DO
                IF SYMBOL('zones.z_indef.!rules.0') == 'LIT' THEN
                    j = 1
                ELSE
                    j = zones.z_indef.!rules.0 + 1
                zones.in_zdef.!rules.j = z_rule
                zones.in_zdef.!rules.0 = j
                zones.in_zdef.!name = z_format
                zones.in_zdef.!offset = z_offset
                in_zdef = ''
            END
        END
    END
RETURN


/* MakeTzString
 *
 * Generates an OS/2-format 'TZ' string corresponding to the input timezone data.
 *
 * PARAMTERS (see zic.8 from the tzdata package for precise formats):
 *  zone_symbol  The base TZ name, e.g. 'CET', 'GMT/BST', 'E%sT'
 *  zone_offset  The amount of time to add to UTC to get standard time in this zone
 *  letter1      If zone_symbol contains %s, this is the letter substituted for %s in daylight savings time
 *  letter2      If zone_symbol contains %s, this is the letter substituted for %s in standard time
 *  month1       Name of the month in which daylight savings time begins
 *  day1         Day of the month in which daylight savings time begins
 *  hour1        Hour at which daylight savings time begins
 *  save         Amount of time savings (i.e. how much to advance the clock by) during daylight savings time
 *  month2       Name of the month in which standard time begins
 *  day2         Day of the month in which standard time begins
 *  hour2        Hour at which standard time begins
 */
MakeTzString: PROCEDURE
    PARSE ARG zone_symbol, zone_offset, letter1, letter2, month1, day1, hour1, save, month2, day2, hour2

    IF zone_symbol == '' THEN RETURN ''
    tzvar = ''

    IF letter1 = '-' THEN letter1 = ''
    IF letter2 = '-' THEN letter2 = ''
    IF month1 = '-' THEN month1 = ''
    IF month2 = '-' THEN month2 = ''
    IF day1 = '-' THEN day1 = ''
    IF day2 = '-' THEN day2 = ''
    IF hour1 = '-' THEN hour1 = ''
    IF hour2 = '-' THEN hour2 = ''
    IF save = '-' THEN save1 = ''

    /* Convert the offset hours into OS/2 format */
    IF ( zone_offset == '-') | ( zone_offset == '') THEN zone_offset = '0'
    ELSE DO
        PARSE VAR zone_offset zo_hours ':' zo_mins ':' .
        IF zo_mins == '0' | zo_mins == '00' THEN zo_mins = ''
        IF LEFT( zo_hours, 1 ) == '-' THEN
            zo_hours = SUBSTR( zo_hours, 2 )
        ELSE IF zo_hours <> '0' THEN
            zo_hours = '-'zo_hours
        IF zo_mins <> '' THEN
            zone_offset = zo_hours':'zo_mins
        ELSE
            zone_offset = zo_hours
    END
    /* Also get the offset value in minutes, in case we need it for calculations below */
    gmtoff = ConvertTime( zone_offset )

    /* Generate the base TZ name(s) */
    s_pos = POS('%S', TRANSLATE( zone_symbol ))
    IF s_pos > 0 THEN DO
        zone_prefix = SUBSTR( zone_symbol, 1, s_pos - 1 )
        zone_suffix = SUBSTR( zone_symbol, s_pos + 2 )
        IF ( zone_prefix == '') & ( zone_suffix == '') THEN DO
            zone_standard = 'GMT'
            zone_saving   = ''
        END
        ELSE DO
            IF letter1 == '' THEN letter1 = 'D'
            IF letter2 == '' THEN letter2 = 'S'
            zone_standard = zone_prefix || letter2 || zone_suffix
            IF month1 <> '' THEN
                zone_saving = zone_prefix || letter1 || zone_suffix
            ELSE
                zone_saving = ''
        END
    END
    ELSE IF POS('/', zone_symbol ) > 0 THEN DO
        PARSE VAR zone_symbol zone_standard '/' zone_saving
    END
    ELSE DO
        zone_standard = zone_symbol
        IF month1 <> '' THEN
            zone_saving = zone_symbol
        ELSE
            zone_saving = ''
    END
    zone_name = LEFT( zone_standard, 3 ) || zone_offset || STRIP( LEFT( zone_saving, 3 ))

    IF month1 == '' THEN
        RETURN zone_name

    /* Now convert the start/end values */
    start_month = ConvertMonth( month1 )
    start_date = ConvertDay( day1, start_month )
    start_time = ConvertTime( hour1 )

    time_flag = TRANSLATE( RIGHT( hour1, 1 ))
    IF time_flag == 'U' | time_flag == 'G' | time_flag == 'Z' THEN
        start_time = start_time + gmtoff

    end_month = ConvertMonth( month2 )
    end_date = ConvertDay( day2, end_month )
    end_time = ConvertTime( hour2 )
    save_amount = ConvertTime( save )

    time_flag = TRANSLATE( RIGHT( hour2, 1 ))
    IF time_flag == 'S' THEN
        end_time = end_time - save_amount
    ELSE IF time_flag == 'U' | time_flag == 'G' | time_flag == 'Z' THEN
        end_time = end_time + gmtoff

    tzvar = zone_name','start_month','start_date','start_time','end_month','end_date','end_time','save_amount
RETURN tzvar


ConvertMonth: PROCEDURE
    ARG mname
    names = 'JAN FEB MAR APR MAY JUN JUL AUG SEP OCT NOV DEC'
    mnum = WORDPOS( LEFT( mname, 3 ), names )
    IF mnum == 0 THEN mnum = 1
RETURN mnum


ConvertDay: PROCEDURE
    ARG dayspec, month
    dnames = 'SUN MON TUE WED THU FRI SAT'

    IF VERIFY( dayspec, '1234567890') == 0 THEN DO
        week = '0'
        day = dayspec
    END
    ELSE IF LEFT( dayspec, 4 ) == 'LAST' THEN DO
        week = '-1'
        PARSE VAR dayspec 'LAST' dayname .
        day = WORDPOS( dayname, dnames )
        IF day > 0 THEN day = day - 1
    END
    ELSE IF POS('>=', dayspec ) > 0 THEN DO
        PARSE VAR dayspec dayname '>=' mday
        day = WORDPOS( dayname, dnames )
        IF day > 0 THEN day = day - 1
        week = ( mday % 7 ) + 1
    END
    ELSE DO
        SELECT
            WHEN WORDPOS( month, '4 6 9 11') THEN mtotal = 30
            WHEN month == '2'                THEN mtotal = 28
            OTHERWISE                             mtotal = 31
        END
        PARSE VAR dayspec dayname '<=' mday
        day = WORDPOS( dayname, dnames )
        IF day > 0 THEN day = day - 1
        week = '-' || (( mtotal - mday ) % 7 ) + 1
    END

    day = week','day
RETURN day


ConvertTime: PROCEDURE
    ARG timespec

    timespec = STRIP( timespec, 'T', 'W')
    timespec = STRIP( timespec, 'T', 'S')
    timespec = STRIP( timespec, 'T', 'G')
    timespec = STRIP( timespec, 'T', 'U')
    timespec = STRIP( timespec, 'T', 'Z')

    neg = 0
    IF timespec == '-' THEN timespec = '0'
    ELSE IF LEFT( timespec, 1 ) == '-' THEN DO
        timespec = SUBSTR( timespec, 2 )
        neg = 1
    END

    PARSE VAR timespec hours ':' mins ':' secs

    totsecs = hours * 3600
    IF mins <> '' THEN
        totsecs = totsecs + ( mins * 60 )
    IF secs <> '' THEN
        totsecs = totsecs + secs

    IF neg == 1 THEN totsecs = '-'totsecs
RETURN totsecs
