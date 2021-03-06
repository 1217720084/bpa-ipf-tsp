# File: Makefile
# Purpose: compile and link gui.c.
# 
#
# Need extra suffixes for uid file compilation.
#
.SUFFIXES: .uil .uid

CC=/usr/bin/gcc

FOR=/usr/bin/g77
 
TOP=/usr/X11R6
UIL=/usr/X11R6/bin/uil
UILINC =	-I../gui \
		-I/usr/X11R6/include/uil
UILFLAGS = 	-I/usr/X11R6/include/uil
UIDDIR          = ../gui

CINCLUDES =	-I.  -I../gui -I../ipc -I$(TOP) -I$(TOP)/include \
		-I$(TOP)/include/Mrm -I$(TOP)/include/X11 -I$(TOP)/include/Xm

COPTIONS =	-O4 -DTOP_DOWN -DOSF_2_0 -DIPFCONSTOK -DUNDERSCORE -DX_LOCALE
CFLAGS =	$(COPTIONS)

LINTFLAGS =	$(CINCLUDES)

LDFLAGS =	-g -L../gui -L../ipc -L$(TOP)/lib
 
LIBS =		-lgui -lipc \
		-lMrm -lXm -lXpm -lXmu -lXt -lXext -lX11 -lSM -lICE -lm 

#
# the IMAGE variable is used to create the executable name
 
IMAGE = gui

#
# OBJS is a customized list of modules that are being worked on
# and are present in the current directory
  
OBJS = gui.o
 
UILS = gui.uil

UID = gui.uid

#
# This is the dependancy relationship for powerflow which
# specifies the link and causes the use of the utility compile 
# procedures below.
# WARNING: 
# The <tab> as the first character signifies the action to
# take for a given dependancy.
# The back slash causes a continuation of the current line 
# even if the line is commented out. 

all: $(IMAGE) $(UID)

$(IMAGE) : $(OBJS)
	$(CC) $(CFLAGS) -o $(IMAGE) $(OBJS) $(LDFLAGS) $(LIBS)
	size $(IMAGE)

#
#
# The following are the compile procedures for any source code specified
# in the dependancy.  These override the system defaults.
# the first procedure is for any fortran code (appendix .f creating .o)
# the second is for an C code (appendix .c creating .o)

.c.o:
	$(CC) $(CFLAGS) $(CINCLUDES) $(DEFINES) -c $*.c

#
# new suffix rule for compiling uil files.
#

.uil.uid:
	$(UIL) $(UILFLAGS) $(UILINC) -o $*.uid $<
