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


static char rcs_id[]="$Header: /afs/andrew.cmu.edu/system/src/local/depot/019/RCS/check.c,v 4.17 1992/08/14 18:55:03 ww0r Exp $";


/*
 * Author: Sohan C. Ramakrishna Pillai
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/dir.h>
#include <errno.h>
#include <strings.h>

#include "globals.h"
#include "depot.h"
#include "DepotDBStruct.h"
#include "DepotDB.h"
#include "DepotDBCommandStruct.h"
#include "DepotDBCommand.h"
#include "PreferenceStruct.h"
#include "Preference.h"

#ifdef ibm032
extern int errno;
#endif /* ibm032 */

static Boolean Check_CleanDir();
Boolean CheckSourceConsistency();
Boolean Collection_Uptodate();
int strarrcmp();
int GetMapCommand();

Boolean Check_Noop(entryp, path)
     ENTRY *entryp;
     char *path;
{
  Boolean CheckStatus;
  struct stat stb;
  SOURCE *sourcep;
  Boolean NoOpFileExists;

  dbgprint(F_TRACE, (stderr, "Check_Noop\n"));

  CheckStatus = TRUE; /* unless proven otherwise! */
  sourcep = entryp->sourcelist + (entryp->nsources - 1);
  if (Depot_TrustTargetDir && (sourcep->status & S_TARGET))
    {
      dbgprint(F_TRUSTTARGET, (stderr, "NOOP %s - Trustable targetdir!\n", path));
      sourcep->status |= S_TRUSTTARGET;
      CheckStatus = TRUE;
    }
  else
    {
      if (lstat(path,&stb) < 0)
	{
	  if (errno == ENOENT) NoOpFileExists = FALSE;
	  else {FatalError(E_LSTATFAILED, (stderr, "Could not lstat %s: %s\n", path, strerror(errno)));}
	    }
      else
	NoOpFileExists = TRUE;
      if (!NoOpFileExists)
	{
	  WarningError((stderr, "Warning: File %s not found\n", path));
	  ShowActionMessage((stdout, "Warning: NOOP %s - File not found\n", path));
	  CheckStatus = FALSE;
	}
      else
	{
	  ShowActionMessage((stdout, "NOOP %s\n", path));
	}
    }
  dbgprint(F_TRACE, (stderr, "Check_Noop done\n"));
  return CheckStatus;
}


Boolean Check_Copy(entryp, dest, src)
     ENTRY *entryp;
     char *dest, *src;
{
  Boolean CheckStatus;
  struct stat stbsrc, stbdest;
  SOURCE *sourcep;
  Boolean DestFileDeleteable;
  char linksrc[MAXPATHLEN], linkdest[MAXPATHLEN], srcpath[MAXPATHLEN];
  int ccsrc, ccdest;

  dbgprint(F_TRACE, (stderr, "Check_Copy - being implemented\n"));

  CheckStatus = FALSE;

  sourcep = entryp->sourcelist + (entryp->nsources - 1);
  if (Depot_TrustTargetDir && (sourcep->status & S_TARGET)
      && ((sourcep->update_spec & (U_NOOP|U_DELETE)) ==  (sourcep->old_update_spec & (U_NOOP|U_DELETE))) )
    {
      dbgprint(F_TRUSTTARGET, (stderr, "COPY %s %s - Trustable targetdir!\n", src, dest));
      sourcep->status |= S_TRUSTTARGET;
      CheckStatus = TRUE;
    }
  else
    {
      if ( src[0] == '/' ) /* absolute path */
	(void)strcpy(srcpath, src);
      else /* relative path */
	(void)sprintf(srcpath, "%s/%s", Depot_TargetPath, src);

      if (lstat(srcpath, &stbsrc) < 0)
	{
	  FatalError(E_BADSRCFILE, (stderr, "COPY %s %s : Could not lstat %s\n", src, dest, srcpath));
	}

      if (!(sourcep->status & S_IMMACULATE)
	  && (lstat(dest,&stbdest) == 0))
	{ /* not pre-empted by a virgin parent and dest file exists */

	  if ((stbsrc.st_mode & S_IFMT) == S_IFLNK)
	    {
	      if (((stbdest.st_mode & S_IFMT) == S_IFLNK)
		  && ((ccsrc = readlink(srcpath, linksrc, MAXPATHLEN-1)) > 0)
		  && ((ccdest = readlink(dest, linkdest, MAXPATHLEN-1)) > 0)
		  && (ccsrc == ccdest)
		  && (strncmp(linksrc, linkdest, ccsrc) == 0))
		CheckStatus = TRUE;
	    }
	  else if ((stbsrc.st_mode & S_IFMT) == S_IFREG)
	    {
	      if (Depot_UseModTimes
		  && (stbdest.st_mtime == stbsrc.st_mtime)
		  && (stbdest.st_ino != stbsrc.st_ino))
		/* same mod-time, but different files */
		/* - to guard against symlinks on a parent directory */
		/* inode check obviated by earlier check on S_IMMACULATE, */
		/* but since we stat anyway ... replace if/when we */
		/* put in a way to avoid statting here */
		CheckStatus = TRUE;
	    }
	  if (!CheckStatus)
	    {
	      DestFileDeleteable = ((Depot_DeleteUnReferenced == TRUE) || DepotDB_AntiqueEntry(entryp));
	      if (!DestFileDeleteable)
		{
		  FatalError(E_BADDESTFILE, (stderr, "COPY %s %s : %s not writable\n", src, dest, dest));
		}
	      else
		{
		  ShowActionMessage((stdout, "REMOVE %s\n", dest));
		}
	    }
	}
    }

  if (!CheckStatus)
    {
      entryp->status |= S_UPDATE;
      ShowActionMessage((stdout, "COPY %s %s\n", src, dest));
    }
  else
    {
      dbgprint(F_APPLYCHECK, (stderr, "COPY %s %s is OK\n", src, dest));
    }

  dbgprint(F_TRACE, (stderr, "Check_Copy done\n"));
  return CheckStatus;
}


/*
 * Boolean Check_Link(entryp, dest, src, depth)
 *	returns TRUE iff dest is already a link to src
 * depth indicates the depth of the dest from the top of the destination tree.
 */
Boolean Check_Link(entryp, dest, src, depth)
     ENTRY *entryp;
     char *dest, *src;
     unsigned depth;
{
  Boolean CheckStatus;
  register unsigned i;
  register char *cp;
  SOURCE *sourcep;
  Boolean DestFileExists, DestFileDeleteable;
  char linksrc[MAXPATHLEN], srcpath[MAXPATHLEN];
  int cc;

  dbgprint(F_TRACE, (stderr, "Check_Link\n"));

  CheckStatus = FALSE;

  sourcep = entryp->sourcelist + (entryp->nsources - 1);
  if (Depot_TrustTargetDir && (sourcep->status & S_TARGET)
      && ((sourcep->update_spec & (U_NOOP|U_DELETE)) ==  (sourcep->old_update_spec & (U_NOOP|U_DELETE)))
      && !((sourcep->update_spec & U_MKDIR) && (entryp->status & S_MODIFIED)))
    {
      dbgprint(F_TRUSTTARGET, (stderr, "LINK %s %s - Trustable targetdir!\n", src, dest));
      sourcep->status |= S_TRUSTTARGET;
      CheckStatus = TRUE;
    }
  else
    {
      if (sourcep->status & S_IMMACULATE)
	{ /* was pre-empted by a virgin parent */
	  CheckStatus = FALSE;
	}
      else
	{
	  cc = readlink(dest, linksrc, MAXPATHLEN-1);

	  if (cc < 0) /* error on readlink */
	    {
	      if (errno == ENOENT)
		{ DestFileExists = FALSE; }
	      else if (errno == EINVAL)
		{ DestFileExists = TRUE; }
	      else
		{ FatalError(E_RDLINKFAILED, (stderr, "Could not readlink %s: %s\n", dest, strerror(errno))); }
	    }
	  else /* readlink succeeded */
	    {
	      DestFileExists = TRUE;
	      linksrc[cc] = '\0';
	      /* if src is relative, prefix src with a string of ../s depth minus 1 deep to get sourcepath */
	      if ( src[0] == '/' )
		{ /* absolute path */
		  (void)strcpy(srcpath, "");
		}
	      else
		{ /* relative path */
		  cp = srcpath; i = 1;
		  while ( i < depth )
		    { *cp++ = '.'; *cp++ = '.'; *cp++ = '/'; i++; }
		  *cp = '\0';
		}
	      (void)strcat(srcpath, src);
	      if (strcmp(srcpath, linksrc) == 0)
		CheckStatus = TRUE;
	    }

	  if (DestFileExists && !CheckStatus)
	    {
	      DestFileDeleteable = ((Depot_DeleteUnReferenced == TRUE) || DepotDB_AntiqueEntry(entryp));
	      if (!DestFileDeleteable)
		{ FatalError(E_BADDESTFILE, (stderr, "LINK %s %s : %s not writable\n", src, dest, dest)); }
	      else
		{ ShowActionMessage((stdout, "REMOVE %s\n", dest)); }
	    }
	}
    }
  if (!CheckStatus)
    {
      entryp->status |= S_UPDATE;
      ShowActionMessage((stdout, "LINK %s %s\n", src, dest));
    }
  else
    {
      dbgprint(F_APPLYCHECK, (stderr, "LINK %s %s is OK\n", src, dest));
    }

  dbgprint(F_TRACE, (stderr, "Check_Link done\n"));
  return CheckStatus;
}


/*
 * Boolean Check_Map(entryp, dest, src, depth)
 * depth indicates the depth of the dest from the top of the destination tree.
 */
Boolean Check_Map(entryp, dest, src, depth)
     ENTRY *entryp;
     char *dest, *src;
     unsigned depth;
{
  int MapCommandType;
  Boolean CheckStatus;

  CheckStatus = FALSE;

  MapCommandType = GetMapCommand((entryp->sourcelist+(entryp->nsources-1))->collection_name, preference);
  switch (MapCommandType)
    {
    case U_MAPCOPY:
      CheckStatus = Check_Copy(entryp, dest, src);
      break;
    case U_MAPLINK:
    default:
      CheckStatus = Check_Link(entryp, dest, src, depth);
      break;
    }
  return CheckStatus;
}


Boolean Check_MakeDir(entryp, path)
     ENTRY *entryp;
     char *path;
{
  Boolean CheckStatus;
  struct stat stb;
  SOURCE *sourcep;

  dbgprint(F_TRACE, (stderr, "Check_MakeDir\n"));

  CheckStatus = FALSE;

  sourcep = entryp->sourcelist + (entryp->nsources - 1);
  if (Depot_TrustTargetDir && (sourcep->status & S_TARGET)
      && !(entryp->status & S_MODIFIED)
      && !(sourcep->status & S_NONVIRGINTRG)
      && ((sourcep->update_spec & (U_NOOP|U_DELETE)) ==  (sourcep->old_update_spec & (U_NOOP|U_DELETE))))
    {
      dbgprint(F_TRUSTTARGET, (stderr, "MKDIR %s - Trustable targetdir!\n", path));
      sourcep->status |= S_TRUSTTARGET;
      CheckStatus = TRUE;
    }
  else
    {
      if (sourcep->status & S_IMMACULATE)
	{ /* was pre-empted by a virgin parent */
	  CheckStatus = FALSE;
	  ShowActionMessage((stdout, "MKDIR %s\n", path));
	}
      else if (lstat(path,&stb) == 0)
	{
	  if ((stb.st_mode & S_IFMT) == S_IFDIR)
	    {
	      CheckStatus = Check_CleanDir(entryp, path);
	    }
	  else
	    {
	      ShowActionMessage((stdout, "REMOVE %s\n", path));
	      ShowActionMessage((stdout, "MKDIR %s\n", path));
	    }
	}
      else
	{
	  ShowActionMessage((stdout, "MKDIR %s\n", path));
	}
    }

  if ( !CheckStatus) entryp->status |= S_UPDATE;

  dbgprint(F_TRACE, (stderr, "Check_MakeDir done\n"));
  return CheckStatus;
}




void Check_ExecRCFile(db_root, commandfilep)
     DEPOTDB *db_root;
     COMMANDFILE *commandfilep;
{
  register char **cpp;
  ENTRY *ep;
  char *compath;
  char execfilepath[MAXPATHLEN], *execfilesrc;
  struct stat stb;
  char **oldcommand;
  Boolean FoundExecFile;
  Boolean CommandCollectionsUptodate;
  Boolean CheckStatus;

  dbgprint(F_TRACE, (stderr, "Check_ExecRCFile\n"));

  CheckStatus = FALSE;
  commandfilep->command = NULL;
  if (((char *)rindex(commandfilep->label, '/')) == NULL) /* no /s in commandfilep->label */
    commandfilep->command = Preference_ExtractCommand(commandfilep->label, preference);
  if (commandfilep->command == NULL)
    { compath = commandfilep->label; }
  else
    { compath = (commandfilep->command)[0]; }

  if (Depot_TrustTargetDir && (commandfilep->status & S_TARGET))
    {
      oldcommand = Preference_ExtractCommand(commandfilep->label, PreferencesSaved);
      if (((oldcommand == NULL) && (commandfilep->command == NULL))
	  || (strarrcmp(oldcommand, commandfilep->command) == 0))
	{
	  CommandCollectionsUptodate = TRUE;
	  cpp = commandfilep->collection_list;
	  if (cpp)
	    {
	      while (CommandCollectionsUptodate && *cpp)
		{
		  if (Collection_Uptodate(*cpp) == FALSE)
		    CommandCollectionsUptodate = FALSE;
		  else
		    cpp++;
		}
	    }
	  if (CommandCollectionsUptodate)
	    {
	      dbgprint(F_TRUSTTARGET, (stderr, "EXEC %s - Trustable targetdir!\n", commandfilep->label));
	      commandfilep->status |= S_TRUSTTARGET;
	      CheckStatus = TRUE;
	    }
	}
    }

  if ( !CheckStatus)
    {
      /*
       * if compath is an absolute path,
       *   check if it exists and is execable;
       * else
       *   check if it is being created and its src is execable
       *   if (it is not being created as an executable by depot
       *	   && no /s are present in its name)
       *	 check for its existence in the current PATH
       */
      if (compath[0] == '/')
	{/* absolute path */
	  (void)strcpy(execfilepath, compath);
	  if (stat(execfilepath, &stb) < 0)
	    { FatalError(E_STATFAILED, (stderr, "Could not stat %s: %s\n", execfilepath, strerror(errno))); }
	  if (!(stb.st_mode & 0111))
	    { /* no exec permission */
	      FatalError(E_BADEXECFILE, (stderr, "Could not execute %s; permission denied\n", execfilepath));
	    }
	  FoundExecFile = TRUE;
	  if (commandfilep->command == NULL)
	    { commandfilep->command = DepotDB_Command_BuildDepotDBTargetCommand(compath); }
	}
      else
	{
	  FoundExecFile = FALSE;

	  /* check the possible source of the execfile if it is being created by depot
	   * the execfile is being created by depot if either
	   *	(commandfilep->command == NULL) => commandfilep->label indicates a command
	   * or	compath begins with the %t/ indicating a file under the targetdir
	   */
	  if ((commandfilep->command == NULL) || (strncmp("%t/", compath, 3) == 0))
	    {
	      if (commandfilep->command == NULL)
		ep = DepotDB_LocateEntryByName(db_root, compath, DB_LOCATE|DB_LAX);
	      else
		ep = DepotDB_LocateEntryByName(db_root, compath+3, DB_LOCATE|DB_LAX);
	      if (ep != NULL)
		{ /* exec file possibly being created by the depot program */
		  execfilesrc = (ep->sourcelist+(ep->nsources-1))->name;
		  if (execfilesrc == NULL)
		    { FatalError(E_UNNAMEDSRC, (stderr, "No source specified for %s\n", compath)); }
		  else if (execfilesrc[0] == '/')
		    (void)strcpy(execfilepath, execfilesrc);
		  else
		    (void)sprintf(execfilepath, "%s/%s", Depot_TargetPath, execfilesrc);
		  if ((stat(execfilepath, &stb) >= 0) && (stb.st_mode & 0111))
		    {
		      dbgprint(F_EXECRCFILE, (stderr, "Found ExecFile in %s\n", execfilepath));
		      FoundExecFile = TRUE;
		      if (commandfilep->command == NULL)
			{
			  (void)sprintf(execfilepath, "%s/%s", "%t", compath);
			  commandfilep->command = DepotDB_Command_BuildDepotDBTargetCommand(execfilepath);
			}
		    }
		}
	    }
	  if (!FoundExecFile)
	    {
	      if (((char *)rindex(compath, '/')) == NULL) /* no /s in compath */
		{
		  FoundExecFile = LocateExecFileInPATH(compath, execfilepath);
		  if (FoundExecFile)
		    {
		      if (commandfilep->command == NULL)
			{ commandfilep->command = DepotDB_Command_BuildDepotDBTargetCommand(execfilepath); }
		      else
			{
			  /* replace the first component of the command by execfilepath */
			  compath = (commandfilep->command)[0];
			  free(compath); compath = NULL;
			  *(commandfilep->command) = (char *)emalloc(strlen(execfilepath)+1);
			  *(commandfilep->command) = strcpy(*(commandfilep->command), execfilepath);
			}
		    }
		}
	    }
	}

      if (!FoundExecFile)
	{ FatalError(E_NOEXECFILE, (stderr, "No executable file found for %s\n", compath)); }

      ShowActionMessage((stdout, "EXEC %s\n", commandfilep->label));
    }


  dbgprint(F_TRACE, (stderr, "Check_ExecRCFile done\n"));
  return;
}



static Boolean Check_CleanDir(entryp, path)
     ENTRY *entryp;
     char *path;
{
  Boolean CheckStatus;
  register ENTRY *ep;
  char newpath[MAXPATHLEN];
  register struct direct *de;
  register DIR *dp;
  Boolean FoundReference;
  SOURCE *sp;

  VerboseActionMessage((stdout, "checking directory %s\n", path));
  if ((dp = opendir(path)) == NULL)
    {
      FatalError(E_OPENDIRFAILED, (stderr, "Could not open directory %s\n", path));
    }

  CheckStatus = TRUE;
  while ((de = readdir(dp)) != NULL)
    {
      if ( (strcmp(de->d_name, ".") != 0)
	  && (strcmp(de->d_name, "..") != 0))
	{
	  /* KLUDGEY - write a routine to do this in DepotDBUtil.c? */
	  ep = ENTRY_child(entryp); FoundReference = FALSE;
	  while ( (ep!= NULL) && !FoundReference)
	    {
	      if (ep->name == NULL)
		{
		  FatalError(E_UNNAMEDENTRY, (stderr, "Entry with no name found in database tree\n"));
		}
	      else if (strcmp(ep->name, de->d_name) == 0)
		FoundReference = TRUE;
	      else
		ep = ENTRY_sibling(ep);
	    }
	  if (FoundReference)
	    {
	      sp = ep->sourcelist+(ep->nsources-1);
	      if (sp->update_spec & U_DELETE)
		{
		  CheckStatus = FALSE;
		  (void)sprintf(newpath, "%s/%s", path, de->d_name);
		  ShowActionMessage((stdout, "REMOVE %s\n", newpath));
		}
	    }
	  else /* !FoundReference */
	    {
	      CheckStatus = FALSE;
	      (void)sprintf(newpath, "%s/%s", path, de->d_name);
	      ShowActionMessage((stdout, "REMOVE %s\n", newpath));
	    }
	}
    }
  (void)closedir(dp);

  return CheckStatus;
}




Boolean CheckSourceConsistency(entryp)
     ENTRY *entryp;
{
  register SOURCE *sp1, *sp2, *sp;
  register int i1, i2;
  Boolean InconsistencyFound, OverrideFound;

  dbgprint(F_TRACE, (stderr, "CheckSourceConsistency\n"));

  InconsistencyFound = FALSE;
  if (entryp->nsources != 1)
    {
      sp = entryp->sourcelist + ( entryp->nsources - 1);
      if (sp->update_spec & (U_MKDIR | U_RCFILE))
	{
	  i1 = 0; sp1 = entryp->sourcelist; InconsistencyFound = FALSE;
	  while ( (i1 < entryp->nsources-1) && !InconsistencyFound )
	    {
	      if ( ((sp1->update_spec & (U_MKDIR|U_RCFILE)) == (sp->update_spec & (U_MKDIR|U_RCFILE)))
		  || (strcmp(sp1->collection_name, "") == 0))
		{i1++; sp1++;}
	      else
		{
		  OverrideFound = FALSE; i2 = i1+1; sp2 = sp1+1;
		  while ((i2 < entryp->nsources) && !OverrideFound)
		    {
		      if (Override(sp2->collection_name, sp1->collection_name, preference) == TRUE)
			OverrideFound = TRUE;
		      else
			{ i2++; sp2++; }
		    }
		  if (!OverrideFound)
		    {
		      InconsistencyFound = TRUE;
		      FatalError(E_TYPECONFLICT, (stderr, "SOURCE CONFLICT for: %s\n\t%-15s (%s)\n\t%-15s (%s)\n", entryp->name, sp->collection_name, sp->name, sp1->collection_name, sp1->name));
		    }
		  else
		    {i1++; sp1++;}
		}
	    }
	}
#if 0
      /*
       * This has been commented out as a policy decision.
       * Overriding of unreferenced material by collection contributions
       * should be reported as conflicts		-- Sohan
       */
      else if (   !Depot_DeleteUnReferenced
	       && (sp->update_spec & U_NOOP)
	       && (strcmp(sp->collection_name, "") == 0))
	{
	  i1 = 0; sp1 = entryp->sourcelist; InconsistencyFound = FALSE;
	  /* check all but last 2 entries, since the last but one */
	  /* source may have been overridden by the unreferenced data */
	  while ( (i1 < entryp->nsources-2) && !InconsistencyFound )
	    {
	      if (strcmp(sp1->collection_name, "") == 0)
		{i1++; sp1++;}
	      else
		{
		  OverrideFound = FALSE; i2 = i1+1; sp2 = sp1+1;
		  while ((i2 < entryp->nsources) && !OverrideFound)
		    {
		      if (Override(sp2->collection_name, sp1->collection_name, preference) == TRUE)
			OverrideFound = TRUE;
		      else
			{ i2++; sp2++; }
		    }
		  if (!OverrideFound)
		    {
		      InconsistencyFound = TRUE;
		      FatalError(E_TYPECONFLICT, (stderr, "SOURCE CONFLICT for: %s\n\t%-15s (%s)\n\t%-15s (%s)\n", entryp->name, sp->collection_name, sp->name, sp1->collection_name, sp1->name));
		    }
		  else
		    {i1++; sp1++;}
		}
	    }
	}
#endif /* 0 */
      else /* sp->update_spec & (U_NOOP|U_COPY|U_LINK) */
	{
	  i1 = 0; sp1 = entryp->sourcelist; InconsistencyFound = FALSE;
	  while ( (i1 < entryp->nsources-1) && !InconsistencyFound )
	    {
	      if (strcmp(sp1->collection_name, "") == 0)
		{i1++; sp1++;}
	      else
		{
		  OverrideFound = FALSE; i2 = i1+1; sp2 = sp1+1;
		  while ((i2 < entryp->nsources) && !OverrideFound)
		    {
		      if (Override(sp2->collection_name, sp1->collection_name, preference) == TRUE)
			OverrideFound = TRUE;
		      else
			{ i2++; sp2++; }
		    }
		  if (!OverrideFound)
		    {
		      InconsistencyFound = TRUE;
		      FatalError(E_TYPECONFLICT, (stderr, "SOURCE CONFLICT for: %s\n\t%-15s (%s)\n\t%-15s (%s)\n", entryp->name, sp->collection_name, sp->name, sp1->collection_name, sp1->name));
		    }
		  else
		    {i1++; sp1++;}
		}
	    }
	}
    }

  dbgprint(F_TRACE, (stderr, "CheckSourceConsistency done\n"));
  return ( InconsistencyFound ? FALSE : TRUE );
}

/* $Source: /afs/andrew.cmu.edu/system/src/local/depot/019/RCS/check.c,v $ */
