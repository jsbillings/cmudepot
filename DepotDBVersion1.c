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


static char rcs_id[]="$Header: /afs/andrew.cmu.edu/system/src/local/depot/019/RCS/DepotDBVersion1.c,v 4.25 1992/07/30 22:22:45 sohan Exp $";


/*
 * Author: Sohan C. Ramakrishna Pillai
 */


#include <stdio.h>
#include <sys/param.h>
#include <values.h>

#include "globals.h"
#include "depot.h"
#include "DepotDBStruct.h"
#include "DepotDBCommandStruct.h"
#include "DepotDBVersion1.h"
#include "DepotDB.h"
#include "PreferenceStruct.h"
#include "Preference.h"

#ifdef ibm032
#define BITSPERBYTE 8
#endif /* ibm032 */

extern Boolean CheckSourceConsistency();
extern Boolean Check_Noop();
extern Boolean Check_Copy();
extern Boolean Check_Link();
extern Boolean Check_Map();
extern Boolean Check_MakeDir();
extern void Check_ExecRCFile();
extern Boolean Update_Noop();
extern Boolean Update_Copy();
extern Boolean Update_Link();
extern Boolean Update_Map();
extern Boolean Update_MakeDir();
extern void Update_ExecRCFile();


static void	DepotDB1_Tree_Write();
static DEPOTDB	*DepotDB1_Tree_Append();
static Boolean	DepotDB1_Tree_Apply();
static DEPOTDB	*DepotDB1_Tree_RemoveCollection();
static void	DepotDB1_Tree_ObsoleteCollection();
static DEPOTDB	*DepotDB1_Tree_PruneCollection();
static void	DepotDB1_Tree_Antiquate();
static void	DepotDB1_Tree_SetTargetMappings();
static void	DepotDB1_Entry_SetTargetMappings();
static void	DepotDB1_SetTargetMappingPath();
static ENTRY	*DepotDB1_AllocEntry();
static void	DepotDB1_Entry_RemoveCollection();
static void	DepotDB1_Entry_PruneCollection();
static Boolean	DepotDB1_Entry_VirginityStatus();
static void	DepotDB1_Free_Subtree();
static void	DepotDB1_QuotedPrintString();
static void	dbgDumpSourcelist();



DEPOTDB *DepotDB1_Create()
{
  DEPOTDB *db;
  ENTRY *ep;
  SOURCE thissrc;

  dbgprint(F_TRACE, (stderr, "DepotDB1_Create\n"));

  ep = DepotDB1_AllocEntry();
  ENTRY_sibling(ep) = NULL; ENTRY_child(ep) = NULL;
  ep->name = (char *)emalloc(2);
  strcpy(ep->name, "/");
  ep->nsources = 0; ep->sourcelist = NULL;
  ep->status = 0;
  thissrc.name = (char *)emalloc(2);
  strcpy(thissrc.name, "/"); 
  thissrc.collection_name = "";
  thissrc.update_spec = U_MKDIR; 
  thissrc.old_update_spec = 0;  /* XXX ask sohan */
  thissrc.status = S_NONVIRGINSRC;

#ifdef USE_FSINFO
  thissrc.fs_status = FS_UNKNOWN;
#endif USE_FSINFO
  DepotDB1_SourceList_AddSource(ep, &thissrc);

  db = ep;
  dbgprint(F_TRACE, (stderr, "DepotDB1_Create done\n"));
  return db;
}


int DepotDB1_GetVersionNumber(dbfile)
     FILE *dbfile;
{
  register char *cp;
  register int ch;
  char VersionString[MAXPATHLEN];
  int VersionNumber;

  dbgprint(F_TRACE, (stderr, "DepotDB1_GetVersionNumber\n"));
  cp = VersionString; ch = getc(dbfile);
  while ((ch != EOF) && (ch != '\n'))
    { *cp++ = (char)ch; ch = getc(dbfile); }
  *cp = '\0';

  if (strncmp(VersionString, "VERSION ", 8) == 0)
    VersionNumber = atoi(VersionString+8);
  else
    VersionNumber = -1;
  dbgprint(F_DBREAD, (stderr, "VERSION %d\n", VersionNumber));

  dbgprint(F_TRACE, (stderr, "DepotDB1_GetVersionNumber done\n"));
  return VersionNumber;
}


/*
 * DEPOTDB *DepotDB1_Read(dbfile, PreferencesSavedp)
 *	reads in the database from the dbfile and returns it.
 *	As a side-effect, it also reads in any important preference values saved
 *	in the db file during the last run of depot.
 */
DEPOTDB *DepotDB1_Read(dbfile, PreferencesSavedp)
     FILE *dbfile;
     PREFERENCELIST **PreferencesSavedp;
{
  DEPOTDB *db_root;
  Boolean EmptyDBFile;
  register int ch;
  register char *cp, *cp2;
  Boolean eodb, eofname, eostr, eoline;
  unsigned lineno;
  ENTRY thisentry, direntry;
  SOURCE thissrc;
  Boolean QuotedChar, PreferenceEntry;
  char filename[MAXPATHLEN], sourcename[MAXPATHLEN], collname[MAXPATHLEN], prefval[PREFERENCELINESIZE];
  char *prefitem, *prefname;
#ifdef USE_FSINFO
  char longstr[(BITSPERBYTE * 10 * sizeof(long))/3 + 2];
  /*
   * (BITSPERBYTE * 10 * sizeof(long))/3 = BITSPERBYTE*sizeof(long)/0.3
   *	> logBITS(BYTE)*logBYTE(long)/logBITS(10) = log10(long)
   */
#endif USE_FSINFO

  dbgprint(F_TRACE, (stderr, "DepotDB1_Read\n"));
  dbgprint(F_CHECK, (stderr, "DepotDB1_Read does not handle ill-written db files very well\n"));

  /* initialize database */
  db_root = DepotDB1_Create();
  db_root->sourcelist->status |= S_OBSOLETE;
  *PreferencesSavedp = NULL;

  /* set up direntry to handle intermediate directories */
  direntry.name = filename;
  /* set up thisentry to handle final filename */
  thisentry.name = filename;

  /*  specified in current dbfile format */
  eodb = FALSE; lineno = 1; EmptyDBFile = TRUE;
  while (!eodb)
    {
      direntry.status = 0; thisentry.status = 0; thissrc.status = S_ANTIQUE;
#ifdef USE_FSINFO
      thissrc.fs_status = FS_UNKNOWN;
#endif USE_FSINFO
      PreferenceEntry = FALSE;

      if ( (ch = getc(dbfile)) == EOF)
	{
	  eodb = TRUE;
	  dbgprint(F_DBREAD, (stderr, "ENDOF DATABASE FILE\n"));
	}
      else
	{
	  EmptyDBFile = FALSE;
	  /* get update specification */
	  switch (ch)
	    {
	    case 'L':
	      thissrc.update_spec = U_MAP;
	      dbgprint(F_DBREAD, (stderr, "MAP\n"));
	      break;
	    case 'F':
	      thissrc.update_spec = U_NOOP;
	      dbgprint(F_DBREAD, (stderr, "NOOP\n"));
	      break;
	    case 'D':
	      thissrc.update_spec = U_MKDIR;
#ifdef USE_FSINFO
	      thissrc.fs_status = FS_INHERIT;
#endif USE_FSINFO
	      dbgprint(F_DBREAD, (stderr, "MKDIR\n"));
	      break;
	    case 'd':
	      thissrc.update_spec = U_MKDIR; thissrc.status |= S_NONVIRGINSRC;
#ifdef USE_FSINFO
	      thissrc.fs_status = FS_INHERIT;
#endif USE_FSINFO
	      dbgprint(F_DBREAD, (stderr, "MKDIR\n"));
	      break;
#ifdef USE_FSINFO
	    case 'M':
	      thissrc.update_spec = U_MKDIR;
	      thissrc.fs_status = FS_NEWFILESYSTEM;
	      dbgprint(F_DBREAD, (stderr, "MKDIR mountpoint\n"));
	      break;
	    case 'm':
	      thissrc.update_spec = U_MKDIR; thissrc.status |= S_NONVIRGINSRC;
	      thissrc.fs_status = FS_NEWFILESYSTEM;
	      dbgprint(F_DBREAD, (stderr, "MKDIR mountpoint\n"));
	      break;
#endif USE_FSINFO
	    case 'R':
	      thissrc.update_spec = U_RCFILE;
	      dbgprint(F_DBREAD, (stderr, "RCFILE\n"));
	      break;
	    case 'P':
	      PreferenceEntry = TRUE;
	      dbgprint(F_DBREAD, (stderr, "PREFERENCE\n"));
	      break;
	    default:
	      dbgprint(F_DBREAD, (stderr, "BAD UPDATE SPECIFICATION\n"));
	      FatalError(E_BADUPDTFLAG, (stderr, "Unknown update specification %c in dbfile, line %u\n", (char)ch, lineno));
	      break;
	    }
	  ch = getc(dbfile);
	  while ((ch != ' ') && (ch != '\t'))
	    {
	      switch(ch)
		{
		case 'N':
		  thissrc.status &= ~S_ANTIQUE;
		  break;
		case 'T':
		  thissrc.status |= S_TARGET;
		  break;
		case 'I':
		  thissrc.status |= S_IMMACULATE;
		  break;
		case 'F':
		  thissrc.update_spec |= U_NOOP;
		  break;
		case 'X':
		  thissrc.update_spec |= U_DELETE;
		  break;
		case 'L':
		  thissrc.update_spec |= U_MAPLINK;
		  break;
		case 'C':
		  thissrc.update_spec |= U_MAPCOPY;
		  break;
		default:
		  dbgprint(F_DBREAD, (stderr, "BAD STATUS SPECIFICATION\n"));
		  FatalError(E_BADUPDTFLAG, (stderr, "Unknown status specification %c in dbfile, line %u\n", (char)ch, lineno));
		}
	      ch = getc(dbfile);
	    }

	  /* skip whitespaces */
	  while ((ch == ' ') || (ch == '\t'))
	    ch = getc(dbfile);

	  if (PreferenceEntry)
	    {
	      eoline = FALSE;
	      /* get prefitem */
	      prefitem = collname; cp = collname;  *cp++ = (char)ch; ch = getc(dbfile); eostr = FALSE; QuotedChar = FALSE;
	      while (!eostr)
		{
		  if (((ch == ' ') || (ch == '\t') || (ch == '\n')) && !QuotedChar)
		    eostr = TRUE;
		  else
		    {
		      if (!QuotedChar && (ch == DepotDB_QUOTCHAR))
			{ QuotedChar = TRUE; }
		      else
			{ QuotedChar = FALSE; *cp++ = (char)ch;}
		      ch = getc(dbfile);
		    }
		}
	      *cp++ = '\0';
	      dbgprint(F_DBREAD, (stderr, "ITEM %s\n", prefitem));

	      /* get prefname */
	      while ((ch == ' ') || (ch == '\t')) /* skip whitespaces */
		ch = getc(dbfile);
	      if (ch == '\n') {eoline = TRUE; lineno++;}
	      if (eoline)
		{ FatalError(E_INCOMPLETEPREFINFO, (stderr, "Incomplete preference info found for item %s, line %u\n", prefname, lineno)); }
	      prefname = sourcename; cp = sourcename; *cp++ = (char)ch; ch = getc(dbfile); eostr = FALSE; QuotedChar = FALSE;
	      while (!eostr)
		{
		  if (((ch == ' ') || (ch == '\t') || (ch == '\n')) && !QuotedChar)
		    eostr = TRUE;
		  else
		    {
		      if (!QuotedChar && (ch == DepotDB_QUOTCHAR))
			{ QuotedChar = TRUE; }
		      else
			{ QuotedChar = FALSE; *cp++ = (char)ch;}
		      ch = getc(dbfile);
		    }
		}
	      *cp++ = '\0';
	      dbgprint(F_DBREAD, (stderr, "PREFNAME %s\n", prefname));

	      /* get prefval */
	      while ((ch == ' ') || (ch == '\t')) /* skip whitespaces */
		ch = getc(dbfile);
	      if (ch == '\n') {eoline = TRUE; lineno++;}
	      if (eoline)
		{ FatalError(E_INCOMPLETEPREFINFO, (stderr, "Incomplete preference info found for item %s, prefname %s, line %u\n", prefname, prefval, lineno)); }
	      cp = prefval; *cp++ = (char)ch; ch = getc(dbfile); eostr = FALSE; QuotedChar = FALSE;
	      while (!eostr)
		{
		  if (((ch == ' ') || (ch == '\t') || (ch == '\n')) && !QuotedChar)
		    eostr = TRUE;
		  else
		    {
		      if (!QuotedChar && (ch == DepotDB_QUOTCHAR))
			{ QuotedChar = TRUE; }
		      else
			{ QuotedChar = FALSE; *cp++ = (char)ch;}
		      ch = getc(dbfile);
		    }
		}
	      *cp++ = '\0';
	      dbgprint(F_DBREAD, (stderr, "PREFVAL %s\n", prefval));
	      /* assume only string valued preferences now -- Sohan */
	      *PreferencesSavedp = Preference_SetString(*PreferencesSavedp, prefitem, prefname, prefval);
	    }
	  else /* !PreferenceEntry => regular entry */
	    {
	      /* get file/dir name */
	      direntry.nsources = 0; direntry.sourcelist = NULL;
	      cp = filename; *cp++ = (char)ch; eofname = FALSE;
	      while (!eofname)
		{
		  if (ch == '/')
		    {
		      *cp = '\0';
		      if (strcmp(filename, "/") != 0) /* not entry for root "/" */
			*(cp-1)='\0'; /* remove trailing / symbol */
		      dbgprint(F_DBREAD, (stderr, "DIRENTRY %s\n", filename));
		      db_root = DepotDB1_UpdateEntry(db_root, &direntry);
		      if (strcmp(filename, "/") != 0) /* not entry for root "/" */
			*(cp-1)='/'; /* replace trailing / symbol */
		      DepotDB1_FreeSourceList(&direntry);
		      direntry.sourcelist = NULL; direntry.nsources = 0;
		    }
		  ch = getc(dbfile); QuotedChar = FALSE;
		  while (((ch != ' ') && (ch != '\t') && (ch != '\n') && (ch != '/'))
			 || QuotedChar)
		    {
		      if (!QuotedChar && (ch == DepotDB_QUOTCHAR))
			{ QuotedChar = TRUE; }
		      else
			{ QuotedChar = FALSE; *cp++ = (char)ch; }
		      ch = getc(dbfile);
		    }
		  if (ch == '/') *cp++ = (char)ch;
		  else /*(ch == ' ')||(ch == '\t')||(ch == '\n')*/ eofname = TRUE;
		}
	      *cp = '\0';
	      dbgprint(F_DBREAD, (stderr, "TARGETENTRY %s\n", filename));

	      /* get list of source files */
	      eoline = FALSE;
	      thisentry.nsources = 0; thisentry.sourcelist = NULL;
	      while (!eoline)
		{
		  while ((ch == ' ') || (ch == '\t')) /* skip whitespaces */
		    ch = getc(dbfile);
		  if (ch == '\n') {eoline = TRUE; lineno++;}
		  else /* get file/dir name and source collection name, if any */
		    {
		      cp = sourcename; *cp++ = (char)ch; ch = getc(dbfile); eofname = FALSE; QuotedChar = FALSE;
		      while (!eofname)
			{
			  if (((ch == ' ') || (ch == '\t') || (ch == '\n')) && !QuotedChar)
			    eofname = TRUE;
			  else
			    {
			      if (!QuotedChar && (ch == DepotDB_QUOTCHAR))
				{ QuotedChar = TRUE; }
			      else
				{ QuotedChar = FALSE; *cp++ = (char)ch; }
			      ch = getc(dbfile);
			    }
			}
		      /* eofname is TRUE */
		      *cp++ = '\0';
		      dbgprint(F_DBREAD, (stderr, "SOURCE %s\n", sourcename));
		      while ((ch == ' ') || (ch == '\t')) /* skip whitespaces */
			ch = getc(dbfile);
		      if (ch == '\n') {eoline = TRUE; lineno++;}
		      if (eoline)
			{
			  /* unreferenced entry */
			  cp2 = collname; *cp2 = '\0';
			  dbgprint(F_DBREAD, (stderr, "COLLECTION %s\n", collname));
			}
		      else
			{
			  if (strcmp(sourcename, ".") == 0)
			    /* unreferenced target collection root */
			    { collname[0] = '\0'; } /* collname = "" */
			  else
			    {
			      /* get collection name */
			      cp = collname; *cp++ = (char)ch; ch = getc(dbfile); eofname = FALSE; QuotedChar = FALSE;
			      while (!eofname)
				{
				  if (((ch == ' ') || (ch == '\t') || (ch == '\n')) && !QuotedChar)
				    eofname = TRUE;
				  else
				    {
				      if (!QuotedChar && (ch == DepotDB_QUOTCHAR))
					{ QuotedChar = TRUE; }
				      else
					{ QuotedChar = FALSE; *cp++ = (char)ch;}
				      ch = getc(dbfile);
				    }
				}
			      *cp++ = '\0';
			    }
			  dbgprint(F_DBREAD, (stderr, "COLLECTION %s\n", collname));
#ifdef USE_FSINFO
			  if ((thissrc.update_spec & U_MKDIR) && (thissrc.fs_status & FS_NEWFILESYSTEM))
			    {
			      while ((ch == ' ') || (ch == '\t')) /* skip whitespaces */
				ch = getc(dbfile);
			      if (ch == '\n') {eoline = TRUE; lineno++;}
			      if (eoline)
				{ FatalError(E_NOFSINFO, (stderr, "No filesystem info found for mountpoint %s in collection %s, line %u\n", sourcename, collname, lineno-1)); }
			      else
				{
				  /* get fs id (volume id.) */
				  cp = longstr; *cp++ = (char)ch; ch = getc(dbfile); eostr = FALSE;
				  while (!eostr)
				    {
				      if ((ch == ' ') || (ch == '\t') || (ch == '\n'))
					eostr = TRUE;
				      else
					{
					  *cp++ = (char)ch; ch = getc(dbfile);
					}
				    }
				  *cp++ = '\0';
				  dbgprint(F_DBREAD, (stderr, "MOUNTPOINT ID %s\n", longstr));
				  thissrc.fs_id = atol(longstr);
				  
				  while ((ch == ' ') || (ch == '\t')) /* skip whitespaces */
				    ch = getc(dbfile);
				  if (ch == '\n') {eoline = TRUE; lineno++;}
				  if (eoline)
				    { FatalError(E_INCOMPLETEFSINFO, (stderr, "Incomplete filesystem info found for mountpoint %s in collection %s, line %u\n", sourcename, collname, lineno-1)); }
				  else
				    {
				      /* get fs modtime (volume modtime) */
				      cp = longstr; *cp++ = (char)ch; ch = getc(dbfile); eostr = FALSE;
				      while (!eostr)
					{
					  if ((ch == ' ') || (ch == '\t') || (ch == '\n'))
					    eostr = TRUE;
					  else
					    {
					      *cp++ = (char)ch; ch = getc(dbfile);
					    }
					}
				      *cp++ = '\0';
				      dbgprint(F_DBREAD, (stderr, "MOUNTPOINT MODTIME %s\n", longstr));
				      thissrc.fs_modtime = (u_long)atol(longstr);
				      
				      while ((ch == ' ') || (ch == '\t')) /* skip whitespaces */
					ch = getc(dbfile);
				      if (ch == '\n') {eoline = TRUE; lineno++;}
				      if (eoline)
					{ FatalError(E_INCOMPLETEFSINFO, (stderr, "Incomplete filesystem info found for mountpoint %s in collection %s, line %u\n", sourcename, collname, lineno-1)); }
				      else
					{
					  /* get collection id */
					  cp = longstr; *cp++ = (char)ch; ch = getc(dbfile); eostr = FALSE;
					  while (!eostr)
					    {
					      if ((ch == ' ') || (ch == '\t') || (ch == '\n'))
						eostr = TRUE;
					      else
						{
						  *cp++ = (char)ch; ch = getc(dbfile);
						}
					    }
					  *cp++ = '\0';
					  dbgprint(F_DBREAD, (stderr, "COLECTION ID %s\n", longstr));
					  thissrc.col_id = atol(longstr);
					  
					  while ((ch == ' ') || (ch == '\t')) /* skip whitespaces */
					    ch = getc(dbfile);
					  if (ch == '\n') {eoline = TRUE; lineno++;}
					  if (eoline)
					    { FatalError(E_INCOMPLETEFSINFO, (stderr, "Incomplete filesystem info found for mountpoint %s in collection %s, line %u\n", sourcename, collname, lineno-1)); }
					  else
					    {
					      /* get collection conftime */
					      cp = longstr; *cp++ = (char)ch; ch = getc(dbfile); eostr = FALSE;
					      while (!eostr)
						{
						  if ((ch == ' ') || (ch == '\t') || (ch == '\n'))
						    eostr = TRUE;
						  else
						    {
						      *cp++ = (char)ch; ch = getc(dbfile);
						    }
						}
					      *cp++ = '\0';
					      dbgprint(F_DBREAD, (stderr, "COLECTION CONFTIME %s\n", longstr));
					      thissrc.col_conftime = atol(longstr);
					    }
					}
				    }
				}
			    }
#endif USE_FSINFO
			}
		      /* add source info to source list for thisentry */
		      thissrc.old_update_spec = thissrc.update_spec;
		      thissrc.name = sourcename; thissrc.collection_name = collname;
		      DepotDB1_SourceList_AddSource(&thisentry, &thissrc);
		    }
		}
	      /*
	       * eoline is TRUE; thisentry should be added to the loaded database
	       * the top level entry for unreferenced entries will have a NULL source;
	       * kludge it before adding this entry to database
	       */
	      if ((thisentry.nsources == 0) && (thisentry.sourcelist == NULL))
		{
		  thissrc.old_update_spec = thissrc.update_spec;
		  thissrc.name = ""; thissrc.collection_name = ""; 
		  DepotDB1_SourceList_AddSource(&thisentry, &thissrc);
		}
	      db_root = DepotDB1_UpdateEntry(db_root, &thisentry);
	      DepotDB1_FreeSourceList(&thisentry);
	      thisentry.sourcelist = NULL; thisentry.nsources = 0;
	    }
	  /* !PreferenceEntry => regular entry done */
	}
    }
  /* eodb is TRUE */

  if (EmptyDBFile)
    { FatalError(E_EMPTYDBFILE, (stderr, "Database file %s/%s/%s has been corrupted, please rebuild it.", Depot_TargetPath, DEPOTSPECIALDIRECTORY, DEPOTDBFILE)); }

  dbgprint(F_TRACE, (stderr, "DepotDB1_Read done\n"));
  return db_root;
}

Boolean DepotDB1_Write(dbfile, db, preference)
     FILE *dbfile;
     DEPOTDB *db;
     PREFERENCELIST *preference;
{
  register PREFERENCELIST *pp;

  dbgprint(F_TRACE, (stderr, "DepotDB1_Write\n"));

  fprintf(dbfile, "VERSION %d\n", DEPOTDB_VERSION);

  DepotDB1_Tree_Write(dbfile, db, "", S_NONVIRGINTRG);

  for (pp = preference; pp != NULL; pp = PREFERENCELIST_next(pp))
    {
      /* we save only command and mapcommand preferences now. */
      if ((strcmp(pp->preference.PreferenceName, "command") == 0)
	  || (strcmp(pp->preference.PreferenceName, "mapcommand") == 0))
	{
	  fputc('P',dbfile);
	  fputc('\t',dbfile); DepotDB1_QuotedPrintString(dbfile, pp->preference.ItemName);
	  fputc('\t',dbfile); DepotDB1_QuotedPrintString(dbfile, pp->preference.PreferenceName);
	  fputc('\t',dbfile); DepotDB1_QuotedPrintString(dbfile, pp->preference.value.strval);
	  /* write a carriage return to end this entry */
	  fputc('\n',dbfile);
	}
    }

  dbgprint(F_TRACE, (stderr, "DepotDB1_Write done\n"));
  return TRUE;
}


DEPOTDB *DepotDB1_Append(db, deltadb)
     DEPOTDB *db, *deltadb;
{
  DEPOTDB *newdb;

  dbgprint(F_TRACE, (stderr, "DepotDB1_Append\n"));

  newdb = db;
  newdb = DepotDB1_Tree_Append(newdb, deltadb, "");
  dbgprint(F_TRACE, (stderr, "DepotDB1_Append done\n"));

  return newdb;
}


Boolean DepotDB1_Apply(db_root, applspec, commandfilelist)
     DEPOTDB *db_root;
     unsigned applspec;
     COMMANDFILE ***commandfilelist;
{
  register COMMANDFILE **fp;

  dbgprint(F_TRACE, (stderr, "DepotDB1_Apply\n"));

  (void)DepotDB1_Tree_Apply(db_root, Depot_TargetPath, 0, applspec, commandfilelist);

  if (*commandfilelist != NULL)
    {
      for (fp = *commandfilelist; *fp != NULL; fp++)
	{
	  if (applspec & APPL_CHECK) Check_ExecRCFile(db_root, *fp);
	  else if (applspec & APPL_UPDATE) Update_ExecRCFile(*fp);
	}
    }

  dbgprint(F_TRACE, (stderr, "DepotDB1_Apply done\n"));
  return TRUE;
}


DEPOTDB *DepotDB1_RemoveCollection(db, colname)
     DEPOTDB *db;
     char *colname;
{
  DEPOTDB *newdb;
  Boolean PlaceHolder;

  dbgprint(F_TRACE, (stderr, "DepotDB1_RemoveCollection\n"));

  newdb = db;
  newdb = DepotDB1_Tree_RemoveCollection(newdb, colname, &PlaceHolder);
  dbgprint(F_TRACE, (stderr, "DepotDB1_RemoveCollection done\n"));

  return newdb;
}


DEPOTDB *DepotDB1_ObsoleteCollection(db, colname)
     DEPOTDB *db;
     char *colname;
{
  dbgprint(F_TRACE, (stderr, "DepotDB1_ObsoleteCollection\n"));

  DepotDB1_Tree_ObsoleteCollection(db, colname);
  dbgprint(F_TRACE, (stderr, "DepotDB1_ObsoleteCollection done\n"));

  return db;
}


DEPOTDB *DepotDB1_PruneCollection(db, colname)
     DEPOTDB *db;
     char *colname;
{
  DEPOTDB *newdb;
  Boolean PlaceHolder;

  dbgprint(F_TRACE, (stderr, "DepotDB1_PruneCollection\n"));

  newdb = db;
  newdb = DepotDB1_Tree_PruneCollection(newdb, colname, &PlaceHolder);
  dbgprint(F_TRACE, (stderr, "DepotDB1_PruneCollection done\n"));

  return newdb;
}


void DepotDB1_Antiquate(db)
     DEPOTDB *db;
{
  Boolean PlaceHolder;
  dbgprint(F_TRACE, (stderr, "DepotDB1_Antiquate\n"));
  DepotDB1_Tree_Antiquate(db, &PlaceHolder);
  dbgprint(F_TRACE, (stderr, "DepotDB1_Antiquate done\n"));
}


/*
 * static void DepotDB1_Tree_Write(dbfile, db, pathprefix, flags)
 * go through the db (sub)tree rooted at db
 * representing files prefixed with pathprefix,
 * recursively in pre-order fashion
 * while writing out entries into dbfile.
 * flags is used to indicate whether the entries are being mapped
 * or have been pre-empted by a link on a virgin parent.
 */
static void DepotDB1_Tree_Write(dbfile, db, pathprefix, flags)
     FILE *dbfile;
     DEPOTDB *db;
     char *pathprefix;
     Boolean flags;
{
  register ENTRY *ep;
  char newpath[MAXPATHLEN];
  register u_short i;
  register SOURCE *sp, *TargetSourcep;
  Boolean VirginPreEmpt;
  Boolean newflags;

  dbgprint(F_TRACE, (stderr, "DepotDB1_Tree_Write\n"));

  if ((db == NULL) || (db->name == NULL)) /* null (sub)tree or initial dummy entry */
    {
      dbgprint(F_TRACE, (stderr, "DepotDB1_Tree_Write done\n"));
      return ;
    }

  for ( ep = db; ep != NULL; ep = ENTRY_sibling(ep) )
    {
      if (ep->name == NULL)
	{
	  FatalError(E_UNNAMEDENTRY, (stderr, "No name for entry in database tree\n"));
	}
      if ((pathprefix == NULL) || (strcmp(pathprefix,"") == 0)
	  || (strcmp(pathprefix, ".") == 0) || (strcmp(pathprefix, "/") == 0))
	(void)strcpy(newpath, ep->name);
      else
	(void)sprintf(newpath, "%s/%s", pathprefix, ep->name);

      if ((ep->nsources == 0) || (ep->sourcelist == NULL))
	{
	  FatalError(E_SRCLESSENTRY, (stderr, "target %s has no source\n", newpath));
	}

      VirginPreEmpt = DepotDB1_Entry_VirginityStatus(ep, FALSE);
      TargetSourcep = ep->sourcelist+(ep->nsources-1);
      for ( i = 0, sp = ep->sourcelist; i < ep->nsources; i++, sp++)
	{
	  if (sp->name == NULL)
	    {
	      FatalError(E_UNNAMEDSRC, (stderr, "source name missing in sourcelist\n"));
	    }
	  else
	    {
	      if (ep->status & S_SPECIAL) fputc('F',dbfile);
	      else
		{
		  /* write out info corresponding to source */
		  if (sp->update_spec & U_MAP)
		    {
		      fputc('L',dbfile);
		    }
		  else if (sp->update_spec & U_MKDIR)
		    {
#ifdef USE_FSINFO
		      if (sp->fs_status & FS_NEWFILESYSTEM)
			{
			  if (sp->status & S_NONVIRGINSRC) fputc('m',dbfile);
			  else fputc('M',dbfile);
			}
		      else
#endif USE_FSINFO
			{
			  if (sp->status & S_NONVIRGINSRC) fputc('d',dbfile);
			  else fputc('D',dbfile);
			}
		    }
		  else if (sp->update_spec & U_RCFILE) fputc('R',dbfile);
		  if (sp->update_spec & U_NOOP) fputc('F',dbfile);
		  if (sp->update_spec & U_DELETE) fputc('X',dbfile);
		  if (sp->update_spec & U_MAPLINK) fputc('L',dbfile);
		  if (sp->update_spec & U_MAPCOPY) fputc('C',dbfile);
		}
	      if (!(sp->status & S_ANTIQUE)) fputc('N',dbfile);

	      if (flags & S_NONVIRGINTRG) /* no pre-empt by a virgin parent */
		{
		  if (i == (ep->nsources-1))
		    { fputc('T',dbfile); }
		  else if (TargetSourcep->update_spec & U_MKDIR)
		    {
		      if ((VirginPreEmpt == FALSE)
			  && (sp->update_spec & U_MKDIR))
			fputc('T',dbfile);
		    }
		  else if (TargetSourcep->update_spec & U_RCFILE)
		    {
		      if ((sp->update_spec & U_RCFILE)
			  && (strcmp(sp->name, TargetSourcep->name) == 0))
			fputc('T',dbfile);
		    }
		}
	      else /* !(flags & S_NONVIRGINTRG) => pre-empt by virgin parent */
		{ fputc('I',dbfile); }

	      fputc('\t', dbfile);
	      DepotDB1_QuotedPrintString(dbfile, newpath);
	      fputc('\t', dbfile);
	      DepotDB1_QuotedPrintString(dbfile, sp->name);
	      fputc('\t',dbfile);
	      DepotDB1_QuotedPrintString(dbfile, sp->collection_name);
#ifdef USE_FSINFO
	      if ((sp->update_spec & U_MKDIR) && (sp->fs_status & FS_NEWFILESYSTEM))
		{
		  fprintf(dbfile, "\t%ld", sp->fs_id);
		  fprintf(dbfile, "\t%lu", sp->fs_modtime);
		  fprintf(dbfile, "\t%ld", sp->col_id);
		  fprintf(dbfile, "\t%lu", sp->col_conftime);
		}
#endif USE_FSINFO
	      /* write a carriage return to end this entry */
	      fputc('\n',dbfile);
	    }
	}

      /* recursively write children */
      if (   (flags & S_NONVIRGINTRG)
	  && (TargetSourcep->update_spec & U_MKDIR)
	  && (VirginPreEmpt == TRUE))
	newflags = (flags & ~S_NONVIRGINTRG);
      else
	newflags = flags;
      DepotDB1_Tree_Write(dbfile, ENTRY_child(ep), newpath, newflags);
    }
  dbgprint(F_TRACE, (stderr, "DepotDB1_Tree_Write done\n"));
  return;
}




/*
 * static void DepotDB1_Tree_Append(db, deltadb, pathprefix)
 * go through the db (sub)tree rooted at deltadb
 * representing files prefixed with pathprefix,
 * recursively in pre-order fashion
 * while appending entries to the database db
 */
static DEPOTDB *DepotDB1_Tree_Append(db, deltadb, pathprefix)
     DEPOTDB *db, *deltadb;
     char *pathprefix;
{
  DEPOTDB *newdb;
  register ENTRY *ep;
  char newpath[MAXPATHLEN];
  ENTRY thisentry;
  register u_short i;
  register SOURCE *sp;

  dbgprint(F_TRACE, (stderr, "DepotDB1_Tree_Append\n"));

  newdb = db;
  if ((deltadb == NULL) || (deltadb->name == NULL)) /* null tree or initial dummy entry */
    {
      dbgprint(F_TRACE, (stderr, "DepotDB1_Tree_Append done\n"));
      return newdb;
    }

  for ( ep = deltadb; ep != NULL; ep = ENTRY_sibling(ep) )
    {
      /* append entry pointed to by ep */
      if (ep->name == NULL)
	{
	  FatalError(E_UNNAMEDENTRY, (stderr, "No name for entry in database tree\n"));
	}
      if ((pathprefix ==NULL) || (strcmp(pathprefix,"") == 0))
	(void)strcpy(newpath, "");
      else
	{
	  (void)strcpy(newpath, pathprefix);
	  (void)strcat(newpath, "/");
	}
      (void)strcat(newpath, ep->name);
      if ((ep->nsources == 0) || (ep->sourcelist == NULL))
	{
	  FatalError(E_SRCLESSENTRY, (stderr, "target %s has no source\n", newpath));
	}

      thisentry.sibling = NULL; thisentry.child = NULL;
      thisentry.name = newpath;
      if ((ep->nsources != 0)
	  && (strcmp((ep->sourcelist+(ep->nsources-1))->collection_name, "") != 0))
	thisentry.status = S_MODIFIED;
      else
	thisentry.status = 0;
      thisentry.sourcelist = NULL; thisentry.nsources = 0;
      for (i = 0, sp = ep->sourcelist; i < ep->nsources; i++, sp++)
	DepotDB1_SourceList_AddSource(&thisentry, sp);
      newdb = DepotDB1_UpdateEntry(newdb, &thisentry);

      DepotDB1_FreeSourceList(&thisentry);

      newdb = DepotDB1_Tree_Append(newdb, ENTRY_child(ep), newpath);
    }
  dbgprint(F_TRACE, (stderr, "DepotDB1_Tree_Append done\n"));
  return newdb;
}



/*
 * static Boolean DepotDB1_Tree_Apply(db, pathprefix, depth, applspec, CommandFileList)
 * go through the db (sub)tree rooted at db
 * representing files prefixed with pathprefix,
 * recursively in pre-order fashion
 * while applying according to the update spec given.
 * depth keeps track of the depth of the current level from the top of the db tree.
 * applspec = APPL_CHECK or APPL_UPDATE
 * CommandFileList is an IN-OUT parameter which accumulates a sorted list of
 * files to be executed after other updates are completed.
 */
static Boolean DepotDB1_Tree_Apply(db, pathprefix, depth, applspec, CommandFileList)
     DEPOTDB *db;
     char *pathprefix;
     unsigned depth;
     unsigned applspec;
     COMMANDFILE ***CommandFileList;
{
  register ENTRY *ep;
  char newpath[MAXPATHLEN];
  register SOURCE *sp;
  Boolean VirginPreEmpt;
  Boolean ApplyStatus, ApplyChildStatus;

  dbgprint(F_TRACE, (stderr, "DepotDB1_Tree_Apply\n"));

  ApplyStatus = TRUE;
  if ((db == NULL) || (db->name == NULL)) /* null tree or initial dummy entry */
    {
      dbgprint(F_TRACE, (stderr, "DepotDB1_Tree_Apply done\n"));
      return ApplyStatus;
    }
  for ( ep = db; ep != NULL; ep = ENTRY_sibling(ep) )
    {
      /* apply entry pointed to by ep */
      if (ep->name == NULL)
	{
	  FatalError(E_UNNAMEDENTRY, (stderr, "No name for entry in database tree\n"));
	}
      
      if ((ep->nsources == 0) || (ep->sourcelist == NULL))
	{
	  FatalError(E_SRCLESSENTRY, (stderr, "target %s has no source\n", newpath));
	}


      sp = ep->sourcelist+(ep->nsources-1);
      if (   (ep->status & S_SPECIAL) /* special thingies */
	  || (Depot_QuickUpdate && !(ep->status & S_MODIFIED)) /* quick update and unmodified entry */
	  || ((applspec & APPL_UPDATE) && !(ep->status & S_UPDATE)) ) /* target is already OK */
	/* leave as such, don't apply  */
	;
      else
	{
	  if (!(applspec & APPL_CHECK) || (CheckSourceConsistency(ep) == TRUE))
	    {
	      if ((pathprefix == NULL) || (strcmp(pathprefix,"") == 0))
		{
		  (void)strcpy(newpath, ep->name);
		}
	      else
		{
		  if (strcmp(ep->name, "/") == 0)
		    (void)strcpy(newpath, pathprefix);
		  else
		    (void)sprintf(newpath, "%s/%s", pathprefix, ep->name);
		}

	      /* update according to update_spec */
	      if (sp->update_spec & U_NOOP)
		{
		  dbgprint(F_DBAPPLY, (stderr, "NOOP %s\n", newpath));
		  dbgprint(F_CHECK, (stderr, "don't bother to stat etc. to check existence of noop entries\n"));
#if 0
		  if (applspec & APPL_CHECK)
		    { ApplyStatus &=  Check_Noop(ep, newpath); }
		  else if (applspec & APPL_UPDATE)
		    { ApplyStatus &=  Update_Noop(ep, newpath); }
#endif 0
		}
	      else if (sp->update_spec & U_DELETE)
		{
		  dbgprint(F_DBAPPLY, (stderr, "DELETE %s\n", newpath));
		  dbgprint(F_CHECK, (stderr, "don't bother to stat etc. to check non-existence of delete entries\n"));
#if 0
		  if (applspec & APPL_CHECK)
		    { ApplyStatus &=  Check_Delete(ep, newpath); }
		  else if (applspec & APPL_UPDATE)
		    { ApplyStatus &=  Update_Delete(ep, newpath); }
#endif 0
		}
	      else if (sp->update_spec & U_MAP)
		{
		  if (sp->update_spec & U_MAPCOPY)
		    {
		      dbgprint(F_DBAPPLY, (stderr, "COPY %s %s\n", sp->name, newpath));
		      if (applspec & APPL_CHECK)
			{ ApplyStatus &= Check_Copy(ep, newpath, sp->name); }
		      else if (applspec & APPL_UPDATE)
			{ ApplyStatus &= Update_Copy(ep, newpath, sp->name); }
		    }
		  else if (sp->update_spec & U_MAPLINK)
		    {
		      dbgprint(F_DBAPPLY, (stderr, "LINK %s %s\n", sp->name, newpath));
		      if (applspec & APPL_CHECK)
			{ ApplyStatus &= Check_Link(ep, newpath, sp->name, depth); }
		      else if (applspec & APPL_UPDATE)
			{ ApplyStatus &= Update_Link(ep, newpath, sp->name, depth); }
		    }
		  else
		    {
		      dbgprint(F_DBAPPLY, (stderr, "MAP %s %s\n", sp->name, newpath));
		      if (applspec & APPL_CHECK)
			{ ApplyStatus &= Check_Map(ep, newpath, sp->name, depth); }
		      else if (applspec & APPL_UPDATE)
			{ ApplyStatus &= Update_Map(ep, newpath, sp->name, depth); }
		    }
		}
	      else if (sp->update_spec & U_MKDIR)
		{
		  VirginPreEmpt = DepotDB1_Entry_VirginityStatus(ep, TRUE);
		  if (VirginPreEmpt == TRUE)
		    {
		      dbgprint(F_DBAPPLY, (stderr, "VIRGIN PREEMPT\nLINK %s %s\n", sp->name, newpath));
		      if (applspec & APPL_CHECK)
			{ ApplyStatus &= Check_Link(ep, newpath, sp->name, depth); }
		      else if (applspec & APPL_UPDATE)
			{ ApplyStatus &= Update_Link(ep, newpath, sp->name, depth); }
		    }
		  else /* VirginPreEmpt == FALSE */
		    {
		      dbgprint(F_DBAPPLY, (stderr, "MKDIR %s\n", newpath));
		      if (applspec & APPL_CHECK)
			{
			  /* recursively apply children */
			  ApplyStatus &= Check_MakeDir(ep, newpath);
			  ApplyChildStatus = DepotDB1_Tree_Apply(ENTRY_child(ep), newpath, depth+1, applspec, CommandFileList);
			  ApplyStatus &= ApplyChildStatus;
		      if ( !ApplyChildStatus ) ep->status |= S_UPDATE;
			}
		      else if (applspec & APPL_UPDATE)
			{
			  /* recursively apply children */
			  ApplyStatus &= Update_MakeDir(ep, newpath);
			  ApplyStatus &= DepotDB1_Tree_Apply(ENTRY_child(ep), newpath, depth+1, applspec, CommandFileList);
			}
		    }
		}
	      else if (sp->update_spec & U_RCFILE)
		{
		  dbgprint(F_DBAPPLY, (stderr, "EXECRCFILE %s to get %s\n", sp->name, newpath));
		  (void)DepotDB_Command_Insert(CommandFileList, ep);
		  ApplyStatus = FALSE; ep->status |= S_UPDATE;
		}
	    }
	}
    }
  dbgprint(F_TRACE, (stderr, "DepotDB1_Tree_Apply done\n"));
  return ApplyStatus;
}


/*
 * static DEPOTDB *DepotDB1_Tree_RemoveCollection(db, colname, SourceRemoved)
 *	go through the db (sub)tree rooted at db
 *	recursively in pre-order fashion
 *	while removing dependencies on collection colname
 *	SourceRemoved keeps track of whether any source was removed
 *	at or below this level.
 */
static DEPOTDB *DepotDB1_Tree_RemoveCollection(db, colname, SourceRemoved)
     DEPOTDB *db;
     char *colname;
     Boolean *SourceRemoved;
{
  DEPOTDB *newdb;
  register ENTRY *ep, *prevep, *nextep;
  u_short OldNSources;
  Boolean ChildSourceRemoved, EntrySourceRemoved;

  dbgprint(F_TRACE, (stderr, "DepotDB1_Tree_RemoveCollection - being implemented\n"));
  newdb = db;
  if ((db == NULL) || (db->name == NULL)) /* null (sub)tree or initial dummy entry */
    { /* do nothing */
      *SourceRemoved = FALSE;
    }
  else
    {
      *SourceRemoved = FALSE;
      ep = newdb; prevep = NULL; nextep = ENTRY_sibling(ep);
      while( ep != NULL )
	{
	  OldNSources = ep->nsources; EntrySourceRemoved = FALSE;
	  /* remove any reference to colname in sourcelist of entry */
	  DepotDB1_Entry_RemoveCollection(ep, colname, &EntrySourceRemoved);
	  if (OldNSources != ep->nsources)
	    { /* a reference to collection colname was found and removed */
	      if (ep->nsources != 0)
		{ /*ep is dependent on collections other than colname */
		  ChildSourceRemoved = FALSE;
		  /* remove colname from children */
		  ENTRY_child(ep) = DepotDB1_Tree_RemoveCollection(ENTRY_child(ep), colname, &ChildSourceRemoved);
		  if (ChildSourceRemoved | EntrySourceRemoved)
		    { *SourceRemoved = TRUE; ep->status |= S_MODIFIED; }
		  prevep = ep;
		}
	      else
		{
		  *SourceRemoved = TRUE;
	      	  /* remove subtree rooted at this entry */
		  ENTRY_sibling(ep) = NULL;
		  DepotDB1_Free_Subtree(ep);
		  if (prevep == NULL)
		    newdb = nextep;
		  else
		    ENTRY_sibling(prevep) = nextep;
		}
	    }
	  else
	    prevep = ep;
	  ep = nextep;
	  if (ep != NULL) nextep = ENTRY_sibling(ep);
	}
    }
  dbgprint(F_TRACE, (stderr, "DepotDB1_Tree_RemoveCollection done\n"));
  return newdb;
}


/*
 * static void DepotDB1_Tree_ObsoleteCollection(db, colname)
 *	go through the db (sub)tree rooted at db
 *	recursively in pre-order fashion
 *	while marking source from collection colname as obsolete
 */
static void DepotDB1_Tree_ObsoleteCollection(db, colname)
     DEPOTDB *db;
     char *colname;
{
  register ENTRY *ep;
  register SOURCE *sp;
  register u_short i;
  Boolean CollectionObsoleted;

  dbgprint(F_TRACE, (stderr, "DepotDB1_Tree_ObsoleteCollection - being implemented\n"));

  if ((db == NULL) || (db->name == NULL)) /* null (sub)tree or initial dummy entry */
    {
      /* do nothing */;
    }
  else
    {
      for ( ep = db; ep != NULL;  ep = ENTRY_sibling(ep) )
	{
	  for (i = 0, sp = ep->sourcelist, CollectionObsoleted = FALSE;
	       i < ep->nsources;
	       i++, sp++)
	    {
	      if ( sp->collection_name == NULL)
		{
		  FatalError(E_UNREFSOURCE, (stderr, "Entry found with a source which has no named collection\n" ));
		}
	      else if ( strcmp(sp->collection_name, colname) == 0)
		{
		  sp->status |= S_OBSOLETE; CollectionObsoleted = TRUE;
		}
	    }
	  if (CollectionObsoleted)
	    DepotDB1_Tree_ObsoleteCollection(ENTRY_child(ep), colname);
	}
    }
  dbgprint(F_TRACE, (stderr, "DepotDB1_Tree_ObsoleteCollection done\n"));
  return;
}



/*
 * static DEPOTDB *DepotDB1_Tree_PruneCollection(db, colname)
 *	go through the db (sub)tree rooted at db
 *	recursively in pre-order fashion
 *	while removing obsolete dependencies on collection colname
 *	ObsoletePruned keeps track of whether any obsolete sources
 *	were pruned from this level or below.
 */
static DEPOTDB *DepotDB1_Tree_PruneCollection(db, colname, ObsoletePruned)
     DEPOTDB *db;
     char *colname;
     Boolean *ObsoletePruned;
{
  DEPOTDB *newdb;
  register ENTRY *ep, *prevep, *nextep;
  Boolean CollectionFound, ObsoleteChildSourcePruned, ObsoleteEntrySourcePruned;

  dbgprint(F_TRACE, (stderr, "DepotDB1_Tree_PruneCollection\n"));
  newdb = db;
  if ((db == NULL) || (db->name == NULL)) /* null (sub)tree or initial dummy entry */
    { /* do nothing */
      *ObsoletePruned = FALSE;
    }
  else
    {
      *ObsoletePruned = FALSE;
      ep = newdb; prevep = NULL; nextep = ENTRY_sibling(ep);
      while( ep != NULL )
	{
	  ObsoleteEntrySourcePruned = FALSE;
	  /* remove any obsolete reference to colname in sourcelist of entry */
	  DepotDB1_Entry_PruneCollection(ep,colname, &CollectionFound, &ObsoleteEntrySourcePruned);
	  if (!CollectionFound)
	    prevep = ep;
	  else
	    { /* a reference to collection colname was found */
	      if (ep->nsources != 0)
		{
		  ObsoleteChildSourcePruned = FALSE;
		  /* remove obsolete dependencies on colname from children */
		  ENTRY_child(ep) = DepotDB1_Tree_PruneCollection(ENTRY_child(ep), colname, &ObsoleteChildSourcePruned);
		  if (ObsoleteChildSourcePruned | ObsoleteEntrySourcePruned)
		    { *ObsoletePruned = TRUE; ep->status |= S_MODIFIED; }
		  prevep = ep;
		}
	      else
		{
		  *ObsoletePruned = TRUE;
	      	  /* remove subtree rooted at this entry */
		  ENTRY_sibling(ep) = NULL;
		  DepotDB1_Free_Subtree(ep);
		  if (prevep == NULL)
		    newdb = nextep;
		  else
		    ENTRY_sibling(prevep) = nextep;
		}
	    }
	  ep = nextep;
	  if (ep != NULL) nextep = ENTRY_sibling(ep);
	}
    }
  dbgprint(F_TRACE, (stderr, "DepotDB1_Tree_PruneCollection done\n"));
  return newdb;
}


/*
 * static void DepotDB1_Tree_Antiquate(db, UnReferencedExists)
 *	go through the db (sub)tree rooted at db
 *	recursively in pre-order fashion
 *	while marking entries not overridden by unreferenced entries as antiques.
 *	UnReferencedExists keeps track of whether any unreferenced entries exist
 *	at or below this level.
 */
static void DepotDB1_Tree_Antiquate(db, UnReferencedExists)
     DEPOTDB *db;
     Boolean *UnReferencedExists;
{
  register ENTRY *ep;
  register SOURCE *sp;
  register u_short i;
  Boolean UnReferencedChildrenFound;

  dbgprint(F_TRACE, (stderr, "DepotDB1_Tree_Antiquate\n"));
  if ((db == NULL) || (db->name == NULL)) /* null (sub)tree or initial dummy entry */
    { /* do nothing */
      *UnReferencedExists = FALSE;
    }
  else
    {
      *UnReferencedExists = FALSE;
      for ( ep = db; ep != NULL;  ep = ENTRY_sibling(ep) )
	{
	  UnReferencedChildrenFound = FALSE;
	  DepotDB1_Tree_Antiquate(ENTRY_child(ep), &UnReferencedChildrenFound);
	  if (UnReferencedChildrenFound) *UnReferencedExists = TRUE;
	  sp = ep->sourcelist+(ep->nsources-1);
	  if (!Depot_DeleteUnReferenced && !(sp->status & S_OBSOLETE) && (strcmp(sp->collection_name, "") == 0))
	    /* unreferenced entry overrides everything */
	    *UnReferencedExists = TRUE;
	  else
	    {
	      if (!UnReferencedChildrenFound)
		{
		  /*
		   * no unreferenced noop style entries under this level;
		   * remove any unreferenced noop-style source at this level,
		   * unless we are at the root
		   */
		  if (strcmp(ep->name, "/") != 0)
		    {
		      for (i = 0, sp = ep->sourcelist; i < ep->nsources; i++, sp++)
			if ( strcmp(sp->collection_name, "") == 0)
			  sp->status |= S_OBSOLETE;
		    }
		}
	      else
		*UnReferencedExists = TRUE;
	      for (i = 0, sp = ep->sourcelist; i < ep->nsources; i++, sp++)
		sp->status |= S_ANTIQUE;
	    }
	}
    }
  dbgprint(F_TRACE, (stderr, "DepotDB1_Tree_Antiquate done\n"));
  return;
}

/*
 * DEPOTDB *DepotDB1_UpdateEntry(db, entryp)
 *	enters info from the entry pointed to by entryp
 *	into the appropriate location in the database tree db.
 */
DEPOTDB *DepotDB1_UpdateEntry(db, entryp)
     DEPOTDB *db;
     ENTRY *entryp;
{
  register ENTRY *ep;
  DEPOTDB *newdb;

  dbgprint(F_TRACE, (stderr, "DepotDB1_UpdateEntry\n"));

  newdb = db;
  if (newdb == NULL)
    newdb = DepotDB1_AllocEntry();

  ep = DepotDB1_LocateEntryByName(newdb, entryp->name, DB_LOCATE|DB_CREATE);
  if (ep == NULL)
    {
      FatalError(E_NULLENTRY, (stderr, "Unable to locate entry in database for %s\n", entryp->name));
    }
  else
    {
      register SOURCE *sp; register u_short i;
      register ENTRY *ep2;

      /*
       * if ep represented a virgin tree, and the new source
       * indicates a modification which would break the virgin link,
       * mark ep's children as modified.
       * We need to do this if and only if ep has children and they
       * have been immaculately conceived.
       */
      if ((entryp->status & S_MODIFIED)
	  && (ep->sourcelist != NULL)
	  && (ep->sourcelist->update_spec & U_MKDIR)
	  && !(ep->sourcelist->status & S_NONVIRGINSRC))
	{
	  ep2 = ENTRY_child(ep);
	  if ((ep2 != NULL) && (ep2->sourcelist != NULL)
	      && (ep2->sourcelist->status & S_IMMACULATE))
	    {
	      while (ep2 != NULL)
		{
		  ep2->status |= S_MODIFIED;
		  ep2 = ENTRY_sibling(ep2);
		}
	    }
	}

      /* copy sourcelist */
      for (i = 0, sp = entryp->sourcelist; i < entryp->nsources; i++, sp++)
	{
	  DepotDB1_SourceList_AddSource(ep, sp);
	}
      /*
       * nsources should have been updated during calls to
       * DepotDB1_SourceList_AddSource
       */
      ep->status |= (entryp->status & S_SPECIAL);
      ep->status |= (entryp->status & S_MODIFIED);
    }
  dbgprint(F_TRACE, (stderr, "DepotDB1_UpdateEntry done\n"));
  return newdb;
}


/*
 * ENTRY *DepotDB1_LocateEntryByName(db, ename, searchflags)
 * attempts to locate the entry in the database pointed to by db
 * corresponding to the entry given by ename.
 * searchflags is used to control the navigation:
 * if DB_CREATE is set
 *	an entry which is a leaf node may be created, if necessary.
 * if DB_LAX is set
 *	return NULL instead of quitting with FatalError, if search fails
 */
ENTRY *DepotDB1_LocateEntryByName(db, ename, searchflags)
     DEPOTDB *db;
     char *ename;
     Boolean searchflags;
{
  ENTRY *ep, *newep, *locatedentry;
  char path[MAXPATHLEN], *head, *tail;
  Boolean leafnode;

  dbgprint(F_TRACE, (stderr, "DepotDB1_LocateEntryByName\n"));
  if (db == NULL)
    {
      if (searchflags & DB_LAX) return NULL;
      else FatalError(E_NULLDATABASE, (stderr, "no database found while attempting to locate %s\n", ename));
    }
  else
    {
      dbgprint(F_LOCATEENTRY, (stderr, "trying to locate entry for %s\n", ename));
      strcpy(path, ename); ep = db;
      head = "/"; tail = path;
      while (*tail == '/') tail++;
      if (*tail != '\0') leafnode = FALSE;
      else /* *tail == '\0' */ leafnode = TRUE;
      locatedentry = NULL;
      while ( (ep != NULL) && (locatedentry == NULL) )
	{
	  dbgprint(F_LOCATEENTRY, (stderr, "searching for head =%s; tail is %s\n", head, tail));
	  /* locate the entry corresponding to head in siblings at this level */
	  newep = ep;
	  while ( (newep != NULL) && ((newep->name == NULL) || (strcmp(newep->name, head) != 0)))
	    newep = ENTRY_sibling(newep);
	  if (newep == NULL)
	    {
	      dbgprint(F_LOCATEENTRY, (stderr, "Could not locate entry for the head\n"));
	      if (leafnode && (searchflags & DB_CREATE))
		{
		  if (ep->name == NULL)
		    {
		      locatedentry = ep; /* special case of dummy initial node of db tree */
		      dbgprint(F_LOCATEENTRY, (stderr, "Created initial node\n"));
		    }
		  else
		    { /* create a sibling entry and return it */
		      locatedentry = DepotDB1_AllocEntry();
		      ENTRY_sibling(locatedentry) = ENTRY_sibling(ep);
		      ENTRY_sibling(ep) = locatedentry;
		      dbgprint(F_LOCATEENTRY, (stderr, "Created sibling node\n"));
		    }
		  locatedentry->name = (char *)emalloc(strlen(head)+1);
		  locatedentry->name = strcpy(locatedentry->name, head);
		}
	      else
		{
		  if (searchflags & DB_LAX) return NULL;
		  else FatalError(E_NULLENTRY, (stderr, "no entry found for %s\n", ename));
		}
	    }
	  else /* newep != NULL */
	    {
	      dbgprint(F_LOCATEENTRY, (stderr, "located entry for the head\n"));
	      if (leafnode)
		{
		  locatedentry = newep;
		  dbgprint(F_LOCATEENTRY, (stderr, "entry for %s located\n", ename));
		}
	      else
		{
		  head = tail;
		  while ((*tail != '/') && (*tail != '\0')) tail++;
		  if (*tail == '/') {*tail++ = '\0'; leafnode = FALSE;}
		  else /* *tail == '\0' */ leafnode = TRUE;
		  dbgprint(F_LOCATEENTRY, (stderr, "new head is %s; tail is %s\n", head, tail));
		  if (ENTRY_child(newep) == NULL)
		    {
		      dbgprint(F_LOCATEENTRY, (stderr, "no children for entry here\n"));
		      if (leafnode && (searchflags & DB_CREATE))
			{
			  dbgprint(F_LOCATEENTRY, (stderr, "leaf node, create a child with this entry and return it\n"));
			  ENTRY_child(newep) = DepotDB1_AllocEntry();
			  locatedentry = ENTRY_child(newep);
			  locatedentry->name = (char *)emalloc(strlen(head)+1);
			  locatedentry->name = strcpy(locatedentry->name, head);
			}
		      else
			{
			  if (searchflags & DB_LAX) return NULL;
			  else FatalError(E_NULLENTRY, (stderr, "no entry found for %s\n", ename));		  
			}
		    }
		  dbgprint(F_LOCATEENTRY, (stderr, "move on to child level\n"));
		  ep = ENTRY_child(newep);
		}
	    }
	}
    }
  dbgprint(F_TRACE, (stderr, "DepotDB1_LocateEntryByName done\n"));
  return locatedentry;
}


/*
 * DEPOTDB *DepotDB1_DeletePath(db, ename, deleteflags)
 *	Deletes the subtree corresponding to ename from database db
 *	This is to be used only on the depotdb corresponding to a single collection.
 *	deleteflags controls the required severity of deletion
 *	if DB_LAX is *not* set
 *		an attempt to delete a non-existent path causes exit as a FatalError
 */
DEPOTDB *DepotDB1_DeletePath(db, ename, deleteflags)
     DEPOTDB *db;
     char *ename;
     Boolean deleteflags;
{
  DEPOTDB *newdb;
  ENTRY *parent;
  register ENTRY *ep, *newep, *oldep;
  register SOURCE *sp;
  register u_short i;
  char path[MAXPATHLEN], *head, *tail;
  Boolean finalnode, locatedentry;
  
  dbgprint(F_TRACE, (stderr, "DepotDB1_DeletePath\n"));
  newdb = db;

  if (db == NULL)
    {
      if (!(deleteflags & DB_LAX))
	{FatalError(E_NULLDATABASE, (stderr, "no database found while attempting to delete %s\n", ename));}
    }
  else
    {
      dbgprint(F_DELETEPATH, (stderr, "trying to delete entry for %s\n", ename));
      parent = NULL; strcpy(path, ename); ep = db;
      head = "/"; tail = path;
      while (*tail == '/') tail++;
      if (*tail != '\0') finalnode = FALSE;
      else /* *tail == '\0' */ finalnode = TRUE;

      locatedentry = FALSE;
      while ( (ep != NULL) && !locatedentry )
	{
	  dbgprint(F_DELETEPATH, (stderr, "searching for head =%s; tail is %s\n", head, tail));
	  /* locate the entry corresponding to head in siblings at this level */
	  newep = ep; oldep = NULL;
	  while ( (newep != NULL) && (newep->name != NULL) && (strcmp(newep->name, head) != 0) )
	    { oldep = newep; newep = ENTRY_sibling(newep);}
	  if (newep == NULL)
	    {
	      if (!(deleteflags & DB_LAX))
		{FatalError(E_NOENTRY, (stderr, "Could not locate entry for %s\n", ename));}
	      else
		{
		  dbgprint(F_DELETEPATH, (stderr, "Could not locate entry for %s, returning from lax delete\n", ename));
		  dbgprint(F_TRACE, (stderr, "DepotDB1_DeletePath done \n"));
		  return newdb;
                }
	    }
	  else if (newep->name == NULL)
	    {
	      FatalError(E_UNNAMEDENTRY, (stderr, "Entry with no name found in database tree\n"));
	    }
	  else
	    {
	      dbgprint(F_DELETEPATH, (stderr, "located entry for the head\n"));
	      if (finalnode)
		{
		  /* located our node, do the honors */
		  locatedentry = TRUE;
		  if (oldep == NULL)
		    {
		      /* first entry in sibling list */
		      if (parent == NULL)
			{
			  newdb = ENTRY_sibling(newep);
			  ENTRY_sibling(newep) = NULL;
			  DepotDB1_Free_Subtree(newep);
			}
		      else
			{
			  ENTRY_child(parent) = ENTRY_sibling(newep);
			  ENTRY_sibling(newep) = NULL;
			  DepotDB1_Free_Subtree(newep);
			}
		    }
		  else
		    {
		      /* remove entry from sibling list, purge it */
		      ENTRY_sibling(oldep) = ENTRY_sibling(newep);
		      ENTRY_sibling(newep) = NULL;
		      DepotDB1_Free_Subtree(newep);
		    }
		}
	      else
		{
		  /* mark newep's sources as non-virgin */
		  for (i = 0, sp = newep->sourcelist; i < newep->nsources; i++, sp++)
		    {
		      sp->status |= S_NONVIRGINSRC;
		      dbgprint(F_DELETEPATH, (stderr, "Marking %s as non-virgin\n", sp->name));
		    }
		  if (newep->nsources > 1)
		    dbgprint(F_CHECK, (stderr, "WARNING - DepotDB1_DeletePath: Deleting subtree from non-collection depotdb\n"));

		  head = tail;
		  while ((*tail != '/') && (*tail != '\0')) tail++;
		  if (*tail == '/') {*tail++ = '\0'; finalnode = FALSE;}
		  else /* *tail == '\0' */ finalnode = TRUE;

		  dbgprint(F_DELETEPATH, (stderr, "new head is %s; tail is %s\n", head, tail));
		  if (ENTRY_child(newep) == NULL)
		    {
		      dbgprint(F_DELETEPATH, (stderr, "no children for entry here\n"));
		      if (!(deleteflags & DB_LAX))
			{FatalError(E_NULLENTRY, (stderr, "no entry found for %s\n", ename));}
		    }
		  dbgprint(F_DELETEPATH, (stderr, "move on to child level\n"));
		  parent = newep; ep = ENTRY_child(newep);
		}
	    }
	}
    }

  dbgprint(F_TRACE, (stderr, "DepotDB1_DeletePath done \n"));
  return newdb;
}



/*
 * DEPOTDB *DepotDB1_SetNonVirginPath(db, ename, colname)
 *	Marks source from collection colname for the entry corresponding to ename
 *	as nonvirgin.
 */
DEPOTDB	*DepotDB1_SetNonVirginPath(db, ename, colname)
     DEPOTDB *db;
     char *ename, *colname;
{
  DEPOTDB *newdb;
  register ENTRY *ep;
  register SOURCE *sp;
  register u_short i;
  char path[MAXPATHLEN], *head, *tail;
  Boolean finalnode, DefloweredPath, FoundCollection;

  dbgprint(F_TRACE, (stderr, "DepotDB1_SetNonVirginPath\n"));
  newdb = db;

  if (db == NULL)
    {
      FatalError(E_NULLDATABASE, (stderr, "no database found while attempting to deflower %s\n", ename));
    }
  else
    {
      dbgprint(F_DEFLOWERPATH, (stderr, "trying to deflower entry for %s\n", ename));
      strcpy(path, ename); ep = db;
      head = "/"; tail = path;
      while (*tail == '/') tail++;
      if (*tail != '\0') finalnode = FALSE;
      else /* *tail == '\0' */ finalnode = TRUE;

      DefloweredPath = FALSE;
      while ( (ep != NULL) && !DefloweredPath )
	{
	  dbgprint(F_DEFLOWERPATH, (stderr, "searching for head =%s; tail is %s\n", head, tail));
	  /* locate the entry corresponding to head in siblings at this level */
	  while ( (ep != NULL) && (ep->name != NULL) && (strcmp(ep->name, head) != 0) )
	    ep = ENTRY_sibling(ep);
	  if (ep == NULL)
	    { FatalError(E_NOENTRY, (stderr, "Could not locate entry for %s\n", ename)); }
	  else if (ep->name == NULL)
	    { FatalError(E_UNNAMEDENTRY, (stderr, "Entry with no name found in database tree\n")); }
	  else
	    {
	      dbgprint(F_DEFLOWERPATH, (stderr, "located entry for the head\n"));
	      /* mark ep's sources from colname as non-virgin */
	      FoundCollection = FALSE; i = 0; sp = ep->sourcelist;
	      while ( !FoundCollection && (i < ep->nsources))
		{
		  if (sp->collection_name == NULL)
		    { FatalError(E_UNREFSOURCE, (stderr, "NULL collection name found in source list of entry for %s\n", ename)); }
		  if (strcmp(sp->collection_name, colname) == 0)
		    {
		      FoundCollection = TRUE;
		      sp->status |= S_NONVIRGINSRC;
		      dbgprint(F_DEFLOWERPATH, (stderr, "Marking %s as non-virgin\n", sp->name));
		    }
		  else
		    { i++, sp++; }
		}
	      if (!FoundCollection)
		{ FatalError(E_BADENTRY, (stderr, "Could not find source for collection %s in entry for %s\n", colname, ename));}
	      if (finalnode)
		DefloweredPath = TRUE;
	      else
		{
		  head = tail;
		  while ((*tail != '/') && (*tail != '\0')) tail++;
		  if (*tail == '/') {*tail++ = '\0'; finalnode = FALSE;}
		  else /* *tail == '\0' */ finalnode = TRUE;

		  dbgprint(F_DEFLOWERPATH, (stderr, "new head is %s; tail is %s\n", head, tail));
		  if (ENTRY_child(ep) == NULL)
		    {
		      dbgprint(F_DEFLOWERPATH, (stderr, "no children for entry here\n"));
		      FatalError(E_NULLENTRY, (stderr, "no entry found for %s\n", ename));
		    }
		  dbgprint(F_DEFLOWERPATH, (stderr, "move on to child level\n"));
		  ep = ENTRY_child(ep);
		}
	    }
	}
    }

  dbgprint(F_TRACE, (stderr, "DepotDB1_SetNonVirginPath done\n"));

  return newdb;
}



/*
 * void DepotDB1_SelfReferenceRoot(db)
 *	adds a self-referential source to root, so that we don't rmdir the targetdir ever
 */
void DepotDB1_SelfReferenceRoot(db)
     DEPOTDB *db;
{
  SOURCE rootsrc;

  dbgprint(F_TRACE, (stderr, "DepotDB1_SelfReferenceRoot\n"));

  if (db->nsources == 1)
    {
      rootsrc.name = (char *)emalloc(2);
      strcpy(rootsrc.name, "/"); 
      rootsrc.update_spec = U_MKDIR;
      rootsrc.collection_name = ""; rootsrc.status = S_ANTIQUE|S_UPDATE;
#ifdef USE_FSINFO
      rootsrc.fs_status = FS_UNKNOWN;
#endif USE_FSINFO
      DepotDB_SourceList_AddSource(db, &rootsrc);
    }

  dbgprint(F_TRACE, (stderr, "DepotDB1_SelfReferenceRoot done\n"));
  return;
}



/*
 *void DepotDB1_ProtectSpecialFiles(db, filelist)
 *	marks the files on filelist as being special
 */
void DepotDB1_ProtectSpecialFiles(db, filelist)
     DEPOTDB *db;
     char **filelist;
{
  register char **fp;
  ENTRY *ep;
  SOURCE thissrc;

  dbgprint(F_TRACE, (stderr, "DepotDB1_ProtectSpecialFiles\n"));
  if (filelist != NULL)
    {
      for (fp = filelist; *fp != NULL; fp++)
	{
	  ep = DepotDB1_LocateEntryByName(db, *fp, DB_LOCATE|DB_CREATE);
	  ep->status |= S_SPECIAL;
	  if (ep->nsources == 0)
	    {
	      thissrc.name = *fp; thissrc.collection_name = "";
	      thissrc.update_spec = U_NOOP; 
	      thissrc.status = S_NONVIRGINSRC;
	      thissrc.old_update_spec = 0; /* XXX ask sohan */
	      DepotDB1_SourceList_AddSource(ep, &thissrc);
	    }
	  /* might as well release all storage associated with ep's children */
	  DepotDB1_Free_Subtree(ENTRY_child(ep));
	  ENTRY_child(ep) = NULL;
	}
    }
  dbgprint(F_TRACE, (stderr, "DepotDB1_ProtectSpecialFiles done\n"));
  return;
}


/*
 * void DepotDB1_SetTargetMappings(db)
 *	marks update specifications in db to mapcopy or maplink
 *	or to noop or delete
 *	as per preference lists specified.
 */
void DepotDB1_SetTargetMappings(db)
     DEPOTDB *db;
{
  register char **cp, **lp;
  register ENTRY *ep;
  register Boolean StringComparison;
  char **CopyList, **LinkList, **DeleteList, **NoopList;

  dbgprint(F_TRACE, (stderr, "DepotDB1_SetTargetMappings\n"));

  /* first erase any targetmappings specified in the tree */
  DepotDB1_Tree_SetTargetMappings(db, (u_short)0);

  CopyList = Preference_GetStringSet(preference, NULL, "copytarget");
  LinkList = Preference_GetStringSet(preference, NULL, "linktarget");

  if (!CopyList && !LinkList)
    /* do nothing */;
  else if (CopyList && !LinkList)
    {
      for (cp = CopyList; *cp != NULL; cp++)
	{
	  if ((ep = DepotDB_LocateEntryByName(db, *cp, DB_LOCATE|DB_LAX)) != NULL)
	    {
	      DepotDB1_SetTargetMappingPath(db, *cp, (u_short)U_MAPCOPY);
	      DepotDB1_Entry_SetTargetMappings(ep, (u_short)U_MAPCOPY);
	      DepotDB1_Tree_SetTargetMappings(ENTRY_child(ep), (u_short)U_MAPCOPY);
	    }
	}
    }
  else if (!CopyList && LinkList)
    {
      for (lp = LinkList; *lp != NULL; lp++)
	{
	  if ((ep = DepotDB_LocateEntryByName(db, *lp, DB_LOCATE|DB_LAX)) != NULL)
	    {
	      DepotDB1_SetTargetMappingPath(db, *lp, (u_short)U_MAPLINK);
	      DepotDB1_Entry_SetTargetMappings(ep, (u_short)U_MAPLINK);
	      DepotDB1_Tree_SetTargetMappings(ENTRY_child(ep), (u_short)U_MAPLINK);
	    }
	}
    }
  else /* CopyList && LinkList */
    {
      cp = CopyList; lp = LinkList;
      while ((*cp != NULL) || (*lp != NULL))
	{
	  if ((*cp != NULL) && (*lp == NULL))
	    {
	      while (*cp != NULL)
		{
		  if ((ep = DepotDB_LocateEntryByName(db, *cp, DB_LOCATE|DB_LAX)) != NULL)
		    {
		      DepotDB1_SetTargetMappingPath(db, *cp, (u_short)U_MAPCOPY);
		      DepotDB1_Entry_SetTargetMappings(ep, (u_short)U_MAPCOPY);
		      DepotDB1_Tree_SetTargetMappings(ENTRY_child(ep), (u_short)U_MAPCOPY);
		    }
		  cp++;
		}
	    }
	  else if ((*cp == NULL) && (*lp != NULL))
	    {
	      while (*lp != NULL)
		{
		  if ((ep = DepotDB_LocateEntryByName(db, *lp, DB_LOCATE|DB_LAX)) != NULL)
		    {
		      DepotDB1_SetTargetMappingPath(db, *lp, (u_short)U_MAPLINK);
		      DepotDB1_Entry_SetTargetMappings(ep, (u_short)U_MAPLINK);
		      DepotDB1_Tree_SetTargetMappings(ENTRY_child(ep), (u_short)U_MAPLINK);
		    }
		  lp++;
		}
	    }
	  else /* if ((*cp != NULL) && (*lp != NULL)) */
	    {
	      StringComparison = strcmp(*cp, *lp);
	      if (StringComparison > 0)
		{
		  if ((ep = DepotDB_LocateEntryByName(db, *cp, DB_LOCATE|DB_LAX)) != NULL)
		    {
		      DepotDB1_SetTargetMappingPath(db, *cp, (u_short)U_MAPCOPY);
		      DepotDB1_Entry_SetTargetMappings(ep, (u_short)U_MAPCOPY);
		      DepotDB1_Tree_SetTargetMappings(ENTRY_child(ep), (u_short)U_MAPCOPY);
		    }
		  cp++;
		}
	      else if (StringComparison < 0)
		{
		  if ((ep = DepotDB_LocateEntryByName(db, *lp, DB_LOCATE|DB_LAX)) != NULL)
		    {
		      DepotDB1_SetTargetMappingPath(db, *lp, (u_short)U_MAPLINK);
		      DepotDB1_Entry_SetTargetMappings(ep, (u_short)U_MAPLINK);
		      DepotDB1_Tree_SetTargetMappings(ENTRY_child(ep), (u_short)U_MAPLINK);
		    }
		  lp++;
		}
	      else if (StringComparison == 0)
		{
		  if ((ep = DepotDB_LocateEntryByName(db, *cp, DB_LOCATE|DB_LAX)) != NULL)
		    {
		      DepotDB1_SetTargetMappingPath(db, *cp, (u_short)U_MAPCOPY);
		      DepotDB1_Entry_SetTargetMappings(ep, (u_short)U_MAPCOPY);
		      DepotDB1_Tree_SetTargetMappings(ENTRY_child(ep), (u_short)U_MAPCOPY);
		    }
		  cp++; lp++;
		}
	    }
	}
    }

  DeleteList = Preference_GetStringSet(preference, NULL, "deletetarget");
  if (DeleteList)
    {
      for (cp = DeleteList; *cp != NULL; cp++)
	{
	  if ((ep = DepotDB_LocateEntryByName(db, *cp, DB_LOCATE|DB_LAX)) != NULL)
	    {
	      DepotDB1_SetTargetMappingPath(db, *cp, (u_short)U_DELETE);
	      DepotDB1_Entry_SetTargetMappings(ep, (u_short)U_DELETE);
	      DepotDB1_Tree_SetTargetMappings(ENTRY_child(ep), (u_short)U_DELETE);
	    }
	}
    }

  NoopList = Preference_GetStringSet(preference, NULL, "nooptarget");
  if (NoopList)
    {
      for (cp = NoopList; *cp != NULL; cp++)
	{
	  if ((ep = DepotDB_LocateEntryByName(db, *cp, DB_LOCATE|DB_LAX)) != NULL)
	    {
	      DepotDB1_SetTargetMappingPath(db, *cp, (u_short)U_NOOP);
	      DepotDB1_Entry_SetTargetMappings(ep, (u_short)U_NOOP);
	      DepotDB1_Tree_SetTargetMappings(ENTRY_child(ep), (u_short)U_NOOP);
	    }
	}
    }

  dbgprint(F_TRACE, (stderr, "DepotDB1_SetTargetMappings done\n"));
  return;
}


/*
 * static void DepotDB1_Tree_SetTargetMappings(db, maptype)
 *	marks update specifications for sources in all entries in subtree db to specified maptype
 */
static void DepotDB1_Tree_SetTargetMappings(db, maptype)
     DEPOTDB *db;
     u_short maptype;
{
  register ENTRY *ep;

  dbgprint(F_TRACE, (stderr, "DepotDB1_Tree_SetTargetMappings\n"));

  for (ep = db; ep != NULL; ep = ENTRY_sibling(ep))
    {
      DepotDB1_Entry_SetTargetMappings(ep, maptype);
      DepotDB1_Tree_SetTargetMappings(ENTRY_child(ep), maptype);
    }

  dbgprint(F_TRACE, (stderr, "DepotDB1_Tree_SetTargetMappings done\n"));
  return;
}



/*
 * static void DepotDB1_Entry_SetTargetMappings(entryp, maptype)
 *	marks update specifications for sources in entry pointed to by entryp to specified maptype
 */
static void DepotDB1_Entry_SetTargetMappings(entryp, maptype)
     ENTRY *entryp;
     u_short maptype;
{
  register int i;
  register SOURCE *sp;
  dbgprint(F_TRACE, (stderr, "DepotDB1_Entry_SetTargetMappings\n"));

  if (entryp->sourcelist != NULL)
    {
      if (maptype & (U_MAPCOPY|U_MAPLINK))
	{
	  for ( i = 0, sp = entryp->sourcelist; i < entryp->nsources; i++, sp++ )
	    {
	      sp->update_spec &= (~(U_MAPCOPY|U_MAPLINK)); /* erase any previous copy or link update_spec */
	      sp->update_spec |= (maptype & (U_MAPCOPY|U_MAPLINK)); /* add specified maptype */
	    }
	}
      else if (maptype & (U_NOOP|U_DELETE))
	{
	  for ( i = 0, sp = entryp->sourcelist; i < entryp->nsources; i++, sp++ )
	    {
	      sp->update_spec &= (~U_DELETE); /* erase any previous delete update_spec */
	      sp->update_spec |= (maptype & (U_NOOP|U_DELETE)); /* add specified maptype */
	    }
	}
      else /* erase target updates! */
	{
	  for ( i = 0, sp = entryp->sourcelist; i < entryp->nsources; i++, sp++ )
	    {
	      if (strcmp(sp->collection_name, "") != 0)
		/* not an unreferenced entry */
		sp->update_spec &= (~(U_MAPCOPY|U_MAPLINK|U_NOOP|U_DELETE));
	      else
		sp->update_spec &= (~(U_MAPCOPY|U_MAPLINK|U_DELETE));
	    }
	}
    }

  dbgprint(F_TRACE, (stderr, "DepotDB1_Entry_SetTargetMappings done\n"));
  return;
}



static void DepotDB1_SetTargetMappingPath(db, ename, maptype)
     DEPOTDB *db;
     char *ename;
     u_short maptype;
{
  register ENTRY *ep;
  register SOURCE *sp;
  register u_short i;
  char path[MAXPATHLEN], *head, *tail;
  Boolean finalnode, MarkedPath;

  dbgprint(F_TRACE, (stderr, "DepotDB1_SetTargetMappingPath\n"));

  if (db == NULL)
    {
      FatalError(E_NULLDATABASE, (stderr, "no database found while attempting to set target mapping path for %s\n", ename));
    }
  else
    {
      dbgprint(F_TARGETMAPPATH, (stderr, "trying to set target mapping path for %s\n", ename));
      strcpy(path, ename); ep = db;
      head = "/"; tail = path;
      while (*tail == '/') tail++;
      if (*tail != '\0') finalnode = FALSE;
      else /* *tail == '\0' */ finalnode = TRUE;

      MarkedPath = FALSE;
      while ( (ep != NULL) && !MarkedPath )
	{
	  dbgprint(F_TARGETMAPPATH, (stderr, "searching for head =%s; tail is %s\n", head, tail));
	  /* locate the entry corresponding to head in siblings at this level */
	  while ( (ep != NULL) && (ep->name != NULL) && (strcmp(ep->name, head) != 0) )
	    ep = ENTRY_sibling(ep);
	  if (ep == NULL)
	    { FatalError(E_NOENTRY, (stderr, "Could not locate entry for %s\n", ename)); }
	  else if (ep->name == NULL)
	    { FatalError(E_UNNAMEDENTRY, (stderr, "Entry with no name found in database tree\n")); }
	  else
	    {
	      dbgprint(F_TARGETMAPPATH, (stderr, "located entry for the head\n"));
	      if (maptype & (U_MAPLINK|U_MAPCOPY))
		{
		  /* mark mapping type for update_spec for ep's sources */
		  for (i = 0, sp = ep->sourcelist; i < ep->nsources ; i++, sp++)
		    {
		      sp->update_spec |= (maptype & (U_MAPLINK|U_MAPCOPY));
		    }
		}
	      else if (maptype & U_DELETE)
		{
		  for (i = 0, sp = ep->sourcelist; i < ep->nsources ; i++, sp++)
		    {
		      sp->status |= S_NONVIRGINTRG;
		    }
		}
	      else if (maptype & U_NOOP)
		{
		  for (i = 0, sp = ep->sourcelist; i < ep->nsources ; i++, sp++)
		    {
		      sp->update_spec &= (~U_DELETE); /* erase any delete specification */
		      sp->status |= S_NONVIRGINTRG;
		    }
		}
	      if (finalnode)
		MarkedPath = TRUE;
	      else
		{
		  head = tail;
		  while ((*tail != '/') && (*tail != '\0')) tail++;
		  if (*tail == '/') {*tail++ = '\0'; finalnode = FALSE;}
		  else /* *tail == '\0' */ finalnode = TRUE;

		  dbgprint(F_TARGETMAPPATH, (stderr, "new head is %s; tail is %s\n", head, tail));
		  if (ENTRY_child(ep) == NULL)
		    {
		      dbgprint(F_TARGETMAPPATH, (stderr, "no children for entry here\n"));
		      FatalError(E_NULLENTRY, (stderr, "no entry found for %s\n", ename));
		    }
		  dbgprint(F_TARGETMAPPATH, (stderr, "move on to child level\n"));
		  ep = ENTRY_child(ep);
		}
	    }
	}
    }

  dbgprint(F_TRACE, (stderr, "DepotDB1_SetTargetMappingPath done\n"));
  return;
}




/*
 * void DepotDB1_SourceList_AddSource(entryp, srcp)
 *	adds source info from srcp to the sourcelist of the entry pointed to by entryp.
 *	A new source is added prior to a source which overrides it. This
 *	ensures that the last source will be the one to use during updates.
 *	Duplicate sources may be used to update outdated specifications.
 *	Command sources will replace other contributions from the same collection.
 *	Such replacement could lead to multiple command sources in the same sourcelist,
 *	but these duplicate sources will be coalesced on any copying of the entry and
 *	even though unsightly, will not cause problems with the functioning of the program.
 */
void DepotDB1_SourceList_AddSource(entryp, srcp)
     ENTRY *entryp;
     SOURCE *srcp;
{
  register SOURCE *sp, *tp;
  register u_short i;
  u_short AddPoint;	/* Index in Sourcelist at which to add new source */
  Boolean DuplicateSource, LocatedOverride, CommandReplacement;

  dbgprint(F_TRACE, (stderr, "DepotDB1_SourceList_AddSource\n"));

  /* locate point at which a source is found which overrides srcp->name */
  i = 0; sp = entryp->sourcelist;
  DuplicateSource = FALSE; CommandReplacement = FALSE; LocatedOverride = FALSE;
  while (!LocatedOverride && !DuplicateSource && !CommandReplacement && (i < entryp->nsources))
    {
      if ((strcmp(sp->collection_name, srcp->collection_name) == 0)
	  && (srcp->update_spec & U_RCFILE))
	{ /* command updates overwrite all other updates from the same collection. */
	  CommandReplacement = TRUE; AddPoint = i;
	}
      else if ((strcmp(sp->collection_name, srcp->collection_name) == 0)
	       && (strcmp(sp->name, srcp->name) == 0))
	{ DuplicateSource = TRUE; AddPoint = i; }
      else if ((strcmp(srcp->collection_name, "") == 0)
	       && (Depot_DeleteUnReferenced || (sp->status & S_ANTIQUE) || (sp->update_spec & U_MKDIR))
	       && !(entryp->status & S_SPECIAL))
	{
	  LocatedOverride = TRUE; AddPoint = 0;
	  dbgprint(F_CHECK, (stderr, "Overridden unreferenced sources should probably not be added at all\n"));
	}
      else if ((strcmp(sp->collection_name, "") == 0)
	       && !Depot_DeleteUnReferenced
	       && !(srcp->status & S_ANTIQUE)
	       && !(srcp->update_spec & U_MKDIR))
	{LocatedOverride = TRUE; AddPoint = i;}
      else if (Override(sp->collection_name, srcp->collection_name, preference) == TRUE)
	{LocatedOverride = TRUE; AddPoint = i;}
      else
	{i++; sp++;}
    }

  if (!LocatedOverride && !DuplicateSource && !CommandReplacement)
    AddPoint = entryp->nsources;	/* append to end of sourcelist */

  if (CommandReplacement)
    {
      /* replace all contributions from srcp->collection_name */
      i = AddPoint; sp = entryp->sourcelist + AddPoint;
      while (i < entryp->nsources)
	{
	  if (strcmp(sp->collection_name, srcp->collection_name) == 0)
	    {
	      if ((sp->update_spec & U_RCFILE)
		  && !(sp->status & S_OBSOLETE)
		  && (strcmp(sp->name, srcp->name) != 0))
		{ FatalError(E_BADSRCUPDT, (stderr, "Conflicting command update sources %s and %s while updating sourcelist for %s from collection %s\n", sp->name, srcp->name, entryp->name, srcp->collection_name)); }
	      if ((!(sp->update_spec & U_RCFILE)) || (strcmp(sp->name, srcp->name) != 0))
		{ sp->status = srcp->status; }
	      else /* (sp->update_spec & U_RCFILE) && (strcmp(sp->name, srcp->name) == 0) */
		{
		  sp->status |= (srcp->status & S_ANTIQUE);
		  if ((sp->status & S_OBSOLETE) && !(srcp->status & S_OBSOLETE))
		    {
		      /* if (!(srcp->status & S_TARGET)) sp->status &= ~S_TARGET; */
		      sp->status &= (srcp->status | ~S_TARGET);
		    }
		  else if (!(srcp->status & S_OBSOLETE))
		    { sp->status |= (srcp->status & S_TARGET); }
		  /* if (!(srcp->status & S_OBSOLETE)) sp->status &= ~S_OBSOLETE; */
		  sp->status &= (srcp->status | ~S_OBSOLETE);
		}
	      free(sp->name);
	      sp->name = (char *)emalloc(strlen(srcp->name)+1);
	      sp->name = strcpy(sp->name, srcp->name);
	      sp->update_spec = srcp->update_spec;
	      /* sp->old_update_spec = ???$$$??? don't care? */
	    }
	  i++; sp++;
	}
    }
  else if (DuplicateSource)
    {
      sp = entryp->sourcelist + AddPoint;
      if (!(sp->status & S_OBSOLETE) && (sp->update_spec != srcp->update_spec))
	{ FatalError(E_BADSRCUPDT, (stderr, "Conflicting update specifications %o from %s and %o from %s while updating sourcelist for %s from collection %s\n", sp->update_spec, sp->name, srcp->update_spec, srcp->name, entryp->name, srcp->collection_name)); }
      else
	{
	  if (((sp->update_spec & U_NOOP) != (srcp->update_spec & U_NOOP))
	      || ((sp->update_spec & U_DELETE) != (srcp->update_spec & U_DELETE)))
	    { sp->status &= ~S_TARGET; }
	  else if (   ((sp->update_spec & U_NOOP) && (srcp->update_spec & U_NOOP))
		   || ((sp->update_spec & U_DELETE) && (srcp->update_spec & U_DELETE))
		   || ((sp->update_spec & U_MKDIR) && (srcp->update_spec & U_MKDIR))
		   || ((sp->update_spec & U_MAP) && (srcp->update_spec & U_MAP)
		       && (   (sp->update_spec & srcp->update_spec & U_MAPLINK) /* both maplinks */
			   || (   (sp->update_spec & U_MAPLINK) && !(srcp->update_spec & U_MAPCOPY)
			       && (GetMapCommand(srcp->collection_name, preference) == U_MAPLINK))
			   || (   (sp->status & S_OBSOLETE) && !(srcp->status & S_OBSOLETE)
			       && (srcp->update_spec & U_MAPLINK) && !(sp->update_spec & U_MAPCOPY)
			       && (GetMapCommand(srcp->collection_name, PreferencesSaved) == U_MAPLINK))
			   || (   (sp->status & S_OBSOLETE) && !(srcp->status & S_OBSOLETE)
			       && !((sp->update_spec|srcp->update_spec) & (U_MAPLINK|U_MAPCOPY))
			       && (GetMapCommand(srcp->collection_name, preference) == U_MAPLINK)
			       && (GetMapCommand(srcp->collection_name, PreferencesSaved) == U_MAPLINK)))))
	    { sp->status |= (srcp->status & S_TARGET); }
	  else if (   (srcp->status & S_TARGET)
		   && (sp->update_spec & U_MAP) && (srcp->update_spec & U_MAP)
		   && (   (sp->update_spec & srcp->update_spec & U_MAPCOPY) /* both mapcopys */
		       || (   (sp->update_spec & U_MAPCOPY) && !(srcp->update_spec & U_MAPLINK)
			   && (GetMapCommand(srcp->collection_name, preference) == U_MAPCOPY))
		       || (   (sp->status & S_OBSOLETE) && !(srcp->status & S_OBSOLETE)
			   && (srcp->update_spec & U_MAPCOPY) && !(sp->update_spec & U_MAPLINK)
			   && (GetMapCommand(srcp->collection_name, PreferencesSaved) == U_MAPCOPY))
		       || (   (sp->status & S_OBSOLETE) && !(srcp->status & S_OBSOLETE)
			  && !((sp->update_spec|srcp->update_spec) & (U_MAPLINK|U_MAPCOPY))
			  && (GetMapCommand(srcp->collection_name, preference) == U_MAPCOPY)
			  && (GetMapCommand(srcp->collection_name, PreferencesSaved) == U_MAPCOPY))))
	    { sp->status |= S_TARGET; }
	  else
	    { sp->status &= ~S_TARGET; }
	  /* upgrade other status flags */
	  sp->status |= (srcp->status & S_NONVIRGINSRC);
	  sp->status |= (srcp->status & S_ANTIQUE);
	  /* if (!(srcp->status & S_OBSOLETE)) sp->status &= ~S_OBSOLETE; */
	  sp->status &= (srcp->status | ~S_OBSOLETE);

	  sp->update_spec = srcp->update_spec; /* latest update specification is it! */
	  /* the foll: should happen only with the dummy obsolete root */
	  if (srcp->status & S_OBSOLETE)
	    {
	      if (strcmp(entryp->name, "/") != 0)
		{ FatalError(E_BADSRCUPDT, (stderr, "Attempt to update entry for %s with a duplicate obsolete source %s from collection %s", entryp->name, sp->name, sp->collection_name)); }
	      else
		{ sp->old_update_spec = srcp->old_update_spec; }
	    }
	}
    }
  else /* !DuplicateSource && !CommandReplacement */
    {
      if (entryp->sourcelist == NULL)
	{/* allocate array of length 1 */
	  entryp->nsources = 1; entryp->sourcelist = (SOURCE *)emalloc(sizeof(SOURCE));
	}
      else
	{/* reallocate to increase size by 1 */
	  entryp->nsources++; entryp->sourcelist = (SOURCE *)erealloc((char *)entryp->sourcelist, (unsigned)entryp->nsources*sizeof(SOURCE));
	}

      /* move up all sources after the AddPoint */
      i = entryp->nsources - 1;
      while (i > AddPoint)
	{
	  sp = entryp->sourcelist+i-1; tp = entryp->sourcelist+i;
	  tp->name = sp->name;
	  tp->update_spec = sp->update_spec; tp->old_update_spec = sp->old_update_spec;
	  tp->collection_name = sp->collection_name; tp->status = sp->status;
#ifdef USE_FSINFO
	  if ((tp->update_spec & U_MKDIR)
	      && ((tp->fs_status = sp->fs_status) & FS_NEWFILESYSTEM))
	    {
	      tp->fs_id = sp->fs_id;
	      tp->fs_modtime = sp->fs_modtime;
	      tp->col_id = sp->col_id;
	      tp->col_conftime = sp->col_conftime;
	    }
#endif USE_FSINFO
	  i--;
	}

      /* insert srcp at the AddPoint */
      sp = entryp->sourcelist + AddPoint;
      sp->name = (char *)emalloc(strlen(srcp->name)+1);
      sp->name = strcpy(sp->name, srcp->name);
      sp->collection_name = (char *)emalloc(strlen(srcp->collection_name)+1);
      sp->collection_name = strcpy(sp->collection_name, srcp->collection_name);
      sp->update_spec = srcp->update_spec; 
      sp->old_update_spec = srcp->old_update_spec;
      sp->status = srcp->status;
    }

#ifdef USE_FSINFO
  if ((sp->update_spec & U_MKDIR)
      && ((sp->fs_status = srcp->fs_status) & FS_NEWFILESYSTEM))
    {
      sp->fs_id = srcp->fs_id;
      sp->fs_modtime = srcp->fs_modtime;
      sp->col_id = srcp->col_id;
      sp->col_conftime = srcp->col_conftime;
    }
#endif USE_FSINFO

  /* if adding a non-target rcfile source, */
  /* label all rcfile sources as not corresponding to the target */
  if (!(srcp->status & S_OBSOLETE) && (srcp->update_spec & U_RCFILE) && !(srcp->status & S_TARGET))
    {
      for (i = 0, sp = entryp->sourcelist; i < entryp->nsources; i++, sp++)
	{
	  if ((sp->update_spec & U_RCFILE) && (strcmp(sp->name, srcp->name) == 0))
	    sp->status &= ~S_TARGET;
	}
    }
  dbgDumpSourcelist(entryp->sourcelist, entryp->nsources);
  dbgprint(F_TRACE, (stderr, "DepotDB1_SourceList_AddSource done\n"));
  return;
}


/*
 * SOURCE *DepotDB1_SourceList_LocateCollectionSourceByName(entryp, sourcename, colname, searchflags)
 *	Returns a pointer to the source info in entryp's sourcelist
 *	which corresponds to the source given by sourcename
 *	from the collection colname.
 *	if DB_LAX is set
 *		any U_RCFILE source from collection colname will be accepted
 *	Returns NULL if no such source exists.
 */
SOURCE *DepotDB1_SourceList_LocateCollectionSourceByName(entryp, sourcename, colname, searchflags)
     ENTRY *entryp;
     char *sourcename;
     char *colname;
     Boolean searchflags;
{
  register SOURCE *sp;
  register u_short i;
  Boolean LocatedSource;

  dbgprint(F_TRACE, (stderr, "DepotDB1_SourceList_LocateCollectionSourceByName\n"));

  i = 0; sp = entryp->sourcelist; LocatedSource = FALSE;
  while (!LocatedSource && (i < entryp->nsources))
    {
      if ((strcmp(sp->collection_name, colname) == 0)
	  && ((strcmp(sp->name, sourcename) == 0)
	      || ((searchflags & DB_LAX) && (sp->update_spec & U_RCFILE))))
	LocatedSource = TRUE;
      else
	{i++; sp++;}
    }
  if (!LocatedSource) sp = NULL;

  dbgprint(F_TRACE, (stderr, "DepotDB1_SourceList_LocateCollectionSourceByName done\n"));
  return sp;
}



/*
 * static void DepotDB1_Entry_RemoveCollection(ep, colname, CollectionSourceRemoved)
 *	Removes references to colname in the source list of entry given by ep
 */
static void DepotDB1_Entry_RemoveCollection(ep, colname, CollectionSourceRemoved)
     ENTRY *ep;
     char *colname;
     Boolean *CollectionSourceRemoved;
{
  register SOURCE *sp, *tp;
  u_short i, nremoved;

  dbgprint(F_TRACE, (stderr, "DepotDB1_Entry_RemoveCollection\n"));
  /*
   * remove references to collections and pack the array by moving down higher
   * elements. tp is used to keep track of the last free slot. nremoved
   * keeps track of the number of elements removed.
   */
  for ( i = 0, sp = ep->sourcelist, nremoved = 0, tp = sp, *CollectionSourceRemoved = FALSE;
        i < ep->nsources;
        i++, sp++ )
    {
      if ( sp->collection_name == NULL)
	{
	  FatalError(E_UNREFSOURCE, (stderr, "Entry found with a source which has no named collection\n" ));
	}
      else if ( strcmp(sp->collection_name, colname) == 0)
	{ /* reference to collection here */
	  if ( i == ep->nsources - 1) /* actual source entry is being pruned */
	    { *CollectionSourceRemoved = TRUE; ep->status |= S_MODIFIED; }
	  free(sp->name); free(sp->collection_name);
	  sp->name = NULL; sp->collection_name = NULL;
	  sp->update_spec = 0; sp->status = 0;
#ifdef USE_FSINFO
	  sp->fs_status = FS_UNKNOWN;
#endif USE_FSINFO
	  nremoved++;
	}
      else
	{
	  if (nremoved != 0)
	    {
	      tp->name = sp->name; sp->name = NULL;
	      tp->collection_name = sp->collection_name; sp->collection_name = NULL;
	      tp->update_spec = sp->update_spec; tp->old_update_spec = sp->old_update_spec;
	      tp->status = sp->status;
#ifdef USE_FSINFO
	      if ((tp->update_spec & U_MKDIR)
		  && ((tp->fs_status = sp->fs_status) & FS_NEWFILESYSTEM))
		{
		  tp->fs_id = sp->fs_id;
		  tp->fs_modtime = sp->fs_modtime;
		  tp->col_id = sp->col_id;
		  tp->col_conftime = sp->col_conftime;
		}
#endif USE_FSINFO
	    }
	  tp++;
	}
    }
  if (nremoved != 0)
    { /* realloc sourcelist to right size */
      ep->nsources -= nremoved;
      ep->sourcelist = (SOURCE *)erealloc((char *)ep->sourcelist, (unsigned)ep->nsources*sizeof(SOURCE));
    }
   dbgprint(F_TRACE, (stderr, "DepotDB1_Entry_RemoveCollection done\n"));
}



/*
 * static void DepotDB1_Entry_PruneCollection(ep, colname, CollectionFound, CollectionSourcePruned)
 *	Removes obsolete references to colname in the source list
 *	of entry given by ep.
 *	CollectionFound is an OUT parameter which indicates whether
 *	a reference to collection colname was found in the source list.
 *	CollectionSourcePruned is an OUT parameter which indicates whether
 *	the actual source for the entry ep was pruned.
 */
static void DepotDB1_Entry_PruneCollection(ep, colname, CollectionFound, CollectionSourcePruned)
     ENTRY *ep;
     char *colname;
     Boolean *CollectionFound;
     Boolean *CollectionSourcePruned;
{
  register SOURCE *sp, *tp;
  u_short i, nremoved;

  dbgprint(F_TRACE, (stderr, "DepotDB1_Entry_PruneCollection - being implemented\n"));
  /*
   * remove obsolete references to collections and pack the array by moving down higher
   * elements. tp is used to keep track of the last free slot. nremoved
   * keeps track of the number of elements removed.
   */
  for ( i = 0, sp = ep->sourcelist, nremoved = 0, tp = sp, *CollectionFound = FALSE, *CollectionSourcePruned = FALSE;
        i < ep->nsources;
        i++, sp++ )
    {
      if ( sp->collection_name == NULL)
	{
	  FatalError(E_UNREFSOURCE, (stderr, "Entry found with a source which has no named collection\n" ));
	}
      else if (( strcmp(sp->collection_name, colname) == 0)
	       && (sp->status & S_OBSOLETE))
	{ /* obsolete reference to collection here */
	  *CollectionFound = TRUE;
	  if ( i == ep->nsources - 1) /* actual source entry is being pruned */
	    { *CollectionSourcePruned = TRUE; ep->status |= S_MODIFIED; }
	  free(sp->name); free(sp->collection_name);
	  sp->name = NULL; sp->collection_name = NULL;
	  sp->update_spec = 0; sp->status = 0;
#ifdef USE_FSINFO
	  sp->fs_status = FS_UNKNOWN;
#endif USE_FSINFO
	  nremoved++;
	}
      else
	{
	  if ( strcmp(sp->collection_name, colname) == 0)
	    *CollectionFound = TRUE;
	  if (nremoved != 0)
	    {
	      tp->name = sp->name; sp->name = NULL;
	      tp->collection_name = sp->collection_name; sp->collection_name = NULL;
	      tp->update_spec = sp->update_spec; tp->old_update_spec = sp->old_update_spec;
	      tp->status = sp->status;
#ifdef USE_FSINFO
	      if ((tp->update_spec & U_MKDIR)
		  && ((tp->fs_status = sp->fs_status) & FS_NEWFILESYSTEM))
		{
		  tp->fs_id = sp->fs_id;
		  tp->fs_modtime = sp->fs_modtime;
		  tp->col_id = sp->col_id;
		  tp->col_conftime = sp->col_conftime;
		}
#endif USE_FSINFO
	    }
	  tp++;
	}
    }
  if (nremoved != 0)
    { /* realloc sourcelist to right size */
      ep->nsources -= nremoved;
      ep->sourcelist = (SOURCE *)erealloc((char *)ep->sourcelist, (unsigned)ep->nsources*sizeof(SOURCE));
    }
   dbgprint(F_TRACE, (stderr, "DepotDB1_Entry_PruneCollection done\n"));
}


/*
 * static Boolean DepotDB1_Entry_VirginityStatus(entryp, flags)
 *     ENTRY *entryp;
 *     Boolean flags;
 * Checks whether entryp specifies a virgin mapping of some directory.
 * flags specifies whether the virgin source is to be moved to the end
 * of the sourcelist.
 */
static Boolean DepotDB1_Entry_VirginityStatus(entryp, flags)
     ENTRY *entryp;
     Boolean flags;
{
  register ENTRY *ep;
  register u_short i;
  register SOURCE *sp;
  SOURCE *sourcep;
  char *colname;
  u_short ci;
  SOURCE temp;
  Boolean VirginEntry;

  dbgprint(F_TRACE, (stderr, "DepotDB1_Entry_VirginityStatus\n"));

  /*
   * The following cause loss of virginity
   *  NonVirginTarget
   *  Some NonVirginSource
   *  Some CopySource
   *  Some NonLinkOrCopySource With Copy MapCommand
   *  MultipleSources & MultipleCollectionChildren
   *  MultipleSources & Multiple Contribution from Source Collection
   * Other methods are not relevant to depot. :-)
   */
  VirginEntry = TRUE;
  if (entryp != NULL)
    {
      sourcep = entryp->sourcelist+entryp->nsources-1;
      if (!(sourcep->update_spec & U_MKDIR))
	VirginEntry = FALSE; /* care only for the U_MKDIR entries */
      if (sourcep->status & S_NONVIRGINTRG)
	VirginEntry = FALSE;
      if (VirginEntry == TRUE)
	{
	  /*
	   * check if any source is nonvirgin or
	   * to be mapped by copy either by mapcommand or copytarget
	   */
	  i= 0; sp = entryp->sourcelist;
	  while ((VirginEntry == TRUE) && (i < entryp->nsources))
	    {
	      if (   (sp->status & S_NONVIRGINSRC)
		  || (sp->update_spec & U_MAPCOPY)
		  || (  !(sp->update_spec & U_MAPLINK)
		      && (GetMapCommand(sp->collection_name, preference) != U_MAPLINK)))
		VirginEntry = FALSE;
	      i++; sp++;
	    }
	}
      if ((VirginEntry == TRUE) && (entryp->nsources > 1))
	{
	  /*
	   * check sources of children to see if
	   * contributions from multiple collections exist
	   */
	  ep = ENTRY_child(entryp);
	  if (ep != NULL)
	    {
	      if (ep->name == NULL)
		{
		  FatalError(E_UNNAMEDENTRY, (stderr, "No name for entry in database tree\n"));
		}
	      if ((ep->nsources == 0) || (ep->sourcelist == NULL))
		{
		  FatalError(E_SRCLESSENTRY, (stderr, "target found which has no source\n"));
		}

	      if (flags == TRUE)
		colname = ep->sourcelist->collection_name;
	      else
		colname = sourcep->collection_name;
	      while ((VirginEntry == TRUE) && (ep != NULL))
		{
		  i = 0; sp = ep->sourcelist;
		  while ((VirginEntry == TRUE) && (i < ep->nsources))
		    {
		      if ((colname == NULL) || (sp->collection_name == NULL))
			{
			  FatalError(E_UNREFSOURCE, (stderr, "Entry found with a source which has no named collection\n" ));
			}
		      if (strcmp(colname, sp->collection_name) != 0)
			/* multiple collections in children */
			VirginEntry = FALSE;
		      i++; sp++;
		    }
		  ep = ENTRY_sibling(ep);
		}
	      if (VirginEntry == TRUE)
		{
		  /*
		   * if children have contributions from only one collection
		   * locate the source corresponding to that collection;
		   * if more than one source source exists,
		   * the entry has been deflowered.
		   * else move it to the end of sourcelist if flags is on.
		   */
		  ci = entryp->nsources;
		  i = 0; sp = entryp->sourcelist;
		  while ((VirginEntry == TRUE) && (i < entryp->nsources))
		    {
		      if (sp->collection_name == NULL)
			{
			  FatalError(E_UNREFSOURCE, (stderr, "Entry found with a source which has no named collection\n" ));
			}
		      if (strcmp(colname, sp->collection_name) == 0)
			{
			  if (ci == entryp->nsources)
			    ci = i;
			  else
			    /* ci set earlier */
			    /* multiple contributions from collection */
			    VirginEntry = FALSE;
			}
		      i++; sp++;
		    }
		  if (flags && (VirginEntry == TRUE))
		    {
		      if (ci != (entryp->nsources-1))
			{
			  /* swap colname's source with sourcep */
			  sp = entryp->sourcelist+ci;
			  bcopy((char *)sourcep, (char *)&temp, sizeof(SOURCE));
			  bcopy((char *)sp, (char *)sourcep, sizeof(SOURCE));
			  bcopy((char *)&temp, (char *)sp, sizeof(SOURCE));
			}
		    }
		}
	    }
	}
    }
  
  dbgprint(F_TRACE, (stderr, "DepotDB1_Entry_VirginityStatus done\n"));

  return VirginEntry;
}


Boolean DepotDB1_AntiqueEntry(entryp)
     ENTRY *entryp;
{
  register u_short i;
  register SOURCE *sp;
  Boolean AntiquityFound;
  
  dbgprint(F_TRACE, (stderr, "DepotDB1_AntiqueEntry\n"));

  i = 0; sp = entryp->sourcelist; AntiquityFound = FALSE;
  while ( !AntiquityFound && i < entryp->nsources)
    {
      if (sp->status & S_ANTIQUE) AntiquityFound = TRUE;
      i++; sp++;
    }

  dbgprint(F_TRACE, (stderr, "DepotDB1_AntiqueEntry done\n"));
  return AntiquityFound;
}


/*
 * static ENTRY *DepotDB1_AllocEntry()
 * attempts to allocate storage for an entry in the database tree
 * if successful, returns a pointer to the entry,
 * on failure, emalloc() would have exited with an error condition
 */
static ENTRY *DepotDB1_AllocEntry()
{
  ENTRY *ep;

  ep = (ENTRY *)emalloc(sizeof(ENTRY));
  ENTRY_sibling(ep) = NULL; ENTRY_child(ep) = NULL;
  ep->name = NULL; ep->sourcelist = NULL; ep->nsources = 0; ep->status = 0;

  return ep;
}


/*
 * static void DepotDB1_Free_Subtree(ep)
 *	release all storage associated with the subtree rooted at ep
 */
static void DepotDB1_Free_Subtree(ep)
     ENTRY *ep;
{
  dbgprint(F_TRACE, (stderr, "DepotDB1_Free_Subtree\n"));
  
  if (ep != NULL) {
    free(ep->name);
    DepotDB1_FreeSourceList(ep);
    DepotDB1_Free_Subtree(ENTRY_sibling(ep));
    DepotDB1_Free_Subtree(ENTRY_child(ep));
    free((char *)ep);
  }

  dbgprint(F_TRACE, (stderr, "DepotDB1_Free_Subtree done\n"));
}

/*
 * void DepotDB1_FreeSourceList(sourcelist)
 *	frees memory allocated
 *	for the sourcelist of the entry pointed to by entryp
 */
void DepotDB1_FreeSourceList(entryp)
     ENTRY *entryp;
{
  register u_short i;
  register SOURCE *sp;

  dbgprint(F_TRACE, (stderr, "DepotDB1_FreeSourceList\n"));

  if (entryp->sourcelist != NULL)
    {
      for ( i = 0, sp = entryp->sourcelist; i < entryp->nsources; i++, sp++ )
	{
	  free(sp->name); 
	  sp->name = NULL;
	  free(sp->collection_name);
	  sp->collection_name = NULL;
	}
      free(entryp->sourcelist); 
      entryp->sourcelist = NULL; 
      entryp->nsources = 0;
      entryp->sibling = NULL;
      entryp->child = NULL;
    }
  dbgprint(F_TRACE, (stderr, "DepotDB1_FreeSourceList done\n"));

}


/*
 * static void DepotDB1_QuotedPrintString(file, string)
 *	prints string into the given file, with whitespace, returns
 *	and the quoting character DepotDB_QUOTCHAR
 *	quoted by DepotDB_QUOTCHAR
 */
static void DepotDB1_QuotedPrintString(file, string)
     FILE *file;
     char *string;
{
  register char *cp, *bp;
  char *buffer;

/* sohan says it's faster to dump things into a buffer and then fputs it 
 * than fputc the characters to the output stream
 */
  if (string != NULL)
    {
      buffer = (char *)emalloc(2*strlen(string)+1);
      bp = buffer; cp = string;
      while (*cp != '\0')
	{
	  if ((*cp == ' ') || (*cp == '\t') || (*cp == '\n') || (*cp == DepotDB_QUOTCHAR))
	    { *bp++ = DepotDB_QUOTCHAR; }
	  *bp++ = *cp++;
	}
      *bp = '\0';
      fputs(buffer, file);
      free(buffer);
    }
  return;
}

#ifdef DEBUG

/* print out the sourcelist list of size nlist -- for debugging purposes */
static void dbgDumpSourcelist(list, nlist)
     SOURCE *list;
     u_short nlist;
{
  register u_short i;
  register SOURCE *sp;

  dbgprint(F_DBSOURCEADD, (stderr, "SOURCELIST is"));
  if (nlist == 0)
    {dbgprint(F_DBSOURCEADD, (stderr, " NULL"));}
  else
    for ( i = 0, sp = list; i < nlist; i++, sp++ )
      dbgprint(F_DBSOURCEADD, (stderr, " %s", sp->name));
  dbgprint(F_DBSOURCEADD, (stderr, "\n"));
}

#else DEBUG

static void dbgDumpSourcelist(list, nlist)
     SOURCE *list;
     u_short nlist;
{
}
#endif DEBUG

/* $Source: /afs/andrew.cmu.edu/system/src/local/depot/019/RCS/DepotDBVersion1.c,v $ */
