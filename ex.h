/* Copyright (c) 1979 Regents of the University of California */
#ifdef V6
#include <retrofit.h>
#endif

/*
 * Ex version 2.2, Release 1
 *
 * Bill Joy, UC Berkeley
 * May 1979
 *
 * This file contains most of the declarations common to a large number
 * of routines.  The file ex_vis.h contains declarations
 * which are used only inside the screen editor.
 * The file ex_tune.h contains parameters which can be diddled per installation.
 *
 * The declarations relating to the argument list, regular expressions,
 * the temporary file data structure used by the editor
 * and the data describing terminals are each fairly substantial and
 * are kept in the files ex_{argv,re,temp,tty}.h which
 * we #include separately.
 *
 * If you are going to dig into ex, you should look at the outline of the
 * distribution of the code into files at the beginning of ex.c and ex_v.c.
 * Code which is similar to that of ed is lightly or undocumented in spots
 * (e.g. the regular expression code).  Newer code (e.g. open and visual)
 * is much more carefully documented, and still rough in spots.
 *
 * Please forward bug reports to
 *
 *	Bill Joy
 *	Computer Science Division, EECS
 *	EVANS HALL
 *	U.C. Berkeley 94704
 *	(415) 642-4948
 *	(415) 642-1024 (dept. office)
 *
 * or to wnj@mit-mc on the ARPA-net.  I would particularly like to hear
 * of additional terminal descriptions you add to the termcap data base.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <ctype.h>
#include <errno.h>
#include <signal.h>
#include <setjmp.h>
#include <sys/stat.h>
#include <termios.h>
#include <fcntl.h>

int tgetent(char *bp, const char *name);
int tgetflag(char *id);
int tgetnum(char *id);
char *tgetstr(char *id, char **area);
char *tgoto(const char *cap, int col, int row);
int tputs(const char *str, int affcnt, int (*putc)(int));

typedef	int	line;
typedef	short	bool;

#include "ex_tune.h"

/*
 * LISP is defined for makeoptions;  ex_vars.h will redefine it
 */
#ifdef LISP
#undef LISP
#endif

#include "ex_vars.h"
/*
 * Options in the editor are referred to usually by "value(name)" where
 * name is all uppercase, i.e. "value(PROMPT)".  This is actually a macro
 * which expands to a fixed field in a static structure and so generates
 * very little code.  The offsets for the option names in the structure
 * are generated automagically from the structure initializing them in
 * ex_data.c... see the shell script "makeoptions".
 */
struct	option {
	char	*oname;
	char	*oabbrev;
	int	otype;		/* Types -- see below */
	int	odefault;	/* Default value */
	int	ovalue;		/* Current value */
	char	*osvalue;
};

#define	ONOFF	0
#define	NUMERIC	1
#define	STRING	2		/* SHELL or DIRECTORY */
#define	OTERM	3

#define	value(a)	options[a].ovalue
#define	svalue(a)	options[a].osvalue

extern struct	option options[NOPTS + 1];


/*
 * The editor does not normally use the standard i/o library.  Because
 * we expect the editor to be a heavily used program and because it
 * does a substantial amount of input/output processing it is appropriate
 * for it to call low level read/write primitives directly.  In fact,
 * when debugging the editor we use the standard i/o library.  In any
 * case the editor needs a printf which prints through "putchar" ala the
 * old version 6 printf.  Thus we normally steal a copy of the "printf.c"
 * and "strout" code from the standard i/o library and mung it for our
 * purposes to avoid dragging in the stdio library headers, etc if we
 * are not debugging.  Such a modified printf exists in "printf.c" here.
 */
#ifdef TRACE
	FILE	*trace;
	bool	trubble;
	bool	techoin;
	char	tracbuf[BUFSIZ];
#endif

/*
 * Character constants and bits
 *
 * The editor uses the QUOTE bit as a flag to pass on with characters
 * e.g. to the putchar routine.  The editor never uses a simple char variable.
 * Only arrays of and pointers to characters are used and parameters and
 * registers are never declared character.
 */
#ifdef BIT8
# define	QUOTE	0400
#else
# define	QUOTE	0200
#endif
#define	RE_QUOTE	0200
#define	TRIM	0177
#ifndef CTRL
#define	CTRL(c)	(c & 037)
#endif
#define	NL	CTRL('j')
#define	CR	CTRL('m')
#define	DELETE	0177		/* See also ATTN, QUIT in ex_tune.h */
#define	ESCAPE	033

/*
 * Miscellaneous random variables used in more than one place
 */
bool	aiflag;			/* Append/change/insert with autoindent */
bool	anymarks;		/* We have used '[a-z] */
int	chng;			/* Warn "No write" */
char	*Command;
bool	die;			/* We are child, go away on error */
int	dirtcnt;		/* When >= MAXDIRT, should sync temporary */
bool	edited;			/* Current file is [Edited] */
line	*endcore;		/* Last available core location */
extern bool	endline;		/* Last cmd mode command ended with \n */
#if 0
short	erfile;			/* Error message file unit */
#endif
line	*fendcore;		/* First address in line pointer space */
char	file[FNSIZE];		/* Working file name */
char	genbuf[LBSIZE];		/* Working buffer when manipulating linebuf */
bool	hush;			/* Command line option - was give, hush up! */
char	*globp;			/* (Untyped) input string to command mode */
bool	holdcm;			/* Don't cursor address */
bool	inglobal;		/* Inside g//... or v//... */
char	*initev;		/* Initial : escape for visual */
bool	inopen;			/* Inside open or visual */
char	*input;			/* Current position in cmd line input buffer */
bool	intag;			/* In tag file... force nomagic re's */
bool	intty;			/* Input is a tty */
short	io;			/* General i/o unit (auto-closed on error!) */
extern short	lastc;			/* Last character ret'd from cmd input */
bool	laste;			/* Last command was an "e" (or "rec") */
char	lasttag[TAGSIZE];	/* Last argument to a tag command */
char	*linebp;		/* Used in substituting in \n */
char	linebuf[LBSIZE];	/* The primary line buffer */
bool	listf;			/* Command should run in list mode */
char	*loc1;			/* Where re began to match (in linebuf) */
char	*loc2;			/* First char after re match (") */
line	names['z'-'a'+2];	/* Mark registers a-z,' */
int	notecnt;		/* Count for notify (to visual from cmd) */
bool	numberf;		/* Command should run in number mode */
char	obuf[BUFSIZ];		/* Buffer for tty output */
speed_t	ex_ospeed;			/* Output speed (from gtty) */
short	peekc;			/* Peek ahead character (cmd mode input) */
char	*pkill[2];		/* Trim for put with ragged (LISP) delete */
bool	pfast;			/* Have stty -nl'ed to go faster */
int	pid;			/* Process id of child */
jmp_buf	resetlab;		/* For error throws to top level (cmd mode) */
int	rpid;			/* Pid returned from wait() */
bool	ruptible;		/* Interruptible is normal state */
bool	shudclob;		/* Have a prompt to clobber (e.g. on ^D) */
int	status;			/* Status returned from wait() */
int	tchng;			/* If nonzero, then [Modified] */
extern int	tfile;			/* Temporary file unit */
bool	vcatch;			/* Want to catch an error (open/visual) */
jmp_buf	vreslab;		/* For error throws to a visual catch */
int	xchng;			/* Suppresses multiple "No writes" in !cmd */

/*
 * Macros
 */
#define	CP(a, b)	memmove(a, b, strlen(b) + 1)
#define	copy(a,b,c)	Copy((char *) a, (char *) b, c)
#define	eq(a, b)	((void *)(a) != NULL && (void *)(b) != NULL && strcmp(a, b) == 0)
#define	getexit(a)	copy(a, resetlab, sizeof (jmp_buf))
#define	lastchar()	lastc
#define	outchar(c)	(*Outchar)(c)
#define	pastwh()	(ignore(skipwh()))
#define	pline(no)	(*Pline)(no)
#define	reset()		longjmp(resetlab,1)
#define	resexit(a)	copy(resetlab, a, sizeof (jmp_buf))
#define	setexit()	setjmp(resetlab)
#define	setlastchar(c)	lastc = c
#define	ungetchar(c)	peekc = c
#define	ungetchar(c)	peekc = c

#define	CATCH		vcatch = 1; if (setjmp(vreslab) == 0) {
#define	ONERR		} else { vcatch = 0;
#define	ENDCATCH	} vcatch = 0;

/*
 * Environment like memory
 */
char	altfile[FNSIZE];	/* Alternate file name */
extern char	direct[ONMSZ];		/* Temp file goes here */
extern char	shell[ONMSZ];		/* Copied to be settable */
extern char	ttytype[ONMSZ];		/* A long and pretty name */
char	uxb[UXBSIZE + 2];	/* Last !command for !! */

/*
 * The editor data structure for accessing the current file consists
 * of an incore array of pointers into the temporary file tfile.
 * Each pointer is 15 bits (the low bit is used by global) and is
 * padded with zeroes to make an index into the temp file where the
 * actual text of the line is stored.
 *
 * To effect undo, copies of affected lines are saved after the last
 * line considered to be in the buffer, between dol and unddol.
 * During an open or visual, which uses the command mode undo between
 * dol and unddol, a copy of the entire, pre-command buffer state
 * is saved between unddol and truedol.
 */
line	*addr1;			/* First addressed line in a command */
line	*addr2;			/* Second addressed line */
line	*dol;			/* Last line in buffer */
line	*dot;			/* Current line */
line	*one;			/* First line */
line	*truedol;		/* End of all lines, including saves */
line	*unddol;		/* End of undo saved lines */
line	*zero;			/* Points to empty slot before one */

/*
 * Undo information
 *
 * For most commands we save lines changed by salting them away between
 * dol and unddol before they are changed (i.e. we save the descriptors
 * into the temp file tfile which is never garbage collected).  The
 * lines put here go back after unddel, and to complete the undo
 * we delete the lines [undap1,undap2).
 *
 * Undoing a move is much easier and we treat this as a special case.
 * Similarly undoing a "put" is a special case for although there
 * are lines saved between dol and unddol we don't stick these back
 * into the buffer.
 */
short	undkind;

line	*unddel;		/* Saved deleted lines go after here */
line	*undap1;		/* Beginning of new lines */
line	*undap2;		/* New lines end before undap2 */
line	*undadot;		/* If we saved all lines, dot reverts here */

#define	UNDCHANGE	0
#define	UNDMOVE		1
#define	UNDALL		2
#define	UNDNONE		3
#define	UNDPUT		4

/*
 * Function type definitions
 */
#define	NOSTR	(char *) 0
#define	NOLINE	(line *) 0

extern void	(*Outchar)();
extern void	(*Pline)();
extern void	(*Putchar)();
void	(*oldhup)(int);
void	(*setlist(bool))();
void	(*setnumb(bool))();
line	*address(void);
char	*cgoto(void);
char	*genindent(int);
char	*getblock(line, int);
line	*getmark(int);
char	*longname(char *, char *);
char	*mesg(char *);
char	*place(char *, char *, char *);
char	*plural(long);
void	setin(line *);
char	*strend(char *);
line	*vback(line *, int);
char	*vfindcol(int);
char	*vgetline(int, char *, bool *);
char	*vpastwh(char *);
char	*vskipwh(char *);
void	put(void);
void	putreg(int);
void	YANKreg(int);
void	delete(bool);
void	filter(int);
int	getfile(void);
int	getsub(void);
int	gettty(void);
void	join(int);
void	listchar(int);
void	normline(void);
void	numbline(int);
void	(*oldquit)(int);
void	onhup(int);
void	onintr(int);
int	putch(int);
void	shift(int, int);
void	termchar(int);
void	vfilter(void);
#ifndef V6
void	vintr(int);
#endif
void	vshftop(void);
void	yank(void);
void	ostop(struct termios);
struct termios setty(struct termios);
struct termios unixex(char *, char *, int, int);
struct termios ostart(void);
void	unixwt(bool, struct termios);
void	setall(void);
void	setcount(void);
void	commands(bool, bool);
void	vcontin(bool);
void	resetflav(void);
void	nomore(void);
void	ex_newline(void);
void	zop2(int, int);
void	tagfind(bool);
void	source(char *, bool);
void	putfile(void);
void	filename(int);
void	pstop(void);
void	pstart(void);
void	fgoto(void);
void	set(void);
void	merror(char *);
void	imerror(char *, int);
void	smerror(char *, char *);
void	error(char *);
void	ierror(char *, int);
void	serror(char *, char *);
void	reverse(line *, line *);
void	netchange(int);
void	killcnt(int);
void	synctmp(void);
void	fileinit(void);
void	gettmode(void);
void	vsetsiz(int);
void	savevis(void);
void	vop(void);
void	vdirty(int, int);
void	sethard(void);
void	vreplace(int, int, int);
void	vsync1(int);
void	vredraw(int);
void	vrepaint(char *);
void	vscrap(void);
void	vmoveitup(int);
void	addtext(char *);
void	setLAST(void);
void	vsave(void);
void	vmain(void);
void	operate(int, int);
void	vrep(int);
void	vundo(void);
void	vUndo(void);
void	takeout(char *);
void	physdc(int, int);
void	vgoto(int, int);
void	vclrech(bool);
void	vclreol(void);
void	vshow(line *, line *);
void	vdown(int, int, bool);
void	vup(int, int, bool);
void	ex_printf(const char *, ...);
void	tvliny(void);
void	clrstats(void);
void	ex_getline(line);
void	syserror(void);
void	ex_putchar(int);
void	flush(void);
void	flush1(void);
void	tab(int);
void	noteinp(void);
void	termreset(void);
void	draino(void);
void	flusho(void);
void	putnl(void);
void	putpad(char *);
void	setoutt(void);
void	lprintf(char *, char *);
void	putNFL(void);
void	gTTY(int);
void	noonl(void);
int	any(int, char *);
int	backtab(int);
void	change(void);
int	column(char *);
void	Copy(char *, char *, int);
void	copyw(line *, line *, int);
int	ctlof(int);
void	dingdong(void);
int	fixindent(int);
void	filioerr(char *);
void	getDOT(void);
int	getn(char *);
void	ignnEOF(void);
int	iswhite(int);
int	junk(int);
void	killed(void);
int	lineno(line *);
int	lineDOL(void);
int	lineDOT(void);
void	markDOT(void);
void	markpr(line *);
int	markreg(int);
int	morelines(void);
void	nonzero(void);
int	notable(int);
void	notempty(void);
void	netchHAD(int);
void	putmark(line *);
void	putmk1(line *, int);
int	qcolumn(char *, char *);
void	save12(void);
void	saveall(void);
int	span(void);
void	ex_sync(void);
int	skipwh(void);
void	strcLIN(char *);
int	whitecnt(char *);
void	markit(line *);
int	append(int (*)(), line *);
void	appendnone(void);
void	pargs(void);
void	deletenone(void);
void	squish(void);
void	move(void);
void	pragged(bool);
void	zop(int);
void	plines(line *, line *, bool);
void	pofix(void);
void	undo(bool);
void	cleanup(bool);
line	putline(void);
void	TSYNC(void);
int	partreg(int);
void	notpart(int);
int	getargs(void);
void	getone(void);
void	rop(int);
void	rop2(void);
void	rop3(int);
void	wop(void);
void	unix0(bool);
void	recover(void);
void	waitfor(void);
void	revocer(void);
int	cmdreg(void);
int	endcmd(int);
void	eol(void);
void	erewind(void);
void	fixol(void);
int	exclam(void);
void	makargs(void);
void	next(void);
int	quickly(void);
int	skipend(void);
void	tailspec(int);
void	tail(char *);
void	tail2of(char *);
void	tailprim(char *, int, bool);
void	vnfl(void);
void	init(void);
void	setrupt(void);
int	preserve(void);
void	setdot(void);
void	setdot1(void);
int	getnum(void);
void	setnoaddr(void);
void	setCNL(void);
void	setNAEOL(void);
void	global(bool);
int	substitute(int);
int	compile(int, int);
int	execute(int, line *);
void	ignchar(void);
int	ex_getchar(void);
int	getcd(void);
int	peekchar(void);
int	peekcd(void);
void	setterm(char *);

/*
 * C doesn't have a (void) cast, so we have to fake it for lint's sake.
 */
#ifdef lint
#	define	ignore(a)	Ignore((char *) (a))
#	define	ignorf(a)	Ignorf((int (*) ()) (a))
#else
#	define	ignore(a)	a
#	define	ignorf(a)	a
#endif
