/* Copyright (c) 1979 Regents of the University of California */
#include <time.h>
#include "ex.h"
#include "ex_temp.h"
#include "ex_vis.h"

/*
 * Editor temporary file routines.
 * Very similar to those of ed, except uses 2 input buffers.
 */
#define	READ	0
#define	WRITE	1

struct	strreg {
	short	rg_flags;
	short	rg_nleft;
	short	rg_first;
	short	rg_last;
} strregs[('z'-'a'+1) + ('9'-'0'+1)], *strp;

struct	rbuf {
	short	rb_prev;
	short	rb_next;
	char	rb_text[BUFSIZ - 2 * sizeof (short)];
} *rbuf;

#define	INCORB	64
char	incorb[INCORB+1][BUFSIZ];
#define	pagrnd(a)	((char *)(((intptr_t)a)&~(BUFSIZ-1)))
int	stilinc;	/* up to here not written yet */

static void rbflush(void);
static void blkio(int, void *, ssize_t (*)());
static void regio(int, ssize_t (*)());
static int REGblk(void);
static struct strreg *mapreg(int);
static void KILLreg(int);
static ssize_t shread(void);
static int getREG(void);
static void kshift(void);
static void YANKline(void);
static void tflush(void);

char	tfname[40];
char	rfname[40];
int	havetmp;
int	tfile = -1;
int	rfile = -1;

void
fileinit(void)
{
	register char *p;
	register int i, j;
	struct stat stbuf;

	if (tline == INCRMT * 4)
		return;
	cleanup(0);
	close(tfile);
	tline = INCRMT * 4;
	blocks[0] = 2;
	blocks[1] = 3;
	blocks[2] = -1;
	dirtcnt = 0;
	iblock = -1;
	iblock2 = -1;
	oblock = -1;
	CP(tfname, svalue(DIRECTORY));
	if (stat(tfname, &stbuf)) {
dumbness:
		if (setexit() == 0)
			filioerr(tfname);
		else
			putNFL();
		cleanup(1);
		exit(1);
	}
	if ((stbuf.st_mode & S_IFMT) != S_IFDIR) {
		errno = ENOTDIR;
		goto dumbness;
	}
	ichanged = 0;
	ichang2 = 0;
	ignore(strcat(tfname, "/ExXXXXX"));
	for (p = strend(tfname), i = 5, j = getpid(); i > 0; i--, j /= 10)
		*--p = j % 10 | '0';
	tfile = creat(tfname, 0600);
	if (tfile < 0)
		goto dumbness;
	stilinc = 0;
	havetmp = 1;
	close(tfile);
	tfile = open(tfname, O_RDWR);
	if (tfile < 0)
		goto dumbness;
/* 	brk((char *)fendcore); */
}

void
cleanup(bool all)
{
	if (havetmp)
		unlink(tfname);
	havetmp = 0;
	if (all && rfile >= 0) {
		unlink(rfname);
		close(rfile);
		rfile = -1;
	}
}

void
ex_getline(line tl)
{
	register char *bp, *lp;
	register int nl;

	lp = linebuf;
	bp = getblock(tl, READ);
	nl = nleft;
	tl &= ~OFFMSK;
	while ((*lp++ = *bp++))
		if (--nl == 0) {
			bp = getblock(tl += INCRMT, READ);
			nl = nleft;
		}
}

line
putline(void)
{
	register char *bp, *lp;
	register int nl;
	line tl;

	dirtcnt++;
	lp = linebuf;
	change();
	tl = tline;
	bp = getblock(tl, WRITE);
	nl = nleft;
	tl &= ~OFFMSK;
	while ((*bp = *lp++)) {
		if (*bp++ == '\n') {
			*--bp = 0;
			linebp = lp;
			break;
		}
		if (--nl == 0) {
			bp = getblock(tl += INCRMT, WRITE);
			nl = nleft;
		}
	}
	tl = tline;
	tline += (((lp - linebuf) + BNDRY - 1) >> SHFT) & 077776;
	return (tl);
}

char *
getblock(line atl, int iof)
{
	register int bno, off;
	
	bno = (atl >> OFFBTS) & BLKMSK;
	off = (atl << SHFT) & LBTMSK;
	if (bno >= NMBLKS)
		error(" Tmp file too large");
	nleft = BUFSIZ - off;
	if (bno == iblock) {
		ichanged |= iof;
		hitin2 = 0;
		return (ibuff + off);
	}
	if (bno == iblock2) {
		ichang2 |= iof;
		hitin2 = 1;
		return (ibuff2 + off);
	}
	if (bno == oblock)
		return (obuff + off);
	if (iof == READ) {
		if (hitin2 == 0) {
			if (ichang2)
				blkio(iblock2, ibuff2, write);
			ichang2 = 0;
			iblock2 = bno;
			blkio(bno, ibuff2, read);
			hitin2 = 1;
			return (ibuff2 + off);
		}
		hitin2 = 0;
		if (ichanged)
			blkio(iblock, ibuff, write);
		ichanged = 0;
		iblock = bno;
		blkio(bno, ibuff, read);
		return (ibuff + off);
	}
	if (oblock >= 0)
		blkio(oblock, obuff, write);
	oblock = bno;
	return (obuff + off);
}

static void
blkio(int b, void *buf, ssize_t (*iofcn)())
{

	if (b < INCORB) {
		if (iofcn == read) {
			memmove(buf, pagrnd(incorb[b+1]), BUFSIZ);
			return;
		}
		memmove(pagrnd(incorb[b+1]), buf, BUFSIZ);
		if (laste) {
			if (b >= stilinc)
				stilinc = b + 1;
			return;
		}
	} else if (stilinc)
		tflush();
	lseek(tfile, b * BUFSIZ, SEEK_SET);
	if ((*iofcn)(tfile, buf, BUFSIZ) != BUFSIZ)
		filioerr(tfname);
}

static void
tflush(void)
{
	int i = stilinc;
	
	stilinc = 0;
	lseek(tfile, (long) 0, SEEK_SET);
	if (write(tfile, pagrnd(incorb[1]), i * BUFSIZ) != (i * BUFSIZ))
		filioerr(tfname);
}

/*
 * Synchronize the state of the temporary file in case
 * a crash occurs.
 */
void
synctmp(void)
{
	register int cnt;
	register line *a;
	register short *bp;

	if (stilinc)
		return;
	if (dol == zero)
		return;
	if (ichanged)
		blkio(iblock, ibuff, write);
	ichanged = 0;
	if (ichang2)
		blkio(iblock2, ibuff2, write);
	ichang2 = 0;
	if (oblock != -1)
		blkio(oblock, obuff, write);
	time(&H.Time);
	uid = getuid();
	*zero = (line) H.Time;
	for (a = zero, bp = blocks; a <= dol; a += BUFSIZ / sizeof *a, bp++) {
		if (*bp < 0) {
			tline = (tline + OFFMSK) &~ OFFMSK;
			*bp = ((tline >> OFFBTS) & BLKMSK);
			tline += INCRMT;
			oblock = *bp + 1;
			bp[1] = -1;
		}
		lseek(tfile, *bp * BUFSIZ, SEEK_SET);
		cnt = ((dol - a) + 2) * sizeof (line);
		if (cnt > BUFSIZ)
			cnt = BUFSIZ;
		if (write(tfile, (char *) a, cnt) != cnt) {
oops:
			*zero = 0;
			filioerr(tfname);
		}
		*zero = 0;
	}
	flines = lineDOL();
	lseek(tfile, 0l, SEEK_SET);
	if (write(tfile, (char *) &H, sizeof H) != sizeof H)
		goto oops;
}

void
TSYNC(void)
{

	if (dirtcnt > 12) {
		if (stilinc)
			tflush();
		dirtcnt = 0;
		synctmp();
	}
}

/*
 * Named buffer routines.
 * These are implemented differently than the main buffer.
 * Each named buffer has a chain of blocks in the register file.
 * Each block contains roughly 508 chars of text,
 * and a previous and next block number.  We also have information
 * about which blocks came from deletes of multiple partial lines,
 * e.g. deleting a sentence or a LISP object.
 *
 * We maintain a free map for the temp file.  To free the blocks
 * in a register we must read the blocks to find how they are chained
 * together.
 *
 * BUG:		The default savind of deleted lines in numbered
 *		buffers may be rather inefficient; it hasn't been profiled.
 */
short	rused[32];
short	rnleft;
short	rblock;
short	rnext;
char	*rbufcp;

static void
regio(int b, ssize_t (*iofcn)())
{

	if (rfile == -1) {
		CP(rfname, tfname);
		*(strend(rfname) - 7) = 'R';
		rfile = creat(rfname, 0600);
		if (rfile < 0)
oops:
			filioerr(rfname);
		close(rfile);
		rfile = open(rfname, O_RDWR);
		if (rfile < 0)
			goto oops;
	}
	lseek(rfile, b * BUFSIZ, SEEK_SET);
	if ((*iofcn)(rfile, rbuf, BUFSIZ) != BUFSIZ)
		goto oops;
	rblock = b;
}

static int
REGblk(void)
{
	register int i, j, m;

	for (i = 0; i < (ssize_t)(sizeof rused / sizeof rused[0]); i++) {
		m = (rused[i] ^ 0177777) & 0177777;
		if (i == 0)
			m &= ~1;
		if (m != 0) {
			j = 0;
			while ((m & 1) == 0)
				j++, m >>= 1;
			rused[i] |= (1 << j);
#ifdef RDEBUG
			ex_printf("allocating block %d\n", i * 16 + j);
#endif
			return (i * 16 + j);
		}
	}
	error("Out of register space (ugh)");
	/*NOTREACHED*/
	return 0;
}

static struct strreg *
mapreg(int c)
{

	if (isupper(c))
		c = tolower(c);
	return (isdigit(c) ? &strregs[('z'-'a'+1)+(c-'0')] : &strregs[c-'a']);
}

static void
KILLreg(int c)
{
	struct rbuf arbuf;
	register struct strreg *sp;

	rbuf = &arbuf;
	sp = mapreg(c);
	rblock = sp->rg_first;
	sp->rg_first = sp->rg_last = 0;
	sp->rg_flags = sp->rg_nleft = 0;
	while (rblock != 0) {
#ifdef RDEBUG
		ex_printf("freeing block %d\n", rblock);
#endif
		rused[rblock / 16] &= ~(1 << (rblock % 16));
		regio(rblock, shread);
		rblock = rbuf->rb_next;
	}
}

static ssize_t
shread(void)
{
	struct front { short a; short b; };

	if (read(rfile, (char *) rbuf, sizeof (struct front)) == sizeof (struct front))
		return (sizeof (struct rbuf));
	return (0);
}

void
putreg(int c)
{
	struct rbuf arbuf;
	register line *odot = dot;
	register line *odol = dol;
	register int cnt;

	deletenone();
	appendnone();
	rbuf = &arbuf;
	rnleft = 0;
	rblock = 0;
	rnext = mapreg(c)->rg_first;
	if (rnext == 0) {
		if (inopen) {
			splitw++;
			vclean();
			vgoto(WECHO, 0);
		}
		vreg = -1;
		ierror("Nothing in register %c", c);
	}
	if (inopen && partreg(c)) {
		squish();
		addr1 = addr2 = dol;
	}
	ignore(append(getREG, addr2));
	if (inopen && partreg(c)) {
		unddol = dol;
		dol = odol;
		dot = odot;
		pragged(0);
	}
	cnt = undap2 - undap1;
	killcnt(cnt);
	notecnt = cnt;
}

int
partreg(int c)
{

	return (mapreg(c)->rg_flags);
}

void
notpart(int c)
{

	if (c)
		mapreg(c)->rg_flags = 0;
}

static int
getREG(void)
{
	register char *lp = linebuf;
	register int c;

	for (;;) {
		if (rnleft == 0) {
			if (rnext == 0)
				return (EOF);
			regio(rnext, read);
			rnext = rbuf->rb_next;
			rbufcp = rbuf->rb_text;
			rnleft = sizeof rbuf->rb_text;
		}
		c = *rbufcp;
		if (c == 0)
			return (EOF);
		rbufcp++, --rnleft;
		if (c == '\n') {
			*lp++ = 0;
			return (0);
		}
		*lp++ = c;
	}
}

void
YANKreg(int c)
{
	struct rbuf arbuf;
	register line *addr;
	register struct strreg *sp;

	if (isdigit(c))
		kshift();
	if (islower(c))
		KILLreg(c);
	strp = sp = mapreg(c);
	sp->rg_flags = inopen && cursor && wcursor;
	rbuf = &arbuf;
	if (sp->rg_last) {
		regio(sp->rg_last, read);
		rnleft = sp->rg_nleft;
		rbufcp = &rbuf->rb_text[sizeof rbuf->rb_text - rnleft];
	} else {
		rblock = 0;
		rnleft = 0;
	}
	for (addr = addr1; addr <= addr2; addr++) {
		ex_getline(*addr);
		if (sp->rg_flags) {
			if (addr == addr2)
				*wcursor = 0;
			if (addr == addr1)
				CP(linebuf, cursor);
		}
		YANKline();
	}
	rbflush();
	killed();
}

static void
kshift(void)
{
	register int i;

	KILLreg('9');
	for (i = '8'; i >= '0'; i--)
		copy(mapreg(i+1), mapreg(i), sizeof (struct strreg));
}

static void
YANKline(void)
{
	register char *lp = linebuf;
	register struct rbuf *rp = rbuf;
	register int c;

	do {
		c = *lp++;
		if (c == 0)
			c = '\n';
		if (rnleft == 0) {
			rp->rb_next = REGblk();
			rbflush();
			rblock = rp->rb_next;
			rp->rb_next = 0;
			rp->rb_prev = rblock;
			rnleft = sizeof rp->rb_text;
			rbufcp = rp->rb_text;
		}
		*rbufcp++ = c;
		--rnleft;
	} while (c != '\n');
	if (rnleft)
		*rbufcp = 0;
}

static void
rbflush(void)
{
	register struct strreg *sp = strp;

	if (rblock == 0)
		return;
	regio(rblock, write);
	if (sp->rg_first == 0)
		sp->rg_first = rblock;
	sp->rg_last = rblock;
	sp->rg_nleft = rnleft;
}
