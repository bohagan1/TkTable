/* 
 *
 * Copyright (c) 1994 by Roland King
 * All rights reserved.
 * See the file COPYRIGHT for the copyright notes.
 *
*/

#include "Table.h"

/* 
** The list of command values 
** for all the widget commands
*/
#define	CMD_CONFIGURE 	1	/* general configure command */
#define	CMD_WIDTH	2	/* (re)set column widths */
#define	CMD_HEIGHT	3	/* (re)set row heights */
#define	CMD_GETWIDTH	4	/* print the width of a column */
#define	CMD_GETHEIGHT	5	/* print the height of a column */
#define	CMD_TAG		6	/* tag command menu */
#define CMD_TOPROW	7	/* change the toprow displayed */
#define CMD_LEFTCOL	8	/* change the leftcol displayed*/
#define CMD_WHATCELL	9	/* get the cell at an (x,y) coord */
#define CMD_WHEREIS	10	/* where is the cell (row,col) */
#define CMD_SETCELL	11	/* set the current cell to row, col */
#define CMD_ICURSOR	12	/* set the insertion cursor */
#define CMD_INSERT	13	/* insert text at any position */
#define	CMD_DELETE	14	/* delete text in the selection */
#define	CMD_REREAD	15	/* reread the current selection */
#define CMD_EDITMODE	16	/* change edit mode */
#define CMD_DRAWMODE	17	/* change the drawing mode */
#define CMD_FLASH	18	/* set up the flash parameters */
#define	CMD_BATCH	19	/* command to turn batching on/off */
#define CMD_ROW_STRETCH	20	/* change the row stretching mode */
#define CMD_COL_STRETCH	21	/* change the row stretching mode */
#define CMD_CGET	22	/* get configuration value */

/* The list of commands for the command parser*/
command_struct commandos[]=
{
	{ "configure"	, CMD_CONFIGURE},
	{ "width"	, CMD_WIDTH},
	{ "height"	, CMD_HEIGHT},
	{ "getwidth"	, CMD_GETWIDTH},
	{ "getheight"	, CMD_GETHEIGHT},
	{ "tag"		, CMD_TAG},
	{ "toprow"	, CMD_TOPROW},
	{ "leftcol"	, CMD_LEFTCOL},
	{ "whatcell"	, CMD_WHATCELL},
	{ "whereis"	, CMD_WHEREIS},
	{ "setcell"	, CMD_SETCELL},
	{ "icursor"	, CMD_ICURSOR},
	{ "insert"	, CMD_INSERT},
	{ "delete"	, CMD_DELETE},
	{ "reread"	, CMD_REREAD},
	{ "editmode"	, CMD_EDITMODE},
	{ "drawmode"	, CMD_DRAWMODE},
	{ "flash"	, CMD_FLASH},
	{ "batch"	, CMD_BATCH},
	{ "rowstretch"	, CMD_ROW_STRETCH},
	{ "colstretch"	, CMD_COL_STRETCH},
	{ "cget"	, CMD_CGET},
	{ ""		, 0}
};

/* List of tag subcommands */
#define	TAG_NAMES	1	/* print the tag names */
#define	TAG_DELETE	2	/* delete a tag */
#define	TAG_CONFIGURE	3	/* config/create a new tag */
#define TAG_ROWTAG	4	/* tag a row */
#define TAG_COLTAG	5	/* tag a column */
#define TAG_CELLTAG	6	/* tag a cell */

command_struct tag_commands[]=
{
	{ "names"		, TAG_NAMES },
	{ "delete"		, TAG_DELETE },
	{ "configure"		, TAG_CONFIGURE },
	{ "rowtag"		, TAG_ROWTAG },
	{ "coltag"		, TAG_COLTAG },
	{ "celltag"		, TAG_CELLTAG },
	{ "" 			, 0 }
};


/* List of edit subcommands */
#define	EDIT_NOEDIT	1	/* editing is disbaled */
#define EDIT_AUTOCLEAR	2	/* selection is cleared when a key is pressed */
#define	EDIT_NOCLEAR	3	/* no clearing of the selection on first keypress */


command_struct editmode_commands[]=
{
	{ "off"			, EDIT_NOEDIT },
	{ "autoclear"		, EDIT_AUTOCLEAR },
	{ "noclear"		, EDIT_NOCLEAR },
	{ ""			, 0 }
};



/* drawmode subcommands */
command_struct drawmode_commands[]=
{
	{ "fast"		, DRAW_MODE_FAST },
	{ "compatible"		, DRAW_MODE_TK_COMPAT},
	{ "slow"		, DRAW_MODE_SLOW},
	{ ""			, 0 }
};


/* flash subcommands */
#define FLASH_MODE	1
#define	FLASH_TAG	2
#define	FLASH_TIMEOUT	3

command_struct flash_commands[]=
{
	{ "mode"		, FLASH_MODE },
	{ "tag"			, FLASH_TAG },
	{ "timeout"		, FLASH_TIMEOUT},
	{ ""			, 0 }
};


/* stretch subcommands */

command_struct stretch_commands[]=
{
	{ "none"		, STRETCH_MODE_NONE },
	{ "unset"		, STRETCH_MODE_UNSET },
	{ "all"			, STRETCH_MODE_ALL },
	{ ""			, 0 }
};




/* 
** The command procedure for a table widget
*/

int TableWidgetCmd(clientdata, interp, argc, argv)
     ClientData clientdata;
     Tcl_Interp *interp;
     int argc;
     char **argv;
{
	int result, retval, row, col, x, y, x1, y1, subcommand_retval;
	int paramParsed=0;
	Tcl_HashEntry *entryPtr;
	Tcl_HashSearch search;
	Tcl_HashTable  *hashTablePtr;
	int i, column, width, height, dummy, bd, newEntry;
	int entry, key, numfields, maxval, value, posn;
	int posnFrom, posnTo, oldlen, newlen;
	char buf1[50], buf2[50];

	Table *tablePtr=(Table *)clientdata;

	if(argc<2)
	{
		Tcl_AppendResult(	interp, "wrong # args: should be \"",
					argv[0], " option ?arg arg ..?\"",
					(char *)NULL);
		return  TCL_ERROR;
	}
	Tk_Preserve(clientdata);

	/* parse the first parameter */
	retval=parse_command(interp, commandos, argv[1]);

	/* Switch on the parameter value */
	switch(retval)
	{
	/* error */
	case 0:
		/* the return string is already set up */
		result=TCL_ERROR;
		break;

	/* configure */
	case CMD_CONFIGURE: 
		switch (argc) 
		{
		/* Only one argument, print the configuration */
		case 2:
			result=Tk_ConfigureInfo(interp, tablePtr->tkwin, TableConfig,
						(char *)tablePtr, (char *)NULL, 0);
			break;
		/* Two arguments, print the value for one variable */
		case 3:
			result=Tk_ConfigureInfo(interp, tablePtr->tkwin, TableConfig,
						(char *)tablePtr, argv[2], 0);
			break;
		/* More than two arguments, parse the values */
		default:
			result=TableConfigure(	interp, tablePtr, argc-2, argv+2, 
						 TK_CONFIG_ARGV_ONLY);
		}
		break;

	case CMD_CGET:
		switch (argc) {
		case 3:
		    result = Tk_ConfigureValue(interp, tablePtr->tkwin,
			TableConfig, (char *)tablePtr, argv[2], 0);
		    break;
		default:
		    Tcl_AppendResult(tablePtr->interp,
			"wrong # args should be \"",
			 argv[0], "-option", (char *)NULL);
		    result=TCL_ERROR;
		    break;
		}
                break;
	    
	/* changes the widths of certain selected columns */
	case CMD_WIDTH:
	case CMD_HEIGHT:
		bd=tablePtr->borderWidth;
		hashTablePtr=(retval==CMD_WIDTH)?(tablePtr->colWidths):(tablePtr->rowHeights);
		maxval=(retval==CMD_WIDTH)?(tablePtr->cols):(tablePtr->rows);

		switch (argc)
		{
		/* print out all the preset column widths or row heights */
		case 2:
			entryPtr=Tcl_FirstHashEntry(hashTablePtr, &search);
			while(entryPtr!=NULL)
			{
				key=(int)Tcl_GetHashKey(hashTablePtr, entryPtr); 
				if(key<maxval)
				{
					sprintf(buf1, "%d", key);
					entry=(int)Tcl_GetHashValue(entryPtr); sprintf(buf2, "%d", entry);
					Tcl_AppendResult(interp, "{",  buf1, " ", buf2, "} ", (char *)NULL);
				}
				entryPtr=Tcl_NextHashEntry(&search);
			}
			result=TCL_OK;	
			break;

		/* add an entry */
		default:
			result=TCL_OK;
			for(i=2;i<argc && result!=TCL_ERROR; i++)
			{
				numfields=sscanf(argv[i], "%d %d", &key, &entry);
				switch(numfields)
				{
				/* no fields, error */
				case 0:
					Tcl_AppendResult(interp, "{" , argv[i], "} invalid pair {", 
							argv[i], "} : usage {int ?int?} ...", (char *)NULL);
					result=TCL_ERROR;
					break;
	
				/* 1 field, reset that value */
				case 1:
					if((entryPtr=Tcl_FindHashEntry(hashTablePtr, (char *)key))!=NULL)
						Tcl_DeleteHashEntry(entryPtr);

					break;
							
				/* 2 fields - set the value */
				case 2:
					/* make sure the value is reasonable */
					entry=max(0,entry);

					entryPtr=Tcl_CreateHashEntry(hashTablePtr, (char *)key, &dummy);
					Tcl_SetHashValue(entryPtr, (ClientData)entry);

					break;
				}
			}
			TableAdjustParams(tablePtr);

			/* rerequest geometry */
			x=min(tablePtr->maxWidth,  max( Tk_Width(tablePtr->tkwin), tablePtr->maxReqWidth));
			y=min(tablePtr->maxHeight, max(Tk_Height(tablePtr->tkwin), tablePtr->maxReqHeight));
			Tk_GeometryRequest(tablePtr->tkwin, x, y);
	
			/*
			** Invalidate the whole window as TableAdjustParams
			** will only check to see if the top left cell has moved
			*/
			TableInvalidate(tablePtr, 0, 0, Tk_Width(tablePtr->tkwin), Tk_Height(tablePtr->tkwin), 0);
		}
		break;

	/* get the width of a column or the height of a row */
	case CMD_GETWIDTH:
	case CMD_GETHEIGHT:
		/* which hashtable do I need to look at */
		hashTablePtr=(retval==CMD_GETWIDTH)?(tablePtr->colWidths):(tablePtr->rowHeights);

		/* what is the maximum value of the parameter */
		maxval=(retval==CMD_GETWIDTH)?tablePtr->cols:tablePtr->rows;

		if(argc!=3 || sscanf(argv[2], "%d", &key)!=1 || (key<0) || (key>=maxval))
		{
			Tcl_AppendResult(	interp, "usage ", argv[0], 
						(retval==CMD_GETWIDTH) ? " getwidth" : " getheight",
						" arg", (char *)NULL);
			result=TCL_ERROR;
			break;
		}
		entryPtr=Tcl_FindHashEntry(hashTablePtr, (char *)key);
		if(entryPtr!=NULL)
			sprintf(buf1, "%d", (int)Tcl_GetHashValue(entryPtr));
		else
		{
			if(retval==CMD_GETWIDTH)
				sprintf(buf1, "%d", tablePtr->defColWidth);
			else
				sprintf(buf1, "%d", tablePtr->defRowHeight);
		}
		Tcl_AppendResult(interp, buf1, (char *)NULL);
		result=TCL_OK;
		break;		

	/* 
	** Top row or left column to display (after titles)
	** is being changed. Note that this is relative to
	** the title, so add back the number of row
	** and column titles to the value before we
	** set the internal variables which are absolute column
	** and row numbers. Usually used by the scrollbars anyway
	*/
	case CMD_TOPROW:
	case CMD_LEFTCOL:
		/* just a printout */
		if(argc==2)
		{
			char buf[150];
			if(retval==CMD_TOPROW)
				sprintf(buf, "%d", tablePtr->topRow);
			else
				sprintf(buf, "%d", tablePtr->leftCol);

			Tcl_AppendResult(interp, buf, (char *)NULL);
			result=TCL_OK;
			break;
		}
		if(argc>3 || (sscanf(argv[2], "%d", &value))!=1)
		{
			Tcl_AppendResult(	tablePtr->interp, "wrong # args should be \"",
						argv[0], (retval==CMD_TOPROW)?" toprow":" leftcol", 
						" arg", (char *)NULL);
			result=TCL_ERROR;
			break;
		}

		/* 
		** set the top row or left column
		** also adjust the selected cell 
		** to try to keep it on the screen
		*/
		if(retval==CMD_TOPROW)
		{
			tablePtr->topRow=value+tablePtr->titleRows;
			tablePtr->selRow+=(tablePtr->topRow-tablePtr->oldTopRow);
		}
		else
		{
			tablePtr->leftCol=value+tablePtr->titleCols;
			tablePtr->selCol+=(tablePtr->leftCol-tablePtr->oldLeftCol);
		}

		/* 
		** Do the table adjustment, which will 
		** also invalidate the table if necessary
		*/	
		TableAdjustParams(tablePtr);
		result=TCL_OK;
		break;
		
	/* a veritable plethora of tag commands */
	case CMD_TAG:
	/* do we have another argument */
		if(argc<3)
		{
			Tcl_AppendResult(	tablePtr->interp, "wrong # args should be \"",
						argv[0], " tag option ?arg arg ...?", (char *)NULL);
			result=TCL_ERROR;
			break;
		}

		/* all the rest is now done in a separate function */
		result=TableTagCommand(tablePtr, argc, argv);
		break; /* TAG FUNCTIONS */

	/* get the coords of the cell at the x,y address */
	case CMD_WHATCELL:
		if(argc!=4 || (sscanf(argv[2], "%d", &x)!=1) || (sscanf(argv[3], "%d", &y)!=1))
		{
			Tcl_AppendResult( tablePtr->interp, "wrong # args should be \"",
						argv[0], " whatcell arg arg ", (char *)NULL);
			result=TCL_ERROR;
			break;
		}
		TableWhatCell(tablePtr, x, y, &row, &col);
		sprintf(buf1, "%d %d", row+tablePtr->rowOffset, col+tablePtr->colOffset);
		Tcl_AppendResult(tablePtr->interp, buf1, (char *)NULL);
		result=TCL_OK;
		break; /* WHAT CELL */

	/* where is the cell (row,col) */
	case CMD_WHEREIS:
		if(argc!=4 || (sscanf(argv[2], "%d", &row)!=1) || (sscanf(argv[3], "%d", &col)!=1))
		{
			Tcl_AppendResult( tablePtr->interp, "wrong # args should be \"",
						argv[0], " whereis arg arg ", (char *)NULL);
			result=TCL_ERROR;
			break;
		}
		TableCellCoords(tablePtr, row-tablePtr->rowOffset, col-tablePtr->colOffset, &x, &y, &width, &height);
		sprintf(buf1, "%d %d %d %d", x, y, width, height);
		Tcl_AppendResult(tablePtr->interp, buf1, (char *)NULL);
		result=TCL_OK;
		break; /* WHERE IS */
	

	/* set the current cell */
	case CMD_SETCELL:
		/* printout only */
		if(argc==2)
		{
			sprintf(buf1, "%d %d", 
				tablePtr->selRow+tablePtr->rowOffset, 
				tablePtr->selCol+tablePtr->colOffset);
			Tcl_AppendResult(tablePtr->interp, buf1, (char *)NULL);
			result=TCL_OK;	
			break;
		}	
		if(argc!=4 || (sscanf(argv[2], "%d", &row)!=1) || (sscanf(argv[3], "%d", &col)!=1))
		{
			Tcl_AppendResult( tablePtr->interp, "wrong # args should be \"",
						argv[0], " setcell arg arg ", (char *)NULL);
			result=TCL_ERROR;
			break;
		}
		
		row-=tablePtr->rowOffset; col-=tablePtr->colOffset;
		
		row=min(max(tablePtr->titleRows, row), tablePtr->rows-1);
		col=min(max(tablePtr->titleCols, col), tablePtr->cols-1);

		tablePtr->selRow=row, tablePtr->selCol=col;

		/* Adjust the table for top left, selection on screen etc */
		TableAdjustParams(tablePtr);
		TableConfigCursor(tablePtr);
		result=TCL_OK;
		break;

	/* set the insertion cursor */
	case CMD_ICURSOR:
		switch(argc)
		{
		case 2:
			sprintf(buf1, "%d", tablePtr->textCurPosn);
			Tcl_AppendResult( tablePtr->interp, buf1, (char *)NULL);
			result=TCL_OK;
			break;
		case 3:
			if(TableParseStringPosn(tablePtr, argv[2], &posn)!=TCL_OK)
			{
				result=TCL_ERROR;
				break;
			}
			tablePtr->textCurPosn=posn;
			TableCellCoords(tablePtr, tablePtr->selRow, tablePtr->selCol, &x, &y, &width, &height);
			TableInvalidate(tablePtr, x, y, width, height, 1);
			result=TCL_OK;
			break;
		default:
			Tcl_AppendResult( tablePtr->interp, "wrong # args should be \"",
						argv[0], "\" icursor arg", (char *)NULL);
			result=TCL_ERROR;
			break;
		}
		break;

	case CMD_INSERT:
		/* are edits enabled */
		if(!(tablePtr->tableFlags&TBL_EDIT_ALLOWED))			
		{
			result=TCL_OK;
			break;
		}
		if(argc!=4)
		{
			Tcl_AppendResult( tablePtr->interp, "wrong # args should be \"",
						argv[0], "\" insert index text", (char *)NULL);
			result=TCL_ERROR;
			break;
		}
		if(TableParseStringPosn(tablePtr, argv[2], &posn)!=TCL_OK)
		{
			result=TCL_ERROR;
			break;
		}
		
		/* Is this an autoclear and this is the first update */
		if((tablePtr->tableFlags&TBL_1_KEY_DESTROY)&&!(tablePtr->tableFlags&TBL_TEXT_CHANGED))
		{
			/* set the buffer to be empty */
			tablePtr->selectBuf[0]='\0';

			/* mark the text as changed*/
			tablePtr->tableFlags|=TBL_TEXT_CHANGED;

			/* the insert position now has to be 0 */
			posn=0;
		}

		/* get the buffer to at least the right length */
		TableBufLengthen(tablePtr, strlen(tablePtr->selectBuf)+strlen(argv[3]));
		
		/* cache the old string length and the new*/
		oldlen=strlen(tablePtr->selectBuf); newlen=strlen(argv[3]);
		for(i=oldlen; i>=posn;i--)
			tablePtr->selectBuf[i+newlen]=tablePtr->selectBuf[i];
		for(i=0; i<newlen ; i++)
			tablePtr->selectBuf[posn+i]=argv[3][i];

		/* put the cursor after the insertion */
		tablePtr->textCurPosn=posn+newlen;

		TableCellCoords(tablePtr, tablePtr->selRow, tablePtr->selCol, &x, &y, &width, &height);
		TableInvalidate(tablePtr, x, y, width, height, 1);
		result=TCL_OK;
		break;

	case CMD_DELETE:
		/* are edits even enabled */
		if(!(tablePtr->tableFlags&TBL_EDIT_ALLOWED))			
		{
			result=TCL_OK;
			break;
		}

		if(argc>4)
		{
			Tcl_AppendResult( tablePtr->interp, "wrong # args should be \"",
						argv[0], "\" delete index ?index?", (char *)NULL);
			result=TCL_ERROR;
			break;
		}
		if(	(TableParseStringPosn(tablePtr, argv[2], &posnFrom)!=TCL_OK) ||
			((argc==4) && TableParseStringPosn(tablePtr, argv[3], &posnTo)!=TCL_OK))
		{
			result=TCL_ERROR;
			break;
		}
		if(argc==3)
			posnTo=posnFrom;
		else if(posnTo<posnFrom)
		{
			int tmp;
			tmp=posnFrom;
			posnFrom=posnTo;
			posnTo=tmp;
		}

		/* do the deletion */
		oldlen=strlen(tablePtr->selectBuf);
		for(i=0; i<=oldlen-posnTo; i++)
			tablePtr->selectBuf[i+posnFrom]=tablePtr->selectBuf[i+posnTo+1];

		/* make sure this string is null terminated */
		tablePtr->selectBuf[oldlen-(posnTo-posnFrom)]='\0';

		/* put the cursor at the deletion point */
		tablePtr->textCurPosn=posnTo;
	
		/* mark the text as changed */
		tablePtr->tableFlags|=TBL_TEXT_CHANGED;

		TableCellCoords(tablePtr, tablePtr->selRow, tablePtr->selCol, &x, &y, &width, &height);
		TableInvalidate(tablePtr, x, y, width, height,1);
		result=TCL_OK;
		break;

	/* this rereads the selection from the array - useful when <escape> is pressed */
	case CMD_REREAD:
		TableGetSelection(tablePtr);
		TableCellCoords(tablePtr, tablePtr->selRow, tablePtr->selCol, &x, &y, &width, &height);
		TableInvalidate(tablePtr, x, y, width, height,1);
		result=TCL_OK;
		break;
	
	/* change the edit mode */
	case CMD_EDITMODE:
		if(argc!=3)
		{
			Tcl_AppendResult( tablePtr->interp, "wrong # args should be \"",
						argv[0], "\" edit editmode", (char *)NULL);
			result=TCL_ERROR;
			break;
		}
		subcommand_retval=parse_command(tablePtr->interp, editmode_commands, argv[2]);
		switch(subcommand_retval)
		{
		/* not parsed break */
		case 0:
			result=TCL_ERROR;
			break;

		case EDIT_NOEDIT:
			tablePtr->tableFlags&=~TBL_EDIT_ALLOWED;
			break;
		case EDIT_AUTOCLEAR:
			tablePtr->tableFlags|=TBL_EDIT_ALLOWED;
			tablePtr->tableFlags|=TBL_1_KEY_DESTROY;
			break;
		case EDIT_NOCLEAR:
			tablePtr->tableFlags|=TBL_EDIT_ALLOWED;
			tablePtr->tableFlags&=~TBL_1_KEY_DESTROY;
			break;
		}
		/* reread the selection */
		TableGetSelection(tablePtr);
		TableConfigCursor(tablePtr);
		result=TCL_OK;
		break;

	case CMD_DRAWMODE:
		if(argc!=3)
		{
			Tcl_AppendResult( tablePtr->interp, "wrong # args should be \"",
						argv[0], "\" drawmode mode", (char *)NULL);
			result=TCL_ERROR;
			break;
		}
		subcommand_retval=parse_command(tablePtr->interp, drawmode_commands, argv[2]);
		if(subcommand_retval==0)
		{
		/* not parsed break */
			result=TCL_ERROR;
		}
		else
		{
			/* just set the drawing mode and refresh the sreen */
			tablePtr->drawMode=subcommand_retval;
			TableInvalidate(tablePtr, 0, 0, Tk_Width(tablePtr->tkwin),
					Tk_Height(tablePtr->tkwin),0);
			result=TCL_OK;
		}
		break;
	
	case CMD_FLASH:
		if(argc<3)
		{
			Tcl_AppendResult( tablePtr->interp, "wrong # args should be \"",
						argv[0], "\" flash arg ?arg?", (char *)NULL);
			result=TCL_ERROR;
			break;
		}
		subcommand_retval=parse_command(tablePtr->interp, flash_commands, argv[2]);
		/* what commmand */
		switch(subcommand_retval)
		{
		/* not parsed, break */
		case 0:
			result=TCL_ERROR;
			break;

		/* on or off */
		case FLASH_MODE:
			if(argc!=4 || Tcl_GetBoolean(tablePtr->interp, argv[3], &value)!=TCL_OK)
			{
				Tcl_AppendResult(	tablePtr->interp, "wrong # args : should be \"",
							argv[0], " flash mode on|off\"", (char *)NULL);
				result=TCL_ERROR;
				break;
			}
			/* set the mode and call the configuration routine */
			if(value)
			{
				tablePtr->tableFlags|=TBL_FLASH_ENABLED;
				TableFlashConfigure(tablePtr,1);
			}
			else
			{
				tablePtr->tableFlags&=~TBL_FLASH_ENABLED;
				TableFlashConfigure(tablePtr,0);
			}
			result=TCL_OK;
			break;

		case FLASH_TAG:
			{
				Tcl_HashEntry *entryPtr;
		
				if(argc!=4)
				{
					Tcl_AppendResult(	tablePtr->interp, "wrong # args : should be \"",
								argv[0], " flash tag <tagname>\"", (char *)NULL);
					result=TCL_ERROR;
					break;
				}

				/* is the tag name valid */
				if((entryPtr=Tcl_FindHashEntry(tablePtr->tagTable, argv[3]))==NULL)
				{
					Tcl_AppendResult(	tablePtr->interp, "invalid tag name \"",
								argv[3], "\"", (char *)NULL);
					result=TCL_ERROR;
					break;
				}

				/* put the tag pointer into the widget record */
				tablePtr->flashTag=(tagStruct *)Tcl_GetHashValue(entryPtr);
				TableFlashConfigure(tablePtr, 1);
				result=TCL_OK;
				break;
			}
		
		case FLASH_TIMEOUT:
			if(argc!=4 || Tcl_GetInt(tablePtr->interp, argv[3], &value)!=TCL_OK)
			{
				Tcl_AppendResult(	tablePtr->interp, "wrong # args : should be \"",
							argv[0], " flash timeout arg\"", (char *)NULL);
				result=TCL_ERROR;
				break;
			}
			tablePtr->flashTime=max(1,value);
			result=TCL_OK;
			break;
		} /* CMD_FLASH */
		break;

	case CMD_BATCH:
		if(argc!=3 || Tcl_GetBoolean(tablePtr->interp, argv[2], &value)!=TCL_OK)
		{
			Tcl_AppendResult(	tablePtr->interp, "wrong # args : should be \"",
						argv[0], " batch on|off\"", (char *)NULL);
			result=TCL_ERROR;
			break;
		}
		if(value)
			tablePtr->tableFlags|=TBL_BATCH_ON;
		else
			tablePtr->tableFlags&=~TBL_BATCH_ON;
		result=TCL_OK;
		break;

	case CMD_ROW_STRETCH:
	case CMD_COL_STRETCH:
		if(argc!=3)
		{
			Tcl_AppendResult( tablePtr->interp, "wrong # args should be \"",
						argv[0], (retval==CMD_ROW_STRETCH)?"row":"col","stretch mode", (char *)NULL);
			result=TCL_ERROR;
			break;
		}
		subcommand_retval=parse_command(tablePtr->interp, stretch_commands, argv[2]);
		if(subcommand_retval==0)
		{
		/* not parsed break */
			result=TCL_ERROR;
		}
		else
		{
			if(retval==CMD_ROW_STRETCH)
				tablePtr->rowStretchMode=subcommand_retval;
			else
				tablePtr->colStretchMode=subcommand_retval;

			TableAdjustParams(tablePtr);
			TableInvalidate(tablePtr, 0, 0, Tk_Width(tablePtr->tkwin),
					Tk_Height(tablePtr->tkwin),0);
			result=TCL_OK;
		}
		break;
	
		
	}
	Tk_Release(clientdata);
	return result;
}


/* 
** the tag commands are pretty big, so move them
** to their own routine
*/

int
TableTagCommand(tablePtr, argc, argv)
     Table *tablePtr;
     int argc;
     char **argv;
{
	int result, retval, i, newEntry, maxval, value;
	int row, col;
	int x, y, width, height;
	tagStruct *tagPtr;
	Tcl_HashEntry *entryPtr, *scanPtr, *newEntryPtr, *oldEntryPtr;
	Tcl_HashTable *hashTblPtr;
	Tcl_HashSearch search;
	char buf[150], *keybuf;

	/* parse the next argument */
	retval=parse_command(tablePtr->interp, tag_commands, argv[2]);
	switch(retval)
	{
	/* failed to parse the argument, error */
	case 0:
		return TCL_ERROR;

		/* just print out the tag names */
	case TAG_NAMES:
		entryPtr=Tcl_FirstHashEntry(tablePtr->tagTable, &search);
		while(entryPtr!=NULL)
		{
			Tcl_AppendResult(	tablePtr->interp, 
						Tcl_GetHashKey(tablePtr->tagTable, entryPtr)," ",
						(char *)NULL);
			entryPtr=Tcl_NextHashEntry(&search);
		}
		return TCL_OK;

	/* delete a tag */
	case TAG_DELETE:
		/* any arguments */
		if(argc<4)
		{
			Tcl_AppendResult(	tablePtr->interp, "wrong # args: should be \"",
						argv[0], " tag delete tagName ?tagName ...?", (char *)NULL);
			return TCL_ERROR;
		}

		/* run through the remaining arguments */
		for(i=3;i<argc;i++)
		{
			result=TCL_OK;
			/* cannot delete the Title tag */
			if(strcmp(argv[i], "Title")==0 || strcmp(argv[i], "sel")==0)
			{
				Tcl_AppendResult(tablePtr->interp, "cannot delete ",argv[i],
						 " tag", (char *)NULL);
				return TCL_ERROR;
			}
			if((entryPtr=Tcl_FindHashEntry(tablePtr->tagTable, argv[i]))!=NULL)
			{
				/* get the tag pointer */
				tagPtr=(tagStruct *)Tcl_GetHashValue(entryPtr);

				/* delete all references to this tag in rows */
				scanPtr=Tcl_FirstHashEntry(tablePtr->rowStyles, &search);
				for(;scanPtr!=NULL;scanPtr=Tcl_NextHashEntry(&search))
					if(Tcl_GetHashValue(scanPtr)==tagPtr)
						Tcl_DeleteHashEntry(scanPtr);

				/* delete all references to this tag in cols */
				scanPtr=Tcl_FirstHashEntry(tablePtr->colStyles, &search);
				for(;scanPtr!=NULL;scanPtr=Tcl_NextHashEntry(&search))
					if(Tcl_GetHashValue(scanPtr)==tagPtr)
						Tcl_DeleteHashEntry(scanPtr);

				/* delete all references to this tag in cells */
				scanPtr=Tcl_FirstHashEntry(tablePtr->cellStyles, &search);
				for(;scanPtr!=NULL;scanPtr=Tcl_NextHashEntry(&search))
					if(Tcl_GetHashValue(scanPtr)==tagPtr)
						Tcl_DeleteHashEntry(scanPtr);

				/* delete the reference in the flash tag */
				if(tablePtr->flashTag==tagPtr)
				{
					tablePtr->flashTag=NULL;
					TableFlashConfigure(tablePtr, 0);
				}

				/* release the structure */
				TableCleanupTag(tablePtr, (tagStruct *)Tcl_GetHashValue(entryPtr));
				free((tagStruct *)Tcl_GetHashValue(entryPtr));

				/* and free the hash table entry */
				Tcl_DeleteHashEntry(entryPtr);
			}
		}
		/* since we deleted a tag, redraw the screen */
		TableInvalidate(tablePtr, 0, 0, Tk_Width(tablePtr->tkwin), Tk_Height(tablePtr->tkwin),0);
		return result;
		
	case TAG_CONFIGURE:
		if(argc<4)
		{
			Tcl_AppendResult(	tablePtr->interp, "wrong # args: should be \"",
						argv[0], " tag configure tagName ?arg arg  ...?", 
						(char *)NULL);
			return TCL_ERROR;
		}
			
		/* first see if this is a reconfiguration */
		entryPtr=Tcl_FindHashEntry(tablePtr->tagTable, argv[3]);
		if(entryPtr==NULL)
		{
			entryPtr=Tcl_CreateHashEntry(tablePtr->tagTable, argv[3], &newEntry);

			/* create the structure */
			tagPtr=TableNewTag(tablePtr);
				
			/* insert it into the table */
			Tcl_SetHashValue(entryPtr, (ClientData)tagPtr);

			/* configure the tag structure */
			result=Tk_ConfigureWidget(	tablePtr->interp, tablePtr->tkwin, tagConfig, 
								argc-4, argv+4, (char *)tagPtr, 0);
			if(result==TCL_ERROR)
				return TCL_ERROR;

		}
		/*
		** pointer wasn't null, do a reconfig if we 
		** have enough arguments
		*/
		else 
		{
			/* get the tag pointer from the table*/
			tagPtr=(tagStruct *)Tcl_GetHashValue(entryPtr);
				
			/* 5 args means that there are values to replace */
			if(argc>5)
			{
				/* and do a reconfigure */
				result=Tk_ConfigureWidget(	tablePtr->interp, tablePtr->tkwin, tagConfig,
								argc-4, argv+4, (char*)tagPtr, TK_CONFIG_ARGV_ONLY);
				if(result==TCL_ERROR)
					return TCL_ERROR;
			}
		}
		/* 
		** If there were less than 6 args, we need
		** to do a printout of the config, even for
		** new tags
		*/
		if(argc<6)
		{
			result=Tk_ConfigureInfo(tablePtr->interp, tablePtr->tkwin, 
						tagConfig, (char *)tagPtr, 
						(argc==5) ? argv[4] : 0, 0);
		}
		/* 
		** Otherwise we reconfigured so invalidate the table 
		** for a redraw
		*/
		else
			TableInvalidate(tablePtr, 0, 0, Tk_Width(tablePtr->tkwin), Tk_Height(tablePtr->tkwin),0);
		return result;
	

 	/* tag a row or a column */
	case TAG_ROWTAG:
	case TAG_COLTAG:
		if (argc<4)
		{
			Tcl_AppendResult(tablePtr->interp, "wrong # args: should be \"", argv[0], 
					(retval==TAG_ROWTAG) ? " rowtag" : " coltag",
					 " tag ?arg arg ..?\"", (char *)NULL);
			return TCL_ERROR;
		}

		/* if the tag is null, we are deleting */
		if(!(*argv[3]))
			tagPtr=NULL;
		
		else	/* check to see if the tag actually exists */
		{
			if((entryPtr=Tcl_FindHashEntry(tablePtr->tagTable, argv[3]))==NULL)
			{
				Tcl_AppendResult(tablePtr->interp, "invalid tag name \"", argv[3], "\"", (char *)NULL);
				return TCL_ERROR;	
			}
			/* get the pointer to the tag structure */
			tagPtr=(tagStruct *)Tcl_GetHashValue(entryPtr);
		}
	
		/* and choose the correct hash table */
		hashTblPtr=(retval==TAG_ROWTAG) ? tablePtr->rowStyles : tablePtr->colStyles;

		/* No more args -> display only */
		if(argc==4)
		{
			for(	scanPtr=Tcl_FirstHashEntry(hashTblPtr, &search);
				scanPtr!=NULL ; scanPtr=Tcl_NextHashEntry(&search))
				/* is this the tag pointer on this row */
				if((tagStruct *)Tcl_GetHashValue(scanPtr)==tagPtr)
				{
					sprintf(buf, "%d ", (int)Tcl_GetHashKey(hashTblPtr, scanPtr));
					Tcl_AppendResult(tablePtr->interp, buf, (char *)NULL);
				}
			return TCL_OK;
		}
		/* Now loop through the arguments and fill in the hash table */
		for(i=4;i<argc;i++)
		{
			/* can I parse this argument */
			if(	sscanf(argv[i], "%d", &value)!=1)
			{
				Tcl_AppendResult(tablePtr->interp, "bad value \"", argv[i], "\"", (char *)NULL);
				return TCL_ERROR;
			}

			/* deleting or adding */
			if(tagPtr==NULL)
			{
				oldEntryPtr=Tcl_FindHashEntry( hashTblPtr, (char *)value);
				if(oldEntryPtr!=NULL)
					Tcl_DeleteHashEntry(oldEntryPtr);
			}
			else
			{
				/* add a key to the hash table */
				newEntryPtr=Tcl_CreateHashEntry( hashTblPtr, (char *)value, &newEntry);
			
				/* and set it to point to the Tag structure */
				Tcl_SetHashValue(newEntryPtr, (ClientData)tagPtr);
			}
			/* and invalidate the row or column affected */
			if(retval==TAG_ROWTAG)
			{
				/* get the position of the leftmost cell in the row */
				TableCellCoords(tablePtr, value-tablePtr->rowOffset, 0, &x, &y, &width, &height);
				
				/* Invalidate the row */
				TableInvalidate(tablePtr, 0, y, Tk_Width(tablePtr->tkwin), height,0);
			}
			else
			{
				/* get the position of the topmost cell on the column */
				TableCellCoords(tablePtr, 0, value-tablePtr->colOffset, &x, &y, &width, &height);
				
				/* Invalidate the column */
				TableInvalidate(tablePtr, x, 0, width, Tk_Height(tablePtr->tkwin),0);
			}
		}
		return TCL_OK;

 	/* tag a cell */
	case TAG_CELLTAG:
		if (argc<4)
		{
			Tcl_AppendResult(tablePtr->interp, "wrong # args: should be \"", argv[0], 
					" celltag", " tag ?arg arg ..?\"", (char *)NULL);
			return TCL_ERROR;
		}

		/* are we deleting */
		if(!(*argv[3]))
			tagPtr=NULL;
		else
		{
			/* check to see if the tag actually exists */
			if((entryPtr=Tcl_FindHashEntry(tablePtr->tagTable, argv[3]))==NULL)
			{
				Tcl_AppendResult(tablePtr->interp, "invalid tag name \"", argv[3], "\"", (char *)NULL);
				return TCL_ERROR;	
			}
			/* get the pointer to the tag structure */
			tagPtr=(tagStruct *)Tcl_GetHashValue(entryPtr);
		}
	
		/* No more args -> display only */
		if(argc==4)
		{
			for(	scanPtr=Tcl_FirstHashEntry(tablePtr->cellStyles, &search);
				scanPtr!=NULL ; scanPtr=Tcl_NextHashEntry(&search))
				/* is this the tag pointer for this cell */
				if((tagStruct *)Tcl_GetHashValue(scanPtr)==tagPtr)
				{	
					keybuf=Tcl_GetHashKey(tablePtr->cellStyles, scanPtr);
		
					/* Split the value into its two components */
					if (TableParseArrayIndex(tablePtr, &row, &col, keybuf) != 2)
					    return TCL_ERROR;
					sprintf(buf, "{%d %d} ", row, col);
					Tcl_AppendResult(tablePtr->interp, buf, (char *)NULL);
				}
			return TCL_OK;
		}
		/* Now loop through the arguments and fill in the hash table */
		for(i=4;i<argc;i++)
		{
			/* can I parse this argument */
			if(	sscanf(argv[i], "%d %d", &row, &col)!=2) 
			{
				Tcl_AppendResult(tablePtr->interp, "bad value \"", argv[i], "\"", (char *)NULL);
				return TCL_ERROR;
			}
			/* get the hash key ready */
			TableMakeArrayIndex(tablePtr, row, col, buf);
			
			/* is this a deletion */
			if(tagPtr==NULL)
			{
				oldEntryPtr=Tcl_FindHashEntry(tablePtr->cellStyles, buf);
				if(oldEntryPtr!=NULL)
					Tcl_DeleteHashEntry(oldEntryPtr);
			}
			else
			{
				/* add a key to the hash table */
				newEntryPtr=Tcl_CreateHashEntry( tablePtr->cellStyles, buf, &newEntry);
			
				/* and set it to point to the Tag structure */
				Tcl_SetHashValue(newEntryPtr, (ClientData)tagPtr);
			}
			/* now invalidate the area */
			TableCellCoords(tablePtr, row-tablePtr->rowOffset, col-tablePtr->colOffset, &x, &y, &width, &height);
			TableInvalidate(tablePtr, x, y, width, height,0);
		}
		return TCL_OK;
	}
}



/* 
** Parses a command string passed in in arg comparing it with all the command strings
** in the command array. If it finds a string which is a unique identifier of one of the
** commands, returns the index . If none of the commands match, or the abbreviation is not 
** unique, then it sets up the message accordingly and returns 0
*/

int parse_command(interp, commands, arg)
     Tcl_Interp *interp;
     command_struct *commands;
     char *arg;
{
	int len=strlen(arg);
	command_struct *matched=(command_struct *)0;
	int err=0;
	command_struct *next=commands;

	while(*(next->name))
	{
		if(strncmp(next->name, arg, len)==0)
		{
			/* 
			** have we already matched this one 
			** if so make up an error message 
			*/
			if(matched)
			{
				if(!err)
				{
					Tcl_AppendResult(	interp, "ambiguous option \"", arg,
								"\" could be ", matched->name, (char *)0);
					matched=next;
					err=1;
				}
				Tcl_AppendResult(interp, ", ", next->name, (char *)0);
			}
			else
				matched=next;
		}
		next++;
	}

	/* did we get multiple possibilities */
	if(err)
		return 0;

	/* did we match any at all */
	if(matched)
	{
		return matched->value;
	}
	else
	{
		Tcl_AppendResult(	interp, "bad option \"", arg,
					"\" must be ", (char *)NULL);
		next=commands;
		while(1)
		{
			Tcl_AppendResult(interp, next->name, (char *)NULL);

			/* the end of them all ?*/
			if(!*((++next)->name))
				return 0;

			/* or the last one at least */
			if(*((next+1)->name))
				Tcl_AppendResult(interp, ", ", (char *)NULL);
			else
				Tcl_AppendResult(interp, " or ", (char *)NULL);
		}
	}
}



/* 
** Get the current selection into the
** buffer and mark it as unedited
** set the position to the end of the string
*/
void
TableGetSelection(tablePtr)
     Table *tablePtr;
{
	char *data;
	char buf[100];

	TableMakeArrayIndex(	tablePtr, 
				tablePtr->selRow+tablePtr->rowOffset, tablePtr->selCol+tablePtr->colOffset, 
				buf) ;

	if	(tablePtr->arrayVar==NULL || 
		(data=Tcl_GetVar2(tablePtr->interp, tablePtr->arrayVar, buf, TCL_GLOBAL_ONLY))==NULL)
	{
		data="";
	}
	
	/* is the buffer long enough */
	TableBufLengthen(tablePtr, strlen(data));
	
	strcpy(tablePtr->selectBuf, data);
	tablePtr->textCurPosn=strlen(data);
	tablePtr->tableFlags &=~TBL_TEXT_CHANGED;
}


/* Turn row/col into an index into the table */
void 
TableMakeArrayIndex(tablePtr, row, col, buf)
     Table *tablePtr;
     int row;
     int col;
     char *buf;
{
	if(tablePtr->rowThenCol)
		sprintf(buf, "%d,%d", row, col);
	else
		sprintf(buf, "%d,%d", col, row);
}

/* 
**Turn array index back into row/col 
** return the number of args parsed, 
** should be two 
*/
int
TableParseArrayIndex(tablePtr, row, col, index)
     Table *tablePtr;
     int *row;
     int *col;
     char *index;
{
	if(tablePtr->rowThenCol)
	 		return sscanf(index, "%d,%d", row, col);
	else
	 		return sscanf(index, "%d,%d", col, row);
}

/*
** Parse the argument as an index into the
** selected cell text.
** recognises 'end' or an integer or insert
** constrains it to the size of the buffer 
*/
int
TableParseStringPosn(tablePtr, arg, posn)
     Table *tablePtr;
     char *arg;
     int *posn;
{
  	int temp;

	/* is this end */
	if(strncmp(arg, "end", min(strlen(arg), 3))==0)
		*posn=strlen(tablePtr->selectBuf);
	else if (strncmp(arg, "insert", min(strlen(arg), 6))==0)
		*posn=tablePtr->textCurPosn;
	else
	{
		if(sscanf(arg, "%d", &temp)!=1)
		{
			Tcl_AppendResult(tablePtr->interp, "error \"", arg, 
					"\" is not a valid index", (char *)NULL);		
			return TCL_ERROR;
		}
		*posn= min( max(0,temp) , strlen(tablePtr->selectBuf));
	}
	return TCL_OK;
}


/*
** Makes the selection buffer long enough to hold
** a string of length len PLUS an extra character
** for the \0. Copies over the data if it lengthens
** the buffer and frees the old one
*/
void
TableBufLengthen(tablePtr, len)
     Table *tablePtr;
     int len;
{
	char *newbuf;
	if(tablePtr->selectBufLen<len+1)
	{
		tablePtr->selectBufLen=(int)(len*1.25)+8;
		newbuf=malloc(tablePtr->selectBufLen);
		if(tablePtr->selectBuf)
		{
			/* copy over the data */
			strcpy(newbuf, tablePtr->selectBuf);

			/* free the old buffer */
			free(tablePtr->selectBuf);			
		}
		tablePtr->selectBuf=newbuf;
	}
}
