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


static char rcs_id[]="$Header: /afs/andrew.cmu.edu/system/src/local/depot/019/RCS/PreferenceUtil.c,v 4.6 1992/07/28 18:30:29 sohan Exp $";


/*
 * Author: Sohan C. Ramakrishna Pillai
 */


#include <stdio.h>
#include <sys/param.h>

#include "globals.h"
#include "PreferenceStruct.h"
#include "PreferenceTypes.h"
#include "Preference.h"
#include "DepotDBStruct.h"
#include "DepotDB.h"


/*
 * Boolean Override(c1, c2, preflist)
 *	if ( c1 overrides c2 )
 *	  returns TRUE
 *	else
 *	  returns FALSE;
 */
Boolean Override(c1, c2, preflist)
     char *c1, *c2;
     PREFERENCELIST *preflist;
{
  register char **op;
  char **overridelist;
  Boolean FoundOverride;

  overridelist = Preference_GetStringArray(preflist, c1, "override");
  if (overridelist == NULL)
    FoundOverride = FALSE;
  else
    {
      /* search for c2 in the overridelist */
      op = overridelist;
      FoundOverride = FALSE;
      while (!FoundOverride && (*op != NULL))
	{
	  if (strcmp(*op, c2) == 0)
	    FoundOverride = TRUE;
	  op++;
	}
    }
  if (!FoundOverride && (strcmp(c1, "*") != 0))
    FoundOverride = Override("*", c2, preflist);

  return FoundOverride;
}



/*
 * Boolean Ignore(cname, preflist)
 *	if collection cname is to be ignored as per preferences in preflist
 *	  return TRUE;
 *	else
 *	  return FALSE;
 */
Boolean Ignore(cname, preflist)
     char *cname;
     PREFERENCELIST *preflist;
{
  register char **cp;
  char **ignorelist;
  Boolean FoundCollection; /* true if cname is found in the ignorelist */

  FoundCollection = FALSE;
  if ((cname != NULL)
      && ((ignorelist = Preference_GetStringArray(preflist, NULL, "ignore")) != NULL))
    {
      FoundCollection = FALSE; cp = ignorelist;
      while ( !FoundCollection && (*cp != NULL))
	{
	  if (strcmp(*cp, cname) == 0)
	    FoundCollection = TRUE;
	  cp++;
	}
    }
  return FoundCollection;
}

/*
 * int GetMapCommand(cname, preflist)
 *	returns the mapcommand preference for collection cname as
 *	U_COPY or U_LINK(default)
 */
int GetMapCommand(cname, preflist)
     char *cname;
     PREFERENCELIST *preflist;
{
  register char *cp;
  char *mapcommand, buffer[PREFERENCELINESIZE];
  int MapCommandType;

  mapcommand = Preference_GetString(preflist, cname, "mapcommand");
  if (mapcommand == NULL)
    MapCommandType = U_MAPLINK;
  else
    {
      (void)strcpy(buffer, mapcommand);
      cp = buffer;
      while ( *cp != '\0' )
	{
	  if ((*cp > 'A') && (*cp < 'Z')) *cp -= ('A' - 'a');
	  cp++;
	}
      if (strcmp(buffer, "copy") == 0)
	MapCommandType = U_MAPCOPY;
      else if ((strcmp(buffer, "link") == 0) || (strcmp(buffer, "symlink") == 0))
	MapCommandType = U_MAPLINK;
      else
	MapCommandType = U_MAPLINK;
    }
  return MapCommandType;
}



int VersionToUse(cname, preflist)
     char *cname;
     PREFERENCELIST *preflist;
{
  return Preference_GetInt(preflist, cname, "version");
}


/*
 * char **Preference_ExtractCommand(label, preflist)
 *	returns the actual command corresponding to the given label
 *	as an array of strings terminated by (char *)0
 *	suitable for passing to execv as char *argv[].
 *	If no command is found NULL is returned.
 */
char **Preference_ExtractCommand(label, preflist)
     char *label;
     PREFERENCELIST *preflist;
{
  register char **fp, **tp;
  char *commandstring;
  char **argval;
  unsigned nremoved;

  dbgprint(F_TRACE, (stderr, "Preference_ExtractCommand\n"));
  commandstring = Preference_GetString(preflist, label, "command");
  if (commandstring == NULL)
    argval = NULL;
  else
    {
      argval = splitstrarr(commandstring, ' ');
      /* squeeze out any empty argvals left by multiple spaces */
      if (argval != NULL)
	{
	  fp = argval; tp = argval; nremoved = 0;
	  while ( *fp != NULL )
	    {
	      if ((*fp)[0] == '\0')
		{
		  free(*fp); *fp = NULL;
		  nremoved++;
		}
	      else
		{
		  if (nremoved != 0)
		    { *tp = *fp; *fp = NULL; }
		  tp++;
		}
	      fp++;
	    }
	  *tp = (char*)0;
	}
    }
  dbgprint(F_TRACE, (stderr, "Preference_ExtractCommand done\n"));
  return argval;
}


char **Preference_GetItemList(preflist)
     PREFERENCELIST *preflist;
{
  register PREFERENCELIST *pp;
  char *previtem;
  char **itemlist;

  pp = preflist; previtem = ""; itemlist = NULL;
  while (pp!= NULL)
    {
      if (pp->preference.ItemName == NULL)
	previtem = "";
      else
	{
	  if ((strcmp(previtem, pp->preference.ItemName) != 0)
	      && (strcmp("command", pp->preference.PreferenceName) != 0))
	    { /* new item name and not a command preference */
	      if ((strcmp("", pp->preference.ItemName) != 0) && (strcmp("*", pp->preference.ItemName) != 0))
		{ /* not a special character or NULL item */
		  /* add this item to the itemlist */
		  itemlist = sortedstrarrinsert(itemlist, pp->preference.ItemName);
		}
	      previtem = pp->preference.ItemName;
	    }
	}
      pp = PREFERENCELIST_next(pp);
    }
  return itemlist;
}

/* $Source: /afs/andrew.cmu.edu/system/src/local/depot/019/RCS/PreferenceUtil.c,v $ */
