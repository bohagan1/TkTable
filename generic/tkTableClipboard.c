/*
 * tkTableClipboard.c --
 *
 *	This module implements clipboard import/export commands for table
 *	widgets.
 *
 * Copyright (c) 1998-2002 Jeffrey Hobbs
 * Copyright (c) 2025 Brian O'Hagan
 *
 * See the file "license.txt" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include "tkTable.h"

/*
 *----------------------------------------------------------------------
 *
 * TableSelectionGetSingle --
 *	This function gets the selected cells from the table, encodes
 *	their values using 1-byte delimiters, and returns the data in
 *	the dynamic string.
 *
 * Results:
 *	Returns TCL result with dynamic string set to selected data.
 *
 * Side effects:
 *	None
 *
 *----------------------------------------------------------------------
 */

static int TableSelectionGetSingle(
    Table *tablePtr,
    Tcl_DString *dsPtr) {

    Tcl_Size dlen, i, llen;
    char special[5];
    Tcl_Obj *listPtr, *itemPtr;
    const char *data;
    char *colsep = tablePtr->colSep, *rowsep = tablePtr->rowSep, *quoteChar = tablePtr->quoteChar;
    int col, row, prev = 0;

    /* Set special characters */
    special[0] = colsep[0];
    special[1] = rowsep[0];
    special[2] = quoteChar[0];
    special[3] = '\r';
    special[4] = '\0';

    /* Get selected cells  */
    listPtr = TableGetSortedIndices(tablePtr->selCells);
    if (Tcl_ListObjLength(tablePtr->interp, listPtr, &llen) == TCL_ERROR || llen == 0) {
	return TCL_ERROR;
    }

    /* Get cell values and convert to string using delimiters */
    for (i = 0; i < llen; i++) {
	/* Get cell index */
	if (Tcl_ListObjIndex(tablePtr->interp, listPtr, i, &itemPtr) == TCL_OK) {
	    TableParseArrayIndex(&row, &col, Tcl_GetString(itemPtr));
	} else {
	    break;
	}

	/* Get cell value */
	data = TableGetCellValue(tablePtr, row, col);
	dlen = (Tcl_Size)strlen(data);

	/* Append column/row separator */
	if (i > 0) {
	    if (row == prev) {
		Tcl_DStringAppend(dsPtr, colsep, 1);
	    } else {
		Tcl_DStringAppend(dsPtr, rowsep, 1);
	    }
	}

	/* Append cell value if no special chars or quote with escapes if there are */
	if (strpbrk(data, special) == NULL) {
	    Tcl_DStringAppend(dsPtr, data, dlen);
	} else {
	    const char *end = data + dlen, *ptr;
	    Tcl_DStringAppend(dsPtr, quoteChar, 1);
	    while (*data && (ptr = strchr(data, (int)quoteChar[0])) != NULL) {
		Tcl_DStringAppend(dsPtr, data, (Tcl_Size) (ptr - data + 1));
		Tcl_DStringAppend(dsPtr, quoteChar, 1);
		data = ptr + 1;
	    }
	    Tcl_DStringAppend(dsPtr, data, (Tcl_Size) (end - data));
	    Tcl_DStringAppend(dsPtr, quoteChar, 1);
	}
	prev = row;
    }

    /* Clean-up */
    Tcl_IncrRefCount(listPtr);
    Tcl_DecrRefCount(listPtr);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * TableClipboardExport --
 *	This function is called by the user to return the selection in
 *	clipboard format so it can be appended to the clipboard.
 *
 * Results:
 *	Returns TCL result with interp result set to selected data.
 *
 * Side effects:
 *	None
 *
 *----------------------------------------------------------------------
 */

static int TableClipboardExport(
    ClientData clientData,	/* Information about the table widget. */
    Tcl_Interp *interp) {	/* TCL interpreter. */

    Table *tablePtr = (Table *) clientData;
    char *colsep = tablePtr->colSep, *rowsep = tablePtr->rowSep, *quoteChar = tablePtr->quoteChar;
    int clen = (colsep?((int) strlen(tablePtr->colSep)):0);
    int rlen = (rowsep?((int) strlen(tablePtr->rowSep)):0);
    int qlen = (quoteChar?((int) strlen(tablePtr->quoteChar)):0);
    Tcl_DString ds;

    /* if we have no data source, abort */
    if (tablePtr->dataSource == DATA_NONE) {
	return -1;
    }

    Tcl_DStringInit(&ds);

    /* Get selected data */
    if ((tablePtr->flags & HAS_ACTIVE) && tablePtr->activeRow != -1 &&
	    tablePtr->selectFirst != -1 && tablePtr->selectFirst < tablePtr->selectLast) {
	/* Get active cell selected text */
	Tcl_Size byteCount;
	const char *data;

	data = Tcl_UtfAtIndex(tablePtr->activeBuf, (Tcl_Size)tablePtr->selectFirst);
	byteCount = (Tcl_Size) (Tcl_UtfAtIndex(tablePtr->activeBuf,
	    (Tcl_Size)tablePtr->selectLast) - data);
	Tcl_DStringAppend(&ds, data, byteCount);

    } else if (clen == 1 && rlen == 1 && qlen == 1) {
	/* Get cell values using single byte delimiter characters */
	TableSelectionGetSingle(tablePtr, &ds);

    } else if (clen == 0 && rlen == 0) {
	/* Get cell values in list format */
	Tcl_AppendResult(interp, "Unsupported delimiter char lengths", (char *)NULL);
	return TCL_ERROR;

    } else {
	/* Get cell values using multi-byte delimiter characters */
	Tcl_AppendResult(interp, "Unsupported delimiter char lengths", (char *)NULL);
	return TCL_ERROR;
    }
    Tcl_DStringResult(interp, &ds);
    return TCL_OK;
}


#define MAX_CELL_SIZE 65536
typedef struct {
    Table *tablePtr;		/* Information about the table widget. */
    Tcl_Obj *listPtr;		/* Row of cell data as a TCL list */
    Tcl_Obj *resultPtr;		/* Selection in list of lists format */
    int escaped;		/* Is the next character escaped */
    int quoted;			/* Is cell text quoted */
    int length;			/* Length of data in buffer in bytes */
    char *buffer;		/* Temp cell data buffer */
} TableGetSelection;

/*
 *----------------------------------------------------------------------
 *
 * Table_ImportHandler --
 *	This function is the handler used by TableClipboardImport to
 *	convert the clipboard data into the list of lists format.
 *
 * Results:
 *	Returns TCL result.
 *
 * Side effects:
 *	Process a portion of the imported clipboard contents.
 *
 * Io do: Convert to use Unicode functions.
 *
 *----------------------------------------------------------------------
 */

int Table_ImportHandler(
    ClientData clientData,	/* Information about selection. */
    Tcl_Interp *interp,		/* TCL interpreter. */
    const char *portion) {	/* Portion of selection to process. */

    TableGetSelection *dataPtr = (TableGetSelection *) clientData;
    Table *tablePtr = dataPtr->tablePtr;
    char *colsep = tablePtr->colSep, *rowsep = tablePtr->rowSep, *quoteChar = tablePtr->quoteChar;
    int i, len, clen=0, rlen=0, qlen=0;
    Tcl_Obj *tmpPtr;

    if (portion == NULL) {
	Tcl_AppendResult(interp, "No data", (char *)NULL);
	return TCL_ERROR;
    }

    len = (int) strlen(portion);
    clen = (colsep?((int) strlen(colsep)):0);
    rlen = (rowsep?((int) strlen(rowsep)):0);
    qlen = (quoteChar?((int) strlen(quoteChar)):0);

    /* Works if colsep, rowsep, and quoteChar are all 1 byte */
    if (clen != 1 || rlen != 1 || qlen != 1) {
	Tcl_AppendResult(interp, "Unsupported delimiter char lengths", (char *)NULL);
	return TCL_ERROR;
    }

    for (i = 0; i < len; i++) {
	if (dataPtr->length == 0 && portion[i] == quoteChar[0]) {
	    /* Start of quoted cell text */
	    dataPtr->quoted = 1;

	} else if (!(dataPtr->escaped) && !(dataPtr->quoted) && portion[i] == '\\') {
	    /* Escape character */
	    dataPtr->escaped = 1;

	} else if (!(dataPtr->escaped) && !(dataPtr->quoted) && portion[i] == colsep[0]) {
	    /* End of cell text */
	    if ((tmpPtr = Tcl_NewStringObj(dataPtr->buffer, dataPtr->length)) == NULL ||
		Tcl_ListObjAppendElement(interp, dataPtr->listPtr, tmpPtr) != TCL_OK) {
		return TCL_ERROR;
	    }
	    dataPtr->length = 0;

	} else if (!(dataPtr->escaped) && !(dataPtr->quoted) && portion[i] == rowsep[0]) {
	    /* End of row */
	    if ((tmpPtr = Tcl_NewStringObj(dataPtr->buffer, dataPtr->length)) == NULL ||
		Tcl_ListObjAppendElement(interp, dataPtr->listPtr, tmpPtr) != TCL_OK ||
		Tcl_ListObjAppendElement(interp, dataPtr->resultPtr, dataPtr->listPtr) != TCL_OK) {
		return TCL_ERROR;
	    }
	    dataPtr->length = 0;
	    dataPtr->listPtr = Tcl_NewListObj(0, NULL);

	} else if (!(dataPtr->escaped) &&(dataPtr->quoted) && portion[i] == quoteChar[0]) {
	    /* Embedded quote or end of quoted cell text */
	    if (portion[i+1] == quoteChar[0]) {
		dataPtr->buffer[dataPtr->length++] = portion[i++];
	    } else {
		dataPtr->quoted = 0;
	    }

	} else {
	    /* Cell text character */
	    dataPtr->buffer[dataPtr->length++] = portion[i];
	    dataPtr->escaped = 0;
	}

	if (dataPtr->length >= MAX_CELL_SIZE) {
	    Tcl_AppendResult(interp, "Cell text too large", (char *)NULL);
	    return TCL_ERROR;
	}
    }
    return TCL_OK;
 }

/*
 *----------------------------------------------------------------------
 *
 * TableClipboardImport --
 *	This function is called by the user when the selection is to be
 *	retrieved from the clipboard and converted into a list of lists
 *	(list of cell values for each row) for insertion into the table.
 *
 * Results:
 *	Returns TCL result with result set to a list of lists with cell
 *	values suitable for insertion into the table.
 *
 * Side effects:
 *	Creates handler to get clipboard data.
 *
 *----------------------------------------------------------------------
 */

static int TableClipboardImport(
    ClientData clientData,	/* Information about the table widget. */
    Tcl_Interp *interp) {	/* TCL interpreter. */

    Table *tablePtr = (Table *) clientData;
    int result = TCL_OK;
    TableGetSelection *dataPtr;

    /* target is ICCCM type, format is used for transmit */
    Atom selection = Tk_InternAtom(tablePtr->tkwin, "CLIPBOARD");
#ifndef _WIN32
    Atom target = Tk_InternAtom(tablePtr->tkwin, "UTF8_STRING");
#else
    Atom target = XA_STRING;
#endif

    /* Allocate temp storage for selection data */
    dataPtr = (TableGetSelection *) ckalloc(sizeof(TableGetSelection));
    if (dataPtr == NULL) {
	return TCL_ERROR;
    }

    /* Initialize temp storage */
    dataPtr->tablePtr = tablePtr;
    dataPtr->listPtr = Tcl_NewListObj(0, NULL);
    dataPtr->resultPtr = Tcl_NewListObj(0, NULL);
    dataPtr->escaped = 0;
    dataPtr->quoted = 0;
    dataPtr->length = 0;
    dataPtr->buffer = (char *) ckalloc(MAX_CELL_SIZE);

    if (dataPtr->listPtr == NULL || dataPtr->resultPtr == NULL || dataPtr->buffer == NULL) {
	ckfree(dataPtr);
	return TCL_ERROR;
    }

    /* Create handler to process the clipboard data. Returns when the selection
       has been completely retrieved. */
    result = Tk_GetSelection(interp, tablePtr->tkwin, selection, target, Table_ImportHandler,
	(ClientData) dataPtr);

    /* Handle missing end of data row terminator */
    if (dataPtr->length > 0 && result == TCL_OK) {
	Table_ImportHandler((ClientData) dataPtr, interp, tablePtr->rowSep);
    }

    /* Clean-up */
    if (dataPtr->listPtr != NULL) {
	Tcl_IncrRefCount(dataPtr->listPtr);
	Tcl_DecrRefCount(dataPtr->listPtr);
    }
    if (dataPtr->resultPtr != NULL) {
	if (result == TCL_OK) {
	    Tcl_SetObjResult(interp, dataPtr->resultPtr);
	} else {
	    Tcl_IncrRefCount(dataPtr->resultPtr);
	    Tcl_DecrRefCount(dataPtr->resultPtr);
	}
    }
    ckfree(dataPtr->buffer);
    ckfree(dataPtr);
    return result;
}


/*
 *----------------------------------------------------------------------
 *
 * Table_ClipboardCmd --
 *	This function is called by the user when the selection is to be
 *	exported or imported from the clipboard. Also used by bindings.
 *
 * Results:
 *	Returns TCL result.
 *
 * Side effects:
 *	Function dependent.
 *
 *----------------------------------------------------------------------
 */

static const char *clpbdCmdNames[] = {
    "export", "import", (char *)NULL
};
enum clipboard {
    CLIPBOARD_EXPORT, CLIPBOARD_IMPORT
};

int Table_ClipboardCmd(
    ClientData clientData,	/* Information about the table widget. */
    Tcl_Interp *interp,		/* TCL interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[]) {	/* Argument objects. */

    Table *tablePtr = (Table *) clientData;
    int result = TCL_OK, cmdIndex;

    Tcl_ResetResult(interp);

    if (objc != 3) {
	Tcl_WrongNumArgs(interp, 2, objv, "option");
	return TCL_ERROR;
    }

    /* parse the first parameter */
    if (Tcl_GetIndexFromObj(interp, objv[2], clpbdCmdNames, "option", 0, &cmdIndex) != TCL_OK) {
	return TCL_ERROR;
    }

    switch ((enum bdCmd) cmdIndex) {
	case CLIPBOARD_EXPORT:
	    /* Make currently selected cell values available to clipboard */
	    result = TableClipboardExport(clientData, interp);
	    break;
	case CLIPBOARD_IMPORT:
	    /* Get data from clipboard, parse it, and return as a list of lists */
	    result = TableClipboardImport(clientData, interp);
	    break;
    }
    return result;
}

/*****************************************************************************/

/*
 *----------------------------------------------------------------------
 *
 * TableLostSelection --
 *	This function is called by Tk when the selection is grabbed
 *	away from the table widget.
 *
 * Results:
 *	None
 *
 * Side effects:
 *	The existing selection is cleared, and the window is marked as
 *	not containing a selection.
 *
 *----------------------------------------------------------------------
 */

void TableLostSelection(
     ClientData clientData) {	/* Information about table widget. */

    Table *tablePtr = (Table *) clientData;

    if (tablePtr->exportSelection) {
	Tcl_HashEntry *entryPtr;
	Tcl_HashSearch search;
	int row, col, selchanged = 0;

	/* Same as SEL CLEAR ALL */
	for (entryPtr = Tcl_FirstHashEntry(tablePtr->selCells, &search);
	     entryPtr != NULL; entryPtr = Tcl_NextHashEntry(&search)) {
	    TableParseArrayIndex(&row, &col, Tcl_GetHashKey(tablePtr->selCells,entryPtr));
	    Tcl_DeleteHashEntry(entryPtr);
	    TableRefresh(tablePtr, row-tablePtr->rowOffset, col-tablePtr->colOffset, CELL);
	    selchanged = 1;
	}

	/* Clear cached selection */
	if (tablePtr->haveSelection) {
	    Tcl_DStringFree(&tablePtr->selection);
	    tablePtr->haveSelection = 0;
	}

	/*
	 * On Windows and Mac systems, we want to remember the selection for the
	 * next time the focus enters the window. On Unix, we need to clear the
	 * selection since it is always visible.
	 * This is controlled by ::tk::AlwaysShowSelection.
	 */
	if (tablePtr->selectFirst != -1 && tablePtr->exportSelection
	    && (!Tcl_IsSafe(tablePtr->interp))
#if TCL_MAJOR_VERSION > 8
	    && Tk_AlwaysShowSelection(tablePtr->tkwin)
#endif
	    ) {
	    tablePtr->selectFirst = -1;
	    tablePtr->selectLast = -1;
	    if (tablePtr->activeRow > -1 || tablePtr->activeCol > -1) {
		TableRefresh(tablePtr, tablePtr->activeRow, tablePtr->activeCol, CELL);
	    }
	}

	if (selchanged) {
	    TableGenerateSelectEvent(tablePtr);
	}
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TableFetchSelection --
 *	This function is called by Tk when the PRIMARY selection is
 *	requested. It returns all or part of the selection in buffer.
 *
 * Results:
 *	The return value is the number of non-NULL bytes stored
 *	at buffer.  Buffer is filled (or partially filled) with a
 *	NULL-terminated string containing part or all of the selection,
 *	as given by offset and maxBytes.
 *
 * Side effects:
 *	None
 *
 *----------------------------------------------------------------------
 */

Tcl_Size TableFetchSelection(
     ClientData clientData,	/* Information about table widget. */
     Tcl_Size offset,		/* Offset within selection of first character to be returned. */
     char *buffer,		/* Location in which to place selection. */
     Tcl_Size maxBytes) {	/* Maximum number of bytes to place at buffer, not including
				 * terminating NULL. */

    Table *tablePtr = (Table *) clientData;
    Tcl_Interp *interp = tablePtr->interp;
    char *data, *rowsep = tablePtr->rowSep, *colsep = tablePtr->colSep;
    int lastrow=0, needcs=0, r, c, rslen=0, cslen=0;
    Tcl_Size count, length, index;
    int numcols, numrows;
    Tcl_Obj *listPtr, *objPtr;

    /* if we are not exporting the selection or we have no data source, abort */
    if (!tablePtr->exportSelection || (tablePtr->dataSource == DATA_NONE)) {
	return -1;
    }

    /* First Time through, get the selection, otherwise, just use the
     * selection obtained before */
    if ((offset == 0) || !tablePtr->haveSelection) {
	/* If we have fetched a selection before, free it */
	if (tablePtr->haveSelection) {
	    Tcl_DStringFree(&tablePtr->selection);
	    tablePtr->haveSelection = 0;
	}

	/* First get a sorted list of the selected elements */
	listPtr = TableGetSortedIndices(tablePtr->selCells);
	Tcl_ListObjLength(interp, listPtr, &length);

	/* Abort if no data or conversion to list fails */
	if (listPtr == NULL || length == 0) {
	    return -1;
	}

	/* Add elements to selection */
	Tcl_DStringInit(&tablePtr->selection);
	tablePtr->haveSelection = 1;
	rslen = (rowsep?((int) strlen(rowsep)):0);
	cslen = (colsep?((int) strlen(colsep)):0);
	numrows = numcols = 0;
	for (index = 0; index < length; index++) {
	    Tcl_ListObjIndex(interp, listPtr, index, &objPtr);
	    TableParseArrayIndex(&r, &c, Tcl_GetString(objPtr));
	    if (index) {
		if (lastrow != r) {
		    lastrow = r;
		    needcs = 0;
		    if (rslen) {
			Tcl_DStringAppend(&tablePtr->selection, rowsep, (Tcl_Size)rslen);
		    } else {
			Tcl_DStringEndSublist(&tablePtr->selection);
			Tcl_DStringStartSublist(&tablePtr->selection);
		    }
		    ++numrows;
		} else {
		    if (++needcs > numcols)
			numcols = needcs;
		}
	    } else {
		lastrow = r;
		needcs = 0;
		if (!rslen) {
		    Tcl_DStringStartSublist(&tablePtr->selection);
		}
	    }
	    data = TableGetCellValue(tablePtr, r, c);
	    if (cslen) {
		if (needcs) {
		    Tcl_DStringAppend(&tablePtr->selection, colsep, (Tcl_Size)cslen);
		}
		Tcl_DStringAppend(&tablePtr->selection, data, -1);
	    } else {
		Tcl_DStringAppendElement(&tablePtr->selection, data);
	    }
	}
	if (!rslen && index > 0) {
	    Tcl_DStringEndSublist(&tablePtr->selection);
	}

	if (tablePtr->selCmd != NULL) {
	    int code;
	    Tcl_DString script;
	    Tcl_DStringInit(&script);
	    ExpandPercents(tablePtr, tablePtr->selCmd, numrows+1, numcols+1,
		    Tcl_DStringValue(&tablePtr->selection), (char *)NULL,
		    length, &script, 0);
	    if ((code = Tcl_EvalEx(interp, Tcl_DStringValue(&script), -1, TCL_EVAL_GLOBAL)) == TCL_ERROR) {
		Tcl_AddErrorInfo(interp, "\n    (error in table selection command)");
		Tcl_BackgroundException(interp, code);
		Tcl_DStringFree(&script);
		Tcl_DStringFree(&tablePtr->selection);
		tablePtr->haveSelection = 0;
		return -1;
	    } else {
		Tcl_DStringGetResult(interp, &tablePtr->selection);
	    }
	    Tcl_DStringFree(&script);
	}
    }

    /* Get selection length */
    if (tablePtr->haveSelection) {
	length = Tcl_DStringLength(&tablePtr->selection);
    } else {
	length = 0;
    }

    /* If no selection, abort */
    if (length == 0) {
	if (tablePtr->haveSelection) {
	    Tcl_DStringFree(&tablePtr->selection);
	    tablePtr->haveSelection = 0;
	}
	return -1;
    }

    /* Copy the requested portion of the selection to the buffer. */
    count = length - offset;
    if (count <= 0) {
	count = 0;
    } else {
	if (count > maxBytes) {
	    count = maxBytes;
	}
	memcpy((void *) buffer,
	       (void *) (Tcl_DStringValue(&tablePtr->selection) + offset),
	       (size_t) count);
    }
    buffer[count] = '\0';
    if (count < maxBytes) {
	/*
	 * This should be the last call in this range, so free selection now.
	 */
	Tcl_DStringFree(&tablePtr->selection);
	tablePtr->haveSelection = 0;
    }
    return count;
}

/*
 *----------------------------------------------------------------------
 *
 * TableRegisterSelectionHandlers --
 *	This function is called at init to register the handler.
 *
 * Results:
 *	Returns TCL result.
 *
 * Side effects:
 *	Registers handlers.
 *
 *----------------------------------------------------------------------
 */

void TableRegisterSelectionHandlers(
    ClientData clientData) {	/* Information about the table widget. */

    Table *tablePtr = (Table *) clientData;

    /* target is ICCCM type, format is used for transmit */
    Atom format = XA_STRING;
#ifndef _WIN32
    Atom target = Tk_InternAtom(tablePtr->tkwin, "UTF8_STRING");
#else
    Atom target = XA_STRING;
#endif

    tablePtr->haveSelection = 0;
    Tcl_DStringInit(&tablePtr->selection);

    /* Register handlers */
    Tk_CreateSelHandler(tablePtr->tkwin, XA_PRIMARY, target, TableFetchSelection,
	clientData, format);
}

/*
 *----------------------------------------------------------------------
 *
 * TableDeregisterSelectionHandlers --
 *	This function is called at exit to deregister the handler.
 *
 * Results:
 *	Returns TCL result.
 *
 * Side effects:
 *	Deregisters handlers.
 *
 *----------------------------------------------------------------------
 */

void TableDeregisterSelectionHandlers(
    ClientData clientData) {	/* Information about the table widget. */

    Table *tablePtr = (Table *) clientData;

    /* target is ICCCM type, format is used for transmit */
    if (tablePtr->tkwin != NULL) {
#ifndef _WIN32
	Atom target = Tk_InternAtom(tablePtr->tkwin, "UTF8_STRING");
#else
	Atom target = XA_STRING;
#endif

	/* Deregister handlers */
	Tk_DeleteSelHandler(tablePtr->tkwin, XA_PRIMARY, target);
    }

    /* Free selection */
    if (tablePtr->haveSelection) {
	Tcl_DStringFree(&tablePtr->selection);
	tablePtr->haveSelection = 0;
    }
}

