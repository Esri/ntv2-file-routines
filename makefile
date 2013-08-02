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
# top-level makefile for the libntv2 library                                #
# ------------------------------------------------------------------------- #

# ------------------------------------------------------------------------
# Setup our make environment
#
TOP_DIR  := .
include $(TOP_DIR)/build/make_includes/make.i

# ------------------------------------------------------------------------
# List of directories to process
#
DIRS := \
  build   \
  include \
  doc     \
  samples \
  src     \
  etc     \
  $(NULL)

# ------------------------------------------------------------------------
# source files
#
OTHER  := \
  .gitignore    \
  CONTENTS      \
  license.txt   \
  README.md     \
  ntv2_2008.sln \
  ntv2_2012.sln \
  $(NULL)

DIRSRC  = $(OTHER)
ALLSRC  = makefile $(DIRSRC)

# ------------------------------------------------------------------------
# Build targets
#
all compile link :
	@ for d in $(DIRS); \
	  do \
	    echo ==== $$d >&2; \
	    [ -d $$d ] && { (cd $$d; $(MAKE) $@ || exit 1) || exit 1; }; \
	  done

# ------------------------------------------------------------------------
# List all source files
#
# (The crazy sed/sort/sed part of the commands sorts all filenames
# in a directory ahead of any sub-directories.)
#
srclist :
	@ ( \
	    for i in $(ALLSRC); \
	    do echo ./$$i; \
	    done; \
	    for d in $(DIRS); \
	    do \
	      echo ==== $$d >&2; \
	      [ -d $$d ] && (cd $$d; $(MAKE) $@); \
	    done \
	  ) | \
	  sed \
	    -e '/^\.\/.*\//s/\.\///' \
	    -e '/^[^/]*$$/s/^/.\//'  \
	    -e '/\/[^/]*$$/s//\/.&/' | \
	  sort | \
	  sed -e 's/\/\.\//\//'

src.list :
	@ $(MAKE) srclist >$@

flist :
	@ find . -type f -print | \
	  grep -v .git/ | \
	  sed \
	    -e '/^\.\/.*\//s/\.\///' \
	    -e '/^[^/]*$$/s/^/.\//'  \
	    -e '/\/[^/]*$$/s//\/.&/' | \
	  sort | \
	  sed -e 's/\/\.\//\//'

file.list :
	@ $(MAKE) flist >$@

# ------------------------------------------------------------------------
# Clean all files
#
clean :
	@ for d in $(DIRS); \
	  do \
	    echo ==== $$d >&2; \
	    [ -d $$d ] && (cd $$d; $(MAKE) $@); \
	  done
	  $(RD) $(INT_DIR)
