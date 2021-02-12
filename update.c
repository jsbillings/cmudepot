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

static char rcs_id[]="$Header: /afs/andrew.cmu.edu/system/src/local/depot/019/RCS/update.c,v 4.19 1992/08/14 18:55:49 ww0r Exp $";


/*
 * Author: Sohan C. Ramakrishna Pillai
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/dir.h>
#include <time.h>
#include <errno.h>
#include <sys/wait.h>
#ifdef sun
#include <sys/time.h>
#include <vfork.h>
#endif /* sun */



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

static void Update_Remove();
static void Update_CleanDir();

Boolean Update_Noop(entryp, path)
     ENTRY *entryp;
     char *path;
{
  struct stat stb;
  SOURCE *sourcep;
  Boolean NoOpFileExists;

  dbgprint(F_TRACE, (stderr, "Update_Noop\n"));

  sourcep = entryp->sourcelist + (entryp->nsources - 1);
  if (Depot_TrustTargetDir && (sourcep->status & S_TRUSTTARGET))
    {
      dbgprint(F_TRUSTTARGET, (stderr, "NOOP %s - Trusting targetdir!\n", path));
      VerboseMessage((stdout, "NOOP %s\n", path));
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
	  VerboseMessage((stdout, "Warning: NOOP %s - File not found\n", path));
	}
      else
	{
	  VerboseMessage((stdout, "NOOP %s\n", path));
	}
    }

  dbgprint(F_TRACE, (stderr, "Update_Noop done\n"));
  return TRUE;
}


#ifndef MAXBSIZE
#define MAXBSIZE 8192
#endif /* MAXBSIZE */

Boolean Update_Copy(entryp, dest, src)
     ENTRY *entryp;
     char *dest, *src;
{
  int dp, sp;
  int nread;
  struct stat stbsrc, stbdest;
  int destuid, destgid;
#ifdef USE_UTIME
   time_t desttime[2];
#else /* USE_UTIME */
  struct timeval desttime[2];
#endif /* USE_UTIME */
  char *destdate;
  SOURCE *sourcep;
  char buffer[MAXBSIZE];
  Boolean DestFileExists, DestFileDeleteable;
  char linksrc[MAXPATHLEN], srcpath[MAXPATHLEN], temp[MAXPATHLEN];
  int cc;

  dbgprint(F_TRACE, (stderr, "Update_Copy\n"));

  sourcep = entryp->sourcelist + (entryp->nsources - 1);
  if (Depot_TrustTargetDir && (sourcep->status & S_TRUSTTARGET))
    {
      dbgprint(F_TRUSTTARGET, (stderr, "COPY %s %s - Trusting targetdir!\n", src, dest));
    }
  else
    {
      if (lstat(dest,&stbdest) < 0)
	{
	  if (errno == ENOENT) DestFileExists = FALSE;
	  else {FatalError(E_LSTATFAILED, (stderr, "Could not lstat %s: %s\n", dest, strerror(errno)));}
	}
      else
	DestFileExists = TRUE;
      
      if (DestFileExists)
	{
	  DestFileDeleteable = ((Depot_DeleteUnReferenced == TRUE) || DepotDB_AntiqueEntry(entryp));
	  if (!DestFileDeleteable)
	    {
	      FatalError(E_BADDESTFILE, (stderr, "COPY %s %s : %s not writable\n", src, dest, dest));
	    }
	  else
	    Update_Remove(dest);
	}

      if ( src[0] == '/' ) /* absolute path */
	(void)strcpy(srcpath, src);
      else /* relative path */
	(void)sprintf(srcpath, "%s/%s", Depot_TargetPath, src);

      if (lstat(srcpath, &stbsrc) < 0)
	{ FatalError(E_LSTATFAILED, (stderr, "Could not lstat %s: %s\n", srcpath, strerror(errno))); }

      if ((stbsrc.st_mode & S_IFMT) == S_IFLNK)
	{
	  if ((cc = readlink(srcpath, linksrc, MAXPATHLEN-1)) <= 0)
	    { FatalError(E_RDLINKFAILED, (stderr, "Could not read the link %s: %s\n", srcpath, strerror(errno))); }
	  else
	    {
	      linksrc[cc] = '\0';
	      if (symlink(linksrc, dest) < 0)
		{ FatalError(E_LINKFAILED, (stderr, "Could not make symlink from %s->%s: %s\n", dest, linksrc, strerror(errno))); }
	    }
	}
      else
	{
	  /* copy file over */
	  (void)sprintf(temp, "%s.NEW", dest);
	  VerboseMessage((stdout, "COPY %s %s\n", src, temp));
	  if ((sp = open(srcpath, O_RDONLY)) < 0)
	    { FatalError(E_OPENFAILED, (stderr, "COPY %s %s : Could not open %s to read\n", src, dest, srcpath)); }
	  if ((dp = open(temp, O_WRONLY|O_CREAT|O_TRUNC, stbsrc.st_mode & 0755)) < 0)
	    { FatalError(E_OPENFAILED, (stderr, "COPY %s %s : Could not open %s to write\n", src, dest, temp)); }
      
	  while ( (nread = read(sp, buffer, MAXBSIZE)) > 0 )
	    {
	      if (write(dp, buffer, nread) != nread)
		{ FatalError(E_WRITEFAILED, (stderr, "COPY %s %s : write to %s failed!\n", src, dest, temp)); }
	    }
      
	  (void)close(sp); 
	  if (close(dp) < 0) {
	    FatalError(E_WRITEFAILED, (stderr, "close:Unable to close destination file:%s\n", 
				       strerror(errno)));
	  }
	  VerboseMessage((stdout, "RENAME %s %s\n", temp, dest));
	  if (rename(temp, dest) < 0)
	    { FatalError(E_RENAMEFAILED, (stderr, "COPY %s %s : Could not move %s to %s\n", src, dest, temp, dest)); }

	  /* if suid or sgid, set owner/group and mode bits */
	  if (stbsrc.st_mode & (S_ISUID | S_ISGID))
	    {
	      destuid = (stbsrc.st_mode & S_ISUID) ?  stbsrc.st_uid : -1 ;
	      destgid = (stbsrc.st_mode & S_ISGID) ?  stbsrc.st_gid : -1 ;
	      VerboseMessage((stdout, "CHOWN %s %d %d\n", dest, destuid, destgid));
	      if (chown(dest, destuid, destgid) < 0)
		{ DepotError(DE_CHOWNFAILED, (stderr, "CHOWN %s %d %d failed: %s\n", dest, destuid, destgid, strerror(errno))); }
	      else
		{
		  VerboseMessage((stdout, "CHMOD %s %o\n", dest, stbsrc.st_mode & 07755));
		  if (chmod(dest, (stbsrc.st_mode & 07755)) < 0)
		    { DepotError(DE_CHMODFAILED, (stderr, "CHMOD %s %o failed: %s\n", dest, stbsrc.st_mode & 07755, strerror(errno))); }
		}
	    }

	  /* set mod time */
#ifdef USE_UTIME
	  desttime[0] = desttime[1] = (long)(stbsrc.st_mtime);
	  destdate = ctime(&desttime[1]); destdate[24] = '\0';
	  VerboseMessage((stdout, "UTIME %s %s\n", dest, destdate));

	  if (utime(dest, desttime) < 0)
	    { DepotError(DE_UTIMEFAILED, (stderr, "UTIME %s %s failed: %s\n", dest, destdate, strerror(errno))); }

#else /* USE_UTIME */
	  desttime[0].tv_sec = desttime[1].tv_sec = (long)(stbsrc.st_mtime);
	  desttime[0].tv_usec = desttime[1].tv_usec = 0;
	  destdate = ctime((long *)&(desttime[1].tv_sec)); destdate[24] = '\0';
	  VerboseMessage((stdout, "UTIMES %s %s\n", dest, destdate));

	  if (utimes(dest, desttime) < 0)
	    { DepotError(DE_UTIMESFAILED, (stderr, "UTIMES %s %s failed: %s\n", dest, destdate, strerror(errno))); }

#endif /* USE_UTIME */
	}
    }
  dbgprint(F_TRACE, (stderr, "Update_Copy done\n"));
  return TRUE;
}


/*
 * Boolean Update_Link(entryp, dest, src, depth)
 * depth indicates the depth of the dest from the top of the destination tree.
 */
Boolean Update_Link(entryp, dest, src, depth)
     ENTRY *entryp;
     char *dest, *src;
     unsigned depth;
{
  struct stat stb;
  SOURCE *sourcep;
  Boolean DestFileExists, DestFileDeleteable;
  char srcpath[MAXPATHLEN];
  register unsigned i;
  register char *cp;

  dbgprint(F_TRACE, (stderr, "Update_Link\n"));

  sourcep = entryp->sourcelist + (entryp->nsources - 1);
  if (Depot_TrustTargetDir && (sourcep->status & S_TRUSTTARGET))
    {
      dbgprint(F_TRUSTTARGET, (stderr, "LINK %s %s - Trusting targetdir!\n", src, dest));
    }
  else
    {
      if (lstat(dest,&stb) < 0)
	{
	  if (errno == ENOENT) DestFileExists = FALSE;
	  else {FatalError(E_LSTATFAILED, (stderr, "Could not lstat %s: %s\n", dest, strerror(errno)));}
	}
      else
	DestFileExists = TRUE;

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
      
      DestFileDeleteable = ((Depot_DeleteUnReferenced == TRUE) || DepotDB_AntiqueEntry(entryp));
      if (!DestFileExists || DestFileDeleteable)
	{
	  Update_Remove(dest);
	  VerboseMessage((stdout, "LINK %s %s\n", src, dest));
	  if (symlink(srcpath, dest) < 0)
	    {
	      FatalError(E_LINKFAILED, (stderr, "Could not symlink %s->%s: %s\n", dest, srcpath, strerror(errno)));
	    }
	}
    }

  dbgprint(F_TRACE, (stderr, "Update_Link done\n"));
  return TRUE;
}


/*
 * Boolean Update_Map(entryp, dest, src, depth)
 * depth indicates the depth of the dest from the top of the destination tree.
 */
Boolean Update_Map(entryp, dest, src, depth)
     ENTRY *entryp;
     char *dest, *src;
     unsigned depth;
{
  int MapCommandType;
  Boolean ret_val;

  MapCommandType = GetMapCommand((entryp->sourcelist+(entryp->nsources-1))->collection_name, preference);
  switch (MapCommandType)
    {
    case U_MAPCOPY:
      ret_val = Update_Copy(entryp, dest, src);
      break;
    case U_MAPLINK:
    default:
      ret_val = Update_Link(entryp, dest, src, depth);
      break;
    }
  return ret_val;
}


Boolean Update_MakeDir(entryp, path)
     ENTRY *entryp;
     char *path;
{
  struct stat stb;
  SOURCE *sourcep;

  dbgprint(F_TRACE, (stderr, "Update_MakeDir\n"));

  sourcep = entryp->sourcelist + (entryp->nsources - 1);
  if (Depot_TrustTargetDir && (sourcep->status & S_TRUSTTARGET))
    {
      dbgprint(F_TRUSTTARGET, (stderr, "MKDIR %s - Trusting targetdir!\n", path));
    }
  else
    {
      if (lstat(path,&stb) < 0)
	{
	  if (errno != ENOENT)
	    {
	      FatalError(E_LSTATFAILED, (stderr, "Could not lstat %s: %s\n", path, strerror(errno)));
	    }
	}
      else if ((stb.st_mode & S_IFMT) == S_IFDIR)
	{
	  VerboseMessage((stdout, "DIRECTORY %s\n", path));
	  Update_CleanDir(entryp, path);
	  dbgprint(F_TRACE, (stderr, "Update_MakeDir - being implemented\n"));
	  return TRUE;
	}
      else
	{
	  Update_Remove(path);
	}
      VerboseMessage((stdout, "MKDIR %s\n", path));
      if (mkdir(path, 0755) < 0)
	{
	  FatalError(E_MKDIRFAILED, (stderr, "Could not make directory %s\n", path));
	}
    }

  dbgprint(F_TRACE, (stderr, "Update_MakeDir - being implemented\n"));
  return TRUE;
}


void Update_ExecRCFile(commandfilep)
     COMMANDFILE *commandfilep;
{
  char **av;
  struct stat stb;
  int pid;
  union wait ExecStatus;

  dbgprint(F_TRACE, (stderr, "Update_ExecRCFile\n"));

  if (Depot_TrustTargetDir && (commandfilep->status & S_TRUSTTARGET))
    {
      dbgprint(F_TRUSTTARGET, (stderr, "EXEC %s - Trusting targetdir!\n", commandfilep->label));
    }
  else
    {
      if (commandfilep->command == NULL)
	{
	  FatalError(E_NOEXECFILE, (stderr, "No executable file found for label %s\n", commandfilep->label));
	}
      /* expand any "%t"s in the command to the targetdir */
      av = DepotDB_Command_ExpandMagic(commandfilep->command);

      if (stat(av[0], &stb) < 0)
	{ FatalError(E_STATFAILED, (stderr, "Could not stat %s: %s\n", av[0], strerror(errno))); }
      if (!(stb.st_mode & 0111))
	{ /* no exec permission */
	  FatalError(E_BADEXECFILE, (stderr, "Could not execute %s; permission denied\n", av[0]));
	}

      VerboseMessage((stdout, "EXEC %s\n", commandfilep->label));
  
      pid = vfork();

      if ( pid < 0 )
	{
	  FatalError(E_VFORKFAILED, (stderr, "Could not execute %s; vfork failed\n", av[0]));
	}
      else if ( pid == 0 )
	{ /* child */
	  execv(av[0], av);
	  perror("exec");
	  _exit(-4);      /* fluff */
	}
      wait(&ExecStatus);

      if (ExecStatus.w_retcode != 0)
	{
	  FatalError(E_EXECFAILED, (stderr, "Execution of %s failed with exit status = %d\n", av[0], ExecStatus.w_retcode));
	}
      strarrfree(av);
    }
  dbgprint(F_TRACE, (stderr, "Update_ExecRCFile done\n"));
  return;
}


/*
 * static void Update_Remove(path)
 * Removes any file/dir/symlink represented by path - Handle with care!!
 */
static void Update_Remove(path)
     char *path;
{
  char newpath[MAXPATHLEN];
  register struct direct *de;
  register DIR *dp;
  struct stat stb;

  if (lstat(path,&stb) < 0)
    {
      if (errno == ENOENT) return;
      else
	{
	  FatalError(E_LSTATFAILED, (stderr, "Could not lstat %s: %s\n", path, strerror(errno)));
	}
    }

  if ((stb.st_mode & S_IFMT) != S_IFDIR)
    {
      VerboseMessage((stdout, "REMOVE %s\n", path));
      if (unlink(path) < 0)
	{
	  FatalError(E_UNLINKFAILED, (stderr, "Could not unlink %s:%s\n", path, strerror(errno)));
	}
      else return;
    }

  if ((dp = opendir(path)) == NULL)
    {
      FatalError(E_OPENDIRFAILED, (stderr, "Could not open directory %s\n", path));
    }

  while ((de = readdir(dp)) != NULL)
    {
      if ( (strcmp(de->d_name, ".") != 0) && (strcmp(de->d_name, "..") != 0))
	{
	  (void)sprintf(newpath, "%s/%s", path, de->d_name);
	  Update_Remove(newpath);
	}
    }
  (void)closedir(dp);
  VerboseMessage((stdout, "REMOVE DIRECTORY %s\n", path));
  if (rmdir(path) < 0)
    {
      FatalError(E_RMDIRFAILED, (stderr, "Could not remove directory %s: %s\n", path, strerror(errno)));
    }
  else return;
}



/*
 * static void Update_CleanDir(entryp, path)
 * removes all unreferenced files other than depot under path
 */
static void Update_CleanDir(entryp, path)
     ENTRY *entryp;
     char *path;
{
  register ENTRY *ep;
  char newpath[MAXPATHLEN];
  register struct direct *de;
  register DIR *dp;
  Boolean FoundReference;
  SOURCE *sp;

  if ((dp = opendir(path)) == NULL)
    {
      if (errno == ENOENT) return;
      if (errno == ENOTDIR) {
	FatalError(E_NOTDIR, 
		   (stderr, "Update_CleanDir: %s is not a directory\n", path));
      } else {
	FatalError(E_OPENDIRFAILED, (stderr, "Could not open directory %s\n", path));
      }
    }

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
		  (void)sprintf(newpath, "%s/%s", path, de->d_name);
		  Update_Remove(newpath);
		}
	    }
	  else /* !FoundReference */
	    {
	      (void)sprintf(newpath, "%s/%s", path, de->d_name);
	      Update_Remove(newpath);
	    }
	}
    }
  (void)closedir(dp);

  return;
}
/* $Source: /afs/andrew.cmu.edu/system/src/local/depot/019/RCS/update.c,v $ */
