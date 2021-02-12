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

#ifndef _PREFERENCE_H
#define _PREFERENCE_H

/* $Header: /afs/andrew.cmu.edu/system/src/local/depot/019/RCS/Preference.h,v 4.5 1992/06/19 20:26:13 ww0r Exp $ */


/*
 * Author: Sohan C. Ramakrishna Pillai
 */

#define PREFERENCELINESIZE 4*MAXPATHLEN

#define COLON	':'
#define DOT	'.'

#define STRINGARRAY_SEPARATOR	','

/* lookup modes for the preference list */
#define PL_LOCATE	001
#define PL_CREATE	002
#define PL_OVERWRITE	004

extern PREFERENCELIST	*Preference_Read();
extern Boolean	Preference_Write();

extern int	Preference_GetInt();
extern unsigned	Preference_GetUnsigned();
extern Boolean	Preference_GetBoolean();
extern char	*Preference_GetString();
extern char	**Preference_GetStringArray();
extern char	**Preference_GetStringSet();

extern PREFERENCELIST	*Preference_SetInt();
extern PREFERENCELIST	*Preference_SetUnsigned();
extern PREFERENCELIST	*Preference_SetBoolean();
extern PREFERENCELIST	*Preference_SetString();
extern PREFERENCELIST	*Preference_SetStringArray();
extern PREFERENCELIST	*Preference_SetStringSet();

/* util functions */
extern Boolean Override();
extern Boolean Ignore();
extern int VersionToUse();
extern char **Preference_GetItemList();
extern char **Preference_ExtractCommand();


/* KLUDGE */
extern PREFERENCELIST *preference;
extern PREFERENCELIST *PreferencesSaved;

#endif /* _PREFERENCE_H */
/* $Source: /afs/andrew.cmu.edu/system/src/local/depot/019/RCS/Preference.h,v $ */
