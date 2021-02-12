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

#ifndef _PREFERENCETYPES_H
#define _PREFERENCETYPES_H

/* $Header: /afs/andrew.cmu.edu/system/src/local/depot/019/RCS/PreferenceTypes.h,v 4.8 1992/06/19 20:34:21 ww0r Exp $ */

/*
 * Author: Sohan C. Ramakrishna Pillai
 */



#define PREF_UNKNOWN	0
#define PREF_INT	1
#define PREF_UNSIGNED	2
#define PREF_BOOLEAN	3
#define PREF_STRING	4
#define PREF_STRINGARRAY 5
#define PREF_STRINGSET	6

#define PREFDEFAULT_INT		0
#define PREFDEFAULT_UNSIGNED	0
#define PREFDEFAULT_BOOLEAN	FALSE
#define PREFDEFAULT_STRING	NULL
#define PREFDEFAULT_STRINGARRAY NULL
#define PREFDEFAULT_STRINGSET	NULL


#define Preference_DefaultValue(x) \
  (((x) == PREF_INT) ? PREFDEFAULT_INT : \
   ((x) == PREF_UNSIGNED) ? PREFDEFAULT_UNSIGNED : \
   ((x) == PREF_BOOLEAN) ? PREFDEFAULT_BOOLEAN : \
   ((x) == PREF_STRING) ? PREFDEFAULT_STRING : \
   ((x) == PREF_STRINGARRAY) ? PREFDEFAULT_STRINGARRAY : \
   ((x) == PREF_STRINGSET) ? PREFDEFAULT_STRINGSET : \
   0 )


static struct PrefTypePair
{
  char *prefname;
  unsigned preftype;
} PrefTypeList[] =
{
/* Test for unsigned resources
  { "unsignedtypetest",	PREF_UNSIGNED },
 */
  /* resources for collections */
  { "path",		PREF_STRING },
  { "override",		PREF_STRINGSET },
  { "mapcommand",	PREF_STRING },

  { "searchpath",	PREF_STRINGARRAY },
  { "usemodtimes",	PREF_BOOLEAN },
  { "ignore",		PREF_STRINGSET },

  /* specification of a command to be executed */
  { "command",		PREF_STRING },

  { "deleteunreferenced",	PREF_BOOLEAN },
  /* specification of path-specific mappings */
  { "copytarget",	PREF_STRINGSET },
  { "linktarget",	PREF_STRINGSET },
  { "deletetarget",	PREF_STRINGSET },
  { "nooptarget",	PREF_STRINGSET },

  { "specialfile",	PREF_STRINGSET },
  { "version",		PREF_INT },
  { "versiondelimiter",	PREF_STRING },	/* actually char stored as string */
};

static unsigned NPrefTypes = sizeof(PrefTypeList)/sizeof(PrefTypeList[0]);

#endif /* _PREFERENCETYPES_H */
/* $Source: /afs/andrew.cmu.edu/system/src/local/depot/019/RCS/PreferenceTypes.h,v $ */
