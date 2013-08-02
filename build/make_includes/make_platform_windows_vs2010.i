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
# Windoze setup for Visual Studio 2010
# ------------------------------------------------------------------------

OS_LD_VAR = PATH

# ------------------------------------------------------------------------
# cygwin stuff
#
# cygwin maps all existing environment variables to all upper-case.
# So, if we find the environment variable "PROGRAMFILES", we know that
# we are in a cygwin environment, since Windoze uses the variable
# "ProgramFiles".  So we simply set our ProgramFiles variable to that value.
# We also change all \ to / in the name.  Note that it still has to be
# quoted when it is used, since it usually contains spaces in the name.
#
# The reason for needing the absolute path to the Visual Studio programs
# (cl, link, rc, etc.) is that the 32-bit and the 64-bit versions are
# different programs, instead of just being the same program with different
# arguments (like all other C/C++ compilers).  These programs have the
# same names, but are in different directories. To make things worse,
# they keep changing the name of the default install directories and
# they insist on having spaces in the name. So, we have to end up hard-
# coding these pathnames below. You may have to change them for your
# machine. Sorry.
#
# Also, cygwin behaves like Unix, so the bit-bucket is "/dev/null"
# instead of "nul".
#
ifdef PROGRAMFILES
  override ProgramFiles := $(subst \\,/,$(PROGRAMFILES))
  override DEV_NULL     := /dev/null
endif

# ------------------------------------------------------------------------
# Using Microsoft Visual Studio 2010 (10.0)
#
# The following directories may have to be modified according to your
# install location:
#
VSInstallDir  := $(ProgramFiles)/Microsoft Visual Studio 10.0
WindowsSdkDir := $(ProgramFiles)/Microsoft SDKs/Windows/v7.0A

# ------------------------------------------------------------------------
#
# NOTE: The following directories must be in your PATH for using
#       Visual Studio from the command line (the install directories
#       may have to be modified):
#
#    $ProgramFiles/Microsoft Visual Studio 10.0/VC/bin
#    $ProgramFiles/Microsoft Visual Studio 10.0/Common7/IDE
#    $ProgramFiles/Microsoft Visual Studio 10.0/Common7/tools
#    $ProgramFiles/Microsoft SDKs/Windows/v7.0A/bin
#
# The following environment variables must contain the following directories:
#
#    INCLUDE =
#       $ProgramFiles/Microsoft Visual Studio 10.0/VC/include
#       $ProgramFiles/Microsoft SDKs/Windows/v7.0A/include
#
#    LIB =
#       $ProgramFiles/Microsoft Visual Studio 10.0/VC/lib
#       $ProgramFiles/Microsoft SDKs/Windows/v7.0A/lib
#
# ------------------------------------------------------------------------

INCLUDE := $(VSInstallDir)/VC/include
INCLUDE := $(INCLUDE);$(WindowsSdkDir)/include

PATH    := $(VSInstallDir)/VC/bin;$(PATH)
PATH    := $(VSInstallDir)/Common7/IDE;$(PATH)
PATH    := $(VSInstallDir)/Common7/tools;$(PATH)
PATH    := $(WindowsSdkDir)/bin;$(PATH)

ifeq ($(OS_ARCH), 64)
  LIB   := $(VSInstallDir)/VC/lib/amd64;$(WindowsSdkDir)/lib/x64

  VCC   := $(VSInstallDir)/VC/bin/x86_amd64/cl
  VLD   := $(VSInstallDir)/VC/bin/x86_amd64/link
  VLIB  := $(VSInstallDir)/VC/bin/x86_amd64/lib
else
  LIB   := $(VSInstallDir)/VC/lib;$(WindowsSdkDir)/lib

  VCC   := $(VSInstallDir)/VC/bin/cl
  VLD   := $(VSInstallDir)/VC/bin/link
  VLIB  := $(VSInstallDir)/VC/bin/lib
endif

export INCLUDE
export LIB
export PATH

VCCPP      := $(VCC)
VRC        := $(WindowsSdkDir)/bin/rc
VMT        := $(WindowsSdkDir)/bin/mt
VLDPP      := $(VLD)
VDLL       := $(VLD)
VDLLPP     := $(VLD)

OBJ_EXT    := obj
LIB_EXT    := lib
DLL_EXT    := dll
EXE_EXT    := .exe

LIB_PREFIX :=
LIB_S_EXT  := lib
LIB_D_EXT  := lib

# ------------------------------------------------------------------------
# Windoze-specific compiler options:
#
#  -MD           link with MSVCRT (multi-thread dynamic library)
#
#  -W[1-4]       warning level (minimal to anal, we use 3,
#                               although we are clean at level 4)
#  -WX           treat warnings as errors
#
#  -fp:precise   enable predictible floating-point operations
#
#  -GF           enable read-only string pooling
#  -EHs          (C++) enable exception handling
#  -EHc          (C++) extern "C" defaults to nothrow
#  -Zi           enable debugging information
#  -FD           undocumented (but needed for VS compatibility)
#
#  -Fo<name>     name the object file (or directory if ending with a /)
#  -Fd<name>     name the PDB    file (or directory if ending with a /)
# ------------------------------------------------------------------------
# Note that the following must be deferred
# ------------------------------------------------------------------------

ifndef LINK_LIB
  LINK_LIB = -MD
endif

ifndef WARN
   WARN    = -W4 -WX
endif
WARNLVL    = $(WARN) -D_CRT_SECURE_NO_WARNINGS -D_CRT_NONSTDC_NO_DEPRECATE

ifndef FPOPTS
   FPOPTS  = -fp:precise
endif

CDEFS      = -nologo -GF -EHsc $(FPOPTS) -Zi $(WARNLVL) $(LINK_LIB) -FD
CPPDEFS    = -nologo -GF -EHsc $(FPOPTS) -Zi $(WARNLVL) $(LINK_LIB) -FD
ICDEFS     = \
   -Fo$(INT_DIR)/ \
   -Fd$(INT_DIR)/
EXEDEFS    = -nologo -manifest -incremental:no -debug -libpath:$(LIB_DIR)
DLLDEFS    = -nologo -manifest -incremental:no -debug -libpath:$(LIB_DIR) -dll
LIBDEFS    = -nologo

# ------------------------------------------------------------------------
# Finally! In Visual Studio 2010, Microsoft finally added a -nologo
# option to the rc program!  (In VS2005 they added logo output to the
# rc command but failed to add the -nologo option.  When I complained to
# Microsoft, their response was that the programmer assumed that everyone
# would always want to see that output.  The fact is that no one wants to
# see it, especially when the idiot stoopidly ended each line with "\r\r\n".)
#
RCDEFS  := -nologo
