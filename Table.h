/* 
 *
 * Copyright (c) 1994 by Roland King
 * All rights reserved.
 * See the file COPYRIGHT for the copyright notes.
 *
*/


/*
**	Table Widget
**
*/

#ifndef _TABLE_INCL
	#include <tk.h>
	#define _TABLE_INCL
	#include "malloc.h"
#endif

#define max(A,B) ((A)>(B))?(A):(B)
#define min(A,B) ((A)>(B))?(B):(A)

#define TBL_REDRAW_PENDING	1	/* redraw event already pending */
#define TBL_CURSOR_ON		2	/* The edit cursor is on */
#define	TBL_HAS_FOCUS		4	/* the window has the focus */
#define TBL_TEXT_CHANGED	8	/* the text has been edited */
#define TBL_EDIT_ALLOWED	16	/* are edits allowed */
#define	TBL_1_KEY_DESTROY	32	/* Does the first key in an edit delete the whole string */
#define TBL_FLASH_ENABLED	64	/* is flashing enabled */
#define	TBL_BATCH_ON		128	/* no updates are forced */

#define	DRAW_MODE_SLOW		1	/* The display redraws with a pixmap using TK function calls */
#define	DRAW_MODE_TK_COMPAT	2	/* The redisplay is direct to the screen, but TK function calls
					** are still used to give correct 3-d border appearance and
					** thus remain compatible with other TK apps */
#define DRAW_MODE_FAST		3	/* the redisplay goes straight to the screen and the
					** 3d borders are rendered with a single pixel wide line
					** only. It cheats and uses the internal border structure
					** to do the borders */

#define	STRETCH_MODE_NONE	1	/* No additional pixels will be added to rows or cols */
#define	STRETCH_MODE_UNSET	2	/* All default rows or columns will be stretched to fill the screen */
#define STRETCH_MODE_ALL	3	/* All rows/columns will be padded to fill the window */

/* ripped  out of the tk3d.c code */
typedef struct {
    Display *display;		
    int refCount;		
    XColor *bgColorPtr;		
    XColor *lightColorPtr;	
    XColor *darkColorPtr;	
    Pixmap shadow;		
    GC lightGC;			
    GC darkGC;			
    GC bgGC;			
    Tcl_HashEntry *hashPtr;	
} Border;


/* The tag structure */
typedef struct {
	Tk_3DBorder	bgBorder;
	XColor		*foreground;	/* foreground colour */
	int		relief;		/* relief type */
	XFontStruct	*fontPtr;	/* default font pointer */
	Tk_Anchor	anchor;		/* default anchor point */
} tagStruct ;

/* structure for the GC cache */
typedef struct {
	Tk_3DBorder	bgBorder;
	XColor		*foreground;
	XFontStruct	*fontPtr;
	int		pad;		/* to make the structure long */
} tableGcInfo ;

/* 
** The widget structure for the table Widget
*/
typedef struct {
	/* 
	** basic information about the 
	** window and the interpreter 
	*/
	Tk_Window 	tkwin;	
	Display 	*display;
	Tcl_Interp	*interp;
	
	/* Configurable things */
	int		rows, cols;	/* number of rows and columns */
	int		defRowHeight;	/* default row height in pixels */
	int		defColWidth;	/* default column width in chars */
	int		maxReqWidth;	/* the maximum requested width in pixels */
	int		maxReqHeight;	/* the maximum requested height in pixels */
	char 		*arrayVar;	/* name af array variable */
	int		borderWidth;	/* internal borderwidth */
	tagStruct	defaultTag;	/* the default tag colours/fonts etc */
	char 		*yScrollCmd;	/* the y-scroll command */
	char		*xScrollCmd;	/* the x-scroll command */
	Tk_3DBorder	cursorBg;	/* the cursor colour */
	int		rowThenCol;	/* is the table in row then col mode */
	int		selectionOn;	/* Is selection enabled */

	/* 
	** the topleft cell displayed excluding the fixed title rows 
	** this is just the configuration request. The actual cell 
	** used may be different to keep the screen full
	*/
	int		topRow;		
	int		leftCol;	
	int		titleRows;	/* the number of rows to use as a title */
	int 		titleCols;	/* the number of columns to use as a title */

	/* Cached Information */
	int		selRow, selCol;	/* the row and column of the selected cell */
	int 		oldTopRow;	/* the top row the last time TableAdjust Params */
	int 		oldLeftCol;	/* was called */
	int		oldSelRow;	/* the previous selected row and column */
	int		oldSelCol;
	int		textCurPosn;	/* The position of the text cursor in the string */
	int		tableFlags;	/* An or'ed combination of flags concerning redraw/cursor etc. */
	int		drawMode;	/* The mode to use when redrawing */
	int		colStretchMode;	/* The way to stretch columns if the window is too large */
	int		rowStretchMode;	/* The way to stretch rows if the window is too large */
	int		maxWidth;	/* max width required in pixels */
	int		maxHeight;	/* max height required in pixels */
	int		charWidth;	/* width of a character in the default font */
	int		*colPixels;	/* Array of the pixel width of each column */
	int		*rowPixels;	/* Array of the pixel height of each row */
	int		*colStarts;	/* Array of start pixels for columns */
	int 		*rowStarts;	/* Array of start pixels for rows */
	int		colOffset;	/* X index of the leftmost column in the display */
	int		rowOffset;	/* Y index of the topmost row in the display */
	int		flashTime;	/* The number of ticks to flash a cell for */
	tagStruct	*flashTag;	/* the tag used for flashing cells */
	Tcl_HashTable	*colWidths;	/* hash table of non default column widths */
	Tcl_HashTable	*rowHeights;	/* hash table of non default row heights */
	Tcl_HashTable	*tagTable;	/* the table for the style tags */
	Tcl_HashTable	*rowStyles;	/* the table for row styles */
	Tcl_HashTable	*colStyles;	/* the table for col styles */
	Tcl_HashTable	*cellStyles;	/* the table for cell styles */
	Tcl_HashTable	*flashCells;	/* the table full of flashing cells */
	Tcl_HashTable	*gcCache;	/* the Graphic context cache */
	Tk_TimerToken	cursorTimer;	/* the timer token for the cursor blinking */
	Tk_TimerToken	flashTimer;	/* the timer token for the cell flashing */
	char 		*selectBuf;	/* the buffer where the selection is kept for editing */
	int		selectBufLen;	/* the current length of the buffer */

	/* The invalid recatangle if there is an update pending */
	int		invalidX,
			invalidY,
			invalidWidth,
			invalidHeight;
} Table ;


/* structure for use in command parsing tables */
typedef struct  {
	char 	*name;
	int	value;
} command_struct;



extern Tk_ConfigSpec TableConfig[];

/* Forward Function Definitions */

Tcl_CmdProc		TableCmd;
Tk_EventProc 		TableEventProc;
Tcl_CmdProc		TableWidgetCmd;
Tk_IdleProc  		TableDisplay;
Tk_FreeProc  		TableDestroy;
Tcl_VarTraceProc	TableVarProc;
void TableDimensionCalc(Table *);
void TableInvalidate(Table *, int, int, int, int, int);
void TableCellCoords(Table *, int, int, int *, int *, int *, int *);
void TableGetRange(Table *, int, int, int, int,	int *, int *, int *, int *);
int parse_command(Tcl_Interp *, command_struct *,char *);
void TablePixelCalc(Table *tablePtr);
void TableAdjustParams(Table *);
void TableTagConfig(Table *, tagStruct *);
void TableWhatCell(Table *, int , int , int *, int *);
tagStruct * TableNewTag( Table *);
int TableTagCommand(Table *, int, char **);
void TableCleanupTag(Table *, tagStruct *);
void TableCursorEvent(ClientData);
void TableMakeArrayIndex(Table *, int, int, char *);
int TableParseArrayIndex(Table *, int *, int *, char *);
int TableParseStringPosn(Table *, char *, int *);
void TableBufLengthen(Table *, int);
void TableGetSelection(Table *);
void TableConfigCursor(Table *tablePtr);
void TableFlashConfigure(Table *, int );
void TableFlashEvent(ClientData);
void TableAddFlash(Table *, int , int );
void TableMergeTag(tagStruct *, tagStruct *);
GC TableGetGc(Table *, tagStruct *);


extern Tk_ConfigSpec tagConfig[];

