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
	Tk_Cursor	cursor;         /* the regular mouse pointer */
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
	char           *rowTagProc,
	               *colTagProc; 
} Table ;


/* structure for use in command parsing tables */
typedef struct  {
	char 	*name;
	int	value;
} command_struct;



extern Tk_ConfigSpec TableConfig[];

/* Forward Function Definitions */
#ifdef __STDC__
#define PL_(x) x
#else
#define PL_(x) ( )
#endif /* __STDC__ */
 
extern int TableWidgetCmd PL_(( ClientData clientdata, Tcl_Interp *interp, int argc, char *argv[] ));
extern int TableTagCommand PL_(( Table *tablePtr, int argc, char *argv[] ));
extern int parse_command PL_(( Tcl_Interp *interp, command_struct *commands, char *arg ));
extern void TableGetSelection PL_(( Table *tablePtr ));
extern void TableMakeArrayIndex PL_(( Table *tablePtr, int row, int col, char *buf ));
extern int TableParseArrayIndex PL_(( Table *tablePtr, int *row, int *col, char *index ));
extern int TableParseStringPosn PL_(( Table *tablePtr, char *arg, int *posn ));
extern void TableBufLengthen PL_(( Table *tablePtr, int len ));
extern int TableConfigure PL_(( Tcl_Interp *interp, Table *tablePtr, int argc, char *argv[], int flags ));
extern void TableAdjustParams PL_(( Table *tablePtr ));
extern tagStruct * TableNewTag PL_(( Table *tablePtr ));
extern void TableCleanupTag PL_(( Table *tablePtr, tagStruct *tagPtr ));
extern void TableDisplay PL_(( ClientData clientdata ));
extern void TableCellCoords PL_(( Table *tablePtr, int row, int col, int *x, int *y, int *width, int *height ));
extern void TableWhatCell PL_(( Table *tablePtr, int x, int y, int *row, int *col ));
extern void TableMergeTag PL_(( tagStruct *baseTag, tagStruct *addTag ));
extern GC TableGetGc PL_(( Table *tablePtr, tagStruct *tagPtr ));
extern void TableEventProc PL_(( ClientData clientdata, XEvent *eventPtr ));
extern void TableInvalidate PL_(( Table *tablePtr, int x, int y, int width, int height, int force ));
extern void TableDestroy PL_(( ClientData clientdata ));
extern char * TableVarProc PL_(( ClientData clientdata, Tcl_Interp *interp, char *name, char *index, int flags ));
extern void TableCursorEvent PL_(( ClientData clientdata ));
extern void TableConfigCursor PL_(( Table *tablePtr ));
extern void TableFlashConfigure PL_(( Table *tablePtr, int startStop ));
extern void TableFlashEvent PL_(( ClientData clientdata ));
extern void TableAddFlash PL_(( Table *tablePtr, int row, int col ));
extern int Table_Init PL_(( Tcl_Interp *interp ));
extern int TableCmd PL_(( ClientData clientdata, Tcl_Interp *interp, int argc, char *argv[] ));
 
#undef PL_


extern Tk_ConfigSpec tagConfig[];

#endif
