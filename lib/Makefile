##===- polly/lib/Makefile -----------------------*- Makefile -*-===##

#
# Indicate where we are relative to the top of the source tree.
#
LEVEL :=..

DIRS := Support

LIBRARYNAME=polly
BUILD_ARCHIVE := 1

CPP.Flags += $(POLLY_INC)

#
# Include Makefile.common so we know what to do.
#
include $(LEVEL)/Makefile.common

#
# Dose scoplib found?
#
ifdef OPENSCOP_FOUND
  DIRS += Exchange
endif
