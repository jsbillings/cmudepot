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


static char rcs_id[]="$Header: /afs/andrew.cmu.edu/system/src/local/depot/019/RCS/DepotDBCommand.c,v 4.6 1992/06/23 17:10:01 ww0r Exp $";


/*
 * Author: Sohan C. Ramakrishna Pillai
 */



#include <stdio.h>
#include "globals.h"
#include "depot.h"
#include "DepotDBStruct.h"
#include "DepotDB.h"
#include "DepotDBCommandStruct.h"
#include "DepotDBCommand.h"


/*
 * void DepotDB_Command_Insert(CommandList, entryp)
 *	Inserts command from the source for entryp
 *	into a uniquely sorted NULL-terminated array of commands
 */
void DepotDB_Command_Insert(CommandList, entryp)
     COMMANDFILE ***CommandList;
     ENTRY *entryp;
{
  register COMMANDFILE **fp, **tp;
  register u_short i;
  register SOURCE *sp;
  SOURCE *sourcep;
  char *commandlabel;
  u_short InsertPoint;
  int LabelComparison, ListSize;
  Boolean DuplicateLabel, LocatedInsertPoint;

  dbgprint(F_TRACE, (stderr, "DepotDB_Command_Insert\n"));

  if ((entryp == NULL) || ((sourcep=entryp->sourcelist+(entryp->nsources-1)) == NULL))
    { FatalError(E_NULLCMDSOURCE, (stderr, "DepotDB_Command_Insert: Attempt to insert command from NULL source\n")); }
  commandlabel = sourcep->name;
  if (commandlabel == NULL)
    { FatalError(E_NULLCMDLABEL, (stderr, "DepotDB_Command_Insert: Attempt to insert NULL commandlabel into a commandfile list\n")); }

  if (*CommandList == NULL)
    { /* allocate array of length 1 */
      *CommandList = (COMMANDFILE **)emalloc(sizeof(COMMANDFILE *));
      **CommandList = NULL;
    }

  i = 0; fp = *CommandList; DuplicateLabel = FALSE; LocatedInsertPoint = FALSE;
  while (!LocatedInsertPoint && !DuplicateLabel && (*fp != NULL))
    {
      LabelComparison = strcmp((*fp)->label, commandlabel);
      if (LabelComparison == 0)
	{DuplicateLabel = TRUE; InsertPoint = i;}
      else if (LabelComparison > 0 )
	{LocatedInsertPoint = TRUE; InsertPoint = i;}
      else /* LabelComparison < 0 */
	{ i++; fp++; }
    }

  if ( !DuplicateLabel )
    {
      /* compute size of CommandList and realloc to a larger size */
      i = 0; fp = *CommandList;
      while (*fp++ != NULL)
	i++;
      ListSize = i + 1;

      if (!LocatedInsertPoint) InsertPoint = (u_short)ListSize -1 ;
      ListSize++; *CommandList = (COMMANDFILE **)erealloc((char *)(*CommandList), (unsigned)ListSize*sizeof(COMMANDFILE *));
      i = ListSize - 1; fp = (*CommandList)+i-1; tp = (*CommandList)+i;
      while (i > InsertPoint)
	{
	  *tp-- = *fp--; i--;
	}
      fp = (*CommandList) + InsertPoint;
      *fp = (COMMANDFILE *)emalloc(sizeof(COMMANDFILE));
      (*fp)->label = (char *)emalloc(strlen(commandlabel)+1);
      (*fp)->label = strcpy((*fp)->label, commandlabel);
      (*fp)->command = NULL; (*fp)->collection_list = NULL;
      (*fp)->status = sourcep->status;
    }
  else /* DuplicateLabel */
    {
      fp = (*CommandList) + InsertPoint;
      /* if (!(sourcep->status & S_TARGET)) (*fp)->status &= ~S_TARGET; */
      (*fp)->status &= (sourcep->status | ~S_TARGET);
    }

  fp = (*CommandList) + InsertPoint;
  for (i = 0, sp = entryp->sourcelist; i < entryp->nsources; i++, sp++)
    {
      if ((sp->update_spec & U_RCFILE) && (strcmp(sp->name, (*fp)->label) == 0))
	{ (*fp)->collection_list = sortedstrarrinsert((*fp)->collection_list, sp->collection_name); }
    }

  /* dbgDumpStringArray(*CommandList); */
  dbgprint(F_TRACE, (stderr, "DepotDB_Command_Insert done\n"));
  return;
}



/*
 * char **DepotDB_Command_BuildDepotDBTargetCommand()
 *	returns the command "ExecFilePath Depot_TargetPath"
 *	as an array of strings terminated by (char *)0
 *	suitable for passing to execv as char *argv[].
 */
char **DepotDB_Command_BuildDepotDBTargetCommand(ExecFilePath)
     char *ExecFilePath;
{
  char **av;

  dbgprint(F_TRACE, (stderr, "DepotDB_Command_BuildDepotDBTargetCommand -- being implemented\n"));
  av = (char **)ecalloc(3,  sizeof(char *));
  av[0] = (char *)emalloc(strlen(ExecFilePath)+1);
  (void)strcpy(av[0], ExecFilePath);
  av[1] = (char *)emalloc(strlen(Depot_TargetPath)+1);
  (void)strcpy(av[1], Depot_TargetPath);
  av[2] = (char *)0;

  dbgprint(F_TRACE, (stderr, "DepotDB_Command_BuildDepotDBTargetCommand done\n"));
  return av;
}


/*
 * char **DepotDB_Command_ExpandMagic(comarr)
 *	expands instances of %t in strings in comarr
 *	to the Depot_TargetPath.
 *	May be expanded if necessary to accept more
 *	expand more magic mappings via an argument or arg-list.
 */
char **DepotDB_Command_ExpandMagic(comarr)
     char **comarr;
{
  register char **fp, **tp;
  register char *cp, *bp;
  char **av;
  char buffer[BUFSIZ];
  Boolean QuotedChar;

  dbgprint(F_TRACE, (stderr, "DepotDB_ExpandMagic -- being implemented\n"));

  if (comarr == NULL)
    { FatalError(E_NULLSTRARRAY, (stderr, "attempt to expand magic from NULL array of strings\n")); }
  av = (char **)ecalloc((unsigned)(strarrsize(comarr)+1), sizeof(char *));
  fp = comarr; tp = av;
  while ((*fp != NULL) && (*fp != (char *)0))
    {
      cp = *fp; bp = buffer; QuotedChar = FALSE;
      while (*cp != '\0')
	{
	  if (QuotedChar)
	    { *bp++ = *cp++; QuotedChar = FALSE; }
	  else
	    {
	      if ((*cp == '%') && (*(cp+1) == 't'))
		{
		  (void)sprintf(bp, "%s", Depot_TargetPath);
		  cp += 2; bp += strlen(Depot_TargetPath);
		}
	      else if (*cp == DepotDB_QUOTCHAR)
		{ *bp++ = *cp++; QuotedChar = TRUE; }
	      else
		{ *bp++ = *cp++; }
	    }
	}
      *bp = '\0';
      *tp = (char *)emalloc(strlen(buffer)+1);
      *tp = strcpy(*tp, buffer);
      dbgprint(F_EXECRCFILE, (stderr, "MAGIC %s => %s\n", *fp, *tp));
      fp++; tp++;
    }
  *tp = (char *)0;

  dbgprint(F_TRACE, (stderr, "DepotDB_ExpandMagic done\n"));
  return av;
}

/* $Source: /afs/andrew.cmu.edu/system/src/local/depot/019/RCS/DepotDBCommand.c,v $ */
