##############################################################################
##                                                                          ##
##  COPYRIGHT (C) 2013 -- Eivind Storm Aarnæs (eistaa)                      ##
##                     -- eiv.s.aa@eistaa.com                               ##
##                                                                          ##
##   This program is free software: you can redistribute it and/or modify   ##
##   it under the terms of the GNU General Public License as published by   ##
##   the Free Software Foundation, either version 3 of the License, or      ##
##   (at your option) any later version.                                    ##
##                                                                          ##
##   This program is distributed in the hope that it will be useful,        ##
##   but WITHOUT ANY WARRANTY; without even the implied warranty of         ##
##   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          ##
##   GNU General Public License for more details.                           ##
##                                                                          ##
##############################################################################
##                                                                          ##
##   VERSION:   0.3                                                         ##
##   LAST EDIT: 21. Mar 2013                                                ##
##                                                                          ##
##############################################################################


# shell used by make
SHELL = /bin/sh


# -----------------------------------------------------------
#                       START OF CONFIGS
# -----------------------------------------------------------


PROJECT    = rf-memberlist
EXECNAME   = $(PROJECT)

SOURCEDIR  = ./src
BUILDDIR   = ./build

# prerequisites path (: separated list)
VPATH = .

# source extension (c / cpp / cxx / ...)
SOURCEEXT = c


##  COMPILER OPTIONS  ##
 ## ---------------- ##

# location to look for header files
INCLUDES    =
#INCLUDES   +=

# libraries to link with
LIBS        = -lncursesw
LIBS       += -lpanelw
LIBS       += -lformw

# compiler
CC          = clang

# compiler specific flags
CFLAGS      = -Wall
# linker flags
LDFLAGS     = -Wall

# space separated list of object files to compile
OBJS        = rf.o

# debug on/off (everything not  1  is turn it off)
DEBUG      ?= 0
# debug define (added as: '-D$(DEBUGDEFINE)' to CFLAGS )
DEBUGDEFINE = DEBUG


##  PACKAGING OPTIONS  ##
 ## ----------------- ##

## --------------------
## By default; only this makefile and SOURCEDIR with sources matching objects
## in OBJS are archived.
## Extras include a readme-file is defined in READMEFILE, and any content
## defined in EXTRAPACKPATHS
## --------------------

# package name (excluding file type suffix)
PACKNAME      = $(PROJECT)

# archiving commands
# - <out>      :: name of output file
# - <pathlist> :: list of paths archived
PACKCMDTAR    = tar czvf <out>.tar.gz <pathlist>
PACKCMDZIP    = zip <out>.zip <pathlist>

# command used for packaging
PACKCMD       = $(PACKCMDTAR)

##  EXTRA FILES  ##

# readmefile (empty to ignore)
READMEFILE    =

# space separated list of paths to add to archive
EXTRAPACKPATH =


# -----------------------------------------------------------
#                        END OF CONFIG
# -----------------------------------------------------------


##  SETUP  ##
 ## ----- ##

##  COMPILING  ##

# compiled object files
COMPOBJS = $(patsubst %,$(BUILDDIR)/%,$(OBJS))
# source files from object files
SRCS = $(patsubst %.o,$(SOURCEDIR)/%.$(SOURCEEXT),$(OBJS))

# add flag for debug mode
ifeq ($(DEBUG), 1)
    CFLAGS += -D$(DEBUGDEFINE)
endif

##  PACKAGING  ##

# generate path list
PACKPATHLIST = $(SRCS) $(firstword $(MAKEFILE_LIST)) \
        $(READMEFILE) $(EXTRAPACKPATHS)

# generate the actual package command
tmp = $(subst <out>,$(PACKNAME),$(PACKCMD))
ACTUALPACKCMD = $(subst <pathlist>,$(PACKPATHLIST),$(tmp))

# name of package file
FULLPACKNAME = $(patsubst <out>%,$(PACKNAME)%, \
        $(filter <out>%,$(PACKCMD)))


##  TARGETS  ##
 ## ------- ##

##  COMPILING  ##

# executed on 'make' or 'make all'
.PHONY: all
all: $(BUILDDIR)/$(EXECNAME)

# make $(BUILDDIR) only if it does not exist
$(BUILDDIR)/$(EXECNAME): $(COMPOBJS)
	$(CC) -o $(BUILDDIR)/$(EXECNAME) $(COMPOBJS) $(LDFLAGS) $(LIBS)

# make any file $(BUILDDIR)/stem.o from $(SOURCEDIR)/stem.$(SOURCEEXT)
# $@ :: name of current target, (left of : , $(BUILDDIR)/%.o)
# $^ :: name of prerequisites, (right of : , $(SOURCEDIR)/%.$(SOURCEEXT))
$(BUILDDIR)/%.o: $(SOURCEDIR)/%.$(SOURCEEXT) | $(BUILDDIR)
#	$(CC) $(INCLUDES) $(LIBS) -c $(CFLAGS) -o $@ $^
	$(CC) $(INCLUDES) -c $(CFLAGS) -o $@ $^

$(BUILDDIR):
	mkdir -p $(BUILDDIR)

##  PACKAGING  ##

.PHONY: pack
# package the project as specified in the PACKAGE section of the
# configuration
pack:
	rm -f $(FULLPACKNAME)
	$(ACTUALPACKCMD)

##  CLEANING  ##

.PHONY: clean
# clean object files and the executable
clean:
	rm -f $(BUILDDIR)/*.o $(BUILDDIR)/$(EXECNAME)

.PHONY: cleanobjs
# clean only object files
cleanobjs:
	rm -f $(BUILDDIR)/*.o

.PHONY: distclean
# clean all files (possibly) generated by this makefile
# (entire BUILDDIR, and archive created by pack)
distclean:
	rm -rf $(BUILDDIR) $(FULLPACKNAME)
