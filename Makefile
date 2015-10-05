# Makefile for the lab2 driver (based on that of hello).

COMPILER_TYPE= gnu

CC=gcc

PROG=	lab2
SRCS=	lab2.c timer.c

CCFLAGS= -Wall

DPADD+=	${LIBDRIVER} ${LIBSYS}
LDADD+= -ldriver -lsys

LDFLAGS+= -L .

MAN=

.include <bsd.prog.mk>
.include <bsd.gcc.mk>
