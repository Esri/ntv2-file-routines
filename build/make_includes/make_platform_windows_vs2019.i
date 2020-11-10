# ------------------------------------------------------------------------- #
# Copyright 2020 Esri                                                       #
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
# Windoze setup for Visual Studio 2019
# ------------------------------------------------------------------------

OS_SUBTYPE := vs2019

ifeq ($(OS_ARCH), 32)
  ifndef CPU
    CPU    := x86
  endif
  CPU_ARCH := x86
else
  ifndef CPU
    CPU    := x64
  endif
  CPU_ARCH := x64
endif

PLATFORM   := win

OS_LD_VAR  := PATH

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
# arguments (like the compilers in any other OS).  These programs have the
# same names, but are in different directories.
#
# Also, cygwin behaves like Unix, so the bit-bucket is "/dev/null"
# instead of "nul".
#
ifdef PROGRAMFILES
  override ProgramFiles := $(subst \\,/,$(PROGRAMFILES))
  override DEV_NULL     := /dev/null
endif

# ------------------------------------------------------------------------
# Using Microsoft Visual Studio 2019 (16.0)
#
# The following directories may have to be modified according to your
# install location:
#
VSInstallDir   := $(ProgramFiles)/Microsoft Visual Studio/2019/Professional
WindowsSdkDir  := $(ProgramFiles)/Windows Kits/10
MSBuildDir     := $(VSInstallDir)/MSBuild/Current/Bin

# Get the Visual Studio Tools and Platform Version.
VCToolsVersion := $(strip $(firstword $(shell ls -r "$(VSInstallDir)/VC/Tools/MSVC")))
WindowsTargetPlatformVersion := $(strip $(firstword $(shell ls -r "$(WindowsSdkDir)/Include")))
VCInstallDir   := $(VSInstallDir)/VC/Tools/MSVC/$(VCToolsVersion)

# ------------------------------------------------------------------------

INCLUDE := $(VCInstallDir)/include
INCLUDE := $(INCLUDE);$(VCInstallDir)/atlmfc/include
INCLUDE := $(INCLUDE);$(WindowsSdkDir)/include/$(WindowsTargetPlatformVersion)/ucrt
INCLUDE := $(INCLUDE);$(WindowsSdkDir)/include/$(WindowsTargetPlatformVersion)/um
INCLUDE := $(INCLUDE);$(WindowsSdkDir)/include/$(WindowsTargetPlatformVersion)/shared
INCLUDE := $(INCLUDE);$(WindowsSdkDir)/include/$(WindowsTargetPlatformVersion)/winrt

LIB     := $(VCInstallDir)/lib/$(CPU_ARCH)
LIB     := $(LIB);$(VCInstallDir)/atlmfc/lib/$(CPU_ARCH)
LIB     := $(LIB);$(WindowsSdkDir)/lib/$(WindowsTargetPlatformVersion)/ucrt/$(CPU_ARCH)
LIB     := $(LIB);$(WindowsSdkDir)/lib/$(WindowsTargetPlatformVersion)/um/$(CPU_ARCH)

PATH    := $(VSInstallDir)/Common7/IDE;$(PATH)
PATH    := $(VSInstallDir)/Common7/tools;$(PATH)
PATH    := $(WindowsSdkDir)/bin/$(WindowsTargetPlatformVersion)/$(CPU_ARCH);$(PATH)
PATH    := $(MSBuildDir);$(PATH)

export INCLUDE
export LIB
export PATH

# ------------------------------------------------------------------------

VCC     := $(VCInstallDir)/bin/Host$(CPU_ARCH)/$(CPU_ARCH)/cl
VLD     := $(VCInstallDir)/bin/Host$(CPU_ARCH)/$(CPU_ARCH)/link
VLIB    := $(VCInstallDir)/bin/Host$(CPU_ARCH)/$(CPU_ARCH)/lib

VRC     := $(WindowsSdkDir)/bin/$(WindowsTargetPlatformVersion)/$(CPU_ARCH)/rc
VMT     := $(WindowsSdkDir)/bin/$(WindowsTargetPlatformVersion)/$(CPU_ARCH)/mt

VCCPP   := $(VCC)
VLDPP   := $(VLD)
VDLL    := $(VLD)
VDLLPP  := $(VLD)
VLINT   := true

# ------------------------------------------------------------------------

OBJ_EXT    := obj
LIB_EXT    := lib
DLL_EXT    := dll
EXE_EXT    := .exe

LIB_PREFIX :=
LIB_S_EXT  := lib
LIB_D_EXT  := lib

OS_CLEAN   := \
  *.obj \
  *.bsc \
  *.htm \
  *.idb \
  *.ilk \
  *.map \
  *.pch \
  *.pdb \
  *.res \
  *.sbr \
  *.tlh

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

CDEFS      = -nologo -permissive- -GF -EHsc $(FPOPTS) -Zi $(WARNLVL) $(LINK_LIB) -FD
CPPDEFS    = -nologo -permissive- -GF -EHsc $(FPOPTS) -Zi $(WARNLVL) $(LINK_LIB) -FD
ICDEFS     = \
   -Fo$(INT_DIR)/ \
   -Fd$(INT_DIR)/
EXEDEFS    = -nologo -manifest -incremental:no -debug -libpath:$(LIB_DIR)
DLLDEFS    = -nologo -manifest -incremental:no -debug -libpath:$(LIB_DIR) -dll
LIBDEFS    = -nologo

NETLIBS    = wsock32.lib

# ------------------------------------------------------------------------
# Finally! In Visual Studio 2010, Microsoft finally added a -nologo
# option to the rc program!  (In VS2005 they added logo output to the
# rc command but failed to add the -nologo option.  When I complained to
# Microsoft, their response was that the programmer assumed that everyone
# would always want to see that output.  The fact is that no one wants to
# see it, especially when the idiot stoopidly ended each line with "\r\r\n".)
#
RCDEFS   = -nologo -n -I$(BUILDNUM_DIR)
RCFLAGS += $(RCDEFS)
