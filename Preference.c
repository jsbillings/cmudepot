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


static char rcs_id[]="$Header: /afs/andrew.cmu.edu/system/src/local/depot/019/RCS/Preference.c,v 4.11 1992/06/24 18:12:45 ww0r Exp $";


/*
 * Author: Sohan C. Ramakrishna Pillai
 */


#include <stdio.h>
#include <sys/param.h>
#include <math.h> /* for atoi */

#include "globals.h"
#include "PreferenceStruct.h"
#include "PreferenceTypes.h"
#include "Preference.h"


static PREFERENCELIST *PreferenceList_LocatePreference();
static PREFERENCELIST *Preference_Default();
static unsigned PreferenceType();


/*
 * PREFERENCELIST *Preference_Read(f)
 */
PREFERENCELIST *Preference_Read(f)
     FILE *f;
{
  PREFERENCELIST *plist;
  register int ch;
  register char *cp;
  char *itemname, *prefname, *prefval;
  char line[PREFERENCELINESIZE];
  unsigned lineno;
  Boolean eofile, eoline;
  Boolean FoundDot, FoundColon;
  char *DotPosition, *ColonPosition;
  char **strarr;

  dbgprint(F_TRACE, (stderr, "Preference_Read\n"));

  plist = Preference_Default();
  if (f == NULL) {
    return (plist);
  }

  eofile = FALSE;  lineno = 0;
  while ( !eofile )
    {
      itemname = prefname = prefval = NULL;
      /* read a line, noting down the possible DotPosition and the ColonPosition  */
      cp = line; FoundDot = FoundColon = FALSE; eoline = FALSE;
      while (!eoline && !eofile)
	{
	  ch = getc(f);
	  if (ch == EOF)
	    eofile = TRUE;
	  else if (!FoundColon && (ch == COLON))
	    { FoundColon = TRUE; ColonPosition = cp; }
	  else if (!FoundColon && (ch == DOT))
	    { FoundDot = TRUE; DotPosition = cp; }
	  else if (ch == '\n')
	    { eoline = TRUE; lineno++;}
	  if (eoline) *cp = '\0';
	  else *cp++ = (char)ch;
	}
      if (!eofile)
	{
	  if (line[0] == '#'|| line[0] == '\n' || line[0] == '\0')
	    /* comment or blank line, do nothing */ continue;
	  if ( !FoundColon )
	    {
	      FatalError(E_BADPREFFILE, (stderr, "Syntax error while reading preferences file, line %u\n", lineno));
	    }
	  /* set prefval pointer to first non-white character following the colon */
	  prefval = ColonPosition + 1;
	  while (((*prefval == ' ') || (*prefval == '\t')) && (*prefval != '\0'))
	    prefval++;
	  /* set the colon position less whitespace to eostring '\0' */
	  cp = ColonPosition - 1;
	  while ((cp >= line) && ((*cp == ' ') || (*cp == '\t')))
	    cp--;
	  *(cp+1) = '\0';
	  if (FoundDot)
	    { /* line is item.prefname:prefvalue */
	      /* set prefname pointer to the first non-white character folowing the . position */
	      prefname = DotPosition + 1;
	      while (((*prefname == ' ') || (*prefname == '\t')) && (*prefname != '\0'))
		prefname++;
	      /* set . position less whitespace to eostring '\0' */
	      cp = DotPosition - 1;
	      while ((cp >= line) && ((*cp == ' ') || (*cp == '\t')))
		cp--;
	      *(cp+1) = '\0';
	      /* set itemname to beginning of line plus whitespace */
	      itemname = line;
	      while (((*itemname == ' ') || (*itemname == '\t')) && (*itemname != '\0'))
		itemname++;
	      dbgprint(F_PREFREAD, (stderr, "PREFERENCE %s of %s is %s\n", prefname, itemname, prefval));
	    }
	  else /* !FoundDot */
	    { /*  line is prefname:prefvalue */
	      /* set prefname to beginning of line plus whitespace */
	      prefname = line;
	      while (((*prefname == ' ') || (*prefname == '\t')) && (*prefname != '\0'))
		prefname++;
	      itemname = NULL;
	      dbgprint(F_PREFREAD, (stderr, "PREFERENCE %s is %s\n", prefname, prefval));
	    }
	  /* make prefname all lowercase */
	  cp = prefname;
	  while ( *cp != '\0' )
	    {
	      if ((*cp > 'A') && (*cp < 'Z')) *cp -= ('A' - 'a');
	      cp++;
	    }

	  /* depending upon type of preference, add to preferencelist */
	  switch( PreferenceType(prefname) )
	    {
	    case PREF_INT:
	      dbgprint(F_PREFREAD, (stderr, "integer preference value %d\n", atoi(prefval)));
	      plist = Preference_SetInt(plist, itemname, prefname, atoi(prefval));
	      break;
	    case PREF_UNSIGNED:
	      dbgprint(F_PREFREAD, (stderr, "unsigned preference value %u\n", (unsigned)(atol(prefval))));
	      plist = Preference_SetUnsigned(plist, itemname, prefname, (unsigned)(atol(prefval)));
	      break;
	    case PREF_BOOLEAN:
	      dbgprint(F_PREFREAD, (stderr, "boolean preference value %u\n", (unsigned)(atol(prefval))));
	      if ((prefval[0] == 't') || (prefval[0] == 'T'))
		plist = Preference_SetBoolean(plist, itemname, prefname, TRUE);
	      else
		plist = Preference_SetBoolean(plist, itemname, prefname, FALSE);
	      break;
       	    case PREF_STRING:
	      dbgprint(F_PREFREAD, (stderr, "string preference value %s\n", prefval));
	      plist = Preference_SetString(plist, itemname, prefname, prefval);
	      break;
	    case PREF_STRINGARRAY:
	      dbgprint(F_PREFREAD, (stderr, "string array preference value %s\n", prefval));
	      strarr = splitstrarr(prefval, STRINGARRAY_SEPARATOR);
	      plist = Preference_SetStringArray(plist, itemname, prefname, strarr);
	      strarrfree(strarr);	      
	      break;
	    case PREF_STRINGSET:
	      dbgprint(F_PREFREAD, (stderr, "string set preference value %s\n", prefval));
	      strarr = splitstrarr(prefval, STRINGARRAY_SEPARATOR);
	      plist = Preference_SetStringSet(plist, itemname, prefname, strarr);
	      strarrfree(strarr);	      
	      break;
	    default:
	      FatalError(E_BADPREFERENCE, (stderr, "UNKNOWN preference type %s\n", prefname));
	      break;
	    }
	}
    }
  /* eofile is TRUE now */
  dbgprint(F_TRACE, (stderr, "Preference_Read done\n"));
  return plist;
}

/*
 * Boolean Preference_Write(f, plist)
 */
Boolean Preference_Write(f, plist)
     FILE *f;
     PREFERENCELIST *plist;
{
  PREFERENCELIST *pp;
  register char **cpp;

  dbgprint(F_TRACE, (stderr, "Preference_Write\n"));

  for (pp = plist; pp != NULL; pp = PREFERENCELIST_next(pp))
    {
      if ((pp->preference.ItemName != NULL)
	  && (strcmp(pp->preference.ItemName, "") != 0))
	fprintf(f, "%s%c", pp->preference.ItemName, DOT);
      fprintf(f, "%s%c", pp->preference.PreferenceName, COLON);
      switch(pp->preference.type)
	{
	case PREF_INT:
	  fprintf(f, "%d", pp->preference.value.ival);
	  break;
	case PREF_UNSIGNED:
	  fprintf(f, "%u", pp->preference.value.uval);
	  break;
	case PREF_STRING:
	  fprintf(f, "%s", pp->preference.value.strval);
	  break;
       	case PREF_STRINGARRAY:
       	case PREF_STRINGSET:
	  cpp = pp->preference.value.strarrval;
	  fprintf(f, "%s", *cpp); cpp++; /* first one without a separator */
	  while (*cpp != NULL)
	    {
	      fprintf(f, "%c%s", STRINGARRAY_SEPARATOR, *cpp);
	      cpp++;
	    }
	  break;
	}
      fprintf(f, "\n"); /* print a new line */
    }

  dbgprint(F_TRACE, (stderr, "Preference_Write done\n"));
  return TRUE;
}

/*
 * int Preference_GetInt(plist, item, prefname)
 *	returns integer value for preference prefname of item
 *	from preference list plist
 */
int Preference_GetInt(plist, item, prefname)
     PREFERENCELIST *plist;
     char *item, *prefname;
{
  PREFERENCELIST *pp;
  int prefval;

  dbgprint(F_TRACE, (stderr, "Preference_GetInt\n"));

  pp = PreferenceList_LocatePreference(&plist, item, prefname, PL_LOCATE);
  if ((pp == NULL) && (item != NULL))
    pp = PreferenceList_LocatePreference(&plist, "*", prefname, PL_LOCATE);
  if (pp == NULL)
    prefval = PREFDEFAULT_INT;
  else
    prefval = pp->preference.value.ival;

  dbgprint(F_TRACE, (stderr, "Preference_GetInt done\n"));
  return prefval;
}
/*
 * unsigned Preference_GetUnsigned(plist, item, prefname)
 *	returns unsigned value for preference prefname of item
 *	from preference list plist
 */
unsigned Preference_GetUnsigned(plist, item, prefname)
     PREFERENCELIST *plist;
     char *item, *prefname;
{
  PREFERENCELIST *pp;
  unsigned prefval;

  dbgprint(F_TRACE, (stderr, "Preference_GetUnsigned\n"));

  pp = PreferenceList_LocatePreference(&plist, item, prefname, PL_LOCATE);
  if ((pp == NULL) && (item != NULL))
    pp = PreferenceList_LocatePreference(&plist, "*", prefname, PL_LOCATE);
  if (pp == NULL)
    prefval = PREFDEFAULT_UNSIGNED;
  else
    prefval = pp->preference.value.uval;

  dbgprint(F_TRACE, (stderr, "Preference_GetUnsigned done\n"));
  return prefval;
}
/*
 * Boolean Preference_GetBoolean(plist, item, prefname)
 *	returns boolean value for preference prefname of item
 *	from preference list plist
 */
Boolean Preference_GetBoolean(plist, item, prefname)
     PREFERENCELIST *plist;
     char *item, *prefname;
{
  PREFERENCELIST *pp;
  Boolean prefval;

  dbgprint(F_TRACE, (stderr, "Preference_GetBoolean\n"));

  pp = PreferenceList_LocatePreference(&plist, item, prefname, PL_LOCATE);
  if ((pp == NULL) && (item != NULL))
    pp = PreferenceList_LocatePreference(&plist, "*", prefname, PL_LOCATE);
  if (pp == NULL)
    prefval = PREFDEFAULT_BOOLEAN;
  else
    prefval = pp->preference.value.boolval;

  dbgprint(F_TRACE, (stderr, "Preference_GetBoolean done\n"));
  return prefval;
}
/*
 * char *Preference_GetString(plist, item, prefname)
 *	returns string for preference prefname of item
 *	from preference list plist
 */
char *Preference_GetString(plist, item, prefname)
     PREFERENCELIST *plist;
     char *item, *prefname;
{
  PREFERENCELIST *pp;
  char *prefval;

  dbgprint(F_TRACE, (stderr, "Preference_GetString\n"));

  pp = PreferenceList_LocatePreference(&plist, item, prefname, PL_LOCATE);
  if ((pp == NULL) && (item != NULL))
    pp = PreferenceList_LocatePreference(&plist, "*", prefname, PL_LOCATE);
  if (pp == NULL)
    prefval = PREFDEFAULT_STRING;
  else
    prefval = pp->preference.value.strval;

  dbgprint(F_TRACE, (stderr, "Preference_GetString done\n"));
  return prefval;
}
/*
 * char **Preference_GetStringArray(plist, item, prefname)
 *	returns array of strings for preference prefname of item
 *	from preference list plist
 */
char **Preference_GetStringArray(plist, item, prefname)
     PREFERENCELIST *plist;
     char *item, *prefname;
{
  PREFERENCELIST *pp;
  char **prefval;

  dbgprint(F_TRACE, (stderr, "Preference_GetStringArray\n"));

  pp = PreferenceList_LocatePreference(&plist, item, prefname, PL_LOCATE);
  if ((pp == NULL) && (item != NULL))
    pp = PreferenceList_LocatePreference(&plist, "*", prefname, PL_LOCATE);
  if (pp == NULL)
    prefval = PREFDEFAULT_STRINGARRAY;
  else
    prefval = pp->preference.value.strarrval;

  dbgprint(F_TRACE, (stderr, "Preference_GetStringArray done\n"));
  return prefval;
}
/*
 * char **Preference_GetStringSet(plist, item, prefname)
 *	returns array of strings for preference prefname of item
 *	from preference list plist
 */
char **Preference_GetStringSet(plist, item, prefname)
     PREFERENCELIST *plist;
     char *item, *prefname;
{
  PREFERENCELIST *pp;
  char **prefval;

  dbgprint(F_TRACE, (stderr, "Preference_GetStringSet\n"));

  pp = PreferenceList_LocatePreference(&plist, item, prefname, PL_LOCATE);
  if ((pp == NULL) && (item != NULL))
    pp = PreferenceList_LocatePreference(&plist, "*", prefname, PL_LOCATE);
  if (pp == NULL)
    prefval = PREFDEFAULT_STRINGSET;
  else
    prefval = pp->preference.value.strarrval;

  dbgprint(F_TRACE, (stderr, "Preference_GetStringSet done\n"));
  return prefval;
}

/*
 * PREFERENCELIST *Preference_SetInt(plist, item, prefname, prefval)
 */
PREFERENCELIST *Preference_SetInt(plist, item, prefname, prefval)
     PREFERENCELIST *plist;
     char *item, *prefname;
     int prefval;
{
  PREFERENCELIST **plistp;
  PREFERENCELIST *pp;

  dbgprint(F_TRACE, (stderr, "Preference_SetInt\n"));

  plistp = &plist;
  pp = PreferenceList_LocatePreference(plistp, item, prefname, PL_LOCATE | PL_CREATE);
  if (pp == NULL)
    {FatalError(E_PREFSETFAILED, (stderr, "Could not set preference %s for %s\n", item, prefname));}
  pp->preference.type = PREF_INT; pp->preference.value.ival = prefval;

  dbgprint(F_TRACE, (stderr, "Preference_SetInt done\n"));
  return *plistp;
}
/*
 * PREFERENCELIST *Preference_SetUnsigned(plist, item, prefname, prefval)
 */
PREFERENCELIST *Preference_SetUnsigned(plist, item, prefname, prefval)
     PREFERENCELIST *plist;
     char *item, *prefname;
     unsigned prefval;
{
  PREFERENCELIST **plistp;
  PREFERENCELIST *pp;

  dbgprint(F_TRACE, (stderr, "Preference_SetUnsigned\n"));

  plistp = &plist;
  pp = PreferenceList_LocatePreference(plistp, item, prefname, PL_LOCATE | PL_CREATE);
  if (pp == NULL)
    {FatalError(E_PREFSETFAILED, (stderr, "Could not set preference %s for %s\n", item, prefname));}
  pp->preference.type = PREF_UNSIGNED; pp->preference.value.uval = prefval;

  dbgprint(F_TRACE, (stderr, "Preference_SetUnsigned done\n"));
  return *plistp;
}
/*
 * PREFERENCELIST *Preference_SetBoolean(plist, item, prefname, prefval)
 */
PREFERENCELIST *Preference_SetBoolean(plist, item, prefname, prefval)
     PREFERENCELIST *plist;
     char *item, *prefname;
     Boolean prefval;
{
  PREFERENCELIST **plistp;
  PREFERENCELIST *pp;

  dbgprint(F_TRACE, (stderr, "Preference_SetBoolean\n"));

  plistp = &plist;
  pp = PreferenceList_LocatePreference(plistp, item, prefname, PL_LOCATE | PL_CREATE);
  if (pp == NULL)
    {FatalError(E_PREFSETFAILED, (stderr, "Could not set preference %s for %s\n", item, prefname));}
  pp->preference.type = PREF_BOOLEAN; pp->preference.value.boolval = prefval;

  dbgprint(F_TRACE, (stderr, "Preference_SetBoolean done\n"));
  return *plistp;
}
/*
 * PREFERENCELIST *Preference_SetString(plist, item, prefname, prefval)
 */
PREFERENCELIST *Preference_SetString(plist, item, prefname, prefval)
     PREFERENCELIST *plist;
     char *item, *prefname;
     char *prefval;
{
  PREFERENCELIST **plistp;
  PREFERENCELIST *pp;

  dbgprint(F_TRACE, (stderr, "Preference_SetString\n"));

  plistp = &plist;
  pp = PreferenceList_LocatePreference(plistp, item, prefname, PL_LOCATE | PL_CREATE);
  if (pp == NULL)
    {FatalError(E_PREFSETFAILED, (stderr, "Could not set preference %s for %s\n", item, prefname));}
  pp->preference.type = PREF_STRING;
  pp->preference.value.strval = (char *)emalloc(strlen(prefval) + 1);
  pp->preference.value.strval = strcpy(pp->preference.value.strval, prefval);

  dbgprint(F_TRACE, (stderr, "Preference_SetString done\n"));
  return *plistp;
}
/*
 * PREFERENCELIST *Preference_SetStringArray(plist, item, prefname, prefval)
 */
PREFERENCELIST *Preference_SetStringArray(plist, item, prefname, prefval)
     PREFERENCELIST *plist;
     char *item, *prefname;
     char **prefval;
{
  PREFERENCELIST **plistp;
  PREFERENCELIST *pp;

  dbgprint(F_TRACE, (stderr, "Preference_SetStringArray\n"));

  plistp = &plist;
  pp = PreferenceList_LocatePreference(plistp, item, prefname, PL_LOCATE | PL_CREATE);
  if (pp == NULL)
    {FatalError(E_PREFSETFAILED, (stderr, "Could not set preference %s for %s\n", item, prefname));}
  pp->preference.type = PREF_STRINGARRAY;
  pp->preference.value.strarrval = (char **)ecalloc((unsigned)(strarrsize(prefval)+1), sizeof(char *));
  pp->preference.value.strarrval = strarrcpy(pp->preference.value.strarrval, prefval);

  dbgprint(F_TRACE, (stderr, "Preference_SetStringArray done\n"));
  return *plistp;
}
/*
 * PREFERENCELIST *Preference_SetStringSet(plist, item, prefname, prefval)
 */
PREFERENCELIST *Preference_SetStringSet(plist, item, prefname, prefval)
     PREFERENCELIST *plist;
     char *item, *prefname;
     char **prefval;
{
  register char **sp;
  PREFERENCELIST **plistp;
  PREFERENCELIST *pp;

  dbgprint(F_TRACE, (stderr, "Preference_SetStringSet\n"));

  plistp = &plist;
  pp = PreferenceList_LocatePreference(plistp, item, prefname, PL_LOCATE | PL_CREATE);
  if (pp == NULL)
    {FatalError(E_PREFSETFAILED, (stderr, "Could not set preference %s for %s\n", item, prefname));}
  pp->preference.type = PREF_STRINGSET;
  sp = prefval;
  if (sp != NULL)
    {
      while (*sp != NULL)
	{
	  pp->preference.value.strarrval = sortedstrarrinsert(pp->preference.value.strarrval, *sp);
	  sp++;
	}
    }

  dbgprint(F_TRACE, (stderr, "Preference_SetStringSet done\n"));
  return *plistp;
}


/*
 * static PREFERENCELIST *PreferenceList_LocatePreference(plistp, item, prefname, searchflags)
 * attempts to locate the entry for preference prefname of item
 * in the list pointed to by plistp.
 * plistp is an IN-OUT parameter, which maintains the pointer to
 * the head of the preference list.
 * searchflags is used to control the navigation.
 * if PL_CREATE is set
 *	a new entry is added to the preference list, if necessary.
 * else NULL is returned to indicate absence of entry
 * NOTE: the list is sorted primarily in increasing lexicographical order of
 * item name and then in increasing lexicographical order of preference name.
 */
static PREFERENCELIST *PreferenceList_LocatePreference(plistp, item, prefname, searchflags)
     PREFERENCELIST **plistp;
     char *item, *prefname;
     Boolean searchflags;
{
  register PREFERENCELIST *pcurr, *pprev;
  Boolean FoundPreference, PastPreference;
  int ItemComparison, PreferenceComparison;
  PREFERENCELIST *prefentry;
  int preftype;
  register char *itemptr;

  dbgprint(F_TRACE, (stderr, "PreferenceList_LocatePreference\n"));
  if (prefname == NULL)
    {FatalError(E_BADPREFERENCE, (stderr, "Attempt to locate NULL preference\n"));}
  preftype = PreferenceType(prefname);
  if (preftype == PREF_UNKNOWN)
    {FatalError(E_BADPREFERENCE, (stderr, "Attampt to locate unknown preference type %s\n", prefname));}

  pcurr = *plistp; pprev = NULL; FoundPreference = FALSE; PastPreference = FALSE;
  if (item == NULL) 
    itemptr = "";
  else 
    itemptr = item;
  
  while ( !FoundPreference && !PastPreference && (pcurr != NULL ))
    {
      /* search first for item and then for prefname with the same item */
      ItemComparison = strcmp(pcurr->preference.ItemName, itemptr);
      if (ItemComparison < 0)
	{ pprev = pcurr; pcurr = PREFERENCELIST_next(pcurr); }
      else if (ItemComparison > 0)
	PastPreference = TRUE;
      else /* if (ItemComparison == 0) */
	{
	  PreferenceComparison = strcmp(pcurr->preference.PreferenceName, prefname);
	  if (PreferenceComparison < 0)
	    { pprev = pcurr; pcurr = PREFERENCELIST_next(pcurr); }
	  else if (PreferenceComparison > 0)
	    PastPreference = TRUE;
	  else /* if PreferenceComparison == 0) */
	    FoundPreference = TRUE;
	}
    }

  if (FoundPreference)
    prefentry = pcurr;
  else if (searchflags & PL_CREATE) /* PastPreference || pcurr == NULL */
    {
      prefentry = (PREFERENCELIST *)emalloc(sizeof(PREFERENCELIST));
      if (item == NULL)
	prefentry->preference.ItemName = "";
      else
	{
	  prefentry->preference.ItemName = (char *)emalloc(strlen(item)+1);
	  prefentry->preference.ItemName = strcpy(prefentry->preference.ItemName, item);
	}
      prefentry->preference.PreferenceName = (char *)emalloc(strlen(prefname)+1);
      prefentry->preference.PreferenceName = strcpy(prefentry->preference.PreferenceName, prefname);
      prefentry->preference.type = preftype;
      switch(preftype)
	{
	case PREF_INT:
	  prefentry->preference.value.ival = PREFDEFAULT_INT;
	  break;
	case PREF_UNSIGNED:
	  prefentry->preference.value.uval = PREFDEFAULT_UNSIGNED;
	  break;
	case PREF_BOOLEAN:
	  prefentry->preference.value.boolval = PREFDEFAULT_BOOLEAN;
	  break;
	case PREF_STRING:
	  prefentry->preference.value.strval = PREFDEFAULT_STRING;
	  break;
	case PREF_STRINGARRAY:
	  prefentry->preference.value.strarrval = PREFDEFAULT_STRINGARRAY;
	  break;
	case PREF_STRINGSET:
	  prefentry->preference.value.strarrval = PREFDEFAULT_STRINGSET;
	  break;
	default:
	  FatalError(E_BADPREFERENCE, (stderr, "UNKNOWN preference type %s\n", prefname));
	  break;
	}
      PREFERENCELIST_next(prefentry) = pcurr;
      if (pprev == NULL) /* prefentry was prepended to first entry */
	*plistp = prefentry;
      else
	PREFERENCELIST_next(pprev) = prefentry;
    }
  else /* PastPreference || pcurr == NULL */
    prefentry = NULL;

  dbgprint(F_TRACE, (stderr, "PreferenceList_LocatePreference done\n"));
  return prefentry;
}



/*
 * static unsigned PreferenceType()
 */
static unsigned PreferenceType(prefname)
     char *prefname;
{
  struct PrefTypePair *pp;
  Boolean FoundPreference;
  unsigned i;
  unsigned preftype;

  dbgprint(F_TRACE, (stderr, "PreferenceType\n"));

  if (prefname == NULL)
    {FatalError(E_BADPREFERENCE, (stderr, "PreferenceType: Attempt to obtain type of NULL preference!\n"));}
  FoundPreference = FALSE; preftype = PREF_UNKNOWN;
  pp = PrefTypeList; i = 0;
  while ( !FoundPreference && (i < NPrefTypes) )
    {
      if (strcmp(prefname, pp->prefname) == 0)
	{ FoundPreference = TRUE; preftype = pp->preftype; }
      else
	{ pp++; i++; }
    }

  dbgprint(F_TRACE, (stderr, "PreferenceType done\n"));
  return preftype;
}



static PREFERENCELIST *Preference_Default()
{
  PREFERENCELIST *plist;

  plist = NULL;
  plist = Preference_SetBoolean(plist, NULL, "deleteunreferenced", TRUE);
  plist = Preference_SetString(plist, "*", "mapcommand", "symlink");
  plist = Preference_SetInt(plist, "*", "version", -1);
  plist = Preference_SetString(plist, NULL, "versiondelimiter", "~");

  return plist;
}

/* $Source: /afs/andrew.cmu.edu/system/src/local/depot/019/RCS/Preference.c,v $ */
