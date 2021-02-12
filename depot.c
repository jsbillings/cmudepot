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

static char rcs_id[]="$Header: /afs/andrew.cmu.edu/system/src/local/depot/019/RCS/depot.c,v 4.21 1992/08/14 18:55:19 ww0r Exp $";

/*
 * Author: Sohan C. Ramakrishna Pillai
 */

#include <stdio.h>
#include <pwd.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <errno.h>
#include <strings.h>

#include "AndrewFileSystem.h"
#include "globals.h"
#include "depot.h"
#include "Lock.h"
#include "PreferenceStruct.h"
#include "Preference.h"
#include "DepotDBStruct.h"
#include "DepotDB.h"
#include "DepotDBCommandStruct.h"
#include "DepotDBCommand.h"
#include "CollectionStruct.h"
#include "Collection.h"
#include "patchlevel.h"

#ifdef ibm032
extern int errno;
#endif /* ibm032 */

/* KLUDGE */
PREFERENCELIST *preference;
PREFERENCELIST *PreferencesSaved;

COLLECTIONLIST *Depot_CollectionList;
Boolean Depot_DeleteUnReferenced = TRUE;
Boolean Depot_UseModTimes = FALSE;		/* option -t */
Boolean Depot_RepairMode = FALSE;     		/* option -R */
Boolean Depot_QuickUpdate = FALSE;		/* option -q */
unsigned Depot_UpdateMode = 0;			/* options -n, -v */
Boolean Depot_DeleteMode = FALSE;		/* option -d */
Boolean Depot_TrustTargetDir = FALSE;		/* option -Q */

char *Depot_TargetPath = DEFAULTDEPOTTARGETPATH;
char  Depot_VersionDelimiter = DEFAULTDEPOTVERSIONDELIMITER;

static char **CollectionNameList;
static Boolean BuildDepotDB = FALSE;	/* option -B */
static Boolean UpdateAllCollections = FALSE;	/* option -a */
static Boolean UseSpecifiedCollectionsOnly = FALSE; /* option -c */
static Boolean ExplicitLockingRequired = TRUE;	/* option -i */
static unsigned LockAction = 0;		/* options -l, L, u, U, w */
char *CollectionListFileName = NULL;	/* option -f */

static void Usage();
static void HandleArgs();
static void CheckOptionsConsistency();
static char **GetCollectionNameList();
static char **GetCollectionNameListFromFile();
static void ProcessLockAction();
static void ProcessUpdates();
static void GetPreferenceOptions();


#define ERRORLINESIZE 80


static void Usage(ExitCode, ErrorMessage)
     int ExitCode;
     char *ErrorMessage;
{
  static  char *UsageString = "Usage: depot [-BLRUacdhilnqtuvx] [-T targetdir] [-f collection-list-file] [collection .. collection]";

  static struct DepotOption
    {
      char *option;
      char *description;
    } DepotOptionList[] =
      {
	{ "-B",		"Build database" },
	{ "-L",      	"Lock, by force if necessary" },
	{ "-Q",		"Quicker update, trust the targetdir to reflect the stored database" },
	{ "-R",      	"Repair targetdir" },
	{ "-U",		"Unlock, by force if necessary" },
	{ "-a",		"update All collections on searchpath" },
	{ "-c",		"use only explicitly specified Collections on searchpath" },
	{ "-d",      	"remove (Delete) specified collections" },
	{ "-h",      	"print Help message" },
	{ "-i",      	"locking not required by owner of lock (Implicit locking)" },
	{ "-l",      	"Lock" },
	{ "-n",      	"pretend to run, show update actions without actually updating" },
	{ "-q",      	"Quick update, run faster" },
	{ "-t",      	"Use modification Times for updating maps by copy" },
	{ "-u",      	"Unlock" },
	{ "-v",      	"Verbose output, show update actions" },
	{ "-x",		"print Help message" },
	{ "-T targetdir", "use targetdir as Target for depot" },
	{ "-f filename",  "list of collections to update is specified in the File filename" },
	{ "collection",	"collections to update" },
      };

  static unsigned NDepotOptions = sizeof(DepotOptionList)/sizeof(DepotOptionList[0]);
  register unsigned i;
  register struct DepotOption *op;
  FILE *outfile;

  outfile = (ExitCode == 0) ? stdout : stderr;
  if (ErrorMessage != NULL) WarningError((outfile, ErrorMessage));
  (void)fprintf(outfile, "%s: Version %03d\n", DepotPatchlevel, PATCHLEVEL);
  (void)fprintf(outfile, "%s\n\n", UsageString);
  for (i = 0, op = DepotOptionList; i < NDepotOptions; i++, op++)
    {
      (void)fprintf (outfile, "  %-12s %s\n", op->option, op->description);
    }
  (void)fflush(outfile);
  exit(ExitCode);
}

main(argc, argv)
int argc;
char **argv;
{
  dbgprint(F_TRACE, (stderr, "main\n"));
#ifdef USE_FSINFO
  if (AFS_vol_init() < 0) {
    FatalError(E_AFSPROBLEM, (stderr, "Unable to init VLDB connection\n"));
  }
#endif USE_FSINFO
  HandleArgs(argc, argv);
  if (Depot_UpdateMode & M_LOCKONLY)
    ProcessLockAction();
  else
    ProcessUpdates();
#ifdef USE_FSINFO
  AFS_vol_done();
#endif USE_FSINFO
  dbgprint(F_TRACE, (stderr, "main done\n"));
  exit(0);
}

static void ProcessUpdates()
{
  register COLLECTIONLIST *cp;
  register char **cpp;
  register unsigned i;
  struct passwd *pw;	       	/* password entry of the invoker of depot */
  char *DepotHighPriest;	/* Name of the invoker of depot */
  char *DepotLockOwner;		/* whoever has the depot databse locked now */
  Boolean ExplicitlyLocked;	/* TRUE if lock was explicitly set by depot */
  DEPOTDB *db_root;
  COLLECTION DestinationCollection;
  COLLECTIONLIST *KnownCollectionList;
  COMMANDFILE **CommandFileList;
  char PreferenceFileName[MAXPATHLEN], DepotDBFileName[MAXPATHLEN], NewDepotDBFileName[MAXPATHLEN], BackupDepotDBFileName[MAXPATHLEN], DepotLockFileName[MAXPATHLEN];
  FILE *PreferenceFile, *DepotDBFile, *NewDepotDBFile;
  char **SpecialFileList;
  char **IgnoreCollectionList;
  struct stat stb;
  int DepotDBVersionNo;
  
  dbgprint(F_TRACE, (stderr, "ProcessUpdates\n"));

  (void)sprintf(DepotDBFileName, "%s/%s/%s", Depot_TargetPath, DEPOTSPECIALDIRECTORY, DEPOTDBFILE);
  (void)sprintf(NewDepotDBFileName, "%s/%s/%s.NEW", Depot_TargetPath, DEPOTSPECIALDIRECTORY, DEPOTDBFILE);
  (void)sprintf(BackupDepotDBFileName, "%s/%s/%s.OLD", Depot_TargetPath, DEPOTSPECIALDIRECTORY, DEPOTDBFILE);
  (void)sprintf(PreferenceFileName, "%s/%s/%s", Depot_TargetPath, DEPOTSPECIALDIRECTORY, DEPOTPREFERENCESFILE);

  if (!Depot_RepairMode && ( ! ((lstat(NewDepotDBFileName, &stb) < 0) && (errno == ENOENT)) ))
    {
      FatalError(E_NEWDBEXISTS, (stderr, "A previous run of depot seems to be incomplete.\nRepair the target directory using the -R option\n"));
    }
    
  if ((PreferenceFile = fopen(PreferenceFileName, "r")) == (FILE *)NULL) {
    if (errno == ENOENT) {
      WarningError((stderr, "WARNING: No custom.depot found.\n"));
      preference = Preference_Read(NULL);
    } else {
      FatalError(E_FOPENFAILED, 
		 (stderr, "Error: could not open file\nfopen(%s, %s) failed\n", 
		  PreferenceFileName, "r"));
    }
  } else {
    preference = Preference_Read(PreferenceFile);
    fclose(PreferenceFile);
  }

#ifdef DEBUG
  if (FDEBUG & F_PREFREAD)
    {
      FILE *dbgf;
      dbgf = efopen("preferences.debug", "w");
      (void)Preference_Write(dbgf, preference);
      fclose(dbgf);
    }
#endif DEBUG

  GetPreferenceOptions(preference);

  if (!(Depot_UpdateMode & M_SHOWACTIONSONLY))
    {
      (void)sprintf(DepotLockFileName, "%s/%s/%s", Depot_TargetPath, DEPOTSPECIALDIRECTORY, DEPOTDBLOCKFILE);
      /* lock the database */
      if ((pw = getpwuid(getuid())) == NULL)
	{ FatalError(E_BADUSER, (stderr, "Attempt to invoke depot by unknown user\n")); }
      else
	{
	  DepotHighPriest = pw->pw_name;
	  DepotLockOwner = Lock_QueryLock(DepotLockFileName);
	  if (DepotLockOwner == NULL)
	    {
	      DepotLockOwner = Lock_SetLock(DepotLockFileName);
	      if (DepotLockOwner == NULL) /* locked successfully */
		{
		  DepotLockOwner = DepotHighPriest;
		  ExplicitlyLocked = TRUE;
		}
	      else
		ExplicitlyLocked = FALSE;
	    }
	  else
	    ExplicitlyLocked = FALSE;
	  if (strcmp(DepotLockOwner, DepotHighPriest) != 0)
	    { FatalError(E_CANTLOCK, (stderr, "Error: Depot in use by %s\n", DepotLockOwner)); }
	  else if (ExplicitLockingRequired && !ExplicitlyLocked)
	    { FatalError(E_CANTLOCK, (stderr, "Error: Depot already in use by %s\n", DepotLockOwner)); }
	}
    }

  if (BuildDepotDB)
    { db_root = DepotDB_Create(); }
  else
    {
      DepotDBFile = efopen(DepotDBFileName, "r");
      DepotDBVersionNo = DepotDB_GetVersionNumber(DepotDBFile);
      if (   (DepotDBVersionNo != DEPOTDB_VERSION)
	  && (DepotDBVersionNo != COMPAT_DEPOTDB_VERSION))
	{
	  FatalError(E_BADVERSION, (stderr, "The database file Version %d is out of date, please rebuild with the -B option to update to the current Version %d\n", DepotDBVersionNo, DEPOTDB_VERSION));
	}
      db_root = DepotDB_Read(DepotDBFile, &PreferencesSaved);
      (void)fclose(DepotDBFile);
    }

  if (!Depot_RepairMode)
    {
      if (Depot_DeleteMode)
	{
	  if (UpdateAllCollections)
	    Depot_CollectionList = CollectionList_GetAllKnownCollections(db_root);
	  else
	    Depot_CollectionList = CollectionList_GetKnownCollections(db_root, CollectionNameList);

	  for (cp = Depot_CollectionList; cp; cp=COLLECTION_next(cp))
	    {
	      db_root = DepotDB_ObsoleteCollection(db_root, cp->collection.name);
	    }
	}
      else
	{
	  if (UseSpecifiedCollectionsOnly)
	    {
	      /*
	       * Obsolete all known collections;
	       * the specified collections will be updated later.
	       */
	      KnownCollectionList = CollectionList_GetAllKnownCollections(db_root);
	      for (cp = KnownCollectionList; cp; cp=COLLECTION_next(cp))
		{
		  db_root = DepotDB_ObsoleteCollection(db_root, cp->collection.name);
		}
	      /* free the KnownCollectionList */
	      CollectionList_FreeList(KnownCollectionList);
	      KnownCollectionList = NULL;
	    }
	  if (UpdateAllCollections)
	    Depot_CollectionList = CollectionList_GetAllCollections();
	  else
	    Depot_CollectionList = CollectionList_GetCollectionList(CollectionNameList);
	  for (cp = Depot_CollectionList; cp; cp=COLLECTION_next(cp))
	    {
	      if (BuildDepotDB)
		Collection_Read(&cp->collection, NULL);
	      else if (UseSpecifiedCollectionsOnly)
		Collection_Read(&cp->collection, db_root);
	      else
		{
		  db_root = DepotDB_ObsoleteCollection(db_root, cp->collection.name);
		  Collection_Read(&cp->collection, db_root);
		}
	      db_root = DepotDB_Append(db_root, cp->collection.info);
	    }
	}
      DestinationCollection.name = "";
      DestinationCollection.path = ".";
      DestinationCollection.info = NULL;
#ifdef USE_FSINFO
      DestinationCollection.fslist = NULL;
#endif USE_FSINFO
      db_root = DepotDB_ObsoleteCollection(db_root, DestinationCollection.name);
      if  (!Depot_DeleteUnReferenced)
	{
	  Collection_Read(&DestinationCollection,db_root);
	  db_root = DepotDB_Append(db_root, DestinationCollection.info);
	}
      DepotDB_Antiquate(db_root);
      db_root = DepotDB_PruneCollection(db_root, DestinationCollection.name);

      IgnoreCollectionList = Preference_GetStringArray(preference, NULL, "ignore");
      if (IgnoreCollectionList != NULL)
	{
	  for (cpp = IgnoreCollectionList; *cpp != NULL; cpp++)
	    {
	      db_root = DepotDB_RemoveCollection(db_root, *cpp);
	    }
	}

      if (UseSpecifiedCollectionsOnly)
	{
	  /*
	   * Prune all known collections;
	   * the explicitly specified collections have been updated.
	   */
	  KnownCollectionList = CollectionList_GetAllKnownCollections(db_root);
	  for (cp = KnownCollectionList; cp; cp=COLLECTION_next(cp))
	    {
	      db_root = DepotDB_PruneCollection(db_root, cp->collection.name);
	    }
	  /* free the KnownCollectionList */
	  CollectionList_FreeList(KnownCollectionList);
	  KnownCollectionList = NULL;
	}
      else
	{
	  for (cp = Depot_CollectionList; cp; cp=COLLECTION_next(cp))
	    {
	      db_root = DepotDB_PruneCollection(db_root, cp->collection.name);
	    }
	}

      /*
       * if all entries have been removed from the database,
       *	create a dummy entry for the top level
       */
      if (db_root == NULL)
	db_root = DepotDB_Create();
    }

  if (!UpdateAllCollections && !Depot_QuickUpdate && !UseSpecifiedCollectionsOnly)
    DepotDB_SetTargetMappings(db_root);

  /* protect depot's special files */
  SpecialFileList = NULL;
  for ( i = 0; i < NDepot_SpecialFiles; i++)
    { SpecialFileList = sortedstrarrinsert(SpecialFileList, Depot_SpecialFileList[i].filename); }
  if (SpecialFileList)
    {
      DepotDB_ProtectSpecialFiles(db_root, SpecialFileList);
      strarrfree(SpecialFileList);
    }
  /* protect user's special files */
  SpecialFileList = Preference_GetStringArray(preference, NULL, "specialfile");
  if (SpecialFileList)
    {
      DepotDB_ProtectSpecialFiles(db_root, SpecialFileList);
      strarrfree(SpecialFileList);
    }
  if ((db_root->nsources == 1) && !(db_root->sourcelist->status & S_NONVIRGINSRC))
    db_root->sourcelist->status |= S_NONVIRGINSRC;

  CommandFileList = NULL;
  DepotDB_Apply(db_root, APPL_CHECK, &CommandFileList);
  if (!(Depot_UpdateMode & M_SHOWACTIONSONLY))
    {
      /* save the new database to a new file */
      NewDepotDBFile = efopen(NewDepotDBFileName, "w");
      DepotDB_Write(NewDepotDBFile, db_root, preference);
      (void)fclose(NewDepotDBFile);

      /* applythe new database to the target directory */
      DepotDB_Apply(db_root, APPL_UPDATE, &CommandFileList);

      /* backup the old dbfile and move the new db file */
      VerboseMessage((stdout, "Backing up old database .."));
      if (rename(DepotDBFileName, BackupDepotDBFileName) < 0)
	{
	  if ((errno != ENOENT) || !BuildDepotDB)
	    { FatalError(E_DBBACKFAILED, (stderr, "Could not backup database file %s to %s\n", DepotDBFileName, BackupDepotDBFileName)); }
	}
      VerboseMessage((stdout, " done\n"));

      VerboseMessage((stdout, "Moving in new database .."));
      if (rename(NewDepotDBFileName, DepotDBFileName) < 0)
	{ FatalError(E_DBUPDTFAILED, (stderr, "Could not move new database file %s to %s\n", NewDepotDBFileName, DepotDBFileName)); }
      VerboseMessage((stdout, " done\n"));

      if (ExplicitlyLocked)
	{
	  Lock_UnsetLock(DepotLockFileName);
	}
    }

  dbgprint(F_TRACE, (stderr, "ProcessUpdates done\n"));
  return;
}


static void HandleArgs(argc, argv)
int argc;
char **argv;
{
  register char *cp;
  Boolean eooptions;
  char UsageErrorMessage[ERRORLINESIZE];

  dbgprint(F_TRACE, (stderr, "HandleArgs\n"));
  argv++,argc--;
  eooptions = FALSE;
  while (!eooptions && (argc > 0))
    {
      cp = *argv;
      if (*cp != '-')
	eooptions = TRUE;
      else
	{
	  while ( *(++cp) != '\0')
	    {
	      switch (*cp)
		{
		case 'R':
		  Depot_RepairMode = TRUE;
		  break;
		case 'B':
		  BuildDepotDB = TRUE;
		  break;
		case 'Q':
		  Depot_TrustTargetDir = TRUE;
		  break;
		case 'T':
		  argv++, argc--;
		  if (argc <= 0) Usage(E_NULLTARGET, "No Target Specified\n");
		  Depot_TargetPath = emalloc(strlen(*argv)+1);
		  Depot_TargetPath = strcpy(Depot_TargetPath, *argv);
		  break;
		case 'L':
		  LockAction |= L_PICKLOCK; Depot_UpdateMode |= M_LOCKONLY;
		  break;
		case 'U':
		  LockAction |= L_PICKLOCK | L_UNLOCK; Depot_UpdateMode |= M_LOCKONLY;
		  break;
		case 'l':
		  LockAction |= L_LOCK; Depot_UpdateMode |= M_LOCKONLY;
		  break;
		case 'u':
		  LockAction |= L_UNLOCK; Depot_UpdateMode |= M_LOCKONLY;
		  break;
		case 'w':
		  LockAction |= L_QUERYLOCK; Depot_UpdateMode |= M_LOCKONLY;
		  break;
		case 'n':
		  Depot_UpdateMode |= M_SHOWACTIONSONLY;
		  break;
		case 'v':
		  Depot_UpdateMode |= M_VERBOSE;
		  break;
		case 'd':
		  Depot_DeleteMode = TRUE;
		  break;
		case 'a':
		  UpdateAllCollections = TRUE;
		  break;
		case 'c':
		  UseSpecifiedCollectionsOnly = TRUE;
		  break;
		case 'f':
		  argv++, argc--;
		  if (argc <= 0) Usage(E_UNNAMEDCOLLISTFILE, "File with list of collections not specified\n");
		  CollectionListFileName = emalloc(strlen(*argv)+1);
		  CollectionListFileName = strcpy(CollectionListFileName, *argv);
		  break;
		case 'q':
		  Depot_QuickUpdate = TRUE;
		  break;
		case 'i':
		  ExplicitLockingRequired = FALSE;
		  break;
		case 't':
		  Depot_UseModTimes = TRUE;
		  break;
		case 'h':
		case 'x':
		  Usage(0, NULL);
		  break;
		default:
		  (void)sprintf(UsageErrorMessage, "Bad command line option %c\n", *cp);
		  Usage(E_BADOPTION, UsageErrorMessage);
		  break;
		}
	    }
	  argv++, argc--;
	}
    }
  CheckOptionsConsistency();
  if (eooptions && (argc >0))
    CollectionNameList = GetCollectionNameList(argc, argv);
  else
    CollectionNameList = NULL;
  if (CollectionListFileName != NULL)
    CollectionNameList = GetCollectionNameListFromFile(CollectionListFileName, CollectionNameList);
  dbgprint(F_TRACE, (stderr, "HandleArgs done - partially implemented\n"));
}



static void CheckOptionsConsistency()
{
  dbgprint(F_TRACE, (stderr, "CheckOptionsConsistency - not implemented\n"));
}



static void ProcessLockAction()
{
  char DepotLockFileName[MAXPATHLEN];
  char *rval;

  dbgprint(F_TRACE, (stderr, "ProcessLockAction - being implemented\n"));

  (void)sprintf(DepotLockFileName, "%s/%s/%s", Depot_TargetPath, DEPOTSPECIALDIRECTORY, DEPOTDBLOCKFILE);
  if (LockAction & L_PICKLOCK)
    {
      rval = Lock_PickLock(DepotLockFileName);
      if (rval == NULL)
	{ fprintf(stdout, "Picked lock.\n"); }
      else
	{FatalError(E_FOOLPROOFLOCK, (stderr, "Error: Picking of lock failed - locked by %s.\n", rval));}
    }
  else if (LockAction & L_LOCK)
    {
      rval = Lock_SetLock(DepotLockFileName);
      if (rval == NULL)
	{ fprintf(stdout, "Locked lock.\n"); }
      else
	{ FatalError(E_CANTLOCK, (stderr, "Error: Lock failed - locked by %s\n", rval));}
    }
  if (LockAction & L_UNLOCK)
    {
      rval = Lock_UnsetLock(DepotLockFileName);
      if (rval == NULL)
	{ fprintf(stdout, "Unlocked lock\n"); }
      else
	{ FatalError(E_CANTLOCK, (stderr, "Error: Unlock failed - locked by %s\n", rval));}
    }
  if (LockAction & L_QUERYLOCK)
    {
      rval = Lock_QueryLock(DepotLockFileName);
      if ( rval == NULL )
	{ fprintf(stdout, "lock not locked\n"); }
      else
	{ fprintf(stdout, "lock locked by %s\n", rval); }
    }

  dbgprint(F_TRACE, (stderr, "ProcessLockAction done\n"));
  return;
}


static void GetPreferenceOptions(p)
     PREFERENCELIST *p;
{
  char *delimstr;

  if (Preference_GetBoolean(p, NULL, "deleteunreferenced") == FALSE)
    Depot_DeleteUnReferenced = FALSE;
  if (!Depot_UseModTimes)
    {
      if (Preference_GetBoolean(p, NULL, "usemodtimes") == TRUE)
	Depot_UseModTimes = TRUE;
    }
  if ((delimstr = Preference_GetString(p, NULL, "versiondelimiter")) == NULL)
    Depot_VersionDelimiter = DEFAULTDEPOTVERSIONDELIMITER;
  else
    Depot_VersionDelimiter = *delimstr;
}


static char **GetCollectionNameList(argc, argv)
     int argc;
     char **argv;
{
  char **clist;
  char *cname;

  dbgprint(F_TRACE, (stderr, "GetCollectionNameList\n"));
  clist = NULL;
  while (argc > 0)
    {
      argc--;
      cname = *argv++;
      clist = sortedstrarrinsert(clist, cname);
    }

  dbgprint(F_TRACE, (stderr, "GetCollectionNameList done\n"));
  return clist;
}


static char **GetCollectionNameListFromFile(clistfile, clist)
     char *clistfile;
     char **clist;
{
  FILE *fp;
  char cname[BUFSIZ];

  dbgprint(F_TRACE, (stderr, "GetCollectionNameListFromFile\n"));

  fp = efopen(clistfile, "r");
  while (fgets(cname, BUFSIZ, fp) != NULL) {
    char *ptr;
    
    if ((ptr = rindex(cname, '\n')) == NULL) { /* get rid of trailing \n */
      WarningError((stderr, "Warning: cname (%s) may be too long. Discarding it.\n",
		    cname));
    } else {
      *ptr = '\0';
      clist = sortedstrarrinsert(clist, cname);
    }
  }
  if (ferror(fp)) {
    FatalError(E_BADCOLLISTFILE, 
	       (stderr, "Error while reading list of collections from file %s\n", 
		clistfile));
    return NULL;
  }
  fclose(fp);

  dbgprint(F_TRACE, (stderr, "GetCollectionNameListFromFile done\n"));
  return clist;
}


/* $Source: /afs/andrew.cmu.edu/system/src/local/depot/019/RCS/depot.c,v $ */
