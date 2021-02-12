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


static char rcs_id[]="$Header: /afs/andrew.cmu.edu/system/src/local/depot/019/RCS/Lock.c,v 4.7 1992/06/19 20:47:34 ww0r Exp $";


/*
 * Author: Sohan C. Ramakrishna Pillai
 */

#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/param.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <pwd.h>

#include "globals.h"
#include "Lock.h"

#ifdef ibm032
extern int errno;
#endif /* ibm032 */

/*
 * char * Lock_QueryLock(LockFile)
 *	returns NULL if not locked
 *	     or "?" if the identity of the owner of the lock is not known
 *	     or the identity of the owner of the lock.
 */
char * Lock_QueryLock(LockFile)
     char *LockFile;
{
  char *Locker;		/* identity of person who has lock file locked */
  int lockfd;		/* file descriptor for lock file */
  long lockfpos;	/* current position during scan of lock file */
  register int ch;
  register char *cp;
  char buffer[LOCKSMITHNAMESIZE];
  struct stat stb;

  dbgprint(F_TRACE, (stderr, "Lock_QueryLock\n"));

  if ( (stat(LockFile, &stb) < 0) && (errno != ENOENT))
    { FatalError(E_BADLOCKFILE, (stderr, "Error: Could not query lock file %s\n", LockFile)); }
  if ((lockfd = open(LockFile, O_RDONLY)) < 0)
    Locker = NULL;
  else
    {
      cp = buffer; lockfpos = 0;
      ch = fdgetc(lockfd, lockfpos++);
      while ((ch != EOF) && (ch != '\n'))
	{ *cp++ = (char)ch; ch = fdgetc(lockfd, lockfpos++); }
      *cp = '\0';
      if (buffer[0] == '\0')
	{ buffer[0] = '?'; buffer[1] = '\0'; }
      Locker = (char *)emalloc(strlen(buffer)+1);
      Locker = strcpy(Locker, buffer);
      close(lockfd);
    }

  dbgprint(F_TRACE, (stderr, "Lock_QueryLock done\n"));
  return Locker;
}



/*
 * char * Lock_SetLock(LockFile)
 *	returns NULL if the file is locked successfully
 * 	     or the name of the person who has locked the file.
 */
char * Lock_SetLock(LockFile)
     char *LockFile;
{
  char *Locker;		/* identity of person trying to set the lock */
  int lockfd;		/* file descriptor for lock file */
  int namesize;
  char buffer[LOCKSMITHNAMESIZE];
  struct passwd *pw;
  struct stat stb;

  dbgprint(F_TRACE, (stderr, "Lock_SetLock\n"));

  if ( (stat(LockFile, &stb) < 0) && (errno != ENOENT))
    { FatalError(E_BADLOCKFILE, (stderr, "Error: Could not query lock file %s\n", LockFile)); }

  if ((lockfd = open(LockFile, O_EXCL|O_CREAT|O_WRONLY, 0777)) < 0)
    {
      /* this assumes that open returns EACCES on O_CREAT if the file does
       * not exist and the directory in which it is to be created does not 
       * permit writing 
       */
      if (errno == EACCES) { 
	FatalError(E_CANTLOCK, (stderr, "Error: Permission denied in writing the lock file (%s)\n", LockFile));
      }
      if ((Locker = Lock_QueryLock(LockFile)) == NULL)
	{
	  buffer[0] = '?'; buffer[1] = '\0';
	  Locker = (char *)emalloc(strlen(buffer)+1);
	  Locker = strcpy(Locker, buffer);
	}
    }
  else
    {
      Locker = NULL;
      if ((pw = getpwuid(getuid())) == NULL)
	{
	  close(lockfd); (void)unlink(LockFile);
	  FatalError(E_BADUSER, (stderr, "Attempt to set lock by unknown user\n"));
	}
      else
	{
	  (void)strcpy(buffer, pw->pw_name);
	  namesize = strlen(buffer) + 1;
	  if (write(lockfd, buffer, namesize) != namesize)
	    {
	      close(lockfd); (void)unlink(LockFile);
	      FatalError(E_NOLOCK, (stderr, "Attempt to set lock failed\nPlease try again later!\n"));
	    }
	  close(lockfd);
	  Locker = (char *)emalloc(strlen(buffer)+1);
	  Locker = strcpy(Locker, buffer);
	  free(Locker); Locker = NULL;
	}
      close(lockfd);
    }

  dbgprint(F_TRACE, (stderr, "Lock_SetLock done\n"));
  return Locker;
}



/*
 * char * Lock_UnsetLock(LockFile)
 *	returns NULL if the file is unlocked successfully
 * 	     or the name of the person who has locked the file.
 */
char * Lock_UnsetLock(LockFile)
     char *LockFile;
{
  char *Locker;		/* identity of person who has the lock file locked */
  char *UnLocker;	/* identity of person trying to unset the lock */
  char buffer[LOCKSMITHNAMESIZE];
  struct passwd *pw;
  struct stat stb;

  dbgprint(F_TRACE, (stderr, "Lock_UnsetLock\n"));

  if ( (stat(LockFile, &stb) < 0) && (errno != ENOENT))
    { FatalError(E_BADLOCKFILE, (stderr, "Error: Could not query lock file %s\n", LockFile)); }

  /* find out who owns the lock */
  if ((Locker = Lock_QueryLock(LockFile)) == NULL)
    {
      FatalError(E_NOLOCK, (stderr, "Attempt to unset a lock which was not set failed\n"));
    }
  /* find out who wants to break the lock */
  if ((pw = getpwuid(getuid())) == NULL)
    {
      FatalError(E_BADUSER, (stderr, "Attempt to unset lock by unknown user\n"));
    }
  else
    {
      (void)strcpy(buffer, pw->pw_name);
      UnLocker = (char *)emalloc(strlen(buffer)+1);
      UnLocker = strcpy(UnLocker, buffer);
    }

  if (strcmp(Locker, UnLocker) == 0)
    { /* Locker wants to unset Lock */
      (void)unlink(LockFile); 
      free(Locker); Locker = NULL;
    }

  free(UnLocker); UnLocker = NULL;

  dbgprint(F_TRACE, (stderr, "Lock_UnsetLock done\n"));
  return Locker;
}



/*
 * char * Lock_PickLock(LockFile)
 *	returns NULL if the lock is picked successfully
 *	     or the name of the person who has locked the file.
 */
char * Lock_PickLock(LockFile)
     char *LockFile;
{
  char *Locker;		/* identity of person who has lock file locked */
  char *LockPicker;    	/* identity of person who wants to pick the lock */
  int lockfd;		/* file descriptor for lock file */
  int namesize;
  char buffer[LOCKSMITHNAMESIZE];
  struct passwd *pw;
  struct stat stb;

  dbgprint(F_TRACE, (stderr, "Lock_PickLock\n"));

  if ( (stat(LockFile, &stb) < 0) && (errno != ENOENT))
    { FatalError(E_BADLOCKFILE, (stderr, "Error: Could not query lock file %s\n", LockFile)); }

  /* find out who wants to break the lock */
  if ((pw = getpwuid(getuid())) == NULL)
    {
      FatalError(E_BADUSER, (stderr, "Attempt to pick lock by unknown user\n"));
    }
  else
    {
      (void)strcpy(buffer, pw->pw_name);
      namesize = strlen(buffer) + 1;
      LockPicker = (char *)emalloc(strlen(buffer)+1);
      LockPicker = strcpy(LockPicker, buffer);
    }

  if ((Locker = Lock_QueryLock(LockFile)) == NULL)
    {
      Locker = Lock_SetLock(LockFile);
    }
  else if ((lockfd = open(LockFile, O_WRONLY, 0777)) >= 0)
    {
      if (write(lockfd, buffer, namesize) != namesize)
	{
	  close(lockfd); (void)unlink(LockFile);
	  FatalError(E_NOLOCK, (stderr, "Attempt to pick lock failed\n"));
	}
      else { 
	close(lockfd); 
	free(Locker);
	Locker = NULL; 
      }
    }
  
  free(LockPicker); LockPicker = NULL;

  dbgprint(F_TRACE, (stderr, "Lock_PickLock done\n"));
  return Locker;
}


/* $Source: /afs/andrew.cmu.edu/system/src/local/depot/019/RCS/Lock.c,v $ */
