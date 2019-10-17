/* MAKEDB.CMD
 *
 * Generate ZONEINFO.xx (OS/2 profile db containing timezone information) from
 * the open source tzdata database files.
 *
 * ZONEINFO.xx (where 'XX' is a two-letter language code, e.g. 'EN') takes the
 * following entries (in OS/2 profile format):
 *
 * YY (where 'YY' is a two-letter ISO country code) - application
 *   - .XX      (where 'XX' is the same language code as used in the filename)
 *              = Localized name of the country in the current language
 *   - zone    (where 'zone' is a timezone name in or overlapping this country)
 *              = our (localized) region name for this country/timezone
 *   - zone    (etc.)
 *   - ...
 *
 * zone (where 'zone' is a timezone name) - application
 *   - TZ           (TZ string)
 *   - Coordinates  (geographical coordinates) - for possible future use
 *
 * Country ('.xx') and tzdata timezone name ('zone') strings should be UTF-8
 * encoded. (This script does not care but applications which use the resulting
 * data will.)
 *
 */
SIGNAL ON NOVALUE

CALL RxFuncAdd 'SysLoadFuncs', 'REXXUTIL', 'SysLoadFuncs'
CALL SysLoadFuncs

list_only = 0
tzdata_path = DIRECTORY()
nlv = 'EN'

PARSE UPPER ARG _params
CALL SplitParameter _params, ':'
DO i = 1 TO argv.0
    SELECT
        WHEN argv.i.__keyword == '/T'   THEN tzdata_path = argv.i.__keyValue
        WHEN LEFT( argv.i.__original, 1 ) <> '/' THEN nlv = argv.i.__original
        OTHERWISE NOP
    END
END

SAY 'Generating zoneinfo database for' nlv
SAY 'Looking for tz database in' tzdata_path

cn_file = tzdata_path'\iso3166.tab'
zn_file = tzdata_path'\zone1970.tab'
profile = 'ZONEINFO.'nlv

nametbl = STREAM('names.'nlv, 'C', 'QUERY EXISTS')
IF nametbl == '' THEN DO
    SAY 'Names table not found.'
    RETURN 2
END

IF ( list_only == 0 ) & ( STREAM( profile, 'C', 'QUERY EXISTS') <> '') THEN
    CALL SysFileDelete profile

/* Read the country list */
CALL LINEIN cn_file, 1, 0
DO WHILE LINES( cn_file )
    l = STRIP( LINEIN( cn_file ))
    IF l == '' THEN ITERATE
    IF LEFT( l, 1 ) == '#' THEN ITERATE
    PARSE VAR l ccode '09'x cname
    ctrynames.ccode = cname
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

/* Read the NLV translations file and update the country and zone names. */
IF nametbl <> '' THEN DO
    DO WHILE LINES( nametbl )
        next = LINEIN( nametbl )
        IF LEFT( next, 1 ) == '#' THEN ITERATE
        PARSE VAR next cc '09'x tz '09'x cnlv '09'x znlv '09'x flg '09'x .
        IF cnlv == '' THEN ITERATE
        IF flg == '' THEN NOP
        ELSE IF flg <> '*' THEN flg = ''
        ctrynames.cc = cnlv
        IF list_only == 0 THEN
            CALL SysIni profile, cc, '.'nlv, cnlv || '00'x
        IF znlv <> '' THEN
            ctrynames.cc.tz = znlv || '09'x || flg
    END
END

CALL ReadRulesFile tzdata_path'\africa'
CALL ReadRulesFile tzdata_path'\antarctica'
CALL ReadRulesFile tzdata_path'\asia'
CALL ReadRulesFile tzdata_path'\australasia'
CALL ReadRulesFile tzdata_path'\europe'
CALL ReadRulesFile tzdata_path'\northamerica'
CALL ReadRulesFile tzdata_path'\southamerica'

ccount = 0
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
    /* Make sure zone_symbol starts with a letter */
    IF POS( TRANSLATE( LEFT( zone_symbol, 1 )), "ABCDEFGHIJKLMNOPQRSTUVWXYZ") == 0 THEN DO
        /* Take the first letter of the region name, or 'L' (Local) if that fails */
        PARSE VAR zone_name . '/' locality .
        IF locality == '' THEN zone_initial = 'L'
        ELSE zone_initial = LEFT( locality, 1 )
        zone_symbol = zone_initial'ST/'zone_initial'DT'
    END
    ELSE DO
        PARSE VAR zone_symbol zsym1 '/' zsym2
        IF ( zsym2 <> '') & POS( TRANSLATE( LEFT( zsym2, 1 )), "ABCDEFGHIJKLMNOPQRSTUVWXYZ") == 0 THEN DO
            zone_symbol = zsym1'/'||LEFT( zsym1, 1 )||'DT'
        END
    END
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

    /*
    parse var tzvar _id','.','.','.','t1','.
    if left( t1, 1 ) == '-' then do
        say zone_name': Time conversion error:' t1 '(rule file has' hour1')'
    end
    */

    IF list_only == 0 THEN CALL SysIni profile, zone_name, 'TZ', tzvar || '00'x
    IF list_only == 0 THEN CALL SysIni profile, zone_name, 'Coordinates', zone_geo || '00'x
    IF list_only == 0 THEN CALL SysIni profile, zone_name, 'AdjustedOffset', ConvertTime( zone_offset ) + 43200 || '00'x
    DO j = 1 TO zones.zone_name.!countries.0
        ccount = ccount + 1
        ccode = zones.zone_name.!countries.j
        IF ( nametbl <> '') & SYMBOL('ctrynames.ccode.zone_name') == 'VAR' THEN
            PARSE VAR ctrynames.ccode.zone_name tzname '09'x flg .
        ELSE DO
            tzname = ''
            flg = ''
        END
        ctryzones.ccount = ccode || '09'x || zone_name || '09'x || tzname || '09'x tzvar
        IF list_only == 0 THEN
            CALL SysIni profile, ccode, flg||zone_name, tzname || '00'x
    END
END
ctryzones.0 = ccount
IF ccount == 0 THEN DO
    SAY 'No countries found!  (Missing tzdata files?)'
    RETURN 1
END

CALL SysStemSort 'ctryzones.'
IF list_only == 1 THEN DO i = 1 TO ctryzones.0
    SAY ctryzones.i
END

RETURN 0


/* -------------------------------------------------------------------------- *
 * ReadRulesFile                                                              *
 *                                                                            *
 * Parse a tzdata continent file, parsing all defined timezones and all       *
 * 'active' (still-current) rules.  These are saved to the stems 'zones' and  *
 * 'rules', respectively.                                                     *
 * -------------------------------------------------------------------------- */
ReadRulesFile: PROCEDURE EXPOSE zones. rules.
    PARSE ARG r_file

    in_zdef = ''
    CALL LINEIN r_file, 1, 0
    DO WHILE LINES( r_file )
        l = STRIP( TRANSLATE( LINEIN( r_file ), ' ', '09'x ))
        IF l == '' THEN ITERATE
        IF LEFT( l, 1 ) == '#' THEN ITERATE
        ELSE IF POS('#', l ) > 1 THEN
            l = STRIP( SUBSTR( l, 1, POS('#', l ) - 1 ))
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


/* --------------------------------------------------------------------------------------------------------- *
 * MakeTzString                                                                                              *
 *                                                                                                           *
 * Generates an OS/2-format 'TZ' string corresponding to the input timezone                                  *
 * data.                                                                                                     *
 *                                                                                                           *
 * PARAMTERS (see zic.8 from the tzdata package for precise formats):                                        *
 *  zone_symbol  The base TZ name, e.g. 'CET', 'GMT/BST', 'E%sT'                                             *
 *  zone_offset  The amount of time to add to UTC to get standard time in this zone                          *
 *  letter1      If zone_symbol contains %s, this is the letter substituted for %s in daylight savings time  *
 *  letter2      If zone_symbol contains %s, this is the letter substituted for %s in standard time          *
 *  month1       Name of the month in which daylight savings time begins                                     *
 *  day1         Day of the month in which daylight savings time begins                                      *
 *  hour1        Hour at which daylight savings time begins                                                  *
 *  save         Amount of time savings (i.e. how much to advance the clock by) during daylight savings time *
 *  month2       Name of the month in which standard time begins                                             *
 *  day2         Day of the month in which standard time begins                                              *
 *  hour2        Hour at which standard time begins                                                          *
 * --------------------------------------------------------------------------------------------------------- */
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
    IF gmtoff < 0 THEN
        gmtoff = ABS( gmtoff )
    ELSE
        gmtoff = '-'gmtoff

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
            IF ( letter2 == '') & ( month1 == '') THEN letter2 = 'S'
            zone_standard = zone_prefix || letter2 || zone_suffix
            IF month1 <> '' THEN
                zone_saving = zone_prefix || letter1 || zone_suffix
            ELSE
                zone_saving = ''
        END
    END
    ELSE IF POS('/', zone_symbol ) > 0 THEN DO
        PARSE VAR zone_symbol zone_standard '/' zone_saving
        IF month1 == '' THEN zone_saving = ''
    END
    ELSE DO
        zone_standard = zone_symbol
        IF month1 <> '' THEN
            zone_saving = zone_symbol
        ELSE
            zone_saving = ''
    END
    zone_name = LEFT( zone_standard, 3, 'X') || zone_offset || STRIP( LEFT( zone_saving, 3 ))

    IF month1 == '' THEN
        RETURN zone_name

    /* Now convert the start/end values */
    start_month = ConvertMonth( month1 )
    start_date = ConvertDay( day1, start_month )
    start_time = ConvertTime( hour1 )

    time_flag = TRANSLATE( RIGHT( hour1, 1 ))
    IF time_flag == 'U' | time_flag == 'G' | time_flag == 'Z' THEN
        start_time = start_time + gmtoff
    /* Handle case where the converted time moves us back to the previous day.
     * This includes moving back the week as well, if necessary - but we don't
     * get into changing the month (just use midnight in that event).
     */
    IF start_time < 0 THEN DO
        PARSE VAR start_date _wk','_day
        IF _wk > 1 THEN DO
            start_time = start_time + 86400
            IF _day == 0 THEN DO
                _wk  = _wk - 1
                _day = 6
            END
            ELSE _day = _day - 1
            start_date = _wk','_day
        END
        ELSE start_time = 0
    END

    end_month = ConvertMonth( month2 )
    end_date = ConvertDay( day2, end_month )
    end_time = ConvertTime( hour2 )
    save_amount = ConvertTime( save )

    time_flag = TRANSLATE( RIGHT( hour2, 1 ))
    IF time_flag == 'S' THEN
        end_time = end_time - save_amount
    ELSE IF time_flag == 'U' | time_flag == 'G' | time_flag == 'Z' THEN
        end_time = end_time + gmtoff
    IF end_time < 0 THEN DO
        PARSE VAR end_date _wk','_day
        IF _wk > 1 THEN DO
            end_time = end_time + 86400
            IF _day == 0 THEN DO
                _wk  = _wk - 1
                _day = 6
            END
            ELSE _day = _day - 1
            end_date = _wk','_day
        END
        ELSE end_time = 0
    END

    tzvar = zone_name','start_month','start_date','start_time','end_month','end_date','end_time','save_amount
RETURN tzvar


/* -------------------------------------------------------------------------- *
 * ConvertMonth                                                               *
 * -------------------------------------------------------------------------- */
ConvertMonth: PROCEDURE
    ARG mname
    names = 'JAN FEB MAR APR MAY JUN JUL AUG SEP OCT NOV DEC'
    mnum = WORDPOS( LEFT( mname, 3 ), names )
    IF mnum == 0 THEN mnum = 1
RETURN mnum


/* -------------------------------------------------------------------------- *
 * ConvertDay                                                                 *
 * -------------------------------------------------------------------------- */
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


/* -------------------------------------------------------------------------- *
 * ConvertTime                                                                *
 * -------------------------------------------------------------------------- */
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


/* ------------------------------------------------------------------ *
 * SplitParameter                                                     *
 *                                                                    *
 * Split a string into separate arguments                             *
 * (Source: 'Rexx Tips & Tricks' (v3.60) by Bernd Schemmer)           *
 *                                                                    *
 * call:     call SplitParameter Parameter_string {, separator }      *
 *                                                                    *
 * where:    parameter_string - string to split                       *
 *           separator - separator character to split a parameter     *
 *                       into keyword and keyvalue                    *
 *                       (Def.: Don't split the parameter into        *
 *                              keyword and keyvalue)                 *
 *                                                                    *
 * returns:  the number of arguments                                  *
 *           The arguments are returned in the stem argv.:            *
 *                                                                    *
 *             argv.0 = number of arguments                           *
 *                                                                    *
 *             argv.n.__keyword = keyword                             *
 *             argv.n.__keyValue = keyValue                           *
 *             argv.n.__original = original_parameter                 *
 *                                                                    *
 *           The variables 'argv.n.__keyvalue' are only used if       *
 *           the parameter 'separator' is not omitted.                *
 *                                                                    *
 * note:     This routine handles arguments in quotes and double      *
 *           quotes also. You can use either the format               *
 *                                                                    *
 *             keyword:'k e y v a l u e'                              *
 *                                                                    *
 *           or                                                       *
 *                                                                    *
 *             'keyword:k e y v a l u e'                              *
 *                                                                    *
 *           (':' is the separator in this example).                  *
 *                                                                    *
 * ------------------------------------------------------------------ */
SplitParameter: PROCEDURE EXPOSE argv.

                    /* get the parameter                              */
  parse arg thisArgs, thisSeparator

                    /* init the result stem                           */
  argv. = ''
  argv.0 = 0

  do while thisargs <> ''

    parse value strip( thisArgs, "B" ) with curArg thisArgs

    parse var curArg tc1 +1 .
    if tc1 = '"' | tc1 = "'" then
      parse value curArg thisArgs with (tc1) curArg (tc1) ThisArgs

    if thisSeparator <> '' then
    do
                    /* split the parameter into keyword and keyvalue  */
      parse var curArg argType (thisSeparator) argValue

      parse var argValue tc2 +1 .
      if tc2 = '"' | tc2 = "'" then
        parse value argValue thisArgs with (tc2) argValue (tc2) ThisArgs

      if tc1 <> '"' & tc1 <> "'" & tc2 <> '"' & tc2 <> "'" then
      do
        argtype  = strip( argType  )
        argValue = strip( argValue )
      end /* if */
      else
         if argValue <> '' then
           curArg = argtype || thisSeparator || argValue

      i = argv.0 + 1
      argv.i.__keyword = translate( argType )
      argv.i.__KeyValue = argValue
      argv.i.__original = strip( curArg )
      argv.0 = i

   end /* if thisSeparator <> '' then */
   else
   do
     i = argv.0 + 1
     argv.i.__keyword = strip( curArg )
     argv.i.__original = strip( curArg )
     argv.0 = i
   end /* else */

  end /* do while thisArgs <> '' */

RETURN argv.0


/******************************************************************************
 * SIGNAL HANDLERS                                                            *
 ******************************************************************************/
NOVALUE:
    SAY LEFT( sigl, 6 ) '+++ Uninitialized variable'
    SAY LEFT( sigl, 6 ) '+++' SOURCELINE( sigl )
EXIT sigl

