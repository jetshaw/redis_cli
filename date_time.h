/*
 * file name: date_time.h
 * purpose  :
 * author   : huang chunping
 * date     : 2014-09-24
 * history  :
 */
#ifndef __DATE_TIME_H__
#define __DATE_TIME_H__

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <time.h>
#include <sys/time.h>

class DateTime {
public:
	DateTime() {
		struct timeval tv;
		gettimeofday(&tv, NULL);
		usecond = tv.tv_usec;
		elapsed_seconds = tv.tv_sec;
		elapsed_useconds = usecond + elapsed_seconds*1000000;

		struct tm now;
		localtime_r(&tv.tv_sec, &now);
		year = now.tm_year + 1900;
		month = now.tm_mon + 1;
		day = now.tm_mday;
		hour = now.tm_hour;
		minute = now.tm_min;
		second = now.tm_sec;
		snprintf(date_str, sizeof(date_str)-1, "%04d-%02d-%02d %02d:%02d:%02d.%d",
				 year, month, day, hour, minute, second, usecond);
	}

	~DateTime() {
	}

public:
	int year;
	int month;
	int day;

	int hour;
	int minute;
	int second;
	int usecond;

	unsigned int elapsed_seconds;
	unsigned long long elapsed_useconds;

	char date_str[64];
};

#endif // __DATE_TIME_H__
