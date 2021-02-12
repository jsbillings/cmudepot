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

#ifndef _DEPOT_H
#define _DEPOT_H

/* $Header: /afs/andrew.cmu.edu/system/src/local/depot/019/RCS/depot.h,v 4.6 1992/07/22 21:26:08 sohan Exp $ */

/*
 * Author: Sohan C. Ramakrishna Pillai
 */

#define DEPOTSPECIALDIRECTORY  	"depot"
#define DEPOTDBFILE		"struct.depot"
#define DEPOTDBLOCKFILE		"lock.depot"
#define DEPOTPREFERENCESFILE	"custom.depot"
#define COLLECTIONCONFIGFILE	"depot.conf"

#define DEFAULTDEPOTTARGETPATH	".."
#define DEFAULTCOLLECTIONPATH	DEPOTSPECIALDIRECTORY
#define DEFAULTDEPOTVERSIONDELIMITER '~'

/* modes in which depot could be run */
#define M_VERBOSE		001
#define M_SHOWACTIONSONLY	002
#define M_LOCKONLY		004

/* modes in which database may be applied */
#define APPL_CHECK	001
#define APPL_UPDATE	002

/* types of locking actions */
#define L_LOCK		001
#define L_UNLOCK       	002
#define L_PICKLOCK  	004
#define L_QUERYLOCK	010

#define VerboseMessage(x) \
  if (Depot_UpdateMode & M_VERBOSE) \
    {(void)fprintf x ; (void)fflush(stdout); }

#define ShowActionMessage(x) \
  if (Depot_UpdateMode & M_SHOWACTIONSONLY) \
    {(void)fprintf x ; (void)fflush(stdout); }

#define VerboseActionMessage(x) \
  if (Depot_UpdateMode & M_SHOWACTIONSONLY & M_VERBOSE) \
    {(void)fprintf x ; (void)fflush(stdout); }

extern Boolean	Depot_DeleteUnReferenced;
extern Boolean	Depot_UseModTimes;
extern Boolean	Depot_RepairMode;
extern Boolean	Depot_QuickUpdate;
extern unsigned	Depot_UpdateMode;
extern Boolean	Depot_DeleteMode;
extern Boolean	Depot_TrustTargetDir;
extern char *	Depot_TargetPath;
extern char	Depot_VersionDelimiter;

static struct Depot_SpecialFile
{
  char *filename;
} Depot_SpecialFileList[] =
{
  { DEPOTSPECIALDIRECTORY },
  { COLLECTIONCONFIGFILE },
};

static unsigned NDepot_SpecialFiles = sizeof(Depot_SpecialFileList)/sizeof(Depot_SpecialFileList[0]);

#endif _DEPOT_H
/* $Source: /afs/andrew.cmu.edu/system/src/local/depot/019/RCS/depot.h,v $ */
