/* 
 *
 * Copyright (c) 1994 by Roland King
 * All rights reserved.
 * See the file COPYRIGHT for the copyright notes.
 *
*/

#include "Table.h"


void  TableEventProc(ClientData clientdata, XEvent *eventPtr)
{
	Table *tablePtr=(Table *)clientdata;

	switch (eventPtr->type)
	{
	case Expose:

		TableInvalidate(tablePtr, 
				eventPtr->xexpose.x, eventPtr->xexpose.y,
				eventPtr->xexpose.width, eventPtr->xexpose.height,0);
		break;

	case DestroyNotify:
		/* remove the command from the interpreter */
		Tcl_DeleteCommand(tablePtr->interp, Tk_PathName(tablePtr->tkwin));

		tablePtr->tkwin=NULL;
	
		/* cancel any pending update or timer */
		if(tablePtr->tableFlags & TBL_REDRAW_PENDING)
			Tk_CancelIdleCall(TableDisplay, (ClientData)tablePtr);
		if(tablePtr->cursorTimer!=NULL)
			Tk_DeleteTimerHandler(tablePtr->cursorTimer);
		if(tablePtr->flashTimer!=NULL)
			Tk_DeleteTimerHandler(tablePtr->flashTimer);

		/* delete the variable trace */
		if(tablePtr->arrayVar!=NULL)
			Tcl_UntraceVar(	tablePtr->interp, tablePtr->arrayVar, 
				 	TCL_TRACE_WRITES | TCL_TRACE_UNSETS | TCL_GLOBAL_ONLY,
					TableVarProc, (ClientData) tablePtr);

		/* free the widget in time */	
		Tk_EventuallyFree((ClientData) tablePtr, TableDestroy);
		break;
	
	case ConfigureNotify:
		TableAdjustParams(tablePtr);
		TableInvalidate(tablePtr, 0, 0, Tk_Width(tablePtr->tkwin), Tk_Height(tablePtr->tkwin),0);
		break;

	case FocusIn:
		{
			int x, y, width, height;
			tablePtr->tableFlags |= TBL_HAS_FOCUS;
			
			/* Turn the cursor on */
			TableConfigCursor(tablePtr);
			break;
		}

	case FocusOut:
		tablePtr->tableFlags &= ~TBL_HAS_FOCUS;

		/* cancel the timer */
		TableConfigCursor(tablePtr);
		break;
	}
	
}



/* 
** Invalidates a rectangle and adds it to the
** total invalid rectangle waiting to be redrawn 
** if the force flag is set, does an update instantly
** else waits until TK is idle
*/

void 
TableInvalidate(Table *tablePtr, int x, int y, int width, int height, int force)
{

  	int old_x, old_y, old_width, old_height;

	/* If not even mapped, ignore */
	if(!Tk_IsMapped(tablePtr->tkwin))
		return;

	/* is this rectangle even on the screen */
	if((x>Tk_Width(tablePtr->tkwin))||(y>Tk_Height(tablePtr->tkwin)))
		return;

	/* 
	** if no pending updates then 
	** replace the rectangle, otherwise		
	** find the bounding rectangle
	*/
	if(!(tablePtr->tableFlags & TBL_REDRAW_PENDING)) 
	{
		tablePtr->invalidX=x;
		tablePtr->invalidY=y;
		tablePtr->invalidWidth=width;
		tablePtr->invalidHeight=height;
		if(force && !(tablePtr->tableFlags & TBL_BATCH_ON))
			TableDisplay((ClientData)tablePtr);
		else
		{
			tablePtr->tableFlags |= TBL_REDRAW_PENDING;	
			Tk_DoWhenIdle(TableDisplay, (ClientData)tablePtr);	
		}
	}
	else
	{
		tablePtr->invalidWidth		=max(tablePtr->invalidX+tablePtr->invalidWidth, x+width);
		tablePtr->invalidHeight		=max(tablePtr->invalidY+tablePtr->invalidHeight, y+height);
		tablePtr->invalidX		=min(tablePtr->invalidX, x);
		tablePtr->invalidY		=min(tablePtr->invalidY, y);
		tablePtr->invalidWidth		-=tablePtr->invalidX;
		tablePtr->invalidHeight		-=tablePtr->invalidY;
		/* are we forcing this update out */
		if(force && !(tablePtr->tableFlags & TBL_BATCH_ON))
		{
			Tk_CancelIdleCall(TableDisplay, (ClientData)tablePtr);
			TableDisplay((ClientData)tablePtr);
		}			
	}
}



void TableDestroy(ClientData clientdata)
{
	Table *tablePtr=(Table *)clientdata;
	Tcl_HashEntry *entryPtr;
	Tcl_HashSearch search;

	/* delete the variable trace */
	if(tablePtr->arrayVar!=NULL)
		Tcl_UntraceVar(	tablePtr->interp, tablePtr->arrayVar, 
			 	TCL_TRACE_WRITES | TCL_TRACE_UNSETS | TCL_GLOBAL_ONLY,
				TableVarProc, (ClientData) tablePtr);
	
	/* destroy the row height and column width hash tables */
	if(tablePtr->colWidths!=NULL)
	{
		Tcl_DeleteHashTable(tablePtr->colWidths);
		free(tablePtr->colWidths);
	}
	if(tablePtr->rowHeights!=NULL)
	{
		Tcl_DeleteHashTable(tablePtr->rowHeights);
		free(tablePtr->rowHeights);
	}

	/* free the arrays with row/column pixel sizes */
	if(tablePtr->colPixels)
		free(tablePtr->colPixels);
	if(tablePtr->rowPixels)
		free(tablePtr->rowPixels);
	if(tablePtr->colStarts)
		free(tablePtr->colStarts);
	if(tablePtr->rowStarts)
		free(tablePtr->rowStarts);

	/* free the selection buffer */
	if(tablePtr->selectBuf)
		free(tablePtr->selectBuf);

	/* delete the row, column and cell style hash tables */
	if(tablePtr->rowStyles!=NULL)
	{
		Tcl_DeleteHashTable(tablePtr->rowStyles);
		free(tablePtr->rowStyles);
	}
	if(tablePtr->colStyles!=NULL)
	{
		Tcl_DeleteHashTable(tablePtr->colStyles);
		free(tablePtr->colStyles);
	}
	if(tablePtr->cellStyles!=NULL)
	{
		Tcl_DeleteHashTable(tablePtr->cellStyles);
		free(tablePtr->cellStyles);
	}

	if(tablePtr->flashCells!=NULL)
	{
		Tcl_DeleteHashTable(tablePtr->flashCells);
		free(tablePtr->flashCells);
	}

	if (tablePtr->gcCache!=NULL)
	{
		entryPtr=Tcl_FirstHashEntry(tablePtr->gcCache, &search);
		while(entryPtr!=NULL)
		{
			Tk_FreeGC(tablePtr->display, (GC)Tcl_GetHashValue(entryPtr));
			entryPtr=Tcl_NextHashEntry(&search);
		}
		Tcl_DeleteHashTable(tablePtr->gcCache);
		free(tablePtr->gcCache);
	}

	/* Now free up all the tag information */
	if(tablePtr->tagTable!=NULL)
	{
		entryPtr=Tcl_FirstHashEntry(tablePtr->tagTable, &search);		
		for(;entryPtr!=NULL;entryPtr=Tcl_NextHashEntry(&search))
		{
			/* free up the Gcs etc */
			TableCleanupTag(tablePtr, (tagStruct *)Tcl_GetHashValue(entryPtr));
	
			/* free the memory */
			free((tagStruct *)Tcl_GetHashValue(entryPtr));
		}
		/* And delete the actual hash table */
		Tcl_DeleteHashTable(tablePtr->tagTable);
		free(tablePtr->tagTable);
	}

	/* free up the stuff in the default tag */
	TableCleanupTag(tablePtr, &(tablePtr->defaultTag));

	/* free the configuration options in the widget */
	Tk_FreeOptions(TableConfig, (char *)tablePtr, tablePtr->display, 0);

	/* and free the widget memory at last! */
	free(tablePtr);
}


char *
TableVarProc(	ClientData clientdata, Tcl_Interp *interp, 
		char *name, char *index, int flags)
{
	Table *tablePtr=(Table *)clientdata;
	int x, y, width, height, i, j;
	char buf[100];

	/* 
	** is this the whole variable being destroyed 
	** or just one cell being deleted
	*/
	if(flags & TCL_TRACE_UNSETS && index==NULL)
	{
		/* 
		**if this isn't the interpreter being destroyed 
		** reinstate the trace
		*/
		if((flags & TCL_TRACE_DESTROYED) && !( flags & TCL_INTERP_DESTROYED))
		{
			Tcl_SetVar2(interp, tablePtr->arrayVar,  "XXXXX", "", TCL_GLOBAL_ONLY);
			Tcl_UnsetVar2(interp, tablePtr->arrayVar, "XXXXX", TCL_GLOBAL_ONLY);
			Tcl_ResetResult(interp);
			
			/* clear the selection buffer */
			TableGetSelection(tablePtr);

			/* set a trace on the variable */
			Tcl_TraceVar(	interp, tablePtr->arrayVar, 
					TCL_TRACE_WRITES | TCL_TRACE_UNSETS | TCL_GLOBAL_ONLY,
					TableVarProc, (ClientData) tablePtr);
			
			/* and invalidate the table */
			TableInvalidate(tablePtr, 0, 0, Tk_Width(tablePtr->tkwin), Tk_Height(tablePtr->tkwin),0);
		}
		return (char *)NULL;
	}
	
	/* get the cell address and invalidate that region only */
	TableParseArrayIndex(tablePtr, &i, &j, index);

	i-=tablePtr->rowOffset;
	j-=tablePtr->colOffset;

	/* did the selected cell just update */
	if(i==tablePtr->selRow&&j==tablePtr->selCol)
		TableGetSelection(tablePtr);

	/* Flash the cell */
	TableAddFlash(tablePtr, i, j);

	TableCellCoords(tablePtr, i, j, &x, &y, &width, &height);
	TableInvalidate(tablePtr, x, y, width, height,1);
	return (char *)NULL;
}



/*
** Toggle the cursor status 
*/
void	
TableCursorEvent(ClientData clientdata)
{
	Table *tablePtr=(Table *)clientdata;
	int x, y, width, height;

	/* Toggle the cursor */
	tablePtr->tableFlags ^= TBL_CURSOR_ON;
	
	/* Disable any old timers */
	if(tablePtr->cursorTimer!=NULL)
		/* 
		** we ARE allowed to delete a timer that has 
		** gone off.
		*/
		Tk_DeleteTimerHandler(tablePtr->cursorTimer);

	/* set up a new timer */
	tablePtr->cursorTimer=Tk_CreateTimerHandler(500, TableCursorEvent, clientdata);

	/* invalidate the cell */
	TableCellCoords(tablePtr, tablePtr->selRow, tablePtr->selCol, &x, &y, &width, &height);
	TableInvalidate(tablePtr, x, y, width, height,1);
}


/*
** Configures the timer depending on the 
** state of the table.
*/
void
TableConfigCursor(Table *tablePtr)
{
	int mask, x, y, width, height;

	mask=TBL_HAS_FOCUS | TBL_EDIT_ALLOWED;
	
	/* to get a cursor, we have to have focus, selection and edits allowed */
	if(((tablePtr->tableFlags & mask)==mask) && tablePtr->selectionOn)
	{
		/* turn the cursor on */
		tablePtr->tableFlags |= TBL_CURSOR_ON;
	
		/* set up the first timer */
		tablePtr->cursorTimer=Tk_CreateTimerHandler(500, TableCursorEvent, (ClientData)tablePtr);
	}
	else 
	{	
		/* turn the cursor off */
		tablePtr->tableFlags &= ~TBL_CURSOR_ON;
	
		/* and disable the timer */
		if(tablePtr->cursorTimer!=NULL)
			/* we ARE allowed to delete a timer that has gone off */
			Tk_DeleteTimerHandler(tablePtr->cursorTimer);
	}

	/* Invalidate the selection window to show or hide the cursor*/
	TableCellCoords(tablePtr, tablePtr->selRow, tablePtr->selCol,
			&x, &y, &width, &height);
	TableInvalidate(tablePtr, x, y, width, height,0); 
}


/* 
** starts or stops the timer for the flashing cells 
** if stopping, clears out the flash structure
*/
void
TableFlashConfigure(Table *tablePtr, int startStop)
{
	if(startStop && (tablePtr->tableFlags&TBL_FLASH_ENABLED) && tablePtr->flashTag!=NULL)
	{
		if(tablePtr->flashTimer!=NULL)
			Tk_DeleteTimerHandler(tablePtr->flashTimer);
			tablePtr->flashTimer=Tk_CreateTimerHandler(100, TableFlashEvent, (ClientData)tablePtr);
	}
	else
	{
		
		if(tablePtr->flashTimer!=NULL)
			Tk_DeleteTimerHandler(tablePtr->flashTimer);
		tablePtr->flashTimer=(Tk_TimerToken)NULL;

		/* clear out the flash hash table */
		if(tablePtr->flashCells!=NULL)
			Tcl_DeleteHashTable(tablePtr->flashCells);

		/* set up a fresh blank one for the next time */
		Tcl_InitHashTable(tablePtr->flashCells, TCL_STRING_KEYS);
	}
}


/* 
** called when the flash timer goes off
** decrements all the entries in the hash 
** table and invalidates any cells that 
** expire, deleteing them from the table
** if the table is now empty, stops the timer
** else reenables it 
*/
void
TableFlashEvent(ClientData clientdata)
{
	Table *tablePtr=(Table *)clientdata;
	Tcl_HashEntry *entryPtr;
	Tcl_HashSearch search;
	int entries;
	int count;

	entries=0;
	for(	entryPtr=Tcl_FirstHashEntry(tablePtr->flashCells, &search);
		entryPtr!=NULL;
		entryPtr=Tcl_NextHashEntry(&search))
	{
		count=(int)Tcl_GetHashValue(entryPtr);
		if(--count<=0)
		{
			int row, col, x, y, width, height;

			/* get the cell address and invalidate that region only */
			TableParseArrayIndex(tablePtr, &row, &col, Tcl_GetHashKey(tablePtr->flashCells, entryPtr));

			row-=tablePtr->rowOffset;
			col-=tablePtr->colOffset;

			/* delete the entry from the table */
			Tcl_DeleteHashEntry(entryPtr);

			TableCellCoords(tablePtr, row, col, &x, &y, &width, &height);
			TableInvalidate(tablePtr, x, y, width, height,1);

		}
		else
		{
			Tcl_SetHashValue(entryPtr, (ClientData)count);
			entries++;
		}
	}

	/* do I need to restart the timer */
	if(entries)
		tablePtr->flashTimer=Tk_CreateTimerHandler(100, TableFlashEvent, (ClientData)tablePtr);
	else
		tablePtr->flashTimer=(Tk_TimerToken)NULL;
}	


/*
** adds a flash to the list with the default
** timeout. Only adds it if flashing is
** enabled and there is a flash ta defined
*/
void
TableAddFlash(Table *tablePtr, int row, int col)
{
	char buf[100];
	int dummy, x, y, width, height;
	Tcl_HashEntry *entryPtr;

	if(!(tablePtr->tableFlags&TBL_FLASH_ENABLED && tablePtr->flashTag!=NULL))
		return;
	
	/* create the array index */
	TableMakeArrayIndex(	tablePtr, 
				row+tablePtr->rowOffset, col+tablePtr->colOffset, 
				buf);

	/* add the flash to the hash table */
	entryPtr=Tcl_CreateHashEntry(tablePtr->flashCells, buf, &dummy);
	Tcl_SetHashValue(entryPtr, tablePtr->flashTime);

	/* now set the timer if it's not already going and invalidate the area */
	if(tablePtr->flashTimer==NULL)
		tablePtr->flashTimer=Tk_CreateTimerHandler(100, TableFlashEvent, (ClientData)tablePtr);
	
	/*TableCellCoords(tablePtr, row, col,
			&x, &y, &width, &height);
	TableInvalidate(tablePtr, x, y, width, height,); */
}




