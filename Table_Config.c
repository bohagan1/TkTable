/* 
 *
 * Copyright (c) 1994 by Roland King
 * All rights reserved.
 * See the file COPYRIGHT for the copyright notes.
 *
*/

#include "Table.h"

/* 
** The widget configuration table
*/

Tk_ConfigSpec TableConfig[]= 
{
	{ 	TK_CONFIG_BORDER, "-background", "background",
		"Background", "BISQUE1",
		Tk_Offset(Table, defaultTag.bgBorder), 0, (Tk_CustomOption *)NULL
	},
	{ 	TK_CONFIG_SYNONYM, "-bg", "background",
		(char *)NULL, (char *)NULL,
		0, 0, (Tk_CustomOption *)NULL
	},
	{ 	TK_CONFIG_COLOR, "-foreground", "foreground",
		"Foreground", "BLACK",
		Tk_Offset(Table, defaultTag.foreground), 0, (Tk_CustomOption *)NULL
	},
	{ 	TK_CONFIG_SYNONYM, "-fg", "foreground",
		(char *)NULL, (char *)NULL,
		0, 0, (Tk_CustomOption *)NULL
	},
	{ 	TK_CONFIG_BORDER, "-cursorbg", "cursor",
		"Cursor", "Black",
		Tk_Offset(Table, cursorBg), 0, (Tk_CustomOption *)NULL
	},
	{ 	TK_CONFIG_PIXELS, "-borderwidth", "borderWidth",
		"BorderWidth", "1",
		Tk_Offset(Table, borderWidth), 0, (Tk_CustomOption *)NULL
	},
	{ 	TK_CONFIG_INT, "-rows", "rows",
		"Rows", "10",
		Tk_Offset(Table, rows), 0, (Tk_CustomOption *)NULL
	},
	{ 	TK_CONFIG_INT, "-cols", "cols",
		"Cols", "10",
		Tk_Offset(Table, cols), 0, (Tk_CustomOption *)NULL
	},
	{ 	TK_CONFIG_INT, "-width", "width",
		"Width", "10",
		Tk_Offset(Table, defColWidth), 0, (Tk_CustomOption *)NULL
	},
	{ 	TK_CONFIG_INT, "-colorigin", "colorigin",
		"Colorigin", "0",
		Tk_Offset(Table, colOffset), 0, (Tk_CustomOption *)NULL
	},
	{ 	TK_CONFIG_PIXELS, "-maxwidth", "maxwidth",
		"Maxwidth", "1000",
		Tk_Offset(Table, maxReqWidth), 0, (Tk_CustomOption *)NULL
	},
	{ 	TK_CONFIG_PIXELS, "-maxheight", "maxheight",
		"Maxheight", "800",
		Tk_Offset(Table, maxReqHeight), 0, (Tk_CustomOption *)NULL
	},
	{ 	TK_CONFIG_INT, "-roworigin", "roworigin",
		"Roworigin", "0",
		Tk_Offset(Table, rowOffset), 0, (Tk_CustomOption *)NULL
	},
	{ 	TK_CONFIG_INT, "-rowtitle", "rowtitle",
		"Rowtitle", 0,
		Tk_Offset(Table, titleRows), TK_CONFIG_NULL_OK, (Tk_CustomOption *)NULL
	},
	{ 	TK_CONFIG_INT, "-coltitle", "coltitle",
	   	"Coltitle", 0,
		   Tk_Offset(Table, titleCols), TK_CONFIG_NULL_OK, (Tk_CustomOption *)NULL
	},
	{ 	TK_CONFIG_INT, "-height", "height",
		"Height", 0,
		Tk_Offset(Table, defRowHeight), TK_CONFIG_NULL_OK, (Tk_CustomOption *)NULL
	},
	{ 	TK_CONFIG_STRING, "-variable", "variable",
		"Variable", NULL,
		Tk_Offset(Table, arrayVar), TK_CONFIG_NULL_OK, (Tk_CustomOption *)NULL
	},
	{ 	TK_CONFIG_RELIEF, "-relief", "relief",
		"Relief", "sunken",
		Tk_Offset(Table, defaultTag.relief), 0, (Tk_CustomOption *)NULL
	},
	{	TK_CONFIG_FONT, "-font", "font", 
		"Font",  "-Adobe-Helvetica-Bold-R-Normal--*-120-*" ,
		Tk_Offset(Table, defaultTag.fontPtr), 0, (Tk_CustomOption *)NULL
	},
	{	TK_CONFIG_ANCHOR, "-anchor", "anchor", 
		"Anchor",  "center" ,
		Tk_Offset(Table, defaultTag.anchor), 0, (Tk_CustomOption *)NULL
	},	
	{	TK_CONFIG_STRING, "-yscrollcmd", "yscrollcmd", 
		"Yscrollcmd",  NULL ,
		Tk_Offset(Table, yScrollCmd), TK_CONFIG_NULL_OK, (Tk_CustomOption *)NULL
	},
	{	TK_CONFIG_STRING, "-xscrollcmd", "xscrollcmd", 
		"Xscrollcmd",  NULL ,
		Tk_Offset(Table, xScrollCmd), TK_CONFIG_NULL_OK, (Tk_CustomOption *)NULL
	},
	{	TK_CONFIG_BOOLEAN, "-rowfirstmode", "rowfirstmode", 
		"Rowfirstmode",  "1" ,
		Tk_Offset(Table, rowThenCol), 0, (Tk_CustomOption *)NULL
	},
	{	TK_CONFIG_BOOLEAN, "-selection", "selection", 
		"Selection", "1", 
		Tk_Offset(Table, selectionOn), 0, (Tk_CustomOption *)NULL
	},
	{	TK_CONFIG_END, (char *)NULL, (char *)NULL, (char *)NULL,
		(char *)NULL, 0, 0
	}
};


/* 
** The default specification for configuring tags
** Done like this to make the command line parsing 
** easy 
*/
Tk_ConfigSpec tagConfig[] =
{
	{ 	TK_CONFIG_BORDER, "-background", "background",
		"Background", NULL,
		Tk_Offset(tagStruct, bgBorder), TK_CONFIG_NULL_OK, (Tk_CustomOption *)NULL
	},
	{ 	TK_CONFIG_SYNONYM, "-bg", "background",
		(char *)NULL, (char *)NULL,
		0, 0, (Tk_CustomOption *)NULL
	},
	{ 	TK_CONFIG_COLOR, "-foreground", "foreground",
		"Foreground", NULL,
		Tk_Offset(tagStruct, foreground), TK_CONFIG_NULL_OK, (Tk_CustomOption *)NULL
	},
	{ 	TK_CONFIG_SYNONYM, "-fg", "foreground",
		(char *)NULL, (char *)NULL,
		0, 0, (Tk_CustomOption *)NULL
	},
	{	TK_CONFIG_FONT, "-font", "font", 
		"Font",  NULL ,
		Tk_Offset(tagStruct, fontPtr), TK_CONFIG_NULL_OK, (Tk_CustomOption *)NULL
	},
	{	TK_CONFIG_ANCHOR, "-anchor", "anchor", 
		"Anchor",  NULL ,
		Tk_Offset(tagStruct, anchor), 0, (Tk_CustomOption *)NULL
	},	
	{ 	TK_CONFIG_RELIEF, "-relief", "relief",
		"Relief", NULL,
		Tk_Offset(tagStruct, relief), TK_CONFIG_NULL_OK, (Tk_CustomOption *)NULL
	},
	{	TK_CONFIG_END, (char *)NULL, (char *)NULL, (char *)NULL,
		(char *)NULL, 0, 0
	}
};


int TableConfigure(	Tcl_Interp *interp, Table *tablePtr,
		   	int argc, char *argv[], int flags)
{
	XGCValues	gcValues;
	GC		newGC;
	char 		buf[200];
	Tcl_HashEntry	*hashEntry;
	Tcl_HashSearch	search;
	Tcl_HashEntry *entryPtr;
	tagStruct *tagPtr;
	int dummy, x, y;

	/* 
	** remove the trace on the old array 
	** variable if there was one
	*/
	if(tablePtr->arrayVar!=NULL)
		Tcl_UntraceVar(	interp, tablePtr->arrayVar, 
			 	TCL_TRACE_WRITES | TCL_TRACE_UNSETS | TCL_GLOBAL_ONLY,
				TableVarProc, (ClientData) tablePtr);

	/* Do the configuration */
	if (Tk_ConfigureWidget(	interp, tablePtr->tkwin, TableConfig, 
				argc, argv, (char *)tablePtr, flags) != TCL_OK)
		return TCL_ERROR;

	/* 
	** Check the variable exists and
	** trace it if it does
	*/
	if(tablePtr->arrayVar!=NULL)
	{
		/* Create a command for checking the array variable */
		sprintf(buf, "array size %s\n", tablePtr->arrayVar);	

		/* does the variable exist? */
		if(Tcl_Eval(interp, buf)!=TCL_OK)
		{
			/* if not, create it but leave it blank */
			Tcl_SetVar2(interp, tablePtr->arrayVar,  "XXXXX", "", TCL_GLOBAL_ONLY);
			Tcl_UnsetVar2(interp, tablePtr->arrayVar, "XXXXX", TCL_GLOBAL_ONLY);
		}

		/* remove the effect of the evaluation */	
		Tcl_ResetResult(interp);

		/* set a trace on the variable */
		Tcl_TraceVar(	interp, tablePtr->arrayVar, 
				TCL_TRACE_WRITES | TCL_TRACE_UNSETS | TCL_GLOBAL_ONLY,
				TableVarProc, (ClientData) tablePtr);
	
		/*  get the current value of the selection */
		if(tablePtr->selectionOn)
			TableGetSelection(tablePtr);
	}

	/*
	** set up the two hash tables for row height and
	** column width if not already set
	*/
	if (tablePtr->colWidths==NULL)
	{
		tablePtr->colWidths=(Tcl_HashTable *)malloc(sizeof(Tcl_HashTable));
		Tcl_InitHashTable(tablePtr->colWidths,TCL_ONE_WORD_KEYS);
	}
	if (tablePtr->rowHeights==NULL)
	{
		tablePtr->rowHeights=(Tcl_HashTable *)malloc(sizeof(Tcl_HashTable));
		Tcl_InitHashTable(tablePtr->rowHeights, TCL_ONE_WORD_KEYS);
	}

	/*
	** Make the Graphics context cache if it
	** doesnt already exist. If it does exist,
	** release all the GCs in it 
	*/
	if (tablePtr->gcCache==NULL)
	{
		tablePtr->gcCache=(Tcl_HashTable *)malloc(sizeof(Tcl_HashTable));
		Tcl_InitHashTable(tablePtr->gcCache, sizeof(tableGcInfo)/sizeof(int));
	}
	else
	{
		entryPtr=Tcl_FirstHashEntry(tablePtr->gcCache, &search);
		while(entryPtr!=NULL)
		{
			Tk_FreeGC(tablePtr->display, (GC)Tcl_GetHashValue(entryPtr));
			Tcl_DeleteHashEntry(entryPtr);
			entryPtr=Tcl_NextHashEntry(&search);
		}
	}

	/* 
	** set up the hash tables for style tags 
	** if it doesn't exist and put the Title
	** tag into it
	*/
	if((tablePtr->tagTable)==NULL)
	{
		char *titleArgs[]={"-bg", "BISQUE2"};
		char *selArgs[]={"-bg", "BISQUE3"};

		tablePtr->tagTable=(Tcl_HashTable *)malloc(sizeof(Tcl_HashTable));
		Tcl_InitHashTable(tablePtr->tagTable, TCL_STRING_KEYS);
		
		/* Insert the first entry for Titles */
		tagPtr=TableNewTag(tablePtr);


		/* Do a configuration */
		Tk_ConfigureWidget(	interp, tablePtr->tkwin, tagConfig, 
					2, titleArgs, (char *)tagPtr, 0); 
		
		entryPtr=Tcl_CreateHashEntry(tablePtr->tagTable, "Title", &dummy);
		Tcl_SetHashValue(entryPtr, (ClientData)tagPtr);

		/* and an entry for the selected cell */
		tagPtr=TableNewTag(tablePtr);

		/* Do a configuration */
		Tk_ConfigureWidget(	interp, tablePtr->tkwin, tagConfig, 
					2, selArgs, (char *)tagPtr, 0); 
		
		entryPtr=Tcl_CreateHashEntry(tablePtr->tagTable, "sel", &dummy);
		Tcl_SetHashValue(entryPtr, (ClientData)tagPtr);

		/*
		** now configure the hash table for keeping the
		** per row, per column, per cell and flashinfo
		*/
		tablePtr->rowStyles=(Tcl_HashTable *)malloc(sizeof(Tcl_HashTable));
		Tcl_InitHashTable(tablePtr->rowStyles, TCL_ONE_WORD_KEYS);

		tablePtr->colStyles=(Tcl_HashTable *)malloc(sizeof(Tcl_HashTable));
		Tcl_InitHashTable(tablePtr->colStyles, TCL_ONE_WORD_KEYS);

		tablePtr->cellStyles=(Tcl_HashTable *)malloc(sizeof(Tcl_HashTable));
		Tcl_InitHashTable(tablePtr->cellStyles, TCL_STRING_KEYS);
	
		tablePtr->flashCells=(Tcl_HashTable *)malloc(sizeof(Tcl_HashTable));
		Tcl_InitHashTable(tablePtr->flashCells, TCL_STRING_KEYS);
	}

	/* set up the default column width and row height */	
	tablePtr->charWidth=XTextWidth(tablePtr->defaultTag.fontPtr, "0", 1);
	if(tablePtr->defRowHeight==0)
		tablePtr->defRowHeight=(tablePtr->defaultTag.fontPtr)->ascent+(tablePtr->defaultTag.fontPtr)->descent+2;


	/* 
	** Calculate the row and column starts 
	** Adjust the top left corner of the internal display 
	*/
	TableAdjustParams(tablePtr);

	/* reset the cursor */
	TableConfigCursor((ClientData)tablePtr);

	/* set up the background colour in the window*/
	Tk_SetBackgroundFromBorder(tablePtr->tkwin, tablePtr->defaultTag.bgBorder);
	
	/* set the geometry and border */
	x=min(tablePtr->maxWidth,  max( Tk_Width(tablePtr->tkwin), tablePtr->maxReqWidth));
	y=min(tablePtr->maxHeight, max(Tk_Height(tablePtr->tkwin), tablePtr->maxReqHeight));
	Tk_GeometryRequest(tablePtr->tkwin, x, y);
	Tk_SetInternalBorder(tablePtr->tkwin, tablePtr->borderWidth);

	/* invalidate the whole table */
	TableInvalidate(tablePtr, 0, 0, Tk_Width(tablePtr->tkwin), Tk_Height(tablePtr->tkwin),0);

	return TCL_OK;
}



/* 
** Calculate the row and column starts.
** Adjusts the topleft corner variable to keep
** it within the screen range, out of the titles
** and keep the screen full 
** make sure the selected cell is in the visible area
** checks to see if the top left cell has changed at all
** and invalidates the table if it has
*/

void
TableAdjustParams(Table *tablePtr)
{
	int minX, maxX, minY, maxY,i,j;
	int topLeftCol, topLeftRow, botRightCol, botRightRow;
	int colPixelOffset, rowPixelOffset;
	int topRow, leftCol, total;
	int x, y, width, height;
	int diff, colPad, unpresetCols, lastUnpresetCol, lastColPad, setColPixels;
	int rowPad, unpresetRows, lastUnpresetRow, lastRowPad, setRowPixels;
	Tcl_HashEntry *entryPtr;
	int bd;


	/* make sure the top row and col have  reasonable values */
	tablePtr->topRow=topRow= max(tablePtr->titleRows, min(tablePtr->topRow, tablePtr->rows-1));
	tablePtr->leftCol=leftCol=max(tablePtr->titleCols, min(tablePtr->leftCol, tablePtr->cols-1));

	/* make sure the selected cell has a reasonable value */
	tablePtr->selRow=max(tablePtr->titleRows, min(tablePtr->selRow, tablePtr->rows-1));
	tablePtr->selCol=max(tablePtr->titleCols, min(tablePtr->selCol, tablePtr->cols-1));

	/* Set up the arrays to hold the row pixels and row starts */
	if(tablePtr->rowPixels)
		free(tablePtr->rowPixels);
	tablePtr->rowPixels=(int *)calloc(tablePtr->rows, sizeof(int));

	if(tablePtr->rowStarts)
		free(tablePtr->rowStarts);
	tablePtr->rowStarts=(int *)calloc(tablePtr->rows+1, sizeof(int));

	/* Set up the arrays to hold the row pixels and row starts */
	if(tablePtr->colPixels)
		free(tablePtr->colPixels);
	tablePtr->colPixels=(int *)calloc(tablePtr->cols, sizeof(int));

	if(tablePtr->colStarts)
		free(tablePtr->colStarts);
	tablePtr->colStarts=(int *)calloc(tablePtr->cols+1, sizeof(int));

	/* get the borderwidth for several upcoming calculations */
	bd=tablePtr->borderWidth;

	/* 
	** get all the preset columns and set their widths
	*/
	setColPixels=0;
	unpresetCols=0;
	total=0;
	for(i=0;i<tablePtr->cols;i++)
	{
		if((entryPtr=Tcl_FindHashEntry(tablePtr->colWidths, (char*)i))==NULL)
		{
			(tablePtr->colPixels)[i]=-1;
			unpresetCols++;
			lastUnpresetCol=i;
		}
		else
		{
			(tablePtr->colPixels)[i]=(int)Tcl_GetHashValue(entryPtr)*(tablePtr->charWidth)+2*bd;
			setColPixels+=(tablePtr->colPixels)[i];
		}
	}

	/* 
	** get all the preset rows and set their heights
	*/
	setRowPixels=0;
	unpresetRows=0;
	for(i=0;i<tablePtr->rows;i++)
	{
		if((entryPtr=Tcl_FindHashEntry(tablePtr->rowHeights, (char*)i))==NULL)
		{
			(tablePtr->rowPixels)[i]=-1;
			unpresetRows++;
			lastUnpresetRow=i;
		}
		else
		{
			(tablePtr->rowPixels)[i]=(int)Tcl_GetHashValue(entryPtr)+2*bd;
			setRowPixels+=(tablePtr->rowPixels)[i];
		}
	}

	/* 
	** work out how much to pad each column and how much to
	** pad the last column, depending on the mode 
	*/
	if(	/*!Tk_IsMapped(tablePtr->tkwin) || */
		(tablePtr->colStretchMode==STRETCH_MODE_NONE) ||
		(tablePtr->colStretchMode==STRETCH_MODE_UNSET && unpresetCols==0) ||
		(tablePtr->cols==0))
	{
		colPad=0; lastColPad=0; 
	}
	else if(tablePtr->colStretchMode==STRETCH_MODE_UNSET)
	{
		diff=	max(0,(Tk_Width(tablePtr->tkwin)-setColPixels)-
			unpresetCols*((tablePtr->charWidth)*(tablePtr->defColWidth)+2*bd));
		colPad=diff/unpresetCols;
		lastColPad=diff-colPad*(unpresetCols-1);
	}
	else /* STRETCH_MODE_ALL */
	{
		diff=	max(0,(Tk_Width(tablePtr->tkwin)-setColPixels)-
			unpresetCols*((tablePtr->charWidth)*(tablePtr->defColWidth)+2*bd));
		colPad=diff/(tablePtr->cols);
		lastColPad=diff-colPad*(tablePtr->cols-1);

		/* force it to be applied to the last column too */
		lastUnpresetCol=tablePtr->cols-1;
	}
	
	/* now do the padding and calculate the column starts */
	total=0;
	for(i=0;i<tablePtr->cols;i++)
	{
		if((tablePtr->colPixels)[i]==-1)
			(tablePtr->colPixels)[i]=	(tablePtr->charWidth)*(tablePtr->defColWidth)+
							2*bd+((i!=lastUnpresetCol)?colPad:lastColPad);
		else if(tablePtr->colStretchMode==STRETCH_MODE_ALL)
			(tablePtr->colPixels)[i]+=	(i!=lastUnpresetCol)?colPad:lastColPad;

		total=(((tablePtr->colStarts)[i]=total)+(tablePtr->colPixels)[i]);
	}
	(tablePtr->colStarts)[i]=tablePtr->maxWidth= total;


	/* 
	** work out how much to pad each row and how much to
	** pad the last row, depending on the mode 
	*/
	if(	/*!Tk_IsMapped(tablePtr->tkwin) || */
	   	(tablePtr->rowStretchMode==STRETCH_MODE_NONE) ||
		(tablePtr->rowStretchMode==STRETCH_MODE_UNSET && unpresetRows==0) ||
		(tablePtr->rows==0))
	{
		rowPad=0; lastRowPad=0; 
	}
	else if(tablePtr->rowStretchMode==STRETCH_MODE_UNSET)
	{
		diff=	max(0, (Tk_Height(tablePtr->tkwin)-setRowPixels)-
			unpresetRows*(tablePtr->defRowHeight+2*bd));
		rowPad=diff/unpresetRows;
		lastRowPad=diff-rowPad*(unpresetRows-1);
	}
	else /* STRETCH_MODE_ALL */
	{
		diff=	max(0, (Tk_Height(tablePtr->tkwin)-setRowPixels)-
			unpresetRows*(tablePtr->defRowHeight+2*bd));
		rowPad=diff/(tablePtr->rows);
		lastRowPad=diff-rowPad*(tablePtr->rows-1);

		/* force it to be applied to the last column too */
		lastUnpresetRow=tablePtr->rows-1;
	}
	
	/* now do the padding and calculate the row starts */
	total=0;
	for(i=0;i<tablePtr->rows;i++)
	{
		if((tablePtr->rowPixels)[i]==-1)
			(tablePtr->rowPixels)[i]=	(tablePtr->defRowHeight)+
							2*bd+((i!=lastUnpresetRow)?rowPad:lastRowPad);
		else if(tablePtr->colStretchMode==STRETCH_MODE_ALL)
			(tablePtr->colPixels)[i]+=	(i!=lastUnpresetRow)?rowPad:lastRowPad;

		/* calculate the start of each row */
		total=(((tablePtr->rowStarts)[i]=total)+(tablePtr->rowPixels)[i]);
	}
	(tablePtr->rowStarts)[i]=tablePtr->maxHeight= total;

	/* If we dont have the info, dont bother to fix up the other parameters */
	if(Tk_WindowId(tablePtr->tkwin)==None)
	{
		tablePtr->oldTopRow=tablePtr->oldLeftCol=-1;
		return;
	}

	/* make sure the top row and col have  reasonable values */
	tablePtr->topRow=topRow= max(tablePtr->titleRows, min(tablePtr->topRow, tablePtr->rows-1));
	tablePtr->leftCol=leftCol=max(tablePtr->titleCols, min(tablePtr->leftCol, tablePtr->cols-1));

	/* make sure the selected cell has a reasonable value */
	tablePtr->selRow=max(tablePtr->titleRows, min(tablePtr->selRow, tablePtr->rows-1));
	tablePtr->selCol=max(tablePtr->titleCols, min(tablePtr->selCol, tablePtr->cols-1));




	/* 
	** constrain the top row and column to make the selected
	** cell lie on the visible portion of the table if selection
	** is enabled
	*/
	if(tablePtr->selectionOn)
	{
		/* where is the new selected cell */
		TableCellCoords(tablePtr, tablePtr->selRow, tablePtr->selCol, &x, &y, &width, &height);
		
		if(x<(tablePtr->colStarts)[tablePtr->titleCols])
		{
			leftCol= tablePtr->selCol;
		}
		else if((x+width)>Tk_Width(tablePtr->tkwin))
		{
			int col;
			for(col=tablePtr->titleCols; col<tablePtr->selCol; col++)
			{	
				if( (tablePtr->colStarts)[tablePtr->titleCols]+
				((tablePtr->colStarts)[tablePtr->selCol+1]-(tablePtr->colStarts)[col])<=Tk_Width(tablePtr->tkwin))
					break; 
			}
			leftCol=col;
		}

		if(y<(tablePtr->rowStarts)[tablePtr->titleRows])
			topRow=tablePtr->selRow;
		else if((y+height)>Tk_Height(tablePtr->tkwin))
		{
			int row;
			for(row=tablePtr->titleRows; row<tablePtr->selRow; row++)
			{
				if( (tablePtr->rowStarts)[tablePtr->titleRows]+
				((tablePtr->rowStarts)[tablePtr->selRow+1]-(tablePtr->rowStarts)[row])<=Tk_Height(tablePtr->tkwin))
					break; 
			}
			topRow=row;
		}
	}

	/* 
	** If we use this value of topRow, will we fill the window 
	** if not, decrease it until we will, or until it gets to titleRows 
	** make sure we dont cut off the bottom row
	*/
	for(; topRow>tablePtr->titleRows; topRow--)
		if(	(tablePtr->maxHeight -
			((tablePtr->rowStarts)[topRow-1]-(tablePtr->rowStarts)[tablePtr->titleRows])) >
			Tk_Height(tablePtr->tkwin))
			break;

	/* 
	** If we use this value of topCol, will we fill the window 
	** if not, decrease it until we will, or until it gets to titleCols 
	** make sure we dont cut off the left column
	*/
	for(; leftCol>tablePtr->titleCols; leftCol--)
		if(	(tablePtr->maxWidth - 
			((tablePtr->colStarts)[leftCol-1]-(tablePtr->colStarts)[tablePtr->titleCols])) >
			Tk_Width(tablePtr->tkwin))
			break;

	tablePtr->topRow=topRow;
	tablePtr->leftCol=leftCol;

	/* 
	** Do we have scrollbars, if so, calculate and call the TCL functions
	** In order to get the scrollbar to be completely
	** full when the whole screen is shown and there
	** are titles, we have to arrange for the scrollbar
	** range to be 0 -> rows-titleRows etc. This leads
	** to the position setting methods, toprow and leftcol,
	** being relative to the titles, not absolute row and
	** column numbers.
	*/
	if(tablePtr->yScrollCmd!=NULL || tablePtr->xScrollCmd!=NULL)
	{
		/* 
		** Now work out where the bottom right  
		** is so we can update the scrollbars
		*/
		TableWhatCell(	tablePtr, 
				Tk_Width(tablePtr->tkwin)-1, Tk_Height(tablePtr->tkwin)-1, 
				&botRightRow, &botRightCol);

		/* Do we have a Y-scrollbar */
		if(tablePtr->yScrollCmd!=NULL)
		{
			char buffer[250];
			sprintf(buffer, " %d %d %d %d", tablePtr->rows-tablePtr->titleRows, max(1,botRightRow-topRow+1), 
				topRow-tablePtr->titleRows, max(topRow-tablePtr->titleRows+1, botRightRow-tablePtr->titleRows));
			if(Tcl_VarEval(	tablePtr->interp, 
					tablePtr->yScrollCmd, buffer, (char *)NULL)!=TCL_OK)
			{
				Tcl_AddErrorInfo(tablePtr->interp, 
						"\n(error executing yscroll command in table widget)");
				Tk_BackgroundError(tablePtr->interp);
			}
		}

		/* Do we have a X-scrollbar */
		if(tablePtr->xScrollCmd!=NULL)
		{
			char buffer[250];
			sprintf(buffer, " %d %d %d %d", tablePtr->cols-tablePtr->titleCols, max(1,botRightCol-leftCol+1), 
				leftCol-tablePtr->titleCols, max(leftCol-tablePtr->titleCols+1, botRightCol-tablePtr->titleCols));
			if(Tcl_VarEval(	tablePtr->interp, 
					tablePtr->xScrollCmd, buffer, (char *)NULL)!=TCL_OK)
			{
				Tcl_AddErrorInfo(tablePtr->interp, 
						"\n(error executing xscroll command in table widget)");
				Tk_BackgroundError(tablePtr->interp);
			}
		}
	}

	/* 
	** now check the new value of topRow and 
	** top column against the originals, did they
	** even change. If they did, invalidate the
	** area, if not, leave it alone
	*/

	/* did the selected cell move then */
	if(	(tablePtr->selectionOn) && 
	 	(tablePtr->oldSelRow!=tablePtr->selRow || tablePtr->oldSelCol!=tablePtr->selCol))
	{
		int x, y, width, height; 
		char buf[100];

		/* put the value back in the cell */
		if(tablePtr->arrayVar!=NULL && (tablePtr->tableFlags & TBL_EDIT_ALLOWED))
		{
			char *tempvar;

			/* 
			** if the value has not changed, do nothing
			** else we will set off the 'flash' timer
			*/
			
			TableMakeArrayIndex(	tablePtr, 
						tablePtr->oldSelRow+tablePtr->rowOffset, tablePtr->oldSelCol+tablePtr->colOffset, 
						buf);
			tempvar=Tcl_GetVar2(tablePtr->interp, tablePtr->arrayVar, buf, TCL_GLOBAL_ONLY);
			/* 
			** if they are different, replace the value 
			** the first check is: if the old value was null but the new
			** buffer is not empty, replace, otherwise leave the value
			** uninitialized
			*/
			if(	(!tempvar && *(tablePtr->selectBuf)!='\0') ||
				( tempvar  && strcmp(tempvar, tablePtr->selectBuf)))
				Tcl_SetVar2(tablePtr->interp, tablePtr->arrayVar, buf, tablePtr->selectBuf, TCL_GLOBAL_ONLY);
		}
	
		/* get the selection */
		TableGetSelection(tablePtr);

		/* Invalidate the cells */
		TableCellCoords(tablePtr, tablePtr->oldSelRow, tablePtr->oldSelCol,
				&x, &y, &width, &height);
		TableInvalidate(tablePtr, x, y, width, height,1);
		TableCellCoords(tablePtr, tablePtr->selRow, tablePtr->selCol,
				&x, &y, &width, &height);
		TableInvalidate(tablePtr, x, y, width, height,1);


	}

	/* how about the rest of the table */
 	if(tablePtr->topRow!=tablePtr->oldTopRow || tablePtr->leftCol!=tablePtr->oldLeftCol)
	{
		TableInvalidate(tablePtr, 0, 0, Tk_Width(tablePtr->tkwin), Tk_Height(tablePtr->tkwin),0);
	}

	/* 
	** set the old top row and column for the next time this	
	** function is called
	*/
	tablePtr->oldTopRow=tablePtr->topRow; tablePtr->oldLeftCol=tablePtr->leftCol;
	tablePtr->oldSelRow=tablePtr->selRow; tablePtr->oldSelCol=tablePtr->selCol;
}


/*
** mallocs space for a new tag structure 
** and clears the structure returns 
** the pointer to the new structure
*/
tagStruct * 
TableNewTag( Table *tablePtr )
{
	tagStruct *tagPtr;

	tagPtr=(tagStruct *)malloc(sizeof(tagStruct));
	tagPtr->bgBorder=NULL;
	tagPtr->foreground=NULL;
	tagPtr->relief=0;
	tagPtr->fontPtr=NULL;
	tagPtr->anchor=-1;
	
	return tagPtr;
}



/* 
** this releases the resources used by a tag
** before it is freed up.
*/
void
TableCleanupTag(Table *tablePtr, tagStruct *tagPtr)
{
	/* free the options in the widget */
	Tk_FreeOptions(tagConfig, (char *)tagPtr, tablePtr->display, 0);
}



