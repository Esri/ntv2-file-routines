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
# Unix-specific setup (included by make.i)                                  #
# ------------------------------------------------------------------------- #

# ------------------------------------------------------------------------
# common compile/link options
#
LIB_PREFIX   := lib

ifndef DEBUG_OPTS
  DEBUG_OPTS := -g
endif

ifndef OPTIMIZE_OPTS
  OPTIMIZE_OPTS := -O
endif

ifeq ($(BUILD_MODE), debug)
   CDEFS     += $(DEBUG_OPTS) -D_DEBUG
   CPPDEFS   += $(DEBUG_OPTS) -D_DEBUG
   EXEDEFS   += $(DEBUG_OPTS)
   DLLDEFS   += $(DEBUG_OPTS)
else
   CDEFS     += $(OPTIMIZE_OPTS) -DNDEBUG
   CPPDEFS   += $(OPTIMIZE_OPTS) -DNDEBUG
endif

VCC_NAME   := $(notdir $(VCC))
VCCPP_NAME := $(notdir $(VCCPP))

# ------------------------------------------------------------------------
# create a static library
#
ifdef VERBOSE_MAKE
   define MK_LIB_SHOW
      echo "$(VLIB)" $(LIBDEFS) $@ $(LIB_OBJS) | fold -s
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

   "$(VLIB)" $(LIBDEFS) $@ $(LIB_OBJS)
endef

# ------------------------------------------------------------------------
# create a dynamic (shared) C library from object files
#
ifdef VERBOSE_MAKE
   define MK_SHR_SHOW
      echo "$(VDLL)" $(LDFLAGS) \
         -o $@ \
         $(DLL_OBJS) \
         $(LIBS) $(OS_LIBS) | fold -s
   endef
else
   define MK_SHR_SHOW
      echo $(strip MK_SHR $(LDOPTS) $(OPTS) $@)
   endef
endif

define MK_SHR
   [ -d $(LIB_DIR) ] || \
      { mkdir -p $(LIB_DIR) 2>$(DEV_NULL); [ -d $(LIB_DIR) ]; }
   $(RM) $@

   $(MK_SHR_SHOW)

   "$(VDLL)" $(LDFLAGS) \
      -o $@ \
      $(DLL_OBJS) \
      $(LIBS) $(OS_LIBS)
endef

# ------------------------------------------------------------------------
# create a dynamic (shared) C++ library from object files
#
ifdef VERBOSE_MAKE
   define MK_SHRPP_SHOW
      echo "$(VDLLPP)" $(LDFLAGS) \
         -o $@ \
         $(DLL_OBJS) \
         $(LIBSPP) $(OS_LIBSPP) | fold -s
   endef
else
   define MK_SHRPP_SHOW
      echo $(strip MK_SHRPP $(LDOPTS) $(OPTS) $@)
   endef
endif

define MK_SHRPP
   [ -d $(LIB_DIR) ] || \
      { mkdir -p $(LIB_DIR) 2>$(DEV_NULL); [ -d $(LIB_DIR) ]; }
   $(RM) $@

   $(MK_SHRPP_SHOW)

   "$(VDLLPP)" $(LDFLAGS) \
      -o $@ \
      $(DLL_OBJS) \
      $(LIBSPP) $(OS_LIBSPP)
endef

# ------------------------------------------------------------------------
# create a C executable
#
ifdef VERBOSE_MAKE
   define MK_EXE_SHOW
      echo "$(VLD)" $(EXEFLAGS) \
         -o $@ \
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
   $(RM) $@

   $(MK_EXE_SHOW)

   "$(VLD)" $(EXEFLAGS) \
      -o $@ \
      $(filter %.$(OBJ_EXT), $^) \
      $(LIBS) $(OS_LIBS)
endef

# ------------------------------------------------------------------------
# create a C++ executable
#
ifdef VERBOSE_MAKE
   define MK_EXEPP_SHOW
      echo "$(VLDPP) $(EXEFLAGS) \
         -o $@ \
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
   $(RM) $@

   $(MK_EXEPP_SHOW)

   "$(VLDPP)" $(EXEFLAGS) \
      -o $@ \
      $(filter %.$(OBJ_EXT), $^) \
      $(LIBSPP) $(OS_LIBSPP)
endef

# ------------------------------------------------------------------------
# Rules to build C/C++ object files in an intermediate directory
#
$(INT_DIR)/%.$(OBJ_EXT) : %.c
	@ [ -d $(INT_DIR) ] || \
	    { mkdir $(INT_DIR) 2>$(DEV_NULL); [ -d $(INT_DIR) ]; }

ifndef VERBOSE_MAKE
	@ echo $(strip $(VCC_NAME) -c $(COPTS) $(OPTS) $<)
	@ "$(VCC)" -c $(CFLAGS) -o $(INT_DIR)/$*.$(OBJ_EXT) $<
else
	  "$(VCC)" -c $(CFLAGS) -o $(INT_DIR)/$*.$(OBJ_EXT) $<
endif

$(INT_DIR)/%.$(OBJ_EXT) : %.cpp
	@ [ -d $(INT_DIR) ] || \
	    { mkdir $(INT_DIR) 2>$(DEV_NULL); [ -d $(INT_DIR) ]; }

ifndef VERBOSE_MAKE
	@ echo $(strip $(VCCPP_NAME) -c $(COPTS) $(OPTS) $<)
	@ "$(VCCPP)" -c $(CPPFLAGS) -o $(INT_DIR)/$*.$(OBJ_EXT) $<
else
	  "$(VCCPP)" -c $(CPPFLAGS) -o $(INT_DIR)/$*.$(OBJ_EXT) $<
endif
