/* 
 * mm.c --
 *
 *	This module implements command structure lookups.
 *	This is short for MajorMinor.
 *
 * Copyright (c) 1997,1998 Jeffrey Hobbs
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include "mm.h"

static Tcl_HashTable *majminNames = NULL;

/* local prototypes */
static void	MajminDeletedCmd _ANSI_ARGS_((ClientData clientData));

/*
 * MajorMinor_Cmd structure lookup functions
 * See bottom for how a general command would be defined
 */

int
MM_GetProcExact(const MajorMinor_Cmd *cmds, const char *arg,
		Tcl_CmdProc **proc)
{
  unsigned int len = strlen(arg);
  for(;cmds->name && cmds->name[0];cmds++) {
    if (!strncmp(cmds->name,arg,len)) {
      *proc = cmds->proc;
      return cmds->type;
    }
  }
  return MM_ERROR;
}

void
MM_GetError(Tcl_Interp *interp, const MajorMinor_Cmd *cmds, const char *arg)
{
  int i;
  Tcl_AppendResult(interp, "bad option \"", arg, "\" must be ", (char *) 0);
  for(i=0; cmds->name && cmds->name[0]; cmds++,i++) {
    Tcl_AppendResult(interp, (i?", ":""), cmds->name, (char *) 0);
  }
}

/*
 * Parses a command string passed in an arg comparing it with all the
 * command strings in the command array. If it finds a string which is a
 * unique identifier of one of the commands, returns the index . If none of
 * the commands match, or the abbreviation is not unique, then it sets up
 * the message accordingly and returns 0
 */

int
MM_GetProc(Tcl_Interp *interp, MajorMinor_Cmd *cmds, const char *arg,
	   MajorMinor_Cmd **cmd)
{
  unsigned int len;
  MajorMinor_Cmd *matched = (MajorMinor_Cmd *) 0;
  int multiple_found = 0;
  MajorMinor_Cmd *next = cmds;

  if (arg == NULL) goto badarg;
  len = strlen(arg);
  while (next->name && *(next->name)) {
    if (strncmp(next->name, arg, len) == 0) {
      /* have we already matched this one if so make up an error message */
      if (matched) {
	if (!multiple_found) {
	  Tcl_AppendResult(interp, "ambiguous option \"", arg,
			   "\" could be ", matched->name, (char *) NULL);
	  matched = next;
	  multiple_found = 1;
	}
	Tcl_AppendResult(interp, ", ", next->name, (char *) NULL);
      } else {
	matched = next;
	/* return on an exact match */
	if (len == (int)strlen(next->name)) {
	  *cmd = matched;
	  return (matched->type);
	}
      }
    }
    next++;
  }
  /* did we get multiple possibilities? */
  if (multiple_found) {
    return MM_ERROR;
  }
  /* did we match anything at all? */
  if (matched) {
    *cmd = matched;
    return (matched->type);
  } else if (next->proc != NULL && next->type == MM_PROC) {
    /* The last cmd is a proc, meaning it will handle the rest of the cases */
    *cmd = next;
    return next->type;
  } else {
  badarg:
    Tcl_AppendResult(interp, "bad option \"", (arg==NULL)?"":arg,
		     "\" must be ", (char *) NULL);
    next = cmds;
    while (next->name && *(next->name)) {
      Tcl_AppendResult(interp, next->name, (char *) NULL);
      next++;
      if ((next+1)->name && *((next+1)->name))
	Tcl_AppendResult(interp, ", ", (char *) NULL);
      else if (next->name && *(next->name))
	Tcl_AppendResult(interp, " or ", (char *) NULL);
    }
    return MM_ERROR;
  }
  /* SHOULD NEVER BE REACHED */
  return MM_ERROR;
}

int
MM_HandleArgs(ClientData clientData, Tcl_Interp *interp, MajorMinor_Cmd *cmds, 
	      int argc, char **argv)
{
  MajorMinor_Cmd *cmd = NULL;
  int type, arg = 1, result = TCL_OK;

  if (argc < 2) {
    Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
		     " option ?arg arg ...?\"", (char *) NULL);
    return TCL_ERROR;
  }
  Tcl_Preserve(clientData);
  type = MM_GetProc(interp, cmds, argv[arg++], &cmd);

  /* Switch on the parameter value */
reswitch:
  switch (type) {
  case MM_SUBPROC:
    type = MM_GetProc(interp, (MajorMinor_Cmd *)cmd->proc,
		      ((arg>=argc)?(char *)NULL:argv[arg++]), &cmd);
    goto reswitch;

  case MM_PROC:
    result = (cmd->proc)(clientData, interp, argc, argv);
    break;

    /* no MM_VALUE here, it doesn't make sense */
  case MM_ERROR:
  default:
    result = TCL_ERROR;
    break;
  }

  Tcl_Release(clientData);
  return result;
}

int
MM_HandleCmds(ClientData clientData, Tcl_Interp *interp,
	      int argc, char **argv)
{
  MajorMinor_Cmd *cmdPtr = (MajorMinor_Cmd *) clientData;
  MajorMinor_Cmd *cmd = NULL, *cmds = (MajorMinor_Cmd *) cmdPtr->proc;
  int result = TCL_OK;
  int arg, type;

  arg = 1;
  type = MM_GetProc(interp, cmds, argv[arg++], &cmd);

  /* Switch on the parameter value */
reswitch:
  switch (type) {
  case MM_PROC:
    result = (cmd->proc)(cmdPtr->data, interp, argc, argv);
    break;

  case MM_SUBPROC:
    type = MM_GetProc(interp, (MajorMinor_Cmd *)cmd->proc,
		      ((arg>=argc)?(char *)NULL:argv[arg++]), &cmd);
    goto reswitch;

    /* no MM_VALUE here, it doesn't make sense */
  case MM_ERROR:
  default:
    result = TCL_ERROR;
    break;
  }

  return result;
}

static int
MM_SortCmds(first, second)
    CONST VOID *first, *second;		/* Elements to be compared. */
{
  MajorMinor_Cmd *one = ((MajorMinor_Cmd *)first);
  MajorMinor_Cmd *two = ((MajorMinor_Cmd *)second);

  return strcmp(one->name, two->name);
}

MajorMinor_Cmd *
MM_InitCmds(Tcl_Interp *interp, char *name, MajorMinor_Cmd *cmds,
	    ClientData clientData, int flags)
{
  MajorMinor_Cmd *next, *store;
  Tcl_HashEntry *entryPtr;
  int new;

  if (majminNames == NULL && (Majmin_Init(interp) != TCL_OK)) {
    Tcl_AppendResult(interp, "failed to initialize MajorMinor package",
		     (char *)NULL);
    return NULL;
  }
  entryPtr = Tcl_CreateHashEntry(majminNames, name, &new);
  if (!new && !(flags & (MM_MERGE|MM_OVERWRITE))) {
    Tcl_AppendResult(interp, "cannot create major command \"", name,
		     "\": command already exists", (char *)NULL);
    return NULL;
  }

  new = 0;
  next = cmds;
  while (*(next->name)) {
    next++;
    new++;
  }

  qsort((VOID *) cmds, (size_t) new, sizeof(MajorMinor_Cmd), MM_SortCmds);

  store = (MajorMinor_Cmd *) ckalloc(sizeof(MajorMinor_Cmd));
  store->name = (char *) ckalloc(strlen(name)+1);
  strcpy(store->name, name);
  store->proc = (Tcl_CmdProc *) cmds;
  store->data = clientData;
  Tcl_SetHashValue(entryPtr, store);

  return store;
}

int
MM_InsertCmd(Tcl_Interp *interp, MajorMinor_Cmd *cmds,
	     const char *name, Tcl_CmdProc **proc, int type)
{
  return TCL_OK;
}

static void
MajminDeletedCmd(ClientData clientData)
{
  Tcl_DeleteHashTable((Tcl_HashTable *)clientData);
  Tcl_Free((char *) clientData);
}

/*
 *--------------------------------------------------------------
 *
 * Tcl_MajminCmd --
 *
 *	This procedure is invoked to process the "majmin" Tcl
 *	command.  See the user documentation for details on what
 *	it does.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *--------------------------------------------------------------
 */
int Tcl_MajminCmd(clientData, interp, argc, argv)
     ClientData clientData;	/* Main hash table associated with
				 * interpreter. */
     Tcl_Interp *interp;	/* Current interpreter. */
     int argc;			/* Number of arguments. */
     char **argv;		/* Argument strings. */
{
  Tcl_SetResult(interp, "not yet implemented", TCL_STATIC);
  return TCL_OK;
}


#ifdef MAC_TCL
#pragma export on
#endif
extern int
Majmin_Init(interp)
    Tcl_Interp *interp;
{
  Tcl_CmdInfo info;
#ifdef BUILD_MM
  if (Tcl_PkgRequire(interp, "Tcl", TCL_VERSION, 0) == NULL ||
      Tcl_PkgProvide(interp, "Majmin", MM_VERSION) != TCL_OK) {
    return TCL_ERROR;
  }
#endif
  majminNames = (Tcl_HashTable *) Tcl_Alloc(sizeof(Tcl_HashTable));
  Tcl_InitHashTable(majminNames, TCL_STRING_KEYS);

  if (Tcl_GetCommandInfo(interp, "::majmin", &info) == 0) {
    Tcl_CreateCommand(interp, "::majmin", Tcl_MajminCmd,
		      (ClientData) majminNames,
		      (Tcl_CmdDeleteProc *) MajminDeletedCmd);
  }

  return TCL_OK;
}

extern int
Majmin_SafeInit(interp)
    Tcl_Interp *interp;
{
  return Majmin_Init(interp);
}
#ifdef MAC_TCL
#pragma export reset
#endif
