/* 
 *
 * Copyright (c) 1994 by Roland King
 * All rights reserved.
 * See the file COPYRIGHT for the copyright notes.
 *
*/

#include "Table.h"


/* 
** The main display procedure
*/

void TableDisplay(ClientData clientdata)
{
	Table *tablePtr		=(Table *)clientdata;
	Tk_Window tkwin		=tablePtr->tkwin;
	Display *display	=tablePtr->display;
	Tcl_Interp *interp	=tablePtr->interp;
	Drawable window;
	int i, j;
	int rowFrom, rowTo, colFrom, colTo;
	int firstBodyRow, firstBodyCol, lastBodyRow, lastBodyCol;
	int invalidX, invalidY, invalidWidth, invalidHeight;
	int x, y, width, height;
	int direction, ascent, descent, bd;
	int originX, originY;
	int selectedCell, clipRectSet;
	int offsetX, offsetY;
	XCharStruct bbox, bbox2;
	XRectangle  clipRect;
	GC topGc, bottomGc, copyGc;
	char *value, buf[150];
	tagStruct *tagPtr;
	tagStruct *titlePtr;
	tagStruct *selPtr;
	Tcl_HashEntry *entryPtr;
	XPoint rect[6]={{0,0},{0,0},{0,0},{0,0},{0,0},{0,0}};

	tablePtr->tableFlags &= ~TBL_REDRAW_PENDING;
	invalidX=tablePtr->invalidX;
	invalidY=tablePtr->invalidY;
	invalidWidth=tablePtr->invalidWidth;
	invalidHeight=tablePtr->invalidHeight;

	/* 
	** if we are using the slow drawing mode with a pixmap 
	** create the pixmap and set up offsetX and Y 
	*/
	if(tablePtr->drawMode==DRAW_MODE_SLOW)
	{
		window=XCreatePixmap(	display, Tk_WindowId(tkwin),
					invalidWidth, invalidHeight, 
					Tk_Depth(tkwin));
		offsetX=invalidX; offsetY=invalidY;
	}
	else
	{
		window=Tk_WindowId(tablePtr->tkwin);
		offsetX=0; offsetY=0;
	}
	
	/* find the tag style for titles */
	entryPtr=Tcl_FindHashEntry(tablePtr->tagTable, "Title");
	titlePtr=(tagStruct *)Tcl_GetHashValue(entryPtr);
			
	/* find the tag type for the selection */
	entryPtr=Tcl_FindHashEntry(tablePtr->tagTable, "sel");
	selPtr=(tagStruct *)Tcl_GetHashValue(entryPtr);


	/* find out the cells represented by the invalid region */
	TableWhatCell(tablePtr, invalidX, invalidY, &rowFrom, &colFrom);
	TableWhatCell(tablePtr, invalidX+invalidWidth-1, invalidY+invalidHeight-1, &rowTo, &colTo);


	/* 
	** Cycle through the cells and display them 
	*/	
	for( i=rowFrom; i<=rowTo; i++)
		for(j=colFrom; j<=colTo ; j++)
		{
			/* 
			** are we in the 'dead zone' between the
			** titles and the first displayed cell 
			*/
			if(i<tablePtr->topRow && i>=tablePtr->titleRows)
				i=tablePtr->topRow;			

			if(j<tablePtr->leftCol && j>=tablePtr->titleCols)
				j=tablePtr->leftCol;	

			/* are we in both row and column titles */
			if(i<tablePtr->titleRows && j<tablePtr->titleCols)
				continue;

			/* put the cell ref into a buffer for the hash lookups */
			TableMakeArrayIndex(tablePtr, i+tablePtr->rowOffset, j+tablePtr->colOffset,buf);

			/* get the tag structure for the cell */

			/* 
			** first clear out a new tag structure 
			** that we will build in 
			*/
			tagPtr=TableNewTag(tablePtr);

			/* If there is a selected cell, is this it */
			if(selectedCell=((tablePtr->selectionOn) && 
					 i==tablePtr->selRow && j==tablePtr->selCol))
				TableMergeTag(tagPtr, selPtr);
			/* if flash mode is on, is this cell flashing */
			if(	(tablePtr->tableFlags&TBL_FLASH_ENABLED)&&
					(entryPtr=Tcl_FindHashEntry(tablePtr->flashCells, buf))!=NULL)
				TableMergeTag(tagPtr, tablePtr->flashTag);
			/* Am I in the titles */
			if(i<tablePtr->topRow || j<tablePtr->leftCol)
				TableMergeTag(tagPtr, titlePtr);
			/* check the individual cell reference */
			if((entryPtr=Tcl_FindHashEntry(tablePtr->cellStyles, buf))!=NULL)
				TableMergeTag(tagPtr, (tagStruct *)Tcl_GetHashValue(entryPtr));
			/* then try the rows */
			if((entryPtr=Tcl_FindHashEntry(tablePtr->rowStyles, (char *)i+tablePtr->rowOffset))!=NULL)
				TableMergeTag(tagPtr, (tagStruct *)Tcl_GetHashValue(entryPtr));
			/* then the columns */
			if((entryPtr=Tcl_FindHashEntry(tablePtr->colStyles, (char *)j+tablePtr->colOffset))!=NULL)
				TableMergeTag(tagPtr, (tagStruct *)Tcl_GetHashValue(entryPtr));
			/* then merge in the default type */
			TableMergeTag(tagPtr, &(tablePtr->defaultTag));

			copyGc=TableGetGc(tablePtr, tagPtr);

			/* get the coordinates for the cell */
			TableCellCoords(tablePtr, i, j, &x, &y, &width, &height);

			/* 
			** first fill in a blank rectangle. This is left
			** as a Tk call instead of a direct X call for Tk
			** compatibilty. The TK_RELIEF_FLAT ensures that only
			** XFillRectangle is called anyway so the speed is the same
			*/
			Tk_Fill3DRectangle(	display, window, tagPtr->bgBorder,
						x-offsetX, y-offsetY, width, height,
						tablePtr->borderWidth, TK_RELIEF_FLAT);   

			/* Is there a value in the cell? If so, draw it  */
				
			if(	(tablePtr->arrayVar!=NULL) && 	/* is there a variable at all */
				/* if selectTaed and editing, always show something */
				((selectedCell && (tablePtr->tableFlags & TBL_EDIT_ALLOWED)) || 
				/* or there really is a value there */
				(value=Tcl_GetVar2(interp, tablePtr->arrayVar, buf, TCL_GLOBAL_ONLY))!=NULL))

			{	
				/* if this is the selected cell and we can edit, use the buffer */
				if(selectedCell && (tablePtr->tableFlags & TBL_EDIT_ALLOWED))
					value=tablePtr->selectBuf;

				/* get the border width to adjust the calculations*/
				bd=tablePtr->borderWidth;

				/* get the dimensions of the string */
				XTextExtents(	tagPtr->fontPtr, value, strlen(value),
						&direction, &ascent, &descent, &bbox);

				/* 
				** Set the origin coordinates of the string to draw
				** using the anchor point.
				** origin represents the (x,y) coordinate of the
				** lower left corner of the text box, relative to
				** the internal (ie inside the border) window
				*/
				
				/* set the X origin first */
				switch (tagPtr->anchor)
				{
				case TK_ANCHOR_NE: case TK_ANCHOR_E: case TK_ANCHOR_SE:
					originX=width-bbox.rbearing-2*bd;
					break;
				case TK_ANCHOR_N: case TK_ANCHOR_S: case TK_ANCHOR_CENTER:
					originX=(width-(bbox.rbearing-bbox.lbearing))/2-bd;
					break;
				default:
					originX=-bbox.lbearing;
				}

				/* then set the Y origin */
				switch (tagPtr->anchor)
				{
				case TK_ANCHOR_N: case TK_ANCHOR_NE: case TK_ANCHOR_NW:
					originY=ascent+descent;
					break;
				case TK_ANCHOR_W: case TK_ANCHOR_E: case TK_ANCHOR_CENTER:
					originY=height-(height-(ascent+descent))/2-bd;
					break;
				default:
					originY=height-2*bd;
				}

				/*
				** if this is the selected cell and we are editing
				** ensure that the cursor will be displayed
				*/
				if(selectedCell && (tablePtr->tableFlags & TBL_EDIT_ALLOWED))
				{
					XTextExtents(	tagPtr->fontPtr, value, 
							min(strlen(value), tablePtr->textCurPosn),
							&direction, &ascent, &descent, &bbox2);
					originX=max(3-bbox2.width, min(originX, width-bbox2.width-2*bd-3));
				}

				/* 
				** only if I am desperate will I use a clip rectangle
				** as it means updating the GC in the server which slows
				** everything down.
				*/
				if( 	clipRectSet=
						(originX+bbox.lbearing<-bd || originX+bbox.rbearing > width - bd ||
						(ascent+descent > height)) )
				{
					/* set the clipping rectangle */
					clipRect.x=x-offsetX; clipRect.y=y-offsetY;
					clipRect.width=width; clipRect.height=height;
					XSetClipRectangles(display, copyGc, 0, 0, &clipRect, 1, Unsorted); 
				}

				XDrawString(	display, window, copyGc, 
						x-offsetX+originX+bd, y-offsetY+originY-descent+bd,
						value, strlen(value));

				/* if this is the selected cell draw the cursor if it's on */
				if(selectedCell && (tablePtr->tableFlags&TBL_CURSOR_ON))
				{
					Tk_Fill3DRectangle(	display, window, tablePtr->cursorBg, 
								x-offsetX+originX+bd+bbox2.width, 
								y-offsetY+originY-descent-ascent+bd,
								2, ascent+descent, 0,TK_RELIEF_FLAT); 
				}
				/* reset the clip mask */
				if(clipRectSet)
					XSetClipMask(display, copyGc, None); 
			}
			/* Draw the 3d border on the pixmap correctly offset */
			switch(tablePtr->drawMode)
			{
			case DRAW_MODE_SLOW:
			case DRAW_MODE_TK_COMPAT:
				Tk_Draw3DRectangle(	display, window, tagPtr->bgBorder,
							x-offsetX, y-offsetY, width, height,
							tablePtr->borderWidth, tagPtr->relief);     
				break;
			case DRAW_MODE_FAST:
				/* 
				** choose the GCs to get the best approximation 
				** to the desired drawing style
				*/
				switch(tagPtr->relief)
				{
				case TK_RELIEF_FLAT:
					topGc=bottomGc=((Border *)(tagPtr->bgBorder))->bgGC;
					break;
				case TK_RELIEF_RAISED:
				case TK_RELIEF_RIDGE:
					topGc=((Border *)(tagPtr->bgBorder))->lightGC;
					bottomGc=((Border *)(tagPtr->bgBorder))->darkGC;
					break;
				default: /* TK_RELIEF_SUNKEN TK_RELIEF_GROOVE */
					bottomGc=((Border *)(tagPtr->bgBorder))->lightGC;
					topGc=((Border *)(tagPtr->bgBorder))->darkGC;
					break;
				}
				/* draw a line with no width */
				rect[0].x=x; rect[0].y=y+height-1; rect[1].y=-height+1; rect[2].x=width-1;
				rect[3].x=x+width-1;rect[3].y=y;rect[4].y=height-1;rect[5].x=-width	+1;
				XDrawLines( display, window, bottomGc, rect+3, 3, CoordModePrevious);  
				XDrawLines( display, window, topGc, rect, 3, CoordModePrevious);  
			}

			/* delete the tag structure */
			free(tagPtr);
		}

	/* clear the top left corner */
	if(tablePtr->titleRows!=0 || tablePtr->titleCols!=0)
 	  	Tk_Fill3DRectangle(	display, window, titlePtr->bgBorder, 
					0-offsetX, 0-offsetY, (tablePtr->colStarts)[tablePtr->titleCols], 
			     		(tablePtr->rowStarts)[tablePtr->titleRows],
			     		(tablePtr->drawMode==DRAW_MODE_FAST)?1:tablePtr->borderWidth, titlePtr->relief);  



	/* 
	** if we have got to the end of the table, 
	** clear the area after the last row/column 
	*/
	TableCellCoords(tablePtr, tablePtr->rows, tablePtr->cols, &x, &y, &width, &height);
	if(x+width<invalidX+invalidWidth-1)
		if(tablePtr->drawMode==DRAW_MODE_SLOW)
			/* we are using a pixmap, so use TK to clear it */
			Tk_Fill3DRectangle( 	display, window, tablePtr->defaultTag.bgBorder, 
						x+width-offsetX, 0, invalidX+invalidWidth-x-width, invalidHeight,
						0, TK_RELIEF_FLAT);
		else	
			XClearArea(display, window, x+width, invalidY, invalidX+invalidWidth-x-width, invalidHeight, False);

	if(y+height<invalidY+invalidHeight-1)
		if(tablePtr->drawMode==DRAW_MODE_SLOW)
			/* we are using a pixmap, so use TK to clear it */
			Tk_Fill3DRectangle( 	display, window, tablePtr->defaultTag.bgBorder, 
						0, y+height-offsetY, invalidWidth, invalidY+invalidHeight-y-height,
						0, TK_RELIEF_FLAT);
		else
			XClearArea(display, window, invalidX, y+height, invalidWidth,  invalidY+invalidHeight-y-height, False);

	/* copy on and delete the pixmap if we are in slow mode */
	if(tablePtr->drawMode==DRAW_MODE_SLOW)
	{
		/* Get a default GC */
		copyGc=TableGetGc(tablePtr, &(tablePtr->defaultTag));
			
		XCopyArea(	display, window, Tk_WindowId(tkwin), 
				copyGc, 0, 0, 
				invalidWidth, invalidHeight, 
				invalidX, invalidY);
		XFreePixmap(Tk_Display(tkwin), window);
	}


}


/* 
** Returns the x, y, width and height for a 
** given cell
*/

void 
TableCellCoords(Table *tablePtr, int row, int col,
		int *x, int *y, int *width, int *height)
{
	int i, j, w=0, h=0;

	row=min(tablePtr->rows-1, max(0, row));
	col=min(tablePtr->cols-1, max(0, col));

	*width=(tablePtr->colPixels)[col];
	*height=(tablePtr->rowPixels)[row];
	*x=	(tablePtr->colStarts)[col] -
		((col<tablePtr->titleCols)? 0: 
		(tablePtr->colStarts)[tablePtr->leftCol]-(tablePtr->colStarts)[tablePtr->titleCols]);
	*y=	(tablePtr->rowStarts)[row] -
		((row<tablePtr->titleRows)? 0:
		(tablePtr->rowStarts)[tablePtr->topRow]-(tablePtr->rowStarts)[tablePtr->titleRows]);
}



/* 
** Given an (x,y) coordinate as displayed
** on the screen, returns the 
** Cell that contains that point
*/
void
TableWhatCell(Table *tablePtr, int x, int y, int *row, int *col)
{
int i;

x=max(0, x); 
y=max(0, y); 

/* 
** Adjust the x coord if not in the column titles 
** to change display coords into internal coords
*/
x+=	(x<(tablePtr->colStarts)[tablePtr->titleCols]) ? 0 : 
	(tablePtr->colStarts)[tablePtr->leftCol]-(tablePtr->colStarts)[tablePtr->titleCols];
y+=	(y<(tablePtr->rowStarts)[tablePtr->titleRows]) ? 0 :
	(tablePtr->rowStarts)[tablePtr->topRow]-(tablePtr->rowStarts)[tablePtr->titleRows];

x=min(x, tablePtr->maxWidth-1);
y=min(y, tablePtr->maxHeight-1);

for(i=1;x>=(tablePtr->colStarts)[i];i++); *col=i-1;
for(i=1;y>=(tablePtr->rowStarts)[i];i++); *row=i-1;
}



/* 
** This routine merges two tags by adding 
** any fields from the addTag that are 
** unset in the base Tag
*/

void
TableMergeTag(tagStruct *baseTag, tagStruct *addTag)
{
	if(baseTag->foreground==NULL)
		baseTag->foreground=addTag->foreground;
	if(baseTag->bgBorder==NULL)
		baseTag->bgBorder=addTag->bgBorder;
	if(baseTag->relief==0)
		baseTag->relief=addTag->relief;
	if(baseTag->fontPtr==NULL)
		baseTag->fontPtr=addTag->fontPtr;
	if(baseTag->anchor==-1)
		baseTag->anchor=addTag->anchor;

}


/* Gets a GC correponding to the tag structure passed */

GC
TableGetGc(Table *tablePtr, tagStruct *tagPtr)
{

	XGCValues gcValues;
	GC newGc;
	tableGcInfo	requiredGc;
	Tcl_HashEntry	*entryPtr;
	ClientData	test;
	int new, i;

	/* 
	** first memset the required Gc to zero so that
	** any padding is irrelevant in the search
	*/
	memset((char *)&requiredGc, (char)0, sizeof(tableGcInfo));

	/* then set up the attributes */
	requiredGc.foreground		=tagPtr->foreground;
	requiredGc.bgBorder		=tagPtr->bgBorder;
	requiredGc.fontPtr		=tagPtr->fontPtr;

	/* Now look and see if we already have this GC */
	entryPtr=Tcl_FindHashEntry(tablePtr->gcCache, (char *)&requiredGc);

	/* if we do, return it */
	if(entryPtr!=NULL)
		return (GC)Tcl_GetHashValue(entryPtr);

	/* else get the graphics context */
	gcValues.foreground		=(requiredGc.foreground)->pixel;
	gcValues.background		=Tk_3DBorderColor(requiredGc.bgBorder)->pixel;
	gcValues.font			=(requiredGc.fontPtr)->fid;
	gcValues.graphics_exposures	= False;

	newGc=Tk_GetGC(tablePtr->tkwin, GCForeground|GCBackground|GCFont|GCGraphicsExposures, 
					&gcValues);

	/* and cache it in the table for later */
	entryPtr=Tcl_CreateHashEntry(tablePtr->gcCache, (char *)&requiredGc, &new);

	Tcl_SetHashValue(entryPtr, (ClientData)newGc);
	return newGc;
}
