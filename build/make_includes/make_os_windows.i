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
# Windoze-specific setup (included by make.i)                               #
# ------------------------------------------------------------------------- #

# ------------------------------------------------------------------------
# common compile/link options
#
ifndef DEBUG_OPTS
  DEBUG_OPTS    := -RTC1
endif

ifndef OPTIMIZE_OPTS
  OPTIMIZE_OPTS := -Ot
endif

ifeq ($(BUILD_MODE), debug)
   LINK_LIB  := $(LINK_LIB)d
   CDEFS     += $(DEBUG_OPTS) -D_DEBUG
   CPPDEFS   += $(DEBUG_OPTS) -D_DEBUG
   RCDEFS    += -D_DEBUG
   DLLDEFS   += -nodefaultlib:msvcrt
   EXEDEFS   += -nodefaultlib:msvcrt
else
   CDEFS     += $(OPTIMIZE_OPTS) -DNDEBUG
   CPPDEFS   += $(OPTIMIZE_OPTS) -DNDEBUG
   RCDEFS    += -DNDEBUG
endif

# ------------------------------------------------------------------------
# Get the name of the C & C++ compiler.
# Remember that there may be spaces in the full pathname.
#
VCC_NAME   := $(notdir $(word $(words $(VCC)),   $(VCC)  ))
VCCPP_NAME := $(notdir $(word $(words $(VCCPP)), $(VCCPP)))

# ------------------------------------------------------------------------
# manifest file macros
#
ifdef VERBOSE_MAKE
  define MANIFEST_SHR_SHOW
     echo 'mt -nologo -manifest $@.manifest "-outputresource:$@;2"';
  endef

  define MANIFEST_EXE_SHOW
     echo 'mt -nologo -manifest $@.manifest "-outputresource:$@;1"';
  endef
endif

define MANIFEST_SHR
   [ ! -f $@ -o ! -f $@.manifest ] || \
   { \
      $(MANIFEST_SHR_SHOW) \
      "$(VMT)" -nologo -manifest $@.manifest "-outputresource:$@;2" && \
      { [ "$(KEEP_MANIFEST_FILES)" != "" ] || $(RM) $@.manifest; }; \
   }
endef

define MANIFEST_EXE
   [ ! -f $@ -o ! -f $@.manifest ] || \
   { \
      $(MANIFEST_EXE_SHOW) \
      "$(VMT)" -nologo -manifest $@.manifest "-outputresource:$@;1" && \
      { [ "$(KEEP_MANIFEST_FILES)" != "" ] || $(RM) $@.manifest; }; \
   }
endef

# ------------------------------------------------------------------------
# create a static library
#
ifdef VERBOSE_MAKE
   define MK_LIB_SHOW
      echo '"$(VLIB)"' $(LIBDEFS) \
         -out:$@ \
         $(LIB_OBJS) | fold -s
   endef
else
   define MK_LIB_SHOW
      echo $(strip MK_LIB $@)
   endef
endif

define MK_LIB
   [ -d $(dir $@) ] || \
      { mkdir -p $(dir $@) 2>$(DEV_NULL); [ -d $(dir $@) ]; }
   $(RM) $@

   $(MK_LIB_SHOW)

   "$(VLIB)" $(LIBDEFS) \
      -out:$@ \
      $(LIB_OBJS)
endef

# ------------------------------------------------------------------------
# create a dynamic (shared) C library from object files
#
ifdef VERBOSE_MAKE
   define MK_SHR_SHOW
      echo '"$(VDLL)"' $(LDFLAGS) \
         -out:$@ \
         -pdb:$(PDB_DIR)/$(DLLNAME).pdb \
         -implib:$(LIB_DIR)/$(DLLNAME).$(LIB_EXT) \
         $(DEF_OPTS) \
         $(DLL_OBJS) \
         $(LIBS) $(OS_LIBS) | fold -s
   endef
else
   define MK_SHR_SHOW
      echo $(strip MK_SHR $(LDOPTS) $(OPTS) $@)
   endef
endif

define MK_SHR
   [ -d $(BIN_DIR) ] || \
      { mkdir -p $(BIN_DIR) 2>$(DEV_NULL); [ -d $(BIN_DIR) ]; }
   [ -d $(PDB_DIR) ] || \
      { mkdir -p $(PDB_DIR) 2>$(DEV_NULL); [ -d $(PDB_DIR) ]; }
   [ -d $(LIB_DIR) ] || \
      { mkdir -p $(LIB_DIR) 2>$(DEV_NULL); [ -d $(LIB_DIR) ]; }
   $(RM) $@ $@.manifest

   $(MK_SHR_SHOW)

   "$(VDLL)" $(LDFLAGS) \
      -out:$@ \
      -pdb:$(PDB_DIR)/$(DLLNAME).pdb \
      -implib:$(LIB_DIR)/$(DLLNAME).$(LIB_EXT) \
      $(DEF_OPTS) \
      $(DLL_OBJS) \
      $(LIBS) $(OS_LIBS)

   $(MANIFEST_SHR)
endef

# ------------------------------------------------------------------------
# create a dynamic (shared) C++ library from object files
#
ifdef VERBOSE_MAKE
   define MK_SHRPP_SHOW
      echo '"$(VDLLPP)"' $(LDFLAGS) \
         -out:$@ \
         -pdb:$(PDB_DIR)/$(DLLNAME).pdb \
         -implib:$(LIB_DIR)/$(DLLNAME).$(LIB_EXT) \
         $(DEF_OPTS) \
         $(DLL_OBJS) \
         $(LIBSPP) $(OS_LIBSPP) | fold -s
   endef
else
   define MK_SHRPP_SHOW
      echo $(strip MK_SHRPP $(LDOPTS) $(OPTS) $@)
   endef
endif

define MK_SHRPP
   [ -d $(BIN_DIR) ] || \
      { mkdir -p $(BIN_DIR) 2>$(DEV_NULL); [ -d $(BIN_DIR) ]; }
   [ -d $(PDB_DIR) ] || \
      { mkdir -p $(PDB_DIR) 2>$(DEV_NULL); [ -d $(PDB_DIR) ]; }
   [ -d $(LIB_DIR) ] || \
      { mkdir -p $(LIB_DIR) 2>$(DEV_NULL); [ -d $(LIB_DIR) ]; }
   $(RM) $@ $@.manifest

   $(MK_SHRPP_SHOW)

   "$(VDLLPP)" $(LDFLAGS) \
      -out:$@ \
      -pdb:$(PDB_DIR)/$(DLLNAME).pdb \
      -implib:$(LIB_DIR)/$(DLLNAME).$(LIB_EXT) \
      $(DEF_OPTS) \
      $(DLL_OBJS) \
      $(LIBSPP) $(OS_LIBSPP)

   $(MANIFEST_SHR)
endef

# ------------------------------------------------------------------------
# create a C executable
#
ifdef VERBOSE_MAKE
   define MK_EXE_SHOW
      echo '"$(VLD)"' $(EXEFLAGS) \
         -out:$@ \
         -pdb:$(INT_DIR)/$(basename $(notdir $@)).pdb \
         $(filter %.$(OBJ_EXT), $^) \
         $(LIBS) $(OS_LIBS) | fold -s
   endef
else
   define MK_EXE_SHOW
      echo $(strip MK_EXE_$(LIB_TYPE_NAME) $(LDOPTS) $(OPTS) $@)
   endef
endif

define MK_EXE
   [ -d $(BIN_DIR) ] || \
      { mkdir -p $(BIN_DIR) 2>$(DEV_NULL); [ -d $(BIN_DIR) ]; }
   $(RM) $@ $@.manifest

   $(MK_EXE_SHOW)

   "$(VLD)" $(EXEFLAGS) \
      -out:$@ \
      -pdb:$(INT_DIR)/$(basename $(notdir $@)).pdb \
      $(filter %.$(OBJ_EXT), $^) \
      $(LIBS) $(OS_LIBS)

   $(MANIFEST_EXE)
endef

# ------------------------------------------------------------------------
# create a C++ executable
#
ifdef VERBOSE_MAKE
   define MK_EXEPP_SHOW
      echo '"$(VLDPP)"' $(EXEFLAGS) \
         -out:$@ \
         -pdb:$(INT_DIR)/$(basename $(notdir $@)).pdb \
         $(filter %.$(OBJ_EXT), $^) \
         $(LIBSPP) $(OS_LIBSPP) | fold -s
   endef
else
   define MK_EXEPP_SHOW
      echo $(strip MK_EXEPP_$(LIB_TYPE_NAME) $(LDOPTS) $(OPTS) $@)
   endef
endif

define MK_EXEPP
   [ -d $(BIN_DIR) ] || \
      { mkdir -p $(BIN_DIR) 2>$(DEV_NULL); [ -d $(BIN_DIR) ]; }
   $(RM) $@ $@.manifest

   $(MK_EXEPP_SHOW)

   "$(VLDPP)" $(EXEFLAGS) \
      -out:$@ \
      -pdb:$(INT_DIR)/$(basename $(notdir $@)).pdb \
      $(filter %.$(OBJ_EXT), $^) \
      $(LIBSPP) $(OS_LIBSPP)

   $(MANIFEST_EXE)
endef

# ------------------------------------------------------------------------
# Rules to build C/C++ object files in an intermediate directory
#
$(INT_DIR)/%.$(OBJ_EXT) : %.c
	@ [ -d $(INT_DIR) ] || \
	    { mkdir $(INT_DIR) 2>$(DEV_NULL); [ -d $(INT_DIR) ]; }

ifndef VERBOSE_MAKE
	@ echo $(strip $(VCC_NAME) -c $(COPTS) $(OPTS) $<)
	@ "$(VCC)" -c $(CFLAGS) $(ICDEFS) $<
else
	  "$(VCC)" -c $(CFLAGS) $(ICDEFS) $<
endif

$(INT_DIR)/%.$(OBJ_EXT) : %.cpp
	@ [ -d $(INT_DIR) ] || \
	    { mkdir $(INT_DIR) 2>$(DEV_NULL); [ -d $(INT_DIR) ]; }

ifndef VERBOSE_MAKE
	@ echo $(strip $(VCCPP_NAME) -c $(COPTS) $(OPTS) $<)
	@ "$(VCCPP)" -c $(CPPFLAGS) $(ICDEFS) $<
else
	  "$(VCCPP)" -c $(CPPFLAGS) $(ICDEFS) $<
endif

# ------------------------------------------------------------------------
# Rules to build resource files
#
$(INT_DIR)/%.res : %.rc
	@ [ -d $(INT_DIR) ] || \
	    { mkdir $(INT_DIR) 2>$(DEV_NULL); [ -d $(INT_DIR) ]; }

ifndef VERBOSE_MAKE
	@ echo $(strip rc $<)
	@ "$(VRC)" $(RCFLAGS) -fo$@ $<
else
	  "$(VRC)" $(RCFLAGS) -fo$@ $<
endif

# ------------------------------------------------------------------------
# Build the entire solution or specified projects using VS project files
#
# Usage:
#
#   make devenv [ACTION=build|rebuild|clean] \
#               [PROJECT=xxx,...] \
#               [BUILD_MODE=debug|release] \
#               [OS_ARCH=32|64]
#               [SOLUTION=xxx]
#
#   default ACTION  = "build"
#   default PROJECT = "all"
#
DEVENV := $(VSInstallDir)/Common7/IDE/devenv

.PHONY : devenv
devenv :
	@ if [ "$$SOLUTION" = "" ]; \
	  then \
	    SOLUTION=`ls *.sln 2>nul`; \
	  else \
	    case "$$SOLUTION" in \
	      *.sln) ;; \
	      *)     SOLUTION="$$SOLUTION".sln ;; \
	    esac; \
	  fi; \
	  [ "$$SOLUTION" != "" ] && \
	  { \
	    if [ `echo $$SOLUTION | wc -w` -gt 1 ]; \
	    then \
	      echo "More than one solution: $$SOLUTION" >&2; \
	      exit 1; \
	    fi; \
	  }; \
	  [ "$$SOLUTION" != "" ] && \
	  { \
	    ACTION=$${ACTION:-build}; \
      case "$(OS_ARCH)" in \
	      32)  PLATFORM="Win32";; \
	      64)  PLATFORM="x64";; \
      esac; \
	    if [ "$${PROJECT:-all}" = "all" ]; \
	    then \
	      "$(DEVENV)" -nologo $$SOLUTION -$$ACTION "$(BUILD_MODE)|$$PLATFORM"; \
	    else \
	      for p in `echo $$PROJECT | sed -e 's/,/ /g'`; \
	      do \
	        "$(DEVENV)" -nologo $$SOLUTION -$$ACTION "$(BUILD_MODE)|$$PLATFORM"  \
	          -project $$p; \
	      done; \
	    fi 2>&1; \
	  }
