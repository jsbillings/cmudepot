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

#ifndef _DEPOTDB_H
#define _DEPOTDB_H

/* $Header: /afs/andrew.cmu.edu/system/src/local/depot/019/RCS/DepotDB.h,v 4.9 1992/06/19 20:21:13 ww0r Exp $ */

/*
 * Author: Sohan C. Ramakrishna Pillai
 */


#define DEPOTDB_VERSION	5
#define COMPAT_DEPOTDB_VERSION	4

/* flags specifying update actions in depot database */
#define U_NOOP		0001
#define U_MAP		0002
#define U_MAPCOPY	0004
#define U_MAPLINK	0010
#define U_MKDIR		0020
#define U_RCFILE	0040
#define U_DELETE	0100

/* lookup modes for the database tree */
#define DB_LOCATE	001
#define DB_CREATE	002
#define DB_LAX		004

/* flags indicating status of entries in depot database */
#define S_NONVIRGINSRC	0x001	/* entry does not represent unmodified copy of source tree */
#define S_NONVIRGINTRG	0x002	/* entry has target-specific modifications in its subtree */
#define S_IMMACULATE	0x004	/* entry has a virgin parent */
#define S_ANTIQUE	0x008	/* entry was read in from previous database */
#define S_OBSOLETE	0x010	/* entry is obsolete and must be removed from database */
#define S_TARGET	0x020	/* entry corresponded to the targetdir before execution */
#define S_MODIFIED	0x040	/* entry modified after last application of depot */
#define S_SPECIAL	0x080	/* entry is special and should not be updated */
#define S_UPDATE	0x100	/* entry needs to applied to the depot target tree */
#define S_TRUSTTARGET	0x200	/* entry can be trusted to correspond to the expected target without application */

#ifdef USE_FSINFO
#define FS_UNKNOWN	0
#define FS_INHERIT	1
#define FS_NEWFILESYSTEM 2
#endif USE_FSINFO

/* Quoting character used to quote whitespace and returns in database file */
#define DepotDB_QUOTCHAR	0134

extern DEPOTDB *DepotDB_Create();
extern int	DepotDB_GetVersionNumber();
extern DEPOTDB *DepotDB_Read();
extern Boolean	DepotDB_Write();
extern Boolean	DepotDB_Apply();
extern DEPOTDB *DepotDB_Append();
extern DEPOTDB *DepotDB_RemoveCollection();
extern DEPOTDB *DepotDB_ObsoleteCollection();
extern DEPOTDB *DepotDB_PruneCollection();
extern void	DepotDB_Antiquate();
extern void	DepotDB_ProtectSpecialFiles();
extern void	DepotDB_SetTargetMappings();
extern DEPOTDB *DepotDB_UpdateEntry();
extern ENTRY   *DepotDB_LocateEntryByName();
extern DEPOTDB *DepotDB_DeletePath();
extern DEPOTDB *DepotDB_SetNonVirginPath();
extern void	DepotDB_SelfReferenceRoot();
extern void	DepotDB_SourceList_AddSource();
extern SOURCE  *DepotDB_SourceList_LocateCollectionSourceByName();
extern void	DepotDB_FreeSourceList();
extern Boolean	DepotDB_AntiqueEntry();

#endif /*_DEPOTDB_H */
/* $Source: /afs/andrew.cmu.edu/system/src/local/depot/019/RCS/DepotDB.h,v $ */
