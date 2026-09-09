SOURCE_DIR 	= src
BUILD_DIR	= build
C_SOURCES	= $(wildcard $(SOURCE_DIR)/*.c)
OBJECTS		= $(patsubst %.c, $(BUILD_DIR)/%.o, $(C_SOURCES))
BUILD_DIRS	= $(BUILD_DIR) $(BUILD_DIR)/src


##############################
# Configurable flags and names
##############################
CFLAGS	= -fno-math-errno -Werror -Wno-error=missing-braces -Wno-error=strict-aliasing
LDFLAGS	= -g
ENAME	= murin-1.0.0
TARGET	:= $(ENAME)

# Enables dependency tracking (https://make.mad-scientist.net/papers/advanced-auto-dependency-generation/)
TRACK_DEPENDENCIES=1
# link usin g C compiler
LINK = $(CC)
# Optimisation level in release builds
OPT_LEVEL=1

##########################################################################################
# Determine shell command used to remove files (for "make clean") casue windwos is a bitch
##########################################################################################

ifndef RM
	ifeq ($(OS), Windows_NT)
		RM = del
	else
		RM = rm -rf
	endif
endif

###########################################################
# If target platform isn't specified, default to current OS
###########################################################
ifndef $(PLAT)
	ifeq ($(OS),Windows_NT)
		PLAT = mingw
	else
		PLAT = $(shell uname -s | tr '[:upper:]' '[:lower:]')
	endif
endif

ifeq ($(PLAT),mingw)
	CC     =  gcc
	OEXT    =  .exe
	CFLAGS  += -DUNICODE
	LDFLAGS =  -g
	LIBS    =  -mwindows -lwinmm
	BUILD_DIR = build/win
endif

ifeq ($(PLAT),linux)
	# -lm may be needed for __builtin_sqrtf (in cases where it isn't replaced by a CPU instruction intrinsic)
	LIBS    = 
	BUILD_DIR = build/linux

	# Detect MCST LCC, where -O3 is about equivalent to -O1
	ifeq ($(shell $(CC) -dM -E -xc - < /dev/null | grep -o __MCST__),__MCST__)
		OPT_LEVEL=3
	endif
endif

ifdef RELEASE
	CFLAGS += -O$(OPT_LEVEL)
else
	CFLAGS += -g
endif

default: $(PLAT)

linux:
	$(MAKE) $(TARGET) PLAT=linux
mingw:
	$(MAKE) $(TARGET) PLAT=mingw
release:
	$(MAKE) $(TARGET) RELEASE=1

# Cleans up all build .o files
clean:
	$(RM) $(OBJECTS) $(ENAME)


#################################################
# Source files and executable compilation section
#################################################
$(BUILD_DIRS): 
	mkdir -p $@

$(ENAME): $(BUILD_DIRS) $(OBJECTS)
	$(LINK) $(LDFLAGS) -o $@$(OEXT) $(OBJECTS) $(LIBS)
	@echo "----------------------------------------------------"
	@echo "compiled executable file: $(ENAME)"
	@echo "----------------------------------------------------"

ifeq ($(TRACK_DEPENDENCIES), 1)

DEPFLAGS = -MT $@ -MMD -MP -MF $(BUILD_DIR)/$*.d
DEPFILES := $(patsubst %.o, %.d, $(OBJECTS))
$(DEPFILES):

$(BUILD_DIR)/%.o : %.c $(BUILD_DIR)/%.d
	$(CC) $(CFLAGS) $(DEPFLAGS) -c $< -o $@
$(BUILD_DIR)/%.o : %.cpp $(BUILD_DIR)/%.d
	$(CC) $(CFLAGS) $(DEPFLAGS) -c $< -o $@
$(BUILD_DIR)/%.o : %.m $(BUILD_DIR)/%.d
	$(CC) $(CFLAGS)  $(DEPFLAGS) -c $< -o $@

include $(wildcard $(DEPFILES))
# === Compiling WITHOUT dependency tracking ===
else

$(BUILD_DIR)/%.o : %.c
	$(CC) $(CFLAGS) -c $< -o $@
$(BUILD_DIR)/%.o : %.cpp
	$(CC) $(CFLAGS) -c $< -o $@
endif