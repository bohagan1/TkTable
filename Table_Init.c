/* 
 *
 * Copyright (c) 1994 by Roland King
 * All rights reserved.
 * See the file COPYRIGHT for the copyright notes.
 *
*/

#include "Table.h"

/* 
** Function to call on loading the
** Table module 
*/

int Table_Init(Tcl_Interp *interp) {

  	static char initCmd[] =
		" if [file exists $tk_library/TableInit.tcl] {\n\
			source $tk_library/TableInit.tcl\n\
		} else { \n\
			set msg \"can't find $tk_library/TableInit.tcl; perhaps you \"\n\
            		append msg \"need to\\ninstall TableInit.tcl in your TK_LIBRARY directory?\"\n\
            		append msg \"environment variable?\"\n\
            		error $msg\n\
        	}";

	Tcl_CreateCommand(	interp, "table", TableCmd,
				(ClientData) Tk_MainWindow(interp),
				(Tcl_CmdDeleteProc *)NULL);

	return Tcl_Eval(interp, initCmd);
	
}


/* 
** The command for creating a new instance of a
** Table widget
*/

int TableCmd(	ClientData clientdata, 
			Tcl_Interp *interp, 
			int argc, char *argv[])
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





