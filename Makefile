# Copied from Maxwell Bruce's repository, modified: https://github.com/Protryon/flex-language/blob/master/Makefile

DEPFILE = Makefile.dep
NOINCL = ci clean spotless
NEEDINCL = ${filter ${NOINCL}, ${MAKECMDGOALS}}

CC = gcc
CFLAGS = -O3
CFLAGSDEP = -MM

EXECOUT = riscvsim
SRCDIRS = src
SOURCELIST = ${foreach MOD, ${SRCDIRS}, ${wildcard ${MOD}/*.h ${MOD}/*.c}}
CSRC = ${filter %.c,${SOURCELIST}}
BUILD_DIR = build
OBJS = ${CSRC:.c=.o}
OBJS_BUILD = ${OBJS:%=${BUILD_DIR}/%}
LIBS = -lm

all: ${DEPFILE} ${EXECOUT}

riscvsim: ${OBJS}
	${CC} ${CFLAGS} -o ${BUILD_DIR}/$@ ${OBJS_BUILD} ${LIBS}

${BUILD_DIR}/%.o: %.c
	- mkdir -p ${dir $@}
	${CC} ${CFLAGS} -c $< -o $@

%.o: %.c
	- mkdir -p ${BUILD_DIR}/${dir $@}
	${CC} ${CFLAGS} -c $< -o ${BUILD_DIR}/$@

clean:
	- rm -rf ${BUILD_DIR} ${DEPFILE}

dep: ${ALLSOURCE}
	${CC} ${CFLAGSDEP} ${CSRC} >>${DEPFILE}

${DEPFILE}: dep

ifeq (${NEEDINCL}, )
include ${DEPFILE}
endif