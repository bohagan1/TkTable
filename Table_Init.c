/* 
 *
 * Copyright (c) 1994 by Roland King
 * All rights reserved.
 * See the file COPYRIGHT for the copyright notes.
 *
*/

#include "Table.h"
#include "version.h"

static char *init_script[] = {
#include "TableInit.c"
(char *) NULL
};

/* 
** Function to call on loading the
** Table module 
*/

int
Tktable_Init(interp)
    Tcl_Interp *interp;
{
    char **p = init_script;
    Tcl_DString data;
    Tcl_DStringInit(&data);

    if (Tcl_PkgRequire(interp, "Tcl", TCL_VERSION, 0) == NULL) {
	return TCL_ERROR;
    }
/* 
    if (Tcl_PkgRequire(interp, "Tk", TK_VERSION, 0) == NULL) {
	return TCL_ERROR;
    }
 */
    if (Tcl_PkgProvide(interp, "Tktable", TKTABLE_VERSION) != TCL_OK) {
	return TCL_ERROR;
    }
    Tcl_CreateCommand(interp, "table", TableCmd,
	(ClientData) Tk_MainWindow(interp),(Tcl_CmdDeleteProc *)NULL);
    while(*p) {
        /* Copy the constant into a dynamic string. This */
        /* is necessary because Tcl7.4 doesn't accept    */
        /* constants as an argument to Tcl_Eval()        */
        Tcl_DStringSetLength(&data,0);
        Tcl_DStringAppend(&data,*p++,-1);
        if(Tcl_Eval(interp,Tcl_DStringValue(&data)) == TCL_ERROR) {
            Tcl_DStringFree(&data);
            return TCL_ERROR;
        }
    }
    Tcl_DStringFree(&data);
    return TCL_OK;
}


/* 
** The command for creating a new instance of a
** Table widget
*/

int TableCmd(clientdata, interp, argc, argv)
     ClientData clientdata;
     Tcl_Interp *interp;
     int argc;
     char **argv;
{
	Tk_Window main_window= (Tk_Window) clientdata;
	Table 		*tablePtr;	
	Tk_Window 	tkwin;

	if (argc<2) 
	{
		Tcl_AppendResult(interp, "wrong #args: should be \"",
				argv[0], "pathname ?options?\"", (char *)NULL);
		return TCL_ERROR;
	}

	tkwin=Tk_CreateWindowFromPath(interp, main_window, argv[1], (char *)NULL);
	if (tkwin==NULL)
		return TCL_ERROR;
	Tk_SetClass(tkwin, "Table");

	tablePtr=(Table *)malloc(sizeof(Table));
	tablePtr->tkwin=tkwin;
	tablePtr->display=Tk_Display(tkwin);
	tablePtr->interp=interp;
	tablePtr->rows=0;
	tablePtr->cols=0;
	tablePtr->defRowHeight=0;
	tablePtr->defColWidth=0;
	tablePtr->arrayVar=NULL;
	tablePtr->borderWidth=0;
	tablePtr->defaultTag.bgBorder=NULL;
	tablePtr->defaultTag.foreground=NULL;
	tablePtr->defaultTag.relief=TK_RELIEF_FLAT;
	tablePtr->defaultTag.fontPtr=NULL;
	tablePtr->defaultTag.anchor=TK_ANCHOR_CENTER;
	tablePtr->yScrollCmd=NULL;
	tablePtr->xScrollCmd=NULL;
	tablePtr->cursorBg=NULL;
	tablePtr->cursor=None;
	tablePtr->rowThenCol=1;
	tablePtr->selectionOn=1;
	tablePtr->topRow=0;
	tablePtr->leftCol=0;
	tablePtr->titleRows=0;
	tablePtr->titleCols=0;
	tablePtr->selRow=0;
	tablePtr->selCol=0;
	tablePtr->oldTopRow=-1;
	tablePtr->oldLeftCol=-1;
	tablePtr->oldSelRow=-1;
	tablePtr->oldSelCol=-1;
	tablePtr->textCurPosn=0;
	tablePtr->tableFlags= TBL_EDIT_ALLOWED;
	tablePtr->drawMode=DRAW_MODE_SLOW;
	tablePtr->colStretchMode=STRETCH_MODE_NONE;
	tablePtr->rowStretchMode=STRETCH_MODE_NONE;
	tablePtr->maxWidth=0;
	tablePtr->maxHeight=0;
	tablePtr->charWidth=0;
	tablePtr->colPixels=(int *)0;
	tablePtr->rowPixels=(int *)0;
	tablePtr->colStarts=(int *)0;
	tablePtr->rowStarts=(int *)0;
	tablePtr->colOffset=0;
	tablePtr->rowOffset=0;
	tablePtr->flashTime=8;
	tablePtr->flashTag=(tagStruct *)0;
	tablePtr->colWidths=NULL;
	tablePtr->rowHeights=NULL;
	tablePtr->tagTable=NULL;
	tablePtr->rowStyles=NULL;
	tablePtr->colStyles=NULL;
	tablePtr->cellStyles=NULL;
	tablePtr->flashCells=NULL;
	tablePtr->gcCache=NULL;
	tablePtr->cursorTimer=(Tk_TimerToken)NULL;
	tablePtr->flashTimer=(Tk_TimerToken)NULL;
	tablePtr->selectBuf=malloc(1);*(tablePtr->selectBuf)='\0';
	tablePtr->selectBufLen=1;
	tablePtr->rowTagProc=NULL;
	tablePtr->colTagProc=NULL;

	Tk_CreateEventHandler(	tkwin,
				ExposureMask | StructureNotifyMask | FocusChangeMask, TableEventProc,
				(ClientData) tablePtr);

	Tcl_CreateCommand(	interp, Tk_PathName(tkwin),
				TableWidgetCmd, (ClientData) tablePtr,
				(Tcl_CmdDeleteProc *)NULL);

	if (TableConfigure(interp, tablePtr, argc-2, argv+2, 0) != TCL_OK)
	{
		Tk_DestroyWindow(tkwin);
		return TCL_ERROR;
	}

	interp->result=Tk_PathName(tkwin);
	return TCL_OK;	 
}





