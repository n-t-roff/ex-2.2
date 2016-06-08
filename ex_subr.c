/* Copyright (c) 1979 Regents of the University of California */
#include "ex.h"
#include "ex_re.h"
#include "ex_tty.h"
#include "ex_vis.h"

/*
 * Random routines, in alphabetical order.
 */

static void qcount(int);

any(c, s)
	int c;
	register char *s;
{
	register int x;

	while (x = *s++)
		if (x == c)
			return (1);
	return (0);
}

backtab(i)
	register int i;
{
	register int j;

	j = i % value(SHIFTWIDTH);
	if (j == 0)
		j = value(SHIFTWIDTH);
	i -= j;
	if (i < 0)
		i = 0;
	return (i);
}

change()
{

	tchng++;
	chng = tchng;
}

/*
 * Column returns the number of
 * columns occupied by printing the
 * characters through position cp of the
 * current line.
 */
column(cp)
	register char *cp;
{

	if (cp == 0)
		cp = &linebuf[LBSIZE - 2];
	return (qcolumn(cp, (char *) 0));
}

Copy(to, from, size)
	register char *from, *to;
	register int size;
{

	if (size > 0)
		do
			*to++ = *from++;
		while (--size > 0);
}

copyw(to, from, size)
	register line *from, *to;
	register int size;
{

	if (size > 0)
		do
			*to++ = *from++;
		while (--size > 0);
}

copywR(to, from, size)
	register line *from, *to;
	register int size;
{

	while (--size >= 0)
		to[size] = from[size];
}

ctlof(c)
	int c;
{

	return (c == TRIM ? '?' : c | ('A' - 1));
}

dingdong()
{

	if (VB)
		putpad(VB);
	else if (value(ERRORBELLS))
		putch('\207');
}

fixindent(indent)
	int indent;
{
	register int i;
	register char *cp;

	i = whitecnt(genbuf);
	cp = vpastwh(genbuf);
	if (*cp == 0 && i == indent && linebuf[0] == 0) {
		genbuf[0] = 0;
		return (i);
	}
	CP(genindent(i), cp);
	return (i);
}

filioerr(cp)
	char *cp;
{
	register int oerrno = errno;

	lprintf("\"%s\"", cp);
	errno = oerrno;
	syserror();
}

char *
genindent(indent)
	register int indent;
{
	register char *cp;

	for (cp = genbuf; indent >= value(TABSTOP); indent -= value(TABSTOP))
		*cp++ = '\t';
	for (; indent > 0; indent--)
		*cp++ = ' ';
	return (cp);
}

getDOT()
{

	ex_getline(*dot);
}

line *
getmark(c)
	register int c;
{
	register line *addr;
	
	for (addr = one; addr <= dol; addr++)
		if (names[c - 'a'] == (*addr &~ 01)) {
			return (addr);
		}
	return (0);
}

getn(cp)
	register char *cp;
{
	register int i = 0;

	while (isdigit(*cp))
		i = i * 10 + *cp++ - '0';
	if (*cp)
		return (0);
	return (i);
}

ignnEOF()
{
	register int c = getchar();

	if (c == EOF)
		ungetchar(c);
}

iswhite(c)
	int c;
{

	return (c == ' ' || c == '\t');
}

junk(c)
	register int c;
{

	if (c && !value(BEAUTIFY))
		return (0);
	if (c >= ' ' && c != TRIM)
		return (0);
	switch (c) {

	case '\t':
	case '\n':
	case '\f':
		return (0);

	default:
		return (1);
	}
}

killed()
{

	killcnt(addr2 - addr1 + 1);
}

void
killcnt(int cnt)
{

	if (inopen) {
		notecnt = cnt;
		notenam = notesgn = "";
		return;
	}
	if (!notable(cnt))
		return;
	ex_printf("%d lines", cnt);
	if (value(TERSE) == 0) {
		ex_printf(" %c%s", Command[0] | ' ', Command + 1);
		if (Command[strlen(Command) - 1] != 'e')
			putchar('e');
		putchar('d');
	}
	putNFL();
}

lineno(a)
	line *a;
{

	return (a - zero);
}

lineDOL()
{

	return (lineno(dol));
}

lineDOT()
{

	return (lineno(dot));
}

markDOT()
{

	markpr(dot);
}

markpr(which)
	line *which;
{

	if ((inglobal == 0 || inopen) && which <= endcore) {
		names['z'-'a'+1] = *which & ~01;
		if (inopen)
			ncols['z'-'a'+1] = cursor;
	}
}

markreg(c)
	register int c;
{

	if (c == '\'' || c == '`')
		return ('z' + 1);
	if (c >= 'a' && c <= 'z')
		return (c);
	return (0);
}

/*
 * Mesg decodes the terse/verbose strings. Thus
 *	'xxx@yyy' -> 'xxx' if terse, else 'xxx yyy'
 *	'xxx|yyy' -> 'xxx' if terse, else 'yyy'
 * All others map to themselves.
 */
char *
mesg(str)
	register char *str;
{
	register char *cp;

	str = strcpy(genbuf, str);
	for (cp = str; *cp; cp++)
		switch (*cp) {

		case '@':
			if (value(TERSE))
				*cp = 0;
			else
				*cp = ' ';
			break;

		case '|':
			if (value(TERSE) == 0)
				return (cp + 1);
			*cp = 0;
			break;
		}
	return (str);
}

void
merror(char *s) {
	imerror(s, 0);
}

/*VARARGS2*/
void
imerror(char *seekpt, int i)
{
	register char *cp = linebuf;

	if (seekpt == 0)
		return;
	merror1(seekpt);
	if (*cp == '\n')
		putnl(), cp++;
	if (inopen && CE)
		vclreol();
	if (SO && SE)
		putpad(SO);
	ex_printf(mesg(cp), i);
	if (SO && SE)
		putpad(SE);
}

merror1(seekpt)
#ifdef lint
	char *seekpt;
#else
	int seekpt;
#endif
{

	strcpy(linebuf, seekpt);
}

morelines()
{

#ifdef UNIX_SBRK
	if ((int) sbrk(1024 * sizeof (line)) == -1)
		return (-1);
	endcore += 1024;
	return (0);
#else
	return (-1);
#endif
}

nonzero()
{

	if (addr1 == zero) {
		notempty();
		error("Nonzero address required@on this command");
	}
}

notable(i)
	int i;
{

	return (hush == 0 && !inglobal && i > value(REPORT));
}


notempty()
{

	if (dol == zero)
		error("No lines@in the buffer");
}


netchHAD(cnt)
	int cnt;
{

	netchange(lineDOL() - cnt);
}

void
netchange(int i)
{
	register char *cp;

	if (i > 0)
		notesgn = cp = "more ";
	else
		notesgn = cp = "fewer ", i = -i;
	if (inopen) {
		notecnt = i;
		notenam = "";
		return;
	}
	if (!notable(i))
		return;
	ex_printf(mesg("%d %slines@in file after %s"), i, cp, Command);
	putNFL();
}

putmark(addr)
	line *addr;
{

	putmk1(addr, putline());
}

putmk1(addr, n)
	register line *addr;
	int n;
{
	register line *markp;

	*addr &= ~1;
	for (markp = (anymarks ? names : &names['z'-'a'+1]); markp <= &names['z'-'a'+1]; markp++)
		if (*markp == *addr)
			*markp = n;
	*addr = n;
}

char *
plural(i)
	long i;
{

	return (i == 1 ? "" : "s");
}

short	vcntcol;

qcolumn(lim, gp)
	register char *lim, *gp;
{
	register int x;
	int (*OO)();

	OO = Outchar;
	Outchar = qcount;
	vcntcol = 0;
	if (lim != NULL)
		x = lim[1], lim[1] = 0;
	pline(0);
	if (lim != NULL)
		lim[1] = x;
	if (gp)
		while (*gp)
			putchar(*gp++);
	Outchar = OO;
	return (vcntcol);
}

static void
qcount(int c)
{

	if (c == '\t') {
		vcntcol += value(TABSTOP) - vcntcol % value(TABSTOP);
		return;
	}
	vcntcol++;
}

void
reverse(line *a1, line *a2)
{
	register line t;

	for (;;) {
		t = *--a2;
		if (a2 <= a1)
			return;
		*a2 = *a1;
		*a1++ = t;
	}
}

save(a1, a2)
	line *a1;
	register line *a2;
{
	register int more;

	undkind = UNDNONE;
	undadot = dot;
	more = (a2 - a1 + 1) - (unddol - dol);
	while (more > (endcore - truedol))
		if (morelines() < 0)
			error("Out of memory@saving lines for undo - try using ed or re");
	if (more)
		(*(more > 0 ? copywR : copyw))(unddol + more + 1, unddol + 1,
		    (truedol - unddol));
	unddol += more;
	truedol += more;
	copyw(dol + 1, a1, a2 - a1 + 1);
	undkind = UNDALL;
	unddel = a1 - 1;
	undap1 = a1;
	undap2 = a2 + 1;
}

save12()
{

	save(addr1, addr2);
}

saveall()
{

	save(one, dol);
}

span()
{

	return (addr2 - addr1 + 1);
}

ex_sync()
{

	chng = 0;
	tchng = 0;
	xchng = 0;
}


skipwh()
{
	register int wh;

	wh = 0;
	while (iswhite(peekchar())) {
		wh++;
		ignchar();
	}
	return (wh);
}

/*VARARGS2*/
void
smerror(char *seekpt, char *cp)
{

	if (seekpt == 0)
		return;
	merror1(seekpt);
	if (inopen && CE)
		vclreol();
	if (SO && SE)
		putpad(SO);
	lprintf(mesg(linebuf), cp);
	if (SO && SE)
		putpad(SE);
}

char *
strend(cp)
	register char *cp;
{

	while (*cp)
		cp++;
	return (cp);
}

strcLIN(dp)
	char *dp;
{

	CP(linebuf, dp);
}

syserror()
{
	dirtcnt = 0;
	putchar(' ');
	error(strerror(errno));
}

char *
vfindcol(i)
	int i;
{
	register char *cp;
	register int (*OO)() = Outchar;
	char *s;

	Outchar = qcount;
	s = linebuf;
	ignore(qcolumn(s - 1, NOSTR));
	for (cp = linebuf; *cp && vcntcol < i; cp++)
		putchar(*cp);
	if (cp != linebuf)
		cp--;
	Outchar = OO;
	return (cp);
}

char *
vskipwh(cp)
	register char *cp;
{

	while (iswhite(*cp) && cp[1])
		cp++;
	return (cp);
}


char *
vpastwh(cp)
	register char *cp;
{

	while (iswhite(*cp))
		cp++;
	return (cp);
}

whitecnt(cp)
	register char *cp;
{
	register int i;

	i = 0;
	for (;;)
		switch (*cp++) {

		case '\t':
			i += value(TABSTOP) - i % value(TABSTOP);
			break;

		case ' ':
			i++;
			break;

		default:
			return (i);
		}
}

#ifdef lint
Ignore(a)
	char *a;
{

	a = a;
}

Ignorf(a)
	int (*a)();
{

	a = a;
}
#endif

markit(addr)
	line *addr;
{

	if (addr != dot && addr >= one && addr <= dol)
		markDOT();
}

