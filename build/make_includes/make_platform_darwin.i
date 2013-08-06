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

# ------------------------------------------------------------------------
# Mac OS/X (darwin)  setup
# ------------------------------------------------------------------------

OS_LD_VAR = DYLD_LIBRARY_PATH

VCC       = gcc
VCCPP     = g++
VRC       = true
VLD       = gcc
VLDPP     = g++
VLIB      = ar
VDLL      = gcc
VDLLPP    = g++

OBJ_EXT   = o
LIB_EXT   = a
DLL_EXT   = dylib
EXE_EXT   =

LIB_S_EXT = a
LIB_D_EXT = dylib

ifeq ($(OS_ARCH), 64)
  ARCHITECTURE = -m64
else
  ARCHITECTURE = -m32
endif

ifndef WARN
  WARN    = -Wall -Werror
endif

CDEFS     = $(ARCHITECTURE) $(WARN) -Ddarwin
CPPDEFS   = $(ARCHITECTURE) $(WARN) -Ddarwin
EXEDEFS   = $(ARCHITECTURE) -L$(LIB_DIR)
EXEPPDEFS = $(ARCHITECTURE) -L$(LIB_DIR)
LIBDEFS   = qc
DLLDEFS   = $(ARCHITECTURE) -L$(LIB_DIR) -dynamiclib

OS_LIBS   = -lpthread
OS_LIBSPP = -lpthread -lstdc++
