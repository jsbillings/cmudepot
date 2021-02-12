/***********************************************************
        Copyright 1991 by Carnegie Mellon University

                      All Rights Reserved

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the name of CMU not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.

CMU DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
CMU BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.
******************************************************************/

#ifndef _GLOBALS_H
#define _GLOBALS_H

/* $Header: /afs/andrew.cmu.edu/system/src/local/depot/019/RCS/globals.h,v 4.5 1992/08/14 18:55:42 ww0r Exp $ */

/*
 * Author: Sohan C. Ramakrishna Pillai
 */


#include <sys/types.h>
#include <strings.h>

#include "DepotErrorCodes.h"

/* boolean constants */
#define TRUE	1
#define FALSE	0
typedef unsigned Boolean;

#undef DEBUG
#ifdef DEBUG

#define F_NULL		0x000000
#define F_CHECK		0x000001
#define F_TRACE		0x000002
#define F_DBREAD	0x000004
#define F_PREFREAD	0x000008
#define F_DBSOURCEADD	0x000010
#define F_LOCATEENTRY	0x000020
#define F_DBAPPLY	0x000040
#define F_DELETEPATH	0x000080
#define F_CONFREAD	0x000100
#define F_COLLISTADD	0x000200
#define F_COLLMAP	0x000400
#define F_DEFLOWERPATH	0x000800
#define F_STRARRINSERT	0x001000
#define F_COLLSEARCH	0x002000
#define F_COLLGETLIST	0x004000
#define F_APPLYCHECK	0x008000
#define F_STRARRSPLIT	0x010000
#define F_EXECRCFILE	0x020000
#define F_TRUSTTARGET	0x040000
#ifdef USE_FSINFO
#define F_AFSINFO	0x0100000
#endif USE_FSINFO
#define F_TARGETMAPPATH	0x0200000

#define FDEBUG ( F_NULL )
#define dbgprint(flag, x)   {if (FDEBUG & (flag)) {(void)fprintf x ; (void)fflush(stderr);}}
#else DEBUG
#define dbgprint(flag, x)
#endif DEBUG

#define FatalError(errcode, x)  { (void)fprintf x ; (void)fflush(stderr); exit (errcode); }
#define WarningError(x)  { (void)fprintf x ; (void)fflush(stderr); }

#define DepotError(errcode, x)  { (void)fprintf x ; (void)fflush(stderr); exit (errcode); }

#define LIST_next(list) ((list)->_next)

extern char *emalloc();
extern char *ecalloc();
extern char *erealloc();

extern FILE *efopen();

extern char **strarrcpy();
extern char **strarrcat();
extern int strarrsize();
extern void strarrfree();
extern char **sortedstrarrinsert();
extern char **splitstrarr();

#ifdef sun
extern char *strerror();
#endif /* sun */

extern Boolean LocateExecFileInPATH();

void dbgDumpStringArray();
#endif /* _GLOBALS_H */
/* $Source: /afs/andrew.cmu.edu/system/src/local/depot/019/RCS/globals.h,v $ */

