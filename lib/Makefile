##===- polly/lib/Makefile -----------------------*- Makefile -*-===##

#
# Indicate where we are relative to the top of the source tree.
#
LEVEL :=..

DIRS = Support
USEDLIBS = pollysupport.a

LIBRARYNAME=LLVMPolly
LOADABLE_MODULE = 1

include $(LEVEL)/Makefile.config

CPP.Flags += $(POLLY_INC)

ifeq ($(OPENSCOP_FOUND),yes)
  DIRS += Exchange
  USEDLIBS += pollyexchange.a
endif

# TODO: Export symbols for RTTI or EH?

#
# Include Makefile.common so we know what to do.
#
include $(LEVEL)/Makefile.common

LIBS += $(POLLY_LD) $(POLLY_LIB)

$(LibDir)/libpollyexchange.a : $(LibDir)/.dir $(PROJ_OBJ_DIR)/Exchange/$(BuildMode)/.dir \
                               $(PROJ_SRC_DIR)/Exchange/*
	$(Verb) if [ -d $(PROJ_SRC_DIR)/Exchange ]; then\
    if ([ ! -f Exchange/Makefile ] || \
        command test Exchange/Makefile -ot $(PROJ_SRC_DIR)/Exchange/Makefile ); then \
      $(MKDIR) Exchange; \
      $(CP) $(PROJ_SRC_DIR)/Exchange/Makefile Exchange/Makefile; \
    fi; \
    ($(MAKE) -C Exchange $@ ) || exit 1; \
  fi

$(LibDir)/libpollysupport.a : $(LibDir)/.dir $(PROJ_OBJ_DIR)/Support/$(BuildMode)/.dir \
                              $(PROJ_SRC_DIR)/Support/*
	$(Verb) if [ -d $(PROJ_SRC_DIR)/Support ]; then\
    if ([ ! -f Support/Makefile ] || \
        command test Support/Makefile -ot $(PROJ_SRC_DIR)/Support/Makefile ); then \
      $(MKDIR) Support; \
      $(CP) $(PROJ_SRC_DIR)/Support/Makefile Support/Makefile; \
    fi; \
    ($(MAKE) -C Support $@ ) || exit 1; \
  fi
