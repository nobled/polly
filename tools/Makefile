##===- projects/polly/tools/Makefile ----------------------*- Makefile -*-===##

#
# Relative path to the top of the source tree.
#
LEVEL=..

#
# List all of the subdirectories that we will compile.
#
DIRS := opt bugpoint

TOOLSDIR := $(LEVEL)/..

DIRS := $(addprefix $(TOOLSDIR)/,$(DIRS))

include $(LEVEL)/Makefile.common
