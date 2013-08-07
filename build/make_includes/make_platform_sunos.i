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
# Sunos (Solaris) setup
# ------------------------------------------------------------------------

OS_LD_VAR = LD_LIBRARY_PATH

VCC       = cc
VCCPP     = CC
VRC       = true
VLD       = cc
VLDPP     = CC
VLIB      = ar
VDLL      = CC
VDLLPP    = CC

OBJ_EXT   = o
LIB_EXT   = a
DLL_EXT   = so
EXE_EXT   =

LIB_S_EXT = a
LIB_D_EXT = so

ifeq ($(OS_ARCH), 64)
  ARCHITECTURE = -xarch=v9
endif

CDEFS     = -mt -xcode=pic32 $(ARCHITECTURE) $(WARN) -v
CPPDEFS   = -mt -xcode=pic32 $(ARCHITECTURE) $(WARN)
EXEDEFS   = -mt -L$(LIB_DIR) $(ARCHITECTURE)
EXEPPDEFS = -mt -L$(LIB_DIR) $(ARCHITECTURE)
LIBDEFS   = qc
DLLDEFS   = -mt -G -xcode=pic32 -z defs -L$(LIB_DIR) $(ARCHITECTURE)

OS_LIBS   = -lm -lpthread -lc
OS_LIBSPP = -lm -lpthread -lCstd -lCrun -lc
