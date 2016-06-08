#
# Ex skeletal makefile for version 7
#
# NB: This makefile doesn't indicate any dependencies on header files.
#
# Compiles in both the LISP code and (the undocumented) chdir
# command within the editor; take these out of CFLAGS to make some
# breathing room in the text space if necessary.  If you take out -DLISP
# you should move ex_vars.nolisp to ex_vars.h
#
# If your system expands tabs to 4 spaces you should -DTABS=4 below
#
# Ex is likely to overflow the symbol table in your C compiler, so
# it uses -t0 which is (purportedly) a C compiler with a larger
# symbol table.  The -t1 flag to the C compiler is for a C compiler
# which puts switch code in I space, increasing the text space size
# to the benefit of per-user data space.  If you don't have this it
# doesn't matter much.
#
# Ex wants stdio only to get the doprnt.o routine; if other stdio stuff
# gets dragged in that is a mistake.
#
PREFIX=	${DESTDIR}/usr/local
BINDIR=	${PREFIX}/bin
LIBDIR=	${PREFIX}/libexec
#
# Either none or both of the next two lines needs to be uncommented
#
#D_SBRK=	-DUNIX_SBRK
#MALLOC_O=mapmalloc.o
CTAGS=	/usr/ucb/ctags
_CFLAGS=	-DTABS=8 -DLISP -DCHDIR -DUCVISUAL -Wall -Wextra \
	-g -O0 -O0 -fno-omit-frame-pointer -fno-optimize-sibling-calls \
	-fsanitize=undefined \
	-fsanitize=integer \
	-fsanitize=address
_LDFLAGS=
OBJS=	ex.o ex_addr.o ex_cmds.o ex_cmds2.o ex_cmdsub.o ex_data.o ex_get.o \
	ex_io.o ex_put.o ex_re.o ex_set.o ex_subr.o ex_temp.o ex_tty.o \
	ex_v.o ex_vadj.o ex_vget.o ex_vmain.o ex_voperate.o \
	ex_vops.o ex_vops2.o ex_vops3.o ex_vput.o ex_vwind.o \
	3printf.o

all: a.out exrecover expreserve #tags

tags:
	${CTAGS} ex.c ex_*.c

${OBJS}: ex_vars.h ex.h

#ex_vars.h:
#	csh makeoptions ${_CFLAGS}

a.out: ${OBJS}
	${CC} ${CFLAGS} ${_CFLAGS} ${LDFLAGS} ${_LDFLAGS} ${OBJS} -ltinfo

exrecover: exrecover.o
	${CC} ${CFLAGS} ${_CFLAGS} ${LDFLAGS} ${_LDFLAGS} -o $@ exrecover.o

expreserve: expreserve.o
	${CC} ${CFLAGS} ${_CFLAGS} ${LDFLAGS} ${_LDFLAGS} -o $@ expreserve.o

.c.o:
	${CC} ${CFLAGS} -c $<

clean:
	-rm -f a.out exrecover expreserve ex2.0strings strings core trace tags
	-echo if we dont have ex we cant make it so dont rm ex_vars.h
	-rm -f *.o x*.[cs]

distclean: clean
	rm -rf Makefile config.log

install: ${BINDIR} ${LIBDIR}
	install a.out ${BINDIR}/ex
	for i in vi view edit; do \
		ln -sf ${BINDIR}/ex ${BINDIR}/$$i; \
	done
	for i in recover preserve; do \
		install ex$$i ${LIBDIR}/ex${VERSION}$$i; \
	done

uninstall:
	for i in ex vi view edit; do \
		rm -f ${BINDIR}/$$i; \
	done
	for i in recover preserve; do \
		rm -f ${LIBDIR}/ex${VERSION}$$i; \
	done

${BINDIR} ${LIBDIR}:
	mkdir -p $@

lint:
	lint ex.c ex_?*.c
	lint -u exrecover.c
	lint expreserve.c
