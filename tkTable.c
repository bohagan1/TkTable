/* 
 * tkTable.c --
 *
 *	This module implements table widgets for the Tk
 *	toolkit.  An table displays a 2D array of strings
 *	and allows the strings to be edited.
 *
 * Copyright (c) 1994 by Roland King
 * See the file COPYRIGHT for the copyright notes.
 *
 * Updates 1996 by:
 * Jeffrey Hobbs	jhobbs@cs.uoregon.edu
 * John Ellson		ellson@lucent.com
 * Peter Bruecker	peter@bj-ig.de
 * Tom Moore		tmoore@spatial.ca
 * Sebastian Wangnick	wangnick@orthogon.de
 *
 */

#include <string.h>
#include <stdlib.h>
#include <tk.h>

#define max(A,B) ((A)>(B))?(A):(B)
#define min(A,B) ((A)>(B))?(B):(A)
#define ARSIZE(A)  (sizeof(A)/sizeof(*A))

/* required for -state option */
extern Tk_Uid	tkNormalUid, tkDisabledUid;

#ifdef IMP
#include "tkXimp.h"
extern ImpInfo impInfo;

static void AddXIMeventMask( d, w )
Display *d;
Window   w;
{
  XWindowAttributes attr;
  return; /* Erk, fix this */
  if ( !XGetWindowAttributes( d, w, &attr ) ) return;
  XSelectInput( d, w, (impInfo.eventMask | attr.your_event_mask) );
}

static void ImpEvent(Table *tablePt) {
Display *display = Tk_Display (tablePtr->tkwin);
Window client = Tk_WindowId (tablePtr->tkwin);
  if (!impInfo.init) {
    Tk_XimpStart (tablePtr->interp, tablePtr->tkwin, 0, NULL);
  }
  if ( impInfo.ic && !impInfo.window) {
    XSetICValues (impInfo.ic, XNClientWindow,
                  impInfo.window = Tk_WindowId(Tk_NameToWindow(tablePtr->interp, ".", tablePtr->tkwin)),
                  NULL);
    AddXIMeventMask( display, impInfo.window );
  }
}

static void ImpDestroy(Table *tablePtr) {
Window client = Tk_WindowId (tablePtr->tkwin);
  if (impInfo.init && (client == impInfo.eventWin)) {
    XSetICValues (impInfo.ic, XNFocusWindow,impInfo.window, NULL);
  }
}

static void ImpFocusIn(Table *tablePtr,XEvent *eventPtr) {
Display *display = Tk_Display (tablePtr->tkwin);
Window client = Tk_WindowId (tablePtr->tkwin);
  if (impInfo.init) {
    if (eventPtr->xany.window == impInfo.window) {
      XSetInputFocus (display, client, RevertToParent, CurrentTime);
    } else if (eventPtr->xany.window == client && impInfo.ic) {
      XSetICValues (impInfo.ic, XNFocusWindow, client, NULL);
      XSetICFocus (impInfo.ic);
      /* XmbResetIC( impInfo.ic ); */
      impInfo.icFlag = 1;
      impInfo.eventWin = client;
    }
  }
}

static void ImpFocusOut(Table *tablePtr,XEvent *eventPtr) {
Window client = Tk_WindowId (tablePtr->tkwin);
  if (impInfo.init && (eventPtr->xany.window == client && impInfo.ic)) {
    /*XmbResetIC( impInfo.ic );*/
    XUnsetICFocus (impInfo.ic);
    impInfo.icFlag = 0;
  }
}

static void ImpInit(Table *tablePtr) {
  if (impInfo.init && (impInfo.ClientWindowFlag == 0 && impInfo.ic)) {
    XSetICValues(
      impInfo.ic,
      XNClientWindow,
      impInfo.window=Tk_WindowId(Tk_NameToWindow(tablePtr->interp, ".", tkwin)),
      NULL
    );
    impInfo.ClientWindowFlag = 1;
    AddXIMeventMask( tablePtr->display, impInfo.window );
  }
}

#ifdef KANJI
static wchar *ImpKGetStr(Tcl_Interp *interp, char *cp) {
wchar *n;
  if (impInfo.init && (impInfo.showFlag == 1)) {
    n = Tk_GetWStr (interp, impInfo.buf);
    impInfo.showFlag = 0;
  } else {
    n = Tk_GetWStr (interp, cp);
  } 
  return n;
}
#endif		/* KANJI */

#else		/* IMP */

#define ImpInit(t)
#define ImpEvent(t)
#define ImpFocusIn(t,e)
#define ImpFocusOut(t,e)
#define ImpDestroy(t)
#define ImpKGetStr(interp,cp)  Tk_GetWStr(interp,cp)

#endif		/* IMP */

#ifdef KANJI
#include "tkWStr.h"
#define TkMeasureChars  TkMeasureWChars
#define TkDisplayChars  TkDisplayWChars

#define DEF_TABLE_FONT          "a14"
#define DEF_TABLE_KANJIFONT     "k14"
#else
#if (TK_MAJOR_VERSION > 4)
#define DEF_TABLE_FONT          "Helvetica 12"
#else
#define DEF_TABLE_FONT          "-Adobe-Helvetica-Bold-R-Normal--*-120-*"
#endif
#endif		/* KANJI */

#define TBL_REDRAW_PENDING	1	/* redraw event already pending */
#define TBL_CURSOR_ON		2	/* The edit cursor is on */
#define	TBL_HAS_FOCUS		4	/* the window has the focus */
#define TBL_TEXT_CHANGED	8	/* the text has been edited */
#define TBL_HAS_ACTIVE		16	/* active specified */
#define TBL_HAS_ANCHOR		32	/* anchor specified */
#define TBL_REDRAW_BORDER	128	/* */

/* The tag structure */
typedef struct {
  Tk_3DBorder bgBorder;
  XColor *foreground;		/* foreground colour */
  int relief;			/* relief type */
#ifdef KANJI
  XWSFontSet *fontPtr;
  XFontStruct *asciiFontPtr;
  XFontStruct *kanjiFontPtr;
#else
#if (TK_MAJOR_VERSION > 4)
  Tk_Font	tkfont;		/* Information about text font, or NULL. */
#else
  XFontStruct   *fontPtr;       /* default font pointer */
#endif              /* KANJI */
#endif              /* KANJI */
  Tk_Anchor anchor;		/* default anchor point */
} TagStruct;

/*  The widget structure for the table Widget */

typedef struct {
  /* basic information about the window and the interpreter */
  Tk_Window tkwin;
  Display *display;
  Tcl_Interp *interp;
  /* Configurable Options */
  int autoClear;
  char * selectMode;		/* single, browse, multiple, extended */
  int rows, cols;		/* number of rows and columns */
  int defRowHeight;		/* default row height in pixels */
  int defColWidth;		/* default column width in chars */
  int maxReqWidth;		/* the maximum requested width in pixels */
  int maxReqHeight;		/* the maximum requested height in pixels */
  char *arrayVar;		/* name af array variable */
  int borderWidth;		/* internal borderwidth */
  TagStruct defaultTag;		/* the default tag colours/fonts etc */
  char *yScrollCmd;		/* the y-scroll command */
  char *xScrollCmd;		/* the x-scroll command */
  Tk_3DBorder insertBg;		/* the cursor colour */
  Tk_Cursor cursor;		/* the regular mouse pointer */
  int rowThenCol;		/* is the table in row then col mode */
  int selectionOn;		/* Is selection enabled */
  Tk_Uid state;			/* Normal or disabled.	Entry is read-only
				 * when disabled. */
  int insertWidth;		/* Total width of insert cursor. */
  int insertBorderWidth;	/* Width of 3-D border around insert cursor. */
  int insertOnTime;		/* Number of milliseconds cursor should spend
				 * in "on" state for each blink. */
  int insertOffTime;		/* Number of milliseconds cursor should spend
				 * in "off" state for each blink. */
  int colStretch;		/* The way to stretch columns if the window
				   is too large */
  int rowStretch;		/* The way to stretch rows if the window is
				   too large */
  int colOffset;		/* X index of leftmost col in the display */
  int rowOffset;		/* Y index of topmost row in the display */
  int drawMode;			/* The mode to use when redrawing */
  int flashMode;		/* Specifies whether flashing is enabled */
  int flashTime;		/* The number of ms to flash a cell for */
  int batchMode;		/*  */
  char *rowTagCmd, *colTagCmd;
  int highlightWidth;		/* Width in pixels of highlight to draw
				 * around widget when it has the focus.
				 * <= 0 means don't draw a highlight. */
  XColor *highlightBgColorPtr;	/* Color for drawing traversal highlight
				 * area when highlight is off. */
  XColor *highlightColorPtr;	/* Color for drawing traversal highlight. */
  char *takeFocus;		/* */

  int topRow, leftCol;		/* The topleft cell to display excluding the
				 * fixed title rows.  This is just the
				 * config request.  The actual cell used may
				 * be different to keep the screen full */
  int titleRows, titleCols;	/* the number of rows|cols to use as a title */
  /* Cached Information */
  int anchorRow, anchorCol;	/* the row,col of the anchor cell */
  int activeRow, activeCol;	/* the row,col of the active cell */
  int selRow, selCol;		/* the row,col of the selected cell */
  int oldTopRow, oldLeftCol;	/* cached by TableAdjustParams */
  int oldActRow, oldActCol;	/* cached by TableAdjustParams */
  int textCurPosn;		/* The position of the text cursor in the
				   string */
  int flags;			/* An or'ed combination of flags concerning
				   redraw/cursor etc. */
  int maxWidth, maxHeight;	/* max width|height required in pixels */
  int charWidth;		/* width of a character in the default font */
  int *colPixels;		/* Array of the pixel width of each column */
  int *rowPixels;		/* Array of the pixel height of each row */
  int *colStarts, *rowStarts;	/* Array of start pixels for rows|columns */
  int scanMarkX, scanMarkY;
  int scanMarkRowOffset;
  int scanMarkColOffset;
  Tcl_HashTable *colWidths;	/* hash table of non default column widths */
  Tcl_HashTable *rowHeights;	/* hash table of non default row heights */
  Tcl_HashTable *tagTable;	/* the table for the style tags */
  Tcl_HashTable *rowStyles;	/* the table for row styles */
  Tcl_HashTable *colStyles;	/* the table for col styles */
  Tcl_HashTable *cellStyles;	/* the table for cell styles */
  Tcl_HashTable *flashCells;	/* the table full of flashing cells */
  Tcl_HashTable *selCells;	/* the table full of selected cells */
  Tcl_TimerToken cursorTimer;	/* the timer token for the cursor blinking */
  Tcl_TimerToken flashTimer;	/* the timer token for the cell flashing */
#ifdef KANJI
  wchar	 *activeBuf;		/* the buffer where the selection is kept
				   for editing */
#else
  char *activeBuf;		/* the buffer where the selection is kept
				   for editing */
#endif		    /* KANJI */
  int activeBufLen;		/* the current length of the buffer */
  /* The invalid rectangle if there is an update pending */
  int invalidX, invalidY, invalidWidth, invalidHeight;
} Table;

/* structure for use in command parsing tables */
typedef struct {
  char *name;
  int value;
} command_struct;

/*
 * simple command_struct lookup functions
 */

static char *getCmdName(const command_struct* c,int val)
{
  for(;c->name && c->name[0];c++) {
    if(c->value==val) return c->name;
  }
  return NULL;
}

static int getCmdValue(const command_struct* c,const char *arg)
{
  int len=strlen(arg);
  for(;c->name && c->name[0];c++) {
    if(!strncmp(c->name,arg,len)) return c->value;
  }
  return 0;
}

static void getCmdError(Tcl_Interp *interp,command_struct *c,const char *arg)
{
  int i;
  Tcl_AppendResult (interp, "bad option \"", arg,"\" must be ", (char *) 0);
  for(i=0;c->name && c->name[0];c++,i++) {
    Tcl_AppendResult (interp,i?", ":"",c->name,(char *) 0);
  }
}

/*
 * Functions for handling custom options that use command_structs
 */

static int TableOptionSet(ClientData clientData, Tcl_Interp *interp,
			  Tk_Window tkwin, char *value, char *widgRec,
			  int offset)
{
  command_struct *p=(command_struct *)clientData;
  int mode=getCmdValue(p,value);
  if(!mode) {
    getCmdError(interp,p,value);
    return TCL_ERROR;
  }
  *((int*)(widgRec+offset))=mode;
  return TCL_OK;
}

static char *TableOptionGet(ClientData clientData, Tk_Window tkwin,
			    char *widgRec, int offset,
			    Tcl_FreeProc **freeProcPtr)
{
  command_struct *p=(command_struct *)clientData;
  int mode = *((int*)(widgRec+offset));
  return getCmdName(p,mode);
}


/* The list of command values for all the widget commands */

#define CMD_ACTIVATE	1	/* activate command a la listbox */
#define CMD_BBOX	2	/* bounding box of cell <index> */
#define CMD_BOUNDARY	3	/* set/return boundary row/col */
#define CMD_CGET	4	/* basic cget widget command */
#define	CMD_CONFIGURE	5	/* general configure command */
#define CMD_CURSELECTION 6	/* get current selected cell(s) */
#define CMD_CURVALUE	7	/* get current selection buffer */
#define	CMD_DELETE	8	/* delete text in the selection */
#define CMD_GET		9	/* get mode a la listbox */
#define	CMD_HEIGHT	12	/* (re)set row heights */
#define CMD_ICURSOR	13	/* set the insertion cursor */
#define CMD_INDEX	14	/* get an index */
#define CMD_INSERT	15	/* insert text at any position */
#define	CMD_REREAD	16	/* reread the current selection */
#define CMD_SCAN	17	/* scan command a la listbox */
#define CMD_SEE		18	/* see command a la listbox */
#define CMD_SELECTION	19	/* selection command a la listbox */
#define CMD_SET		20	/* set command, to set multiple items */
#define	CMD_TAG		21	/* tag command menu */
#define	CMD_WIDTH	22	/* (re)set column widths */
#define CMD_XVIEW	23	/* change x view of widget (for scrollbars) */
#define CMD_YVIEW	24	/* change y view of widget (for scrollbars) */

/* The list of commands for the command parser */

static command_struct commandos[] = {
  {"activate",		CMD_ACTIVATE},		/* jf 8/21/96 */
  {"bbox",		CMD_BBOX},
  {"boundary",		CMD_BOUNDARY},		/* jhobbs 12/8/96 */
  {"cget",		CMD_CGET},		/* jf 8/21/96 */
  {"configure",		CMD_CONFIGURE},
  {"curselection",	CMD_CURSELECTION},	/* 12/96 jhobbs modified */
  {"curvalue",		CMD_CURVALUE},		/* jf 8/21/96 */
  {"delete",		CMD_DELETE},
  {"get",		CMD_GET},		/* jf 8/21/96 */
  {"height",		CMD_HEIGHT},
  {"icursor",		CMD_ICURSOR},
  {"index",		CMD_INDEX},		/* jhobbs 12/2/96 */
  {"insert",		CMD_INSERT},
  {"reread",		CMD_REREAD},
  {"scan",		CMD_SCAN},		/* jf 8/21/96 */
  {"see",		CMD_SEE},		/* jf 8/21/96 */
  {"selection",		CMD_SELECTION},		/* jf 8/21/96 */
  {"set",		CMD_SET},		/* jhobbs 12/96 */
  {"tag",		CMD_TAG},
  {"width",		CMD_WIDTH},
  {"xview",		CMD_XVIEW},		/* jf 8/21/96 */
  {"yview",		CMD_YVIEW},		/* jf 8/21/96 */
  {"", 0}
};

/* List of tag subcommands */
#define TAG_CELLTAG	1	/* tag a cell */
#define TAG_CGET	2	/* get a config value */
#define TAG_COLTAG	3	/* tag a column */
#define	TAG_CONFIGURE	4	/* config/create a new tag */
#define	TAG_DELETE	5	/* delete a tag */
#define TAG_EXISTS	6	/* does a tag exist? */
#define	TAG_NAMES	7	/* print the tag names */
#define TAG_ROWTAG	8	/* tag a row */

static command_struct tag_commands[] = {
  {"celltag",	TAG_CELLTAG},
  {"coltag",	TAG_COLTAG},
  {"configure",	TAG_CONFIGURE},
  {"cget",	TAG_CGET},
  {"delete",	TAG_DELETE},
  {"exists",	TAG_EXISTS},
  {"names",	TAG_NAMES},
  {"rowtag",	TAG_ROWTAG},
  {"", 0}
};

#define SEL_ANCHOR      1
#define SEL_CLEAR       2
#define SEL_INCLUDES    3
#define SEL_SET         4

command_struct sel_commands[]= {
  {"anchor",	 SEL_ANCHOR},
  {"clear",	 SEL_CLEAR},
  {"includes",	 SEL_INCLUDES},
  {"set",	 SEL_SET},
  {"",		 0 }
};

/* drawmode subcommands */
#define	DRAW_MODE_SLOW		1	/* The display redraws with a pixmap
					   using TK function calls */
#define	DRAW_MODE_TK_COMPAT	2	/* The redisplay is direct to the
					   screen, but TK function calls are
					   still used to give correct 3-d
					   border appearance and thus remain
					   compatible with other TK apps */
#define DRAW_MODE_FAST		3	/* the redisplay goes straight to
					   the screen and the 3d borders are
					   rendered with a single pixel wide
					   line only. It cheats and uses the
					   internal border structure to do
					   the borders */

static command_struct drawmode_commands[] = {
  {"fast",		DRAW_MODE_FAST},
  {"compatible",	DRAW_MODE_TK_COMPAT},
  {"slow",		DRAW_MODE_SLOW},
  {"", 0}
};

/* boundary subcommands */
#define BOUNDARY_LEFT	1
#define	BOUNDARY_TOP	2
#define	BOUNDARY_BOTTOM	3
#define	BOUNDARY_RIGHT	4

static command_struct boundary_commands[] = {
  {"left",	BOUNDARY_LEFT},
  {"top",	BOUNDARY_TOP},
  {"bottom",	BOUNDARY_BOTTOM},
  {"right",	BOUNDARY_RIGHT},
  {"", 0}
};

/* insert/delete subcommands */
#define MOD_ACTIVE	1
#define MOD_COLS	2
#define MOD_ROWS	3
static command_struct mod_commands[] = {
  {"active",	MOD_ACTIVE},
  {"cols",	MOD_COLS},
  {"rows",	MOD_ROWS},
  {"", 0}
};

/* stretch subcommands */
#define	STRETCH_MODE_NONE	1	/* No additional pixels will be
					   added to rows or cols */
#define	STRETCH_MODE_UNSET	2	/* All default rows or columns will
					   be stretched to fill the screen */
#define STRETCH_MODE_ALL	3	/* All rows/columns will be padded
					   to fill the window */
#define STRETCH_MODE_LAST	4	/* Stretch last elememt to fill
					   window */
#define STRETCH_MODE_FILL       5       /* More ROWS in Window */

static command_struct stretch_commands[] = {
  {"none",	STRETCH_MODE_NONE},
  {"unset",	STRETCH_MODE_UNSET},
  {"all",	STRETCH_MODE_ALL},
  {"last",	STRETCH_MODE_LAST},
  {"fill",	STRETCH_MODE_FILL},
  {"", 0}
};

/* The widget configuration table */
static Tk_CustomOption stretchOption  = { TableOptionSet, TableOptionGet,
					  (ClientData)(&stretch_commands) };
static Tk_CustomOption drawOption = { TableOptionSet, TableOptionGet,
				      (ClientData)(&drawmode_commands) };

static Tk_ConfigSpec TableConfig[] = {
  {TK_CONFIG_ANCHOR, "-anchor", "anchor", "Anchor", "center",
   Tk_Offset(Table, defaultTag.anchor), 0 },
  {TK_CONFIG_BOOLEAN, "-autoclear", "autoClear", "AutoClear", "0",
   Tk_Offset(Table, autoClear), 0 },
  {TK_CONFIG_BORDER, "-background", "background", "Background", "gray80",
   Tk_Offset(Table, defaultTag.bgBorder), 0 },
  {TK_CONFIG_BOOLEAN, "-batchmode", "batchMode", "BatchMode", "0",
   Tk_Offset(Table, batchMode), 0 },
  {TK_CONFIG_SYNONYM, "-bg", "background", (char *) NULL,
   (char *) NULL, 0, 0},
  {TK_CONFIG_SYNONYM, "-bd", "borderWidth", (char *) NULL,
   (char *) NULL, 0, 0},
  {TK_CONFIG_PIXELS, "-borderwidth", "borderWidth", "BorderWidth", "1",
   Tk_Offset(Table, borderWidth), 0 },
  {TK_CONFIG_INT, "-colorigin", "colOrigin", "Origin", "0",
   Tk_Offset(Table, colOffset), 0 },
  {TK_CONFIG_INT, "-cols", "cols", "Cols", "10",
   Tk_Offset(Table, cols), 0 },
  {TK_CONFIG_CUSTOM, "-colstretchmode", "colStretch", "StretchMode",
   "none", Tk_Offset (Table, colStretch), 0 , &stretchOption },
  {TK_CONFIG_STRING, "-coltagcommand", "colTagCommand", "TagCommand", NULL,
   Tk_Offset(Table, colTagCmd), TK_CONFIG_NULL_OK },
  {TK_CONFIG_ACTIVE_CURSOR, "-cursor", "cursor", "Cursor", "xterm",
   Tk_Offset(Table, cursor), TK_CONFIG_NULL_OK },
  {TK_CONFIG_CUSTOM, "-drawmode", "drawMode", "DrawMode", "compatible",
   Tk_Offset (Table, drawMode), 0, &drawOption },
  {TK_CONFIG_SYNONYM, "-fg", "foreground", (char *) NULL, (char *) NULL,
   0, 0 },
  {TK_CONFIG_BOOLEAN, "-flashmode", "flashMode", "FlashMode", "0",
   Tk_Offset(Table, flashMode), 0 },
  {TK_CONFIG_INT, "-flashtime", "flashTime", "FlashTime", "2",
   Tk_Offset(Table, flashTime), 0 },
#ifdef KANJI
  {TK_CONFIG_FONT, "-font", "font", "Font",  DEF_TABLE_FONT,
   Tk_Offset(Table, defaultTag.asciiFontPtr), 0 },
  {TK_CONFIG_FONT, "-kanjifont", "kanjifont", "KanjiFont", DEF_TABLE_KANJIFONT,
   Tk_Offset(Table, defaultTag.kanjiFontPtr), 0 },
#else
  {TK_CONFIG_FONT, "-font", "font", "Font",  DEF_TABLE_FONT,
#if (TK_MAJOR_VERSION > 4)
   Tk_Offset(Table, defaultTag.tkfont), 0 },
#else
   Tk_Offset(Table, defaultTag.fontPtr), 0 },
#endif
#endif              /* KANJI */
  {TK_CONFIG_COLOR, "-foreground", "foreground", "Foreground", "black",
   Tk_Offset(Table, defaultTag.foreground), 0 },
  {TK_CONFIG_INT, "-height", "height", "Height", 0,
   Tk_Offset(Table, defRowHeight), TK_CONFIG_NULL_OK },
  {TK_CONFIG_COLOR, "-highlightbackground", "highlightBackground",
   "HighlightBackground", "GRAY80", Tk_Offset(Table, highlightBgColorPtr), 0 },
  {TK_CONFIG_COLOR, "-highlightcolor", "highlightColor", "HighlightColor",
   "Black", Tk_Offset(Table, highlightColorPtr), 0 },
  {TK_CONFIG_PIXELS, "-highlightthickness", "highlightThickness",
   "HighlightThickness", "2", Tk_Offset(Table, highlightWidth), 0 },
  {TK_CONFIG_BORDER, "-insertbackground", "insertBackground", "Foreground",
   "black", Tk_Offset(Table, insertBg), 0 },
  {TK_CONFIG_PIXELS, "-insertborderwidth", "insertBorderWidth", "BorderWidth",
   "0", Tk_Offset(Table, insertBorderWidth), TK_CONFIG_COLOR_ONLY},
  {TK_CONFIG_PIXELS, "-insertborderwidth", "insertBorderWidth", "BorderWidth",
   "0", Tk_Offset(Table, insertBorderWidth), TK_CONFIG_MONO_ONLY},
  {TK_CONFIG_INT, "-insertofftime", "insertOffTime", "OffTime", "300",
   Tk_Offset(Table, insertOffTime), 0},
  {TK_CONFIG_INT, "-insertontime", "insertOnTime", "OnTime", "600",
   Tk_Offset(Table, insertOnTime), 0},
  {TK_CONFIG_PIXELS, "-insertwidth", "insertWidth", "InsertWidth", "2",
   Tk_Offset(Table, insertWidth), 0},
  {TK_CONFIG_PIXELS, "-maxheight", "maxHeight", "MaxHeight", "800",
   Tk_Offset(Table, maxReqHeight), 0 },
  {TK_CONFIG_PIXELS, "-maxwidth", "maxWidth", "MaxWidth", "1000",
   Tk_Offset(Table, maxReqWidth), 0 },
  {TK_CONFIG_RELIEF, "-relief", "relief", "Relief", "sunken",
   Tk_Offset(Table, defaultTag.relief), 0 },
  {TK_CONFIG_BOOLEAN, "-rowfirstmode", "rowFirstMode", "RowFirstMode", "1",
   Tk_Offset(Table, rowThenCol), 0 },
  {TK_CONFIG_INT, "-roworigin", "rowOrigin", "Origin", "0",
   Tk_Offset(Table, rowOffset), 0 },
  {TK_CONFIG_INT, "-rows", "rows", "Rows", "10", Tk_Offset(Table, rows), 0 },
  {TK_CONFIG_CUSTOM, "-rowstretchmode", "rowStretch", "StretchMode",
   "none", Tk_Offset (Table, rowStretch), 0 , &stretchOption },
  {TK_CONFIG_STRING, "-rowtagcommand", "rowTagCommand", "TagCommand", NULL,
   Tk_Offset(Table, rowTagCmd), TK_CONFIG_NULL_OK },
  {TK_CONFIG_STRING, "-selectmode", "selectMode", "SelectMode", "single",
   Tk_Offset(Table, selectMode), TK_CONFIG_NULL_OK },
  {TK_CONFIG_UID, "-state", "state", "State", "normal",
   Tk_Offset(Table, state), 0},
  {TK_CONFIG_STRING, "-takefocus", "takeFocus", "TakeFocus", NULL,
   Tk_Offset(Table, takeFocus), TK_CONFIG_NULL_OK },
  {TK_CONFIG_INT, "-titlecols", "titleCols", "TitleCols", "0",
   Tk_Offset(Table, titleCols), TK_CONFIG_NULL_OK },
  {TK_CONFIG_INT, "-titlerows", "titleRows", "TitleRows", "0",
   Tk_Offset(Table, titleRows), TK_CONFIG_NULL_OK },
  {TK_CONFIG_STRING, "-variable", "variable", "Variable", NULL,
   Tk_Offset(Table, arrayVar), TK_CONFIG_NULL_OK },
  {TK_CONFIG_INT, "-width", "width", "Width", "10",
   Tk_Offset(Table, defColWidth), 0 },
  {TK_CONFIG_STRING, "-xscrollcommand", "xScrollCommand", "ScrollCommand",
   NULL, Tk_Offset(Table, xScrollCmd), TK_CONFIG_NULL_OK },
  {TK_CONFIG_STRING, "-yscrollcommand", "yScrollCommand", "ScrollCommand",
   NULL, Tk_Offset(Table, yScrollCmd), TK_CONFIG_NULL_OK },
  {TK_CONFIG_END, (char *) NULL, (char *) NULL, (char *) NULL,
   (char *) NULL, 0, 0 }
};

/* 
  The default specification for configuring tags
  Done like this to make the command line parsing 
  easy 
 */

static Tk_ConfigSpec tagConfig[] = {
  {TK_CONFIG_BORDER, "-background", "background", "Background", NULL,
   Tk_Offset(TagStruct, bgBorder), TK_CONFIG_NULL_OK },
  {TK_CONFIG_SYNONYM, "-bg", "background", (char *) NULL,
   (char *) NULL, 0, 0 },
  {TK_CONFIG_COLOR, "-foreground", "foreground", "Foreground", NULL,
   Tk_Offset(TagStruct, foreground), TK_CONFIG_NULL_OK },
  {TK_CONFIG_SYNONYM, "-fg", "foreground", (char *) NULL,
   (char *) NULL, 0, 0 },
#ifdef KANJI
  {TK_CONFIG_FONT, "-font", "font", "Font",  NULL,
   Tk_Offset(TagStruct,asciiFontPtr), TK_CONFIG_NULL_OK },
  {TK_CONFIG_FONT, "-kanjifont", "kanjifont", "KanjiFont", NULL,
   Tk_Offset(TagStruct,kanjiFontPtr), TK_CONFIG_NULL_OK },
#else
#if (TK_MAJOR_VERSION > 4)
  {TK_CONFIG_FONT, "-font", "font", "Font", NULL,
   Tk_Offset(TagStruct, tkfont), 0 },
#else
  {TK_CONFIG_FONT, "-font", "font", "Font", NULL,
   Tk_Offset(TagStruct, fontPtr), TK_CONFIG_NULL_OK },
#endif
#endif              /* KANJI */
  {TK_CONFIG_ANCHOR, "-anchor", "anchor", "Anchor", NULL,
   Tk_Offset(TagStruct, anchor), 0 },
  {TK_CONFIG_RELIEF, "-relief", "relief", "Relief", NULL,
   Tk_Offset(TagStruct, relief), TK_CONFIG_NULL_OK },
  {TK_CONFIG_END, (char *) NULL, (char *) NULL, (char *) NULL,
   (char *) NULL, 0, 0 }
};

/* 
   Parses a command string passed in an arg comparing it with all the 
   command strings in the command array. If it finds a string which is
   a unique identifier of one of the commands, returns the index . If none
   of the commands match, or the abbreviation is not 
   unique, then it sets up the message accordingly and returns 0
   */

static int
parse_command (Tcl_Interp * interp, command_struct * commands, char *arg)
{
  int len = strlen (arg);
  command_struct *matched = (command_struct *) 0;
  int err = 0;
  command_struct *next = commands;
  while (*(next->name)) {
    if (strncmp (next->name, arg, len) == 0) {
      /* have we already matched this one if so make up an error message */
      if (matched) {
	if (!err) {
	  Tcl_AppendResult (interp, "ambiguous option \"", arg,
			    "\" could be ", matched->name, (char *) 0);
	  matched = next;
	  err = 1;
	}
	Tcl_AppendResult (interp, ", ", next->name, (char *) 0);
      } else {
	matched = next;
	/* return on an exact match */
	if (len == strlen(next->name))
	  return matched->value;
      }
    }
    next++;
  }
  /* did we get multiple possibilities */
  if (err) return 0;
  /* did we match any at all */
  if (matched) {
    return matched->value;
  } else {
    Tcl_AppendResult (interp, "bad option \"", arg,"\" must be ",
		      (char *) NULL);
    next = commands;
    while (1) {
      Tcl_AppendResult (interp, next->name, (char *) NULL);
      /* the end of them all ? */
      if (!*((++next)->name)) return 0;
      /* or the last one at least */
      if (*((next + 1)->name))
	Tcl_AppendResult (interp, ", ", (char *) NULL);
      else
	Tcl_AppendResult (interp, " or ", (char *) NULL);
    }
  }
}

/* Turn row/col into an index into the table */

static void TableMakeArrayIndex (Table * tablePtr, int row, int col, char *buf)
{
  if (tablePtr->rowThenCol)
    sprintf (buf, "%d,%d", row, col);
  else
    sprintf (buf, "%d,%d", col, row);
}

/* Turn array index back into row/col return the number of args parsed,
   should be two */

static int TableParseArrayIndex (Table * tablePtr, int *row, int *col,
				 char *index)
{
  if (tablePtr->rowThenCol)
    return sscanf (index, "%d,%d", row, col);
  else
    return sscanf (index, "%d,%d", col, row);
}

/* Returns the x, y, width and height for a given cell */

static void TableCellCoords (Table * tablePtr, int row, int col,
			     int *x, int *y, int *width, int *height)
{
  if (tablePtr->rows <= 0 || tablePtr->cols <= 0) {
    *width = *height = *x = *y = 0;
    return;
  }
  row = min (tablePtr->rows - 1, max (0, row));
  col = min (tablePtr->cols - 1, max (0, col));
  *width = (tablePtr->colPixels)[col];
  *height = (tablePtr->rowPixels)[row];
  *x = tablePtr->highlightWidth + (tablePtr->colStarts)[col] - 
    ((col < tablePtr->titleCols) ? 0 : (tablePtr->colStarts)[tablePtr->leftCol]
     - (tablePtr->colStarts)[tablePtr->titleCols]);
  *y = tablePtr->highlightWidth + (tablePtr->rowStarts)[row] -
    ((row < tablePtr->titleRows) ? 0 : (tablePtr->rowStarts)[tablePtr->topRow]
     - (tablePtr->rowStarts)[tablePtr->titleRows]);
}

/* Given an (x,y) coordinate as displayed on the screen, returns the Cell
   that contains that point */
static void TableWhatCell (Table * tablePtr, int x, int y, int *row, int *col)
{
  int i;
  x = max (0, x);
  y = max (0, y);
  x -= tablePtr->highlightWidth;
  y -= tablePtr->highlightWidth;
  /* Adjust the x coord if not in the column titles to change display coords
     into internal coords */
  x += (x < (tablePtr->colStarts)[tablePtr->titleCols]) ? 0 :
    (tablePtr->colStarts)[tablePtr->leftCol] -
    (tablePtr->colStarts)[tablePtr->titleCols];
  y += (y < (tablePtr->rowStarts)[tablePtr->titleRows]) ? 0 :
    (tablePtr->rowStarts)[tablePtr->topRow] -
    (tablePtr->rowStarts)[tablePtr->titleRows];
  x = min (x, tablePtr->maxWidth - 1);
  y = min (y, tablePtr->maxHeight - 1);
  for (i = 1; x >= (tablePtr->colStarts)[i]; i++);
  *col = i - 1;
  for (i = 1; y >= (tablePtr->rowStarts)[i]; i++);
  *row = i - 1;
}

/* Get Contets of Cell r,c VOLATILE */
 
static char *TableGetCellValue(Table *tablePtr,int r,int c)
{
  char buf[100];
  char *result;
  if (!tablePtr->arrayVar) return "";
  TableMakeArrayIndex(tablePtr,r,c,buf);
  result=Tcl_GetVar2(tablePtr->interp,tablePtr->arrayVar,buf,TCL_GLOBAL_ONLY);
#ifdef KANJI
  result=Tk_GetWStr(tablePtr->interp,result);
#endif
  if (!result) return "";
  return result;
}
 
/* Set Contents of Cell */
 
static void TableSetCellValue(Table *tablePtr,int r,int c,char *value)
{
  char buf[100];
  if (tablePtr->arrayVar==NULL || tablePtr->state==tkDisabledUid) return;
  TableMakeArrayIndex(tablePtr, r, c, buf);
#ifdef KANJI
  Tcl_SetVar2(tablePtr->interp, tablePtr->arrayVar, buf, Tk_DecodeWStr(value),
	      TCL_GLOBAL_ONLY);
#else
  Tcl_SetVar2(tablePtr->interp, tablePtr->arrayVar, buf, value,
	      TCL_GLOBAL_ONLY);
#endif
}

/*
 * Is the cell not visible on the display?
 */
static int TableCellHidden (Table *tablePtr, int row, int col)
{
  int x, y, width, height;
  row -= tablePtr->rowOffset;
  col -= tablePtr->colOffset;
  TableCellCoords (tablePtr, row,
		   col, &x, &y, &width, &height);
  if (x<0 || x>Tk_Width(tablePtr->tkwin) ||
      y<0 || y>Tk_Height(tablePtr->tkwin)) {
    /* definitely off the screen */
    return 1;
  } else {
    /* could be hiding in "dead" space */
    return ((row < tablePtr->topRow && row >= tablePtr->titleRows) ||
	    (col < tablePtr->leftCol && col >= tablePtr->titleCols));
  }
}

/*
  Parse the argument as an index into the selected cell text.
  recognises 'end' or an integer or insert constrains it to the
  size of the buffer 
*/

static int TableParseStringPosn (Table * tablePtr, char *arg, int *posn)
{
  int temp;
  /* is this end */
  if (strncmp (arg, "end", min (strlen (arg), 3)) == 0) {
#ifdef KANJI
    *posn=Tcl_WStrlen (tablePtr->activeBuf);
#else   /* KANJI */
    *posn = strlen (tablePtr->activeBuf);
#endif
  } else if (strncmp (arg, "insert", min (strlen (arg), 6)) == 0) {
    *posn = tablePtr->textCurPosn;
  } else {
    if (Tcl_GetInt (tablePtr->interp, arg, &temp) != TCL_OK) {
      Tcl_AppendResult (tablePtr->interp, "error \"", arg,
			"\" is not a valid index", (char *) NULL);
      return TCL_ERROR;
    }
#ifdef KANJI
    *posn = min( max (0, temp), Tcl_WStrlen(tablePtr->activeBuf));
#else   /* KANJI */
    *posn = min( max (0, temp), strlen(tablePtr->activeBuf));
#endif
  }
  return TCL_OK;
}

/*
  Makes the selection buffer long enough to hold
  a string of length len PLUS an extra character
  for the \0. Copies over the data if it lengthens
  the buffer and frees the old one
*/

static void TableBufLengthen (Table * tablePtr, int len)
{
  char *newbuf;
  if (tablePtr->activeBufLen < len + 1) {
    tablePtr->activeBufLen = (int) (len * 1.25) + 8;
    newbuf = ckalloc(tablePtr->activeBufLen);
    if (tablePtr->activeBuf) {
      /* copy over the data */
      strcpy (newbuf, tablePtr->activeBuf);
      /* free the old buffer */
      ckfree (tablePtr->activeBuf);
    }
    tablePtr->activeBuf = newbuf;
  }
}

/*
  ckallocs space for a new tag structure 
  and clears the structure returns 
  the pointer to the new structure
*/

static TagStruct *TableNewTag (Table * tablePtr)
{
  TagStruct *tagPtr;
  tagPtr = (TagStruct *) ckalloc(sizeof (TagStruct));
  tagPtr->bgBorder = NULL;
  tagPtr->foreground = NULL;
  tagPtr->relief = 0;
#if (TK_MAJOR_VERSION > 4)
  tagPtr->tkfont = NULL;
#else
  tagPtr->fontPtr = NULL;
#endif
#ifdef KANJI
  tagPtr->asciiFontPtr=NULL;
  tagPtr->kanjiFontPtr=NULL;
#endif
  tagPtr->anchor = -1;
  return tagPtr;
}

/* 
  Get the current selection into the
  buffer and mark it as unedited
  set the position to the end of the string
*/

static void TableGetActiveBuf (Table * tablePtr)
{
  char *data;
#ifdef KANJI
  wchar *oldBuf;
#endif
  char buf[100];
  /* ACTIVE */
  TableMakeArrayIndex (tablePtr, tablePtr->activeRow + tablePtr->rowOffset,
		       tablePtr->activeCol + tablePtr->colOffset, buf);
  if (tablePtr->arrayVar == NULL ||
      (data = Tcl_GetVar2(tablePtr->interp, tablePtr->arrayVar, buf,
			  TCL_GLOBAL_ONLY)) == NULL) {
    data = "";
  }
#ifdef KANJI
  /* copy over the data */
  oldBuf = tablePtr->activeBuf; /* jf 2/28/96 */
  tablePtr->activeBuf = Tk_GetWStr (tablePtr->interp, data);
  Tk_FreeWStr (oldBuf); /* jf 2/28/96 */
  tablePtr->textCurPosn=Tcl_WStrlen (tablePtr->activeBuf);
#else
  /* is the buffer long enough */
  TableBufLengthen (tablePtr, strlen (data));
  strcpy (tablePtr->activeBuf, data);
  tablePtr->textCurPosn = strlen (data);
#endif
  tablePtr->flags &= ~TBL_TEXT_CHANGED;
}

/*
  This routine merges two tags by adding
  any fields from the addTag that are
  unset in the base Tag
*/

static void TableMergeTag (TagStruct * baseTag, TagStruct * addTag)
{
  if (baseTag->foreground == NULL)
    baseTag->foreground = addTag->foreground;
  if (baseTag->bgBorder == NULL)
    baseTag->bgBorder = addTag->bgBorder;
  if (baseTag->relief == 0)
    baseTag->relief = addTag->relief;
#if (TK_MAJOR_VERSION > 4)
  if (baseTag->tkfont == NULL)
    baseTag->tkfont = addTag->tkfont;
#else
  if (baseTag->fontPtr == NULL)
    baseTag->fontPtr = addTag->fontPtr;
#endif
#ifdef KANJI
  if (baseTag->asciiFontPtr==NULL)
    baseTag->asciiFontPtr=addTag->asciiFontPtr;
  if (baseTag->kanjiFontPtr==NULL)
    baseTag->kanjiFontPtr=addTag->kanjiFontPtr;
#endif
  if (baseTag->anchor == -1)
    baseTag->anchor = addTag->anchor;
}

static Tcl_HashEntry * checkTagCmd (Tcl_Interp *interp, Table *tablePtr,
				    char *tagCmd, Tcl_HashTable *tagCmdTbl,
				    int which)
{
  Tcl_HashEntry *entryPtr;
  char temp[16];
  int new;
  Tcl_HashEntry *tblEntry;
  Tcl_DString string;
  /* 
    if this row/col already exists in the cache, return the already
    computed tag value.
  */
  if ((entryPtr = Tcl_FindHashEntry (tagCmdTbl, (char *) which)) == NULL) {
    /* Add an entry into the cache so we don't
       have to eval this row/col again */
    entryPtr = Tcl_CreateHashEntry (tagCmdTbl, (char *) which, &new);
    Tcl_SetHashValue (entryPtr, NULL);
    /* Since it does not exist, construct a command string and eval it */
    Tcl_DStringInit (&string);
    Tcl_DStringAppend (&string, tagCmd, -1);
    sprintf (temp, "%10d", which);
    Tcl_DStringAppend (&string, temp, -1);
    if (Tcl_Eval(interp, Tcl_DStringValue (&string)) == TCL_OK && 
	interp->result && *interp->result) {
      /* If a result was returned, check to see if it is a known tag */
      if ((tblEntry = Tcl_FindHashEntry (tablePtr->tagTable,
					 interp->result)) != NULL)
	/* A valid tag was return.  Add it into the cache for safe keeping */
	Tcl_SetHashValue (entryPtr, Tcl_GetHashValue (tblEntry));
    }
    Tcl_DStringFree (&string);
  }
  Tcl_ResetResult (interp);
  /*
    Check the entry pointer found in the hash table.  If the
    value is NULL, return NULL.  If there is a valid value, return
    the entryPtr.
   */
  return (Tcl_GetHashValue (entryPtr))?entryPtr:0;
}

static void CreateTagEntry(Table *tablePtr,char *name,int argc,char **argv) {
  Tcl_HashEntry *entryPtr;
  TagStruct *tagPtr = TableNewTag(tablePtr);
  int dummy;
  Tk_ConfigureWidget(tablePtr->interp,tablePtr->tkwin,
		     tagConfig,argc,argv,(char *)tagPtr,0);
  entryPtr = Tcl_CreateHashEntry (tablePtr->tagTable, name, &dummy);
  Tcl_SetHashValue (entryPtr, (ClientData) tagPtr);
}

static void InitTagTable(Table *tablePtr) {
  static char *activeArgs[]	= {"-bg", "GRAY50", "-relief", "flat" };
  static char *selArgs[]	= {"-bg", "GRAY60", "-relief", "sunken"};
  static char *titleArgs[]	= {"-bg", "GRAY70", "-relief", "flat"};
  static char *flashArgs[]	= {"-bg", "red", "-fg", "white" };
  CreateTagEntry(tablePtr, "active", ARSIZE(activeArgs), activeArgs);
  CreateTagEntry(tablePtr, "sel", ARSIZE(selArgs), selArgs);
  CreateTagEntry(tablePtr, "title", ARSIZE(titleArgs), titleArgs);
  CreateTagEntry(tablePtr, "flash", ARSIZE(flashArgs), flashArgs);
}

/* Gets a GC correponding to the tag structure passed */

#ifdef KANJI
static XWSGC
#else
static GC
#endif
TableGetGc (Table * tablePtr, TagStruct * tagPtr) {
  XGCValues gcValues;
  gcValues.foreground = tagPtr->foreground->pixel;
  gcValues.background = Tk_3DBorderColor(tagPtr->bgBorder)->pixel;
  gcValues.graphics_exposures   = False;
#ifdef KANJI
  tagPtr->fontPtr = Tk_GetFontSet(tagPtr->asciiFontPtr, tagPtr->kanjiFontPtr);
  return Tk_GetGCSet(tablePtr->tkwin,
		     GCForeground|GCBackground|GCFont|GCGraphicsExposures,
		     &gcValues,tagPtr->fontPtr);
#else
#if (TK_MAJOR_VERSION > 4)
  gcValues.font = Tk_FontId(tablePtr->defaultTag.tkfont);
#else
  gcValues.font = tagPtr->fontPtr->fid;
#endif
  return Tk_GetGC(tablePtr->tkwin,
		  GCForeground|GCBackground|GCFont|GCGraphicsExposures,
		  &gcValues);
#endif
}

static void TableFreeGc(Table *tablePtr,
#ifdef KANJI
XWSGC
#else
GC
#endif
gc) {
#ifdef KANJI
  Tk_FreeGCSet(tablePtr->display,gc);
#else 
  Tk_FreeGC(tablePtr->display,gc);
#endif
}

#define MAXGCCACHE 10

/* The main display procedure */

static void TableDisplay (ClientData clientdata)
{
  Table *tablePtr = (Table *) clientdata;
  Tk_Window tkwin = tablePtr->tkwin;
  Display *display = tablePtr->display;
  Tcl_Interp *interp = tablePtr->interp;
  Drawable window;
  int i, j;
  int rowFrom, rowTo, colFrom, colTo;
  int invalidX, invalidY, invalidWidth, invalidHeight;
  int x, y, width, height;
  int direction, ascent, descent, bd;
  int originX, originY;
  int activeCell, clipRectSet;
  int offsetX, offsetY;
  XCharStruct bbox, bbox2;
  XRectangle clipRect;
#ifdef KANJI
  XWSGC copyGc;
  GC topGc, bottomGc;
  wchar *value == "";
  char *tmpvar;
#else
  GC topGc, bottomGc, copyGc, lightGc, darkGc;
  char *value = "";
#endif
#if (TK_MAJOR_VERSION > 4)
  Tk_FontMetrics fm;
#endif
  char buf[150];
  TagStruct *tagPtr, *titlePtr, *selPtr, *activePtr, *flashPtr;
  Tcl_HashEntry *entryPtr;
  static XPoint rect[6] = { {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0} };
  Tcl_HashTable *rowProcTbl = NULL, *colProcTbl = NULL;

  tablePtr->flags &= ~TBL_REDRAW_PENDING;
  invalidX = tablePtr->invalidX;
  invalidY = tablePtr->invalidY;
  invalidWidth = tablePtr->invalidWidth;
  invalidHeight = tablePtr->invalidHeight;

  /* 
    if we are using the slow drawing mode with a pixmap 
    create the pixmap and set up offsetX and Y 
   */
  if (tablePtr->drawMode == DRAW_MODE_SLOW) {
    window = XCreatePixmap (display, Tk_WindowId (tkwin),
			    invalidWidth, invalidHeight, Tk_Depth (tkwin));
    offsetX = invalidX;
    offsetY = invalidY;
  } else {
    window = Tk_WindowId (tablePtr->tkwin);
    offsetX = 0;
    offsetY = 0;
  }

  /* set up the permanent tag styles */
  entryPtr = Tcl_FindHashEntry (tablePtr->tagTable, "title");
  titlePtr = (TagStruct *) Tcl_GetHashValue (entryPtr);
  entryPtr = Tcl_FindHashEntry (tablePtr->tagTable, "sel");
  selPtr   = (TagStruct *) Tcl_GetHashValue (entryPtr);
  entryPtr = Tcl_FindHashEntry(tablePtr->tagTable, "active");
  activePtr= (TagStruct *) Tcl_GetHashValue(entryPtr);
  entryPtr = Tcl_FindHashEntry(tablePtr->tagTable, "flash");
  flashPtr = (TagStruct *) Tcl_GetHashValue(entryPtr);

  /* find out the cells represented by the invalid region */
  TableWhatCell (tablePtr, invalidX, invalidY, &rowFrom, &colFrom);
  TableWhatCell (tablePtr, invalidX + invalidWidth - 1,
		 invalidY + invalidHeight - 1, &rowTo, &colTo);

  /* 
    If rowTagCmd or colTagCmd has been specified then initialize
    hash tables to cache evaluations.
   */
  if (tablePtr->rowTagCmd) {
    rowProcTbl = (Tcl_HashTable *) ckalloc(sizeof (Tcl_HashTable));
    Tcl_InitHashTable (rowProcTbl, TCL_ONE_WORD_KEYS);
  }
  if (tablePtr->colTagCmd) {
    colProcTbl = (Tcl_HashTable *) ckalloc(sizeof (Tcl_HashTable));
    Tcl_InitHashTable (colProcTbl, TCL_ONE_WORD_KEYS);
  }
  /* Cycle through the cells and display them */
  for (i = rowFrom; i <= rowTo; i++) {
    for (j = colFrom; j <= colTo; j++) {
      activeCell = 0;
      /* 
        are we in the 'dead zone' between the
        titles and the first displayed cell 
       */
      if (i < tablePtr->topRow && i >= tablePtr->titleRows)
	i = tablePtr->topRow;

      if (j < tablePtr->leftCol && j >= tablePtr->titleCols)
	j = tablePtr->leftCol;

#ifdef NO_TOP_LEFT
      /* are we in both row and column titles */
      if (i < tablePtr->titleRows && j < tablePtr->titleCols)
	continue;
#endif

      /* put the cell ref into a buffer for the hash lookups */
      TableMakeArrayIndex (tablePtr, i + tablePtr->rowOffset,
			   j + tablePtr->colOffset, buf);

      /* get the tag structure for the cell */

      /* first clear out a new tag structure that we will build in */
      tagPtr = TableNewTag (tablePtr);

      /* Add tags as we realize they belong.  Tags with the highest
	 priority are added first */

      /* if flash mode is on, is this cell flashing */
      if (tablePtr->flashMode &&
	  (entryPtr = Tcl_FindHashEntry (tablePtr->flashCells, buf)) != NULL)
	TableMergeTag (tagPtr, flashPtr);
      /* if select mode is on, is this cell selected or active? */
      if (tablePtr->selectionOn) {
	/* ACTIVE */
	if ((tablePtr->flags & TBL_HAS_ACTIVE) && 
	    tablePtr->state == tkNormalUid &&
	    i == tablePtr->activeRow && j == tablePtr->activeCol) {
	  TableMergeTag(tagPtr, activePtr);
	  activeCell = 1;
	}
	if ((entryPtr=Tcl_FindHashEntry(tablePtr->selCells, buf))!=NULL)
	  TableMergeTag(tagPtr, selPtr);
      }
      /* Am I in the titles */
      if (i < tablePtr->topRow || j < tablePtr->leftCol)
	TableMergeTag (tagPtr, titlePtr);
      /* Does this have a cell tag */
      if ((entryPtr = Tcl_FindHashEntry (tablePtr->cellStyles, buf)) != NULL)
	TableMergeTag (tagPtr, (TagStruct *) Tcl_GetHashValue (entryPtr));
      /* 
        Then try the rows.  First check in the row hash table.
        If nothing found there, then try evaluating the row
        tag procedure (if specified).
       */
      if ((entryPtr = Tcl_FindHashEntry (tablePtr->rowStyles, (char *) i + tablePtr->rowOffset)) == NULL) {
	if (tablePtr->rowTagCmd)
	  entryPtr = checkTagCmd (interp, tablePtr, tablePtr->rowTagCmd,
				   rowProcTbl, i + tablePtr->rowOffset);
      }
      if (entryPtr)
	TableMergeTag (tagPtr, (TagStruct *) Tcl_GetHashValue (entryPtr));

      /* 
        Then try the columns.  Same procedure as rows.  First check 
        in the row hash table.  If nothing found there, then try 
        evaluating the col tag procedure (if specified).
       */
      if ((entryPtr = Tcl_FindHashEntry (tablePtr->colStyles, (char *) j + tablePtr->colOffset)) == NULL) {
	if (tablePtr->colTagCmd)
	  entryPtr = checkTagCmd (interp, tablePtr, tablePtr->colTagCmd,
				   colProcTbl, j + tablePtr->colOffset);
      }
      if (entryPtr)
	TableMergeTag (tagPtr, (TagStruct *) Tcl_GetHashValue (entryPtr));

      /* Finally, merge in the default tag */
      TableMergeTag (tagPtr, &(tablePtr->defaultTag));

      copyGc = TableGetGc (tablePtr, tagPtr);

      /* get the coordinates for the cell */
      TableCellCoords (tablePtr, i, j, &x, &y, &width, &height);

      /* 
        first fill in a blank rectangle. This is left
        as a Tk call instead of a direct X call for Tk
        compatibilty. The TK_RELIEF_FLAT ensures that only
        XFillRectangle is called anyway so the speed is the same
       */
      Tk_Fill3DRectangle (tablePtr->tkwin, window, tagPtr->bgBorder,
			  x - offsetX, y - offsetY, width, height,
			  tablePtr->borderWidth, TK_RELIEF_FLAT);

      /* Is there a value in the cell? If so, draw it  */
#ifdef KANJI
      if ( tablePtr->arrayVar ) {
        tmpvar =Tcl_GetVar2(interp, tablePtr->arrayVar, buf, TCL_GLOBAL_ONLY);
      } else {
        tmpvar = NULL;
      }
      if ( tmpvar) {
        value=Tk_GetWStr(interp,tmpvar);
      } else {
        value = NULL;
      }
#else
      if ( tablePtr->arrayVar ) {
        value=Tcl_GetVar2(interp, tablePtr->arrayVar, buf, TCL_GLOBAL_ONLY);
      }
#endif

      /* if active and not disabled, always show something */
      /* or there really is a value there */
      if (value != NULL || activeCell) {
	/* if this is the selected cell and we can edit, use the buffer */
	if (activeCell) {
#ifdef KANJI
	  value = Tk_GetWStr(interp,Tk_DecodeWStr(tablePtr->activeBuf));
#else
	  value = tablePtr->activeBuf;
#endif
	}

	/* get the border width to adjust the calculations */
	bd = tablePtr->borderWidth;

	/* get the dimensions of the string */
#ifdef KANJI
        TkWSTextExtents(copyGc, value, Tcl_WStrlen(value),
                        &ascent, &descent, &bbox);
#else
#if (TK_MAJOR_VERSION > 4)
	Tk_GetFontMetrics(tagPtr->tkfont, &fm);
	bbox.rbearing = Tk_TextWidth (tagPtr->tkfont, value, strlen(value));
	bbox.lbearing = 0;
	ascent = fm.ascent;
	descent = fm.descent;
#else
	XTextExtents (tagPtr->fontPtr, value, strlen(value),
		      &direction, &ascent, &descent, &bbox);
#endif
#endif

	/* 
	  Set the origin coordinates of the string to draw
	  using the anchor point.
	  origin represents the (x,y) coordinate of the
	  lower left corner of the text box, relative to
	  the internal (ie inside the border) window
	 */

	/* set the X origin first */
	switch (tagPtr->anchor) {
	case TK_ANCHOR_NE:
	case TK_ANCHOR_E:
	case TK_ANCHOR_SE:
	  originX = width - bbox.rbearing - 2 * bd;
	  break;
	case TK_ANCHOR_N:
	case TK_ANCHOR_S:
	case TK_ANCHOR_CENTER:
	  originX = (width - (bbox.rbearing - bbox.lbearing)) / 2 - bd;
	  break;
	default:
	  originX = -bbox.lbearing;
	}

	/* then set the Y origin */
	switch (tagPtr->anchor) {
	case TK_ANCHOR_N:
	case TK_ANCHOR_NE:
	case TK_ANCHOR_NW:
	  originY = ascent + descent;
	  break;
	case TK_ANCHOR_W:
	case TK_ANCHOR_E:
	case TK_ANCHOR_CENTER:
	  originY = height - (height - (ascent + descent)) / 2 - bd;
	  break;
	default:
	  originY = height - 2 * bd;
	}

	/*
	  if this is the selected cell and we are editing
	  ensure that the cursor will be displayed
	 */
	if (activeCell) {
#ifdef KANJI
          TkWSTextExtents(copyGc, value,
                          min(Tcl_WStrlen(value),tablePtr->textCurPosn),
                          &ascent, &descent, &bbox2);
#else
#if (TK_MAJOR_VERSION > 4)
	  bbox2.width = Tk_TextWidth(tagPtr->tkfont, value,
				     min(strlen(value),tablePtr->textCurPosn));
#else
	  XTextExtents (tagPtr->fontPtr, value,
			min (strlen (value), tablePtr->textCurPosn),
			&direction, &ascent, &descent, &bbox2);
#endif		/* VERSION 8 Font Change */
#endif
	  originX = max (3 - bbox2.width,
			 min (originX, width - bbox2.width - 2 * bd - 3));
	}
	/* 
	  only if I am desperate will I use a clip rectangle
	  as it means updating the GC in the server which slows
	  everything down.
	 */
	if ((clipRectSet = ((originX + bbox.lbearing < -bd)
			    || (originX + bbox.rbearing > width - bd)
			    || (ascent + descent > height)))) {
	  /* set the clipping rectangle */
	  clipRect.x = x - offsetX;
	  clipRect.y = y - offsetY;
	  clipRect.width = width;
	  clipRect.height = height;
#ifdef KANJI
          /* For Ascii font */
          XSetClipRectangles(display, copyGc->fe[0].gc, 0, 0,
			     &clipRect, 1, Unsorted);
          /* For Kanji font */
          XSetClipRectangles(display, copyGc->fe[1].gc, 0, 0,
			     &clipRect, 1, Unsorted);
#else
	  XSetClipRectangles (display, copyGc, 0, 0, &clipRect, 1, Unsorted);
#endif
	}
#ifdef KANJI
        TkWSDrawString(display, window, copyGc,
		       x-offsetX+originX+bd, y-offsetY+originY-descent+bd,
		       value, Tcl_WStrlen(value));
#else
#if (TK_MAJOR_VERSION > 4)
	Tk_DrawChars(display, window, copyGc, tablePtr->defaultTag.tkfont,
		     value, strlen(value),
		     x-offsetX+originX+bd, y-offsetY+originY-descent+bd);
#else
	XDrawString (display, window, copyGc,
		     x-offsetX+originX+bd, y-offsetY+originY-descent+bd,
		     value, strlen (value));
#endif
#endif

	/* if this is the selected cell draw the cursor if it's on
	   and will fit in the box */
	if (activeCell && (tablePtr->flags & TBL_CURSOR_ON) &&
	    (width > originX+bd+bbox2.width)) {
	  if ((y-offsetY) < (y-offsetY+originY-descent-ascent+bd))
	    Tk_Fill3DRectangle(tablePtr->tkwin, window, tablePtr->insertBg,
			       x-offsetX+originX+bd+bbox2.width,
			       y-offsetY+originY-descent-ascent+bd,
			       tablePtr->insertWidth, ascent+descent,
			       tablePtr->insertBorderWidth, TK_RELIEF_FLAT);
	  else
	    Tk_Fill3DRectangle(tablePtr->tkwin, window, tablePtr->insertBg,
			       x-offsetX+originX+bd+bbox2.width,
			       y-offsetY, tablePtr->insertWidth, height,
			       tablePtr->insertBorderWidth, TK_RELIEF_FLAT);
	}
	/* reset the clip mask */
	if (clipRectSet) {
#ifdef KANJI
          /* For Ascii font */
          XSetClipMask(display, copyGc->fe[0].gc, None);
          /* For Kanji font */
          XSetClipMask(display, copyGc->fe[1].gc, None);
#else
	  XSetClipMask(display, copyGc, None);
#endif
        }
      }
      /* Draw the 3d border on the pixmap correctly offset */
      /* if (((Border *)(tagPtr->bgBorder))->lightGC == None ) {} */
      lightGc = Tk_3DBorderGC(tablePtr->tkwin,tagPtr->bgBorder,TK_3D_LIGHT_GC);
      darkGc  = Tk_3DBorderGC(tablePtr->tkwin,tagPtr->bgBorder,TK_3D_DARK_GC);

      switch (tablePtr->drawMode) {
      case DRAW_MODE_SLOW:
      case DRAW_MODE_TK_COMPAT:
	Tk_Draw3DRectangle (tablePtr->tkwin, window, tagPtr->bgBorder,
			    x - offsetX, y - offsetY, width, height,
			    tablePtr->borderWidth, tagPtr->relief);
	break;
      case DRAW_MODE_FAST:
        /*
        ** choose the GCs to get the best approximation
        ** to the desired drawing style
        */
#if 0
        switch(tagPtr->relief) {
        case TK_RELIEF_FLAT:
          topGc=bottomGc=Tk_3DBorderGC(tablePtr->tkwin,tagPtr->bgBorder,
				       TK_3D_FLAT_GC);
          break;
        case TK_RELIEF_RAISED:
        case TK_RELIEF_RIDGE:
          topGc=Tk_3DBorderGC(tablePtr->tkwin,tagPtr->bgBorder,TK_3D_LIGHT_GC);
          bottomGc=Tk_3DBorderGC(tablePtr->tkwin,tagPtr->bgBorder,
				 TK_3D_DARK_GC);
          break;
        default: /* TK_RELIEF_SUNKEN TK_RELIEF_GROOVE */
          bottomGc=Tk_3DBorderGC(tablePtr->tkwin,tagPtr->bgBorder,
				 TK_3D_LIGHT_GC);
          topGc=Tk_3DBorderGC(tablePtr->tkwin,tagPtr->bgBorder,TK_3D_DARK_GC);
          break;
        }
#else
        switch(tagPtr->relief) {
        case TK_RELIEF_FLAT:
          topGc=bottomGc=copyGc;
          break;
        case TK_RELIEF_RAISED:
        case TK_RELIEF_RIDGE:
          topGc=lightGc;
          bottomGc=darkGc;
          break;
        default: /* TK_RELIEF_SUNKEN TK_RELIEF_GROOVE */
          bottomGc=lightGc;
          topGc=darkGc;
          break;
        }
#endif
	
	/* draw a line with no width */
	rect[0].x = x;
	rect[0].y = y + height - 1;
	rect[1].y = -height + 1;
	rect[2].x = width - 1;
	rect[3].x = x + width - 1;
	rect[3].y = y;
	rect[4].y = height - 1;
	rect[5].x = -width + 1;
	XDrawLines (display, window, bottomGc, rect+3, 3, CoordModePrevious);
	XDrawLines (display, window, topGc, rect, 3, CoordModePrevious);
      }

      /* delete the tag structure */
      ckfree ((char *) (tagPtr));
      TableFreeGc(tablePtr,copyGc);
    }
  }

#ifdef NO_TOP_LEFT
  /* clear the top left corner */
  if (tablePtr->titleRows != 0 || tablePtr->titleCols != 0)
    Tk_Fill3DRectangle (tablePtr->tkwin, window, titlePtr->bgBorder,
			0 - offsetX + tablePtr->highlightWidth,
			0 - offsetY + tablePtr->highlightWidth,
			(tablePtr->colStarts)[tablePtr->titleCols],
			(tablePtr->rowStarts)[tablePtr->titleRows],
	 (tablePtr->drawMode == DRAW_MODE_FAST) ? 1 : tablePtr->borderWidth,
			titlePtr->relief);
#endif

  /* 
    if we have got to the end of the table, 
    clear the area after the last row/column 
   */
  TableCellCoords (tablePtr, tablePtr->rows, tablePtr->cols,
		   &x, &y, &width, &height);
  if (x + width < invalidX + invalidWidth - 1)
    if (tablePtr->drawMode == DRAW_MODE_SLOW)
      /* we are using a pixmap, so use TK to clear it */
      Tk_Fill3DRectangle (tablePtr->tkwin, window,
			  tablePtr->defaultTag.bgBorder, x+width-offsetX, 0,
			  invalidX+invalidWidth-x-width, invalidHeight,
			  0, TK_RELIEF_FLAT);
    else
      XClearArea (display, window, x + width, invalidY,
		  invalidX + invalidWidth - x - width, invalidHeight, False);

  if (y + height < invalidY + invalidHeight - 1)
    if (tablePtr->drawMode == DRAW_MODE_SLOW)
      /* we are using a pixmap, so use TK to clear it */
      Tk_Fill3DRectangle (tablePtr->tkwin, window,
			  tablePtr->defaultTag.bgBorder, 0, y+height-offsetY,
			  invalidWidth, invalidY+invalidHeight-y-height,
			  0, TK_RELIEF_FLAT);
    else
      XClearArea (display, window, invalidX, y + height, invalidWidth,
		  invalidY + invalidHeight - y - height, False);

  /* copy on and delete the pixmap if we are in slow mode */
  if (tablePtr->drawMode == DRAW_MODE_SLOW) {
    /* Get a default GC */
    GC tmpGc = TableGetGc (tablePtr, &(tablePtr->defaultTag));

#ifdef KANJI
    XCopyArea(display, window, Tk_WindowId(tkwin),
              tmpGc->fe[0].gc, 0, 0,
              invalidWidth, invalidHeight,
              invalidX, invalidY);
    XCopyArea(display, window, Tk_WindowId(tkwin),
              tmpGc->fe[1].gc, 0, 0,
              invalidWidth, invalidHeight,
              invalidX, invalidY);
#if 0
    if (value)Tk_FreeWStr (value); /* jf 2/28/96 */
#endif
#else
    XCopyArea (display, window, Tk_WindowId (tkwin),
	       tmpGc, 0, 0,
	       invalidWidth, invalidHeight,
	       invalidX, invalidY);
#endif /* KANJI */
    XFreePixmap (Tk_Display (tkwin), window);
    TableFreeGc(tablePtr,tmpGc);
  }
  if ((tablePtr->flags & TBL_REDRAW_BORDER) && tablePtr->highlightWidth != 0) {
    GC gc = Tk_GCForColor ((tablePtr->flags & TBL_HAS_FOCUS)
			   ? (tablePtr->highlightColorPtr)
		    : (tablePtr->highlightBgColorPtr), Tk_WindowId (tkwin));
    tablePtr->flags &= ~TBL_REDRAW_BORDER;
    Tk_DrawFocusHighlight (tkwin, gc, tablePtr->highlightWidth,
			   Tk_WindowId (tkwin));
  }
  /* 
    If rowTagCmd or colTagCmd were specified then free up
    the hash tables that were used to cache evaluations.
   */
  if (tablePtr->rowTagCmd) {
    Tcl_DeleteHashTable (rowProcTbl);
    ckfree ((char *) (rowProcTbl));
  }
  if (tablePtr->colTagCmd) {
    Tcl_DeleteHashTable (colProcTbl);
    ckfree ((char *) (colProcTbl));
  }
}

/* 
 * Invalidates a rectangle and adds it to the total invalid rectangle
 * waiting to be redrawn if the force flag is set, does an update
 * instantly else waits until TK is idle
 */

#define TableInvalidateAll(tablePtr,force)  \
  TableInvalidate (tablePtr, 0, 0, Tk_Width (tablePtr->tkwin), \
    Tk_Height (tablePtr->tkwin),force )

static void TableInvalidate (Table * tablePtr, int x, int y,
			     int width, int height, int force)
{
  int hl;

  /* avoid allocating 0 sized pixmaps which would be fatal */
  if (width <= 0 || height <= 0) return;

  /* If not even mapped, ignore */
  if (!Tk_IsMapped (tablePtr->tkwin)) return;

  /* is this rectangle even on the screen */
  if ((x > Tk_Width (tablePtr->tkwin)) || (y > Tk_Height (tablePtr->tkwin)))
    return;

  /* if no pending updates then replace the rectangle, otherwise find the
    bounding rectangle */

  hl=tablePtr->highlightWidth;
  if(x<hl || y<hl || x>Tk_Width(tablePtr->tkwin)-hl ||
     y>Tk_Height(tablePtr->tkwin)-hl) 
    tablePtr->flags |= TBL_REDRAW_BORDER;

  if (!(tablePtr->flags & TBL_REDRAW_PENDING)) {
    tablePtr->invalidX = x;
    tablePtr->invalidY = y;
    tablePtr->invalidWidth = width;
    tablePtr->invalidHeight = height;
    if (force && !(tablePtr->batchMode))
      TableDisplay ((ClientData) tablePtr);
    else {
      tablePtr->flags |= TBL_REDRAW_PENDING;
      Tcl_DoWhenIdle (TableDisplay, (ClientData) tablePtr);
    }
  } else {
    tablePtr->invalidWidth = max (tablePtr->invalidX+tablePtr->invalidWidth,
				  x + width);
    tablePtr->invalidHeight = max (tablePtr->invalidY+tablePtr->invalidHeight,
				   y + height);
    tablePtr->invalidX = min (tablePtr->invalidX, x);
    tablePtr->invalidY = min (tablePtr->invalidY, y);
    tablePtr->invalidWidth -= tablePtr->invalidX;
    tablePtr->invalidHeight -= tablePtr->invalidY;
    /* are we forcing this update out */
    if (force && !(tablePtr->batchMode)) {
      Tcl_CancelIdleCall (TableDisplay, (ClientData) tablePtr);
      TableDisplay ((ClientData) tablePtr);
    }
  }
}

/* 
  called when the flash timer goes off decrements all the entries in the hash 
  table and invalidates any cells that expire, deleting them from the table
  if the table is now empty, stops the timer else reenables it 
 */
static void TableFlashEvent (ClientData clientdata)
{
  Table *tablePtr = (Table *) clientdata;
  Tcl_HashEntry *entryPtr;
  Tcl_HashSearch search;
  int entries;
  int count;

  entries = 0;
  for (entryPtr = Tcl_FirstHashEntry (tablePtr->flashCells, &search);
       entryPtr != NULL;
       entryPtr = Tcl_NextHashEntry (&search)) {
    count = (int) Tcl_GetHashValue (entryPtr);
    if (--count <= 0) {
      int row, col, x, y, width, height;

      /* get the cell address and invalidate that region only */
      if (TableParseArrayIndex(tablePtr, &row, &col, Tcl_GetHashKey (tablePtr->flashCells, entryPtr)) != 2)
	return;

      row -= tablePtr->rowOffset;
      col -= tablePtr->colOffset;

      /* delete the entry from the table */
      Tcl_DeleteHashEntry (entryPtr);

      TableCellCoords (tablePtr, row, col, &x, &y, &width, &height);
      TableInvalidate (tablePtr, x, y, width, height, 1);

    } else {
      Tcl_SetHashValue (entryPtr, (ClientData) count);
      entries++;
    }
  }

  /* do I need to restart the timer */
  if (entries)
    tablePtr->flashTimer = Tcl_CreateTimerHandler (250, TableFlashEvent,
						   (ClientData) tablePtr);
  else
    tablePtr->flashTimer = 0;
}

/*
  adds a flash to the list with the default timeout.
  Only adds it if flashing is enabled and flashtime > 0
 */
static void TableAddFlash (Table * tablePtr, int row, int col)
{
  char buf[100];
  int dummy;
  Tcl_HashEntry *entryPtr;

  if (!tablePtr->flashMode || tablePtr->flashTime < 1)
    return;

  /* create the array index */
  TableMakeArrayIndex (tablePtr, row + tablePtr->rowOffset,
		       col + tablePtr->colOffset, buf);

  /* add the flash to the hash table */
  entryPtr = Tcl_CreateHashEntry (tablePtr->flashCells, buf, &dummy);
  Tcl_SetHashValue (entryPtr, tablePtr->flashTime);

  /* now set the timer if it's not already going and invalidate the area */
  if (tablePtr->flashTimer == NULL)
    tablePtr->flashTimer = Tcl_CreateTimerHandler (250, TableFlashEvent,
						   (ClientData) tablePtr);
}

static char *TableVarProc(ClientData clientdata, Tcl_Interp * interp,
			  char *name, char *index, int flags)
{
  Table *tablePtr = (Table *) clientdata;
  int x, y, width, height, i, j;
  /* is this the whole var being destroyed or just one cell being deleted */
  if ((flags & TCL_TRACE_UNSETS) && index == NULL) {
    /* if this isn't the interpreter being destroyed reinstate the trace */
    if ((flags & TCL_TRACE_DESTROYED) && !(flags & TCL_INTERP_DESTROYED)) {
      Tcl_SetVar2 (interp, tablePtr->arrayVar, "XXXXX", "", TCL_GLOBAL_ONLY);
      Tcl_UnsetVar2 (interp, tablePtr->arrayVar, "XXXXX", TCL_GLOBAL_ONLY);
      Tcl_ResetResult (interp);

      /* clear the selection buffer */
      TableGetActiveBuf (tablePtr);

      /* set a trace on the variable */
      Tcl_TraceVar (interp, tablePtr->arrayVar,
		    TCL_TRACE_WRITES | TCL_TRACE_UNSETS | TCL_GLOBAL_ONLY,
		    (Tcl_VarTraceProc *)TableVarProc, (ClientData) tablePtr);

      /* and invalidate the table */
      TableInvalidate (tablePtr, 0, 0, Tk_Width (tablePtr->tkwin),
		       Tk_Height (tablePtr->tkwin), 0);
    }
    return (char *) NULL;
  }
  /* get the cell address and invalidate that region only.
     * Make sure that it is a valid cell address. */
  if (TableParseArrayIndex (tablePtr, &i, &j, index) != 2)
    return (char *) NULL;

  i -= tablePtr->rowOffset;
  j -= tablePtr->colOffset;

  /* did the selected cell just update */
  /* ACTIVE */
  if (i == tablePtr->activeRow && j == tablePtr->activeCol)
    TableGetActiveBuf (tablePtr);

  /* Flash the cell */
  TableAddFlash (tablePtr, i, j);

  TableCellCoords (tablePtr, i, j, &x, &y, &width, &height);
  TableInvalidate (tablePtr, x, y, width, height, 1);
  return (char *) NULL;
}

/*
 * Calculate the row and column starts.  Adjusts the topleft corner variable
 * to keep it within the screen range, out of the titles and keep the screen
 * full make sure the selected cell is in the visible area checks to see if
 * the top left cell has changed at all and invalidates the table if it has
 *
 * Side Effects:
 *	Resets Result string (*tagCmd can mess it up)
 */
static void TableAdjustParams (Table * tablePtr) {
  int botRightCol, botRightRow, topRow, leftCol, total;
  int i, x, y, width, height;
  int diff, colPad, unpresetCols, lastUnpresetCol, lastColPad, rowPad,
    unpresetRows, lastUnpresetRow, lastRowPad, setRowPixels, setColPixels;
  Tcl_HashEntry *entryPtr;
  Tcl_HashSearch search;
  int bd, recalc = 0;

  do {
    lastUnpresetCol = 0;
    lastUnpresetRow = 0;
    /* make sure the top row and col have reasonable values */
    tablePtr->topRow = topRow =
      max(tablePtr->titleRows, min(tablePtr->topRow, tablePtr->rows-1));
    tablePtr->leftCol = leftCol =
      max(tablePtr->titleCols, min(tablePtr->leftCol,tablePtr->cols-1));
    /* make sure the selected cell has a reasonable value */
    /* ACTIVE */
    tablePtr->activeRow = max(tablePtr->titleRows,
			      min(tablePtr->activeRow, tablePtr->rows-1));
    tablePtr->activeCol = max(tablePtr->titleCols,
			      min(tablePtr->activeCol, tablePtr->cols-1));
    /* Set up the arrays to hold the row/col pixels and starts */
    if (tablePtr->rowPixels) ckfree ((char *) (tablePtr->rowPixels));
    tablePtr->rowPixels = (int *) ckalloc (tablePtr->rows * sizeof (int));
    if (tablePtr->rowStarts) ckfree ((char *) (tablePtr->rowStarts));
    tablePtr->rowStarts = (int *) ckalloc ((tablePtr->rows+1)* sizeof (int));
    if (tablePtr->colPixels) ckfree ((char *) (tablePtr->colPixels));
    tablePtr->colPixels = (int *) ckalloc (tablePtr->cols * sizeof (int));
    if (tablePtr->colStarts) ckfree ((char *) (tablePtr->colStarts));
    tablePtr->colStarts = (int *) ckalloc ((tablePtr->cols+1) * sizeof (int));
    /* get the borderwidth for several upcoming calculations */
    bd = tablePtr->borderWidth;
    /* get all the preset columns and set their widths */
    setColPixels = 0;
    unpresetCols = 0;
    total = 0;
    for (i = 0; i < tablePtr->cols; i++) {
      if ((entryPtr = Tcl_FindHashEntry (tablePtr->colWidths, (char *) i)) == NULL) {
	(tablePtr->colPixels)[i] = -1;
	unpresetCols++;
	lastUnpresetCol = i;
      } else {
	(tablePtr->colPixels)[i] = (int) Tcl_GetHashValue (entryPtr) *
	  (tablePtr->charWidth) + 2 * bd;
	setColPixels += (tablePtr->colPixels)[i];
      }
    }
    /* get all the preset rows and set their heights */
    setRowPixels = 0;
    unpresetRows = 0;
    for (i = 0; i < tablePtr->rows; i++) {
      if ((entryPtr = Tcl_FindHashEntry (tablePtr->rowHeights, (char *) i)) == NULL) {
	(tablePtr->rowPixels)[i] = -1;
	unpresetRows++;
	lastUnpresetRow = i;
      } else {
	(tablePtr->rowPixels)[i] = (int) Tcl_GetHashValue (entryPtr) + 2 * bd;
	setRowPixels += (tablePtr->rowPixels)[i];
      }
    }

    /* work out how much to pad each column and how much to pad the last
       column, depending on the mode */
    /*!Tk_IsMapped(tablePtr->tkwin) || */
    diff = Tk_Width(tablePtr->tkwin)-setColPixels-2*tablePtr->highlightWidth
      -unpresetCols*((tablePtr->charWidth)*(tablePtr->defColWidth)+2*bd);
    switch(tablePtr->colStretch) {
     case STRETCH_MODE_NONE:
      colPad = 0;
      lastColPad = 0;
      break;
     case STRETCH_MODE_UNSET:
      if(unpresetCols == 0) {
         colPad = 0;
         lastColPad = 0;
      } else {
        colPad = max(0,diff) / unpresetCols;
        lastColPad = max(0,diff) - colPad * (unpresetCols - 1);
      }
      break;
     case STRETCH_MODE_LAST:
      lastUnpresetCol = tablePtr->cols - 1;
      colPad = 0;
      lastColPad = max(0,diff);
      break;
     default:
      colPad = max(0,diff) / (tablePtr->cols);
      lastColPad = max(0,diff) - colPad * (tablePtr->cols - 1);
      /* force it to be applied to the last column too */
      lastUnpresetCol = tablePtr->cols - 1;
    }
      
    /* now do the padding and calculate the column starts */
    total = 0;
    for (i = 0; i < tablePtr->cols; i++) {
      if ((tablePtr->colPixels)[i] == -1)
	(tablePtr->colPixels)[i] = (tablePtr->charWidth) *
	  (tablePtr->defColWidth) + 2 * bd +
	  ((i != lastUnpresetCol) ? colPad : lastColPad);
      else if (tablePtr->colStretch == STRETCH_MODE_ALL)
	(tablePtr->colPixels)[i] += (i != lastUnpresetCol) ?
	  colPad : lastColPad;
      total = (((tablePtr->colStarts)[i] = total) + (tablePtr->colPixels)[i]);
    }
    (tablePtr->colStarts)[i] = tablePtr->maxWidth = total;

    /* work out how much to pad each row and how much to pad the last row,
       depending on the mode */
    diff = Tk_Height(tablePtr->tkwin)-setRowPixels-2*tablePtr->highlightWidth
      -unpresetRows*(tablePtr->defRowHeight+2*bd);
    switch(tablePtr->rowStretch) {
      case STRETCH_MODE_NONE:
        rowPad = 0;
        lastRowPad = 0;
        break;
      case STRETCH_MODE_UNSET:
        if(unpresetRows == 0)  {
          rowPad = 0;
          lastRowPad = 0;
        } else {
          rowPad = max(0,diff) / unpresetRows;
          lastRowPad = max(0,diff) - rowPad * (unpresetRows - 1);
        }
        break;
      case STRETCH_MODE_LAST:
        rowPad = 0;
        lastRowPad = max(0,diff);
      /* force it to be applied to the last column too */
        lastUnpresetRow = tablePtr->rows - 1;
        break;
      case STRETCH_MODE_FILL:
        rowPad = 0;
        if(diff>0 && !recalc) {
	  lastRowPad = diff;
	  tablePtr->rows+=(diff/(tablePtr->defRowHeight + 2*bd));
	  lastUnpresetRow = tablePtr->rows - 1;
	  recalc=1;
	  continue;
        } else if(diff<0 && !recalc) {
	  lastRowPad = diff;
	  tablePtr->rows+=(diff/(tablePtr->defRowHeight + 2*bd));
	  if(tablePtr->rows<0) tablePtr->rows=0;
	  lastUnpresetRow = tablePtr->rows - 1;
	  recalc=1;
	  continue;
        } else {
	  lastRowPad = diff;
	  lastUnpresetRow = tablePtr->rows - 1;
	  recalc=0;
        }
        break;
     default: 
      rowPad = max(0,diff) / (tablePtr->rows);
      lastRowPad = max(0,diff) - rowPad * (tablePtr->rows - 1);
      /* force it to be applied to the last column too */
      lastUnpresetRow = tablePtr->rows - 1;
    }

    /* now do the padding and calculate the row starts */
    total = 0;
    for (i = 0; i < tablePtr->rows; i++) {
      if ((tablePtr->rowPixels)[i] == -1)
	(tablePtr->rowPixels)[i] = (tablePtr->defRowHeight) +
	  2 * bd + ((i != lastUnpresetRow) ? rowPad : lastRowPad);
      else if (tablePtr->colStretch == STRETCH_MODE_ALL)
	(tablePtr->colPixels)[i] += (i != lastUnpresetRow) ?
	  rowPad : lastRowPad;

      /* calculate the start of each row */
      total = (((tablePtr->rowStarts)[i] = total) + (tablePtr->rowPixels)[i]);
    }
    (tablePtr->rowStarts)[i] = tablePtr->maxHeight = total;
  } while (recalc);

  /* If we dont have the info, dont bother to fix up the other parameters */
  if (Tk_WindowId (tablePtr->tkwin) == None) {
    tablePtr->oldTopRow = tablePtr->oldLeftCol = -1;
    return;
  }
  /* make sure the top row and col have  reasonable values */
  tablePtr->topRow = topRow =
    max (tablePtr->titleRows, min (tablePtr->topRow, tablePtr->rows-1));
  tablePtr->leftCol = leftCol =
    max (tablePtr->titleCols, min (tablePtr->leftCol, tablePtr->cols-1));

  /* make sure the selected cell has a reasonable value */
  /* ACTIVE */
  tablePtr->activeRow = max (tablePtr->titleRows,
			     min (tablePtr->activeRow, tablePtr->rows-1));
  tablePtr->activeCol = max (tablePtr->titleCols,
			     min (tablePtr->activeCol, tablePtr->cols-1));

  /* 
    If we use this value of topRow, will we fill the window 
    if not, decrease it until we will, or until it gets to titleRows 
    make sure we dont cut off the bottom row
   */
  for (; topRow > tablePtr->titleRows; topRow--)
    if ((tablePtr->maxHeight - ((tablePtr->rowStarts)[topRow - 1] -
				(tablePtr->rowStarts)[tablePtr->titleRows])) >
	Tk_Height (tablePtr->tkwin))
      break;
  /* 
    If we use this value of topCol, will we fill the window 
    if not, decrease it until we will, or until it gets to titleCols 
    make sure we dont cut off the left column
   */
  for (; leftCol > tablePtr->titleCols; leftCol--)
    if ((tablePtr->maxWidth - ((tablePtr->colStarts)[leftCol - 1] -
			       (tablePtr->colStarts)[tablePtr->titleCols])) >
	Tk_Width (tablePtr->tkwin))
      break;

  tablePtr->topRow = topRow;
  tablePtr->leftCol = leftCol;

  /* 
    Do we have scrollbars, if so, calculate and call the TCL functions
    In order to get the scrollbar to be completely
    full when the whole screen is shown and there
    are titles, we have to arrange for the scrollbar
    range to be 0 -> rows-titleRows etc. This leads
    to the position setting methods, toprow and leftcol,
    being relative to the titles, not absolute row and
    column numbers.
   */
  if (tablePtr->yScrollCmd != NULL || tablePtr->xScrollCmd != NULL) {
    /* Now work out where the bottom right for scrollbar update */
    TableWhatCell (tablePtr, Tk_Width (tablePtr->tkwin) - 1,
		   Tk_Height (tablePtr->tkwin) - 1,
		   &botRightRow, &botRightCol);

    /* Do we have a Y-scrollbar */
    if (tablePtr->yScrollCmd != NULL) {
      char buffer[250];
      sprintf (buffer, " %d %d %d %d", tablePtr->rows - tablePtr->titleRows,
	       max (1, botRightRow - topRow + 1),
	       topRow - tablePtr->titleRows,
	       max (topRow - tablePtr->titleRows + 1,
		    botRightRow - tablePtr->titleRows));
      if (Tcl_VarEval (tablePtr->interp,
		   tablePtr->yScrollCmd, buffer, (char *) NULL) != TCL_OK) {
	Tcl_AddErrorInfo (tablePtr->interp,
		     "\n(error executing yscroll command in table widget)");
	Tcl_BackgroundError (tablePtr->interp);
      }
    }
    /* Do we have a X-scrollbar */
    if (tablePtr->xScrollCmd != NULL) {
      char buffer[250];
      sprintf (buffer, " %d %d %d %d", tablePtr->cols - tablePtr->titleCols,
	       max (1, botRightCol - leftCol + 1),
	       leftCol - tablePtr->titleCols,
	       max (leftCol - tablePtr->titleCols + 1,
		    botRightCol - tablePtr->titleCols));
      if (Tcl_VarEval (tablePtr->interp, tablePtr->xScrollCmd,
		       buffer, (char *) NULL) != TCL_OK) {
	Tcl_AddErrorInfo (tablePtr->interp,
		     "\n(error executing xscroll command in table widget)");
	Tcl_BackgroundError (tablePtr->interp);
      }
    }
  }

  /* now check the new value of topRow and top column against the originals,
    did they even change. If they did, invalidate the area, if not, leave it
    alone */

  /* did the selected cell move then */
  /* ACTIVE */
  if (tablePtr->selectionOn && (tablePtr->oldActRow != tablePtr->activeRow ||
				tablePtr->oldActCol != tablePtr->activeCol)) {
    int x, y, width, height;
    char buf[100];

    /* put the value back in the cell */
    if (tablePtr->arrayVar != NULL && tablePtr->state == tkNormalUid) {
      char *tempvar;
#ifdef KANJI
      wchar *wstr;
#endif

      /* 
        if the value has not changed, do nothing
	else we will set off the 'flash' timer
       */

      TableMakeArrayIndex (tablePtr, tablePtr->oldActRow+tablePtr->rowOffset,
			   tablePtr->oldActCol + tablePtr->colOffset, buf);
      tempvar = Tcl_GetVar2 (tablePtr->interp, tablePtr->arrayVar, buf,
			     TCL_GLOBAL_ONLY);
      /* 
        if they are different, replace the value the first check is:
	if the old value was null but the new buffer is not empty, replace,
	otherwise leave the value uninitialized
       */
#ifdef KANJI
      if (!tempvar && *(tablePtr->activeBuf)!=(wchar)'\0')
        Tcl_SetVar2(tablePtr->interp, tablePtr->arrayVar, buf,
		    Tk_DecodeWStr(tablePtr->activeBuf), TCL_GLOBAL_ONLY);
      if  ( tempvar  ) {
        wstr = Tk_GetWStr (tablePtr->interp,tempvar);
        if (Tcl_WStrcmp(wstr, tablePtr->activeBuf))
          Tcl_SetVar2(tablePtr->interp, tablePtr->arrayVar, buf,
		      Tk_DecodeWStr(tablePtr->activeBuf), TCL_GLOBAL_ONLY);
        Tk_FreeWStr (wstr);
      }
#else
      if ((!tempvar && *(tablePtr->activeBuf)!='\0') ||
         ( tempvar  && strcmp(tempvar, tablePtr->activeBuf)))
        Tcl_SetVar2(tablePtr->interp, tablePtr->arrayVar, buf,
		    tablePtr->activeBuf, TCL_GLOBAL_ONLY);
#endif              /* KANJI */
    }

    /* get the selection */
    TableGetActiveBuf (tablePtr);

    /* Invalidate the cells */
    TableCellCoords (tablePtr, tablePtr->oldActRow, tablePtr->oldActCol,
		     &x, &y, &width, &height);
    TableInvalidate (tablePtr, x, y, width, height, 1);
    TableCellCoords (tablePtr, tablePtr->activeRow, tablePtr->activeCol,
		     &x, &y, &width, &height);
    TableInvalidate (tablePtr, x, y, width, height, 1);

  }
  /* how about the rest of the table */
  if (tablePtr->topRow != tablePtr->oldTopRow ||
      tablePtr->leftCol != tablePtr->oldLeftCol) {
    TableInvalidate (tablePtr, 0, 0, Tk_Width (tablePtr->tkwin),
		     Tk_Height (tablePtr->tkwin), 0);
  }
  /* set the old top row & column for the next time this function is called */
  tablePtr->oldTopRow = tablePtr->topRow;
  tablePtr->oldLeftCol = tablePtr->leftCol;
  tablePtr->oldActRow = tablePtr->activeRow;
  tablePtr->oldActCol = tablePtr->activeCol;
}

/*
 * Toggle the cursor status 
 *
 * Equivalent to EntryBlinkProc
 */
static void TableCursorEvent (ClientData clientdata)
{
  Table *tablePtr = (Table *) clientdata;
  int x, y, width, height;

  if (!(tablePtr->flags & TBL_HAS_FOCUS) || (tablePtr->insertOffTime == 0)) {
    return;
  }
  if (tablePtr->cursorTimer != NULL)
    Tcl_DeleteTimerHandler (tablePtr->cursorTimer);
  if (tablePtr->flags & TBL_CURSOR_ON) {
    tablePtr->cursorTimer = Tcl_CreateTimerHandler(tablePtr->insertOffTime,
						   TableCursorEvent,
						   (ClientData) tablePtr);
  } else {
    tablePtr->cursorTimer = Tcl_CreateTimerHandler(tablePtr->insertOnTime,
						   TableCursorEvent,
						   (ClientData) tablePtr);
  }
  /* Toggle the cursor */
  tablePtr->flags ^= TBL_CURSOR_ON;

  /* invalidate the cell */
  /* ACTIVE */
  TableCellCoords (tablePtr, tablePtr->activeRow, tablePtr->activeCol,
		   &x, &y, &width, &height);
  TableInvalidate (tablePtr, x, y, width, height, 1);
}

/*
 * Configures the timer depending on the state of the table.
 *
 * Equivalent to EntryFocusProc
 */
static void TableConfigCursor (Table * tablePtr)
{
  int x, y, width, height;

  /* to get a cursor, we have to have focus, selection and edits allowed */
  if ((tablePtr->flags & TBL_HAS_FOCUS) && tablePtr->selectionOn) {
    /* turn the cursor on */
    tablePtr->flags |= TBL_CURSOR_ON;

    /* set up the first timer */
    if (tablePtr->insertOffTime != 0)
      tablePtr->cursorTimer = Tcl_CreateTimerHandler(tablePtr->insertOnTime,
						     TableCursorEvent,
						     (ClientData) tablePtr);
  } else {
    /* turn the cursor off */
    tablePtr->flags &= ~TBL_CURSOR_ON;

    /* and disable the timer */
    if (tablePtr->cursorTimer != NULL)
      Tcl_DeleteTimerHandler (tablePtr->cursorTimer);
    tablePtr->cursorTimer = NULL;
  }

  /* Invalidate the selection window to show or hide the cursor */
  /* ACTIVE */
  TableCellCoords (tablePtr, tablePtr->activeRow, tablePtr->activeCol,
		   &x, &y, &width, &height);
}

static int TableConfigure (Tcl_Interp * interp, Table * tablePtr,
			   int argc, char *argv[], int flags)
{
  char buf[200];
  Tcl_HashSearch search;
  Tcl_HashEntry *entryPtr;
  TagStruct *tagPtr;
  int dummy, x, y;
#if (TK_MAJOR_VERSION > 4)
  Tk_FontMetrics fm;
#endif

  /* remove the trace on the old array variable if there was one */
  if (tablePtr->arrayVar != NULL)
    Tcl_UntraceVar (interp, tablePtr->arrayVar,
		    TCL_TRACE_WRITES | TCL_TRACE_UNSETS | TCL_GLOBAL_ONLY,
		    (Tcl_VarTraceProc *)TableVarProc, (ClientData) tablePtr);
  /* Do the configuration */
  if (Tk_ConfigureWidget (interp, tablePtr->tkwin, TableConfig, argc, argv,
			  (char *) tablePtr, flags) != TCL_OK)
    return TCL_ERROR;
  /* Check the variable exists and trace it if it does */
  if (tablePtr->arrayVar != NULL) {
    /* Create a command for checking the array variable */
    sprintf (buf, "array size %s\n", tablePtr->arrayVar);
    /* does the variable exist? */
    if (Tcl_Eval (interp, buf) != TCL_OK) {
      /* if not, create it but leave it blank */
      Tcl_SetVar2 (interp, tablePtr->arrayVar, "XXXXX", "", TCL_GLOBAL_ONLY);
      Tcl_UnsetVar2 (interp, tablePtr->arrayVar, "XXXXX", TCL_GLOBAL_ONLY);
    }
    /* remove the effect of the evaluation */
    Tcl_ResetResult (interp);
    /* set a trace on the variable */
    Tcl_TraceVar (interp, tablePtr->arrayVar,
		  TCL_TRACE_WRITES | TCL_TRACE_UNSETS | TCL_GLOBAL_ONLY,
		  (Tcl_VarTraceProc *)TableVarProc, (ClientData) tablePtr);

    /*  get the current value of the selection */
    if (tablePtr->selectionOn)
      TableGetActiveBuf (tablePtr);
  }

  /* set up the default column width and row height */
#ifdef KANJI
  tablePtr->charWidth = XTextWidth(tablePtr->defaultTag.asciiFontPtr, "0", 1);
  if (tablePtr->defRowHeight==0)
    tablePtr->defRowHeight = (tablePtr->defaultTag.asciiFontPtr)->ascent + 
      (tablePtr->defaultTag.asciiFontPtr)->descent + 2;
#else
#if (TK_MAJOR_VERSION > 4)
  tablePtr->charWidth = Tk_TextWidth(tablePtr->defaultTag.tkfont, "0", 1);
  Tk_GetFontMetrics(tablePtr->defaultTag.tkfont, &fm);
  if (tablePtr->defRowHeight==0)
    tablePtr->defRowHeight = fm.linespace + 2;
#else
  tablePtr->charWidth = XTextWidth(tablePtr->defaultTag.fontPtr, "0", 1);
  if (tablePtr->defRowHeight==0)
    tablePtr->defRowHeight = (tablePtr->defaultTag.fontPtr)->ascent + 
      (tablePtr->defaultTag.fontPtr)->descent + 2;
#endif
#endif

  if (tablePtr->state != tkNormalUid && tablePtr->state != tkDisabledUid) {
    Tcl_AppendResult(interp, "bad state value \"", tablePtr->state,
		     "\": must be normal or disabled", (char *) NULL);
    tablePtr->state = tkNormalUid;
    return TCL_ERROR;
  }

  if (tablePtr->insertWidth <= 0) {
    tablePtr->insertWidth = 2;
  }
  if (tablePtr->insertBorderWidth > tablePtr->insertWidth/2) {
    tablePtr->insertBorderWidth = tablePtr->insertWidth/2;
  }

  /* 
    Calculate the row and column starts 
    Adjust the top left corner of the internal display 
  */
  TableAdjustParams (tablePtr);
  /* reset the cursor */
  TableConfigCursor ((ClientData) tablePtr);
  /* set up the background colour in the window */
  Tk_SetBackgroundFromBorder (tablePtr->tkwin, tablePtr->defaultTag.bgBorder);
  /* set the geometry and border */
  x = min (tablePtr->maxWidth, max (Tk_Width (tablePtr->tkwin),
				    tablePtr->maxReqWidth));
  y = min (tablePtr->maxHeight, max (Tk_Height (tablePtr->tkwin),
				     tablePtr->maxReqHeight));
  Tk_GeometryRequest (tablePtr->tkwin, x, y);
  Tk_SetInternalBorder (tablePtr->tkwin,
			tablePtr->borderWidth+tablePtr->highlightWidth);
  /* invalidate the whole table */
  TableInvalidateAll(tablePtr, 0);
  return TCL_OK;
}

/* this releases the resources used by a tag before it is freed up.  */

static void TableCleanupTag (Table * tablePtr, TagStruct * tagPtr)
{
  /* free the options in the widget */
  Tk_FreeOptions (tagConfig, (char *) tagPtr, tablePtr->display, 0);
}

/* 
  starts or stops the timer for the flashing cells 
  if stopping, clears out the flash structure
 */

static void TableFlashConfigure (Table * tablePtr, int startStop)
{
  if (startStop && tablePtr->flashMode) {
    if (tablePtr->flashTimer != NULL)
      Tcl_DeleteTimerHandler (tablePtr->flashTimer);
    tablePtr->flashTimer = Tcl_CreateTimerHandler (250, TableFlashEvent,
						   (ClientData) tablePtr);
  } else {
    if (tablePtr->flashTimer != NULL)
      Tcl_DeleteTimerHandler (tablePtr->flashTimer);
    tablePtr->flashTimer = NULL;

    /* clear out the flash hash table */
    if (tablePtr->flashCells != NULL)
      Tcl_DeleteHashTable (tablePtr->flashCells);

    /* set up a fresh blank one for the next time */
    Tcl_InitHashTable (tablePtr->flashCells, TCL_STRING_KEYS);

    /* WHOA, FIX? should this really clear the selCells? */
    /* clear out the select hash table */
    if (tablePtr->selCells != NULL)
      Tcl_DeleteHashTable(tablePtr->selCells);

    /* set up a fresh blank one for the next time */
    Tcl_InitHashTable (tablePtr->selCells, TCL_STRING_KEYS);
  }
}

/*
 *----------------------------------------------------------------------
 *
 * TableScanTo --
 *
 *      Given a point (presumably of the current mouse location)
 *      drag the view in the window to implement the scan operation.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      The view in the window may change.
 *
 *----------------------------------------------------------------------
 */
static void
TableScanTo(tablePtr, x, y)
     register Table *tablePtr;          /* Information about widget. */
     int x;                              /* X-coordinate to use for scan
					  * operation. */
     int y;                              /* Y-coordinate to use for scan
					  * operation. */
{
  int row, col;

  y += (5*(y-tablePtr->scanMarkY));
  x += (5*(x-tablePtr->scanMarkX));

  TableWhatCell(tablePtr, x, y, &row, &col);

  tablePtr->topRow  = row-tablePtr->scanMarkRowOffset;
  tablePtr->leftCol = col-tablePtr->scanMarkColOffset;

  /* Adjust the table for top left, selection on screen etc */
  TableAdjustParams(tablePtr);
  TableConfigCursor(tablePtr);
}

static int
Table_GetIndex(tablePtr,str,row_p,col_p)
     Table *tablePtr; 
     char *str;
     int *row_p;
     int *col_p;
{
  int r,c;
#if 0
  Tcl_HashEntry *entryPtr;
#endif

  if ( str[0] == '@' ) {				/* @x,y coordinate */ 
    int x,y;
    if ( sscanf(str+1, "%d,%d", &x,&y) !=2 ) {
      Tcl_AppendResult(tablePtr->interp,
		       "wrong format should be \"@x,y\"", (char *)NULL);
      return TCL_ERROR;
    }
    TableWhatCell(tablePtr, x, y, &r, &c);
    r += tablePtr->rowOffset;
    c += tablePtr->colOffset;
  } else if ( sscanf(str, "%d,%d", &r,&c) == 2 ) {
    if (!tablePtr->rowThenCol) {
      r = c;
      c = r;
    }
#if 0
  } else if ((entryPtr = Tcl_FindHashEntry(tablePtr->tagTable, str))!=NULL) {
    /* Would be nice to get tag indices */
    Tcl_HashSearch search;
    /* FIX, should get first value of tag index */
    r = 0; c = 0;
#endif
  } else if ( strncmp(str,"select",strlen(str)) == 0 ) {	/* select */
    if (tablePtr->selectionOn) {
      r = tablePtr->selRow+tablePtr->rowOffset;
      c = tablePtr->selCol+tablePtr->colOffset;
    } else {
      Tcl_AppendResult(tablePtr->interp, "no \"select\" cell in table", NULL);
      return TCL_ERROR;
    }
  } else if ( strncmp(str,"active",strlen(str)) == 0 ) {	/* active */
    if (tablePtr->flags & TBL_HAS_ACTIVE) {
      r = tablePtr->activeRow+tablePtr->rowOffset;
      c = tablePtr->activeCol+tablePtr->colOffset;
    } else {
      Tcl_AppendResult(tablePtr->interp, "no \"active\" cell in table", NULL);
      return TCL_ERROR;
    }
  } else if ( strncmp(str,"anchor",strlen(str)) == 0 ) {	/* anchor */
    if (tablePtr->flags & TBL_HAS_ANCHOR) {
      r = tablePtr->anchorRow+tablePtr->rowOffset;
      c = tablePtr->anchorCol+tablePtr->colOffset;
    } else {
      Tcl_AppendResult(tablePtr->interp, "no \"anchor\" cell in table", NULL);
      return TCL_ERROR;
    }
  } else if ( strncmp(str,"end",strlen(str)) == 0 ) {		/* end */
    r = tablePtr->rows-1+tablePtr->rowOffset;
    c = tablePtr->cols-1+tablePtr->colOffset;
  } else if ( strncmp(str,"origin",strlen(str)) == 0 ) {	/* origin */
    r = tablePtr->rowOffset+tablePtr->titleRows;
    c = tablePtr->colOffset+tablePtr->titleCols;
  } else if ( strncmp(str,"topleft",strlen(str)) == 0 ) {	/* topleft */
    r = tablePtr->topRow+tablePtr->rowOffset;
    c = tablePtr->leftCol+tablePtr->colOffset;
  } else {
    Tcl_AppendResult(tablePtr->interp, "bad table index \"",
		     str, "\"", (char *)NULL);
    return TCL_ERROR;
  }

  r = min(max(tablePtr->rowOffset,r),tablePtr->rows-1);
  c = min(max(tablePtr->colOffset,c),tablePtr->cols-1);
  if ( row_p ) *row_p = r;
  if ( col_p ) *col_p = c;
  return TCL_OK;
}

/* the tag commands are pretty big, so move them to their own routine */

static int TableTagCommand (Table * tablePtr, int argc, char *argv[])
{
  int result = TCL_OK, retval, i, newEntry, value;
  int row, col;
  int x, y, width, height;
  TagStruct *tagPtr;
  Tcl_HashEntry *entryPtr, *scanPtr, *newEntryPtr, *oldEntryPtr;
  Tcl_HashTable *hashTblPtr;
  Tcl_HashSearch search;
  char buf[150], *keybuf;

  /* parse the next argument */
  retval = parse_command (tablePtr->interp, tag_commands, argv[2]);
  switch (retval) {
    /* failed to parse the argument, error */
  case 0:
    return TCL_ERROR;

  case TAG_CELLTAG:
    /* tag a (group of) cell(s) */
    if (argc < 4) {
      Tcl_AppendResult(tablePtr->interp, "wrong # args: should be \"", argv[0],
		       " tag celltag tag ?arg arg ..?\"", (char *) NULL);
      return TCL_ERROR;
    }
    /* are we deleting */
    if (!(*argv[3]))
      tagPtr = NULL;
    else {
      /* check to see if the tag actually exists */
      if ((entryPtr = Tcl_FindHashEntry(tablePtr->tagTable, argv[3]))==NULL) {
	Tcl_AppendResult (tablePtr->interp, "invalid tag name \"", argv[3],
			  "\"", (char *) NULL);
	return TCL_ERROR;
      }
      /* get the pointer to the tag structure */
      tagPtr = (TagStruct *) Tcl_GetHashValue (entryPtr);
    }

    /* No more args -> display only */
    if (argc == 4) {
      for (scanPtr = Tcl_FirstHashEntry (tablePtr->cellStyles, &search);
	   scanPtr != NULL; scanPtr = Tcl_NextHashEntry (&search)) {
	/* is this the tag pointer for this cell */
	if ((TagStruct *) Tcl_GetHashValue (scanPtr) == tagPtr) {
	  keybuf = Tcl_GetHashKey (tablePtr->cellStyles, scanPtr);

	  /* Split the value into its two components */
	  if (TableParseArrayIndex (tablePtr, &row, &col, keybuf) != 2)
	    return TCL_ERROR;
	  sprintf (buf, "%d,%d", row, col);
	  Tcl_AppendElement(tablePtr->interp, buf);
	}
      }
      return TCL_OK;
    }
    /* Now loop through the arguments and fill in the hash table */
    for (i = 4; i < argc; i++) {
      /* can I parse this argument */
      if (Table_GetIndex(tablePtr, argv[i], &row, &col) != TCL_OK) {
	return TCL_ERROR;
      }
      /* get the hash key ready */
      TableMakeArrayIndex (tablePtr, row, col, buf);

      /* is this a deletion */
      if (tagPtr == NULL) {
	oldEntryPtr = Tcl_FindHashEntry (tablePtr->cellStyles, buf);
	if (oldEntryPtr != NULL)
	  Tcl_DeleteHashEntry (oldEntryPtr);
      } else {
	/* add a key to the hash table */
	newEntryPtr = Tcl_CreateHashEntry (tablePtr->cellStyles, buf,
					   &newEntry);

	/* and set it to point to the Tag structure */
	Tcl_SetHashValue (newEntryPtr, (ClientData) tagPtr);
      }
      /* now invalidate the area */
      TableCellCoords (tablePtr, row - tablePtr->rowOffset,
		       col - tablePtr->colOffset, &x, &y, &width, &height);
      TableInvalidate (tablePtr, x, y, width, height, 0);
    }
    return TCL_OK;

  case TAG_COLTAG:
  case TAG_ROWTAG:
    /* tag a row or a column */
    if (argc < 4) {
      Tcl_AppendResult (tablePtr->interp, "wrong # args: should be \"",
			argv[0], " tag ", (retval == TAG_ROWTAG) ? "rowtag" :
			"coltag", " tag ?arg arg ..?\"", (char *) NULL);
      return TCL_ERROR;
    }
    /* if the tag is null, we are deleting */
    if (!(*argv[3]))
      tagPtr = NULL;
    else {			/* check to see if the tag actually exists */
      if ((entryPtr = Tcl_FindHashEntry(tablePtr->tagTable, argv[3]))==NULL) {
	Tcl_AppendResult (tablePtr->interp, "invalid tag name \"", argv[3],
			  "\"", (char *) NULL);
	return TCL_ERROR;
      }
      /* get the pointer to the tag structure */
      tagPtr = (TagStruct *) Tcl_GetHashValue (entryPtr);
    }

    /* and choose the correct hash table */
    hashTblPtr = (retval == TAG_ROWTAG) ?
      tablePtr->rowStyles : tablePtr->colStyles;

    /* No more args -> display only */
    if (argc == 4) {
      for (scanPtr = Tcl_FirstHashEntry (hashTblPtr, &search);
	   scanPtr != NULL; scanPtr = Tcl_NextHashEntry (&search)) {
	/* is this the tag pointer on this row */
	if ((TagStruct *) Tcl_GetHashValue (scanPtr) == tagPtr) {
	  sprintf (buf, "%d", (int) Tcl_GetHashKey (hashTblPtr, scanPtr));
	  Tcl_AppendElement(tablePtr->interp, buf);
	}
      }
      return TCL_OK;
    }
    /* Now loop through the arguments and fill in the hash table */
    for (i = 4; i < argc; i++) {
      /* can I parse this argument */
      if (sscanf (argv[i], "%d", &value) != 1) {
	Tcl_AppendResult (tablePtr->interp, "bad value \"", argv[i],
			  "\"", (char *) NULL);
	return TCL_ERROR;
      }
      /* deleting or adding */
      if (tagPtr == NULL) {
	oldEntryPtr = Tcl_FindHashEntry (hashTblPtr, (char *) value);
	if (oldEntryPtr != NULL)
	  Tcl_DeleteHashEntry (oldEntryPtr);
      } else {
	/* add a key to the hash table */
	newEntryPtr = Tcl_CreateHashEntry (hashTblPtr, (char *) value,
					   &newEntry);

	/* and set it to point to the Tag structure */
	Tcl_SetHashValue (newEntryPtr, (ClientData) tagPtr);
      }
      /* and invalidate the row or column affected */
      if (retval == TAG_ROWTAG) {
	/* get the position of the leftmost cell in the row */
	TableCellCoords (tablePtr, value - tablePtr->rowOffset, 0,
			 &x, &y, &width, &height);

	/* Invalidate the row */
	TableInvalidate (tablePtr, 0, y, Tk_Width (tablePtr->tkwin),
			 height, 0);
      } else {
	/* get the position of the topmost cell on the column */
	TableCellCoords (tablePtr, 0, value - tablePtr->colOffset,
			 &x, &y, &width, &height);

	/* Invalidate the column */
	TableInvalidate (tablePtr, x, 0, width,
			 Tk_Height (tablePtr->tkwin), 0);
      }
    }
    return TCL_OK;	/* COLTAG && ROWTAG */

  case TAG_CGET:
    if (argc != 5) {
      Tcl_AppendResult(tablePtr->interp, "wrong # args: should be \"",
                       argv[0], " tag cget tagName option\"", (char *) NULL);
      return TCL_ERROR;
    }
    if ((entryPtr=Tcl_FindHashEntry(tablePtr->tagTable, argv[3])) == NULL) {
      Tcl_AppendResult(tablePtr->interp, "tag \"", argv[3],
		       "\" isn't defined in table widget", (char *) NULL);
      return TCL_ERROR;
    } else {
      tagPtr = (TagStruct *) Tcl_GetHashValue (entryPtr);
      result = Tk_ConfigureValue(tablePtr->interp, tablePtr->tkwin, tagConfig,
				 (char *) tagPtr, argv[4], 0);
    }
    return result;	/* CGET */

  case TAG_CONFIGURE:
    if (argc < 4) {
      Tcl_AppendResult (tablePtr->interp, "wrong # args: should be \"",
			argv[0], " tag configure tagName ?arg arg  ...?",
			(char *) NULL);
      return TCL_ERROR;
    }
    /* first see if this is a reconfiguration */
    entryPtr = Tcl_FindHashEntry (tablePtr->tagTable, argv[3]);
    if (entryPtr == NULL) {
      entryPtr = Tcl_CreateHashEntry (tablePtr->tagTable, argv[3], &newEntry);

      /* create the structure */
      tagPtr = TableNewTag (tablePtr);

      /* insert it into the table */
      Tcl_SetHashValue (entryPtr, (ClientData) tagPtr);

      /* configure the tag structure */
      result = Tk_ConfigureWidget(tablePtr->interp, tablePtr->tkwin, tagConfig,
				  argc - 4, argv + 4, (char *) tagPtr, 0);
      if (result == TCL_ERROR)
	return TCL_ERROR;
    } else {
      /* pointer wasn't null, do a reconfig if we have enough arguments */
      /* get the tag pointer from the table */
      tagPtr = (TagStruct *) Tcl_GetHashValue (entryPtr);

      /* 5 args means that there are values to replace */
      if (argc > 5) {
	/* and do a reconfigure */
	result = Tk_ConfigureWidget (tablePtr->interp, tablePtr->tkwin,
				     tagConfig, argc - 4, argv + 4,
				     (char *) tagPtr, TK_CONFIG_ARGV_ONLY);
	if (result == TCL_ERROR)
	  return TCL_ERROR;
      }
    }
    /* 
      If there were less than 6 args, we need
      to do a printout of the config, even for new tags
     */
    if (argc < 6) {
      result = Tk_ConfigureInfo (tablePtr->interp, tablePtr->tkwin,
				 tagConfig, (char *) tagPtr,
				 (argc == 5) ? argv[4] : 0, 0);
    } else {
      /* Otherwise we reconfigured so invalidate the table for a redraw */
      TableInvalidate (tablePtr, 0, 0, Tk_Width (tablePtr->tkwin),
		       Tk_Height (tablePtr->tkwin), 0);
    }
    return result;

  case TAG_DELETE:
    /* delete a tag */
    if (argc < 4) {
      Tcl_AppendResult (tablePtr->interp, "wrong # args: should be \"",
	       argv[0], " tag delete tagName ?tagName ...?\"", (char *) NULL);
      return TCL_ERROR;
    }
    /* run through the remaining arguments */
    for (i = 3; i < argc; i++) {
      result = TCL_OK;
      /* cannot delete the title tag */
      if (strcmp(argv[i], "title") == 0 || strcmp (argv[i], "sel") == 0 ||
	  strcmp(argv[i], "flash") == 0 || strcmp (argv[i], "active") == 0) {
	Tcl_AppendResult (tablePtr->interp, "cannot delete ", argv[i],
			  " tag", (char *) NULL);
	return TCL_ERROR;
      }
      if ((entryPtr = Tcl_FindHashEntry(tablePtr->tagTable, argv[i]))!=NULL) {
	/* get the tag pointer */
	tagPtr = (TagStruct *) Tcl_GetHashValue (entryPtr);

	/* delete all references to this tag in rows */
	scanPtr = Tcl_FirstHashEntry (tablePtr->rowStyles, &search);
	for (; scanPtr != NULL; scanPtr = Tcl_NextHashEntry (&search))
	  if (Tcl_GetHashValue (scanPtr) == tagPtr)
	    Tcl_DeleteHashEntry (scanPtr);

	/* delete all references to this tag in cols */
	scanPtr = Tcl_FirstHashEntry (tablePtr->colStyles, &search);
	for (; scanPtr != NULL; scanPtr = Tcl_NextHashEntry (&search))
	  if (Tcl_GetHashValue (scanPtr) == tagPtr)
	    Tcl_DeleteHashEntry (scanPtr);

	/* delete all references to this tag in cells */
	scanPtr = Tcl_FirstHashEntry (tablePtr->cellStyles, &search);
	for (; scanPtr != NULL; scanPtr = Tcl_NextHashEntry (&search))
	  if (Tcl_GetHashValue (scanPtr) == tagPtr)
	    Tcl_DeleteHashEntry (scanPtr);

	/* release the structure */
	TableCleanupTag (tablePtr, (TagStruct *) Tcl_GetHashValue (entryPtr));
	ckfree ((TagStruct *) Tcl_GetHashValue (entryPtr));

	/* and free the hash table entry */
	Tcl_DeleteHashEntry (entryPtr);
      }
    }
    /* since we deleted a tag, redraw the screen */
    TableInvalidate (tablePtr, 0, 0, Tk_Width (tablePtr->tkwin),
		     Tk_Height (tablePtr->tkwin), 0);
    return result;

  case TAG_EXISTS:
    if (argc != 4) {
      Tcl_AppendResult (tablePtr->interp, "wrong # args: should be \"",
	       argv[0], " tag exists tagName\"", (char *) NULL);
      return TCL_ERROR;
    }
    if ((entryPtr = Tcl_FindHashEntry(tablePtr->tagTable, argv[3]))!=NULL) {
      Tcl_SetResult(tablePtr->interp, "1", TCL_VOLATILE);
    } else {
      Tcl_SetResult(tablePtr->interp, "0", TCL_VOLATILE);
    }
    return TCL_OK;

  case TAG_NAMES:
    /* just print out the tag names */
    if (argc != 3 && argc != 4) {
      Tcl_AppendResult(tablePtr->interp, "wrong # args: should be \"", argv[0],
		       " tag names ?pattern?\"", (char *) NULL);
      return TCL_ERROR;
    }
    entryPtr = Tcl_FirstHashEntry (tablePtr->tagTable, &search);
    while (entryPtr != NULL) {
      keybuf = Tcl_GetHashKey(tablePtr->tagTable, entryPtr);
      if (argc == 3 || Tcl_StringMatch(keybuf, argv[3]))
	Tcl_AppendElement(tablePtr->interp, keybuf);
      entryPtr = Tcl_NextHashEntry (&search);
    }
    return TCL_OK;
  }
  return TCL_OK;
}


/********************************************************************
**
** TABLE WIDGET COMMAND
** Function which interprets commands to a table instance
**
*********************************************************************
*/
static int TableWidgetCmd (ClientData clientdata, Tcl_Interp * interp,
			   int argc, char *argv[])
{
  Tcl_HashEntry *entryPtr;
  Tcl_HashSearch search;
  Tcl_HashTable *hashTablePtr;
  int result, retval, sub_retval, row, col, x, y;
  int i, width, height, dummy, key, value, posn;
  int posnFrom, posnTo, oldlen, newlen;
  char buf1[64], buf2[64];

#ifdef KANJI
  wchar *oldstr;
  wchar *newstr;
#endif

  Table *tablePtr = (Table *) clientdata;

  if (argc < 2) {
    Tcl_AppendResult (interp, "wrong # args: should be \"", argv[0],
		      " option ?arg arg ..?\"", (char *) NULL);
    return TCL_ERROR;
  }
  Tcl_Preserve (clientdata);

  result = TCL_OK;
  /* parse the first parameter */
  retval = parse_command (interp, commandos, argv[1]);

  /* Switch on the parameter value */
  switch (retval) {
  case 0:
    /* error */
    /* the return string is already set up */
    result = TCL_ERROR;
    break;

  case CMD_ACTIVATE:
    if (argc != 3) {
      Tcl_AppendResult(interp, "wrong # args should be \"",
		       argv[0], " activate index\"", (char *)NULL);
      result = TCL_ERROR;
    } else if (Table_GetIndex(tablePtr,argv[2],&row,&col)==TCL_ERROR) {
      result = TCL_ERROR;
    } else {
      tablePtr->activeRow = min(max(tablePtr->titleRows,
				    row-tablePtr->rowOffset),tablePtr->rows-1);
      tablePtr->activeCol = min(max(tablePtr->titleCols,
				    col-tablePtr->colOffset),tablePtr->cols-1);
      tablePtr->flags |= TBL_HAS_ACTIVE;
      TableAdjustParams(tablePtr);
      TableConfigCursor(tablePtr);
    }
    break;	/* ACTIVATE */

  case CMD_BBOX:
    /* bounding box of cell at index */
    if (argc != 3 || Table_GetIndex(tablePtr,argv[2],&row,&col)==TCL_ERROR) {
      if (!strlen(interp->result))
	Tcl_AppendResult (interp, "wrong # args should be \"",
			  argv[0], " bbox index\"", (char *) NULL);
      result = TCL_ERROR;
      break;
    }
    TableCellCoords (tablePtr, row - tablePtr->rowOffset,
		     col - tablePtr->colOffset, &x, &y, &width, &height);
    if (!TableCellHidden(tablePtr, row, col))
      sprintf (interp->result, "%d %d %d %d", x, y, width, height);
    break;	/* BBOX */

  case CMD_BOUNDARY:
    /* Find/set boundary row/col.  Usually used by the scrollbar command */
    if (argc < 3 || argc > 4) {
      Tcl_AppendResult (interp, "wrong # args should be \"",
			argv[0], " boundary top|left|bottom|right" ,
			" ?rowOrCol?\"", (char *) NULL);
      result = TCL_ERROR;
      break;
    }
    sub_retval = parse_command (interp, boundary_commands, argv[2]);
    if (argc==4 && ((sscanf (argv[3], "%d", &value) != 1) ||
		    sub_retval==BOUNDARY_BOTTOM ||
		    sub_retval==BOUNDARY_RIGHT)) {
      Tcl_AppendResult (interp, "wrong # args should be \"", argv[0],
			" boundary top|left", " rowOrCol\"", (char *) NULL);
      result = TCL_ERROR;
      break;
    }
    switch (sub_retval) {
    case 0:
      result = TCL_ERROR;
      break;
    case BOUNDARY_LEFT:
    case BOUNDARY_TOP:
      if (argc==4) {
	if (sub_retval==BOUNDARY_LEFT) {
	  tablePtr->leftCol = value + tablePtr->titleCols;
	} else {
	  tablePtr->topRow = value + tablePtr->titleRows;
	}
	TableAdjustParams (tablePtr);
      }
      sprintf(interp->result, "%d", (sub_retval==BOUNDARY_LEFT) ?
	      tablePtr->leftCol+tablePtr->colOffset :
	      tablePtr->topRow+tablePtr->rowOffset);
      break;
    case BOUNDARY_BOTTOM:
    case BOUNDARY_RIGHT:
      TableWhatCell(tablePtr, Tk_Width(tablePtr->tkwin)-1,
		    Tk_Height(tablePtr->tkwin)-1, &row, &col);
      sprintf(interp->result, "%d", (sub_retval==BOUNDARY_BOTTOM) ?
	      row+tablePtr->rowOffset : col+tablePtr->colOffset);
      break;
    }
    break;	/* BOUNDARY */

  case CMD_CGET:
    if (argc != 3) {
      Tcl_AppendResult(interp, "wrong # args: should be \"",
                       argv[0], " cget option\"", (char *) NULL);
      result = TCL_ERROR;
      break;
    }
    result = Tk_ConfigureValue(interp, tablePtr->tkwin, TableConfig,
                               (char *) tablePtr, argv[2], 0);
    break;	/* CGET */

  case CMD_CONFIGURE:
    /* configure */
    switch (argc) {
    case 2:
      result = Tk_ConfigureInfo (interp, tablePtr->tkwin, TableConfig,
				 (char *) tablePtr, (char *) NULL, 0);
      break;
    case 3:
      result = Tk_ConfigureInfo (interp, tablePtr->tkwin, TableConfig,
				 (char *) tablePtr, argv[2], 0);
      break;
    default:
      result = TableConfigure (interp, tablePtr, argc - 2, argv + 2,
			       TK_CONFIG_ARGV_ONLY);
    }
    break;	/* CONFIGURE */

  case CMD_CURVALUE:
    /* Oct,25,95 K.Itoh Added. get current active selection buffer */
    if (argc!=2) {
      Tcl_AppendResult( interp, "wrong # args : should be \"",
                        argv[0], " curvalue\"", (char *)NULL);
      result=TCL_ERROR;
      break;
    }
#ifdef KANJI
    Tcl_AppendResult(interp, Tk_DecodeWStr(tablePtr->activeBuf),
		     (char *) NULL);
#else
    Tcl_AppendResult(interp, tablePtr->activeBuf, (char *)NULL);
#endif
    break;	/* CURVALUE */

  case CMD_CURSELECTION:
    if (argc != 2 && argc != 4) {
      Tcl_AppendResult(	interp, "wrong # args : should be \"", argv[0],
			" curselection ?set <value>?\"", (char *)NULL);
      result = TCL_ERROR;
    } else if (argc == 4 && (tablePtr->arrayVar == NULL ||
			     tablePtr->state == tkDisabledUid)) {
      result = TCL_OK;
    } else {
      Tcl_HashEntry *entryPtr;
      Tcl_HashSearch search;
      for (entryPtr=Tcl_FirstHashEntry(tablePtr->selCells, &search);
	   entryPtr!=NULL;
	   entryPtr=Tcl_NextHashEntry(&search)) {
	if (argc == 2) {
	  Tcl_AppendElement(interp,
			    Tcl_GetHashKey(tablePtr->selCells, entryPtr));
	} else {
	  Tcl_SetVar2(interp, tablePtr->arrayVar,
		      Tcl_GetHashKey(tablePtr->selCells, entryPtr),
		      argv[3], TCL_GLOBAL_ONLY);
	}
      }
    }
    break;	/* CURSELECTION */

  case CMD_DELETE:
    if (argc > 5) {
      Tcl_AppendResult (interp, "wrong # args should be \"", argv[0],
			" delete option arg ?arg?\"", (char *) NULL);
      result = TCL_ERROR;
      break;
    }
    sub_retval = parse_command (interp, mod_commands, argv[2]);
    switch (sub_retval) {
    case 0:
      result = TCL_ERROR;
      break;
    case MOD_ACTIVE:
      if (TableParseStringPosn(tablePtr, argv[3], &posnFrom) == TCL_ERROR ||
	  (argc == 5 &&
	   TableParseStringPosn(tablePtr, argv[4], &posnTo) == TCL_ERROR)) {
	result = TCL_ERROR;
	break;
      }
      if (!(tablePtr->flags & TBL_HAS_ACTIVE) ||
	  tablePtr->state == tkDisabledUid)
	break;
      if (argc == 4)
	posnTo = posnFrom;
      else if (posnTo < posnFrom) {
	int tmp;
	tmp = posnFrom;
	posnFrom = posnTo;
	posnTo = tmp;
      }
      /* do the deletion */
#ifdef KANJI
      oldlen=Tcl_WStrlen (tablePtr->activeBuf);
      if ( oldlen ) {
	if ( posnTo >= oldlen ) posnTo = oldlen-1;
	if ( posnFrom >= oldlen ) posnFrom = oldlen-1;
	tablePtr->activeBuf = Tk_DeleteWStr (interp, tablePtr->activeBuf,
					     posnFrom, posnTo-posnFrom+1);
      }
#else
      oldlen = strlen (tablePtr->activeBuf);
      for (i = 0; i <= oldlen - posnTo; i++)
	tablePtr->activeBuf[i + posnFrom] = tablePtr->activeBuf[i+posnTo+1];
#endif

      /* make sure this string is null terminated */
      tablePtr->activeBuf[oldlen - (posnTo - posnFrom)] = '\0';

      /* put the cursor at the deletion point */
      tablePtr->textCurPosn = posnTo;

      /* mark the text as changed */
      tablePtr->flags |= TBL_TEXT_CHANGED;

      /* ACTIVE */
      TableCellCoords (tablePtr, tablePtr->activeRow, tablePtr->activeCol,
		       &x, &y, &width, &height);
      TableInvalidate (tablePtr, x, y, width, height, 1);
      break;	/* DELETE ACTIVE */
    }
    break;	/* DELETE */

  case CMD_GET: {
    int r1,c1,r2,c2,lr,lc,hr,hc,r,c;
    Tcl_DString list;
    char *value;
    if (argc < 3 || argc > 4 ||
        Table_GetIndex(tablePtr,argv[2],&r1,&c1)!=TCL_OK) {
    Error_get:
      if (!strlen(interp->result)) {
	Tcl_AppendResult(interp, "wrong # args : should be \"",
			 argv[0], " get first ?last?\"", (char *)NULL);
      }
      result = TCL_ERROR;
      break;
    }
    r2 = r1;
    c2 = c1;
    if ( argc == 4 && TCL_OK != Table_GetIndex(tablePtr,argv[3],&r2,&c2) )
      goto Error_get;
    lr = min(r1,r2);
    hr = max(r1,r2);
    lc = min(c1,c2);
    hc = max(c1,c2);
    Tcl_DStringInit(&list);

    for ( r = lr; r <= hr; r++ ) {
      for ( c = lc; c <= hc; c++ ) {
	char *data;
	TableMakeArrayIndex(tablePtr,r,c,buf1);
	data = NULL;
	if ( tablePtr->arrayVar ) {
	  data = Tcl_GetVar2(interp,tablePtr->arrayVar,buf1,TCL_GLOBAL_ONLY);
	}
#ifdef KANJI
	if ( data ) data = Tk_GetWStr(interp,data);
#endif
	if ( !data ) data = "";
	Tcl_DStringAppendElement(&list,data);
      }
    }
    value = (char *)strdup(list.string);
    Tcl_DStringFree(&list);
    Tcl_SetResult(interp,value,TCL_DYNAMIC);
    result = TCL_OK;
  }
  break;	/* GET */

  case CMD_HEIGHT:
  case CMD_WIDTH:
    /* changes the width/height of certain selected columns */
    if (argc != 3 && (argc & 1)) {
      Tcl_AppendResult(interp, "wrong # args : should be \"", argv[0],
		       (retval == CMD_WIDTH) ?
		       " width ?col? ?width col width ...?\"" :
		       " height ?row? ?height row height ...?\"",
		       (char *) NULL);
      result = TCL_ERROR;
      break;
    }
    if (retval == CMD_WIDTH) {
      hashTablePtr = tablePtr->colWidths;
      key = tablePtr->colOffset;
    } else { 
      hashTablePtr = tablePtr->rowHeights;
      key = tablePtr->rowOffset;
    }

    if (argc == 2) {
      /* print out all the preset column widths or row heights */
      entryPtr = Tcl_FirstHashEntry (hashTablePtr, &search);
      while (entryPtr != NULL) {
	posn = (int) Tcl_GetHashKey (hashTablePtr, entryPtr);
	value = (int) Tcl_GetHashValue (entryPtr);
	posn += key;
	sprintf (buf1, "%d %d", posn, value);
	Tcl_AppendElement(interp, buf1);
	entryPtr = Tcl_NextHashEntry (&search);
      }
    } else if (argc == 3) {
      /* get the width/height of a particular row/col */
      if (Tcl_GetInt(interp, argv[2], &posn) != TCL_OK) {
	result = TCL_ERROR;
	break;
      }
      /* no range check is done, why bother? */
      posn -= key;
      entryPtr = Tcl_FindHashEntry (hashTablePtr, (char *) posn);
      if (entryPtr != NULL)
	sprintf (interp->result, "%d", (int) Tcl_GetHashValue (entryPtr));
      else {
	if (retval == CMD_WIDTH)
	  sprintf (interp->result, "%d", tablePtr->defColWidth);
	else
	  sprintf (interp->result, "%d", tablePtr->defRowHeight);
      }
    } else {
      for (i=2; i<argc; i++) {
	/* set new width|height here */
	if (Tcl_GetInt(interp, argv[i], &posn) != TCL_OK ||
	    Tcl_GetInt(interp, argv[++i], &value) != TCL_OK) {
	  result = TCL_ERROR;
	  break;
	}
	posn -= key;
	if (value < 0) {
	  /* reset that field if value is < 0 */
	  if ((entryPtr = Tcl_FindHashEntry (hashTablePtr,
					     (char *) posn)) != NULL)
	    Tcl_DeleteHashEntry (entryPtr);
	} else {
	  entryPtr = Tcl_CreateHashEntry (hashTablePtr, (char *) posn, &dummy);
	  Tcl_SetHashValue (entryPtr, (ClientData)value);
	}
      }
      TableAdjustParams (tablePtr);

      /* rerequest geometry */
      x = min (tablePtr->maxWidth, max (Tk_Width (tablePtr->tkwin),
					tablePtr->maxReqWidth));
      y = min (tablePtr->maxHeight, max (Tk_Height (tablePtr->tkwin),
					 tablePtr->maxReqHeight));
      Tk_GeometryRequest (tablePtr->tkwin, x, y);

      /*
        Invalidate the whole window as TableAdjustParams
        will only check to see if the top left cell has moved
       */
      TableInvalidate (tablePtr, 0, 0, Tk_Width (tablePtr->tkwin),
		       Tk_Height (tablePtr->tkwin), 0);
    }
    break;	/* HEIGHT && WIDTH */

  case CMD_ICURSOR:
    /* set the insertion cursor */
    if (!(tablePtr->flags & TBL_HAS_ACTIVE) ||
	tablePtr->state == tkDisabledUid)
      break;
    switch (argc) {
    case 2:
      sprintf (interp->result, "%d", tablePtr->textCurPosn);
      break;
    case 3:
      if (TableParseStringPosn (tablePtr, argv[2], &posn) != TCL_OK) {
	result = TCL_ERROR;
	break;
      }
      tablePtr->textCurPosn = posn;
      /* ACTIVE */
      TableCellCoords (tablePtr, tablePtr->activeRow, tablePtr->activeCol,
		       &x, &y, &width, &height);
      TableInvalidate (tablePtr, x, y, width, height, 1);
      break;
    default:
      Tcl_AppendResult (interp, "wrong # args should be \"",
			argv[0], " icursor arg\"", (char *) NULL);
      result = TCL_ERROR;
      break;
    }
    break;	/* ICURSOR */

  case CMD_INDEX:
    if (argc < 3 || argc > 4 ||
	Table_GetIndex(tablePtr,argv[2],&row,&col)==TCL_ERROR ||
	(argc == 4 && !Tcl_RegExpMatch(interp, argv[3], "^(row|col)$"))) {
      if (!strlen(interp->result)) {
	Tcl_AppendResult( interp, "wrong # args : should be \"", argv[0],
			  " index index ?row|col?\"", (char *)NULL);
      }
      result=TCL_ERROR;
      break;
    }
    if (argc==3) {
      if (tablePtr->rowThenCol) {
	sprintf(interp->result,"%d,%d",row,col);
      } else {
	sprintf(interp->result,"%d,%d",col,row);
      }
    } else if (argv[3][0] == 'r') { /* INDEX row */
      sprintf(interp->result,"%d",row);
    } else {	/* INDEX col */
      sprintf(interp->result,"%d",col);
    }
    break;	/* INDEX */

  case CMD_INSERT:
    /* are edits enabled */
    if (argc != 5) {
      Tcl_AppendResult (interp, "wrong # args should be \"",
			argv[0], " insert option arg arg\"", (char *) NULL);
      result = TCL_ERROR;
      break;
    }
    sub_retval = parse_command (interp, mod_commands, argv[2]);
    switch (sub_retval) {
    case 0:
      result = TCL_ERROR;
      break;
    case MOD_ACTIVE:
      if (TableParseStringPosn (tablePtr, argv[3], &posn) != TCL_OK) {
	result = TCL_ERROR;
	break;
      }
      if (!(tablePtr->flags & TBL_HAS_ACTIVE) ||
	  tablePtr->state == tkDisabledUid)
	break;
      /* Is this an autoclear and this is the first update */
      if (tablePtr->autoClear && !(tablePtr->flags & TBL_TEXT_CHANGED)) {
#ifndef KANJI
	/* set the buffer to be empty */
	tablePtr->activeBuf[0] = '\0';
#else
	wchar *oldBuf;
	oldBuf = tablePtr->activeBuf;
	tablePtr->activeBuf = Tk_GetWStr (interp,"");
	Tk_FreeWStr (oldBuf);
#endif
	/* mark the text as changed */
	tablePtr->flags |= TBL_TEXT_CHANGED;
	/* the insert position now has to be 0 */
	posn = 0;
      }
#ifdef KANJI
      newstr=ImpKGetStr(interp,argv[4]);
      newlen= Tcl_WStrlen(newstr);
      /* cache the old string length and the new*/
      tablePtr->activeBuf=Tk_InsertWStr(interp, tablePtr->activeBuf,
					posn,newstr);
      Tk_FreeWStr(newstr);
#else              /* KANJI */
      /* get the buffer to at least the right length */
      TableBufLengthen(tablePtr, strlen(tablePtr->activeBuf)+strlen(argv[4]));

      /* cache the old string length and the new */
      oldlen = strlen (tablePtr->activeBuf);
      newlen = strlen (argv[4]);
      for (i = oldlen; i >= posn; i--)
	tablePtr->activeBuf[i + newlen] = tablePtr->activeBuf[i];
      for (i = 0; i < newlen; i++)
	tablePtr->activeBuf[posn + i] = argv[4][i];
#endif              /* KANJI */

      /* put the cursor after the insertion */
      tablePtr->textCurPosn = posn + newlen;

      /* ACTIVE */
      TableCellCoords (tablePtr, tablePtr->activeRow, tablePtr->activeCol,
		       &x, &y, &width, &height);
      TableInvalidate (tablePtr, x, y, width, height, 1);
      break;	/* INSERT ACTIVE */
    }
    break;	/* INSERT */

  case CMD_REREAD:
    /* this rereads the selection from the array */
    TableGetActiveBuf (tablePtr);
    /* ACTIVE */
    TableCellCoords (tablePtr, tablePtr->activeRow, tablePtr->activeCol,
		     &x, &y, &width, &height);
    TableInvalidate (tablePtr, x, y, width, height, 1);
    break;	/* REREAD */

  case CMD_SCAN:
    if (argc != 5 || Tcl_GetInt(interp, argv[3], &x) != TCL_OK ||
	Tcl_GetInt(interp, argv[4], &y) != TCL_OK) {
      Tcl_AppendResult(interp, "wrong # args: should be \"",
		       argv[0], " scan mark|dragto x y\"", (char *) NULL);
      result = TCL_ERROR;
      break;
    }
    if ((argv[2][0] == 'm')
	&& (strncmp(argv[2], "mark", strlen(argv[2])) == 0)) {
      TableWhatCell(tablePtr, x, y, &row, &col);
      tablePtr->scanMarkRowOffset = row;
      tablePtr->scanMarkColOffset = col;
      tablePtr->scanMarkX = x;
      tablePtr->scanMarkY = y;
    } else if ((argv[2][0] == 'd')
	       && (strncmp(argv[2], "dragto", strlen(argv[2])) == 0)) {
      TableScanTo(tablePtr, x, y);
    } else {
      Tcl_AppendResult(interp, "bad scan option \"", argv[2],
		       "\": must be mark or dragto", (char *) NULL);
      result = TCL_ERROR;
      break;
    }
    break;	/* SCAN */

  case CMD_SEE:
    if (argc!=3 || Table_GetIndex(tablePtr,argv[2],&row,&col)==TCL_ERROR) {
      if (!strlen(interp->result)) {
	Tcl_AppendResult(interp, "wrong # args : should be \"",
			 argv[0], " see index\"", (char *)NULL);
      }
      result = TCL_ERROR;
      break;
    }
    if (TableCellHidden(tablePtr, row, col)) {
      tablePtr->topRow  = row-tablePtr->rowOffset-1;
      tablePtr->leftCol = col-tablePtr->colOffset-1;
    }
    TableAdjustParams(tablePtr);
    break;	/* SEE */

  case CMD_SELECTION:
    retval=parse_command(interp, sel_commands, argv[2]);
    switch(retval) {
    case 0: 		/* failed to parse the argument, error */
      return TCL_ERROR;
    case SEL_ANCHOR:
      if (argc != 4 || Table_GetIndex(tablePtr,argv[3],&row,&col) != TCL_OK) {
	if (!strlen(interp->result))
	  Tcl_AppendResult(interp, "wrong # args : should be \"", argv[0],
			   " selection anchor index\"", (char *)NULL);
	result=TCL_ERROR;
	break;
      }
      tablePtr->flags |= TBL_HAS_ANCHOR;
      tablePtr->anchorRow = min(max(tablePtr->titleRows,
				    row-tablePtr->rowOffset),tablePtr->rows-1);
      tablePtr->anchorCol = min(max(tablePtr->titleCols,
				    col-tablePtr->colOffset),tablePtr->cols-1);
      break;
    case SEL_CLEAR: {
      int r1,c1,r2,c2,lr,lc,hr,hc,r,c;
      Tcl_HashEntry *entryPtr;
      if ( argc != 4 && argc != 5 ) {
      Error_clear:
	if (!strlen(interp->result))
	  Tcl_AppendResult(interp, "wrong # args : should be \"", argv[0],
			   " selection clear all|<first> ?<last>?\"",
			   (char *)NULL);
	result=TCL_ERROR;
	break;
      }
      if (strcmp(argv[3],"all") == 0) {
	for( entryPtr=Tcl_FirstHashEntry(tablePtr->selCells, &search);
	     entryPtr!=NULL;
	     entryPtr=Tcl_NextHashEntry(&search)) {
	  TableParseArrayIndex(tablePtr, &r, &c,
			       Tcl_GetHashKey(tablePtr->selCells,entryPtr));
	  Tcl_DeleteHashEntry(entryPtr);
	  TableCellCoords(tablePtr, r-tablePtr->rowOffset,
			  c-tablePtr->colOffset, &x, &y, &width, &height);
	  TableInvalidate(tablePtr, x, y, width, height,0); 
	}
      } else {
	if (Table_GetIndex(tablePtr,argv[3],&r1,&c1) != TCL_OK)
	  goto Error_clear;
	r2 = r1;
	c2 = c1;
	if (argc == 5 && Table_GetIndex(tablePtr,argv[4],&r2,&c2) != TCL_OK)
	  goto Error_clear;
	lr = min(r1,r2);
	hr = max(r1,r2);
	lc = min(c1,c2);
	hc = max(c1,c2);

	for ( r = lr; r <= hr; r++ ) {
	  for ( c = lc; c <= hc; c++ ) {
	    TableMakeArrayIndex(tablePtr, r, c, buf1);
	    if ((entryPtr=Tcl_FindHashEntry(tablePtr->selCells,
					    buf1)) != NULL) {
	      Tcl_DeleteHashEntry(entryPtr);
	      TableCellCoords(tablePtr, r-tablePtr->rowOffset,
			      c-tablePtr->colOffset, &x, &y, &width, &height);
	      TableInvalidate(tablePtr, x, y, width, height,0); 
	    }
	  }
	}
      }
    }
    break;	/* SELECTION CLEAR */
    case SEL_INCLUDES:
      if (argc != 4 || Table_GetIndex(tablePtr,argv[3],&row,&col) != TCL_OK) {
	if (!strlen(interp->result))
	  Tcl_AppendResult(interp, "wrong # args : should be \"", argv[0],
			   " selection includes index\"", (char *)NULL);
	result=TCL_ERROR;
	break;
      }
      interp->result = "0";
      if ( tablePtr->selectionOn ) {
	TableMakeArrayIndex(tablePtr, row, col, buf1);
	if ( Tcl_FindHashEntry(tablePtr->selCells, buf1) )
	  interp->result = "1";
      }
      break;	/* SELECTION INCLUDES */
    case SEL_SET: {
      int r1,c1,r2,c2,lr,lc,hr,hc,r,c;
      if (argc < 4 || argc > 6 ||
	  Table_GetIndex(tablePtr,argv[3],&r1,&c1) == TCL_ERROR ) {
      Error_set:
	if (!strlen(interp->result))
	  Tcl_AppendResult(interp, "wrong # args : should be \"", argv[0],
			   " selection set first ?last? ?setlist?\"",
			   (char *)NULL);
	result=TCL_ERROR;
	break;
      }
      r1=min(max(tablePtr->titleRows, r1-tablePtr->rowOffset),
	     tablePtr->rows-1);
      c1=min(max(tablePtr->titleCols, c1-tablePtr->colOffset),
	     tablePtr->cols-1);
      r2 = r1;
      c2 = c1;
      if (argc > 4) {
	if (Table_GetIndex(tablePtr,argv[4],&r2,&c2)==TCL_ERROR) {
	  goto Error_set;
	} else {
	  r2 = min(max(tablePtr->titleRows, r2-tablePtr->rowOffset),
		   tablePtr->rows-1);
	  c2 = min(max(tablePtr->titleCols, c2-tablePtr->colOffset),
		   tablePtr->cols-1);
	}
      }
      lr = min(r1,r2);
      hr = max(r1,r2);
      lc = min(c1,c2);
      hc = max(c1,c2);

      if ( argc < 6 ) {
	for ( r = lr; r <= hr; r++ ) {
	  for ( c = lc; c <= hc; c++ ) {
	    TableMakeArrayIndex(tablePtr, r+tablePtr->rowOffset,
				c+tablePtr->colOffset, buf1);
	    if (Tcl_FindHashEntry(tablePtr->selCells,buf1)==NULL) {
	      Tcl_CreateHashEntry(tablePtr->selCells,buf1,&dummy);
	      tablePtr->selRow = r;
	      tablePtr->selCol = c;
	      if ( tablePtr->selRow >= tablePtr->titleRows &&
		   tablePtr->selCol >= tablePtr->titleCols &&
		   tablePtr->selRow <= tablePtr->rows &&
		   tablePtr->selCol <= tablePtr->cols ) {
		TableCellCoords(tablePtr, tablePtr->selRow, tablePtr->selCol,
				&x, &y, &width, &height);
		TableInvalidate(tablePtr, x, y, width, height,0); 
	      }
	    }
	  }
	}
      } else {		/* setlist */
	char **list;
	int ct;
	if (Tcl_SplitList(interp,argv[5],&ct,&list) != TCL_OK)
	  goto Error_set;
	/* This hack needs FIXing, seriously */
	lr += tablePtr->rowOffset;
	hr += tablePtr->rowOffset;
	lc += tablePtr->colOffset;
	hc += tablePtr->colOffset;
	for (i=0; i<ct ; i++ ) {
	  if (Table_GetIndex(tablePtr,list[i],&r,&c)==TCL_ERROR) {
	    ckfree(list);
	    goto Error_set;
	  }
	  if ( r<lr || r>hr || c<lc || c>hc ) continue;
	  TableMakeArrayIndex(tablePtr, r, c, buf1);
	  if (Tcl_FindHashEntry(tablePtr->selCells,buf1)==NULL) {
	    Tcl_CreateHashEntry(tablePtr->selCells,buf1,&dummy);
	    tablePtr->selRow = r-tablePtr->rowOffset;
	    tablePtr->selCol = c-tablePtr->colOffset;
	    if ( tablePtr->selRow >= tablePtr->titleRows &&
		 tablePtr->selCol >= tablePtr->titleCols &&
		 tablePtr->selRow <= tablePtr->rows &&
		 tablePtr->selCol <= tablePtr->cols ) {
	      TableCellCoords(tablePtr, tablePtr->selRow, tablePtr->selCol,
			      &x, &y, &width, &height);
	      TableInvalidate(tablePtr, x, y, width, height,0); 
	    }
	  }
	}
	ckfree(list);
      }
      /* Set selection to be the last selected r and c */
      tablePtr->selRow = min(max(tablePtr->titleRows, tablePtr->selRow),
			     tablePtr->rows-1);
      tablePtr->selCol = min(max(tablePtr->titleCols, tablePtr->selCol),
			     tablePtr->cols-1);

      /* Adjust the table for top left, selection on screen etc */
      TableAdjustParams(tablePtr);
    }
    break;	/* SELECTION SET */
    }
    break;	/* SELECTION */

  case CMD_SET:
    /* sets any number of tags/indices to a given value */
    if (argc < 4 || (argc & 1)) {
      Tcl_AppendResult(interp, "wrong # args : should be \"", argv[0],
		       " set index value ?index value ...?\"", (char *) NULL);
      result = TCL_ERROR;
      break;
    }
    if (tablePtr->arrayVar && tablePtr->state == tkNormalUid) {
      for (i=2; i<argc; i++) {
	if (Table_GetIndex(tablePtr, argv[i], &row, &col) != TCL_OK) {
	  result = TCL_ERROR;
	  break;
	}
	TableMakeArrayIndex(tablePtr, row, col, buf1);
	Tcl_SetVar2(interp, tablePtr->arrayVar, buf1, argv[++i],
		    TCL_GLOBAL_ONLY);
      }
    }
    break;

  case CMD_TAG:
    /* a veritable plethora of tag commands */
    /* do we have another argument */
    if (argc < 3) {
      Tcl_AppendResult (interp, "wrong # args should be \"", argv[0],
			" tag option ?arg arg ...?\"", (char *) NULL);
      result = TCL_ERROR;
      break;
    }
    /* all the rest is now done in a separate function */
    result = TableTagCommand (tablePtr, argc, argv);
    break;	/* TAG */

  case CMD_XVIEW:
  case CMD_YVIEW:
    if (argc==2) {
      TableWhatCell(tablePtr, Tk_Width(tablePtr->tkwin)-1,
		    Tk_Height(tablePtr->tkwin)-1, &row, &col);
      if ( retval == CMD_YVIEW ) {
	sprintf(interp->result,"%g %g",
		((double)tablePtr->topRow)/(tablePtr->rows-1),
		((double)row)/(tablePtr->rows-1));
      } else {
	sprintf(interp->result,"%g %g",
		((double)tablePtr->leftCol)/(tablePtr->cols-1),
		((double)col)/(tablePtr->cols-1));
      }
    } else if ( strncmp(argv[2],"moveto",strlen(argv[2])) == 0 ) {
      double frac;
      if (argc != 4 || TCL_OK != Tcl_GetDouble(interp,argv[3],&frac) ||
	  frac < 0.0 || frac > 1.0 ) {
	Tcl_AppendResult(interp, "wrong # or value of args: should be \"",
			 argv[0], " ",argv[1]," moveto <frac>\"",
			 (char *) NULL);
	result = TCL_ERROR;
	break;
      }
      if ( retval == CMD_YVIEW ) {
	tablePtr->topRow = frac * tablePtr->rows + tablePtr->titleRows;
      } else {
	tablePtr->leftCol = frac * tablePtr->cols + tablePtr->titleCols;
      }
      result = TCL_OK;
    } else if ( strncmp(argv[2],"scroll",strlen(argv[2])) == 0 ) {
      int num;
      if ( argc != 5 || TCL_OK != Tcl_GetInt(interp,argv[3],&num) ) {
      Error_yscroll:
	Tcl_AppendResult(interp, "wrong # or value of args: should be \"",
			 argv[0], " ",argv[1]," scroll <num> units|pages\"",
			 (char *) NULL);
	result = TCL_ERROR;
	break;
      }

      if ( strncmp(argv[4],"units",strlen(argv[4])) == 0 ) {
	if ( retval == CMD_YVIEW ) {
	  tablePtr->topRow  += num + tablePtr->titleRows;
	} else {
	  tablePtr->leftCol += num + tablePtr->titleCols;
	}
      } else if ( strncmp(argv[4],"pages",strlen(argv[4])) == 0 ) {
	TableWhatCell(tablePtr, Tk_Width(tablePtr->tkwin)-1,
		      Tk_Height(tablePtr->tkwin)-1, &row, &col);
	if ( retval == CMD_YVIEW ) {
	  tablePtr->topRow += num*(tablePtr->topRow-row+1)+tablePtr->titleRows;
	} else {
	  tablePtr->leftCol+=num*(tablePtr->leftCol-col+1)+tablePtr->titleCols;
	}
      } else {
	goto Error_yscroll;
      }
      
    } else {		/* index */
      if ( TCL_OK != Table_GetIndex(tablePtr,argv[2],&row,&col) ) {
	result = TCL_ERROR;
	break;
      }
      if ( retval == CMD_YVIEW ) {
	tablePtr->topRow  = row + tablePtr->titleRows;
      } else {
	tablePtr->leftCol = col + tablePtr->titleCols;
      }
    }
    /* 
    ** Do the table adjustment, which will 
    ** also invalidate the table if necessary
    */	
    TableAdjustParams(tablePtr);
    break;
  }
  Tk_Release(clientdata);
  return result;
}

static void TableDestroy (ClientData clientdata) {
  Table *tablePtr = (Table *) clientdata;
  Tcl_HashEntry *entryPtr;
  Tcl_HashSearch search;

  /* delete the variable trace */
  if (tablePtr->arrayVar != NULL)
    Tcl_UntraceVar (tablePtr->interp, tablePtr->arrayVar,
		    TCL_TRACE_WRITES | TCL_TRACE_UNSETS | TCL_GLOBAL_ONLY,
		    (Tcl_VarTraceProc *)TableVarProc, (ClientData) tablePtr);

  /* free the arrays with row/column pixel sizes */
  if (tablePtr->colPixels) ckfree ((char *) (tablePtr->colPixels));
  if (tablePtr->rowPixels) ckfree ((char *) (tablePtr->rowPixels));
  if (tablePtr->colStarts) ckfree ((char *) (tablePtr->colStarts));
  if (tablePtr->rowStarts) ckfree ((char *) (tablePtr->rowStarts));

  /* free the selection buffer */
  if (tablePtr->activeBuf) 
#ifdef KANJI
    Tk_FreeWStr (tablePtr->activeBuf);
#else
    ckfree(tablePtr->activeBuf);
#endif              /* KANJI */

  /* delete the row, column and cell style hash tables */
  Tcl_DeleteHashTable (tablePtr->rowStyles);
  ckfree ((char *) (tablePtr->rowStyles));
  Tcl_DeleteHashTable (tablePtr->colStyles);
  ckfree ((char *) (tablePtr->colStyles));
  Tcl_DeleteHashTable (tablePtr->cellStyles);
  ckfree ((char *) (tablePtr->cellStyles));
  Tcl_DeleteHashTable (tablePtr->flashCells);
  ckfree ((char *) (tablePtr->flashCells));
  Tcl_DeleteHashTable(tablePtr->selCells);
  ckfree(tablePtr->selCells);
  Tcl_DeleteHashTable (tablePtr->colWidths);
  ckfree ((char *) (tablePtr->colWidths));
  Tcl_DeleteHashTable (tablePtr->rowHeights);
  ckfree ((char *) (tablePtr->rowHeights));

  /* Now free up all the tag information */
  entryPtr = Tcl_FirstHashEntry (tablePtr->tagTable, &search);
  for (; entryPtr != NULL; entryPtr = Tcl_NextHashEntry (&search)) {
    /* free up the Gcs etc */
    TableCleanupTag (tablePtr, (TagStruct *) Tcl_GetHashValue (entryPtr));
    /* free the memory */
    ckfree ((TagStruct *) Tcl_GetHashValue (entryPtr));
  }
  /* And delete the actual hash table */
  Tcl_DeleteHashTable (tablePtr->tagTable);
  ckfree ((char *) (tablePtr->tagTable));
  /* free up the stuff in the default tag */
  TableCleanupTag (tablePtr, &(tablePtr->defaultTag));

  /* free the configuration options in the widget */
  Tk_FreeOptions (TableConfig, (char *) tablePtr, tablePtr->display, 0);

  /* and free the widget memory at last! */
  ckfree ((char *) (tablePtr));
}

static void TableEventProc (ClientData clientdata, XEvent * eventPtr) {
  Table *tablePtr = (Table *) clientdata;
  ImpEvent(tablePtr);
  switch (eventPtr->type) {
  case Expose:
    TableInvalidate (tablePtr, eventPtr->xexpose.x, eventPtr->xexpose.y,
		     eventPtr->xexpose.width, eventPtr->xexpose.height, 0);
    break;
  case DestroyNotify:
    ImpDestroy(tablePtr);
    /* remove the command from the interpreter */
    if(tablePtr->tkwin) {
      Tcl_DeleteCommand (tablePtr->interp, Tk_PathName (tablePtr->tkwin));
      tablePtr->tkwin = NULL;
    }

    /* cancel any pending update or timer */
    if (tablePtr->flags & TBL_REDRAW_PENDING) {
      Tcl_CancelIdleCall (TableDisplay, (ClientData) tablePtr);
      tablePtr->flags &= ~TBL_REDRAW_PENDING;
    }
    if (tablePtr->cursorTimer != NULL) {
      Tcl_DeleteTimerHandler (tablePtr->cursorTimer);
      tablePtr->cursorTimer = NULL;
    }
    if (tablePtr->flashTimer != NULL) {
      Tcl_DeleteTimerHandler (tablePtr->flashTimer);
      tablePtr->flashTimer = NULL;
    }
    if (tablePtr->arrayVar != NULL) {
      Tcl_UntraceVar (tablePtr->interp, tablePtr->arrayVar,
		      TCL_TRACE_WRITES | TCL_TRACE_UNSETS | TCL_GLOBAL_ONLY,
		      (Tcl_VarTraceProc *)TableVarProc, (ClientData) tablePtr);
    }
    /* free the widget in time */
    Tcl_EventuallyFree ((ClientData) tablePtr, (Tcl_FreeProc *) TableDestroy);
    break;

  case ConfigureNotify:
    TableAdjustParams (tablePtr);
    TableInvalidateAll(tablePtr,0);
    break;
  case FocusIn:
    if (eventPtr->xfocus.detail != NotifyInferior) {
      tablePtr->flags |= TBL_HAS_FOCUS;
      if (tablePtr->highlightWidth > 0) TableInvalidate(tablePtr,0,0,1,1,0);
      /* Turn the cursor on */
      TableConfigCursor (tablePtr);
      ImpFocusIn(tablePtr,eventPtr);
    }
    break;
  case FocusOut:
    if (eventPtr->xfocus.detail != NotifyInferior) {
      tablePtr->flags &= ~TBL_HAS_FOCUS;
      if (tablePtr->highlightWidth > 0)  TableInvalidate(tablePtr,0,0,1,1,0);
      /* cancel the timer */
      TableConfigCursor (tablePtr);
      ImpFocusOut(tablePtr,eventPtr);
    }
    break;
  }
}

#ifndef CLASSPATCH
/* As long as we wait for the Function in general */

static void Tk_ClassOption(Tk_Window tkwin,char *defaultclass,
			   int *argcp,char ***argvp)
{
  char *classname = (((*argcp)<3) || (strcmp((*argvp)[2],"-class"))) ?
    defaultclass : ((*argcp)-=2,(*argcp)+=2,(*argvp)[1]);
  Tk_SetClass(tkwin,classname);
}
#endif

/* The command for creating a new instance of a Table widget */

static int TableCmd (ClientData clientdata, Tcl_Interp * interp,
		     int argc, char *argv[])
{
  Tk_Window main_window = (Tk_Window) clientdata;
  Table *tablePtr;
  Tk_Window tkwin;
  if (argc < 2) {
    Tcl_AppendResult (interp, "wrong #args: should be \"",
		      argv[0], "pathname ?options?\"", (char *) NULL);
    return TCL_ERROR;
  }
  tkwin = Tk_CreateWindowFromPath (interp, main_window, argv[1], (char *)NULL);
  if (tkwin == NULL) return TCL_ERROR;
  tablePtr			= (Table *) ckalloc(sizeof (Table));
  tablePtr->tkwin		= tkwin;
  tablePtr->display		= Tk_Display (tkwin);
  tablePtr->interp		= interp;
  tablePtr->rows		= 0;
  tablePtr->cols		= 0;
  tablePtr->defRowHeight	= 0;
  tablePtr->defColWidth		= 0;
  tablePtr->arrayVar		= NULL;
  tablePtr->borderWidth		= 0;
  tablePtr->defaultTag.bgBorder	= NULL;
  tablePtr->defaultTag.foreground = NULL;
  tablePtr->defaultTag.relief	= TK_RELIEF_FLAT;
#if (TK_MAJOR_VERSION > 4)
  tablePtr->defaultTag.tkfont	= NULL;
#else
  tablePtr->defaultTag.fontPtr	= NULL;
#endif
#ifdef KANJI
  tablePtr->defaultTag.asciiFontPtr = NULL;
  tablePtr->defaultTag.kanjiFontPtr = NULL;
#endif              /* KANJI */
  tablePtr->defaultTag.anchor	= TK_ANCHOR_CENTER;
  tablePtr->yScrollCmd		= NULL;
  tablePtr->xScrollCmd		= NULL;
  tablePtr->insertBg		= NULL;
  tablePtr->cursor		= None;
  tablePtr->rowThenCol		= 1;
  tablePtr->selectionOn		= 1;
  tablePtr->topRow		= 0;
  tablePtr->leftCol		= 0;
  tablePtr->titleRows		= 0;
  tablePtr->titleCols		= 0;
  tablePtr->selRow		= 0;
  tablePtr->selCol		= 0;
  tablePtr->anchorRow		= 0;
  tablePtr->anchorCol		= 0;
  tablePtr->activeRow		= 0;
  tablePtr->activeCol		= 0;
  tablePtr->oldTopRow		= -1;
  tablePtr->oldLeftCol		= -1;
  tablePtr->oldActRow		= -1;
  tablePtr->oldActCol		= -1;
  tablePtr->textCurPosn		= 0;
  tablePtr->flags		= 0;
  tablePtr->drawMode		= DRAW_MODE_TK_COMPAT;
  tablePtr->colStretch		= STRETCH_MODE_NONE;
  tablePtr->rowStretch		= STRETCH_MODE_NONE;
  tablePtr->maxWidth		= 0;
  tablePtr->maxHeight		= 0;
  tablePtr->charWidth		= 0;
  tablePtr->colPixels		= (int *) 0;
  tablePtr->rowPixels		= (int *) 0;
  tablePtr->colStarts		= (int *) 0;
  tablePtr->rowStarts		= (int *) 0;
  tablePtr->colOffset		= 0;
  tablePtr->rowOffset		= 0;
  tablePtr->flashTime		= 3;
  tablePtr->colWidths = (Tcl_HashTable *) ckalloc(sizeof (Tcl_HashTable));
  Tcl_InitHashTable (tablePtr->colWidths, TCL_ONE_WORD_KEYS);
  tablePtr->rowHeights = (Tcl_HashTable *) ckalloc(sizeof (Tcl_HashTable));
  Tcl_InitHashTable (tablePtr->rowHeights, TCL_ONE_WORD_KEYS);
  tablePtr->tagTable = (Tcl_HashTable *) ckalloc(sizeof (Tcl_HashTable));
  Tcl_InitHashTable (tablePtr->tagTable, TCL_STRING_KEYS);
  tablePtr->rowStyles = (Tcl_HashTable *) ckalloc(sizeof (Tcl_HashTable));
  Tcl_InitHashTable (tablePtr->rowStyles, TCL_ONE_WORD_KEYS);
  tablePtr->colStyles = (Tcl_HashTable *) ckalloc(sizeof (Tcl_HashTable));
  Tcl_InitHashTable (tablePtr->colStyles, TCL_ONE_WORD_KEYS);
  tablePtr->cellStyles = (Tcl_HashTable *) ckalloc(sizeof (Tcl_HashTable));
  Tcl_InitHashTable (tablePtr->cellStyles, TCL_STRING_KEYS);
  tablePtr->flashCells = (Tcl_HashTable *) ckalloc(sizeof (Tcl_HashTable));
  Tcl_InitHashTable (tablePtr->flashCells, TCL_STRING_KEYS);
  tablePtr->selCells=(Tcl_HashTable *)ckalloc(sizeof(Tcl_HashTable));
  Tcl_InitHashTable(tablePtr->selCells, TCL_STRING_KEYS);
  tablePtr->cursorTimer		= (Tcl_TimerToken)0;
  tablePtr->flashTimer		= (Tcl_TimerToken)0;
  tablePtr->selectMode		= NULL;
  tablePtr->activeBufLen	= 1;
#ifdef KANJI
  tablePtr->activeBuf		= Tk_GetWStr (interp, "");
#else
  tablePtr->activeBuf		= ckalloc(1);
  *(tablePtr->activeBuf)	= '\0';
#endif              /* KANJI */
  tablePtr->activeBufLen	= 1;
  tablePtr->rowTagCmd		= NULL;
  tablePtr->colTagCmd		= NULL;
  tablePtr->highlightWidth	= 0;
  tablePtr->highlightBgColorPtr	= NULL;
  tablePtr->highlightColorPtr	= NULL;
  tablePtr->takeFocus		= NULL;
  tablePtr->state		= tkNormalUid;
  tablePtr->insertWidth		= 0;
  tablePtr->insertBorderWidth	= 0;
  tablePtr->insertOnTime	= 0;
  tablePtr->insertOffTime	= 0;
  tablePtr->autoClear		= 0;
  tablePtr->batchMode		= 0;
  tablePtr->flashMode		= 0;

  /* selection handlers needed here */

  Tk_ClassOption(tkwin,"Table",&argc,&argv);
  Tk_CreateEventHandler(tkwin,
			ExposureMask | StructureNotifyMask | FocusChangeMask,
			TableEventProc, (ClientData) tablePtr);
  Tcl_CreateCommand (interp, Tk_PathName (tkwin), TableWidgetCmd,
		     (ClientData) tablePtr, (Tcl_CmdDeleteProc *) NULL);
  if (TableConfigure (interp, tablePtr, argc - 2, argv + 2, 0) != TCL_OK) {
    Tk_DestroyWindow (tkwin);
    return TCL_ERROR;
  }
  InitTagTable(tablePtr);
  interp->result = Tk_PathName (tkwin);
  return TCL_OK;
}

/* Function to call on loading the Table module */

int Tktable_Init (Tcl_Interp * interp) {
  static char init_script[] =
  "if [catch {source [lindex $tcl_pkgPath 0]/Tktable/tkTable.tcl}] {\n"
#include "tkTabletcl.h"
  "}\n";
  if (Tcl_PkgRequire (interp, "Tcl", TCL_VERSION, 0) == NULL) {
    return TCL_ERROR;
  }
  if (Tcl_PkgRequire (interp, "Tk", TK_VERSION, 0) == NULL) {
    return TCL_ERROR;
  }
  if (Tcl_PkgProvide (interp, "Tktable", VERSION) != TCL_OK) {
    return TCL_ERROR;
  }
  Tcl_CreateCommand (interp, "table", TableCmd,
	   (ClientData) Tk_MainWindow (interp), (Tcl_CmdDeleteProc *) NULL);
  return Tcl_Eval(interp, init_script);
}

int Tktable_SafeInit (Tcl_Interp * interp) {
  static char init_script[] =
  "if [catch {source [lindex $tcl_pkgPath 0]/Tktable/tkTable.tcl}] {\n"
#include "tkTabletcl.h"
  "}\n";
  if (Tcl_PkgRequire (interp, "Tcl", TCL_VERSION, 0) == NULL) {
    return TCL_ERROR;
  }
  if (Tcl_PkgRequire (interp, "Tk", TK_VERSION, 0) == NULL) {
    return TCL_ERROR;
  }
  if (Tcl_PkgProvide (interp, "Tktable", VERSION) != TCL_OK) {
    return TCL_ERROR;
  }
  Tcl_CreateCommand (interp, "table", TableCmd,
	   (ClientData) Tk_MainWindow (interp), (Tcl_CmdDeleteProc *) NULL);
  return Tcl_Eval(interp, init_script);
}
