/* 
 * tkTable.h --
 *
 *	This is the header file for the module that implements
 *	table widgets for the Tk toolkit.
 *
 * Based on Tk3 table widget written by Roland King
 */

#include <string.h>
#include <stdlib.h>
#include <tk.h>
#include <X11/Xatom.h>

#ifdef WIN32
#   define WIN32_LEAN_AND_MEAN
#   include <windows.h>
#   undef WIN32_LEAN_AND_MEAN

/*
 * VC++ has an alternate entry point called DllMain, so we need to rename
 * our entry point.
 */

#   if defined(_MSC_VER)
#	define EXPORT(a,b) __declspec(dllexport) a b
#	define DllEntryPoint DllMain
#   else
#	if defined(__BORLANDC__)
#	    define EXPORT(a,b) a _export b
#	else
#	    define EXPORT(a,b) a b
#	endif
#   endif

/* Necessary to get XSync call defined */
#include <tkInt.h>

#else
#   define EXPORT(a,b) a b
#endif	/* WIN32 */

#define max(A,B)	((A)>(B))?(A):(B)
#define min(A,B)	((A)>(B))?(B):(A)
#define ARSIZE(A)	(sizeof(A)/sizeof(*A))
#define INDEX_BUFSIZE	64

#ifdef KANJI

#include "tkWStr.h"

#define WCHAR			wchar
#define WSTRLEN			Tcl_WStrlen
#define WSTRCMP			Tcl_WStrcmp
#define DECODE_WSTR		Tk_DecodeWStr
#define ENCODE_WSTR(i,s)	Tk_GetWStr((i),(s))
#define FREE_WSTR		Tk_FreeWStr

#define DEF_TABLE_FONT          "a14"
#define DEF_TABLE_KANJIFONT     "k14"

#else		/* NOT KANJI */

#define WCHAR			char
#define WSTRLEN			strlen
#define WSTRCMP			strcmp
#define DECODE_WSTR	
#define ENCODE_WSTR(i,s)	(s)
#define FREE_WSTR		ckfree

#if (TK_MAJOR_VERSION > 4)
#define DEF_TABLE_FONT          "Helvetica 12"
#else
#define DEF_TABLE_FONT          "-Adobe-Helvetica-Bold-R-Normal--*-120-*"
#endif
#endif		/* KANJI */

/*
 * Assigned bits of "flags" fields of Table structures, and what those
 * bits mean:
 *
 * REDRAW_PENDING:	Non-zero means a DoWhenIdle handler has
 *			already been queued to redisplay the table.
 * REDRAW_BORDER:	Non-zero means 3-D border must be redrawn
 *			around window during redisplay.	 Normally
 *			only text portion needs to be redrawn.
 * CURSOR_ON:		Non-zero means insert cursor is displayed at
 *			present.  0 means it isn't displayed.
 * TEXT_CHANGED:	Non-zero means the active cell text is being edited.
 * HAS_FOCUS:		Non-zero means this window has the input focus.
 * HAS_ACTIVE:		Non-zero means the active cell is set.
 * HAS_ANCHOR:		Non-zero means the anchor cell is set.
 * BROWSE_CMD:		Non-zero means we're evaluating the -browsecommand.
 * VALIDATING:		Non-zero means we are in a valCmd
 * SET_ACTIVE:		About to set the active array element internally
 *
 * FIX - consider adding UPDATE_SCROLLBAR a la entry
 */

#define REDRAW_PENDING		1
#define CURSOR_ON		2
#define	HAS_FOCUS		4
#define TEXT_CHANGED		8
#define HAS_ACTIVE		16
#define HAS_ANCHOR		32
#define BROWSE_CMD		64
#define REDRAW_BORDER		128
#define VALIDATING		256
#define SET_ACTIVE		512

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
  Tk_Font	fontPtr;	/* Information about text font, or NULL. */
#else
  XFontStruct   *fontPtr;       /* default font pointer */
#endif              /* TK 8 */
#endif              /* KANJI */
  Tk_Anchor anchor;		/* default anchor point */
  /* experimental image support */
  char *imageStr;		/* name of image */
  Tk_Image image;		/* actual pointer to image, if any */
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
  char *arrayVar;		/* name of traced array variable */
  char *rowSep;			/* separator string to place between
				 * rows when getting selection */
  char *colSep;			/* separator string to place between
				 * cols when getting selection */
  int borderWidth;		/* internal borderwidth */
  TagStruct defaultTag;		/* the default tag colours/fonts etc */
  char *yScrollCmd;		/* the y-scroll command */
  char *xScrollCmd;		/* the x-scroll command */
  char *browseCmd;		/* the command that is called when the
				 * active cell changes */
  char *selCmd;			/* the command that is called to when a
				 * [selection get] call occurs for a table */
  char *valCmd;			/* Command prefix to use when invoking
				 * validate command.  NULL means don't
				 * invoke commands.  Malloc'ed. */
  int validate;			/* Non-zero means try to validate */
  Tk_3DBorder insertBg;		/* the cursor colour */
  Tk_Cursor cursor;		/* the regular mouse pointer */
  int exportSelection;		/* Non-zero means tie internal table
				 * to X selection. */
  Tk_Uid state;			/* Normal or disabled.  Table is read-only
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
  int batchMode;		/* Whether to */
  char *rowTagCmd, *colTagCmd;
  int highlightWidth;		/* Width in pixels of highlight to draw
				 * around widget when it has the focus.
				 * <= 0 means don't draw a highlight. */
  XColor *highlightBgColorPtr;	/* Color for drawing traversal highlight
				 * area when highlight is off. */
  XColor *highlightColorPtr;	/* Color for drawing traversal highlight. */
  char *takeFocus;		/* Used only in Tcl to check if this
				 * widget will accept focus */

  /* Cached Information */
  int topRow, leftCol;		/* The topleft cell to display excluding the
				 * fixed title rows.  This is just the
				 * config request.  The actual cell used may
				 * be different to keep the screen full */
  int titleRows, titleCols;	/* the number of rows|cols to use as a title */
  int anchorRow, anchorCol;	/* the row,col of the anchor cell */
  int activeRow, activeCol;	/* the row,col of the active cell */
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
  Tcl_HashTable *flashCells;	/* the table of flashing cells */
  Tcl_HashTable *selCells;	/* the table of selected cells */
  Tcl_TimerToken cursorTimer;	/* the timer token for the cursor blinking */
  Tcl_TimerToken flashTimer;	/* the timer token for the cell flashing */
  WCHAR	*activeBuf;		/* the buffer where the selection is kept
				   for editing */
  /* The invalid rectangle if there is an update pending */
  int invalidX, invalidY, invalidWidth, invalidHeight;
} Table;

/* structure for use in parsing table commands/values */
typedef struct {
  char *name;		/* name of the command/value */
  int value;		/* >0 because 0 represents an error */
} CmdStruct;

/* required for -state option */
#ifdef WIN32
static Tk_Uid	tkNormalUid = NULL;
static Tk_Uid	tkDisabledUid = NULL;
#else
extern Tk_Uid	tkNormalUid, tkDisabledUid;
#endif

EXTERN EXPORT(int,Example_Init) _ANSI_ARGS_((Tcl_Interp *interp));
static char *	getCmdName _ANSI_ARGS_((const CmdStruct* c,int val));
static int	getCmdValue _ANSI_ARGS_((const CmdStruct* c,const char *arg));
static void	getCmdError _ANSI_ARGS_((Tcl_Interp *interp, CmdStruct *c,
					 const char *arg));
static int	TableOptionSet _ANSI_ARGS_((ClientData clientData,
					    Tcl_Interp *interp,
					    Tk_Window tkwin, char *value,
					    char *widgRec, int offset));
static char *	TableOptionGet _ANSI_ARGS_((ClientData clientData,
					    Tk_Window tkwin, char *widgRec,
					    int offset,
					    Tcl_FreeProc **freeProcPtr));
static int	TableFetchSelection _ANSI_ARGS_((ClientData clientData,
						 int offset, char *buffer,
						 int maxBytes));
#ifdef KANJI
static int	TableFetchSelectionCtext _ANSI_ARGS_((ClientData clientData,
						      int offset, char *buffer,
						      int maxBytes));
#endif
static void	TableLostSelection _ANSI_ARGS_((ClientData clientData));
static int	TableValidate _ANSI_ARGS_((Table *tablePtr, char *cmd));
static int	TableValidateChange _ANSI_ARGS_((Table *tablePtr, int r,
						 int c, char *old, char *new,
						 int index));
static void	ExpandPercents _ANSI_ARGS_((Table *tablePtr, char *before,
					    int r, int c, char *old, char *new,
					    int index, Tcl_DString *dsPtr,
					    int cmdType));
static Tk_RestrictAction TableRestrictProc _ANSI_ARGS_((ClientData arg,
							XEvent *eventPtr));
static void	TableImageProc _ANSI_ARGS_((ClientData clientData, int x,
					    int y, int width, int height,
					    int imageWidth, int imageHeight));
static int	TableSortCompareProc _ANSI_ARGS_((CONST VOID *first,
						  CONST VOID *second));
static void	TableInvalidate _ANSI_ARGS_((Table * tablePtr, int x, int y,
					 int width, int height, int force));
static void	TableEventProc _ANSI_ARGS_((ClientData clientData,
					    XEvent *eventPtr));
static int	TableWidgetCmd _ANSI_ARGS_((ClientData clientData,
					    Tcl_Interp *interp,
					    int argc, char **argv));
static int	TableCmd _ANSI_ARGS_((ClientData clientData,
				      Tcl_Interp *interp,
				      int argc, char **argv));


/* The list of command values for all the widget commands */

#define CMD_ACTIVATE	1	/* activate command a la listbox */
#define CMD_BBOX	2	/* bounding box of cell <index> */
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
#define CMD_VALIDATE	22	/* validate contents of active cell */
#define	CMD_WIDTH	23	/* (re)set column widths */
#define CMD_XVIEW	24	/* change x view of widget (for scrollbars) */
#define CMD_YVIEW	25	/* change y view of widget (for scrollbars) */

/* The list of commands for the command parser */

static CmdStruct main_cmds[] = {
  {"activate",		CMD_ACTIVATE},
  {"bbox",		CMD_BBOX},
  {"cget",		CMD_CGET},
  {"configure",		CMD_CONFIGURE},
  {"curselection",	CMD_CURSELECTION},
  {"curvalue",		CMD_CURVALUE},
  {"delete",		CMD_DELETE},
  {"get",		CMD_GET},
  {"height",		CMD_HEIGHT},
  {"icursor",		CMD_ICURSOR},
  {"index",		CMD_INDEX},
  {"insert",		CMD_INSERT},
  {"reread",		CMD_REREAD},
  {"scan",		CMD_SCAN},
  {"see",		CMD_SEE},
  {"selection",		CMD_SELECTION},
  {"set",		CMD_SET},
  {"tag",		CMD_TAG},
  {"validate",		CMD_VALIDATE},
  {"width",		CMD_WIDTH},
  {"xview",		CMD_XVIEW},
  {"yview",		CMD_YVIEW},
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

static CmdStruct tag_cmds[] = {
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

/* list of selection subcommands */
#define SEL_ANCHOR      1	/* set selection anchor */
#define SEL_CLEAR       2	/* clear list from selection */
#define SEL_INCLUDES    3	/* query items inclusion in selection */
#define SEL_SET         4	/* include items in selection */

static CmdStruct sel_cmds[]= {
  {"anchor",	 SEL_ANCHOR},
  {"clear",	 SEL_CLEAR},
  {"includes",	 SEL_INCLUDES},
  {"set",	 SEL_SET},
  {"",		 0 }
};

/* insert/delete subcommands */
#define MOD_ACTIVE	1
#define MOD_COLS	2
#define MOD_ROWS	3
static CmdStruct mod_cmds[] = {
  {"active",	MOD_ACTIVE},
  {"cols",	MOD_COLS},
  {"rows",	MOD_ROWS},
  {"", 0}
};

/* drawmode values */
#define	DRAW_MODE_SLOW		1	/* The display redraws with a pixmap
					   using TK function calls */
#define	DRAW_MODE_TK_COMPAT	2	/* The redisplay is direct to the
					   screen, but TK function calls are
					   still used to give correct 3-d
					   border appearance and thus remain
					   compatible with other TK apps */
#define DRAW_MODE_FAST		4	/* the redisplay goes straight to
					   the screen and the 3d borders are
					   rendered with a single pixel wide
					   line only. It cheats and uses the
					   internal border structure to do
					   the borders */

static CmdStruct drawmode_vals[] = {
  {"fast",		DRAW_MODE_FAST},
  {"compatible",	DRAW_MODE_TK_COMPAT},
  {"slow",		DRAW_MODE_SLOW},
  {"", 0}
};

/* stretchmode values */
#define	STRETCH_MODE_NONE	1	/* No additional pixels will be
					   added to rows or cols */
#define	STRETCH_MODE_UNSET	2	/* All default rows or columns will
					   be stretched to fill the screen */
#define STRETCH_MODE_ALL	4	/* All rows/columns will be padded
					   to fill the window */
#define STRETCH_MODE_LAST	8	/* Stretch last elememt to fill
					   window */
#define STRETCH_MODE_FILL       16	/* More ROWS in Window */

static CmdStruct stretch_vals[] = {
  {"none",	STRETCH_MODE_NONE},
  {"unset",	STRETCH_MODE_UNSET},
  {"all",	STRETCH_MODE_ALL},
  {"last",	STRETCH_MODE_LAST},
  {"fill",	STRETCH_MODE_FILL},
  {"", 0}
};

/* The widget configuration table */
static Tk_CustomOption drawOption = { TableOptionSet, TableOptionGet,
				      (ClientData)(&drawmode_vals) };
static Tk_CustomOption stretchOption = { TableOptionSet, TableOptionGet,
					 (ClientData)(&stretch_vals) };

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
  {TK_CONFIG_STRING, "-browsecommand", "browseCommand", "BrowseCommand", "",
   Tk_Offset(Table, browseCmd), TK_CONFIG_NULL_OK},
  {TK_CONFIG_SYNONYM, "-browsecmd", "browseCommand", (char *) NULL,
   (char *) NULL, 0, TK_CONFIG_NULL_OK},
  {TK_CONFIG_INT, "-colorigin", "colOrigin", "Origin", "0",
   Tk_Offset(Table, colOffset), 0 },
  {TK_CONFIG_INT, "-cols", "cols", "Cols", "10",
   Tk_Offset(Table, cols), 0 },
  {TK_CONFIG_STRING, "-colseparator", "colSeparator", "Separator", NULL,
   Tk_Offset(Table, colSep), TK_CONFIG_NULL_OK },
  {TK_CONFIG_CUSTOM, "-colstretchmode", "colStretch", "StretchMode",
   "none", Tk_Offset (Table, colStretch), 0 , &stretchOption },
  {TK_CONFIG_STRING, "-coltagcommand", "colTagCommand", "TagCommand", NULL,
   Tk_Offset(Table, colTagCmd), TK_CONFIG_NULL_OK },
  {TK_CONFIG_ACTIVE_CURSOR, "-cursor", "cursor", "Cursor", "xterm",
   Tk_Offset(Table, cursor), TK_CONFIG_NULL_OK },
  {TK_CONFIG_CUSTOM, "-drawmode", "drawMode", "DrawMode", "compatible",
   Tk_Offset (Table, drawMode), 0, &drawOption },
  {TK_CONFIG_BOOLEAN, "-exportselection", "exportSelection",
   "ExportSelection", "1", Tk_Offset(Table, exportSelection), 0},
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
   Tk_Offset(Table, defaultTag.fontPtr), 0 },
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
  {TK_CONFIG_INT, "-roworigin", "rowOrigin", "Origin", "0",
   Tk_Offset(Table, rowOffset), 0 },
  {TK_CONFIG_INT, "-rows", "rows", "Rows", "10", Tk_Offset(Table, rows), 0 },
  {TK_CONFIG_STRING, "-rowseparator", "rowSeparator", "Separator", NULL,
   Tk_Offset(Table, rowSep), TK_CONFIG_NULL_OK },
  {TK_CONFIG_CUSTOM, "-rowstretchmode", "rowStretch", "StretchMode", "none",
   Tk_Offset (Table, rowStretch), 0 , &stretchOption },
  {TK_CONFIG_STRING, "-rowtagcommand", "rowTagCommand", "TagCommand", NULL,
   Tk_Offset(Table, rowTagCmd), TK_CONFIG_NULL_OK },
  {TK_CONFIG_STRING, "-selectmode", "selectMode", "SelectMode", "single",
   Tk_Offset(Table, selectMode), TK_CONFIG_NULL_OK },
  {TK_CONFIG_SYNONYM, "-selcmd", "selectionCommand", (char *) NULL,
   (char *) NULL, 0, TK_CONFIG_NULL_OK},
  {TK_CONFIG_STRING, "-selectioncommand", "selectionCommand",
   "SelectionCommand", NULL, Tk_Offset(Table, selCmd), TK_CONFIG_NULL_OK },
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
  {TK_CONFIG_BOOLEAN, "-validate", "validate", "Validate", "0",
   Tk_Offset(Table, validate), 0 },
  {TK_CONFIG_STRING, "-validatecommand", "validateCommand", "ValidateCommand",
   "", Tk_Offset(Table, valCmd), TK_CONFIG_NULL_OK},
  {TK_CONFIG_SYNONYM, "-vcmd", "validateCommand", (char *) NULL,
   (char *) NULL, 0, TK_CONFIG_NULL_OK},
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
 * The default specification for configuring tags
 * Done like this to make the command line parsing easy
 */

static Tk_ConfigSpec tagConfig[] = {
  {TK_CONFIG_ANCHOR, "-anchor", "anchor", "Anchor", NULL,
   Tk_Offset(TagStruct, anchor), 0 },
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
  {TK_CONFIG_FONT, "-font", "font", "Font", NULL,
   Tk_Offset(TagStruct, fontPtr), TK_CONFIG_NULL_OK },
#endif              /* KANJI */
  {TK_CONFIG_STRING, "-image", "image", "Image", "",
   Tk_Offset(TagStruct, imageStr), TK_CONFIG_NULL_OK},
  {TK_CONFIG_RELIEF, "-relief", "relief", "Relief", NULL,
   Tk_Offset(TagStruct, relief), TK_CONFIG_NULL_OK },
  {TK_CONFIG_END, (char *) NULL, (char *) NULL, (char *) NULL,
   (char *) NULL, 0, 0 }
};

