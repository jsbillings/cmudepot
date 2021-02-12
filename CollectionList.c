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


static char rcs_id[]="$Header: /afs/andrew.cmu.edu/system/src/local/depot/019/RCS/CollectionList.c,v 4.11 1992/07/01 19:12:34 ww0r Exp $";


/*
 * Author: Sohan C. Ramakrishna Pillai
 */

#include <stdio.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/dir.h>
#include <string.h>
#include <errno.h>

#include "globals.h"
#include "depot.h"
#include "PreferenceStruct.h"
#include "Preference.h"
#include "DepotDBStruct.h"
#include "DepotDB.h"
#include "CollectionStruct.h"
#include "Collection.h"

#ifdef ibm032
char *strrchr();
extern int errno;
#endif /* ibm032 */

static COLLECTIONLIST *CollectionList_AddCollection();
static void dbgDumpCollectionList();



COLLECTIONLIST *CollectionList_GetAllCollections()
{
  register char **fp;
  char *cp, *colname;
  char **cnamelist;
  char **DefaultSearchPath;
  char SearchDir[MAXPATHLEN], CheckPath[MAXPATHLEN];
  COLLECTIONLIST *clist;
  register struct direct *de;
  register DIR *dp;
  struct stat stb;

  dbgprint(F_TRACE, (stderr, "CollectionList_GetAllCollections - being implemented\n"));

  /* get a list of possible collection names from the preferences file */
  cnamelist = Preference_GetItemList(preference);
  /* add to this list the names of all directories on the generic searchpath, if any */
  DefaultSearchPath = Preference_GetStringArray(preference, "*", "searchpath");
  if (DefaultSearchPath == NULL)
    {
      dbgprint(F_COLLGETLIST, (stderr, "No search path specified in preferences, using %s\n", DEFAULTCOLLECTIONPATH));
      /* use hardwired default directory */
      DefaultSearchPath = sortedstrarrinsert(DefaultSearchPath, DEFAULTCOLLECTIONPATH);
    }
  for (fp = DefaultSearchPath; *fp != NULL; fp++)
    {

      if (**fp == '/')
	(void)strcpy(SearchDir, *fp);
      else
	(void)sprintf(SearchDir, "%s/%s", Depot_TargetPath, *fp);

      dbgprint(F_COLLGETLIST, (stderr, "Search path %s = %s for collections\n", SearchDir, *fp));

      if ((dp = opendir(SearchDir)) == NULL) {
	if (errno == ENOTDIR) {
	  WarningError((stderr, "%s in searchpath for collections is not a valid directory\n", *fp)); 
	} else {
	  WarningError((stderr, "Could not open directory %s in searchpath\n", *fp)); 
	}
      } else {
	while ((de = readdir(dp)) != NULL) {
	  if ( (strcmp(de->d_name, ".") != 0) && (strcmp(de->d_name, "..") != 0)) {
	    (void)sprintf(CheckPath, "%s/%s", SearchDir, de->d_name);
	    dbgprint(F_COLLGETLIST, (stderr, "Checking %s ..", CheckPath));
	    if ((stat(CheckPath, &stb) == 0) && ((stb.st_mode & S_IFMT) == S_IFDIR)) {
	      colname = (char *)emalloc(strlen(de->d_name)+1);
	      (void)strcpy(colname, de->d_name);
	      if ((cp = strrchr(colname, Depot_VersionDelimiter)) != NULL)
		*cp = '\0';
	      dbgprint(F_COLLGETLIST, (stderr, "succeeds.\nFound collection name %s under %s\n", colname, SearchDir));
	      /* add colname to list of possible collectionnames */
	      cnamelist = sortedstrarrinsert(cnamelist, colname);
	      free(colname);
	    }
	    else
	      { dbgprint(F_COLLGETLIST, (stderr, "fails.\n")); }
	  }
	}
      }
      (void)closedir(dp);
    }

  /* use this namelist on CollectionList_GetCollectionList */
  clist = CollectionList_GetCollectionList(cnamelist);

  dbgprint(F_TRACE, (stderr, "CollectionList_GetAllCollections done\n"));
  return clist;
}


COLLECTIONLIST *CollectionList_GetCollectionList(namelist)
     char **namelist;
{
  register char **colnamep;
  COLLECTIONLIST *clist;
  char **ColSearchPath, *ColPathDir;
  COLLECTION thiscol;
  char colpath[MAXPATHLEN];
  struct stat stb;

  dbgprint(F_TRACE, (stderr, "CollectionList_GetCollectionList - being implemented\n"));

  clist = NULL;

  if (namelist != NULL)
    {
      /* foreach collectionname in namelist */
      for ( colnamep = namelist; *colnamep != NULL; colnamep++)
	{
	  dbgprint(F_COLLGETLIST, (stderr, "Validating collection %s\n", *colnamep));
	  if (Ignore(*colnamep, preference))
	    { dbgprint(F_COLLGETLIST, (stderr, "Collection %s is to be ignored\n", *colnamep)); }
	  else
	    {
	      ColPathDir = Preference_GetString(preference, *colnamep, "path");
	      if (ColPathDir != NULL)
		{
		  if (*ColPathDir == '/')
		    (void)strcpy(colpath, ColPathDir);
		  else
		    (void)sprintf(colpath, "%s/%s", Depot_TargetPath, ColPathDir);
		  if ((stat(colpath, &stb) == 0) && ((stb.st_mode & S_IFMT) == S_IFDIR))
		    {
		      thiscol.name = *colnamep; thiscol.info = NULL; thiscol.path = ColPathDir;
		      dbgprint(F_COLLGETLIST, (stderr, "%s validated under %s\n", *colnamep, ColPathDir));
#ifdef USE_FSINFO
		      thiscol.fslist = NULL; thiscol.confinfo = NULL;
#endif USE_FSINFO
		      clist = CollectionList_AddCollection(clist, &thiscol);
		    } else {
		      WarningError((stderr, "depot: WARNING: Collection %s has an invalid path: %s\n", *colnamep, ColPathDir));
		    }
		}
	      else
		{
		  /* get the search path for collectionname, if any */
		  ColSearchPath = Preference_GetStringArray(preference, *colnamep, "searchpath");
		  /* if no such specific searchpath, use hardwired searchpath */
		  if (ColSearchPath == NULL)
		    {
		      dbgprint(F_COLLGETLIST, (stderr, "No searchpath, trying under path %s for collection %s\n", DEFAULTCOLLECTIONPATH, *colnamep));
		      ColSearchPath = sortedstrarrinsert(ColSearchPath, DEFAULTCOLLECTIONPATH);
		    }
		  /* try to locate collection */
		  ColPathDir = Collection_LocatePath(*colnamep, ColSearchPath);
		  /* if collection was located add it to clist */
		  if (ColPathDir == NULL)
		    {
		      dbgprint(F_COLLGETLIST, (stderr, "%s is not a valid collection\n", *colnamep));
		      WarningError((stderr, "WARNING: Could not locate path to collection named %s\n", *colnamep));
		    }
		  else
		    {
		      thiscol.name = *colnamep; thiscol.info = NULL;
		      (void)strcpy(colpath, ColPathDir);
		      thiscol.path = colpath;
		      dbgprint(F_COLLGETLIST, (stderr, "%s validated as %s\n", *colnamep, ColPathDir));
#ifdef USE_FSINFO
		      thiscol.fslist = NULL; thiscol.confinfo = NULL;
#endif USE_FSINFO
		      clist = CollectionList_AddCollection(clist, &thiscol);
		    }
		}
	    }
	}
    }
  dbgprint(F_TRACE, (stderr, "CollectionList_GetCollectionList done\n"));
  return clist;
}


static COLLECTIONLIST *CollectionList_AddCollection(list, item)
     COLLECTIONLIST *list;
     COLLECTION *item;
{
  Boolean DuplicateItem;
  COLLECTIONLIST *cp, *new, *newlist;

  dbgprint(F_TRACE, (stderr, "CollectionList_AddCollection\n"));

  dbgprint(F_COLLISTADD, (stderr, "CollectionList_AddCollection: adding %s to list\n", item->name));

  cp = list; DuplicateItem = FALSE;
  while (!DuplicateItem && (cp != NULL))
    {
      if (strcmp(item->name, cp->collection.name) == 0)
	DuplicateItem = TRUE;
      else
	cp = COLLECTION_next(cp);
    }
  if (DuplicateItem)
    { newlist = list; }
  else /* !DuplicateItem */
    {
      new = (COLLECTIONLIST *)emalloc(sizeof(COLLECTIONLIST));
      new->collection.name = emalloc(strlen(item->name)+1);
      new->collection.name = strcpy(new->collection.name, item->name);
      new->collection.path = emalloc(strlen(item->path)+1);
      new->collection.path = strcpy(new->collection.path, item->path);
      new->collection.info = item->info;
#ifdef USE_FSINFO
      new->collection.fslist = NULL; new->collection.confinfo = NULL;
#endif USE_FSINFO
      COLLECTION_next(new) = list;
      newlist = new;
    }
  dbgDumpCollectionList(newlist);

  dbgprint(F_TRACE, (stderr, "CollectionList_AddCollection done\n"));
  return newlist;
}



/*
 * COLLECTIONLIST *CollectionList_GetAllKnownCollections(db)
 */
COLLECTIONLIST *CollectionList_GetAllKnownCollections(db)
     DEPOTDB *db;
{
  register SOURCE *sp;
  register u_short i;
  COLLECTIONLIST *clist;
  COLLECTION thiscol;

  dbgprint(F_TRACE, (stderr, "CollectionList_GetAllKnownCollections\n"));

  clist = NULL;
  if (db != NULL)
    {
      for ( i = 0, sp = db->sourcelist; i < db->nsources; i++, sp++ )
	{
	  if (Ignore(sp->collection_name, preference))
	    { dbgprint(F_COLLGETLIST, (stderr, "Collection %s is to be ignored\n", sp->collection_name)); }
	  else if (*sp->collection_name != '\0')
	    {
	      thiscol.name = sp->collection_name; thiscol.path = sp->name; thiscol.info = NULL;
	      dbgprint(F_COLLGETLIST, (stderr, "Found known collection %s\n", sp->collection_name));
	      clist = CollectionList_AddCollection(clist, &thiscol);
	    }
	}
      dbgDumpCollectionList(clist);
    }

  dbgprint(F_TRACE, (stderr, "CollectionList_GetAllKnownCollections done\n"));

  return clist;
}



/*
 * COLLECTIONLIST *CollectionList_GetKnownCollections(db, namelist)
 */
COLLECTIONLIST *CollectionList_GetKnownCollections(db, namelist)
     DEPOTDB *db;
     char **namelist;
{
  register SOURCE *sp;
  register u_short i;
  register char **np;
  COLLECTIONLIST *clist;
  COLLECTION thiscol;
  Boolean FoundCollection;

  dbgprint(F_TRACE, (stderr, "CollectionList_GetKnownCollections\n"));

  clist = NULL;
  if (namelist != NULL)
    {
      for ( np = namelist; *np != NULL; np++)
	{
	  if (Ignore(*np, preference))
	    { dbgprint(F_COLLGETLIST, (stderr, "Collection %s is to be ignored\n", *np)); }
	  else if (strcmp(*np, "") != 0)
	    {
	      FoundCollection = FALSE; i = 0; sp = db->sourcelist;
	      while ( !FoundCollection && (i < db->nsources))
		{
		  if (strcmp(sp->collection_name, *np) == 0)
		    {
		      FoundCollection = TRUE;
		      thiscol.name = sp->collection_name; thiscol.path = sp->name; thiscol.info = NULL;
		      dbgprint(F_COLLGETLIST, (stderr, "Found known collection %s\n", sp->collection_name));
#ifdef USE_FSINFO
		      thiscol.fslist = NULL; thiscol.confinfo = NULL;
#endif USE_FSINFO
		      clist = CollectionList_AddCollection(clist, &thiscol);
		    }
		  i++; sp++;
		}
	      if (!FoundCollection)
		{ WarningError((stderr, "Warning: collection %s is not known by the database\n", *np)); }
	    }
	}
    }
  dbgDumpCollectionList(clist);

  dbgprint(F_TRACE, (stderr, "CollectionList_GetKnownCollections done\n"));

  return clist;
}


void
CollectionList_FreeList(list)
     COLLECTIONLIST *list;
{
  register COLLECTIONLIST *cp;
  
  for (cp = list; cp; cp = COLLECTION_next(cp)) {
    free(cp->collection.name);
    free(cp->collection.path);
    /* this may not free everything but it is a start */
  }
}

#ifdef DEBUG

static void dbgDumpCollectionList(list)
     COLLECTIONLIST *list;
{
  register COLLECTIONLIST *cp;

  dbgprint(F_COLLISTADD, (stderr, "Collection list now has the following collections:\n"));
  for (cp = list; cp != NULL; cp = COLLECTION_next(cp))
    {
      dbgprint(F_COLLISTADD, (stderr, "\t%s\n", cp->collection.name));
    }
}

#else DEBUG

static void dbgDumpCollectionList(list)
     COLLECTIONLIST *list;
{
}

#endif DEBUG

/* $Source: /afs/andrew.cmu.edu/system/src/local/depot/019/RCS/CollectionList.c,v $ */
