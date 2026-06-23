/*

Copyright (c) 1989  X Consortium

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE X CONSORTIUM BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of the X Consortium shall
not be used in advertising or otherwise to promote the sale, use or
other dealings in this Software without prior written authorization
from the X Consortium.

*/

/*
 * xcalc.c  -  a hand calculator for the X Window system
 *
 *  Original Author:  John H. Bradley, University of Pennsylvania
 *			(bradley@cis.upenn.edu)  March, 1987
 *  RPN mode added and port to X11 by Mark Rosenstein, MIT Project Athena
 *  Rewritten to be a Motif (Xm) and Xt client by OpenMotif migration
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <signal.h>
#include <X11/Xatom.h>
#include <X11/Xfuncproto.h>
#include <X11/Shell.h>
#include <X11/cursorfont.h>
#include "xcalc.h"

#ifndef ONE
#define ONE  1
#endif
#ifndef ZERO
#define ZERO 0
#endif

static void create_keypad(Widget parent);
static void create_display(Widget parent);
static void create_calculator(Widget shell);
static void set_ti_attachments(Widget form, Widget bevel, Widget *btns);
static void set_hp_attachments(Widget form, Widget bevel, Widget *btns);
static void Syntax(int argc, char **argv) _X_NORETURN;

/*
 *	global data
 */
int	rpn = 0;		/* Reverse Polish Notation (HP mode) flag */
char	dispstr[LCD_STR_LEN];		/* string to show up in the LCD */
Atom	wm_delete_window;		/* see ICCCM section 5.2.2 */

/*
 *	local data
 */
static Display	*dpy = NULL;		/* connection to the X server */
static Widget	toplevel=NULL;  	/* top level shell widget */
static Widget   calculator=NULL;	/* an underlying form widget */
static Widget	LCD = NULL;		/* liquid crystal display */
static Widget	bevel = NULL;		/* frame around the display */
static Widget	ind[9];			/* mode indicators in the screen */
static Widget	buttons[55];		/* keypad button widgets */
					/* checkerboard used in mono mode */
static XtAppContext xtcontext;		/* Toolkit application context */
#define check_width 16
#define check_height 16
static unsigned char check_bits[] = {
   0x55, 0x55, 0xaa, 0xaa, 0x55, 0x55, 0xaa, 0xaa, 0x55, 0x55, 0xaa, 0xaa,
   0x55, 0x55, 0xaa, 0xaa, 0x55, 0x55, 0xaa, 0xaa, 0x55, 0x55, 0xaa, 0xaa,
   0x55, 0x55, 0xaa, 0xaa, 0x55, 0x55, 0xaa, 0xaa};

/*	command line options specific to the application */
static XrmOptionDescRec Options[] = {
{"-rpn",	"rpn",		XrmoptionNoArg,		(XtPointer)"on"},
{"-stipple",	"stipple",	XrmoptionNoArg,		(XtPointer)"on"}
};

/*	resources specific to the application */
static struct resources {
    Boolean	rpn;		/* reverse polish notation (HP mode) */
    Boolean	stipple;	/* background stipple */
    Cursor	cursor;
} appResources;

#define offset(field) XtOffsetOf(struct resources, field)
static XtResource Resources[] = {
{"rpn",		"Rpn",		XtRBoolean,	sizeof(Boolean),
     offset(rpn),	XtRImmediate,	(XtPointer) False},
{"stipple",	"Stipple",	XtRBoolean,	sizeof(Boolean),
     offset(stipple),	XtRImmediate,	(XtPointer) False},
{"cursor",	"Cursor",	XtRCursor,	sizeof(Cursor),
     offset(cursor),	XtRCursor,	(XtPointer)NULL}
};
#undef offset


int
main(int argc, char **argv)
{
    Arg		args[3];

    XtSetLanguageProc(NULL, (XtLanguageProc) NULL, NULL);

    /* Handle args that don't require opening a display */
    for (int n = 1; n < argc; n++) {
	const char *argn = argv[n];
	/* accept single or double dash for -help & -version */
	if (argn[0] == '-' && argn[1] == '-') {
	    argn++;
	}
	if (strcmp(argn, "-help") == 0) {
            Syntax(1, argv);
	}
	if (strcmp(argn, "-version") == 0) {
	    puts(PACKAGE_STRING);
	    exit(0);
	}
    }

    toplevel = XtAppInitialize(&xtcontext, "XCalc", Options, XtNumber(Options),
			       &argc, argv, NULL, NULL, 0);
    if (argc != 1) Syntax(argc, argv);

    XtSetArg(args[0], XtNinput, True);
    XtSetValues(toplevel, args, ONE);

    XtGetApplicationResources(toplevel, (XtPointer)&appResources, Resources,
			      XtNumber(Resources), (ArgList) NULL, ZERO);

    create_calculator(toplevel);

    XtAppAddActions(xtcontext, Actions, ActionsCount);

    XtOverrideTranslations(toplevel,
	   XtParseTranslationTable("<Message>WM_PROTOCOLS: quit()\n"));

    XtRealizeWidget(toplevel);

    dpy = XtDisplay(toplevel);
    wm_delete_window = XInternAtom(dpy, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(dpy, XtWindow(toplevel), &wm_delete_window, 1);
    if (appResources.cursor != None)
	XDefineCursor(dpy, XtWindow(toplevel), appResources.cursor);

    if ((appResources.stipple || (CellsOfScreen(XtScreen(toplevel)) <= 2))
	&& XtWindow(toplevel) != None)
    {
	Screen	*screen = XtScreen(toplevel);
	Pixmap	backgroundPix;

	backgroundPix = XCreatePixmapFromBitmapData
	    (dpy, XtWindow(toplevel),
	     (char *)check_bits, check_width, check_height,
	     WhitePixelOfScreen(screen), BlackPixelOfScreen(screen),
	     DefaultDepthOfScreen(screen));
	XtSetArg(args[0], XtNbackgroundPixmap, backgroundPix);
	XtSetValues(calculator, args, ONE);
    }

#ifndef IEEE
    signal(SIGFPE,fperr);
    signal(SIGILL,illerr);
#endif
    ResetCalc();
    XtAppMainLoop(xtcontext);

    return 0;
}

static void create_calculator(Widget shell)
{
    rpn = appResources.rpn;
    calculator = XtCreateManagedWidget(rpn ? "hp" : "ti", xmFormWidgetClass,
				       shell, (ArgList) NULL, ZERO);
    create_display(calculator);
    create_keypad(calculator);
    if (rpn)
	set_hp_attachments(calculator, bevel, buttons);
    else
	set_ti_attachments(calculator, bevel, buttons);
    XtSetKeyboardFocus(calculator, LCD);
}

/*
 *	Do the calculator data display widgets.
 */
static void create_display(Widget parent)
{
    Widget	screen;
    Arg		args[20];
    int		n;

    /* the frame surrounding the calculator display */
    n = 0;
    XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
    XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM); n++;
    XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM); n++;
    XtSetArg(args[n], XmNleftOffset, 4); n++;
    XtSetArg(args[n], XmNtopOffset, 2); n++;
    bevel = XtCreateManagedWidget("bevel", xmFrameWidgetClass, parent,
				  args, n);

    /* the screen of the calculator */
    n = 0;
    XtSetArg(args[n], XmNleftOffset, 6); n++;
    XtSetArg(args[n], XmNtopOffset, 2); n++;
    XtSetArg(args[n], XmNmarginWidth, 0); n++;
    screen = XtCreateManagedWidget("screen", xmFormWidgetClass, bevel,
				   args, n);

    /* M - the memory indicator */
    n = 0;
    XtSetArg(args[n], XtNborderWidth, (XtArgVal)0); n++;
    XtSetArg(args[n], XmNalignment, XmALIGNMENT_END); n++;
    ind[XCalc_MEMORY] = XtCreateManagedWidget("M", xmLabelWidgetClass, screen,
					args, n);

    /* liquid crystal display */
    n = 0;
    XtSetArg(args[n], XtNborderWidth, (XtArgVal)0); n++;
    XtSetArg(args[n], XmNalignment, XmALIGNMENT_END); n++;
    XtSetArg(args[n], XmNeditable, False); n++;
    XtSetArg(args[n], XmNcursorPositionVisible, False); n++;
    XtSetArg(args[n], XmNleftAttachment, XmATTACH_WIDGET); n++;
    XtSetArg(args[n], XmNleftWidget, ind[XCalc_MEMORY]); n++;
    XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM); n++;
    XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM); n++;
    XtSetArg(args[n], XmNleftOffset, 4); n++;
    XtSetArg(args[n], XmNtopOffset, 2); n++;
    LCD = XtCreateManagedWidget("LCD", xmTextFieldWidgetClass, screen, args,
				n);

    /* INV - the inverse function indicator */
    n = 0;
    XtSetArg(args[n], XtNborderWidth, (XtArgVal)0); n++;
    XtSetArg(args[n], XmNalignment, XmALIGNMENT_END); n++;
    XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
    XtSetArg(args[n], XmNtopWidget, LCD); n++;
    XtSetArg(args[n], XmNtopOffset, 4); n++;
    ind[XCalc_INVERSE] = XtCreateManagedWidget("INV", xmLabelWidgetClass,
					 screen, args, n);

    /* DEG - the degrees switch indicator */
    n = 0;
    XtSetArg(args[n], XtNborderWidth, (XtArgVal)0); n++;
    XtSetArg(args[n], XmNalignment, XmALIGNMENT_END); n++;
    XtSetArg(args[n], XmNleftAttachment, XmATTACH_WIDGET); n++;
    XtSetArg(args[n], XmNleftWidget, ind[XCalc_INVERSE]); n++;
    XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
    XtSetArg(args[n], XmNtopWidget, LCD); n++;
    XtSetArg(args[n], XmNleftOffset, 1); n++;
    ind[XCalc_DEGREE] = XtCreateManagedWidget("DEG", xmLabelWidgetClass, screen,
					args, n);

    /* RAD - the radian switch indicator */
    n = 0;
    XtSetArg(args[n], XtNborderWidth, (XtArgVal)0); n++;
    XtSetArg(args[n], XmNalignment, XmALIGNMENT_END); n++;
    XtSetArg(args[n], XmNleftAttachment, XmATTACH_WIDGET); n++;
    XtSetArg(args[n], XmNleftWidget, ind[XCalc_DEGREE]); n++;
    XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
    XtSetArg(args[n], XmNtopWidget, LCD); n++;
    ind[XCalc_RADIAN] = XtCreateManagedWidget("RAD", xmLabelWidgetClass, screen,
					args, n);

    /* GRAD - the grad switch indicator */
    n = 0;
    XtSetArg(args[n], XtNborderWidth, (XtArgVal)0); n++;
    XtSetArg(args[n], XmNalignment, XmALIGNMENT_END); n++;
    XtSetArg(args[n], XmNleftAttachment, XmATTACH_WIDGET); n++;
    XtSetArg(args[n], XmNleftWidget, ind[XCalc_RADIAN]); n++;
    XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
    XtSetArg(args[n], XmNtopWidget, LCD); n++;
    ind[XCalc_GRADAM] = XtCreateManagedWidget("GRAD", xmLabelWidgetClass, screen,
					args, n);

    /* () - the parenthesis indicator */
    n = 0;
    XtSetArg(args[n], XtNborderWidth, (XtArgVal)0); n++;
    XtSetArg(args[n], XmNalignment, XmALIGNMENT_END); n++;
    XtSetArg(args[n], XmNleftAttachment, XmATTACH_WIDGET); n++;
    XtSetArg(args[n], XmNleftWidget, ind[XCalc_GRADAM]); n++;
    XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
    XtSetArg(args[n], XmNtopWidget, LCD); n++;
    XtSetArg(args[n], XmNleftOffset, 2); n++;
    ind[XCalc_PAREN] = XtCreateManagedWidget("P", xmLabelWidgetClass, screen,
					     args, n);

    /* HEX - the hexadecimal (base 16) indicator */
    n = 0;
    XtSetArg(args[n], XtNborderWidth, (XtArgVal)0); n++;
    XtSetArg(args[n], XmNalignment, XmALIGNMENT_END); n++;
    XtSetArg(args[n], XmNleftAttachment, XmATTACH_WIDGET); n++;
    XtSetArg(args[n], XmNleftWidget, ind[XCalc_PAREN]); n++;
    XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
    XtSetArg(args[n], XmNtopWidget, LCD); n++;
    XtSetArg(args[n], XmNleftOffset, 1); n++;
    ind[XCalc_HEX] = XtCreateManagedWidget("HEX", xmLabelWidgetClass, screen,
					   args, n);

    /* DEC - the decimal (base 10) indicator */
    n = 0;
    XtSetArg(args[n], XtNborderWidth, (XtArgVal)0); n++;
    XtSetArg(args[n], XmNalignment, XmALIGNMENT_END); n++;
    XtSetArg(args[n], XmNleftAttachment, XmATTACH_WIDGET); n++;
    XtSetArg(args[n], XmNleftWidget, ind[XCalc_PAREN]); n++;
    XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
    XtSetArg(args[n], XmNtopWidget, LCD); n++;
    XtSetArg(args[n], XmNleftOffset, 1); n++;
    ind[XCalc_DEC] = XtCreateManagedWidget("DEC", xmLabelWidgetClass, screen,
					   args, n);

    /* OCT - the octal (base 8) indicator */
    n = 0;
    XtSetArg(args[n], XtNborderWidth, (XtArgVal)0); n++;
    XtSetArg(args[n], XmNalignment, XmALIGNMENT_END); n++;
    XtSetArg(args[n], XmNleftAttachment, XmATTACH_WIDGET); n++;
    XtSetArg(args[n], XmNleftWidget, ind[XCalc_PAREN]); n++;
    XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
    XtSetArg(args[n], XmNtopWidget, LCD); n++;
    XtSetArg(args[n], XmNleftOffset, 1); n++;
    ind[XCalc_OCT] = XtCreateManagedWidget("OCT", xmLabelWidgetClass, screen,
					   args, n);
}

/*
 *	Do all the buttons.  The application defaults file will give the
 *	default button placement, default button labels, and default
 *      actions connected to the buttons.  The user can change any of
 *      these defaults in an environment-specific resource file.
 */

static void create_keypad(Widget parent)
{
    static const char	*Keyboard[] = {
	"button1", "button2", "button3", "button4", "button5",
	"button6", "button7", "button8", "button9", "button10",
	"button11","button12","button13","button14","button15",
	"button16","button17","button18","button19","button20",
	"button21","button22","button23","button24","button25",
	"button26","button27","button28","button29","button30",
	"button31","button32","button33","button34","button35",
	"button36","button37","button38","button39","button40",
	"button41","button42","button43","button44","button45",
	"button46","button47","button48","button49","button50",
	"button51","button52","button53","button54","button55",
    };
    register int i;
    int		 n = XtNumber(Keyboard);

    if (appResources.rpn) n--; 	/* HP has 39 buttons, TI has 40 */

    for (i=0; i < n; i++)
	buttons[i] = XtCreateManagedWidget(Keyboard[i], xmPushButtonWidgetClass, parent,
			      (ArgList) NULL, ZERO);
}

/*
 *	Set XmForm constraint attachments for TI mode buttons.
 *	This replaces the resource-file XmNleftAttachment/XmNtopAttachment
 *	settings that Motif's String-to-Widget converter fails to apply.
 */
static void set_ti_attachments(Widget form, Widget bevel_w, Widget *btns)
{
    Arg	args[8];
    int	n, i;
    int	cols = 5;
    int	rows = 11;

    for (i = 0; i < rows * cols; i++) {
	n = 0;
	if (i % cols == 0) {
	    XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
	    XtSetArg(args[n], XmNleftOffset, 4); n++;
	} else {
	    XtSetArg(args[n], XmNleftAttachment, XmATTACH_WIDGET); n++;
	    XtSetArg(args[n], XmNleftWidget, btns[i - 1]); n++;
	}
	if (i < cols) {
	    XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
	    XtSetArg(args[n], XmNtopWidget, bevel_w); n++;
	    XtSetArg(args[n], XmNtopOffset, 12); n++;
	} else {
	    XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
	    XtSetArg(args[n], XmNtopWidget, btns[i - cols]); n++;
	}
	XtSetValues(btns[i], args, n);
    }
}

/*
 *	Set XmForm constraint attachments for HP mode buttons.
 *	HP layout: rows 1-3 have 10 columns, rows 4-8 have 5 columns
 *	with button26 (ENTER) spanning 2 rows.
 *	Buttons 21 and 22 are unmapped.
 */
static void set_hp_attachments(Widget form, Widget bevel_w, Widget *btns)
{
    Arg	args[8];
    int	n, i;

    /* Row 0 (buttons 1-10): top=bevel, left=form or prev */
    for (i = 0; i < 10; i++) {
	n = 0;
	if (i == 0) {
	    XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
	    XtSetArg(args[n], XmNleftOffset, 4); n++;
	} else {
	    XtSetArg(args[n], XmNleftAttachment, XmATTACH_WIDGET); n++;
	    XtSetArg(args[n], XmNleftWidget, btns[i - 1]); n++;
	}
	XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
	XtSetArg(args[n], XmNtopWidget, bevel_w); n++;
	XtSetArg(args[n], XmNtopOffset, 12); n++;
	XtSetValues(btns[i], args, n);
    }

    /* Row 1 (buttons 11-20): top=button[i-10], left=form or prev */
    for (i = 10; i < 20; i++) {
	n = 0;
	if (i == 10) {
	    XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
	    XtSetArg(args[n], XmNleftOffset, 4); n++;
	} else {
	    XtSetArg(args[n], XmNleftAttachment, XmATTACH_WIDGET); n++;
	    XtSetArg(args[n], XmNleftWidget, btns[i - 1]); n++;
	}
	XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
	XtSetArg(args[n], XmNtopWidget, btns[i - 10]); n++;
	XtSetValues(btns[i], args, n);
    }

    /* Row 2 (buttons 21-30): top=button[i-10], left=form or prev */
    for (i = 20; i < 30; i++) {
	n = 0;
	if (i == 20) {
	    XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
	    XtSetArg(args[n], XmNleftOffset, 4); n++;
	} else {
	    XtSetArg(args[n], XmNleftAttachment, XmATTACH_WIDGET); n++;
	    XtSetArg(args[n], XmNleftWidget, btns[i - 1]); n++;
	}
	XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
	XtSetArg(args[n], XmNtopWidget, btns[i - 10]); n++;
	XtSetValues(btns[i], args, n);
    }

    /* Row 3 (buttons 31-39): 9 buttons, 5+4 layout */
    /* button31: left=FORM, top=button21 (even though 21 is unmapped) */
    n = 0;
    XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
    XtSetArg(args[n], XmNleftOffset, 4); n++;
    XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
    XtSetArg(args[n], XmNtopWidget, btns[20]); n++; /* button21 */
    XtSetValues(btns[30], args, n);

    /* button32: left=button31, top=button22 */
    n = 0;
    XtSetArg(args[n], XmNleftAttachment, XmATTACH_WIDGET); n++;
    XtSetArg(args[n], XmNleftWidget, btns[30]); n++;
    XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
    XtSetArg(args[n], XmNtopWidget, btns[21]); n++; /* button22 */
    XtSetValues(btns[31], args, n);

    /* button33: left=button32, top=button23 */
    n = 0;
    XtSetArg(args[n], XmNleftAttachment, XmATTACH_WIDGET); n++;
    XtSetArg(args[n], XmNleftWidget, btns[31]); n++;
    XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
    XtSetArg(args[n], XmNtopWidget, btns[22]); n++;
    XtSetValues(btns[32], args, n);

    /* button34: left=button33, top=button24 */
    n = 0;
    XtSetArg(args[n], XmNleftAttachment, XmATTACH_WIDGET); n++;
    XtSetArg(args[n], XmNleftWidget, btns[32]); n++;
    XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
    XtSetArg(args[n], XmNtopWidget, btns[23]); n++;
    XtSetValues(btns[33], args, n);

    /* button35: left=button34, top=button25 */
    n = 0;
    XtSetArg(args[n], XmNleftAttachment, XmATTACH_WIDGET); n++;
    XtSetArg(args[n], XmNleftWidget, btns[33]); n++;
    XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
    XtSetArg(args[n], XmNtopWidget, btns[24]); n++;
    XtSetValues(btns[34], args, n);

    /* button36: left=button26(ENTER spans), top=button27 */
    n = 0;
    XtSetArg(args[n], XmNleftAttachment, XmATTACH_WIDGET); n++;
    XtSetArg(args[n], XmNleftWidget, btns[25]); n++; /* button26=ENTER */
    XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
    XtSetArg(args[n], XmNtopWidget, btns[26]); n++; /* button27 */
    XtSetValues(btns[35], args, n);

    /* button37: left=button36, top=button28 */
    n = 0;
    XtSetArg(args[n], XmNleftAttachment, XmATTACH_WIDGET); n++;
    XtSetArg(args[n], XmNleftWidget, btns[35]); n++;
    XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
    XtSetArg(args[n], XmNtopWidget, btns[27]); n++; /* button28 */
    XtSetValues(btns[36], args, n);

    /* button38: left=button37, top=button29 */
    n = 0;
    XtSetArg(args[n], XmNleftAttachment, XmATTACH_WIDGET); n++;
    XtSetArg(args[n], XmNleftWidget, btns[36]); n++;
    XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
    XtSetArg(args[n], XmNtopWidget, btns[28]); n++; /* button29 */
    XtSetValues(btns[37], args, n);

    /* button39: left=button38, top=button30 */
    n = 0;
    XtSetArg(args[n], XmNleftAttachment, XmATTACH_WIDGET); n++;
    XtSetArg(args[n], XmNleftWidget, btns[37]); n++;
    XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
    XtSetArg(args[n], XmNtopWidget, btns[29]); n++; /* button30 */
    XtSetValues(btns[38], args, n);
}

/*
 *	Miscellaneous utility routines that interact with the widgets.
 */

/*
 * 	called by math routines to write to the liquid crystal display.
 */
void draw(char *string)
{
    XmTextFieldSetString(LCD, string);
}

/*
 *	called by math routines to turn on and off the display indicators.
 */
void setflag(int indicator, Boolean on)
{
    if (on) XtManageChild(ind[indicator]);
    else XtUnmanageChild(ind[indicator]);
}

/*
 *	ring the bell.
 */
void ringbell(void)
{
    XBell(dpy, 0);
}

/*
 *	die.
 */
void Quit(void)
{
    XtDestroyApplicationContext(xtcontext);
    exit(0);
}

/*
 *	recite and die.
 */
static void Syntax(int argc, char **argv)
{
    if (argc > 1) {
        fprintf(stderr, "%s: unknown options:", argv[0]);
        for (int i = 1; i <argc; i++)
	    fprintf(stderr, " %s", argv[i]);
        fprintf(stderr, "\n\n");
    }
    fprintf(stderr, "Usage:  %s", argv[0]);
    for (Cardinal i = 0; i < XtNumber(Options); i++)
	fprintf(stderr, " [%s]", Options[i].option);
    fprintf(stderr, "\n");
    fprintf(stderr, "        %s -help\n", argv[0]);
    fprintf(stderr, "        %s -version\n", argv[0]);
    if (xtcontext != NULL)
        XtDestroyApplicationContext(xtcontext);
    exit((argc > 1) ? 1 : 0);
}