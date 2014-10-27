/*
 * file name: debug.h
 * purpose  :
 * author   : huang chunping
 * date     : 2014-09-24
 * history  :
 */

#ifndef __DEBUG_H__
#define __DEBUG_H__

#include <stdio.h>
#include "date_time.h"

#if defined(DEBUG)
#define INFO(format, ...) fprintf(stdout, "[INFO]"format,##__VA_ARGS__)
#define ERROR(format, ...) fprintf(stderr, "[ERROR]"format,##__VA_ARGS__)
#define TINFO(format, ...) fprintf(stdout, "[INFO][%s]"format, DateTime().date_str,##__VA_ARGS__)
#define TERROR(format, ...) fprintf(stderr, "[ERROR][%s]"format, DateTime().date_str,##__VA_ARGS__)
#else
#define INFO(format, ...)
#define ERROR(format, ...)
#define TINFO(format, ...)
#define TERROR(format, ...)
#endif

#endif // __DEBUG_H__
