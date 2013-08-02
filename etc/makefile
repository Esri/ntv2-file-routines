# ------------------------------------------------------------------------- #
# Copyright 2013 Esri                                                       #
#                                                                           #
# Licensed under the Apache License, Version 2.0 (the "License");           #
# you may not use this file except in compliance with the License.          #
# You may obtain a copy of the License at                                   #
#                                                                           #
#     http://www.apache.org/licenses/LICENSE-2.0                            #
#                                                                           #
# Unless required by applicable law or agreed to in writing, software       #
# distributed under the License is distributed on an "AS IS" BASIS,         #
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  #
# See the License for the specific language governing permissions and       #
# limitations under the License.                                            #
# ------------------------------------------------------------------------- #

# ------------------------------------------------------------------------- #
# makefile for various libntv2 test programs                                #
# ------------------------------------------------------------------------- #

# ------------------------------------------------------------------------
# Setup our make environment
#
TOP_DIR  := ..
include $(TOP_DIR)/build/make_includes/make.i

# ------------------------------------------------------------------------
# List of targets
#
TGTBASE_FILE := ntv2_file
TGTBASE_CVT  := ntv2_cvt

FILE := $(BIN_DIR)/$(TGTBASE_FILE)$(EXE_EXT)
CVT  := $(BIN_DIR)/$(TGTBASE_CVT)$(EXE_EXT)

TARGETS := \
  $(FILE) \
  $(CVT)  \
  $(NULL)

# ------------------------------------------------------------------------
# program options
#
CINCS   = \
  -I../include \
  $(NULL)

CFLAGS   := $(COPTS)  $(OPTS) $(CDEFS)   $(CINCS)
EXEFLAGS := $(LDOPTS) $(OPTS) $(EXEDEFS)

# ------------------------------------------------------------------------
# object files
#
FILE_OBJS := $(addprefix $(INT_DIR)/, $(TGTBASE_FILE).$(OBJ_EXT) )
CVT_OBJS  := $(addprefix $(INT_DIR)/, $(TGTBASE_CVT).$(OBJ_EXT)  )

OBJS := \
  $(FILE_OBJS) \
  $(CVT_OBJS)  \
  $(NULL)

# ------------------------------------------------------------------------
# source files
#
C_SRC  = $(notdir $(OBJS:.$(OBJ_EXT)=.c))

HDRS   = \
  $(NULL)

OTHER  = \
  $(NULL)

CHSRC   = $(C_SRC) $(HDRS)
DIRSRC  = $(CHSRC) $(OTHER)
ALLSRC  = makefile $(DIRSRC)

# ------------------------------------------------------------------------
# libraries
#
ifeq ($(OS_TYPE), windows)
  LIBS   := libntv2$(LIB_SUFFIX).$(LIB_EXT)
else
  LIBS   := -lntv2$(LIB_SUFFIX)
endif

LIBS_DEP := $(LIB_DIR)/libntv2$(LIB_SUFFIX).$(LIB_D_EXT)

# ------------------------------------------------------------------------
# build targets
#
all : compile link

compile : $(OBJS)

link : $(TARGETS)

$(BIN_DIR)/%$(EXE_EXT) : $(INT_DIR)/%.$(OBJ_EXT) $(LIBS_DEP)
	@ $(MK_EXE)

# ------------------------------------------------------------------------
# dependencies
#
$(OBJS) : ../include/libntv2.h

# ------------------------------------------------------------------------
# list all files
#
srclist :
	@ for i in $(ALLSRC); \
	  do echo etc/$$i; \
	  done

# ------------------------------------------------------------------------
# clean targets
#
clean : notarget
	$(RD) $(INT_DIR)

notarget :
	$(RM) $(TARGETS)
