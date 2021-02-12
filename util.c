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

static char rcs_id[]="$Header: /afs/andrew.cmu.edu/system/src/local/depot/019/RCS/util.c,v 4.6 1992/08/14 18:56:21 ww0r Exp $";


/*
 * Author: Sohan C. Ramakrishna Pillai
 */

#include <stdio.h>
#ifndef ibm032
#include <stdlib.h>
#endif /* ibm032 */
#include <fcntl.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/file.h>

#include "globals.h"

#ifdef ibm032 
void *malloc();
void *calloc();
void *realloc();
void *getenv();
#endif /* ibm032 */


#ifdef sun
char *
strerror(num)
     unsigned int num;
{
  extern char *sys_errlist[];

  return(sys_errlist[num]);
}
#endif sun

/*
 * a bunch of useful routines to have around
 */
char *emalloc(size)
unsigned size;
/* malloc(3) with error checking */
{
    char *ptr;

    if ((ptr = (char *)malloc(size)) == NULL)
    {
	FatalError(E_OUTOFMEMORY, (stderr, "Error: out of memory\nmalloc() failed\n"));
	return NULL;
    }
    else
	return ptr;
}


char *ecalloc(nelem, size)
unsigned nelem, size;
/* calloc(3) with error checking */
{
    char *ptr;

    if ((ptr = calloc(nelem, size)) == NULL)
    {
	FatalError(E_OUTOFMEMORY, (stderr, "Error: out of memory\ncalloc() failed\n"));
	return NULL;
    }
    else
	return ptr ;
}



char *erealloc(ptr, size)
char *ptr;
unsigned size;
/* realloc(3) with error checking */
{
    char *newptr;

    if ((newptr = realloc(ptr, size)) == NULL)
    {
	FatalError(E_OUTOFMEMORY, (stderr, "Error: out of memory\nrealloc() failed\n"));
	return NULL;
    }
    else
	return newptr ;
}

FILE *efopen(filename, type)
char *filename, *type;
/* fopen(3S) with error checking */
{
    FILE *f;

    if ((f = fopen(filename, type)) == NULL)
    {
	FatalError(E_FOPENFAILED, (stderr, "Error: could not open file\nfopen(%s, %s) failed\n", filename, type));
	return NULL;
    }
    else
	return f ;
}

/*
 * int fdgetc(fd, pos)
 *	returns the character at offset pos from the beginning
 *	of file given by descriptor fd
 */
int fdgetc(fd, pos)
int fd;
long pos;
{
  char c;	/* buffer of length 1 for reading */

  lseek(fd, pos, L_SET);
  if (read(fd, &c, 1) == 0)
    return EOF;
  else
    return (int)c;
}



/*
 * char **strarrcpy(to, from)
 *	copies the NULL-terminated array of strings from from to to
 *	returns a pointer to the destination array
 */
char **strarrcpy(to, from)
     char **to, **from;
{
  register char **fpp, **tpp;

  if (from == NULL)
    { FatalError(E_NULLSTRARRAY, (stderr, "strarrcpy: attempt to copy NULL array of strings\n"));}
  else
    {
      fpp = from; tpp = to;
      while (*fpp != NULL)
	{
	  *tpp = (char *)emalloc(strlen(*fpp) + 1);
	  *tpp = strcpy(*tpp, *fpp);
	  fpp++; tpp++;
	}
      *tpp = NULL;
    }
  return to;
}



/*
 * int strarrcmp(arr1, arr2)
 *	does the equivalent of a strcmp for strings
 *	on the string arrays arr1 and arr2
 */
int strarrcmp(arr1, arr2)
     char **arr1, **arr2;
{
  int Comparison;
  register char **cp1, **cp2;

  if (arr1 == NULL)
    {
      if (arr2 == NULL) return 0;
      else return -1;
    }
  else if (arr2 == NULL) return 1;

  cp1 = arr1, cp2 = arr2; Comparison = 0;
  while (Comparison == 0)
    {
      if ((*cp1 == NULL) || (*cp1 == (char *)0))
	{
	  if ((*cp2 == NULL) || (*cp2 == (char *)0))
	    return 0;
	  else
	    return -1;
	}
      else if ((*cp2 == NULL) || (*cp2 == (char *)0))
	return 1;
      Comparison = strcmp(*cp1, *cp2);
      cp1++; cp2++;
    }
  return Comparison;
}



/*
 * char **strarrcat(arr1, arr2)
 *	appends a copy of the the NULL-terminated array of strings arr2
 *	to the end of a similarly structured string arrray arr1
 *	returns a pointer to the result
 */
char **strarrcat(arr1, arr2)
     char **arr1, **arr2;
{
  register char **fpp, **tpp;

  if ((arr1 == NULL) || (arr2 == NULL))
    { FatalError(E_NULLSTRARRAY, (stderr, "strarrcat: attempt to catenate NULL arrays of strings\n"));}
  else
    {
      tpp = arr1;
      while (*tpp != NULL) tpp++;
      fpp = arr2;
      while (*fpp != NULL)
	{
	  *tpp = (char *)emalloc(strlen(*fpp) + 1);
	  *tpp = strcpy(*tpp, *fpp);
	  fpp++; tpp++;
	}
      *tpp = NULL;
    }

  dbgDumpStringArray(arr1);
  return arr1;
}



/*
 * int strarrsize(strarr)
 */
int strarrsize(strarr)
     char **strarr;
{
  register int i;
  register char **sp;
  int arrsize;

  if (strarr == NULL)
    arrsize = 0;
  else
    {
      i = 0; sp = strarr;
      while ((*sp != NULL) && (*sp != (char *)0))
	{ i++; sp++; }
      arrsize = i + 1;
    }
  return arrsize;
}


void strarrfree(strarr)
char **strarr;
{
  register char **sp;

  if (strarr != NULL)
    {
      for (sp = strarr; *sp != NULL; sp++)
	{ free(*sp); }
      free((char *)strarr);
    }
}


/*
 * char **sortedstrarrinsert(strarr, str)
 *	Inserts the string str into the appropriate location in the
 *	sorted dynamically allocated NULL-terminated array of strings strarr.
 *	Returns the head of the new sorted array of strings.
 * WARNING: This could lead to unpredictable results if tried on
 *	unsorted string arrays or statically allocated storage.
 */
char **sortedstrarrinsert(strarr, str)
     char **strarr;
     char *str;
{
  register char **sp, **tp;
  register u_short i;
  u_short InsertPoint;
  int StringComparison, ArraySize;
  Boolean DuplicateString, LocatedInsertPoint;
  char **newstrarr;

  if (str == NULL)
    { FatalError(E_NULLSTRING, (stderr, "sortedstrarrinsert: attempt to insert NULL string into NULL-terminated array of strings\n"));}

  newstrarr = strarr;
  if (newstrarr == NULL)
    { /* allocate array of length 1 */
      newstrarr = (char **)emalloc(sizeof(char *));
      *newstrarr = NULL;
    }

  i = 0; sp = newstrarr; DuplicateString = FALSE; LocatedInsertPoint = FALSE;
  while (!LocatedInsertPoint && !DuplicateString && (*sp != NULL))
    {
      StringComparison = strcmp(*sp, str);
      if (StringComparison == 0)
	{DuplicateString = TRUE;}
      else if (StringComparison > 0 )
	{LocatedInsertPoint = TRUE; InsertPoint = i;}
      else /* StringComparison < 0 */
	{ i++; sp++; }    
    }

  if ( !DuplicateString )
    {
      ArraySize = strarrsize(newstrarr);
      if (!LocatedInsertPoint) InsertPoint = (u_short)ArraySize -1 ;
      ArraySize++; newstrarr = (char **)erealloc((char *)newstrarr, (unsigned)ArraySize*sizeof(char *));
      i = ArraySize - 1; sp = newstrarr+i-1; tp = newstrarr+i;
      while (i > InsertPoint)
	{
	  *tp-- = *sp--; i--;
	}
      sp = newstrarr + InsertPoint;
      *sp = (char *)emalloc(strlen(str)+1);
      *sp = strcpy(*sp, str);
    }

  dbgDumpStringArray(newstrarr);

  return newstrarr;
}




/*
 * char **splitstrarr(string, separator)
 *	splits up the separator-separated list of strings represented
 * by the string string into a NULL-terminated array of strings
 */
char **splitstrarr(string, separator)
     char *string;
     int separator;
{
  char **strarr;
  unsigned arrsize;
  register char *cp, *cp2;
  register unsigned i;
  register char **cpp;
  int strsize;
  char newstring[4*MAXPATHLEN];

  dbgprint(F_TRACE, (stderr, "splitstrarr\n"));

  strarr = NULL;
  if (string == NULL)
    strarr = NULL;
  else
    {
      /*
       * count the number of strings in the string array = 1 + no. of separators
       * and replace each separator by a `\0` - the separators are restored later
       * and copy over to newstring,
       * taking care of cases with trailing or leading separators
       */
      if ( (string[0] == (char)separator) && (string[1] == (char)separator) && (string[3] == '\0'))
	{ arrsize = 1; newstring[0] = '\0'; newstring[1] = '\0'; }
      else
	{
	  i = 0; cp = string; cp2 = newstring;
	  if (*cp == (char)separator)
	    cp++;
	  while (*cp != '\0')
	    {
	      if (*cp == (char)separator)
		{
		  if (*(cp+1) != '\0')
		    { *cp2++ = '\0'; i++; }
		}
	      else
		{ *cp2++ = *cp; }
	      cp++;
	    }
	  *cp2 = '\0';
	  arrsize = i + 1;
	}
      dbgprint(F_STRARRSPLIT, (stderr, "splitting into %u strings\n", arrsize));
      /* allocate space to strarr */
      strarr = (char **)ecalloc(arrsize+1, sizeof(char *));
      /* copy the strings one by one */
      i = 0; cp = newstring; cpp = strarr;
      while ( i < arrsize )
	{
	  strsize = strlen(cp) + 1;
	  *cpp = (char *)emalloc(strsize);
	  *cpp = strcpy(*cpp, cp);
	  cp += strsize - 1; *cp = (char)separator; /* restore separator */
	  dbgprint(F_STRARRSPLIT, (stderr, "string %u is %s\n", i+1, *cpp));
	  i++; cp++; cpp++;
	}
      *cpp = NULL;	/* NULL-terminate string array */
      *(cp - 1) = '\0'; /* undo false restore of separator at the end of original string */
      dbgprint(F_STRARRSPLIT, (stderr, "restored string is %s\n", string));
    }
  dbgprint(F_TRACE, (stderr, "splitstrarr done\n"));

  return strarr;
}


/*
 * Boolean LocateExecFileInPATH(filename, filepath)
 *	attempts to locate the path to the executable file filename
 * from the PATH environment variable.
 * If successfully located
 *	the path to the executable is stored in filepath and TRUE is returned;
 * else
 *	FALSE is returned.
 * Caveat:
 *	filepath must have enough storage to accomodate the full pathname of the executable
 */
Boolean LocateExecFileInPATH(filename, filepath)
     char *filename, *filepath;
{
  register char **pp;
  char *Path;
  char **PathArray;
  Boolean FoundExecFile;
  struct stat stb;

  dbgprint(F_TRACE, (stderr, "LocateExecFileInPATH\n"));

  Path = getenv("PATH");
  dbgprint(F_EXECRCFILE, (stderr, "PATH is %s\n", Path));
  PathArray = splitstrarr(Path, ':');

  pp = PathArray;

  FoundExecFile = FALSE;
  if (pp != NULL)
    {
      while ((*pp != NULL) && !FoundExecFile)
	{
	  (void)sprintf(filepath, "%s/%s", *pp, filename);
	  if ((stat(filepath, &stb) >= 0) && (stb.st_mode & 0111))
	    {
	      dbgprint(F_EXECRCFILE, (stderr, "Found ExecFile in %s\n", filepath));
	      FoundExecFile = TRUE;
	    }
	  else
	    {
	      dbgprint(F_EXECRCFILE, (stderr, "ExecFile not under %s\n", *pp));
	      pp++;
	    }
	}
    }

  dbgprint(F_TRACE, (stderr, "LocateExecFileInPATH done\n"));
  return FoundExecFile;
}




#ifdef DEBUG
void dbgDumpStringArray(list)
     char **list;
{
  register char **fp;

  dbgprint(F_STRARRINSERT, (stderr, "STRARR is"));
  if (list != NULL)
    {
      for (fp = list; *fp != NULL; fp++)
	{
	  dbgprint(F_STRARRINSERT, (stderr, " %s", *fp));
	}
      dbgprint(F_STRARRINSERT, (stderr, " NULL"));
    }
  dbgprint(F_STRARRINSERT, (stderr, "\n"));
}

#else DEBUG
void dbgDumpStringArray(list)
     char **list;
{
}
#endif DEBUG

/* $Source: /afs/andrew.cmu.edu/system/src/local/depot/019/RCS/util.c,v $ */
