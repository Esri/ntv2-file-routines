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
# makefile for the libntv2 library includes                                 #
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
HDRS  := \
  libntv2.h  \
  $(NULL)

DIRSRC  := $(HDRS) $(OTHER)
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
	  do echo include/$$i; \
	  done

# ------------------------------------------------------------------------
# clean targets
#
clean :
