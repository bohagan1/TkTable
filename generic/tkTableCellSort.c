/*
 * tkTableCell.c --
 *
 *	This module implements cell sort functions for table
 *	widgets.  The MergeSort algorithm and other aux sorting
 *	functions were taken from tclCmdIL.c lsort command:

 * tclCmdIL.c --
 *
 *	This file contains the top-level command routines for most of
 *	the Tcl built-in commands whose names begin with the letters
 *	I through L.  It contains only commands in the generic core
 *	(i.e. those that don't depend much upon UNIX facilities).
 *
 * Copyright (c) 1987-1993 The Regents of the University of California.
 * Copyright (c) 1993-1997 Lucent Technologies.
 * Copyright (c) 1994-1997 Sun Microsystems, Inc.
 * Copyright (c) 1998-1999 by Scriptics Corporation.

 *
 * Copyright (c) 1998-2002 Jeffrey Hobbs
 * Copyright (c) 2025      Brian O'Hagan
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include "tkTable.h"

#ifndef UCHAR
#define UCHAR(c) ((unsigned char) (c))
#endif

/*
 * During execution of the "lsort" command, structures of the following
 * type are used to arrange the objects being sorted into a collection
 * of linked lists.
 */

typedef struct SortElement {
    Tcl_Obj *objPtr;			/* Object being sorted. */
    struct SortElement *nextPtr;        /* Next element in the list, or
					 * NULL for end of list. */
} SortElement;


/*
 *----------------------------------------------------------------------
 *
 * DictionaryCompare - Not the Unicode version
 *
 *	This function compares two strings as if they were being used in
 *	an index or card catalog.  The case of alphabetic characters is
 *	ignored, except to break ties.  Thus "B" comes before "b" but
 *	after "a".  Also, integers embedded in the strings compare in
 *	numerical order.  In other words, "x10y" comes after "x9y", not
 *      before it as it would when using strcmp().
 *
 * Results:
 *      A negative result means that the first element comes before the
 *      second, and a positive result means that the second element
 *      should come first.  A result of zero means the two elements
 *      are equal and it doesn't matter which comes first.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static int DictionaryCompare(
    char *left, char *right) {         /* The strings to compare */

    int diff, zeros;
    int secondaryDiff = 0;

    while (1) {
	if (isdigit(UCHAR(*right)) && isdigit(UCHAR(*left))) {
	    /*
	     * There are decimal numbers embedded in the two
	     * strings.  Compare them as numbers, rather than
	     * strings.  If one number has more leading zeros than
	     * the other, the number with more leading zeros sorts
	     * later, but only as a secondary choice.
	     */

	    zeros = 0;
	    while ((*right == '0') && (isdigit(UCHAR(right[1])))) {
		right++;
		zeros--;
	    }
	    while ((*left == '0') && (isdigit(UCHAR(left[1])))) {
		left++;
		zeros++;
	    }
	    if (secondaryDiff == 0) {
		secondaryDiff = zeros;
	    }

	    /*
	     * The code below compares the numbers in the two
	     * strings without ever converting them to integers.  It
	     * does this by first comparing the lengths of the
	     * numbers and then comparing the digit values.
	     */

	    diff = 0;
	    while (1) {
		if (diff == 0) {
		    diff = UCHAR(*left) - UCHAR(*right);
		}
		right++;
		left++;
		if (!isdigit(UCHAR(*right))) {
		    if (isdigit(UCHAR(*left))) {
			return 1;
		    } else {
			/*
			 * The two numbers have the same length. See
			 * if their values are different.
			 */

			if (diff != 0) {
			    return diff;
			}
			break;
		    }
		} else if (!isdigit(UCHAR(*left))) {
		    return -1;
		}
	    }
	    continue;
	}
	diff = UCHAR(*left) - UCHAR(*right);
	if (diff) {
	    if (isupper(UCHAR(*left)) && islower(UCHAR(*right))) {
		diff = UCHAR(tolower(*left)) - UCHAR(*right);
		if (diff) {
		    return diff;
		} else if (secondaryDiff == 0) {
		    secondaryDiff = -1;
		}
	    } else if (isupper(UCHAR(*right)) && islower(UCHAR(*left))) {
		diff = UCHAR(*left) - UCHAR(tolower(UCHAR(*right)));
		if (diff) {
		    return diff;
		} else if (secondaryDiff == 0) {
		    secondaryDiff = 1;
		}
	    } else {
		return diff;
	    }
	}
	if (*left == 0) {
	    break;
	}
	left++;
	right++;
    }
    if (diff == 0) {
	diff = secondaryDiff;
    }
    return diff;
}

/*
 *----------------------------------------------------------------------
 *
 * MergeLists -
 *
 *	This procedure combines two sorted lists of SortElement structures
 *	into a single sorted list.
 *
 * Results:
 *      The unified list of SortElement structures.
 *
 * Side effects:
 *	None, unless a user-defined comparison command does something
 *	weird.
 *
 *----------------------------------------------------------------------
 */

static SortElement * MergeLists(
    SortElement *leftPtr,              /* First list to be merged; may be NULL. */
    SortElement *rightPtr) {           /* Second list to be merged; may be NULL. */

    SortElement *headPtr;
    SortElement *tailPtr;

    if (leftPtr == NULL) {
	return rightPtr;
    }
    if (rightPtr == NULL) {
	return leftPtr;
    }
    if (DictionaryCompare(Tcl_GetString(leftPtr->objPtr),
		Tcl_GetString(rightPtr->objPtr)) > 0) {
	tailPtr = rightPtr;
	rightPtr = rightPtr->nextPtr;
    } else {
	tailPtr = leftPtr;
	leftPtr = leftPtr->nextPtr;
    }
    headPtr = tailPtr;
    while ((leftPtr != NULL) && (rightPtr != NULL)) {
	if (DictionaryCompare(Tcl_GetString(leftPtr->objPtr),
		Tcl_GetString(rightPtr->objPtr)) > 0) {
	    tailPtr->nextPtr = rightPtr;
	    tailPtr = rightPtr;
	    rightPtr = rightPtr->nextPtr;
	} else {
	    tailPtr->nextPtr = leftPtr;
	    tailPtr = leftPtr;
	    leftPtr = leftPtr->nextPtr;
	}
    }
    if (leftPtr != NULL) {
       tailPtr->nextPtr = leftPtr;
    } else {
       tailPtr->nextPtr = rightPtr;
    }
    return headPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * MergeSort -
 *
 *	This procedure sorts a linked list of SortElement structures
 *	use the merge-sort algorithm.
 *
 * Results:
 *      A pointer to the head of the list after sorting is returned.
 *
 * Side effects:
 *	None, unless a user-defined comparison command does something
 *	weird.
 *
 *----------------------------------------------------------------------
 */

static SortElement * MergeSort(
    SortElement *headPtr) {            /* First element on the list */
    /*
     * The subList array below holds pointers to temporary lists built
     * during the merge sort.  Element i of the array holds a list of
     * length 2**i.
     */

#   define NUM_LISTS 30
    SortElement *subList[NUM_LISTS];
    SortElement *elementPtr;
    int i;

    for(i = 0; i < NUM_LISTS; i++){
	subList[i] = NULL;
    }
    while (headPtr != NULL) {
	elementPtr = headPtr;
	headPtr = headPtr->nextPtr;
	elementPtr->nextPtr = 0;
	for (i = 0; (i < NUM_LISTS) && (subList[i] != NULL); i++){
	    elementPtr = MergeLists(subList[i], elementPtr);
	    subList[i] = NULL;
	}
	if (i >= NUM_LISTS) {
	    i = NUM_LISTS-1;
	}
	subList[i] = elementPtr;
    }
    elementPtr = NULL;
    for (i = 0; i < NUM_LISTS; i++){
	elementPtr = MergeLists(subList[i], elementPtr);
    }
    return elementPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * TableCellSortObj --
 *	Sorts a list of table cell elements (of form row,col) in place
 *
 * Results:
 *	Sorts list of elements in place.
 *
 * Side effects:
 *	Behaviour undefined for ill-formed input list of elements.
 *
 *----------------------------------------------------------------------
 */

Tcl_Obj * TableCellSortObj(Tcl_Interp *interp, Tcl_Obj *listObjPtr) {
    int length, i;
    Tcl_Size len;
    Tcl_Obj *sortedObjPtr, **listObjPtrs;
    SortElement *elementArray;
    SortElement *elementPtr;

    if (Tcl_ListObjGetElements(interp, listObjPtr, &len, &listObjPtrs) != TCL_OK) {
	return NULL;
    }
    length = (int) len;
    if (length <= 0) {
	return listObjPtr;
    }

    elementArray = (SortElement *) ckalloc(length * sizeof(SortElement));
    for (i=0; i < length; i++){
	elementArray[i].objPtr = listObjPtrs[i];
	elementArray[i].nextPtr = &elementArray[i+1];
	Tcl_IncrRefCount(listObjPtrs[i]);
    }
    elementArray[length-1].nextPtr = NULL;
    elementPtr = MergeSort(elementArray);
    sortedObjPtr = Tcl_NewObj();
    for (; elementPtr != NULL; elementPtr = elementPtr->nextPtr) {
	Tcl_ListObjAppendElement(NULL, sortedObjPtr, elementPtr->objPtr);
    }
    ckfree((char*) elementArray);
    for (i=0; i < length; i++){
	Tcl_DecrRefCount(listObjPtrs[i]);
    }

    return sortedObjPtr;
}

/*
 * Structure used to store cell index info for sort operation.
 */

typedef struct indexTypeStruct {
    int row, col;
    char *hashKey;
} indexType;

/*
 *--------------------------------------------------------------
 *
 * CellIndexComp --
 *	Compares two cell index values and returns the result.
 *
 * Results:
 *	-1 if index1 before, 0 if same, or 1 if after index2.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */

int CellIndexComp(const void *index1, const void *index2) {
    int res;
    indexType *a, *b;
    a = (indexType*) index1;
    b = (indexType*) index2;

    if (a->row < b->row || (a->row == b->row && a->col < b->col)) {
	res = -1;
    } else if (a->row > b->row || (a->row == b->row && a->col > b->col)) {
	res = 1;
    } else {
	res = 0;
    }
    return res;
}

/*
 *--------------------------------------------------------------
 *
 * TableGetSortedIndices --
 *	This returns the cell indices (in row,col format) from a
 *	hash table as a sorted list.
 *
 * Results:
 *	A Tcl list object or NULL for error.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */

Tcl_Obj * TableGetSortedIndices(Tcl_HashTable *hashTablePtr) {
    Tcl_Size numEntries, index = 0;
    Tcl_HashEntry *entryPtr;
    Tcl_HashSearch search;
    indexType *indexArray;
    Tcl_Obj *listPtr, *tempPtr;

    /* Abort if no entries */
    numEntries = hashTablePtr->numEntries;
    if (numEntries == 0) {
	return Tcl_NewListObj(0, NULL);
    }

    /* Allocate storage */
    listPtr = Tcl_NewListObj(numEntries, NULL);
    indexArray = (indexType *)ckalloc((size_t)numEntries * sizeof(indexType));
    if (listPtr == NULL || indexArray == NULL) {
	return NULL;
    }

    /* Get entries and add to array */
    for (entryPtr = Tcl_FirstHashEntry(hashTablePtr, &search);
	    entryPtr != NULL && index < numEntries;
	    entryPtr = Tcl_NextHashEntry(&search), index++) {
	indexArray[index].hashKey = Tcl_GetHashKey(hashTablePtr, entryPtr);
	TableParseArrayIndex(&(indexArray[index].row), &(indexArray[index].col),
	    indexArray[index].hashKey);
    }
    if (index < numEntries) {
	numEntries = index;
    }

    /* Sort entries */
    qsort((void *)indexArray, (size_t)numEntries, sizeof(indexType), CellIndexComp);

    /* Convert array entries to TCL list */
    for (index = 0; index < numEntries; index++) {
	if ((tempPtr = Tcl_NewStringObj(indexArray[index].hashKey, -1)) != NULL
	    && Tcl_ListObjAppendElement(NULL, listPtr, tempPtr) == TCL_OK) {
	} else {
	    Tcl_IncrRefCount(listPtr);
	    Tcl_DecrRefCount(listPtr);
	    listPtr = NULL;
	    break;
	}
    }
    ckfree((char *) indexArray);
    return listPtr;
}

/*
 *--------------------------------------------------------------
 *
 * TableGetSortedIndicesFiltered --
 *	This returns the cell indices (in row,col format) from a
 *	hash table that match the pattern criteria as a sorted list.
 *
 * Results:
 *	A Tcl list object or NULL for error.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */

Tcl_Obj * TableGetSortedIndicesFiltered(Tcl_HashTable *hashTablePtr, int has_value, char *pattern) {
    Tcl_Size numEntries, index = 0;
    Tcl_HashEntry *entryPtr;
    Tcl_HashSearch search;
    indexType *indexArray;
    Tcl_Obj *listPtr, *tempPtr;
    char *hashKey;

    /* Abort if no entries */
    numEntries = hashTablePtr->numEntries;
    if (numEntries == 0) {
	return Tcl_NewListObj(0, NULL);
    }

    /* Allocate storage */
    listPtr = Tcl_NewListObj(numEntries, NULL);
    indexArray = (indexType *)ckalloc((size_t)numEntries * sizeof(indexType));
    if (listPtr == NULL || indexArray == NULL) {
	return NULL;
    }

    /* Get entries, apply patterns, and add to array */
    for (entryPtr = Tcl_FirstHashEntry(hashTablePtr, &search);
	    entryPtr != NULL && index < numEntries;
	    entryPtr = Tcl_NextHashEntry(&search)) {
	hashKey = Tcl_GetHashKey(hashTablePtr, entryPtr);
	if ((has_value && Tcl_GetHashValue(entryPtr) != NULL) ||
		(pattern != NULL && Tcl_StringMatch(hashKey, pattern)) ||
		(!has_value && pattern == NULL)) {
	    indexArray[index].hashKey = hashKey;
	    TableParseArrayIndex(&(indexArray[index].row), &(indexArray[index].col), hashKey);
	    index++;
	}
    }
    if (index < numEntries) {
	numEntries = index;
    }

    /* Sort entries */
    qsort((void *)indexArray, (size_t)numEntries, sizeof(indexType), CellIndexComp);

    /* Convert array entries to TCL list */
    for (index = 0; index < numEntries; index++) {
	if ((tempPtr = Tcl_NewStringObj(indexArray[index].hashKey, -1)) != NULL
	    && Tcl_ListObjAppendElement(NULL, listPtr, tempPtr) == TCL_OK) {
	} else {
	    Tcl_IncrRefCount(listPtr);
	    Tcl_DecrRefCount(listPtr);
	    listPtr = NULL;
	    break;
	}
    }
    ckfree((char *) indexArray);
    return listPtr;
}

