/* 
 * tkTableTag.c --
 *
 *	This module implements tags for table widgets.
 *
 * Copyright (c) 1998-1999 Jeffrey Hobbs
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include "tkTable.h"

static void	CreateTagEntry _ANSI_ARGS_((Table *tablePtr, char *name,
					    int objc, char **argv));
static void	TableImageProc _ANSI_ARGS_((ClientData clientData, int x,
					    int y, int width, int height,
					    int imageWidth, int imageHeight));

static char *tagCmdNames[] = {
    "celltag", "cget", "coltag", "configure", "delete",
    "exists", "includes", "names", "rowtag", (char *) NULL
};

enum tagCmd {
    TAG_CELLTAG, TAG_CGET, TAG_COLTAG, TAG_CONFIGURE, TAG_DELETE,
    TAG_EXISTS, TAG_INCLUDES, TAG_NAMES, TAG_ROWTAG
};

static Cmd_Struct tagState_vals[]= {
    {"unknown",	 STATE_UNKNOWN},
    {"normal",	 STATE_NORMAL},
    {"disabled", STATE_DISABLED},
    {"",	 0 }
};

static Tk_CustomOption tagStateOpt = { Cmd_OptionSet, Cmd_OptionGet,
				       (ClientData)(&tagState_vals) };

/*
 * The default specification for configuring tags
 * Done like this to make the command line parsing easy
 */

static Tk_ConfigSpec tagConfig[] = {
  {TK_CONFIG_ANCHOR, "-anchor", "anchor", "Anchor", "center",
   Tk_Offset(TableTag, anchor), TK_CONFIG_DONT_SET_DEFAULT },
  {TK_CONFIG_BORDER, "-background", "background", "Background", NULL,
   Tk_Offset(TableTag, bg), TK_CONFIG_DONT_SET_DEFAULT|TK_CONFIG_NULL_OK },
  {TK_CONFIG_SYNONYM, "-bd", "borderWidth", (char *)NULL, (char *)NULL, 0, 0},
  {TK_CONFIG_SYNONYM, "-bg", "background", (char *)NULL, (char *)NULL, 0, 0},
  {TK_CONFIG_PIXELS, "-borderwidth", "borderWidth", "BorderWidth", "-1",
   Tk_Offset(TableTag, bd), TK_CONFIG_DONT_SET_DEFAULT },
  {TK_CONFIG_BORDER, "-foreground", "foreground", "Foreground", NULL,
   Tk_Offset(TableTag, fg), TK_CONFIG_DONT_SET_DEFAULT|TK_CONFIG_NULL_OK },
  {TK_CONFIG_SYNONYM, "-fg", "foreground", (char *)NULL, (char *)NULL, 0, 0},
  {TK_CONFIG_FONT, "-font", "font", "Font", NULL,
   Tk_Offset(TableTag, tkfont), TK_CONFIG_DONT_SET_DEFAULT|TK_CONFIG_NULL_OK },
  {TK_CONFIG_STRING, "-image", "image", "Image", NULL,
   Tk_Offset(TableTag, imageStr),
   TK_CONFIG_DONT_SET_DEFAULT|TK_CONFIG_NULL_OK },
  {TK_CONFIG_JUSTIFY, "-justify", "justify", "Justify", "left",
   Tk_Offset(TableTag, justify), TK_CONFIG_DONT_SET_DEFAULT },
  {TK_CONFIG_INT, "-multiline", "multiline", "Multiline", "-1",
   Tk_Offset(TableTag, multiline), TK_CONFIG_DONT_SET_DEFAULT },
  {TK_CONFIG_RELIEF, "-relief", "relief", "Relief", "flat",
   Tk_Offset(TableTag, relief), TK_CONFIG_DONT_SET_DEFAULT|TK_CONFIG_NULL_OK },
  {TK_CONFIG_INT, "-showtext", "showText", "ShowText", "-1",
   Tk_Offset(TableTag, showtext), TK_CONFIG_DONT_SET_DEFAULT },
  {TK_CONFIG_CUSTOM, "-state", "state", "State", "unknown",
   Tk_Offset(TableTag, state), TK_CONFIG_DONT_SET_DEFAULT, &tagStateOpt },
  {TK_CONFIG_INT, "-wrap", "wrap", "Wrap", "-1",
   Tk_Offset(TableTag, wrap), TK_CONFIG_DONT_SET_DEFAULT },
  {TK_CONFIG_END, (char *)NULL, (char *)NULL, (char *)NULL, (char *)NULL, 0, 0}
};

/* 
 *----------------------------------------------------------------------
 *
 * TableImageProc --
 *	Called when an image associated with a tag is changed.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Invalidates the whole table.
 *	This should only invalidate affected cells, but that info
 *	is not managed...
 *
 *----------------------------------------------------------------------
 */
static void
TableImageProc(ClientData clientData, int x, int y, int width, int height,
	       int imageWidth, int imageHeight)
{
    TableInvalidateAll((Table *)clientData, 0);
}

/*
 *----------------------------------------------------------------------
 *
 * TableNewTag --
 *	ckallocs space for a new tag structure and inits the structure.
 *
 * Results:
 *	Returns a pointer to the new structure.  Must be freed later.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
TableTag *
TableNewTag(void)
{
    TableTag *tagPtr = (TableTag *) ckalloc(sizeof(TableTag));
    tagPtr->anchor	= (Tk_Anchor)-1;
    tagPtr->bd		= -1;
    tagPtr->bg		= NULL;
    tagPtr->fg		= NULL;
    tagPtr->tkfont	= NULL;
    tagPtr->image	= NULL;
    tagPtr->imageStr	= NULL;
    tagPtr->justify	= (Tk_Justify)-1;
    tagPtr->multiline	= -1;
    tagPtr->relief	= -1;
    tagPtr->showtext	= -1;
    tagPtr->state	= STATE_UNKNOWN;
    tagPtr->wrap	= -1;
    return tagPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * TableMergeTag --
 *	This routine merges two tags by adding any fields from the addTag
 *	that are set to the baseTag.
 *
 * Results:
 *	baseTag will inherit all set characteristics of addTag
 *	(addTag thus has the priority).
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
void
TableMergeTag(TableTag *baseTag, TableTag *addTag)
{
    if (addTag->anchor != (Tk_Anchor)-1) baseTag->anchor = addTag->anchor;
    if (addTag->bd >= 0)		baseTag->bd = addTag->bd;
    if (addTag->bg != NULL)		baseTag->bg = addTag->bg;
    if (addTag->fg != NULL)		baseTag->fg = addTag->fg;
    if (addTag->tkfont != NULL)		baseTag->tkfont = addTag->tkfont;
    if (addTag->imageStr != NULL) {
	baseTag->imageStr = addTag->imageStr;
	baseTag->image = addTag->image;
    }
    if (addTag->multiline >= 0)		baseTag->multiline = addTag->multiline;
    if (addTag->relief != -1)		baseTag->relief = addTag->relief;
    if (addTag->showtext >= 0)		baseTag->showtext = addTag->showtext;
    if (addTag->state != STATE_UNKNOWN)	baseTag->state = addTag->state;
    if (addTag->justify != (Tk_Justify)-1) baseTag->justify = addTag->justify;
    if (addTag->wrap >= 0)		baseTag->wrap = addTag->wrap;
}

/*
 *----------------------------------------------------------------------
 *
 * TableInvertTag --
 *	This routine swaps background and foreground for the selected tag.
 *
 * Results:
 *	Inverts fg and bg of tag.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
void
TableInvertTag(TableTag *baseTag)
{
    Tk_3DBorder tmpBg;

    tmpBg = baseTag->fg;
    baseTag->fg = baseTag->bg;
    baseTag->bg = tmpBg;
}

/*
 *----------------------------------------------------------------------
 *
 * CreateTagEntry --
 *	Takes a name and optional args and create a tag entry in the
 *	table's tag table.
 *
 * Results:
 *	A new tag entry will be created.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
static void
CreateTagEntry(Table *tablePtr, char *name, int objc, char **argv)
{
    Tcl_HashEntry *entryPtr;
    TableTag *tagPtr = TableNewTag();
    int dummy;
    Tk_ConfigureWidget(tablePtr->interp, tablePtr->tkwin, tagConfig,
		       objc, argv, (char *)tagPtr, TK_CONFIG_ARGV_ONLY);
    entryPtr = Tcl_CreateHashEntry(tablePtr->tagTable, name, &dummy);
    Tcl_SetHashValue(entryPtr, (ClientData) tagPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * TableInitTags --
 *	Creates the static table tags.
 *
 * Results:
 *	active, sel, title and flash are created as tags.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
void
TableInitTags(Table *tablePtr)
{
    static char *activeArgs[]	= {"-bg", ACTIVE_BG, "-relief", "flat" };
    static char *selArgs[]	= {"-bg", SELECT_BG, "-fg", SELECT_FG,
				   "-relief", "sunken" };
    static char *titleArgs[]	= {"-bg", DISABLED, "-fg", "white",
				   "-relief", "flat", "-state", "disabled" };
    static char *flashArgs[]	= {"-bg", "red" };
    CreateTagEntry(tablePtr, "active", ARSIZE(activeArgs), activeArgs);
    CreateTagEntry(tablePtr, "sel", ARSIZE(selArgs), selArgs);
    CreateTagEntry(tablePtr, "title", ARSIZE(titleArgs), titleArgs);
    CreateTagEntry(tablePtr, "flash", ARSIZE(flashArgs), flashArgs);
}

/*
 *----------------------------------------------------------------------
 *
 * FindRowColTag --
 *	Finds a row/col tag based on the row/col styles and tagCommand.
 *
 * Results:
 *	Returns tag associated with row/col cell, if any.
 *
 * Side effects:
 *	Possible side effects from eval of tagCommand.
 *	IMPORTANT: This plays with the interp result object,
 *	so use of resultPtr in prior command may be invalid after
 *	calling this function.
 *
 *----------------------------------------------------------------------
 */
TableTag *
FindRowColTag(Table *tablePtr, int cell, int mode)
{
    Tcl_HashEntry *entryPtr;
    TableTag *tagPtr = NULL;

    entryPtr = Tcl_FindHashEntry((mode == ROW) ? tablePtr->rowStyles
				 : tablePtr->colStyles, (char *) cell);
    if (entryPtr == NULL) {
	char *cmd = (mode == ROW) ? tablePtr->rowTagCmd : tablePtr->colTagCmd;
	if (cmd) {
	    register Tcl_Interp *interp = tablePtr->interp;
	    char buf[INDEX_BUFSIZE];
	    /* Since it does not exist, eval command with row/col appended */
	    sprintf(buf, " %d", cell);
	    Tcl_Preserve((ClientData) interp);
	    if (Tcl_VarEval(interp, cmd, buf, (char *)NULL) == TCL_OK) {
		char *name = Tcl_GetStringResult(interp);
		if (name && *name) {
		    /* If a result was returned,
		     * check to see if it is a known tag */
		    entryPtr = Tcl_FindHashEntry(tablePtr->tagTable, name);
		}
	    }
	    Tcl_Release((ClientData) interp);
	    Tcl_ResetResult(interp);
	}
    }
    if (entryPtr != NULL) {
	/*
	 * This can be either the one in row|colStyles,
	 * or that returned by eval'ing the row|colTagCmd
	 */
	tagPtr = (TableTag *) Tcl_GetHashValue(entryPtr);
    }
    return tagPtr;
}

/* 
 *----------------------------------------------------------------------
 *
 * TableCleanupTag --
 *	Releases the resources used by a tag before it is freed up.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The tag is no longer valid.
 *
 *----------------------------------------------------------------------
 */
void
TableCleanupTag(Table *tablePtr, TableTag *tagPtr)
{
    if (tagPtr->image) {
	Tk_FreeImage(tagPtr->image);
    }
    /* free the options in the widget */
    Tk_FreeOptions(tagConfig, (char *) tagPtr, tablePtr->display, 0);
}

/*
 *--------------------------------------------------------------
 *
 * Table_TagCmd --
 *	This procedure is invoked to process the tag method
 *	that corresponds to a widget managed by this module.
 *	See the user documentation for details on what it does.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *--------------------------------------------------------------
 */
int
Table_TagCmd(ClientData clientData, register Tcl_Interp *interp,
	    int objc, Tcl_Obj *CONST objv[])
{
    register Table *tablePtr = (Table *)clientData;
    int result = TCL_OK, cmdIndex, i, newEntry, value, len;
    int row, col;
    TableTag *tagPtr;
    Tcl_HashEntry *entryPtr, *scanPtr, *newEntryPtr, *oldEntryPtr;
    Tcl_HashTable *hashTblPtr;
    Tcl_HashSearch search;
    Tk_Image image;
    Tcl_Obj *objPtr, *resultPtr;
    char buf[INDEX_BUFSIZE], *keybuf, *tagname;

    if (objc < 3) {
	Tcl_WrongNumArgs(interp, 2, objv, "option ?arg arg ...?");
	return TCL_ERROR;
    }

    /* parse the next parameter */
    result = Tcl_GetIndexFromObj(interp, objv[2], tagCmdNames,
				 "tag option", 0, &cmdIndex);
    if (result != TCL_OK) {
	return result;
    }
    resultPtr = Tcl_GetObjResult(interp);

    switch ((enum tagCmd) cmdIndex) {
    case TAG_CELLTAG:
	/* tag a (group of) cell(s) */
	if (objc < 4) {
	    Tcl_WrongNumArgs(interp, 3, objv, "tag ?arg arg ...?");
	    return TCL_ERROR;
	}
	/* are we deleting */
	tagname = Tcl_GetStringFromObj(objv[3], &len);
	if (len == 0) {
	    tagPtr = NULL;
	} else {
	    /* check to see if the tag actually exists */
	    entryPtr = Tcl_FindHashEntry(tablePtr->tagTable, tagname);
	    if (entryPtr == NULL) {
		Tcl_AppendStringsToObj(resultPtr, "invalid tag name \"",
				       tagname, "\"", (char *) NULL);
		return TCL_ERROR;
	    }
	    /* get the pointer to the tag structure */
	    tagPtr = (TableTag *) Tcl_GetHashValue (entryPtr);
	}

	if (objc == 4) {
	    /* the user just wants the tagged cells to be returned */
	    /* Special handling for tags: active, flash, sel, title */

	    if ((tablePtr->flags & HAS_ACTIVE) &&
		strcmp(tagname, "active") == 0) {
		TableMakeArrayIndex(tablePtr->activeRow+tablePtr->rowOffset,
				    tablePtr->activeCol+tablePtr->colOffset,
				    buf);
		Tcl_SetStringObj(resultPtr, buf, -1);
	    } else if (tablePtr->flashMode && strcmp(tagname, "flash") == 0) {
		for (scanPtr = Tcl_FirstHashEntry(tablePtr->flashCells,
						  &search);
		     scanPtr != NULL; scanPtr = Tcl_NextHashEntry(&search)) {
		    keybuf = (char *) Tcl_GetHashKey(tablePtr->flashCells,
						      scanPtr);
		    Tcl_ListObjAppendElement(NULL, resultPtr,
					     Tcl_NewStringObj(keybuf, -1));
		}
	    } else if (strcmp(tagname, "sel") == 0) {
		for (scanPtr = Tcl_FirstHashEntry(tablePtr->selCells, &search);
		     scanPtr != NULL; scanPtr = Tcl_NextHashEntry(&search)) {
		    keybuf = (char *) Tcl_GetHashKey(tablePtr->selCells,
						      scanPtr);
		    Tcl_ListObjAppendElement(NULL, resultPtr,
					     Tcl_NewStringObj(keybuf, -1));
		}
	    } else if (strcmp(tagname, "title") == 0 &&
		       (tablePtr->titleRows || tablePtr->titleCols)) {
		for (row = tablePtr->rowOffset;
		     row < tablePtr->rowOffset+tablePtr->rows; row++) {
		    for (col = tablePtr->colOffset;
			 col < tablePtr->colOffset+tablePtr->titleCols;
			 col++) {
			TableMakeArrayIndex(row, col, buf);
			Tcl_ListObjAppendElement(NULL, resultPtr,
						 Tcl_NewStringObj(buf, -1));
		    }
		}
		for (row = tablePtr->rowOffset;
		     row < tablePtr->rowOffset+tablePtr->titleRows; row++) {
		    for (col = tablePtr->colOffset+tablePtr->titleCols;
			 col < tablePtr->colOffset+tablePtr->cols; col++) {
			TableMakeArrayIndex(row, col, buf);
			Tcl_ListObjAppendElement(NULL, resultPtr,
						 Tcl_NewStringObj(buf, -1));
		    }
		}
	    } else {
		for (scanPtr = Tcl_FirstHashEntry(tablePtr->cellStyles,
						  &search);
		     scanPtr != NULL; scanPtr = Tcl_NextHashEntry(&search)) {
		    /* is this the tag pointer for this cell */
		    if ((TableTag *) Tcl_GetHashValue(scanPtr) == tagPtr) {
			keybuf = (char *) Tcl_GetHashKey(tablePtr->cellStyles,
							 scanPtr);
			Tcl_ListObjAppendElement(NULL, resultPtr,
						 Tcl_NewStringObj(keybuf, -1));
		    }
		}
	    }
	    return TCL_OK;
	}

	/* Loop through the arguments and fill in the hash table */
	for (i = 4; i < objc; i++) {
	    /* can I parse this argument */
	    if (TableGetIndexObj(tablePtr, objv[i], &row, &col) != TCL_OK) {
		return TCL_ERROR;
	    }
	    /* get the hash key ready */
	    TableMakeArrayIndex(row, col, buf);

	    if (tagPtr == NULL) {
		/* this is a deletion */
		oldEntryPtr = Tcl_FindHashEntry(tablePtr->cellStyles, buf);
		if (oldEntryPtr != NULL) {
		    Tcl_DeleteHashEntry(oldEntryPtr);
		}
	    } else {
		/* add a key to the hash table */
		newEntryPtr = Tcl_CreateHashEntry(tablePtr->cellStyles, buf,
						  &newEntry);

		/* and set it to point to the Tag structure */
		Tcl_SetHashValue(newEntryPtr, (ClientData) tagPtr);
	    }
	    /* now invalidate the area */
	    TableRefresh(tablePtr, row-tablePtr->rowOffset,
			 col-tablePtr->colOffset, CELL);
	}
	return TCL_OK;

    case TAG_COLTAG:
    case TAG_ROWTAG:
	/* tag a row or a column */
	if (objc < 4) {
	    Tcl_WrongNumArgs(interp, 3, objv, "tag ?arg arg ..?");
	    return TCL_ERROR;
	}
	/* if the tag is empty, we are deleting */
	tagname = Tcl_GetStringFromObj(objv[3], &len);
	if (len == 0) {
	    tagPtr = NULL;
	} else {
	    /* check to see if the tag actually exists */
	    entryPtr = Tcl_FindHashEntry(tablePtr->tagTable, tagname);
	    if (entryPtr == NULL) {
		Tcl_AppendStringsToObj(resultPtr, "invalid tag name \"",
				       tagname, "\"", (char *) NULL);
		return TCL_ERROR;
	    }
	    /* get the pointer to the tag structure */
	    tagPtr = (TableTag *) Tcl_GetHashValue (entryPtr);
	}

	/* and choose the correct hash table */
	hashTblPtr = (cmdIndex == TAG_ROWTAG) ?
	    tablePtr->rowStyles : tablePtr->colStyles;

	if (objc == 4) {
	    /* the user just wants the tagged cells to be returned */
	    for (scanPtr = Tcl_FirstHashEntry(hashTblPtr, &search);
		 scanPtr != NULL; scanPtr = Tcl_NextHashEntry(&search)) {
		/* is this the tag pointer on this row */
		if ((TableTag *) Tcl_GetHashValue(scanPtr) == tagPtr) {
		    objPtr = Tcl_NewIntObj((int) Tcl_GetHashKey(hashTblPtr,
								scanPtr));
		    Tcl_ListObjAppendElement(NULL, resultPtr, objPtr);
		}
	    }
	    return TCL_OK;
	}
	/* Now loop through the arguments and fill in the hash table */
	for (i = 4; i < objc; i++) {
	    /* can I parse this argument */
	    if (Tcl_GetIntFromObj(interp, objv[i], &value) != TCL_OK) {
		return TCL_ERROR;
	    }
	    /* deleting or adding */
	    if (tagPtr == NULL) {
		oldEntryPtr = Tcl_FindHashEntry(hashTblPtr, (char *) value);
		if (oldEntryPtr != NULL) {
		    Tcl_DeleteHashEntry(oldEntryPtr);
		}
	    } else {
		/* add a key to the hash table */
		newEntryPtr = Tcl_CreateHashEntry(hashTblPtr, (char *) value,
						  &newEntry);
		/* and set it to point to the Tag structure */
		Tcl_SetHashValue (newEntryPtr, (ClientData) tagPtr);
	    }
	    /* and invalidate the row or column affected */
	    if (cmdIndex == TAG_ROWTAG) {
		TableRefresh(tablePtr, value-tablePtr->rowOffset, 0, ROW);
	    } else {
		TableRefresh(tablePtr, 0, value-tablePtr->colOffset, COL);
	    }
	}
	return TCL_OK;	/* COLTAG && ROWTAG */

    case TAG_CGET:
	if (objc != 5) {
	    Tcl_WrongNumArgs(interp, 3, objv, "tagName option");
	    return TCL_ERROR;
	}
	tagname  = Tcl_GetString(objv[3]);
	entryPtr = Tcl_FindHashEntry(tablePtr->tagTable, tagname);
	if (entryPtr == NULL) {
	    Tcl_AppendStringsToObj(resultPtr, "invalid tag name \"",
				   tagname, "\"", (char *) NULL);
	    return TCL_ERROR;
	} else {
	    tagPtr = (TableTag *) Tcl_GetHashValue (entryPtr);
	    result = Tk_ConfigureValue(interp, tablePtr->tkwin, tagConfig,
				       (char *) tagPtr,
				       Tcl_GetString(objv[4]), 0);
	}
	return result;	/* CGET */

    case TAG_CONFIGURE: {
	char **argv;

	if (objc < 4) {
	    Tcl_WrongNumArgs(interp, 3, objv, "tagName ?arg arg  ...?");
	    return TCL_ERROR;
	}
	/* first see if this is a reconfiguration */
	tagname  = Tcl_GetString(objv[3]);
	entryPtr = Tcl_CreateHashEntry(tablePtr->tagTable, tagname, &newEntry);

	/* Stringify */
	argv = (char **) ckalloc((objc + 1) * sizeof(char *));
	for (i = 0; i < objc; i++)
	    argv[i] = Tcl_GetString(objv[i]);
	argv[objc] = NULL;

	if (newEntry) {
	    /* create the structure */
	    tagPtr = TableNewTag();

	    /* insert it into the table */
	    Tcl_SetHashValue(entryPtr, (ClientData) tagPtr);

	    /* configure the tag structure */
	    result = Tk_ConfigureWidget(interp, tablePtr->tkwin, tagConfig,
					objc-4, argv+4, (char *) tagPtr, 0);
	} else {
	    /* pointer wasn't null, do a reconfig if we have enough args */
	    /* get the tag pointer from the table */
	    tagPtr = (TableTag *) Tcl_GetHashValue(entryPtr);

	    /* 5 args means that there are values to replace */
	    if (objc > 5) {
		/* and do a reconfigure */
		result = Tk_ConfigureWidget(interp, tablePtr->tkwin, tagConfig,
					    objc-4, argv+4, (char *) tagPtr,
					    TK_CONFIG_ARGV_ONLY);
	    }
	}
	ckfree((char *) argv);
	if (result == TCL_ERROR) {
	    return TCL_ERROR;
	}

	/* handle change of image name */
	if (tagPtr->imageStr) {
	    image = Tk_GetImage(interp, tablePtr->tkwin, tagPtr->imageStr,
				TableImageProc, (ClientData)tablePtr);
	    if (image == NULL) {
		result = TCL_ERROR;
	    }
	} else {
	    image = NULL;
	}
	if (tagPtr->image) {
	    Tk_FreeImage(tagPtr->image);
	}
	tagPtr->image = image;

	/* 
	 * If there were less than 6 args, we need
	 * to do a printout of the config, even for new tags
	 */
	if (objc < 6) {
	    result = Tk_ConfigureInfo(interp, tablePtr->tkwin, tagConfig,
				      (char *) tagPtr, (objc == 5) ?
				      Tcl_GetString(objv[4]) : NULL, 0);
	} else {
	    /* Otherwise we reconfigured so invalidate the table to redraw */
	    TableInvalidateAll(tablePtr, 0);
	}
	return result;
    }

    case TAG_DELETE:
	/* delete a tag */
	if (objc < 4) {
	    Tcl_WrongNumArgs(interp, 3, objv, "tagName ?tagName ...?");
	    return TCL_ERROR;
	}
	/* run through the remaining arguments */
	for (i = 3; i < objc; i++) {
	    tagname  = Tcl_GetString(objv[i]);
	    /* cannot delete the title tag */
	    if (strcmp(tagname, "title") == 0 ||
		strcmp(tagname, "sel") == 0 ||
		strcmp(tagname, "flash") == 0 ||
		strcmp(tagname, "active") == 0) {
		Tcl_AppendStringsToObj(resultPtr, "cannot delete ", tagname,
				       " tag", (char *) NULL);
		return TCL_ERROR;
	    }
	    entryPtr = Tcl_FindHashEntry(tablePtr->tagTable, tagname);
	    if (entryPtr != NULL) {
		/* get the tag pointer */
		tagPtr = (TableTag *) Tcl_GetHashValue(entryPtr);

		/* delete all references to this tag in rows */
		scanPtr = Tcl_FirstHashEntry(tablePtr->rowStyles, &search);
		for (; scanPtr != NULL; scanPtr = Tcl_NextHashEntry(&search)) {
		    if ((TableTag *)Tcl_GetHashValue(scanPtr) == tagPtr)
			Tcl_DeleteHashEntry(scanPtr);
		}

		/* delete all references to this tag in cols */
		scanPtr = Tcl_FirstHashEntry(tablePtr->colStyles, &search);
		for (; scanPtr != NULL; scanPtr = Tcl_NextHashEntry(&search)) {
		    if ((TableTag *)Tcl_GetHashValue(scanPtr) == tagPtr)
			Tcl_DeleteHashEntry(scanPtr);
		}

		/* delete all references to this tag in cells */
		scanPtr = Tcl_FirstHashEntry(tablePtr->cellStyles, &search);
		for (; scanPtr != NULL; scanPtr = Tcl_NextHashEntry(&search)) {
		    if ((TableTag *)Tcl_GetHashValue(scanPtr) == tagPtr)
			Tcl_DeleteHashEntry(scanPtr);
		}

		/* release the structure */
		TableCleanupTag(tablePtr, tagPtr);
		ckfree((char *) tagPtr);

		/* and free the hash table entry */
		Tcl_DeleteHashEntry(entryPtr);
	    }
	}
	/* since we deleted a tag, redraw the screen */
	TableInvalidateAll(tablePtr, 0);
	return result;

    case TAG_EXISTS:
	if (objc != 4) {
	    Tcl_WrongNumArgs(interp, 3, objv, "tagName");
	    return TCL_ERROR;
	}
	Tcl_SetBooleanObj(resultPtr,
			  (Tcl_FindHashEntry(tablePtr->tagTable,
					    Tcl_GetString(objv[3])) != NULL));
	return TCL_OK;

    case TAG_INCLUDES:
	/* does a tag contain a index ? */
	if (objc != 5) {
	    Tcl_WrongNumArgs(interp, 3, objv, "tag index");
	    return TCL_ERROR;
	}
	tagname  = Tcl_GetString(objv[3]);
	/* check to see if the tag actually exists */
	entryPtr = Tcl_FindHashEntry(tablePtr->tagTable, tagname);
	if (entryPtr == NULL) {
	    /* Unknown tag, just return 0 */
	    Tcl_SetBooleanObj(resultPtr, 0);
	    return TCL_OK;
	}
	/* parse index */
	if (TableGetIndexObj(tablePtr, objv[4], &row, &col) != TCL_OK) {
	    return TCL_ERROR;
	}
	/* create hash key */
	TableMakeArrayIndex(row, col, buf);
    
	if (strcmp(tagname, "active") == 0) {
	    result = (tablePtr->activeRow+tablePtr->rowOffset==row &&
		      tablePtr->activeCol+tablePtr->colOffset==col);
	} else if (strcmp(tagname, "flash") == 0) {
	    result = (tablePtr->flashMode &&
		      (Tcl_FindHashEntry(tablePtr->flashCells, buf) != NULL));
	} else if (strcmp(tagname, "sel") == 0) {
	    result = (Tcl_FindHashEntry(tablePtr->selCells, buf) != NULL);
	} else if (strcmp(tagname, "title") == 0) {
	    result = (row < tablePtr->titleRows+tablePtr->rowOffset ||
		      col < tablePtr->titleCols+tablePtr->colOffset);
	} else {
	    /* get the pointer to the tag structure */
	    tagPtr = (TableTag *) Tcl_GetHashValue(entryPtr);
	    scanPtr = Tcl_FindHashEntry(tablePtr->cellStyles, buf);
	    /* look to see if there is a cell, row, or col tag for this cell */
	    result = ((scanPtr &&
		       (tagPtr == (TableTag *) Tcl_GetHashValue(scanPtr))) ||
		      (tagPtr == FindRowColTag(tablePtr, row, ROW)) ||
		      (tagPtr == FindRowColTag(tablePtr, col, COL)));
	}
	/* because we may call FindRowColTag above, we can't use
	 * the resultPtr, but this is almost equivalent, and is SAFE
	 */
	Tcl_SetObjResult(interp, Tcl_NewBooleanObj(result));
	return TCL_OK;

    case TAG_NAMES:
	/* just print out the tag names */
	if (objc < 3 || objc > 4) {
	    Tcl_WrongNumArgs(interp, 3, objv, "?pattern?");
	    return TCL_ERROR;
	}
	tagname = (objc == 4) ? Tcl_GetString(objv[3]) : NULL;
	entryPtr = Tcl_FirstHashEntry(tablePtr->tagTable, &search);
	while (entryPtr != NULL) {
	    keybuf = Tcl_GetHashKey(tablePtr->tagTable, entryPtr);
	    if (objc == 3 || Tcl_StringMatch(keybuf, tagname)) {
		objPtr = Tcl_NewStringObj(keybuf, -1);
		Tcl_ListObjAppendElement(NULL, resultPtr, objPtr);
	    }
	    entryPtr = Tcl_NextHashEntry(&search);
	}
	return TCL_OK;

    }

    return TCL_OK;
}
