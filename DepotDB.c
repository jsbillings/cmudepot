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


static char rcs_id[]="$Header: /afs/andrew.cmu.edu/system/src/local/depot/019/RCS/DepotDB.c,v 4.4 1992/06/19 20:45:42 ww0r Exp $";

/*
 * Author: Sohan C. Ramakrishna Pillai
 */

#include <stdio.h>
#include "globals.h"
#include "depot.h"
#include "DepotDBStruct.h"
#include "DepotDB.h"
#include "DepotDBVersion1.h"
#include "DepotDBCommandStruct.h"
#include "DepotDBCommand.h"
#include "PreferenceStruct.h"
#include "Preference.h"


DEPOTDB *DepotDB_Create()
{
  DEPOTDB *db;
  dbgprint(F_TRACE, (stderr, "DepotDB_Create - Version1\n"));
  db = DepotDB1_Create();
  dbgprint(F_TRACE, (stderr, "DepotDB_Create - Version1 done\n"));
  return db;
}
int DepotDB_GetVersionNumber(dbfile)
     FILE *dbfile;
{
  return DepotDB1_GetVersionNumber(dbfile);
}

DEPOTDB *DepotDB_Read(dbfile, PreferencesSavedp)
     FILE *dbfile;
     PREFERENCELIST **PreferencesSavedp;
{
  DEPOTDB *db;
  dbgprint(F_TRACE, (stderr, "DepotDB_Read - Version1\n"));
  db = DepotDB1_Read(dbfile, PreferencesSavedp);
  dbgprint(F_TRACE, (stderr, "DepotDB_Read - Version1 done\n"));
  return db;
}
Boolean DepotDB_Write(dbfile, db, preference)
     FILE *dbfile;
     DEPOTDB *db;
     PREFERENCELIST *preference;
{
  dbgprint(F_TRACE, (stderr, "DepotDB_Write - Version1\n"));
  DepotDB1_Write(dbfile, db, preference);
  dbgprint(F_TRACE, (stderr, "DepotDB_Write - Version1 done\n"));
  return TRUE;
}
DEPOTDB *DepotDB_Append(db1, db2)
     DEPOTDB *db1, *db2;
{
  DEPOTDB *newdb;
  dbgprint(F_TRACE, (stderr, "DepotDB_Append - Version 1\n"));
  newdb = DepotDB1_Append(db1, db2);
  dbgprint(F_TRACE, (stderr, "DepotDB_Append - Version 1 done\n"));
  return newdb;
}
Boolean DepotDB_Apply(db, applspec, commandfilelist)
     DEPOTDB *db;
     unsigned applspec;
     COMMANDFILE ***commandfilelist;
{
  Boolean ret_val;
  dbgprint(F_TRACE, (stderr, "DepotDB_Apply - Version1\n"));
  ret_val = DepotDB1_Apply(db, applspec, commandfilelist);
  dbgprint(F_TRACE, (stderr, "DepotDB_Apply - Version1 done\n"));
  return ret_val;
}

DEPOTDB *DepotDB_DeletePath(db, path, deleteflags)
DEPOTDB *db;
char *path;
Boolean deleteflags;
{
  DEPOTDB *newdb;

  dbgprint(F_TRACE, (stderr, "DepotDB_DeletePath - Version 1\n"));
  newdb = DepotDB1_DeletePath(db, path, deleteflags);

  dbgprint(F_TRACE, (stderr, "DepotDB_DeletePath done\n"));
  return newdb;
}

DEPOTDB *DepotDB_SetNonVirginPath(db, path, colname)
     DEPOTDB *db;
     char *path, *colname;
{
  DEPOTDB *newdb;

  dbgprint(F_TRACE, (stderr, "DepotDB_SetNonVirginPath - Version 1\n"));
  newdb = DepotDB1_SetNonVirginPath(db, path, colname);

  dbgprint(F_TRACE, (stderr, "DepotDB_SetNonVirginPath done\n"));
  return newdb;
}

void DepotDB_SelfReferenceRoot(db)
     DEPOTDB *db;
{
  dbgprint(F_TRACE, (stderr, "DepotDB_SelfReferenceRoot - Version 1\n"));
  DepotDB1_SelfReferenceRoot(db);
  dbgprint(F_TRACE, (stderr, "DepotDB_SelfReferenceRoot done\n"));
  return;
}

DEPOTDB *DepotDB_RemoveCollection(db, colname)
     DEPOTDB *db;
     char *colname;
{
  DEPOTDB *newdb;
  dbgprint(F_TRACE, (stderr, "DepotDB_RemoveCollection - Version 1\n"));
  newdb = DepotDB1_RemoveCollection(db, colname);
  dbgprint(F_TRACE, (stderr, "DepotDB_RemoveCollection done\n"));
  return newdb;
}

DEPOTDB *DepotDB_ObsoleteCollection(db, colname)
     DEPOTDB *db;
     char *colname;
{
  DEPOTDB *newdb;
  dbgprint(F_TRACE, (stderr, "DepotDB_ObsoleteCollection - Version 1\n"));
  newdb = DepotDB1_ObsoleteCollection(db, colname);
  dbgprint(F_TRACE, (stderr, "DepotDB_ObsoleteCollection done\n"));
  return newdb;
}

DEPOTDB *DepotDB_PruneCollection(db, colname)
     DEPOTDB *db;
     char *colname;
{
  DEPOTDB *newdb;
  dbgprint(F_TRACE, (stderr, "DepotDB_PruneCollection - Version 1\n"));
  newdb = DepotDB1_PruneCollection(db, colname);
  dbgprint(F_TRACE, (stderr, "DepotDB_PruneCollection done\n"));
  return newdb;
}

void DepotDB_Antiquate(db)
     DEPOTDB *db;
{
  dbgprint(F_TRACE, (stderr, "DepotDB_Antiquate - Version 1\n"));
  DepotDB1_Antiquate(db);
  dbgprint(F_TRACE, (stderr, "DepotDB_Antiquate done\n"));
  return;
}

void DepotDB_ProtectSpecialFiles(db, filelist)
     DEPOTDB *db;
     char **filelist;
{
  dbgprint(F_TRACE, (stderr, "DepotDB_ProtectSpecialFiles - Version 1\n"));
  DepotDB1_ProtectSpecialFiles(db, filelist);
  dbgprint(F_TRACE, (stderr, "DepotDB_ProtectSpecialFiles done\n"));
  return;
}

void DepotDB_SetTargetMappings(db)
     DEPOTDB *db;
{
  dbgprint(F_TRACE, (stderr, "DepotDB_SetTargetMappings - Version 1\n"));
  DepotDB1_SetTargetMappings(db);
  dbgprint(F_TRACE, (stderr, "DepotDB_SetTargetMappings done\n"));
  return;
}

DEPOTDB *DepotDB_UpdateEntry(db, ep)
     DEPOTDB *db;
     ENTRY	*ep;
{
  DEPOTDB *newdb;
  dbgprint(F_TRACE, (stderr, "DepotDB_UpdateEntry - Version 1\n"));
  newdb = DepotDB1_UpdateEntry(db, ep);
  dbgprint(F_TRACE, (stderr, "DepotDB_UpdateEntry done\n"));
  return newdb;
}

ENTRY *DepotDB_LocateEntryByName(db, ename, searchflags)
     DEPOTDB *db;
     char *ename;
     Boolean searchflags;
{
  ENTRY *ep;

  dbgprint(F_TRACE, (stderr, "DepotDB_LocateEntryByName - Version 1\n"));
  ep = DepotDB1_LocateEntryByName(db, ename, searchflags);
  dbgprint(F_TRACE, (stderr, "DepotDB_LocateEntryByName done\n"));

  return ep;
}

void DepotDB_SourceList_AddSource(entryp, srcp)
     ENTRY *entryp;
     SOURCE *srcp;
{
  dbgprint(F_TRACE, (stderr, "DepotDB_SourceList_AddSource - Version 1\n"));
  DepotDB1_SourceList_AddSource(entryp, srcp);
  dbgprint(F_TRACE, (stderr, "DepotDB_SourceList_AddSource\n"));
}

SOURCE *DepotDB_SourceList_LocateCollectionSourceByName(entryp, sourcename, colname, searchflags)
     ENTRY *entryp;
     char *sourcename;
     char *colname;
     Boolean searchflags;
{
  register SOURCE *sp;

  dbgprint(F_TRACE, (stderr, "DepotDB_SourceList_LocateCollectionSourceByName\n"));
  sp = DepotDB1_SourceList_LocateCollectionSourceByName(entryp, sourcename, colname, searchflags);
  dbgprint(F_TRACE, (stderr, "DepotDB_SourceList_LocateCollectionSourceByName done\n"));
  return sp;
}


void DepotDB_FreeSourceList(entryp)
     ENTRY *entryp;
{
  dbgprint(F_TRACE, (stderr, "DepotDB_FreeSourceList - Version 1\n"));
  DepotDB1_FreeSourceList(entryp);
  dbgprint(F_TRACE, (stderr, "DepotDB_FreeSourceList done\n"));
  return;
}

Boolean DepotDB_AntiqueEntry(entryp)
     ENTRY *entryp;
{
  Boolean ret_val;
  dbgprint(F_TRACE, (stderr, "DepotDB_AntiqueEntry - Version 1\n"));
  ret_val = DepotDB1_AntiqueEntry(entryp);
  dbgprint(F_TRACE, (stderr, "DepotDB_AntiqueEntry done\n"));
  return ret_val;
}
/* $Source: /afs/andrew.cmu.edu/system/src/local/depot/019/RCS/DepotDB.c,v $ */

