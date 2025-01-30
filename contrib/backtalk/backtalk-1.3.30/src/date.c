/* Copyright 1996, Jan D. Wolter and Steven R. Weiss, All Rights Reserved. */

/* DATE SCANNING ROUTINES - This contains routines for reading dates.
 * Yes, I know about strptime(), but it isn't anywhere near flexible enough
 * about reading date strings.
 *
 * This also contains some time functions that understand timezones.  We
 * need them because we want be able to display times to users in their own
 * timezones, but log things in our local time zone.
 *
 * (c) 1996 - Jan Wolter - janc@cyberspace.org
 */

#include "common.h"

#include "str.h"
#include "date.h"
#include "tm2sec.h"

#ifdef HAVE_LIMITS_H
#include <limits.h>
#else
/* assume machines without 'limit.h' are old enough to have int times */
#define INT_MAX 2147483647
#endif

#if !defined(HAVE_TM_GMTOFF) && !defined(HAVE_TZSET)
#include <sys/timeb.h>
#endif

#ifdef DMALLOC
#include "dmalloc.h"
#endif

/* is the given year a leap year?  Note year given is since 1900 */
#define leap_year(y) (((y) % 4) == 0 && (((y) % 100) != 0 || ((y+1900) % 400) == 0))

/* how many days in the given month?  suitable for modern dates only */
#define last_day(m,y) (((m)==1) ? (leap_year(y)?29:28) : monthlength[m])

char *alpha= "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";

char *month_name[12]={"jan_uary",   "feb_ruary", "mar_ch",    "apr_il",
		      "may",       "jun_e",     "jul_y",     "aug_ust",
		      "sep_tember", "oct_ober",  "nov_ember", "dec_ember"};

int monthlength[12]={31,29,31,30, 31,30,31,31, 30,31,30,31};

char *day_name[7]={"sun_day", "mon_day", "tues_day", "wed_nesday", "thu_rsday",
                   "fri_day", "sat_urday"};

#define W_NONE		-1
#define W_YEAR		0
#define W_MONTH		1
#define W_WEEK		2
#define W_DAY		3
#define W_HOUR		4
#define W_MINUTE	5
#define W_SECOND	6
char *time_name[7]= {"year_s","month_s","week_s","day_s",
                     "hour_s","min_utes","sec_onds"};


/* ADDTM - Add a positive amount of time to a tm structure.
 */

void addtm(struct tm *tm,int secs, int mins, int hours,
                         int days, int mons, int years)
{
    int eom;

    tm->tm_sec+= secs;
    while (tm->tm_sec > 59)
    {
	tm->tm_sec  -= 60;
	mins++;
    }

    tm->tm_min+= mins;
    while (tm->tm_min > 59)
    {
	tm->tm_min  -= 60;
	hours++;
    }

    tm->tm_hour+= hours;
    while (tm->tm_hour > 23)
    {
	tm->tm_hour  -= 24;
	days++;
    }

    /* Month lengths vary, so overflowing days is more complex */
    tm->tm_mday+= days;
    while (tm->tm_mday > (eom= last_day(tm->tm_mon,tm->tm_year)))
    {
	tm->tm_mday-= eom;
	if (++tm->tm_mon > 11)
	{
	    tm->tm_mon= 0;
	    tm->tm_year++;
	}
    }

    tm->tm_mon+= mons;
    while (tm->tm_mon > 11)
    {
	tm->tm_mon  -= 12;
	years++;
    }

    tm->tm_year+= years;

    /* Fix date in case we advanced to a shorter month */
    if (tm->tm_mday > (eom= last_day(tm->tm_mon,tm->tm_year)))
	tm->tm_mday= eom;
}


/* SUBTM - Subtract a positive amount of time from a tm structure.
 */

void subtm(struct tm *tm,int secs, int mins, int hours,
                         int days, int mons, int years)
{
    int eom;

    tm->tm_sec-= secs;
    while (tm->tm_sec < 0)
    {
	tm->tm_sec  += 60;
	mins++;
    }

    tm->tm_min-= mins;
    while (tm->tm_min < 0)
    {
	tm->tm_min  += 60;
	hours++;
    }

    tm->tm_hour-= hours;
    while (tm->tm_hour < 0)
    {
	tm->tm_hour  += 24;
	days++;
    }

    /* Month lengths vary, so underflowing days is more complex */
    tm->tm_mday-= days;
    while (tm->tm_mday < 1)
    {
	if (--tm->tm_mon < 0)
	{
	    tm->tm_mon= 11;
	    tm->tm_year--;
	}
	tm->tm_mday += last_day(tm->tm_mon,tm->tm_year);
    }

    tm->tm_mon-= mons;
    while (tm->tm_mon < 0)
    {
	tm->tm_mon  += 12;
	years++;
    }

    tm->tm_year-= years;

    /* Fix date in case we retreated to a shorter month */
    if (tm->tm_mday > (eom= last_day(tm->tm_mon,tm->tm_year)))
	tm->tm_mday= eom;
}

/* NLLOOK - Search a list of names for a match to the given string.  Only
 * the first three characters of the string need to match.  Returns the
 * index if a match is found, or -1 if none is found.
 */

int nllook(char *pat, char **list, int n)
{
int i;

    for (i= 0; i < n; i++)
    {
	if (match(pat, list[i], 0))
	    return i;
    }
    return -1;
}

int gotweek, gotyear, gotmonth, gotday, gothour, gotmin, gotsec, gotampm;
int lastwhat;


/* SETHMS - set whichever of hours, minutes, or seconds hasn't been set yet
 * to the given value.  Try them in that order.
 */

int sethms(int n, struct tm *tm)
{
    if (!gothour)
    {
	tm->tm_hour= n;
	lastwhat= W_HOUR;
	gothour= 1;
	return 0;
    }
    else if (!gotmin)
    {
	tm->tm_min= n;
	lastwhat= W_MINUTE;
	gotmin= 1;
	return 0;
    }
    else if (!gotsec)
    {
	tm->tm_sec= n;
	lastwhat= W_SECOND;
	gotsec= 1;
	return 0;
    }
    else
	return -1;
}


/* SETMDY - set whichever of months, days, or years hasn't been set yet
 * to the given value.  Try them in that order.
 */

int setmdy(int n, struct tm *tm)
{
    if (!gotmonth)
    {
	tm->tm_mon= n-1;
	lastwhat= W_MONTH;
	gotmonth= 1;
	return 0;
    }
    else if (!gotday)
    {
	tm->tm_mday= n;
	lastwhat= W_DAY;
	gotday= 1;
	return 0;
    }
    else if (!gotyear)
    {
	tm->tm_year= n;
	lastwhat= W_YEAR;
	gotyear= 1;
	return 0;
    }
    else
    	return 1;
}


/* STRTIME - Given a character string containing some sort of relative or
 * absolute time, convert it to a standard Unix representation, giving
 * both the unix integer time and the struct tm structure.
 *
 * If future is true, we default to interpreting ambiguous times (like
 * "Wednesday") as being in the future.  If not, we prefer times in the
 * past.
 *
 * If start is true, we return the starting time of time periods that have
 * duration (like "March 1994").  Otherwise we return the ending time.
 *
 * On error, we return (time_t) -1.
 *
 * Obviously, this only works for dates in the 20th century or later.
 */

time_t strtime(char *string, int future, int start, char *tz, struct tm *tm)
{
    char *c;
    int m,n,ln;
    char relative= '\0';
    char prev;
    int lastint= -1;
    struct tm *this;
    time_t now;
    int gotstart;

    lastwhat= W_NONE;
    gotweek=0; gotyear=0; gotmonth=0; gotday=0;
    gothour=0; gotmin=0; gotsec=0; gotampm=0; gotstart= 0;
    tm->tm_yday= 0; tm->tm_wday= 0; tm->tm_isdst= -1;
    tm->tm_sec= 0; tm->tm_min= 0; tm->tm_hour= 0;

    /* Get the current date from system */
    now= time((time_t) NULL);
    this= localtimez(&now,tz);

    /* Skip leading white space - we skip parens too to be a bit compatible
     * with Picospan's habit of enclosing times in parens. */
    c= firstout(string," \t,;()");

    if (*c == '-' || *c == '+')
    {
    	relative= *c;
	c= firstout(c+1," \t,;");
    }

    /* Parse the string */

    while (*c != '\0' && *c != '\n')
    {
	prev= (c > string) ? c[-1] : ' ';

	if (isdigit(*c))
	{
	    if (lastint != -1)
	    {
		if (setmdy(lastint,tm) && sethms(lastint,tm)) return -1;
		lastint= -1;
	    }

	    n= atoi(c);
	    c= firstout(c,"0123456789");

	    if (*c == ':' || prev == ':')
	    {
		if (sethms(n,tm)) return -1;
	    }
	    else if (*c == '/' || prev == '/')
	    {
		if (setmdy(n,tm)) return -1;
	    }
	    else if (lastwhat == W_MONTH)
	    {
	    	/* number after month is probably a day, but could be year */
	    	if (n == 0 || n > 31)
	    	{
		    tm->tm_year= n;
		    lastwhat= W_YEAR;
		    gotyear= 1;
	    	}
	    	else
	    	{
		    tm->tm_mday= n;
		    lastwhat= W_DAY;
		    gotday= 1;
	    	}
	    }
	    else if (lastwhat == W_DAY)
	    {
		tm->tm_year= n;
		lastwhat= W_YEAR;
		gotyear= 1;
	    }
	    else
	    {
	    	lastwhat= W_NONE;
	    	lastint= n;
	    }
	}
	/* We treat "noon" as a synonym for "pm" and "midnight" as a synonym
	 * for "am" (just like Picospan) */
	else if (!strncasecmp(c,"am",ln= 2) ||
		 !strncasecmp(c,"pm",ln= 2) ||
		 !strncasecmp(c,"noon",ln= 4) ||
		 !strncasecmp(c,"midnight",ln= 8) ||
		 !strncasecmp(c,"midnite",ln= 7))
	{
	    if (lastint != -1)
	    {
		if (sethms(lastint,tm)) return -1;
		lastint= -1;
	    }
	    else
	    	lastwhat= W_NONE;

	    if (!gothour || relative || tm->tm_hour > 12)
	    	return -1;

	    if ((*c == 'p' || *c == 'n') && tm->tm_hour < 12) tm->tm_hour+= 12;
	    if ((*c == 'a' || *c == 'm') && tm->tm_hour == 12) tm->tm_hour= 0;

	    gotampm= 1;
	    c+= ln;
	}
	else if ((n= nllook(c,month_name,12)) != -1)
	{
	    if (gotmonth)
	    	return -1;
	    if (lastint != -1)
	    {
	    	if (!gotday)
	    	{
		    tm->tm_mday= lastint;
		    gotday= 1;
	    	}
	    	else if (sethms(lastint,tm))
		    return -1;
		lastint= -1;
	    }
	    tm->tm_mon= n;
	    lastwhat= W_MONTH;
	    gotmonth= 1;
	    c= firstout(c,alpha);
	}
	else if ((n= nllook(c,day_name,7)) != -1)
	{
	    if (gotweek)
	    	return -1;
	    if (lastint != -1)
	    {
		if (sethms(lastint,tm)) return -1;
		lastint= -1;
	    }
	    tm->tm_wday= n;
	    lastwhat= W_WEEK;
	    gotweek= 1;
	    c= firstout(c,alpha);
	}
	else if ((n= nllook(c,time_name,7)) != -1)
	{
	    if (!relative) relative= future ? '+' : '-';
	    if (lastint == -1) lastint= 1;
	    switch (n)
	    {
	    case W_YEAR:   tm->tm_year= lastint;   gotyear= 1;  break;
	    case W_MONTH:  tm->tm_mon= lastint;    gotmonth= 1; break;
	    case W_WEEK:   tm->tm_mday= 7*lastint; gotday= 1;   break;
	    case W_DAY:    tm->tm_mday= lastint;   gotday= 1;   break;
	    case W_HOUR:   tm->tm_hour= lastint;   gothour= 1;  break;
	    case W_MINUTE: tm->tm_min= lastint;    gotmin= 1;   break;
	    case W_SECOND: tm->tm_sec= lastint;    gotsec= 1;   break;
	    }
	    lastint= -1;
	    c= firstout(c,alpha);
	}
	else if (!strncasecmp(c,"yesterday",ln= 9) ||
		 !strncasecmp(c,"tomorrow",ln= 8) ||
		 !strncasecmp(c,"today",ln= 5))
	{
	    if (gotday || gotmonth || gotyear)
	    	return -1;
	    if (lastint != -1)
	    {
		if (sethms(lastint,tm)) return -1;
		lastint= -1;
	    }
	    tm->tm_year= this->tm_year + 1900;
	    tm->tm_mon= this->tm_mon;
	    tm->tm_mday= this->tm_mday;
	    gotyear= gotmonth= gotday= 1;
	    lastwhat= W_YEAR;
	    if (ln == 9)	/* yesterday */
	    	subtm(tm,0,0,0,1,0,0);
	    else if (ln == 8)	/* tomorrow */
	    	addtm(tm,0,0,0,1,0,0);
	    c+= ln;
	}
	else if (!strncasecmp(c,"beginning", ln= 9) ||
		 !strncasecmp(c,"start",ln= 5) ||
		 !strncasecmp(c,"begining",ln= 8))
	{
	    gotstart= 1;
	    start= 1;
	    c+= ln;
	}
	else if (!strncasecmp(c,"end",3))
	{
	    gotstart= 1;
	    start= 0;
	    c+= 3;
	}
	else if (!strncasecmp(c,"ago",3))
	{
	    relative= '-';
	    c+= 3;
	}
	else if (!strncasecmp(c,"hence",5))
	{
	    relative= '+';
	    c+= 5;
	}
	else if (!strncasecmp(c,"last",4))
	{
	    future= 0;
	    c+= 4;
	}
	else if (!strncasecmp(c,"next",4))
	{
	    future= 1;
	    c+= 4;
	}
	else if (!strncasecmp(c,"time",4))
	{
	    /* Ignore */
	    c+= 4;
	}
	else if (!strncasecmp(c,"the",3))
	{
	    /* Ignore */
	    c+= 3;
	}
	else if (!strncasecmp(c,"of",2))
	{
	    /* Ignore */
	    c+= 2;
	}
	else
	    return -1;

	c= firstout(c," \t,;/:-.()");
    }
    if (lastint != -1)
    {
	if (!relative && (gotmonth || gotday || gothour))
	{
	    if (setmdy(lastint,tm) && sethms(lastint,tm)) return -1;
	}
	else if (!relative && (lastint >= 1976 && lastint <= 2099))
	{
	    /* Isolated number that looks like a four digit year */
	    tm->tm_year= lastint;
	    gotyear= 1;
	}
	else
	{
	    /* Other isolated signless numbers are treated as "+<number>" */
	    if (!relative) relative= future ? '+' : '-';
	    if (!gotday)
	    {
	    	tm->tm_mday= lastint;
	    	gotday= 1;
	    }
	    else
	    	return -1;
	}
    }

    /* Handle beginning and end of time */
    if (gotstart && !gotsec && !gotmin && !gothour && !gotday && !gotday &&
        !gotweek && !gotmonth && !gotyear)
	    return (start ? 0 : INT_MAX);

    /* Check range of values and default missing fields */
    if (gotsec)
    {
    	if (!relative && tm->tm_sec > 61)	/* allow for leap seconds */
    	    return -1;
    }
    else
    	tm->tm_sec= (relative || start) ? 0 : 59;

    if (gotmin)
    {
    	if (!relative && tm->tm_min > 59)
    	    return -1;
    }
    else
    	tm->tm_min= (relative || start) ? 0 : 59;

    if (gothour)
    {
    	if (!relative && tm->tm_hour > 23)
    	    return -1;
    }
    else
    	tm->tm_hour= (relative || start) ? 0 : 23;

    if (gotmonth)
    {
    	if (!relative && tm->tm_mon > 11)
    	    return -1;
    }
    else if (relative)
    	tm->tm_mon= 0;
    else if (gotyear)
    	tm->tm_mon= start ? 0 : 11;
    else
    	tm->tm_mon= this->tm_mon;

    if (gotyear)
    {
	if (!relative)
	{
	    /* Two digit years -- decide if this century or next */
	    if (tm->tm_year < 100)
	    {
		if (future)
		    /* decide if this century or next */
		    tm->tm_year+= ((this->tm_year / 100) +
		      (tm->tm_year < (this->tm_year % 100)-10))*100;
		else
		    /* decide if this century or last */
		    tm->tm_year+= ((this->tm_year / 100) -
		      (tm->tm_year > (this->tm_year % 100)+10))*100;
	    }
	    else
		    tm->tm_year-= 1900;
	}
    }
    else if (relative)
    	tm->tm_year= 0;
    else
    {
	if (future)
	    /* use either this year or next */
	    tm->tm_year= this->tm_year + (tm->tm_mon < this->tm_mon);
	else
	    /* use either this year or last */
	    tm->tm_year= this->tm_year - (tm->tm_mon > this->tm_mon);
    }

    if (gotday)
    {
    	if (!relative && (tm->tm_mday == 0 ||
    	                  tm->tm_mday > last_day(tm->tm_mon,tm->tm_year)))
    	   return -1;
    }
    else if (relative)
    	tm->tm_mday= 0;
    else if (gotmonth || gotyear)
    	tm->tm_mday= start ? 1 : last_day(tm->tm_mon,tm->tm_year);
    else
    	tm->tm_mday= this->tm_mday;

    /* Only pay attention to weekdays if the month, day and year not given */
    if (!relative && gotweek && !gotmonth && !gotday && !gotyear)
    {
    	if (future)
    	{
    	    n= (tm->tm_wday - this->tm_wday + 7) % 7;
    	    if (n == 0) n= 7;
	    addtm(tm,0,0,0,n,0,0);
	}
	else
	{
    	    n= (this->tm_wday - tm->tm_wday + 7) % 7;
    	    if (n == 0) n= 7;
	    subtm(tm,0,0,0,n,0,0);
	}
    }

    if (relative == '+')
    {
    	addtm(this,tm->tm_sec, tm->tm_min, tm->tm_hour,
    	           tm->tm_mday, tm->tm_mon, tm->tm_year);
    	*tm= *this;
    }
    else if (relative == '-')
    {
    	subtm(this,tm->tm_sec, tm->tm_min, tm->tm_hour,
    	           tm->tm_mday, tm->tm_mon, tm->tm_year);
    	*tm= *this;
    }

    return mktimez(tm,tz);
}


/* CTIMEZ, MKTIMEZ, LOCALTIMEZ, ASCTIMEZ - These routines are equilvalent to
 * the standard ctime(), mktime(), and localtime() routines, but take a
 * timezone argument.  This has the same syntax as the Unix TZ environment
 * variable.  If the timezone is NULL or an empty string, these routines
 * behave exactly like the standard ones.  Otherwise, they do time
 * computations in the specified timezone.
 *
 * Some caching of state information is done to avoid excess recomputation when
 * we switch back and forth between different timezones.
 *
 * The set_tz() helper routine does this.  It creates a fake environment with
 * TZ set to the desired timezone, and substitutes that in.  If the timezone
 * information has changed since the last call, set_tz() call tzset() to make
 * sure that it gets loaded by the system.  To restore things to normal after
 * a call to tzset, you should do "environ= oldenv".
 *
 * Note that it is not safe to mix-and-match calls to these with calls to
 * the standard functions, since it isn't obvious what timezones the calls
 * to the standard functions will use (the one in the TZ environment variable,
 * or the one in the global variables maintained by tzset()).
 */

char *tzenv[2]= {NULL,NULL};
char **oldenv;
extern char **environ;

void set_tz(char *tz)
{
    /* Always save a pointer to the environment */
    oldenv= environ;

    /* If no timezone set, and none set before, do nothing else */
    if ((tz == NULL || *tz == '\0') && tzenv[0] == NULL)
    	return;

    /* If same timezone set as before, reuse old tz environment */
    if (tz != NULL && tzenv[0] != NULL && !strcmp(tz,tzenv[0]+3))
    {
	oldenv= environ;
	environ= tzenv;
	/* Don't need to call tzset() here */
	return;
    }

    /* If we get here, time zone passed in is different than it was before */

    /* Discard old tz info */
    if (tzenv[0] != NULL) free(tzenv[0]);

    if (tz == NULL || *tz == '\0')
    {
	/* setting back to no time zone */
	tzenv[0]= NULL;
    }
    else
    {
	/* setting a new time zone: build a fake environment */
	tzenv[0]= (char *)malloc(strlen(tz)+4);
	sprintf(tzenv[0],"TZ=%s",tz);

	/* Load in the new environment */
	environ= tzenv;
    }

    /* Force system to re-evaluate timezone */
    tzset();

    return;
}

char *ctimez(time_t *clock, char *tz)
{
    char *c;

    set_tz(tz);
    c= ctime(clock);
    environ= oldenv;
    return c;
}

struct tm *localtimez(time_t *clock, char *tz)
{
    struct tm *tm;

    set_tz(tz);
    tm= localtime(clock);
    environ= oldenv;
    return tm;
}

time_t mktimez(struct tm *tm, char *tz)
{
    time_t clock;
#ifndef HAVE_MKTIME
    struct tm *ctm;
#endif

    set_tz(tz);
#ifdef HAVE_MKTIME
    clock= mktime(tm);
#else
    /* Do conversion assuming 'tm' is in GMT */
    clock= tm2sec(tm);
    /* Clumsy repair of time zone */
    for (;;)
    {
    	ctm= localtime(&clock);
	if (ctm->tm_sec != tm->tm_sec)
	   clock+= tm->tm_sec - ctm->tm_sec;
	else if (ctm->tm_min != tm->tm_min)
	   clock+= (tm->tm_min - ctm->tm_min)*60;
	else if (ctm->tm_year != tm->tm_year)
	   clock+= (ctm->tm_year > tm->tm_year) ? -3600 : 3600;
	else if (ctm->tm_mon != tm->tm_mon)
	   clock+= (ctm->tm_mon > tm->tm_mon) ? -3600 : 3600;
	else if (ctm->tm_mday != tm->tm_mday)
	   clock+= (ctm->tm_mday > tm->tm_mday) ? -3600 : 3600;
	else if (ctm->tm_hour != tm->tm_hour)
	   clock+= (tm->tm_hour - ctm->tm_hour)*3600;
	else
	   break;
    }
#endif
    environ= oldenv;
    return clock;
}


char *asctimez(struct tm *tm, char *tz)
{
    char *c;

    set_tz(tz);
    c= asctime(tm);
    environ= oldenv;
    return c;
}


#ifdef CLEANUP
void free_timezone()
{
    if (tzenv[0] != NULL) free(tzenv[0]);
}
#endif /* CLEANUP */


/* TZ_OFFSET - Return the number of minutes that the current timezone is
 * offset from GMT.  Pass in something returned from a call to localtime(),
 * which may or may not be useful to us.
 */

int tz_offset(struct tm *tm)
{
#ifdef HAVE_TM_GMTOFF
    return (tm->tm_gmtoff / 60) - (tm->tm_isdst ? 60 : 0);
#else
#ifdef HAVE_TZSET
    extern long timezone;
    tzset();   /* This is probably unnecessary since localtime() called it */
    return -(timezone / 60);
#else
    struct timeb tb;
    ftime (&tb);
    return (-tb.timezone);
#endif
#endif

}


/* Some character constants shared by http_time() and mime_time() */
#ifndef HAVE_STRFTIME
static char *wkd= "SunMonTueWedThuFriSat";
static char *mon= "JanFebMarAprMayJunJulAugSepOctNovDec";
#endif /* !HAVE_STRFTIME */


/* HTTP_TIME - Convert a Unix time integer to a string in the http format:
 *    Wdy, DD-Mon-YYYY HH:MM:SS GMT
 * The result is stored in the given buffer, which should always be at
 * least 30 bytes long.
 */

char *http_time(time_t time, char *bf)
{
    struct tm *gmt;

    gmt= gmtime(&time);
#ifdef HAVE_STRFTIME
    strftime(bf, 30, "%a, %d-%b-%Y %H:%M:%S GMT",gmt);
#else
    sprintf(bf, "%3.3s, %02d-%3.3s-%04d %02d:%02d:%02d GMT",
	wkd[gmt->tm_wday*3],
	gmt->tm_mday, mon[gmt->tm_mon*3], gmt->tm_year+1900,
	gmt->tm_hour, gmt->tm_min, gmt->tm_sec);
#endif /* HAVE_STRFTIME */
    return bf;
}


/* MIME_TIME - Convert a Unix time integer to a string in the RFC822 format:
 *    Mon, 27 Jan 2003 09:11:25 -0500
 * This the the date format used in email.  The buffer passed in must be at
 * least 32 characters long.
 *
 * If we have Gnu strftime() this could be done with the format string
 * "%a, %d %b %Y %H:%M:%S %z", however the %z directive is not understood
 * by most strftime() implementations.
 */

char *mime_time(time_t time, char *bf)
{
    struct tm *tm;
    int offset;

    tm= localtime(&time);
    offset= tz_offset(tm);

#ifdef HAVE_STRFTIME

    strftime(bf, 27, "%a, %d %b %Y %H:%M:%S ",tm);
    sprintf(bf+26,"%c%02d%02d", offset < 0 ? '-' : '+', abs(offset)/60,
	    abs(offset)%60);

#else
    sprintf(bf, "%3.3s, %02d %3.3s %04d %02d:%02d:%02d %c%02d%02d",
	wkd[gmt->tm_wday*3],
	gmt->tm_mday, mon[gmt->tm_mon*3], gmt->tm_year+1900,
	gmt->tm_hour, gmt->tm_min, gmt->tm_sec,
	offset < 0 ? '-' : '+', abs(offset)/60, abs(offset)%60 );
#endif /* HAVE_STRFTIME */
    return bf;
}
