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


static char rcs_id[]="$Header: /afs/andrew.cmu.edu/system/src/local/depot/019/RCS/Collection.c,v 4.22 1992/08/14 18:54:35 ww0r Exp $";


/*
 * Author: Sohan C. Ramakrishna Pillai
 */

#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <math.h> /* for atoi */
#include <sys/types.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/dir.h>

#include "globals.h"
#include "depot.h"
#include "PreferenceStruct.h"
#include "Preference.h"
#include "DepotDBStruct.h"
#include "DepotDB.h"
#include "CollectionStruct.h"
#include "Collection.h"
#ifdef USE_FSINFO
#include "AndrewFileSystem.h"
#endif /* USE_FSINFO */

#ifdef ibm032
extern int errno;
#endif /* ibm032 */

static void Collection_Map();
static void Collection_AddCommand();
static Boolean Collection_FSMountPointFreeDir();
static void Collection_MakeNonVirginDirectoriesToPath();
static void dbgCollectionDB_Write();

#ifdef USE_FSINFO
static void Collection_ExtractFileSystemSubtree();
static COLLECTIONFSINFO *Collection_AddAFSMountPoint();
static COLLECTIONFSINFO *Collection_FileSystemInfo();
static COLLECTIONFSINFO *Collection_FSMountPointInfo();
static u_long Collection_ConfigTime();
#endif /* USE_FSINFO */

Boolean Collection_Read(colp, KnownDB)
COLLECTION *colp;
DEPOTDB *KnownDB;
{
  char confpath[MAXPATHLEN];
  int conffd;	/* file descriptor for config file */
  long confpos;	/* current position during scan of config file */
  register int ch;
  register char *cp;
  register int i;
  Boolean eoconf, eoword, eoline, QuotedChar;
  unsigned lineno;
  ENTRY thisentry; SOURCE thissrc;
  char path1[MAXPATHLEN], path2[MAXPATHLEN], src[MAXPATHLEN], trg[MAXPATHLEN];
  u_short UpdateType;
  SOURCE *sp;
#ifdef USE_FSINFO
  COLLECTIONFSINFO *fsinfo;
  struct stat confstb;
#endif /* USE_FSINFO */

  dbgprint(F_TRACE, (stderr, "Collection_Read\n"));
  dbgprint(F_CHECK, (stderr, "Collection_Read does not handle ill-written db files very well\n"));

  colp->info = NULL;
  if (colp->path[0] == '/')
    (void)sprintf(confpath, "%s/%s", colp->path, COLLECTIONCONFIGFILE);
  else
    (void)sprintf(confpath, "%s/%s/%s", Depot_TargetPath, colp->path, COLLECTIONCONFIGFILE);

  /*
   * First add an entry for the root of the collection database;
   */
  (void)strcpy(src, colp->path); (void)strcpy(trg, "/");
  thisentry.sibling = NULL; thisentry.child = NULL;
  thisentry.nsources = 0; thisentry.sourcelist = NULL;
  thisentry.name = trg; thisentry.status = 0;
  thissrc.update_spec = U_MKDIR; thissrc.old_update_spec = (u_short)0;
  thissrc.status = S_OBSOLETE; /* mark as obsolete to enable duplicate source addition */
  thissrc.name = src; thissrc.collection_name = colp->name;
#ifdef USE_FSINFO
  fsinfo = Collection_AddAFSMountPoint(colp, ".");
  if (fsinfo != NULL)
    {
      thissrc.fs_status = FS_NEWFILESYSTEM;
      thissrc.fs_id = fsinfo->fs_id;
      thissrc.fs_modtime = fsinfo->fs_modtime;
      /* Use the collection's volume info as the default configuration info */
      thissrc.col_id = fsinfo->fs_id;
      thissrc.col_conftime = fsinfo->fs_modtime;
      colp->confinfo = (COLLECTIONFSINFO *)emalloc(sizeof(COLLECTIONFSINFO));
      colp->confinfo->fs_id = fsinfo->fs_id;
      colp->confinfo->fs_modtime = fsinfo->fs_modtime;
      if ((stat(confpath, &confstb) < 0) && (errno != ENOENT))
	{ FatalError(E_STATFAILED, (stderr, "Could not stat %s: %s\n", confpath, strerror(errno))); }
      else
	{
	  if (errno == ENOENT) {
	    colp->confinfo->fs_modtime = 0;
	    thissrc.col_conftime = 0;
	  } else {
	    colp->confinfo->fs_modtime = confstb.st_mtime;
	    thissrc.col_conftime = confstb.st_mtime;
	  }
	}
      if (Collection_ConfigTime(KnownDB, colp) == colp->confinfo->fs_modtime)
	colp->fsuptodate = TRUE; /* initialize to TRUE till some changed file system is encountered */
      else
	colp->fsuptodate = FALSE;
    }
  else
    {
      thissrc.fs_status = FS_UNKNOWN;
      colp->fsuptodate = FALSE;
    }
#endif /* USE_FSINFO */
  DepotDB_SourceList_AddSource(&thisentry, &thissrc);
  dbgprint(F_CONFREAD, (stderr, "Adding top level path %s to collection database\n", trg));
  colp->info = DepotDB_UpdateEntry(colp->info, &thisentry);
  DepotDB_FreeSourceList(&thisentry);

  if ((conffd = open(confpath, O_RDONLY)) > 0)
    {	/* there exists a depot.conf file for this collection */
      dbgprint(F_CONFREAD, (stderr, "opened file %s\n", confpath));
      /* read info from config file */
      eoconf = FALSE; confpos = 0; lineno = 1;
      while (!eoconf)
	{
	  if ( (ch = fdgetc(conffd, confpos++)) == EOF)
	    {
	      eoconf = TRUE;
	      dbgprint(F_CONFREAD, (stderr, "ENDOF DATABASE FILE\n"));
	    }
	  else
	    {
	      /* skip whitespaces */
	      while ((ch == ' ') || (ch == '\t'))
		ch = fdgetc(conffd, confpos++);
	      if (ch == EOF)
		{
		  eoconf = TRUE;
		  dbgprint(F_CONFREAD, (stderr, "ENDOF DATABASE FILE\n"));
		}
	      else if (ch == '#')
		{	/* this is a comment line */
		  /* skip line */
		  while ((ch != '\n') && (ch != EOF))
		    ch = fdgetc(conffd, confpos++);
		  if (ch == EOF)
		    {
		      eoconf = TRUE;
		      dbgprint(F_CONFREAD, (stderr, "ENDOF DATABASE FILE\n"));
		    }
		  else lineno++;
		  continue;
		}
	      else if (ch == '\n')
		{ lineno++; continue; }
	      else if (ch == '~')
		{	/* this is a tilde-escape specification */
		  /* get the specification */
		  ch = fdgetc(conffd, confpos++);
		  switch (ch)
		    {
		    case 'a':
		    case 'A':
#ifdef USE_FSINFO
		      UpdateType = CU_AFSMOUNTPOINT;
#else /* USE_FSINFO */
		      WarningError((stderr,"Ignoring ~afsmountpoint in %s since USE_FSINFO is not defined\n", colp->name));
		      while ((ch != '\n') && (ch != EOF))
			ch = fdgetc(conffd, confpos++);
		      if (ch == EOF)
			{
			  eoconf = TRUE;
			  dbgprint(F_CONFREAD, (stderr, "ENDOF DATABASE FILE\n"));
			}
		      else lineno++;
		      continue;
#endif /* USE_FSINFO */
 
		      break;
		    case 'c':
		    case 'C':
		      UpdateType = CU_RCFILE;
		      break;
		    case 'd':
		    case 'D':
		      UpdateType = CU_DELETE;
		      break;
		    default:
		      FatalError(E_BADCOMMAND, (stderr, "Unknown tilde command in conf file %s, line %u\n", confpath, lineno));
		    }
		  /* skip until e.o.word */
		  eoword = FALSE;
		  while (!eoword && !eoconf)
		    {
		      if ((ch == ' ') || (ch == '\t')) eoword = TRUE;
		      else if (ch == EOF)
			{
			  eoconf = TRUE;
			  dbgprint(F_CONFREAD, (stderr, "ENDOF DATABASE FILE\n"));
			}
		      else ch = fdgetc(conffd, confpos++);
		    }
		}
	      else
		{
		  UpdateType = CU_MAP;
		  /* read in file/dir name into path1 */
		  cp = path1; eoword = FALSE; QuotedChar = FALSE;
		  while (!eoword && !eoconf)
		    {
		      if (((ch == ' ') || (ch == '\t') || (ch == '\n')) && !QuotedChar)
			{ eoword = TRUE; }
		      else if (ch == EOF)
			{
			  eoconf = TRUE;
			  dbgprint(F_CONFREAD, (stderr, "ENDOF DATABASE FILE\n"));
			}
		      else
			{
			  if (!QuotedChar && (ch == DepotDB_QUOTCHAR))
			    { QuotedChar = TRUE; }
			  else
			    { QuotedChar = FALSE; *cp++ = (char)ch; }
			  ch = fdgetc(conffd, confpos++);
			}
		    }
		  *cp = '\0';
		}

	      /* skip whitespaces */
	      while ((ch == ' ') || (ch == '\t'))
		ch = fdgetc(conffd, confpos++);

	      /* read in file/dir name, if any, into path2 */
	      cp = path2; eoword = FALSE; QuotedChar = FALSE;
	      while (!eoword && !eoconf)
		{
		  if (((ch == ' ') || (ch == '\t') || (ch == '\n')) && !QuotedChar)
		    eoword = TRUE;
		  else if (ch == EOF)
		    {
		      eoconf = TRUE;
		      dbgprint(F_CONFREAD, (stderr, "ENDOF DATABASE FILE\n"));
		    }
		  else
		    {
		      if (!QuotedChar && (ch == DepotDB_QUOTCHAR))
			{ QuotedChar = TRUE; }
		      else
			{ QuotedChar = FALSE; *cp++ = (char)ch; }
		      ch = fdgetc(conffd, confpos++);
		    }
		}
	      *cp = '\0';

	      /* get specification line by line */
	      switch (UpdateType)
		{
#ifdef USE_FSINFO
		case CU_AFSMOUNTPOINT:
		  if (path2[0] == '\0') /* no mountpoints listed!! */
		    {
		      FatalError(E_BADCONFIGFILE, (stderr, "Syntax error while reading config file %s, line %u", confpath, lineno));
		    }
		  else
		    {
		      /* add all mountpoints to collectiondb */
		      eoline = FALSE;
		      while (!eoline && !eoconf)
			{
			  /* add mountpoint to collection db */
			  dbgprint(F_CONFREAD, (stderr, "Adding AFS mountpoint %s to collection database\n", path2));
			  fsinfo = Collection_AddAFSMountPoint(colp, path2);
			  if (fsinfo == NULL)
			    {FatalError(E_BADMTPT, (stderr, "Bad mountpoint %s specified in config file %s, line %u\n", path2, confpath, lineno));}
			  /* skip whitespaces, if any */
			  while ((ch == ' ') || (ch == '\t'))
			    ch = fdgetc(conffd, confpos++);
			  /* unless e.o.line, read new mountpoint */
			  if (ch == '\n')
			    eoline = TRUE;
			  else if (ch == EOF)
			    {
			      eoconf = TRUE;
			      dbgprint(F_CONFREAD, (stderr, "ENDOF DATABASE FILE\n"));
			    }
			  else
			    {
			      cp = path2; eoword = FALSE; QuotedChar = FALSE;
			      while (!eoword && !eoconf)
				{
				  if (((ch == ' ') || (ch == '\t') || (ch == '\n')) && !QuotedChar)
				    eoword = TRUE;
				  else if (ch == EOF)
				    { eoword = TRUE; eoconf = TRUE; }
				  else
				    {
				      if (!QuotedChar && (ch == DepotDB_QUOTCHAR))
					{ QuotedChar = TRUE; }
				      else
					{ QuotedChar = FALSE; *cp++ = (char)ch; }
				      ch = fdgetc(conffd, confpos++);
				    }
				}
			      *cp = '\0';
			    }
			}
		    }
		  break;
#endif /* USE_FSINFO  */
		case CU_MAP:
		  /* skip any spaces to e.o.line */
		  eoline = FALSE;
		  while (!eoline && !eoconf)
		    {
		      if (ch == '\n')
			{eoline = TRUE; lineno++; }
		      else if (ch == EOF)
			{
			  eoconf = TRUE;
			  dbgprint(F_CONFREAD, (stderr, "ENDOF DATABASE FILE\n"));
			}
		      else if ((ch != ' ') || (ch != '\t'))
			{
			  FatalError(E_BADCONFIGFILE, (stderr, "Syntax error while reading config file %s, line %u", confpath, lineno));
			}
		      else ch = fdgetc(conffd, confpos++);
		    }
		  if (*path2 == '\0')
		    /*  path2 is the same as path1 */
		    (void)strcpy(path2, path1);
		  /*
		   * if path2 is a path with directories in it, add those directories
		   * as non-virgin entries to the collection's database
		   */
		  if (strcmp(path2, "/") != 0)
		    {
		      if (strcmp(path1, "/") == 0) (void)strcpy(src, colp->path);
		      else (void)sprintf(src, "%s/%s", colp->path, path1);
		      (void)strcpy(trg, path2);

		      Collection_MakeNonVirginDirectoriesToPath(colp, trg, src);
		    }
		  /*
		   * read in subtree rooted at path1
		   * as the source for subtree rooted at path2;
		   */
		  if (strcmp(path1, "/") == 0) (void)strcpy(src, colp->path);
		  else (void)sprintf(src, "%s/%s", colp->path, path1);
		  (void)strcpy(trg, path2);
		  dbgprint(F_CONFREAD, (stderr, "Mapping %s onto %s for collection %s\n", src, trg, colp->name));
		  Collection_Map(colp, src, trg, KnownDB, TRUE);
		  break;
		case CU_DELETE:
		  /* skip any spaces to e.o.line */
		  eoline = FALSE;
		  while (!eoline && !eoconf)
		    {
		      if (ch == '\n')
			{eoline = TRUE; lineno++;}
		      else if (ch == EOF)
			{
			  eoconf = TRUE;
			  dbgprint(F_CONFREAD, (stderr, "ENDOF DATABASE FILE\n"));
			}
		      else if ((ch != ' ') || (ch != '\t'))
			{
			  FatalError(E_BADCONFIGFILE, (stderr, "Syntax error while reading config file %s, line %u", confpath, lineno));
			}
		      else ch = fdgetc(conffd, confpos++);
		    }
		  /* delete subdir path2 from database */
		  (void)strcpy(trg, path2);
		  dbgprint(F_CONFREAD, (stderr, "Deleting %s from collection %s\n", trg, colp->name));
#ifdef USE_FSINFO
		  colp->info = DepotDB_DeletePath(colp->info, trg, DB_LAX);
#else /* USE_FSINFO */
		  colp->info = DepotDB_DeletePath(colp->info, trg, 0);
#endif /* USE_FSINFO */
		  break;
		case CU_RCFILE:
		  /* read in list of affected files into path1 and add entries to database */
		  (void)strcpy(src, path2);
		  dbgprint(F_CONFREAD, (stderr, "Adding entries for executable %s in collection %s which affects the following files:\n", src, colp->name));
		  eoline = FALSE;
		  while (!eoline && !eoconf)
		    {
		      /* skip whitespaces */
		      while ((ch == ' ') || (ch == '\t'))
			ch = fdgetc(conffd, confpos++);
		      if (ch == '\n')
			{eoword = TRUE; eoline = TRUE; lineno++; }
		      else if (ch == EOF)
			{ eoline = TRUE; }
		      else
			{
			  cp = path1; eoword = FALSE; QuotedChar = FALSE;
			  while (!eoword && !eoconf)
			    {
			      if (((ch == ' ') || (ch == '\t') || (ch == '\n')) && !QuotedChar)
				eoword = TRUE;
			      else if (ch == EOF)
				{
				  eoword = TRUE; eoconf = TRUE;
				  dbgprint(F_CONFREAD, (stderr, "ENDOF DATABASE FILE\n"));
				}
			      else
				{
				  if (!QuotedChar && (ch == DepotDB_QUOTCHAR))
				    { QuotedChar = TRUE; }
				  else
				    { QuotedChar = FALSE; *cp++ = (char)ch; }
				  ch = fdgetc(conffd, confpos++);
				}
			    }
			  *cp = '\0';
			  (void)strcpy(trg, path1);
			  dbgprint(F_CONFREAD, (stderr, "\t%s\n", trg));
			  Collection_MakeNonVirginDirectoriesToPath(colp, trg, src);
			  Collection_AddCommand(colp, src, trg, KnownDB);
			}
		    }
		  /* eoline is TRUE */
		  break;
		}
	    }
	}
      /* eoconf is TRUE */
    }
  else if (errno != ENOENT)
    {
      FatalError(E_OPENFAILED, (stderr, "Could not open %s\n", confpath));
    }
  else
    {
      dbgprint(F_CONFREAD, (stderr, "no config file %s\n", confpath));
      /* map entire collection */
      (void)strcpy(src, colp->path); (void)strcpy(trg, "/");
      dbgprint(F_CONFREAD, (stderr, "Mapping %s onto %s for collection %s\n", src, trg, colp->name));
      Collection_Map(colp, src, trg, KnownDB, TRUE);
    }

  DepotDB_SetTargetMappings(colp->info);

  /* if root entry has dummy source added in the beginning still obsolete, "unobsolete" it */
  i = 0; sp = colp->info->sourcelist;
  while ((i < colp->info->nsources) && (strcmp(sp->name, colp->path) != 0))
    { i++; sp++; }
  if (i < colp->info->nsources) /* (strcmp(sp->name, colp->path) == 0) */
    sp->status &= ~S_OBSOLETE;

  dbgCollectionDB_Write(colp->info, colp->name);

  dbgprint(F_TRACE, (stderr, "Collection_Read done\n"));
  return TRUE;
}


/*
 * static void Collection_Map(colp, source, target, KnownDB, toplevelmapping)
 *	adds entries in the collection database colp->info for
 *	files in the subtree represented by target using
 *	files from collection colp->name under the subtree
 *	represented by source as their source
 */
static void Collection_Map(colp, source, target, KnownDB, toplevelmapping)
     COLLECTION *colp;
     char *source, *target;
     DEPOTDB *KnownDB;
     Boolean toplevelmapping; /* TRUE iff mapping at the top level */
{
  register struct direct *de;
  register DIR *dp;
  struct stat stb;
  ENTRY thisentry;
  SOURCE thissrc;
  char newsource[MAXPATHLEN], newtarget[MAXPATHLEN], sourcepath[MAXPATHLEN];
  char configfile[MAXPATHLEN];
#ifdef USE_FSINFO
  ENTRY *KnownDB_ep;
  SOURCE *KnownDB_sp;
  COLLECTIONFSINFO *fsinfo;
  Boolean UptoDateFSInfoKnown;
#endif /* USE_FSINFO */

  dbgprint(F_TRACE, (stderr, "Collection_Map\n"));

  (void)sprintf(configfile, "%s/%s", colp->path, COLLECTIONCONFIGFILE);

  /* set up thisentry */
  thisentry.name = target; thisentry.status = 0;
  thisentry.nsources = 0; thisentry.sourcelist = NULL;

  if (source[0] == '/') /* absolute path */
    (void)strcpy(sourcepath, source);
  else /* relative path */
    (void)sprintf(sourcepath, "%s/%s", Depot_TargetPath, source);

  if (lstat(sourcepath, &stb) < 0)
    {
      FatalError(E_LSTATFAILED, (stderr, "Could not lstat %s: %s\n", sourcepath, strerror(errno)));
    }
  else
    {
      /* if link, follow it if and only if mapping at the root level -- KLUDGEY */
      if ((stb.st_mode & S_IFMT) == S_IFLNK)
	{
	  if ((strcmp(target, "/") == 0) && (stat(sourcepath, &stb) < 0))
	    { FatalError(E_STATFAILED, (stderr, "Could not stat %s: %s\n", sourcepath, strerror(errno))); }
	}
      if ((stb.st_mode & S_IFMT) == S_IFDIR)
	{
	  /* source is a dir, map the dir followed by each of its components */
	  thissrc.name = source; thissrc.collection_name = colp->name;
	  thissrc.update_spec = U_MKDIR; thissrc.old_update_spec = (u_short)0;
	  thissrc.status = 0;
	  if (strcmp(colp->name, "") == 0)
	    /* an unreferenced entry noop-style entry */
	    thissrc.status |= S_NONVIRGINSRC;
#ifdef USE_FSINFO
	  if (((fsinfo = Collection_FSMountPointInfo(colp, source)) != NULL)
	      || (toplevelmapping && ((fsinfo = Collection_FileSystemInfo(colp, source)) != NULL)))
	    /* source represents a filesystem mountpoint */
	    /* or a top-level mapping under a filesystem mountpoint */
	    {
	      /* add new file system info to thissrc */
	      thissrc.fs_status = FS_NEWFILESYSTEM;
	      thissrc.fs_id = fsinfo->fs_id;
	      thissrc.fs_modtime = fsinfo->fs_modtime;
	      if (colp->confinfo != NULL)
		{
		  thissrc.col_id = colp->confinfo->fs_id;
		  thissrc.col_conftime = colp->confinfo->fs_modtime;
		}
	      else
		{ thissrc.col_id = -1; }
	    }
	  else
	    thissrc.fs_status = FS_INHERIT;

	  UptoDateFSInfoKnown = FALSE;
	  if ((KnownDB != NULL) && (thissrc.fs_status & FS_NEWFILESYSTEM))
	    /* source represents a filesystem mountpoint */
	    /* or a top-level mapping under a filesystem mountpoint */
	    {
	      /*
	       * locate KnownDB_sp, the source corresponding to
	       * source from the appropriate entry in the known database
	       */
	      KnownDB_ep = DepotDB_LocateEntryByName(KnownDB, target, DB_LOCATE|DB_LAX);
	      if (KnownDB_ep == NULL) KnownDB_sp = NULL;
	      else KnownDB_sp = DepotDB_SourceList_LocateCollectionSourceByName(KnownDB_ep, source, colp->name, DB_LOCATE);
     	      if ( (KnownDB_sp != NULL)
		  && (KnownDB_sp->fs_status & FS_NEWFILESYSTEM)
		  && (!(KnownDB_sp->status & S_NONVIRGINSRC)
		      || (strcmp(source, colp->path) == 0)
		      || ((colp->confinfo != NULL) && (KnownDB_sp->col_id > 0)
			  && (colp->confinfo->fs_id == KnownDB_sp->col_id)
			  && (colp->confinfo->fs_modtime == KnownDB_sp->col_conftime)))
		  && (KnownDB_sp->fs_id == thissrc.fs_id )
		  && (KnownDB_sp->fs_modtime == thissrc.fs_modtime)
		  )
		/* we have new file system info in KnownDB_sp and we have same virgin info */
		{
		  /*
		   * Extract the subtree for the file system from the
		   * known database tree onto the collection database tree
		   */
		  dbgprint(F_COLLMAP, (stderr, "Extracting info for file system id %d for %s -> %s of collection %s from known database\n", thissrc.fs_id, source, target, colp->name));
		  Collection_ExtractFileSystemSubtree(colp, source, target, KnownDB_ep, KnownDB, thissrc.fs_id, thissrc.fs_modtime, toplevelmapping);
		  UptoDateFSInfoKnown = TRUE;
		}
	    }
	  if (!UptoDateFSInfoKnown)
	    colp->fsuptodate = FALSE;
	  if (!UptoDateFSInfoKnown) /* we cannot extract info from the database */
#endif /* USE_FSINFO */
	    {
	      /* map source directory and its components */

	      DepotDB_SourceList_AddSource(&thisentry, &thissrc);
	      colp->info = DepotDB_UpdateEntry(colp->info, &thisentry);
	      DepotDB_FreeSourceList(&thisentry);
	      dbgprint(F_COLLMAP, (stderr, "MAP dir %s -> %s for collection %s\n", source, target, colp->name));

	      /* map components of the source directory */
	      if ((dp = opendir(sourcepath)) == NULL)
		{
		  FatalError(E_OPENDIRFAILED, (stderr, "Could not open directory %s\n", source));
		}
	      
	      /* if nlinks to this dir == 2 (no non-AFS subdirs below this level)
	       *   then map this level only;
	       *   else recursively map;
	       * in an AFS environment, we use the mountpoint info
	       * to determine whether any mountpoint exists under this directory
	       */
	      if ((stb.st_nlink == 2) /* no non-AFS subdirs under this level */
		  && (Collection_FSMountPointFreeDir(colp, source) == TRUE)) /* no mountpoints under this directory */
		{
		  dbgprint(F_COLLMAP, (stderr, "mapping files in %s -> %s for collection %s as leaves\n", source, target, colp->name));
		  while ((de = readdir(dp)) != NULL)
		    {
		      if ( (strcmp(de->d_name, ".") != 0) && (strcmp(de->d_name, "..") != 0))
			{
			  if (strcmp(source, ".") == 0)
			    (void)strcpy(newsource, de->d_name);
			  else
			    (void)sprintf(newsource, "%s/%s", source, de->d_name);
			  if (strcmp(target, "/") == 0)
			    (void)strcpy(newtarget, de->d_name);
			  else
			    (void)sprintf(newtarget, "%s/%s", target, de->d_name);
			  /* map if not the depot.conf file */
			  if (strcmp(configfile, newsource) != 0)
			    {
			      thisentry.name = newtarget; thisentry.status = 0;
			      thisentry.nsources = 0; thisentry.sourcelist = NULL;
			      thissrc.name = newsource; thissrc.collection_name = colp->name; thissrc.status = 0;
			      if (*colp->name == '\0')
				/* a noop entry */
				{
				  thissrc.update_spec = U_NOOP;
				  thissrc.old_update_spec = (u_short)0;
				  thissrc.status |= S_NONVIRGINSRC;
				}
			      else
				/* map as link */
				{
				  thissrc.update_spec = U_MAP;
				  thissrc.old_update_spec = (u_short)0;
				}
			      DepotDB_SourceList_AddSource(&thisentry, &thissrc);
			      colp->info = DepotDB_UpdateEntry(colp->info, &thisentry);
			      DepotDB_FreeSourceList(&thisentry);
			      dbgprint(F_COLLMAP, (stderr, "MAP link %s -> %s for collection %s\n", newsource, newtarget, colp->name));
			    }
			}
		    }
		}
	      else /* stb.st_nlink != 2, subdirs exist under this level */
		{
		  while ((de = readdir(dp)) != NULL)
		    {
		      if ( (strcmp(de->d_name, ".") != 0) && (strcmp(de->d_name, "..") != 0))
			{
			  if (strcmp(source, ".") == 0)
			    (void)strcpy(newsource, de->d_name);
			  else
			    (void)sprintf(newsource, "%s/%s", source, de->d_name);
			  if (strcmp(target, "/") == 0)
			    (void)strcpy(newtarget, de->d_name);
			  else
			    (void)sprintf(newtarget, "%s/%s", target, de->d_name);
			  /* map if not the depot.conf file */
			  if (strcmp(configfile, newsource) != 0)
			    {
			      Collection_Map(colp, newsource, newtarget, KnownDB, FALSE);
			    }
			}
		    }
		}
	      (void)closedir(dp);
	    }
	}
      else if (strcmp(configfile, source) != 0)
	{
	  /* map source to target */
	  thissrc.name = source; thissrc.collection_name = colp->name; thissrc.status = 0;
	  if (*colp->name == '\0')
	    /* sourcepath and target are the same, a noop entry */
	    {thissrc.update_spec = U_NOOP; thissrc.old_update_spec = (u_short)0; thissrc.status |= S_NONVIRGINSRC;}
	  else
	    /* map as link */
	    { thissrc.update_spec = U_MAP; thissrc.old_update_spec = (u_short)0; }
	  DepotDB_SourceList_AddSource(&thisentry, &thissrc);
	  colp->info = DepotDB_UpdateEntry(colp->info, &thisentry);
	  DepotDB_FreeSourceList(&thisentry);
	  dbgprint(F_COLLMAP, (stderr, "MAP link %s -> %s for collection %s\n", source, target, colp->name));
	}
    }

  dbgprint(F_TRACE, (stderr, "Collection_Map done\n"));
  return;
}



/*
 * static void Collection_AddCommand(colp, source, target, KnownDB)
 *	adds entries in the collection database colp->info for
 *	the execution of source to yield target
 */
static void Collection_AddCommand(colp, source, target, KnownDB)
     COLLECTION *colp;
     char *source, *target;
     DEPOTDB *KnownDB;
{
  register int i;
  register SOURCE *sp;
  ENTRY *ep;
  Boolean LocatedCommand;
  ENTRY thisentry; SOURCE thissrc;

  dbgprint(F_TRACE, (stderr, "Collection_AddCommand\n"));

  thisentry.sibling = NULL; thisentry.child = NULL;
  thisentry.nsources = 0; thisentry.sourcelist = NULL;
  thisentry.name = target; thisentry.status = 0;
  thissrc.update_spec = U_RCFILE; thissrc.old_update_spec = (u_short)0;
  thissrc.status = 0;
  thissrc.name = source; thissrc.collection_name = colp->name;

  ep = DepotDB_LocateEntryByName(KnownDB, target, DB_LOCATE|DB_LAX);
  if (ep != NULL)
    {
      LocatedCommand = FALSE; i = 0; sp = ep->sourcelist;
      while ( !LocatedCommand && (i < ep->nsources))
	{
	  if (   (sp->update_spec & U_RCFILE)
	      && (strcmp(sp->name, source) == 0)
	      && (strcmp(sp->collection_name, colp->name) == 0))
	    {
	      LocatedCommand = TRUE;
	      thissrc.status |= (sp->status & S_TARGET);
	    }
	  else
	    { i++; sp++; }
	}
    }
  DepotDB_SourceList_AddSource(&thisentry, &thissrc);
  colp->info = DepotDB_UpdateEntry(colp->info, &thisentry);
  DepotDB_FreeSourceList(&thisentry);
  DepotDB_SetNonVirginPath(colp->info, target, colp->name);

  dbgprint(F_TRACE, (stderr, "Collection_AddCommand done\n"));
  return;
}


static Boolean Collection_FSMountPointFreeDir(colp, dirpath)
     COLLECTION *colp;
     char *dirpath;
{
#ifdef USE_FSINFO
  register COLLECTIONFSINFO **fsp;
  Boolean LocatedFSMountPoint;

  dbgprint(F_TRACE, (stderr, "Collection_FSMountPointFreeDir\n"));
  if (colp->fslist == NULL)
    LocatedFSMountPoint = FALSE;
  else
    {
      fsp = colp->fslist; LocatedFSMountPoint = FALSE;
      while ( !LocatedFSMountPoint && (*fsp != NULL))
	{
	  if (strncmp((*fsp)->path, dirpath, strlen(dirpath)) == 0)
	    LocatedFSMountPoint = TRUE;
	  else
	    fsp++;
	}
    }

  dbgprint(F_TRACE, (stderr, "Collection_FSMountPointFreeDir done\n"));
  if (LocatedFSMountPoint) return FALSE;
  else return TRUE;
#else /* USE_FSINFO */
  return TRUE;
#endif /* USE_FSINFO */
}




#ifdef USE_FSINFO


/*
 * static void Collection_ExtractFileSystemSubtree(colp, source, target, KnownDBentryp, KnownDB, fsid, fsmodtime, toplevelmapping)
 *	extracts the subtree from the subtree of the known database KnownDBEntryp
 *	and adds it to the collection database in colp->info.
 */
static void Collection_ExtractFileSystemSubtree(colp, source, target, KnownDBEntryp, KnownDB, fsid, fsmodtime, toplevelmapping)
     COLLECTION *colp;
     char *source, *target;
     ENTRY *KnownDBEntryp;
     DEPOTDB *KnownDB;
     long fsid;
     u_long fsmodtime;
     Boolean toplevelmapping;
{
  register ENTRY *ep;
  ENTRY thisentry;
  SOURCE thissrc;
  SOURCE *KnownDBSourcep;
  char newsource[MAXPATHLEN], newtarget[MAXPATHLEN];
  COLLECTIONFSINFO *fsinfo;
  long newfsid;
  u_long newfsmodtime;

  dbgprint(F_TRACE, (stderr, "Collection_ExtractFileSystemSubtree\n"));

  /* locate the source of KnownDBEntryp corresponding to the source */
  KnownDBSourcep = DepotDB_SourceList_LocateCollectionSourceByName(KnownDBEntryp, source, colp->name, DB_LOCATE|DB_LAX);

  if (KnownDBSourcep == NULL)
    {
      /* return with no additions to colp->info */
      dbgprint(F_TRACE, (stderr, "Collection_ExtractFileSystemSubtree done\n"));
      return;
    }
  else if ((KnownDBSourcep->update_spec & U_MKDIR)
	   && (KnownDBSourcep->fs_status & FS_NEWFILESYSTEM)
	   && (toplevelmapping || (KnownDBSourcep->fs_id != fsid) || (KnownDBSourcep->fs_modtime != fsmodtime)))
    {
      if (((fsinfo = Collection_FSMountPointInfo(colp, source)) != NULL)
	  || (toplevelmapping && ((fsinfo = Collection_FileSystemInfo(colp, source)) != NULL)))
	/* we have the latest info on the new filesystem */
	{
	  if ( (!(KnownDBSourcep->status & S_NONVIRGINSRC)
		|| (strcmp(source, colp->path) == 0)
		|| ((colp->confinfo != NULL) && (KnownDBSourcep->col_id > 0)
		    && (colp->confinfo->fs_id == KnownDBSourcep->col_id)
		    && (colp->confinfo->fs_modtime == KnownDBSourcep->col_conftime)))
	      && (KnownDBSourcep->fs_id == fsinfo->fs_id)
	      && (KnownDBSourcep->fs_modtime == fsinfo->fs_modtime))
	    /* this info indicates info extractable from known database */
	    {
	      newfsid = KnownDBSourcep->fs_id;
	      newfsmodtime = KnownDBSourcep->fs_modtime; 
	    }
	  else
	    {
	      Collection_Map(colp, source, target, KnownDB, FALSE);
	      dbgprint(F_TRACE, (stderr, "Collection_ExtractFileSystemSubtree done\n"));
	      return;
	    }
	}
      else
	{
	  /* something is wrong, previously existing mountpoint under unchanged filesystem not listed */
	  FatalError(E_UNLISTEDMTPT, (stderr, "Mountpoint or filesystem info for filesystem id %d for source %s of collection %s under unchanged filesystem id %d not listed in config file for collection\n", KnownDBSourcep->fs_id, source, colp->name, fsid));
	}
    }
  else
    { newfsid = fsid; newfsmodtime = fsmodtime; }
  
  /*
   * if we get here we either have
   *	( non-directory || the filesystem info is inherited)
   *	or updated fs usable info for new filesystem
   */
  thisentry.name = target; thisentry.status = 0;
  thisentry.nsources = 0; thisentry.sourcelist = NULL;
  thissrc.name = KnownDBSourcep->name;
  thissrc.collection_name = KnownDBSourcep->collection_name;
  thissrc.update_spec = KnownDBSourcep->update_spec;
  thissrc.old_update_spec = KnownDBSourcep->old_update_spec;
  thissrc.status =   ((KnownDBSourcep->status & S_NONVIRGINSRC)
		      | (KnownDBSourcep->status & S_ANTIQUE)
		      | (KnownDBSourcep->status & S_TARGET));
  if ((thissrc.update_spec & U_MKDIR)
      && ((thissrc.fs_status = KnownDBSourcep->fs_status) & FS_NEWFILESYSTEM))
    {
      thissrc.fs_id = KnownDBSourcep->fs_id;
      thissrc.fs_modtime = KnownDBSourcep->fs_modtime;
      if (colp->confinfo != NULL)
	{
	  thissrc.col_id = colp->confinfo->fs_id;
	  thissrc.col_conftime = colp->confinfo->fs_modtime;
	}
      else
	{ thissrc.col_id = -1; }
    }
  DepotDB_SourceList_AddSource(&thisentry, &thissrc);
  colp->info = DepotDB_UpdateEntry(colp->info, &thisentry);
  DepotDB_FreeSourceList(&thisentry);
  dbgprint(F_COLLMAP, (stderr, "MAP %s -> %s for collection from known database %s\n", source, target, colp->name));

  for (ep = ENTRY_child(KnownDBEntryp); ep!= NULL; ep = ENTRY_sibling(ep))
    {
      if (strcmp(source, ".") == 0)
	(void)strcpy(newsource, ep->name);
      else {
	strcpy(newsource,source);
	strcat(newsource,"/");
	strcat(newsource,ep->name);
      }
      if (strcmp(target, "/") == 0)
	(void)strcpy(newtarget, ep->name);
      else {
	strcpy(newtarget, target);
	strcat(newtarget, "/");
	strcat(newtarget, ep->name);
      }
      Collection_ExtractFileSystemSubtree(colp, newsource, newtarget, ep, KnownDB, newfsid, newfsmodtime, FALSE);
      /* also search for non-virgin paths which may have been created artificially to a subdir */
      Collection_ExtractFileSystemSubtree(colp, source, newtarget, ep, KnownDB, newfsid, newfsmodtime, FALSE);
    }

  dbgprint(F_TRACE, (stderr, "Collection_ExtractFileSystemSubtree done\n"));
  return;
}


/*
 * static COLLECTIONFSINFO *Collection_AddAFSMountPoint(colp, fspath)
 */
static COLLECTIONFSINFO *Collection_AddAFSMountPoint(colp, fspath)
     COLLECTION *colp;
     char *fspath;
{
  register COLLECTIONFSINFO **fsp;
  COLLECTIONFSINFO fsinfo;
  unsigned nfsinfo;
  char fspathfromcwd[MAXPATHLEN];

  dbgprint(F_TRACE, (stderr, "Collection_AddAFSMountPoint\n"));

  if (strcmp(fspath, ".") == 0)
    {
      if (colp->path[0] == '/')
	(void)strcpy(fspathfromcwd, colp->path);
      else
	(void)sprintf(fspathfromcwd, "%s/%s", Depot_TargetPath, colp->path);
      fsinfo.path = fspathfromcwd;
      AFS_GetVolumeInfo(&fsinfo);
    }
  else
    {
      if (colp->path[0] == '/')
	(void)sprintf(fspathfromcwd, "%s/%s", colp->path, fspath);
      else
	(void)sprintf(fspathfromcwd, "%s/%s/%s", Depot_TargetPath, colp->path, fspath);
      fsinfo.path = fspathfromcwd;
      AFS_GetMountPointInfo(&fsinfo);
    }

  if (fsinfo.fs_id == -1)
    {
      /* not an AFS mountpoint or no info found */
      dbgprint(F_TRACE, (stderr, "Collection_AddAFSMountPoint done\n"));
      return NULL;
    }
  else
    {
      if (colp->fslist == NULL)
	{
	  colp->fslist = (COLLECTIONFSINFO **)emalloc(sizeof(COLLECTIONFSINFO *));
	  *(colp->fslist) = NULL;
	  nfsinfo = 1;
	}
      else
	{
	  nfsinfo = 1; fsp = colp->fslist;
	  while (*fsp != NULL)
	    { nfsinfo++; fsp++; }
	}
      colp->fslist = (COLLECTIONFSINFO **)erealloc((char *)(colp->fslist), (nfsinfo+1)*sizeof(COLLECTIONFSINFO *));
      fsp = colp->fslist+nfsinfo; *fsp-- = NULL;
      *fsp = (COLLECTIONFSINFO *)emalloc(sizeof(COLLECTIONFSINFO));
      if (strcmp(fspath, ".") == 0)
	{
	  (*fsp)->path = (char *)emalloc(strlen(colp->path)+1);
	  (void)strcpy((*fsp)->path, colp->path);
	}
      else
	{
	  (*fsp)->path = (char *)emalloc(strlen(colp->path)+strlen(fspath)+2);
	  (void)sprintf((*fsp)->path, "%s/%s", colp->path, fspath);
	}
      (*fsp)->fs_id = fsinfo.fs_id;
      (*fsp)->fs_modtime = fsinfo.fs_modtime;

      dbgprint(F_TRACE, (stderr, "Collection_AddAFSMountPoint done\n"));
      return *fsp;
    }
}


/*
 * static COLLECTIONFSINFO *Collection_FSMountPointInfo(colp, fspath)
 *	returns filesystem mountpoint info corresponding to fspath
 *	from colp->fslist. Returns NULL if no such info exists.
 */
static COLLECTIONFSINFO *Collection_FSMountPointInfo(colp, fspath)
     COLLECTION *colp;
     char *fspath;
{
  register COLLECTIONFSINFO **fsp;
  COLLECTIONFSINFO *fsinfo;
  Boolean LocatedFSPath;

  dbgprint(F_TRACE, (stderr, "Collection_FSMountPointInfo\n"));
  if (colp->fslist == NULL)
    fsinfo = NULL;
  else
    {
      fsp = colp->fslist; LocatedFSPath = FALSE;
      while ( !LocatedFSPath && (*fsp != NULL))
	{
	  if (strcmp((*fsp)->path, fspath) == 0)
	    LocatedFSPath = TRUE;
	  else
	    fsp++;
	}
      fsinfo = LocatedFSPath ? *fsp : NULL;
    }

  dbgprint(F_TRACE, (stderr, "Collection_FSMountPointInfo done\n"));
  return fsinfo;
}


/*
 * static COLLECTIONFSINFO *Collection_FileSystemInfo(colp, fspath)
 *	returns filesystem info corresponding to fspath
 *	from colp->fslist. Returns NULL if no such info exists.
 */
static COLLECTIONFSINFO *Collection_FileSystemInfo(colp, fspath)
     COLLECTION *colp;
     char *fspath;
{
  register COLLECTIONFSINFO **fsp;
  register int fsplen;
  COLLECTIONFSINFO *fsinfo;
  int fsmtptlen;
  int fspathlen;

  dbgprint(F_TRACE, (stderr, "Collection_FileSystemInfo\n"));
  if (colp->fslist == NULL)
    fsinfo = NULL;
  else
    {
      /*
       * Find the largest mountpoint string with fspath as the prefix.
       * This represents the filesystem in which fspath resides.
       */
      fsp = colp->fslist; fsinfo = NULL; fsmtptlen = 0;
      for ( fsp = colp->fslist; *fsp != NULL; fsp++)
	{
	  fspathlen = (int)strlen((*fsp)->path);
	  if (strncmp((*fsp)->path, fspath, fspathlen) == 0)
	    {
	      fsplen = (int)strlen((*fsp)->path);
	      if (fsplen > fsmtptlen)
		{
		  fsmtptlen = fsplen; fsinfo = *fsp;
		}
	    }
	}
    }

  dbgprint(F_TRACE, (stderr, "Collection_FileSystemInfo done\n"));
  return fsinfo;
}


#endif USE_FSINFO


static void Collection_MakeNonVirginDirectoriesToPath(colp, path, sourcepath)
     COLLECTION *colp;
     char *path;
     char *sourcepath;
{
  register char *cp;
  Boolean eopath, DirFound;
  ENTRY thisentry;
  SOURCE thissrc;

  /* Add top level entry */
  thisentry.sibling = NULL; thisentry.child = NULL;
  thisentry.nsources = 0; thisentry.sourcelist = NULL;
  thisentry.name =  "/";
  thisentry.status = 0;
  thissrc.update_spec = U_MKDIR; thissrc.old_update_spec = (u_short)0;
  thissrc.status = S_NONVIRGINSRC;
  thissrc.name = sourcepath; thissrc.collection_name = colp->name;
#ifdef USE_FSINFO
  thissrc.fs_status = FS_UNKNOWN;
#endif USE_FSINFO
  DepotDB_SourceList_AddSource(&thisentry, &thissrc);
  dbgprint(F_CONFREAD, (stderr, "Adding non-virgin path %s to collection database\n", "/"));
  colp->info = DepotDB_UpdateEntry(colp->info, &thisentry);

  /* Add lower level entries */
  cp = path; eopath = FALSE;
  while (*cp == '/') cp++;
  while (!eopath)
    {
      DirFound = FALSE;
      while (!DirFound && !eopath)
	{
	  while ( *cp != '/' && *cp != '\0' ) cp++;
	  if (*cp == '/') DirFound = TRUE;
	  else if (*cp == '\0') eopath = TRUE;
	}
      if (DirFound)
	{
	  /* put an e.o.string at the end of the directory name */
	  *cp = '\0';
	  /* add the directory as non-virgin entry to collection database */
	  thisentry.sibling = NULL; thisentry.child = NULL;
	  thisentry.nsources = 0; thisentry.sourcelist = NULL;
	  thisentry.name = path; thisentry.status = 0;
	  thissrc.update_spec = U_MKDIR; thissrc.old_update_spec = (u_short)0;
	  thissrc.status = S_NONVIRGINSRC;
	  thissrc.name = sourcepath; thissrc.collection_name = colp->name;
#ifdef USE_FSINFO
	  thissrc.fs_status = FS_UNKNOWN;
#endif USE_FSINFO
	  DepotDB_SourceList_AddSource(&thisentry, &thissrc);
	  dbgprint(F_CONFREAD, (stderr, "Adding non-virgin path %s to collection database\n", path));
	  colp->info = DepotDB_UpdateEntry(colp->info, &thisentry);
	  /* back out the '\0' we put in and move forward */
	  *cp = '/'; cp++;
	}
    }
}



/*
 * char *Collection_LocatePath(colname, searchpath)
 */
char *Collection_LocatePath(colname, searchpath)
     char *colname;
     char **searchpath;
{
  register char **fp;
  register struct direct *de;
  register DIR *dp;
  struct stat stb;
  char colpath[MAXPATHLEN], searchdir[MAXPATHLEN];
  int colnamelen, colversion;
  Boolean LocatedCollection;
  int version;
  static char LocatedPath[MAXPATHLEN];
  char *cp;

  dbgprint(F_TRACE, (stderr, "Collection_LocatePath\n"));

  dbgprint(F_COLLSEARCH, (stderr, "Searching for path to collection %s\n", colname));
  if (searchpath == NULL)
    { FatalError(E_NULLSEARCHPATH, (stderr, "NULL searchpath for collection %s\n", colname)); }

  colnamelen = strlen(colname);
  colversion = VersionToUse(colname, preference);
  fp = searchpath; LocatedCollection = FALSE;
  while (!LocatedCollection && (*fp != NULL))
    {
      if (**fp == '/')
	(void)strcpy(searchdir, *fp);
      else
	(void)sprintf(searchdir, "%s/%s", Depot_TargetPath, *fp);
      dbgprint(F_COLLSEARCH, (stderr, "Searching under %s .. ", searchdir));

      if (colversion >= 0)
	{
	  (void)sprintf(colpath, "%s/%s%c%d", searchdir, colname, Depot_VersionDelimiter, colversion);
	  if ((stat(colpath, &stb) == 0) && ((stb.st_mode & S_IFMT) == S_IFDIR))
	    {
	      dbgprint(F_COLLSEARCH, (stderr, "%s%c%d .. ", colname, Depot_VersionDelimiter, colversion));
	      LocatedCollection = TRUE;
	      (void)sprintf(LocatedPath, "%s/%s%c%d", *fp, colname, Depot_VersionDelimiter, colversion);
	    }
	}
      else
	{
	  if ((dp = opendir(searchdir)) != NULL)
	    {
	      while ((de = readdir(dp)) != NULL)
		{
		  if (strncmp(de->d_name, colname, colnamelen) == 0)
		    {
		      (void)sprintf(colpath, "%s/%s", searchdir, de->d_name);
		      if ((stat(colpath, &stb) == 0) && ((stb.st_mode & S_IFMT) == S_IFDIR))
			{
			  if ((de->d_name[colnamelen] == '\0') && (colversion < 0))
			    {
			      dbgprint(F_COLLSEARCH, (stderr, "%s .. ", de->d_name));
			      LocatedCollection = TRUE;
			      (void)sprintf(LocatedPath, "%s/%s", *fp, colname);
			    }
			  else if (de->d_name[colnamelen] == Depot_VersionDelimiter)
			    {
			      dbgprint(F_COLLSEARCH, (stderr, "%s .. ", de->d_name));
			      LocatedCollection = TRUE;
			      cp = de->d_name;
			      version = atoi(cp+colnamelen+1);
			      if (version > colversion)
				{
				  colversion = version;
				  (void)sprintf(LocatedPath, "%s/%s%c%d", *fp, colname, Depot_VersionDelimiter, colversion);
				}
			    }
			}
		    }
		}
	      (void)closedir(dp);
	    }
	}
      if (LocatedCollection)
	{ dbgprint(F_COLLSEARCH, (stderr, " succeeds.\n")); }
      else
	{ dbgprint(F_COLLSEARCH, (stderr, " fails.\n")); }

      fp++;
    }

  if (LocatedCollection)
    { dbgprint(F_COLLSEARCH, (stderr, "Collection %s found in %s\n", colname, LocatedPath)); }
  else
    { dbgprint(F_COLLSEARCH, (stderr, "Collection %s not found in searchpath \n", colname)); }
  dbgprint(F_TRACE, (stderr, "Collection_LocatePath done\n"));
  return (LocatedCollection ? LocatedPath : NULL );
}



/*
 * Boolean Collection_Uptodate(colname)
 */
#ifdef USE_FSINFO
Boolean Collection_Uptodate(colname)
     char *colname;
{
  register COLLECTIONLIST *cp;
  Boolean FoundCollection;
  Boolean Uptodate;

  if (Depot_CollectionList == NULL)
    { Uptodate = FALSE; }
  else
    {
      FoundCollection = FALSE; cp = Depot_CollectionList;
      while (!FoundCollection && (cp != NULL))
	{
	  if (strcmp(colname, cp->collection.name) == 0)
	    FoundCollection = TRUE;
	  else
	    cp = cp->next;
	}
      if (FoundCollection)
	{ Uptodate = cp->collection.fsuptodate; }
      else
	{
	  /*
	   * The collection is not in the given collection list.
	   * If we are in quick update mode,
	   *	we assume the collection is up to date
	   * else
	   *	we assume the collection is out of date
	   */
	  if (Depot_QuickUpdate)
	    Uptodate = TRUE;
	  else
	    Uptodate = FALSE;
	}
    }

  return Uptodate;
}
#else /* USE_FSINFO */
Boolean Collection_Uptodate(colname)
     char *colname;
{
  /* no file system info; we assume the collection could have changed */
  return FALSE;
}
#endif /* USE_FSINFO */


#ifdef USE_FSINFO
static u_long Collection_ConfigTime(db, colp)
     DEPOTDB *db;
     COLLECTION *colp;
{
  register int i;
  register SOURCE *sp;
  Boolean LocatedCollection;
  u_long configtime;

  LocatedCollection = FALSE;
  if (db != NULL)
    {
      i = 0; sp = db->sourcelist;
      while (!LocatedCollection && (i < db->nsources))
	{
	  if ((strcmp(sp->collection_name, colp->name) == 0)
	      && (strcmp(sp->name, colp->path) == 0)
	      && (sp->fs_status & FS_NEWFILESYSTEM))
	    LocatedCollection = TRUE;
	  else
	    { i++; sp++;}
	}
    }
  if (!LocatedCollection) configtime = -1;
  else configtime = sp->col_conftime;

  return configtime;
}
#endif /* USE_FSINFO */



#ifdef DEBUG

static void dbgCollectionDB_Write(db, name)
     DEPOTDB *db;
     char *name;
{
  if (FDEBUG & F_CONFREAD)
    {
      char dbfilename[MAXPATHLEN];
      FILE *dbfile;

      (void)sprintf(dbfilename, "COLLECTIONDB.%s", name);
      dbfile = efopen(dbfilename, "w");
      DepotDB1_Write(dbfile, db);
      fclose(dbfile);
    }
}
#else /* DEBUG */

static void dbgCollectionDB_Write(db, name)
     DEPOTDB *db;
     char *name;
{
}
#endif /* DEBUG */
/* $Source: /afs/andrew.cmu.edu/system/src/local/depot/019/RCS/Collection.c,v $ */


