/* 
 * tkTableEdit.c --
 *
 *	This module implements editing functions of a table widget.
 *
 * Copyright (c) 1998 Jeffrey Hobbs
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include "tkTable.h"


/* insert/delete subcommands */
#define MOD_ACTIVE	1
#define MOD_COLS	2
#define MOD_ROWS	3
static Cmd_Struct modCmds[] = {
  {"active",	MOD_ACTIVE},
  {"cols",	MOD_COLS},
  {"rows",	MOD_ROWS},
  {"", 0}
};

/*
 *--------------------------------------------------------------
 *
 * Table_EditCmd --
 *	This procedure is invoked to process the insert/delete method
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
Table_EditCmd(ClientData clientData, register Tcl_Interp *interp,
	      int argc, char **argv)
{
  register Table *tablePtr = (Table *) clientData;
  int insertType, retval, value, posn;

  insertType = (strncmp(argv[1], "insert", strlen(argv[1]))==0);
  if (argc < 4) {
    Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
		     insertType ? " insert" : " delete",
		     " option ?switches? arg ?arg?\"",
		     (char *) NULL);
    return TCL_ERROR;
  }
  retval = Cmd_Parse(interp, modCmds, argv[2]);
  switch (retval) {
  case 0:
    return TCL_ERROR;
  case MOD_ACTIVE:
    if (insertType) {
      if (argc != 5) {
	Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
			 " insert active index string\"", (char *)NULL);
	return TCL_ERROR;
      } else if (TableGetIcursor(tablePtr, argv[3], &posn) != TCL_OK) {
	return TCL_ERROR;
      } else if ((tablePtr->flags & HAS_ACTIVE) &&
		 !(tablePtr->flags & ACTIVE_DISABLED) &&
		 tablePtr->state == STATE_NORMAL) {
	TableInsertChars(tablePtr, posn, argv[4]);
      }
    } else {
      if (argc > 5) {
	Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
			 " delete active first ?last?\"", (char *) NULL);
	return TCL_ERROR;
      }
      if (TableGetIcursor(tablePtr, argv[3], &posn) == TCL_ERROR) {
	return TCL_ERROR;
      }
      if (argc == 4) {
	value = posn+1;
      } else if (TableGetIcursor(tablePtr, argv[4], &value) == TCL_ERROR) {
	return TCL_ERROR;
      }
      if (value >= posn && (tablePtr->flags & HAS_ACTIVE) &&
	  !(tablePtr->flags & ACTIVE_DISABLED) &&
	  tablePtr->state == STATE_NORMAL)
	TableDeleteChars(tablePtr, posn, value-posn);
    }
    break;	/* EDIT ACTIVE */
  case MOD_COLS:
  case MOD_ROWS:
    return TableModifyRC(tablePtr, interp, insertType?1:0,
			 (retval==MOD_ROWS)?1:0, argc, argv);
  }
  return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * TableDeleteChars --
 *	Remove one or more characters from an table widget.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Memory gets freed, the table gets modified and (eventually)
 *	redisplayed.
 *
 *----------------------------------------------------------------------
 */
void
TableDeleteChars(tablePtr, index, count)
    register Table *tablePtr;	/* Table widget to modify. */
    int index;			/* Index of first character to delete. */
    int count;			/* How many characters to delete. */
{
#ifdef TCL_UTF_MAX
  int byteIndex, byteCount, newByteCount, numBytes, numChars;
  char *new, *string;

  string = tablePtr->activeBuf;
  numBytes = strlen(string);
  numChars = Tcl_NumUtfChars(string, numBytes);
  if ((index + count) > numChars) {
    count = numChars - index;
  }
  if (count <= 0) {
    return;
  }

  byteIndex = Tcl_UtfAtIndex(string, index) - string;
  byteCount = Tcl_UtfAtIndex(string + byteIndex, count) - (string + byteIndex);

  newByteCount = numBytes + 1 - byteCount;
  new = (char *) ckalloc((unsigned) newByteCount);
  memcpy(new, string, (size_t) byteIndex);
  strcpy(new + byteIndex, string + byteIndex + byteCount);
#else
  int oldlen;
  char *new;

  /* this gets the length of the string, as well as ensuring that
   * the cursor isn't beyond the end char */
  TableGetIcursor(tablePtr, "end", &oldlen);

  if ((index+count) > oldlen)
    count = oldlen-index;
  if (count <= 0)
    return;

  new = (char *) ckalloc((unsigned)(oldlen-count+1));
  strncpy(new, tablePtr->activeBuf, (size_t) index);
  strcpy(new+index, tablePtr->activeBuf+index+count);
  /* make sure this string is null terminated */
  new[oldlen-count] = '\0';
#endif
  /* This prevents deletes on BREAK or validation error. */
  if (tablePtr->validate &&
      TableValidateChange(tablePtr, tablePtr->activeRow+tablePtr->rowOffset,
			  tablePtr->activeCol+tablePtr->colOffset,
			  tablePtr->activeBuf, new, index) != TCL_OK) {
    ckfree(new);
    return;
  }

  ckfree(tablePtr->activeBuf);
  tablePtr->activeBuf = new;

  /* mark the text as changed */
  tablePtr->flags |= TEXT_CHANGED;

  if (tablePtr->icursor >= index) {
    if (tablePtr->icursor >= (index+count)) {
      tablePtr->icursor -= count;
    } else {
      tablePtr->icursor = index;
    }
  }

  TableSetActiveIndex(tablePtr);

  TableRefresh(tablePtr, tablePtr->activeRow, tablePtr->activeCol,
	       CELL|INV_FORCE);
}

/*
 *----------------------------------------------------------------------
 *
 * TableInsertChars --
 *	Add new characters to the active cell of a table widget.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	New information gets added to tablePtr; it will be redisplayed
 *	soon, but not necessarily immediately.
 *
 *----------------------------------------------------------------------
 */
void
TableInsertChars(tablePtr, index, value)
    register Table *tablePtr;	/* Table that is to get the new elements. */
    int index;			/* Add the new elements before this element. */
    char *value;		/* New characters to add (NULL-terminated
				 * string). */
{
#ifdef TCL_UTF_MAX
  int oldlen, byteIndex, byteCount;
  char *new, *string;

  byteCount = strlen(value);
  if (byteCount == 0) {
    return;
  }

  /* Is this an autoclear and this is the first update */
  /* Note that this clears without validating */
  if (tablePtr->autoClear && !(tablePtr->flags & TEXT_CHANGED)) {
    /* set the buffer to be empty */
    tablePtr->activeBuf = (char *)ckrealloc(tablePtr->activeBuf, 1);
    tablePtr->activeBuf[0] = '\0';
    /* the insert position now has to be 0 */
    index = 0;
    tablePtr->icursor = 0;
  }

  string = tablePtr->activeBuf;
  byteIndex = Tcl_UtfAtIndex(string, index) - string;

  oldlen = strlen(string);
  new = (char *) ckalloc((unsigned)(oldlen + byteCount + 1));
  memcpy(new, string, (size_t) byteIndex);
  strcpy(new + byteIndex, value);
  strcpy(new + byteIndex + byteCount, string + byteIndex);

  /* validate potential new active buffer */
  /* This prevents inserts on either BREAK or validation error. */
  if (tablePtr->validate &&
      TableValidateChange(tablePtr, tablePtr->activeRow+tablePtr->rowOffset,
			  tablePtr->activeCol+tablePtr->colOffset,
			  tablePtr->activeBuf, new, byteIndex) != TCL_OK) {
    ckfree(new);
    return;
  }

  /*
   * The following construction is used because inserting improperly
   * formed UTF-8 sequences between other improperly formed UTF-8
   * sequences could result in actually forming valid UTF-8 sequences;
   * the number of characters added may not be Tcl_NumUtfChars(string, -1),
   * because of context.  The actual number of characters added is how
   * many characters were are in the string now minus the number that
   * used to be there.
   */

  if (tablePtr->icursor >= index) {
    tablePtr->icursor += Tcl_NumUtfChars(new, oldlen+byteCount)
      - Tcl_NumUtfChars(tablePtr->activeBuf, oldlen);
  }

  ckfree(string);
  tablePtr->activeBuf = new;

#else
  int oldlen, newlen;
  char *new;

  newlen = strlen(value);
  if (newlen == 0) return;

  /* Is this an autoclear and this is the first update */
  /* Note that this clears without validating */
  if (tablePtr->autoClear && !(tablePtr->flags & TEXT_CHANGED)) {
    /* set the buffer to be empty */
    tablePtr->activeBuf = (char *)ckrealloc(tablePtr->activeBuf, 1);
    tablePtr->activeBuf[0] = '\0';
    /* the insert position now has to be 0 */
    index = 0;
  }
  oldlen = strlen(tablePtr->activeBuf);
  /* get the buffer to at least the right length */
  new = (char *) ckalloc((unsigned)(oldlen+newlen+1));
  strncpy(new, tablePtr->activeBuf, (size_t) index);
  strcpy(new+index, value);
  strcpy(new+index+newlen, (tablePtr->activeBuf)+index);
  /* make sure this string is null terminated */
  new[oldlen+newlen] = '\0';

  /* validate potential new active buffer */
  /* This prevents inserts on either BREAK or validation error. */
  if (tablePtr->validate &&
      TableValidateChange(tablePtr, tablePtr->activeRow+tablePtr->rowOffset,
			  tablePtr->activeCol+tablePtr->colOffset,
			  tablePtr->activeBuf, new, index) != TCL_OK) {
    ckfree(new);
    return;
  }
  ckfree(tablePtr->activeBuf);
  tablePtr->activeBuf = new;

  if (tablePtr->icursor >= index) {
    tablePtr->icursor += newlen;
  }
#endif

  /* mark the text as changed */
  tablePtr->flags |= TEXT_CHANGED;

  TableSetActiveIndex(tablePtr);

  TableRefresh(tablePtr, tablePtr->activeRow, tablePtr->activeCol,
	       CELL|INV_FORCE);
}

/*
 *----------------------------------------------------------------------
 *
 * TableModifyRCaux --
 *	Helper function that does the core work of moving rows/cols
 *	and associated tags.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Moves cell data and possibly tag data
 *
 *----------------------------------------------------------------------
 */
static void
TableModifyRCaux(tablePtr, which, movetag, tagTblPtr, dimTblPtr,
		 offset, from, to, lo, hi, check)
    Table *tablePtr;	/* Information about text widget. */
    int which;		/* rows (1) or cols (0) */
    int movetag;	/* whether tags are supposed to be moved */
    Tcl_HashTable *tagTblPtr, *dimTblPtr; /* Pointers to the row/col tags
					   * and width/height tags */
    int offset;		/* appropriate offset */
    int from, to;	/* the from and to row/col */
    int lo, hi;		/* the lo and hi col/row */
    int check;		/* the boundary check for shifting items */
{
  int j, dummy;
  char buf[INDEX_BUFSIZE], buf1[INDEX_BUFSIZE];
  Tcl_HashEntry *entryPtr, *newPtr;

  /* move row/col style && width/height here */
  if (movetag) {
    if ((entryPtr=Tcl_FindHashEntry(tagTblPtr, (char *)from)) != NULL) {
      Tcl_DeleteHashEntry(entryPtr);
    }
    if ((entryPtr=Tcl_FindHashEntry(dimTblPtr, (char *)from-offset)) != NULL) {
      Tcl_DeleteHashEntry(entryPtr);
    }
    if (!check) {
      if ((entryPtr=Tcl_FindHashEntry(tagTblPtr, (char *)to)) != NULL) {
	newPtr = Tcl_CreateHashEntry(tagTblPtr, (char *)from, &dummy);
	Tcl_SetHashValue(newPtr, Tcl_GetHashValue(entryPtr));
	Tcl_DeleteHashEntry(entryPtr);
      }
      if ((entryPtr=Tcl_FindHashEntry(dimTblPtr, (char *)to-offset)) != NULL) {
	newPtr = Tcl_CreateHashEntry(dimTblPtr, (char *)from-offset, &dummy);
	Tcl_SetHashValue(newPtr, Tcl_GetHashValue(entryPtr));
	Tcl_DeleteHashEntry(entryPtr);
      }
    }
  }
  for (j = lo; j <= hi; j++) {
    if (which /* rows */) {
      TableMakeArrayIndex(from, j, buf);
      TableMakeArrayIndex(to, j, buf1);
      TableSetCellValue(tablePtr, from, j, check ? "" :
			TableGetCellValue(tablePtr, to, j));
    } else {
      TableMakeArrayIndex(j, from, buf);
      TableMakeArrayIndex(j, to, buf1);
      TableSetCellValue(tablePtr, j, from, check ? "" :
			TableGetCellValue(tablePtr, j, to));
    }
    /* move cell style here */
    if (movetag) {
      if ((entryPtr=Tcl_FindHashEntry(tablePtr->cellStyles,buf)) != NULL) {
	Tcl_DeleteHashEntry(entryPtr);
      }
      if (!check &&
	  (entryPtr=Tcl_FindHashEntry(tablePtr->cellStyles,buf1))!=NULL) {
	newPtr = Tcl_CreateHashEntry(tablePtr->cellStyles, buf, &dummy);
	Tcl_SetHashValue(newPtr, Tcl_GetHashValue(entryPtr));
	Tcl_DeleteHashEntry(entryPtr);
      }
    }
  }
}

/*
 *----------------------------------------------------------------------
 *
 * TableModifyRC --
 *	Modify rows/cols of the table (insert or delete)
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Modifies associated row/col data
 *
 *----------------------------------------------------------------------
 */
int
TableModifyRC(tablePtr, interp, insertType, which, argc, argv)
    Table *tablePtr;		/* Information about text widget. */
    Tcl_Interp *interp;		/* Current interpreter. */
    int insertType;		/* insert (1) | delete (0) */
    int which;			/* rows (1) or cols (0) */
    int argc;			/* Number of arguments. */
    char **argv;		/* Argument strings. */
{
  int i, first, lo, hi, idx, c, argsLeft, x, y, offset, minkeyoff;
  int maxrow, maxcol, maxkey, minkey, movetitle, movetag, movedim;
  size_t length;
  char *arg;
  Tcl_HashTable *tagTblPtr, *dimTblPtr;
  Tcl_HashSearch search;
  int *dimPtr;

  movetitle	= 1;
  movetag	= 1;
  movedim	= 1;
  maxcol	= tablePtr->cols-1+tablePtr->colOffset;
  maxrow	= tablePtr->rows-1+tablePtr->rowOffset;
  for (i = 3; i < argc; i++) {
    arg = argv[i];
    if (arg[0] != '-') {
      break;
    }
    length = strlen(arg);
    if (length < 2) {
    badSwitch:
      Tcl_AppendResult(interp, "bad switch \"", arg,
		       "\": must be -cols, -keeptitles, -holddimensions, ",
		       "-holdtags, -rows, or --",
		       (char *) NULL);
      return TCL_ERROR;
    }
    c = arg[1];
    if ((c == 'h') && (length > 5) &&
	(strncmp(argv[i], "-holddimensions", length) == 0)) {
      movedim = 0;
    } else if ((c == 'h') && (length > 5) &&
	       (strncmp(argv[i], "-holdtags", length) == 0)) {
      movetag = 0;
    } else if ((c == 'k') && (strncmp(argv[i], "-keeptitles", length) == 0)) {
      movetitle = 0;
    } else if ((c == 'c') && (strncmp(argv[i], "-cols", length) == 0)) {
      if (i >= (argc-1)) {
	Tcl_SetResult(interp, "no value given for \"-cols\" option",
		      TCL_STATIC);
	return TCL_ERROR;
      }
      if (Tcl_GetInt(interp, argv[++i], &maxcol) != TCL_OK) {
	return TCL_ERROR;
      }
      maxcol = MAX(maxcol, tablePtr->titleCols+tablePtr->colOffset);
    } else if ((c == 'r') && (strncmp(argv[i], "-rows", length) == 0)) {
      if (i >= (argc-1)) {
	Tcl_SetResult(interp, "no value given for \"-rows\" option",
		      TCL_STATIC);
	return TCL_ERROR;
      }
      if (Tcl_GetInt(interp, argv[++i], &maxrow) != TCL_OK) {
	return TCL_ERROR;
      }
      maxrow = MAX(maxrow, tablePtr->titleRows+tablePtr->rowOffset);
    } else if ((c == '-') && (strncmp(argv[i], "--", length) == 0)) {
      i++;
      break;
    } else {
      goto badSwitch;
    }
  }
  argsLeft = argc - i;
  if (argsLeft != 1 && argsLeft != 2) {
    Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
		     (insertType) ? " insert" : " delete",
		     (which /* rows */) ? " rows" : " cols",
		     " ?switches? index ?count?\"", (char *) NULL);
    return TCL_ERROR;
  }

  c = 1;
  if (strcmp(argv[i], "end") == 0) {
    /* allow "end" to be specified as an index */
    idx = (which /* rows */) ? maxrow : maxcol ;
  } else if (Tcl_GetInt(interp, argv[i], &idx) != TCL_OK) {
    return TCL_ERROR;
  }
  if (argsLeft == 2 && Tcl_GetInt(interp, argv[++i], &c) != TCL_OK) {
    return TCL_ERROR;
  }
  if (tablePtr->state == STATE_DISABLED || c == 0)
    return TCL_OK;

  if (which /* rows */) {
    maxkey	= maxrow;
    minkey	= tablePtr->rowOffset;
    minkeyoff	= tablePtr->rowOffset+tablePtr->titleRows;
    lo		= tablePtr->colOffset+(movetitle?0:tablePtr->titleCols);
    hi		= maxcol;
    offset	= tablePtr->rowOffset;
    tagTblPtr	= tablePtr->rowStyles;
    dimTblPtr	= tablePtr->rowHeights;
    dimPtr	= &(tablePtr->rows);
  } else {
    maxkey	= maxcol;
    minkey	= tablePtr->colOffset;
    minkeyoff	= tablePtr->colOffset+tablePtr->titleCols;
    lo		= tablePtr->rowOffset+(movetitle?0:tablePtr->titleRows);
    hi		= maxrow;
    offset	= tablePtr->colOffset;
    tagTblPtr	= tablePtr->colStyles;
    dimTblPtr	= tablePtr->colWidths;
    dimPtr	= &(tablePtr->cols);
  }

  if (insertType) {
    /* Handle row/col insertion */
    first = MAX(MIN(idx, maxkey), minkey);
    /* +count means insert after index, -count means insert before index */
    if (c < 0) {
      c = -c;
    } else {
      first++;
    }
    if (movedim) {
      maxkey += c;
      *dimPtr += c;
    }
    if (!movetitle && (first < minkeyoff)) {
      c -= minkeyoff-first;
      if (c <= 0)
	return TCL_OK;
      first = MAX(first, minkeyoff);
    }
    for (i = maxkey; i >= first; i--) {
      /* move row/col style && width/height here */
      TableModifyRCaux(tablePtr, which, movetag, tagTblPtr,
		       dimTblPtr, offset, i, i-c, lo, hi, ((i-c)<first));
    }
  } else {
    /* Handle row/col deletion */
    first = MAX(MIN(idx,idx+c), minkey);
    /* (index = i && count = 1) == (index = i && count = -1) */
    if (c < 0) {
      /* if the count is negative, make sure that the col count will delete
       * no greater than the original index */
      c = idx-first;
      first++;
    }
    if (movedim) {
      *dimPtr -= c;
    }
    if (!movetitle && (first < minkeyoff)) {
      c -= minkeyoff-first;
      if (c <= 0)
	return TCL_OK;
      first = MAX(first, minkeyoff);
    }
    for (i = first; i <= maxkey; i++) {
      TableModifyRCaux(tablePtr, which, movetag, tagTblPtr,
		       dimTblPtr, offset, i, i+c, lo, hi, ((i+c)>maxkey));
    }
  }
  if (Tcl_FirstHashEntry(tablePtr->selCells, &search) != NULL) {
    /* clear selection - forceful, but effective */
    Tcl_DeleteHashTable(tablePtr->selCells);
    ckfree((char *) (tablePtr->selCells));
    tablePtr->selCells = (Tcl_HashTable *)ckalloc(sizeof(Tcl_HashTable));
    Tcl_InitHashTable(tablePtr->selCells, TCL_STRING_KEYS);
  }

  TableAdjustParams(tablePtr);
  /* change the geometry */
  TableGeometryRequest(tablePtr);
  /* FIX: make this TableRefresh */
  if (which /* rows */) {
    TableCellCoords(tablePtr, first, 0, &x, &y, &offset, &offset);
    TableInvalidate(tablePtr, x, y,
		    Tk_Width(tablePtr->tkwin),
		    Tk_Height(tablePtr->tkwin)-y, 0);
  } else {
    TableCellCoords(tablePtr, 0, first, &x, &y, &offset, &offset);
    TableInvalidate(tablePtr, x, y,
		    Tk_Width(tablePtr->tkwin)-x,
		    Tk_Height(tablePtr->tkwin), 0);
  }
  return TCL_OK;
}
