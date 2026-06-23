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
static void set_ti_labels(Widget *btns);
static void set_hp_labels(Widget *btns);
static void set_ti_translations(Widget *btns);
static void set_hp_translations(Widget *btns);
static void set_button_sizes(Widget *btns, int count, int width, int height);
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

/*
 *	TI button translations (buttons 1-55).
 *	These replace the resource-file translations that Motif does not apply.
 */
static const char *ti_translations[] = {
    "#override<Btn1Down>:Arm()\n<Btn1Up>:reciprocal() Disarm()",	/* button1 */
    "#override<Btn1Down>:Arm()\n<Btn1Up>:square() Disarm()",		/* button2 */
    "#override<Btn1Down>:Arm()\n<Btn1Up>:squareRoot() Disarm()",	/* button3 */
    "#override<Btn1Down>:Arm()\n<Btn1Up>:clear() Disarm()",		/* button4 */
    "#override<Btn1Down>:Arm()\n<Btn1Up>:off() Disarm()\n<Btn3Down>,<Btn3Up>:quit()",	/* button5 */
    "#override<Btn1Down>:Arm()\n<Btn1Up>:inverse() Disarm()",		/* button6 */
    "#override<Btn1Down>:Arm()\n<Btn1Up>:sine() Disarm()",		/* button7 */
    "#override<Btn1Down>:Arm()\n<Btn1Up>:cosine() Disarm()",		/* button8 */
    "#override<Btn1Down>:Arm()\n<Btn1Up>:tangent() Disarm()",		/* button9 */
    "#override<Btn1Down>:Arm()\n<Btn1Up>:degree() Disarm()",		/* button10 */
    "#override<Btn1Down>:Arm()\n<Btn1Up>:e() Disarm()",			/* button11 */
    "#override<Btn1Down>:Arm()\n<Btn1Up>:scientific() Disarm()",	/* button12 */
    "#override<Btn1Down>:Arm()\n<Btn1Up>:logarithm() Disarm()",		/* button13 */
    "#override<Btn1Down>:Arm()\n<Btn1Up>:naturalLog() Disarm()",	/* button14 */
    "#override<Btn1Down>:Arm()\n<Btn1Up>:power() Disarm()",		/* button15 */
    "#override<Btn1Down>:Arm()\n<Btn1Up>:not() Disarm()",		/* button16 */
    "#override<Btn1Down>:Arm()\n<Btn1Up>:and() Disarm()",		/* button17 */
    "#override<Btn1Down>:Arm()\n<Btn1Up>:or() Disarm()",			/* button18 */
    "#override<Btn1Down>:Arm()\n<Btn1Up>:xor() Disarm()",		/* button19 */
    "#override<Btn1Down>:Arm()\n<Btn1Up>:trunc() Disarm()",		/* button20 */
    "#override<Btn1Down>:Arm()\n<Btn1Up>:pi() Disarm()",			/* button21 */
    "#override<Btn1Down>:Arm()\n<Btn1Up>:factorial() Disarm()",		/* button22 */
    "#override<Btn1Down>:Arm()\n<Btn1Up>:leftParen() Disarm()",		/* button23 */
    "#override<Btn1Down>:Arm()\n<Btn1Up>:rightParen() Disarm()",	/* button24 */
    "#override<Btn1Down>:Arm()\n<Btn1Up>:base() Disarm()",		/* button25 */
    "#override<Btn1Down>:Arm()\n<Btn1Up>:shl() Disarm()",		/* button26 */
    "#override<Btn1Down>:Arm()\n<Btn1Up>:digit(D) Disarm()",		/* button27 */
    "#override<Btn1Down>:Arm()\n<Btn1Up>:digit(E) Disarm()",		/* button28 */
    "#override<Btn1Down>:Arm()\n<Btn1Up>:digit(F) Disarm()",		/* button29 */
    "#override<Btn1Down>:Arm()\n<Btn1Up>:shr() Disarm()",		/* button30 */
    "#override<Btn1Down>:Arm()\n<Btn1Up>:mod() Disarm()",		/* button31 */
    "#override<Btn1Down>:Arm()\n<Btn1Up>:digit(A) Disarm()",		/* button32 */
    "#override<Btn1Down>:Arm()\n<Btn1Up>:digit(B) Disarm()",		/* button33 */
    "#override<Btn1Down>:Arm()\n<Btn1Up>:digit(C) Disarm()",		/* button34 */
    "#override<Btn1Down>:Arm()\n<Btn1Up>:divide() Disarm()",		/* button35 */
    "#override<Btn1Down>:Arm()\n<Btn1Up>:store() Disarm()",		/* button36 */
    "#override<Btn1Down>:Arm()\n<Btn1Up>:digit(7) Disarm()",		/* button37 */
    "#override<Btn1Down>:Arm()\n<Btn1Up>:digit(8) Disarm()",		/* button38 */
    "#override<Btn1Down>:Arm()\n<Btn1Up>:digit(9) Disarm()",		/* button39 */
    "#override<Btn1Down>:Arm()\n<Btn1Up>:multiply() Disarm()",		/* button40 */
    "#override<Btn1Down>:Arm()\n<Btn1Up>:recall() Disarm()",		/* button41 */
    "#override<Btn1Down>:Arm()\n<Btn1Up>:digit(4) Disarm()",		/* button42 */
    "#override<Btn1Down>:Arm()\n<Btn1Up>:digit(5) Disarm()",		/* button43 */
    "#override<Btn1Down>:Arm()\n<Btn1Up>:digit(6) Disarm()",		/* button44 */
    "#override<Btn1Down>:Arm()\n<Btn1Up>:subtract() Disarm()",		/* button45 */
    "#override<Btn1Down>:Arm()\n<Btn1Up>:sum() Disarm()",		/* button46 */
    "#override<Btn1Down>:Arm()\n<Btn1Up>:digit(1) Disarm()",		/* button47 */
    "#override<Btn1Down>:Arm()\n<Btn1Up>:digit(2) Disarm()",		/* button48 */
    "#override<Btn1Down>:Arm()\n<Btn1Up>:digit(3) Disarm()",		/* button49 */
    "#override<Btn1Down>:Arm()\n<Btn1Up>:add() Disarm()",		/* button50 */
    "#override<Btn1Down>:Arm()\n<Btn1Up>:exchange() Disarm()",		/* button51 */
    "#override<Btn1Down>:Arm()\n<Btn1Up>:digit(0) Disarm()",		/* button52 */
    "#override<Btn1Down>:Arm()\n<Btn1Up>:decimal() Disarm()",		/* button53 */
    "#override<Btn1Down>:Arm()\n<Btn1Up>:negate() Disarm()",		/* button54 */
    "#override<Btn1Down>:Arm()\n<Btn1Up>:equal() Disarm()"		/* button55 */
};

/*
 *	HP button translations (buttons 1-39, buttons 21 & 22 unmapped).
 */
static const char *hp_translations[] = {
    "#override<Btn1Down>:Arm()\n<Btn1Up>:squareRoot() Disarm()",	/* button1 */
    "#override<Btn1Down>:Arm()\n<Btn1Up>:epower() Disarm()",		/* button2 */
    "#override<Btn1Down>:Arm()\n<Btn1Up>:tenpower() Disarm()",		/* button3 */
    "#override<Btn1Down>:Arm()\n<Btn1Up>:power() Disarm()",		/* button4 */
    "#override<Btn1Down>:Arm()\n<Btn1Up>:reciprocal() Disarm()",	/* button5 */
    "#override<Btn1Down>:Arm()\n<Btn1Up>:negate() Disarm()",		/* button6 */
    "#override<Btn1Down>:Arm()\n<Btn1Up>:digit(7) Disarm()",		/* button7 */
    "#override<Btn1Down>:Arm()\n<Btn1Up>:digit(8) Disarm()",		/* button8 */
    "#override<Btn1Down>:Arm()\n<Btn1Up>:digit(9) Disarm()",		/* button9 */
    "#override<Btn1Down>:Arm()\n<Btn1Up>:divide() Disarm()",		/* button10 */
    "#override<Btn1Down>:Arm()\n<Btn1Up>:factorial() Disarm()",		/* button11 */
    "#override<Btn1Down>:Arm()\n<Btn1Up>:pi() Disarm()",			/* button12 */
    "#override<Btn1Down>:Arm()\n<Btn1Up>:sine() Disarm()",		/* button13 */
    "#override<Btn1Down>:Arm()\n<Btn1Up>:cosine() Disarm()",		/* button14 */
    "#override<Btn1Down>:Arm()\n<Btn1Up>:tangent() Disarm()",		/* button15 */
    "#override<Btn1Down>:Arm()\n<Btn1Up>:scientific() Disarm()",	/* button16 */
    "#override<Btn1Down>:Arm()\n<Btn1Up>:digit(4) Disarm()",		/* button17 */
    "#override<Btn1Down>:Arm()\n<Btn1Up>:digit(5) Disarm()",		/* button18 */
    "#override<Btn1Down>:Arm()\n<Btn1Up>:digit(6) Disarm()",		/* button19 */
    "#override<Btn1Down>:Arm()\n<Btn1Up>:multiply() Disarm()",		/* button20 */
    NULL,						/* button21 (unmapped) */
    NULL,						/* button22 (unmapped) */
    "#override<Btn1Down>:Arm()\n<Btn1Up>:roll() Disarm()",		/* button23 */
    "#override<Btn1Down>:Arm()\n<Btn1Up>:XexchangeY() Disarm()",	/* button24 */
    "#override<Btn1Down>:Arm()\n<Btn1Up>:back() Disarm()",		/* button25 */
    "#override<Btn1Down>:Arm()\n<Btn1Up>:enter() Disarm()",		/* button26 */
    "#override<Btn1Down>:Arm()\n<Btn1Up>:digit(1) Disarm()",		/* button27 */
    "#override<Btn1Down>:Arm()\n<Btn1Up>:digit(2) Disarm()",		/* button28 */
    "#override<Btn1Down>:Arm()\n<Btn1Up>:digit(3) Disarm()",		/* button29 */
    "#override<Btn1Down>:Arm()\n<Btn1Up>:subtract() Disarm()",		/* button30 */
    "#override<Btn1Down>:Arm()\n<Btn1Up>:off() Disarm()\n<Btn3Down>,<Btn3Up>:quit()",	/* button31 */
    "#override<Btn1Down>:Arm()\n<Btn1Up>:degree() Disarm()",		/* button32 */
    "#override<Btn1Down>:Arm()\n<Btn1Up>:inverse() Disarm()",		/* button33 */
    "#override<Btn1Down>:Arm()\n<Btn1Up>:store() Disarm()",		/* button34 */
    "#override<Btn1Down>:Arm()\n<Btn1Up>:recall() Disarm()",		/* button35 */
    "#override<Btn1Down>:Arm()\n<Btn1Up>:digit(0) Disarm()",		/* button36 */
    "#override<Btn1Down>:Arm()\n<Btn1Up>:decimal() Disarm()",		/* button37 */
    "#override<Btn1Down>:Arm()\n<Btn1Up>:sum() Disarm()",		/* button38 */
    "#override<Btn1Down>:Arm()\n<Btn1Up>:add() Disarm()"		/* button39 */
};

/*
 *	TI LCD keyboard translations.
 */
static const char ti_lcd_translations[] =
    "#replace\n"
    "Ctrl<Key>c:quit()\n"
    "Ctrl<Key>h:clear()\n"
    "None<Key>0:digit(0)\n"
    "None<Key>1:digit(1)\n"
    "None<Key>2:digit(2)\n"
    "None<Key>3:digit(3)\n"
    "None<Key>4:digit(4)\n"
    "None<Key>5:digit(5)\n"
    "None<Key>6:digit(6)\n"
    "None<Key>7:digit(7)\n"
    "None<Key>8:digit(8)\n"
    "None<Key>9:digit(9)\n"
    "Mod2<Key>0:digit(0)\n"
    "Mod2<Key>1:digit(1)\n"
    "Mod2<Key>2:digit(2)\n"
    "Mod2<Key>3:digit(3)\n"
    "Mod2<Key>4:digit(4)\n"
    "Mod2<Key>5:digit(5)\n"
    "Mod2<Key>6:digit(6)\n"
    "Mod2<Key>7:digit(7)\n"
    "Mod2<Key>8:digit(8)\n"
    "Mod2<Key>9:digit(9)\n"
    "Shift<Key>a:digit(A)\n"
    "Shift<Key>b:digit(B)\n"
    "Shift<Key>c:digit(C)\n"
    "Shift<Key>d:digit(D)\n"
    "Shift<Key>e:digit(E)\n"
    "Shift<Key>f:digit(F)\n"
    "<Key>KP_0:digit(0)\n"
    "<Key>KP_1:digit(1)\n"
    "<Key>KP_2:digit(2)\n"
    "<Key>KP_3:digit(3)\n"
    "<Key>KP_4:digit(4)\n"
    "<Key>KP_5:digit(5)\n"
    "<Key>KP_6:digit(6)\n"
    "<Key>KP_7:digit(7)\n"
    "<Key>KP_8:digit(8)\n"
    "<Key>KP_9:digit(9)\n"
    "<Key>KP_Enter:equal()\n"
    "<Key>KP_Equal:equal()\n"
    "<Key>Return:equal()\n"
    "<Key>KP_Multiply:multiply()\n"
    "<Key>KP_Add:add()\n"
    "<Key>KP_Subtract:subtract()\n"
    "<Key>KP_Decimal:decimal()\n"
    "<Key>KP_Separator:decimal()\n"
    "<Key>KP_Divide:divide()\n"
    "<Key>KP_Tab:equal()\n"
    "<Key>Clear:clear()\n"
    ":<Key>.:decimal()\n"
    ":<Key>+:add()\n"
    ":<Key>-:subtract()\n"
    ":<Key>*:multiply()\n"
    ":<Key>/:divide()\n"
    ":<Key>(:leftParen()\n"
    ":<Key>):rightParen()\n"
    ":<Key>!:factorial()\n"
    ":<Key>|:or()\n"
    ":<Key>&:and()\n"
    ":<Key><:shl()\n"
    ":<Key>>:shr()\n"
    ":<Key>~:not()\n"
    ":<Key>%:mod()\n"
    "<Key>x:xor()\n"
    "<Key>e:e()\n"
    ":<Key>^:power()\n"
    "<Key>p:pi()\n"
    "<Key>i:inverse()\n"
    "<Key>s:sine()\n"
    "<Key>c:cosine()\n"
    "<Key>t:tangent()\n"
    "<Key>d:degree()\n"
    "<Key>l:naturalLog()\n"
    ":<Key>=:equal()\n"
    "<Key>n:negate()\n"
    "<Key>r:squareRoot()\n"
    "<Key>space:clear()\n"
    "<Key>q:quit()\n"
    "<Key>Delete:clear()\n"
    "<Key>BackSpace:clear()";

/*
 *	HP LCD keyboard translations.
 */
static const char hp_lcd_translations[] =
    "#replace\n"
    "Ctrl<Key>c:quit()\n"
    "Ctrl<Key>h:back()\n"
    "None<Key>0:digit(0)\n"
    "None<Key>1:digit(1)\n"
    "None<Key>2:digit(2)\n"
    "None<Key>3:digit(3)\n"
    "None<Key>4:digit(4)\n"
    "None<Key>5:digit(5)\n"
    "None<Key>6:digit(6)\n"
    "None<Key>7:digit(7)\n"
    "None<Key>8:digit(8)\n"
    "None<Key>9:digit(9)\n"
    "<Key>KP_0:digit(0)\n"
    "<Key>KP_1:digit(1)\n"
    "<Key>KP_2:digit(2)\n"
    "<Key>KP_3:digit(3)\n"
    "<Key>KP_4:digit(4)\n"
    "<Key>KP_5:digit(5)\n"
    "<Key>KP_6:digit(6)\n"
    "<Key>KP_7:digit(7)\n"
    "<Key>KP_8:digit(8)\n"
    "<Key>KP_9:digit(9)\n"
    "<Key>KP_Enter:enter()\n"
    "<Key>KP_Multiply:multiply()\n"
    "<Key>KP_Add:add()\n"
    "<Key>KP_Subtract:subtract()\n"
    "<Key>KP_Decimal:decimal()\n"
    "<Key>KP_Divide:divide()\n"
    ":<Key>.:decimal()\n"
    ":<Key>+:add()\n"
    ":<Key>-:subtract()\n"
    ":<Key>*:multiply()\n"
    ":<Key>/:divide()\n"
    ":<Key>!:factorial()\n"
    "<Key>e:e()\n"
    ":<Key>^:power()\n"
    "<Key>p:pi()\n"
    "<Key>i:inverse()\n"
    "<Key>s:sine()\n"
    "<Key>c:cosine()\n"
    "<Key>t:tangent()\n"
    "<Key>d:degree()\n"
    "<Key>l:naturalLog()\n"
    "<Key>n:negate()\n"
    "<Key>r:squareRoot()\n"
    "<Key>space:clear()\n"
    "<Key>q:quit()\n"
    "<Key>Delete:back()\n"
    "<Key>Return:enter()\n"
    "<Key>Linefeed:enter()\n"
    "<Key>x:XexchangeY()\n"
    "<Key>BackSpace:back()";

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
    if (rpn) {
	set_hp_attachments(calculator, bevel, buttons);
	set_hp_labels(buttons);
	set_hp_translations(buttons);
	set_button_sizes(buttons, 39, 40, 26);
	Arg h_arg;
	XtSetArg(h_arg, XmNheight, (XtArgVal)56);
	XtSetValues(buttons[25], &h_arg, 1);
	/* Make buttons 21 and 22 invisible but keep them managed for layout */
	{
	    Arg inv_args[5];
	    int inv_n;
	    XmString empty = XmStringCreateLocalized("");
	    for (int bi = 20; bi <= 21; bi++) {
		inv_n = 0;
		XtSetArg(inv_args[inv_n], XmNlabelString, empty); inv_n++;
		XtSetArg(inv_args[inv_n], XmNshadowThickness, 0); inv_n++;
		XtSetArg(inv_args[inv_n], XmNsensitive, False); inv_n++;
		XtSetArg(inv_args[inv_n], XmNmappedWhenManaged, False); inv_n++;
		XtSetValues(buttons[bi], inv_args, inv_n);
	    }
	    XmStringFree(empty);
	}
    } else {
	set_ti_attachments(calculator, bevel, buttons);
	set_ti_labels(buttons);
	set_ti_translations(buttons);
	set_button_sizes(buttons, 55, 40, 26);
    }
    XtOverrideTranslations(LCD, XtParseTranslationTable(
	rpn ? hp_lcd_translations : ti_lcd_translations));
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
    XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
    XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM); n++;
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
    XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
    XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
    XtSetArg(args[n], XmNtopWidget, LCD); n++;
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
	if (i % cols == cols - 1) {
	    XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM); n++;
	    XtSetArg(args[n], XmNrightOffset, 4); n++;
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
 *	Matches the original Xaw resource layout from app-defaults/XCalc.
 *	Buttons 21 and 22 (indices 20, 21) are kept managed but invisible
 *	(mappedWhenManaged=False) so they serve as valid attachment targets.
 *
 *	Row 0: sqrt e^x 10^x y^x 1/x CHS 7 8 9 ÷    (10 buttons)
 *	Row 1: x!  π  sin  cos  tan  EEX 4 5 6 ×      (10 buttons)
 *	Row 2: [21] [22] Rv  x:y  ←  ENTER  1 2 3 −  (8 visible + 2 invisible + ENTER)
 *	Row 3: ON  DRG  INV  STO  RCL  0 . SUM +       (9 buttons)
 */
static void set_hp_attachments(Widget form, Widget bevel_w, Widget *btns)
{
    Arg	args[8];
    int	n, i;

    /* Row 0 (indices 0-9): top=bevel+12, left=FORM or prev, right=FORM for btn10 */
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
	if (i == 9) {
	    XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM); n++;
	    XtSetArg(args[n], XmNrightOffset, 4); n++;
	}
	XtSetValues(btns[i], args, n);
    }

    /* Row 1 (indices 10-19): top=btns[i-10], left=FORM or prev, right=FORM for btn20 */
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
	if (i == 19) {
	    XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM); n++;
	    XtSetArg(args[n], XmNrightOffset, 4); n++;
	}
	XtSetValues(btns[i], args, n);
    }

    /* Row 2 (indices 20-29): all 10 buttons including invisible 21/22.
     * Matches original resource layout exactly.
     * button21(20): left=FORM+4, top=button11(10)
     * button22(21): left=button21(20), top=button12(11)
     * button23(22): left=button22(21), top=button13(12)
     * ...
     * button30(29): left=button29(28), top=button20(19), right=FORM */
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
	if (i == 29) {
	    XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM); n++;
	    XtSetArg(args[n], XmNrightOffset, 4); n++;
	}
	XtSetValues(btns[i], args, n);
    }

    /* Row 3 (indices 30-38): 9 buttons, matching original resource layout.
     * button31(30): left=FORM+4, top=button21(20)
     * button32(31): left=button31(30), top=button22(21)
     * button33(32): left=button32(31), top=button23(22)
     * button34(33): left=button33(32), top=button24(23)
     * button35(34): left=button34(33), top=button25(24)
     * button36(35): left=button26(25), top=button27(26)
     * button37(36): left=button36(35), top=button28(27)
     * button38(37): left=button37(36), top=button29(28)
     * button39(38): left=button38(37), top=button30(29), right=FORM */

    /* button31(idx30): left=FORM, top=button21(idx20) */
    n = 0;
    XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
    XtSetArg(args[n], XmNleftOffset, 4); n++;
    XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
    XtSetArg(args[n], XmNtopWidget, btns[20]); n++;
    XtSetValues(btns[30], args, n);

    /* button32(idx31): left=button31(30), top=button22(21) */
    n = 0;
    XtSetArg(args[n], XmNleftAttachment, XmATTACH_WIDGET); n++;
    XtSetArg(args[n], XmNleftWidget, btns[30]); n++;
    XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
    XtSetArg(args[n], XmNtopWidget, btns[21]); n++;
    XtSetValues(btns[31], args, n);

    /* button33(idx32): left=button32(31), top=button23(22) */
    n = 0;
    XtSetArg(args[n], XmNleftAttachment, XmATTACH_WIDGET); n++;
    XtSetArg(args[n], XmNleftWidget, btns[31]); n++;
    XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
    XtSetArg(args[n], XmNtopWidget, btns[22]); n++;
    XtSetValues(btns[32], args, n);

    /* button34(idx33): left=button33(32), top=button24(23) */
    n = 0;
    XtSetArg(args[n], XmNleftAttachment, XmATTACH_WIDGET); n++;
    XtSetArg(args[n], XmNleftWidget, btns[32]); n++;
    XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
    XtSetArg(args[n], XmNtopWidget, btns[23]); n++;
    XtSetValues(btns[33], args, n);

    /* button35(idx34): left=button34(33), top=button25(24) */
    n = 0;
    XtSetArg(args[n], XmNleftAttachment, XmATTACH_WIDGET); n++;
    XtSetArg(args[n], XmNleftWidget, btns[33]); n++;
    XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
    XtSetArg(args[n], XmNtopWidget, btns[24]); n++;
    XtSetValues(btns[34], args, n);

    /* button36(idx35): left=button26(25)=ENTER, top=button27(26) */
    n = 0;
    XtSetArg(args[n], XmNleftAttachment, XmATTACH_WIDGET); n++;
    XtSetArg(args[n], XmNleftWidget, btns[25]); n++;
    XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
    XtSetArg(args[n], XmNtopWidget, btns[26]); n++;
    XtSetValues(btns[35], args, n);

    /* button37(idx36): left=button36(35), top=button28(27) */
    n = 0;
    XtSetArg(args[n], XmNleftAttachment, XmATTACH_WIDGET); n++;
    XtSetArg(args[n], XmNleftWidget, btns[35]); n++;
    XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
    XtSetArg(args[n], XmNtopWidget, btns[27]); n++;
    XtSetValues(btns[36], args, n);

    /* button38(idx37): left=button37(36), top=button29(28) */
    n = 0;
    XtSetArg(args[n], XmNleftAttachment, XmATTACH_WIDGET); n++;
    XtSetArg(args[n], XmNleftWidget, btns[36]); n++;
    XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
    XtSetArg(args[n], XmNtopWidget, btns[28]); n++;
    XtSetValues(btns[37], args, n);

    /* button39(idx38): left=button38(37), top=button30(29), right=FORM */
    n = 0;
    XtSetArg(args[n], XmNleftAttachment, XmATTACH_WIDGET); n++;
    XtSetArg(args[n], XmNleftWidget, btns[37]); n++;
    XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
    XtSetArg(args[n], XmNtopWidget, btns[29]); n++;
    XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM); n++;
    XtSetArg(args[n], XmNrightOffset, 4); n++;
    XtSetValues(btns[38], args, n);
}

/*
 *	Set XmNlabelString for TI mode buttons.
 *	The app-defaults resource file's labelString resources are not
 *	applied by Motif, so we set them programmatically here.
 */
static void set_ti_labels(Widget *btns)
{
    static const char *ti_labels[] = {
	"1/x", "x^2", "sqrt", "CE/C", "AC",		/* row 1 */
	"INV", "sin",  "cos",  "tan",  "DRG",		/* row 2 */
	"e",   "EE",   "log",  "ln",   "y^x",		/* row 3 */
	"not", "and",  "or",   "xor",  "trunc",	/* row 4 */
	"pi",  "x!",   "(",    ")",    "base",		/* row 5 */
	"shl", "D",    "E",    "F",    "shr",		/* row 6 */
	"mod", "A",    "B",    "C",    "/",		/* row 7 */
	"STO", "7",    "8",    "9",    "*",		/* row 8 */
	"RCL", "4",    "5",    "6",    "-",		/* row 9 */
	"SUM", "1",    "2",    "3",    "+",		/* row 10 */
	"EXC", "0",    ".",    "+/-",  "="		/* row 11 */
    };
    Arg arg;
    XmString str;
    int i;
    for (i = 0; i < 55; i++) {
	str = XmStringCreateLocalized((char *)ti_labels[i]);
	XtSetArg(arg, XmNlabelString, str);
	XtSetValues(btns[i], &arg, 1);
	XmStringFree(str);
    }
}

/*
 *	Set XmNlabelString for HP mode buttons.
 *	HP has 39 visible buttons (buttons 21 & 22 are unmapped).
 */
static void set_hp_labels(Widget *btns)
{
    static const char *hp_labels[] = {
	"sqrt",					/* button1 */
	"e^x",					/* button2 */
	"10^x",					/* button3 */
	"y^x",					/* button4 */
	"1/x",					/* button5 */
	"CHS",					/* button6 */
	"7",					/* button7 */
	"8",					/* button8 */
	"9",					/* button9 */
	"/",					/* button10 */
	"x!",					/* button11 */
	"pi",					/* button12 */
	"sin",					/* button13 */
	"cos",					/* button14 */
	"tan",					/* button15 */
	"EEX",					/* button16 */
	"4",					/* button17 */
	"5",					/* button18 */
	"6",					/* button19 */
	"*",					/* button20 */
	"",					/* button21 (unmapped) */
	"",					/* button22 (unmapped) */
	"Rv",					/* button23 */
	"x:y",					/* button24 */
	"<-",					/* button25 */
	"E\nN\nT\nE\nR",			/* button26 */
	"1",					/* button27 */
	"2",					/* button28 */
	"3",					/* button29 */
	"-",					/* button30 */
	"ON",					/* button31 */
	"DRG",					/* button32 */
	"INV",					/* button33 */
	"STO",					/* button34 */
	"RCL",					/* button35 */
	"0",					/* button36 */
	".",					/* button37 */
	"SUM",					/* button38 */
	"+"					/* button39 */
    };
    Arg arg;
    XmString str;
    int i;
    for (i = 0; i < 39; i++) {
	str = XmStringCreateLocalized((char *)hp_labels[i]);
	XtSetArg(arg, XmNlabelString, str);
	XtSetValues(btns[i], &arg, 1);
	XmStringFree(str);
    }
}

static void set_ti_translations(Widget *btns)
{
    XtTranslations xlations;
    int i;
    for (i = 0; i < 55; i++) {
	xlations = XtParseTranslationTable(ti_translations[i]);
	XtOverrideTranslations(btns[i], xlations);
    }
}

static void set_hp_translations(Widget *btns)
{
    XtTranslations xlations;
    int i;
    for (i = 0; i < 39; i++) {
	if (i == 20 || i == 21)
	    continue;
	xlations = XtParseTranslationTable(hp_translations[i]);
	XtOverrideTranslations(btns[i], xlations);
    }
}

static void set_button_sizes(Widget *btns, int count, int width, int height)
{
    Arg args[2];
    XtSetArg(args[0], XmNwidth, (XtArgVal)width);
    XtSetArg(args[1], XmNheight, (XtArgVal)height);
    for (int i = 0; i < count; i++) {
	XtSetValues(btns[i], args, 2);
    }
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