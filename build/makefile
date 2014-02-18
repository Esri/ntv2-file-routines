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
# makefile for the libntv2 build files                                      #
#                                                                           #
# The only purpose of this makefile is to produce a source list.            #
# ------------------------------------------------------------------------- #

# ------------------------------------------------------------------------
# Setup our make environment
#
TOP_DIR := ..
include $(TOP_DIR)/build/make_includes/make.i

# ------------------------------------------------------------------------
# source files
#
MAKE_INCLUDES := \
  make_includes/make.i                        \
  make_includes/make_os_unix.i                \
  make_includes/make_os_windows.i             \
  $(wildcard make_includes/make_platform_*.i) \
  $(NULL)

VS_PROJECTS := \
  $(wildcard vs_projects/*.vcproj)            \
  $(wildcard vs_projects/*.vcxproj)           \
  $(NULL)

DIRSRC  := $(MAKE_INCLUDES) $(VS_PROJECTS)
ALLSRC  := makefile $(DIRSRC)

# ------------------------------------------------------------------------
# build targets
#
all     :
compile :
link    :

# ------------------------------------------------------------------------
# list all files
#
srclist :
	@ for i in $(ALLSRC); \
	  do echo build/$$i; \
	  done

# ------------------------------------------------------------------------
# clean targets
#
clean    :
clobber  :
notarget :
