/* 
 * tkTableCmds.c --
 *
 *	This module implements general commands of a table widget,
 *	based on the major/minor command structure.
 *
 * Copyright (c) 1998 Jeffrey Hobbs
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include "tkTable.h"

/*
 *--------------------------------------------------------------
 *
 * Table_ActivateCmd --
 *	This procedure is invoked to process the activate method
 *	that corresponds to a table widget managed by this module.
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
Table_ActivateCmd(ClientData clientData, register Tcl_Interp *interp,
	      int argc, char **argv)
{
  register Table *tablePtr = (Table *) clientData;
  int result = TCL_OK;
  int row, col, x, y, w, dummy;
  char buf1[INDEX_BUFSIZE], buf2[INDEX_BUFSIZE];

  if (argc != 3) {
    Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
		     " activate index\"", (char *)NULL);
    return TCL_ERROR;
  } else if (TableGetIndex(tablePtr, argv[2], &row, &col) == TCL_ERROR) {
    return TCL_ERROR;
  } else {
    /* convert to valid active index in real coords */
    row -= tablePtr->rowOffset;
    col -= tablePtr->colOffset;
    /* we do this regardless, to avoid cell commit problems */
    if ((tablePtr->flags & HAS_ACTIVE) &&
	(tablePtr->flags & TEXT_CHANGED)) {
      tablePtr->flags &= ~TEXT_CHANGED;
      TableSetCellValue(tablePtr, tablePtr->activeRow+tablePtr->rowOffset,
			tablePtr->activeCol+tablePtr->colOffset,
			tablePtr->activeBuf);
    }
    if (row != tablePtr->activeRow || col != tablePtr->activeCol) {
      if (tablePtr->flags & HAS_ACTIVE) {
	TableMakeArrayIndex(tablePtr->activeRow+tablePtr->rowOffset,
			    tablePtr->activeCol+tablePtr->colOffset, buf1);
      } else {
	buf1[0] = '\0';
      }
      tablePtr->flags |= HAS_ACTIVE;
      tablePtr->flags &= ~ACTIVE_DISABLED;
      tablePtr->activeRow = row;
      tablePtr->activeCol = col;
      if (tablePtr->activeTagPtr != NULL) {
	ckfree((char *) (tablePtr->activeTagPtr));
	tablePtr->activeTagPtr = NULL;
      }
      TableAdjustActive(tablePtr);
      TableConfigCursor(tablePtr);
      if (!(tablePtr->flags & BROWSE_CMD) && tablePtr->browseCmd != NULL) {
	Tcl_DString script;
	tablePtr->flags |= BROWSE_CMD;
	row = tablePtr->activeRow+tablePtr->rowOffset;
	col = tablePtr->activeCol+tablePtr->colOffset;
	TableMakeArrayIndex(row, col, buf2);
	Tcl_DStringInit(&script);
	ExpandPercents(tablePtr, tablePtr->browseCmd, row, col, buf1, buf2,
		       tablePtr->icursor, &script, 0);
	result = Tcl_GlobalEval(interp, Tcl_DStringValue(&script));
	if (result == TCL_OK || result == TCL_RETURN)
	  Tcl_ResetResult(interp);
	Tcl_DStringFree(&script);
	tablePtr->flags &= ~BROWSE_CMD;
      }
    } else if ((tablePtr->activeTagPtr != NULL) &&
	       !(tablePtr->flags & ACTIVE_DISABLED) && argv[2][0] == '@' &&
	       TableCellVCoords(tablePtr, row, col, &x, &y, &w, &dummy, 0)) {
      /* we are clicking into the same cell */
      /* If it was activated with @x,y indexing, find the closest char */
      Tk_TextLayout textLayout;
      TableTag *tagPtr = tablePtr->activeTagPtr;
      char *p;

	/* no error checking because GetIndex did it for us */
      p = argv[2]+1;
      x = strtol(p, &p, 0) - x - tablePtr->activeX;
      y = strtol(++p, &p, 0) - y - tablePtr->activeY;

      textLayout = Tk_ComputeTextLayout(tagPtr->tkfont, tablePtr->activeBuf,
					strlen(tablePtr->activeBuf),
					(tagPtr->wrap) ? w : 0,
					tagPtr->justify, 0, &dummy, &dummy);

      tablePtr->icursor = Tk_PointToChar(textLayout, x, y);
      Tk_FreeTextLayout(textLayout);
      TableConfigCursor(tablePtr);
    }
    tablePtr->flags |= HAS_ACTIVE;
  }
  return result;
}

/*
 *--------------------------------------------------------------
 *
 * Table_AdjustCmd --
 *	This procedure is invoked to process the width/height method
 *	that corresponds to a table widget managed by this module.
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
Table_AdjustCmd(ClientData clientData, register Tcl_Interp *interp,
	      int argc, char **argv)
{
  register Table *tablePtr = (Table *) clientData;
  int result = TCL_OK;
  Tcl_HashEntry *entryPtr;
  Tcl_HashSearch search;
  Tcl_HashTable *hashTablePtr;
  int i, widthType, dummy, value, posn, offset;
  char buf1[INDEX_BUFSIZE];

  widthType = (strncmp(argv[1], "width", strlen(argv[1]))==0);
  /* changes the width/height of certain selected columns */
  if (argc != 3 && (argc & 1)) {
    Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
		     (widthType) ? " width ?col? ?width col width ...?\"" :
    " height ?row? ?height row height ...?\"",
    (char *) NULL);
    return TCL_ERROR;
  }
  if (widthType) {
    hashTablePtr = tablePtr->colWidths;
    offset = tablePtr->colOffset;
  } else { 
    hashTablePtr = tablePtr->rowHeights;
    offset = tablePtr->rowOffset;
  }

  if (argc == 2) {
    /* print out all the preset column widths or row heights */
    entryPtr = Tcl_FirstHashEntry(hashTablePtr, &search);
    while (entryPtr != NULL) {
      posn = ((int) Tcl_GetHashKey(hashTablePtr, entryPtr)) + offset;
      value = (int) Tcl_GetHashValue(entryPtr);
      sprintf(buf1, "%d %d", posn, value);
      Tcl_AppendElement(interp, buf1);
      entryPtr = Tcl_NextHashEntry(&search);
    }
  } else if (argc == 3) {
    /* get the width/height of a particular row/col */
    if (Tcl_GetInt(interp, argv[2], &posn) != TCL_OK) {
      return TCL_ERROR;
    }
    /* no range check is done, why bother? */
    posn -= offset;
    entryPtr = Tcl_FindHashEntry(hashTablePtr, (char *) posn);
    if (entryPtr != NULL) {
      sprintf(buf1, "%d", (int) Tcl_GetHashValue(entryPtr));
      Tcl_SetResult(interp, buf1, TCL_VOLATILE);
    } else {
      sprintf(buf1, "%d", (widthType) ?
	      tablePtr->defColWidth : tablePtr->defRowHeight);
      Tcl_SetResult(interp, buf1, TCL_VOLATILE);
    }
  } else {
    for (i=2; i<argc; i++) {
      /* set new width|height here */
      value = -999999;
      if (Tcl_GetInt(interp, argv[i++], &posn) != TCL_OK ||
	  (strncmp(argv[i], "default", strlen(argv[i])) &&
	   Tcl_GetInt(interp, argv[i], &value) != TCL_OK)) {
	return TCL_ERROR;
      }
      posn -= offset;
      if (value == -999999) {
	/* reset that field */
	if ((entryPtr = Tcl_FindHashEntry(hashTablePtr, (char *) posn)))
	  Tcl_DeleteHashEntry(entryPtr);
      } else {
	entryPtr = Tcl_CreateHashEntry(hashTablePtr, (char *) posn, &dummy);
	Tcl_SetHashValue(entryPtr, (ClientData) value);
      }
    }
    TableAdjustParams(tablePtr);
    /* rerequest geometry */
    TableGeometryRequest(tablePtr);
    /*
     * Invalidate the whole window as TableAdjustParams
     * will only check to see if the top left cell has moved
     * FIX: should just move from lowest order visible cell to edge of window
     */
    TableInvalidateAll(tablePtr, 0);
  }
  return result;
}

/*
 *--------------------------------------------------------------
 *
 * Table_BboxCmd --
 *	This procedure is invoked to process the bbox method
 *	that corresponds to a table widget managed by this module.
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
Table_BboxCmd(ClientData clientData, register Tcl_Interp *interp,
	      int argc, char **argv)
{
  register Table *tablePtr = (Table *) clientData;
  int x, y, w, h, row, col, key;
  char buf1[INDEX_BUFSIZE];

  /* Returns bounding box of cell(s) */
  if (argc < 3 || argc > 4) {
    Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
		     " bbox first ?last?\"", (char *) NULL);
    return TCL_ERROR;
  } else if (TableGetIndex(tablePtr, argv[2], &row, &col) == TCL_ERROR ||
	     (argc == 4 &&
	      TableGetIndex(tablePtr, argv[3], &x, &y) == TCL_ERROR)) {
    return TCL_ERROR;
  }

  if (argc == 3) {
    row -= tablePtr->rowOffset; col -= tablePtr->colOffset;
    if (TableCellVCoords(tablePtr, row, col, &x, &y, &w, &h, 0)) {
      sprintf(buf1, "%d %d %d %d", x, y, w, h);
      Tcl_SetResult(interp, buf1, TCL_VOLATILE);
    }
    return TCL_OK;
  } else {
    int r1, c1, r2, c2, minX = 99999, minY = 99999, maxX = 0, maxY = 0;

    row -= tablePtr->rowOffset; col -= tablePtr->colOffset;
    x -= tablePtr->rowOffset; y -= tablePtr->colOffset;
    r1 = MIN(row,x); r2 = MAX(row,x);
    c1 = MIN(col,y); c2 = MAX(col,y);
    key = 0;
    for (row = r1; row <= r2; row++) {
      for (col = c1; col <= c2; col++) {
	if (TableCellVCoords(tablePtr, row, col, &x, &y, &w, &h, 0)) {
	  /* Get max bounding box */
	  if (x < minX) minX = x;
	  if (y < minY) minY = y;
	  if (x+w > maxX) maxX = x+w;
	  if (y+h > maxY) maxY = y+h;
	  key++;
	}
      }
    }
    if (key) {
      sprintf(buf1, "%d %d %d %d", minX, minY, maxX-minX, maxY-minY);
      Tcl_SetResult(interp, buf1, TCL_VOLATILE);
    }
  }
  return TCL_OK;
}

/* border subcommands */
#define BD_MARK		1
#define BD_DRAGTO	2
static Cmd_Struct bd_cmds[] = {
  {"mark",	BD_MARK},
  {"dragto",	BD_DRAGTO},
  {"", 0}
};

/*
 *--------------------------------------------------------------
 *
 * Table_BorderCmd --
 *	This procedure is invoked to process the bbox method
 *	that corresponds to a table widget managed by this module.
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
Table_BorderCmd(ClientData clientData, register Tcl_Interp *interp,
	      int argc, char **argv)
{
  register Table *tablePtr = (Table *) clientData;
  Tcl_HashEntry *entryPtr;
  int x, y, w, h, row, col, key, dummy, retval, value;
  char buf1[INDEX_BUFSIZE];

  if (argc != 5 && argc != 6) {
    Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
		     " border mark|dragto x y ?r|c?\"", (char *) NULL);
    return TCL_ERROR;
  }
  retval = Cmd_Parse(interp, bd_cmds, argv[2]);
  if (retval == 0 || Tcl_GetInt(interp, argv[3], &x) != TCL_OK ||
      Tcl_GetInt(interp, argv[4], &y) != TCL_OK) {
    return TCL_ERROR;
  }
  switch (retval) {
  case BD_MARK:
    /* Use x && y to determine if we are over a border */
    value = TableAtBorder(tablePtr, x, y, &row, &col);
    /* Cache the row && col for use in DRAGTO */
    tablePtr->scanMarkRow = row;
    tablePtr->scanMarkCol = col;
    if (!value) {
      return TCL_OK;
    }
    TableCellCoords(tablePtr, row, col, &x, &y, &dummy, &dummy);
    tablePtr->scanMarkX = x;
    tablePtr->scanMarkY = y;
    if (argc == 5 || argv[5][0] == 'r') {
      if (row < 0)
	buf1[0] = '\0';
      else
	sprintf(buf1, "%d", row+tablePtr->rowOffset);
      Tcl_AppendElement(interp, buf1);
    }
    if (argc == 5 || argv[5][0] == 'c') {
      if (col < 0)
	buf1[0] = '\0';
      else
	sprintf(buf1, "%d", col+tablePtr->colOffset);
      Tcl_AppendElement(interp, buf1);
    }
    return TCL_OK;	/* BORDER MARK */
  case BD_DRAGTO:
    /* check to see if we want to resize any borders */
    if (tablePtr->resize == SEL_NONE) { return TCL_OK; }
    row = tablePtr->scanMarkRow;
    col = tablePtr->scanMarkCol;
    TableCellCoords(tablePtr, row, col, &w, &h, &dummy, &dummy);
    key = 0;
    if (row >= 0 && (tablePtr->resize & SEL_ROW)) {
      /* row border was active, move it */
      value = y-h;
      if (value < -1) value = -1;
      if (value != tablePtr->scanMarkY) {
	entryPtr = Tcl_CreateHashEntry(tablePtr->rowHeights,
				       (char *) row, &dummy);
	/* -value means rowHeight will be interp'd as pixels, not lines */
	Tcl_SetHashValue(entryPtr, (ClientData) MIN(0,-value));
	tablePtr->scanMarkY = value;
	key++;
      }
    }
    if (col >= 0 && (tablePtr->resize & SEL_COL)) {
      /* col border was active, move it */
      value = x-w;
      if (value < -1) value = -1;
      if (value != tablePtr->scanMarkX) {
	entryPtr = Tcl_CreateHashEntry(tablePtr->colWidths,
				       (char *) col, &dummy);
	/* -value means colWidth will be interp'd as pixels, not chars */
	Tcl_SetHashValue(entryPtr, (ClientData) MIN(0,-value));
	tablePtr->scanMarkX = value;
	key++;
      }
    }
    /* Only if something changed do we want to update */
    if (key) {
      TableAdjustParams(tablePtr);
      /* Only rerequest geometry if the basis is the #rows &| #cols */
      if (tablePtr->maxReqCols || tablePtr->maxReqRows)
	TableGeometryRequest(tablePtr);
      TableInvalidateAll(tablePtr, 0);
    }
    return TCL_OK;	/* BORDER DRAGTO */
  }
  return TCL_OK;
}

/*
 *--------------------------------------------------------------
 *
 * Table_CgetCmd --
 *	This procedure is invoked to process the bbox method
 *	that corresponds to a table widget managed by this module.
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
Table_CgetCmd(ClientData clientData, register Tcl_Interp *interp,
	      int argc, char **argv)
{
  register Table *tablePtr = (Table *) clientData;

  if (argc != 3) {
    Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
		     " cget option\"", (char *) NULL);
    return TCL_ERROR;
  }
  return Tk_ConfigureValue(interp, tablePtr->tkwin, tableSpecs,
			   (char *) tablePtr, argv[2], 0);
}

/*
 *--------------------------------------------------------------
 *
 * Table_ConfigureCmd --
 *	This procedure is invoked to process the bbox method
 *	that corresponds to a table widget managed by this module.
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
Table_ConfigureCmd(ClientData clientData, register Tcl_Interp *interp,
	      int argc, char **argv)
{
  register Table *tablePtr = (Table *) clientData;

  switch (argc) {
  case 2:
    return Tk_ConfigureInfo(interp, tablePtr->tkwin, tableSpecs,
			    (char *) tablePtr, (char *) NULL, 0);
  case 3:
    return Tk_ConfigureInfo(interp, tablePtr->tkwin, tableSpecs,
			    (char *) tablePtr, argv[2], 0);
  default:
    return TableConfigure(interp, tablePtr, argc - 2, argv + 2,
			  TK_CONFIG_ARGV_ONLY, 0);
  }
}

/* clear subcommands */
static Cmd_Struct clear_cmds[] = {
  {"tags",	CLEAR_TAGS},
  {"sizes",	CLEAR_SIZES},
  {"cache",	CLEAR_CACHE},
  {"all",	CLEAR_TAGS | CLEAR_SIZES | CLEAR_CACHE},
  {"",		0}
};

/*
 *--------------------------------------------------------------
 *
 * Table_ClearCmd --
 *	This procedure is invoked to process the bbox method
 *	that corresponds to a table widget managed by this module.
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
Table_ClearCmd(ClientData clientData, register Tcl_Interp *interp,
	      int argc, char **argv)
{
  register Table *tablePtr = (Table *) clientData;

  if (argc < 3 || argc > 5) {
    Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
		     " clear option ?first? ?last?\"", (char *) NULL);
    return TCL_ERROR;
  }
  return TableClear(tablePtr, Cmd_Parse(interp, clear_cmds, argv[2]),
		    (argc>3)?argv[3]:NULL, (argc>4)?argv[4]:NULL);
}

/*
 *--------------------------------------------------------------
 *
 * Table_CurselectionCmd --
 *	This procedure is invoked to process the bbox method
 *	that corresponds to a table widget managed by this module.
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
Table_CurselectionCmd(ClientData clientData, register Tcl_Interp *interp,
	      int argc, char **argv)
{
  register Table *tablePtr = (Table *) clientData;
  int result = TCL_OK;
  Tcl_HashEntry *entryPtr;
  Tcl_HashSearch search;
  int row, col;

  if ((argc != 2 && argc != 4) ||
      (argc == 4 && (argv[2][0] == '\0' ||
		     strncmp(argv[2], "set", strlen(argv[2]))))) {
    Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
		     " curselection ?set <value>?\"", (char *)NULL);
    return TCL_ERROR;
  }
  /* make sure there is a data source to accept set */
  if (argc == 4 && (tablePtr->state == STATE_DISABLED ||
		    (tablePtr->dataSource == DATA_NONE)))
    return result;
  for (entryPtr = Tcl_FirstHashEntry(tablePtr->selCells, &search);
       entryPtr != NULL; entryPtr = Tcl_NextHashEntry(&search)) {
    if (argc == 2) {
      Tcl_AppendElement(interp, Tcl_GetHashKey(tablePtr->selCells, entryPtr));
    } else {
      TableParseArrayIndex(&row, &col,
			   Tcl_GetHashKey(tablePtr->selCells, entryPtr));
      TableSetCellValue(tablePtr, row, col, argv[3]);
      row -= tablePtr->rowOffset;
      col -= tablePtr->colOffset;
      if (row == tablePtr->activeRow && col == tablePtr->activeCol) {
	TableGetActiveBuf(tablePtr);
      }
      TableRefresh(tablePtr, row, col, CELL);
    }
  }
  if (argc == 2) {
    Tcl_SetResult(interp, TableCellSort(tablePtr, Tcl_GetStringResult(interp)),
		  TCL_DYNAMIC);
  }
  return result;
}

/*
 *--------------------------------------------------------------
 *
 * Table_CurvalueCmd --
 *	This procedure is invoked to process the curvalue method
 *	that corresponds to a table widget managed by this module.
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
Table_CurvalueCmd(ClientData clientData, register Tcl_Interp *interp,
		  int argc, char **argv)
{
  register Table *tablePtr = (Table *) clientData;

  if (argc > 3) {
    Tcl_AppendResult(interp, "wrong # args: should be \"",
		     argv[0], " curvalue ?<value>?\"", (char *)NULL);
    return TCL_ERROR;
  } else if (!(tablePtr->flags & HAS_ACTIVE)) {
    return TCL_OK;
  }

  if (argc == 3 && strcmp(argv[2], tablePtr->activeBuf)) {
    /* validate potential new active buffer contents
     * only accept if validation returns acceptance. */
    if (tablePtr->validate &&
	TableValidateChange(tablePtr,
			    tablePtr->activeRow+tablePtr->rowOffset,
			    tablePtr->activeCol+tablePtr->colOffset,
			    tablePtr->activeBuf,
			    argv[2], tablePtr->icursor) != TCL_OK) {
      return TCL_OK;
    }
    tablePtr->activeBuf = (char *)ckrealloc(tablePtr->activeBuf,
					    strlen(argv[2])+1);
    strcpy(tablePtr->activeBuf, argv[2]);
    /* mark the text as changed */
    tablePtr->flags |= TEXT_CHANGED;
    TableSetActiveIndex(tablePtr);
    /* check for possible adjustment of icursor */
    TableGetIcursor(tablePtr, "insert", (int *)0);
    TableRefresh(tablePtr, tablePtr->activeRow, tablePtr->activeCol,
		 CELL|INV_FORCE);
  }
  Tcl_AppendResult(interp, tablePtr->activeBuf, (char *)NULL);

  return TCL_OK;
}

/*
 *--------------------------------------------------------------
 *
 * Table_GetCmd --
 *	This procedure is invoked to process the bbox method
 *	that corresponds to a table widget managed by this module.
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
Table_GetCmd(ClientData clientData, register Tcl_Interp *interp,
	      int argc, char **argv)
{
  register Table *tablePtr = (Table *) clientData;
  int result = TCL_OK;
  int r1, c1, r2, c2, row, col;

  if (argc < 3 || argc > 4) {
    Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
		     " get first ?last?\"", (char *)NULL);
    result = TCL_ERROR;
  } else if (TableGetIndex(tablePtr, argv[2], &row, &col) == TCL_ERROR) {
    result = TCL_ERROR;
  } else if (argc == 3) {
    Tcl_SetResult(interp, TableGetCellValue(tablePtr, row, col), TCL_STATIC);
  } else if (TableGetIndex(tablePtr, argv[3], &r2, &c2) == TCL_ERROR) {
    result = TCL_ERROR;
  } else {
    r1 = MIN(row,r2); r2 = MAX(row,r2);
    c1 = MIN(col,c2); c2 = MAX(col,c2);
    for ( row = r1; row <= r2; row++ ) {
      for ( col = c1; col <= c2; col++ ) {
	Tcl_AppendElement(interp, TableGetCellValue(tablePtr, row, col));
      }
    }
  }
  return result;
}

/*
 *--------------------------------------------------------------
 *
 * Table_IcursorCmd --
 *	This procedure is invoked to process the bbox method
 *	that corresponds to a table widget managed by this module.
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
Table_IcursorCmd(ClientData clientData, register Tcl_Interp *interp,
	      int argc, char **argv)
{
  register Table *tablePtr = (Table *) clientData;
  char buf1[INDEX_BUFSIZE];

  if (!(tablePtr->flags & HAS_ACTIVE) ||
      (tablePtr->flags & ACTIVE_DISABLED) ||
      tablePtr->state == STATE_DISABLED) {
    return TCL_OK;
  }
  switch (argc) {
  case 2:
    sprintf(buf1, "%d", tablePtr->icursor);
    Tcl_SetResult(interp, buf1, TCL_VOLATILE);
    break;
  case 3:
    if (TableGetIcursor(tablePtr, argv[2], (int *)0) != TCL_OK) {
      return TCL_ERROR;
    }
    TableRefresh(tablePtr, tablePtr->activeRow, tablePtr->activeCol, CELL);
    break;
  default:
    Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
		     " icursor arg\"", (char *) NULL);
    return TCL_ERROR;
  }
  return TCL_OK;
}

/*
 *--------------------------------------------------------------
 *
 * Table_IndexCmd --
 *	This procedure is invoked to process the bbox method
 *	that corresponds to a table widget managed by this module.
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
Table_IndexCmd(ClientData clientData, register Tcl_Interp *interp,
	      int argc, char **argv)
{
  register Table *tablePtr = (Table *) clientData;
  int row, col;
  char buf1[INDEX_BUFSIZE];

  if (argc < 3 || argc > 4 ||
      TableGetIndex(tablePtr, argv[2], &row, &col) == TCL_ERROR ||
      (argc == 4 && (strcmp(argv[3], "row") && strcmp(argv[3], "col")))) {
    if (!strlen(Tcl_GetStringResult(interp))) {
      Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
		       " index index ?row|col?\"", (char *)NULL);
    }
    return TCL_ERROR;
  }
  if (argc == 3) {
    TableMakeArrayIndex(row, col, buf1);
  } else if (argv[3][0] == 'r') { /* INDEX row */
    sprintf(buf1, "%d", row);
  } else {	/* INDEX col */
    sprintf(buf1, "%d", col);
  }
  Tcl_SetResult(interp, buf1, TCL_VOLATILE);
  return TCL_OK;
}

/*
 *--------------------------------------------------------------
 *
 * Table_RereadCmd --
 *	This procedure is invoked to process the bbox method
 *	that corresponds to a table widget managed by this module.
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
Table_RereadCmd(ClientData clientData, register Tcl_Interp *interp,
	      int argc, char **argv)
{
  register Table *tablePtr = (Table *) clientData;

  if (!(tablePtr->flags & HAS_ACTIVE) ||
      (tablePtr->flags & ACTIVE_DISABLED) ||
      tablePtr->state == STATE_DISABLED) {
    return TCL_OK;
  }
  TableGetActiveBuf(tablePtr);
  TableRefresh(tablePtr, tablePtr->activeRow, tablePtr->activeCol,
	       CELL|INV_FORCE);
  return TCL_OK;
}

/*
 *--------------------------------------------------------------
 *
 * Table_ScanCmd --
 *	This procedure is invoked to process the bbox method
 *	that corresponds to a table widget managed by this module.
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
Table_ScanCmd(ClientData clientData, register Tcl_Interp *interp,
	      int argc, char **argv)
{
  register Table *tablePtr = (Table *) clientData;
  int x, y, row, col;

  if (argc != 5) {
    Tcl_AppendResult(interp, "wrong # args: should be \"",
		     argv[0], " scan mark|dragto x y\"", (char *) NULL);
    return TCL_ERROR;
  } else if (Tcl_GetInt(interp, argv[3], &x) == TCL_ERROR ||
	     Tcl_GetInt(interp, argv[4], &y) == TCL_ERROR) {
    return TCL_ERROR;
  }
  if ((argv[2][0] == 'm')
      && (strncmp(argv[2], "mark", strlen(argv[2])) == 0)) {
    TableWhatCell(tablePtr, x, y, &row, &col);
    tablePtr->scanMarkRow = row-tablePtr->topRow;
    tablePtr->scanMarkCol = col-tablePtr->leftCol;
    tablePtr->scanMarkX = x;
    tablePtr->scanMarkY = y;
  } else if ((argv[2][0] == 'd')
	     && (strncmp(argv[2], "dragto", strlen(argv[2])) == 0)) {
    int oldTop = tablePtr->topRow, oldLeft = tablePtr->leftCol;
    y += (5*(y-tablePtr->scanMarkY));
    x += (5*(x-tablePtr->scanMarkX));

    TableWhatCell(tablePtr, x, y, &row, &col);

    /* maintain appropriate real index */
    tablePtr->topRow  = MAX(MIN(row-tablePtr->scanMarkRow,
				tablePtr->rows-1), tablePtr->titleRows);
    tablePtr->leftCol = MAX(MIN(col-tablePtr->scanMarkCol,
				tablePtr->cols-1), tablePtr->titleCols);

    /* Adjust the table if new top left */
    if (oldTop != tablePtr->topRow || oldLeft != tablePtr->leftCol)
      TableAdjustParams(tablePtr);
  } else {
    Tcl_AppendResult(interp, "bad scan option \"", argv[2],
		     "\": must be mark or dragto", (char *) NULL);
    return TCL_ERROR;
  }
  return TCL_OK;
}

/*
 *--------------------------------------------------------------
 *
 * Table_SeeCmd --
 *	This procedure is invoked to process the bbox method
 *	that corresponds to a table widget managed by this module.
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
Table_SeeCmd(ClientData clientData, register Tcl_Interp *interp,
	      int argc, char **argv)
{
  register Table *tablePtr = (Table *) clientData;
  int row, col, x;

  if (argc != 3 || TableGetIndex(tablePtr,argv[2],&row,&col) == TCL_ERROR) {
    if (!strlen(Tcl_GetStringResult(interp))) {
      Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
		       " see index\"", (char *)NULL);
    }
    return TCL_ERROR;
  }
  /* Adjust from user to master coords */
  row -= tablePtr->rowOffset;
  col -= tablePtr->colOffset;
  if (!TableCellVCoords(tablePtr, row, col, &x, &x, &x, &x, 1)) {
    tablePtr->topRow  = row-1;
    tablePtr->leftCol = col-1;
    TableAdjustParams(tablePtr);
  }
  return TCL_OK;
}

/*
 *--------------------------------------------------------------
 *
 * Table_SelAnchorCmd --
 *	This procedure is invoked to process the selection anchor method
 *	that corresponds to a table widget managed by this module.
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
Table_SelAnchorCmd(ClientData clientData, register Tcl_Interp *interp,
	      int argc, char **argv)
{
  register Table *tablePtr = (Table *) clientData;
  int row, col;

  if (argc != 4) {
    Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
		     " selection anchor index\"", (char *)NULL);
    return TCL_ERROR;
  } else if (TableGetIndex(tablePtr,argv[3],&row,&col) != TCL_OK) {
    return TCL_ERROR;
  }
  tablePtr->flags |= HAS_ANCHOR;
  /* maintain appropriate real index */
  if (tablePtr->selectTitles) {
    tablePtr->anchorRow = MIN(MAX(0,row-tablePtr->rowOffset),
			      tablePtr->rows-1);
    tablePtr->anchorCol = MIN(MAX(0,col-tablePtr->colOffset),
			      tablePtr->cols-1);
  } else {
    tablePtr->anchorRow = MIN(MAX(tablePtr->titleRows,
				  row-tablePtr->rowOffset),
			      tablePtr->rows-1);
    tablePtr->anchorCol = MIN(MAX(tablePtr->titleCols,
				  col-tablePtr->colOffset),
			      tablePtr->cols-1);
  }
  return TCL_OK;
}

/*
 *--------------------------------------------------------------
 *
 * Table_SelClearCmd --
 *	This procedure is invoked to process the selection clear method
 *	that corresponds to a table widget managed by this module.
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
Table_SelClearCmd(ClientData clientData, register Tcl_Interp *interp,
	      int argc, char **argv)
{
  register Table *tablePtr = (Table *) clientData;
  int result = TCL_OK;
  char buf1[INDEX_BUFSIZE];
  int row, col, key, clo=0,chi=0,r1,c1,r2,c2;
  Tcl_HashEntry *entryPtr;

  if ( argc != 4 && argc != 5 ) {
    Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
		     " selection clear all|<first> ?<last>?\"",
		     (char *)NULL);
    return TCL_ERROR;
  }
  if (strcmp(argv[3],"all") == 0) {
    Tcl_HashSearch search;
    for(entryPtr = Tcl_FirstHashEntry(tablePtr->selCells, &search);
	entryPtr != NULL; entryPtr = Tcl_NextHashEntry(&search)) {
      TableParseArrayIndex(&row, &col,
			   Tcl_GetHashKey(tablePtr->selCells,entryPtr));
      Tcl_DeleteHashEntry(entryPtr);
      TableRefresh(tablePtr, row-tablePtr->rowOffset,
		   col-tablePtr->colOffset, CELL);
    }
    return TCL_OK;
  }
  if (TableGetIndex(tablePtr,argv[3],&row,&col) == TCL_ERROR ||
      (argc==5 && TableGetIndex(tablePtr,argv[4],&r2,&c2)==TCL_ERROR)) {
    return TCL_ERROR;
  }
  key = 0;
  if (argc == 4) {
    r1 = r2 = row;
    c1 = c2 = col;
  } else {
    r1 = MIN(row,r2); r2 = MAX(row,r2);
    c1 = MIN(col,c2); c2 = MAX(col,c2);
  }
  switch (tablePtr->selectType) {
  case SEL_BOTH:
    clo = c1; chi = c2;
    c1 = tablePtr->colOffset;
    c2 = tablePtr->cols-1+c1;
    key = 1;
    goto CLEAR_CELLS;
  CLEAR_BOTH:
    key = 0;
    c1 = clo; c2 = chi;
  case SEL_COL:
    r1 = tablePtr->rowOffset;
    r2 = tablePtr->rows-1+r1;
    break;
  case SEL_ROW:
    c1 = tablePtr->colOffset;
    c2 = tablePtr->cols-1+c1;
    break;
  }
  /* row/col are in user index coords */
CLEAR_CELLS:
  for ( row = r1; row <= r2; row++ ) {
    for ( col = c1; col <= c2; col++ ) {
      TableMakeArrayIndex(row, col, buf1);
      if ((entryPtr=Tcl_FindHashEntry(tablePtr->selCells, buf1))!=NULL) {
	Tcl_DeleteHashEntry(entryPtr);
	TableRefresh(tablePtr, row-tablePtr->rowOffset,
		     col-tablePtr->colOffset, CELL);
      }
    }
  }
  if (key) goto CLEAR_BOTH;
  return result;
}

/*
 *--------------------------------------------------------------
 *
 * Table_SelIncludesCmd --
 *	This procedure is invoked to process the selection includes method
 *	that corresponds to a table widget managed by this module.
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
Table_SelIncludesCmd(ClientData clientData, register Tcl_Interp *interp,
	      int argc, char **argv)
{
  register Table *tablePtr = (Table *) clientData;
  int row, col;
  char buf[INDEX_BUFSIZE];

  if (argc != 4) {
    Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
		     " selection includes index\"", (char *)NULL);
    return TCL_ERROR;
  } else if (TableGetIndex(tablePtr, argv[3], &row, &col) == TCL_ERROR) {
    return TCL_ERROR;
  } else {
    TableMakeArrayIndex(row, col, buf);
    if (Tcl_FindHashEntry(tablePtr->selCells, buf)) {
      Tcl_SetResult(interp, "1", TCL_STATIC);
    } else {
      Tcl_SetResult(interp, "0", TCL_STATIC);
    }
  }
  return TCL_OK;
}


/*
 *--------------------------------------------------------------
 *
 * Table_SelSetCmd --
 *	This procedure is invoked to process the selection set method
 *	that corresponds to a table widget managed by this module.
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
Table_SelSetCmd(ClientData clientData, register Tcl_Interp *interp,
	      int argc, char **argv)
{
  register Table *tablePtr = (Table *) clientData;
  int result = TCL_OK;
  int row, col, dummy, key;
  char buf1[INDEX_BUFSIZE];
  Tcl_HashSearch search;
  Tcl_HashEntry *entryPtr;

  int clo=0, chi=0, r1, c1, r2, c2, titleRows, titleCols;
  if (argc < 4 || argc > 5) {
    Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
		     " selection set first ?last?\"", (char *)NULL);
    return TCL_ERROR;
  }
  if (TableGetIndex(tablePtr,argv[3],&row,&col) == TCL_ERROR ||
      (argc==5 && TableGetIndex(tablePtr,argv[4],&r2,&c2)==TCL_ERROR)) {
    return TCL_ERROR;
  }
  key = 0;
  if (tablePtr->selectTitles) {
    titleRows = 0;
    titleCols = 0;
  } else {
    titleRows = tablePtr->titleRows;
    titleCols = tablePtr->titleCols;
  }
  /* maintain appropriate user index */
  row = MIN(MAX(titleRows+tablePtr->rowOffset, row),
	    tablePtr->rows-1+tablePtr->rowOffset);
  col = MIN(MAX(titleCols+tablePtr->colOffset, col),
	    tablePtr->cols-1+tablePtr->colOffset);
  if (argc == 4) {
    r1 = r2 = row;
    c1 = c2 = col;
  } else {
    r2 = MIN(MAX(titleRows+tablePtr->rowOffset, r2),
	     tablePtr->rows-1+tablePtr->rowOffset);
    c2 = MIN(MAX(titleCols+tablePtr->colOffset, c2),
	     tablePtr->cols-1+tablePtr->colOffset);
    r1 = MIN(row,r2); r2 = MAX(row,r2);
    c1 = MIN(col,c2); c2 = MAX(col,c2);
  }
  switch (tablePtr->selectType) {
  case SEL_BOTH:
    clo = c1; chi = c2;
    c1 = titleCols+tablePtr->colOffset;
    c2 = tablePtr->cols-1+tablePtr->colOffset;
    key = 1;
    goto SET_CELLS;
  SET_BOTH:
    key = 0;
    c1 = clo; c2 = chi;
  case SEL_COL:
    r1 = titleRows+tablePtr->rowOffset;
    r2 = tablePtr->rows-1+tablePtr->rowOffset;
    break;
  case SEL_ROW:
    c1 = titleCols+tablePtr->colOffset;
    c2 = tablePtr->cols-1+tablePtr->colOffset;
    break;
  }
SET_CELLS:
  entryPtr = Tcl_FirstHashEntry(tablePtr->selCells, &search);
  for ( row = r1; row <= r2; row++ ) {
    for ( col = c1; col <= c2; col++ ) {
      TableMakeArrayIndex(row, col, buf1);
      if (Tcl_FindHashEntry(tablePtr->selCells, buf1) == NULL) {
	Tcl_CreateHashEntry(tablePtr->selCells, buf1, &dummy);
	TableRefresh(tablePtr, row-tablePtr->rowOffset,
		     col-tablePtr->colOffset, CELL);
      }
    }
  }
  if (key) goto SET_BOTH;

      /* Adjust the table for top left, selection on screen etc */
  TableAdjustParams(tablePtr);

  /* If the table was previously empty and we want to export the
   * selection, we should grab it now */
  if (entryPtr==NULL && tablePtr->exportSelection) {
    Tk_OwnSelection(tablePtr->tkwin, XA_PRIMARY, TableLostSelection,
		    (ClientData) tablePtr);
  }
  return result;
}

/*
 *--------------------------------------------------------------
 *
 * Table_ValidateCmd --
 *	This procedure is invoked to process the validate method
 *	that corresponds to a table widget managed by this module.
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
Table_ValidateCmd(ClientData clientData, register Tcl_Interp *interp,
	      int argc, char **argv)
{
  register Table *tablePtr = (Table *) clientData;
  int row, col, value, result;
  char buf[INDEX_BUFSIZE];

  if (argc != 3) {
    Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
		     " validate index\"", (char *) NULL);
    return TCL_ERROR;
  } else if (TableGetIndex(tablePtr, argv[2], &row, &col) == TCL_ERROR) {
    return TCL_ERROR;
  }
  value = tablePtr->validate;
  tablePtr->validate = 1;
  result = TableValidateChange(tablePtr, row, col, (char *) NULL,
			       (char *) NULL, -1);
  tablePtr->validate = value;
  sprintf(buf, "%d", (result == TCL_OK) ? 1 : 0);
  Tcl_SetResult(interp, buf, TCL_VOLATILE);
  return TCL_OK;
}

/*
 *--------------------------------------------------------------
 *
 * Table_VersionCmd --
 *	This procedure is invoked to process the bbox method
 *	that corresponds to a table widget managed by this module.
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
Table_VersionCmd(ClientData clientData, register Tcl_Interp *interp,
		 int argc, char **argv)
{
  if (argc != 2) {
    Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
		     " version\"", (char *) NULL);
    return TCL_ERROR;
  } else {
    Tcl_SetResult(interp, TBL_VERSION, TCL_VOLATILE);
  }
  return TCL_OK;
}

/*
 *--------------------------------------------------------------
 *
 * Table_ViewCmd --
 *	This procedure is invoked to process the x|yview method
 *	that corresponds to a table widget managed by this module.
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
Table_ViewCmd(ClientData clientData, register Tcl_Interp *interp,
	      int argc, char **argv)
{
  register Table *tablePtr = (Table *) clientData;
  int result = TCL_OK;
  int yview, row, col, value;

  /* Check xview or yview */
  yview = (argv[1][0] == 'y');

  if (argc == 2) {
    int diff, x, y, w, h;
    double first, last;
    char buf1[INDEX_BUFSIZE];

    TableGetLastCell(tablePtr, &row, &col);
    TableCellVCoords(tablePtr, row, col, &x, &y, &w, &h, 0);
    if (yview) {
      if (row < tablePtr->titleRows) {
	first = 0;
	last  = 1;
      } else {
	diff = tablePtr->rowStarts[tablePtr->titleRows];
	last = (double) (tablePtr->rowStarts[tablePtr->rows]-diff);
	first = (tablePtr->rowStarts[tablePtr->topRow]-diff) / last;
	last  = (h+tablePtr->rowStarts[row]-diff) / last;
      }
    } else {
      if (col < tablePtr->titleCols) {
	first = 0;
	last  = 1;
      } else {
	diff = tablePtr->colStarts[tablePtr->titleCols];
	last = (double) (tablePtr->colStarts[tablePtr->cols]-diff);
	first = (tablePtr->colStarts[tablePtr->leftCol]-diff) / last;
	last  = (w+tablePtr->colStarts[col]-diff) / last;
      }
    }
    sprintf(buf1, "%g %g", first, last);
    Tcl_SetResult(interp, buf1, TCL_VOLATILE);
  } else {
    /* cache old topleft to see if it changes */
    int oldTop = tablePtr->topRow, oldLeft = tablePtr->leftCol;

    if (argc == 3) {
      if (Tcl_GetInt(interp, argv[2], &value) != TCL_OK) {
	return TCL_ERROR;
      }
      if (yview) {
	tablePtr->topRow  = value + tablePtr->titleRows;
      } else {
	tablePtr->leftCol = value + tablePtr->titleCols;
      }
    } else {
      double frac;
      switch (Tk_GetScrollInfo(interp, argc, argv, &frac, &value)) {
      case TK_SCROLL_ERROR:
	return TCL_ERROR;
      case TK_SCROLL_MOVETO:
	if (frac < 0) frac = 0;
	if (yview) {
	  tablePtr->topRow = (int)(frac*tablePtr->rows)+tablePtr->titleRows;
	} else {
	  tablePtr->leftCol = (int)(frac*tablePtr->cols)+tablePtr->titleCols;
	}
	break;
      case TK_SCROLL_PAGES:
	TableGetLastCell(tablePtr, &row, &col);
	if (yview) {
	  tablePtr->topRow  += value * (row-tablePtr->topRow+1);
	} else {
	  tablePtr->leftCol += value * (col-tablePtr->leftCol+1);
	}
	break;
      case TK_SCROLL_UNITS:
	if (yview) {
	  tablePtr->topRow  += value;
	} else {
	  tablePtr->leftCol += value;
	}
	break;
      }
    }
    /* maintain appropriate real index */
    tablePtr->topRow  = MAX(tablePtr->titleRows,
			    MIN(tablePtr->topRow, tablePtr->rows-1));
    tablePtr->leftCol = MAX(tablePtr->titleCols,
			    MIN(tablePtr->leftCol, tablePtr->cols-1));
    /* Do the table adjustment if topRow || leftCol changed */	
    if (oldTop != tablePtr->topRow || oldLeft != tablePtr->leftCol)
      TableAdjustParams(tablePtr);
  }

  return result;
}


/*
 *--------------------------------------------------------------
 *
 * Table_Cmd --
 *	This procedure is invoked to process the CMD method
 *	that corresponds to a table widget managed by this module.
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
Table_Cmd(ClientData clientData, register Tcl_Interp *interp,
	      int argc, char **argv)
{
  register Table *tablePtr = (Table *) clientData;
  int result = TCL_OK;

  return result;
}

