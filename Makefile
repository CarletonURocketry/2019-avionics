#----------------------------------------------------------------------------
# SAMD21 Makefile
# By Samuel Dewan.
#
# Adapted from the WinAVR Makefile Template written by Eric B. Weddington, Jörg Wunsch, et al.
#
# Released to the Public Domain
#
#----------------------------------------------------------------------------
# On command line:
#
# make build = Make software.
#
# make clean = Clean out built project files.
#
# make program = Download the elf file to the device, using openocd and GDB.
#
# make debug = Start OpenOCD with a GDB debuging frontend
#
# To rebuild project do "make clean" then "make build".
#----------------------------------------------------------------------------

# MCU name
PTYPE = __SAMD21J18A__
MCU_NAME = samd21

# Processor frequency.
#     This will define a symbol, F_CPU, in all source code files equal to the 
#     processor frequency. You can then use this symbol in your source code to 
#     calculate timings. Do NOT tack on a 'UL' at the end, this will be done
#     automatically to create a 32-bit value in your source code.
F_CPU = 48000000

OPENOCD_CONFIG = openocd.cfg

# Target file name (without extension).
TARGET = main

SRCDIR = src
# List C source files here. (C dependencies are automatically generated.)
SRC = $(wildcard $(SRCDIR)/*.c) $(SRCDIR)/$(MCU_NAME)/startup_$(MCU_NAME).c

OBJDIR = obj
# List Assembler source files here.
#     Make sure they always end in a capital .S.  Files ending in a lowercase .s
#     will not be considered source files but generated files (assembler
#     output from the compiler), and will be deleted upon "make clean"!
#     Even though the DOS/Win* filesystem matches both .s and .S the same,
#     it will preserve the spelling of the filenames, and gcc itself does
#     care about how the name is spelled on its command-line.
ASRC = $(wildcard $(SRCDIR)/*.S)

# Optimization level, can be [0, 1, 2, 3, s]. 
#     0 = turn off optimization. s = optimize for size.
#     (Note: 3 is not always the best optimization level.)
OPT = 2


# List any extra directories to look for include files here.
#     Each directory must be seperated by a space.
#     Use forward slashes for directory separators.
#     For a directory that has spaces, enclose it in quotes.
EXTRAINCDIRS = $(SRCDIR) $(SRCDIR)/$(MCU_NAME)/include $(SRCDIR)/$(MCU_NAME)/source

# Compiler flag to set the C Standard level.
CSTANDARD = -std=gnu11


# Place -D or -U options here
CDEFS = -DF_CPU=$(F_CPU)UL


# Place -I options here
CINCS = 


#---------------- Architecture Options ----------------
ARCHFLAGS += -mcpu=cortex-m0plus -mthumb 


#---------------- Compiler Options ----------------
#  -g*: 			generate debugging information
#  -O*:          optimization level
#  -f...:        	tuning, see GCC manual
#  -Wall...:    warning level
#  -Wa,...:     tell GCC to pass this to the assembler.
#    -adhlns...: create assembler listing
CFLAGS += $(CDEFS) $(CINCS)
CFLAGS += -O$(OPT) -g3
CFLAGS += $(ARCHFLAGS)
CFLAGS += -funsigned-char -funsigned-bitfields -fno-strict-aliasing
CFLAGS += -ffunction-sections -fdata-sections -mlong-calls
CFLAGS += --param max-inline-insns-single=500
CFLAGS += -gstrict-dwarf

# Enable many usefull warnings
# (see https://gcc.gnu.org/onlinedocs/gcc-6.3.0/gcc/Warning-Options.html)
CFLAGS += -Wall -Wextra -Wshadow -Wundef -Wformat=2 -Wtrampolines -Wfloat-equal
CFLAGS += -Wbad-function-cast -Waggregate-return -Wstrict-prototypes -Wpacked
CFLAGS += -Wno-aggressive-loop-optimizations -Wmissing-prototypes -Winit-self
CFLAGS += -Wmissing-declarations -Wmissing-format-attribute -Wunreachable-code
CFLAGS += -Wshift-overflow=2 -Wduplicated-cond -Wpointer-arith -Wwrite-strings
CFLAGS += -Wnested-externs -Wcast-align -Wredundant-decls -Wlong-long
CFLAGS += -Werror=implicit-function-declaration -Wlogical-not-parentheses
CFLAGS += -Wlogical-op -Wold-style-definition -Wcast-qual -Wdouble-promotion
CFLAGS += -Wunsuffixed-float-constants -Wmissing-include-dirs -Wnormalized
CLFAGS += -Wdisabled-optimization -Wsuggest-attribute=const

# These warning may be usefull in some cases, but cause too many false positives
# to be enabled all of the time: -Winline -Wpadded -Wvla -Wpedantic -Wconversion
# -Wnull-dereference -Wsuggest-attribute=noreturn -Wsuggest-attribute=pure
# -Wstack-usage=256

# Disable some anoying warnings
CFLAGS += -Wno-unused-parameter

CFLAGS += -Wa,-adhlns=$(patsubst $(SRCDIR)/%.cpp,$(OBJDIR)/%.lst,$(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.lst,$<))
CFLAGS += $(patsubst %,-I%,$(EXTRAINCDIRS))
CFLAGS += $(CSTANDARD)


#---------------- Assembler Options ----------------
#  -Wa,...:   tell GCC to pass this to the assembler.
#  -ahlms:    create listing
#  -gstabs:   have the assembler create line number information; note that
#             for use in COFF files, additional information about filenames
#             and function names needs to be present in the assembler source
#             files -- see avr-libc docs [FIXME: not yet described there]
#  -listing-cont-lines: Sets the maximum number of continuation lines of hex 
#       dump that will be displayed for a given single line of source input.
ASFLAGS = -Wa,-adhlns=$(patsubst $(SRCDIR)/%.S,$(OBJDIR)/%.lst,$<),-gstabs,--listing-cont-lines=100


#---------------- Library Options ----------------
MATH_LIB = -Lsamd21/lib/libarm_cortexM0l_math.a -lm


#---------------- Linker Options ----------------
#  -Wl,...:     tell GCC to pass this to linker.
#    -Map:      create map file
#    --cref:    add cross reference to  map file
LDSCRIPT = src/samd21/samd21j18a_flash.ld
SPECS = src/samd21/nosys.specs

LDFLAGS += -T$(LDSCRIPT) $(ARCHFLAGS) -Wl,--gc-sections --entry=Reset_Handler
LDFLAGS += --specs=$(SPECS) $(MATH_LIB)


#---------------- Debugging Options ----------------

# Script used to launch OpenOCD and GDB
DEBUG_CMD = PYTHONDONTWRITEBYTECODE=1 ./debug.py

# Debugging port used to communicate between GDB / openocd.
OPENOCD_PORT = 4444
GDB_PORT = 2331

# Debugging host used to communicate between GDB / openocd, normally
#     just set to localhost unless doing some sort of crazy debugging when 
#     openocd is running on a different computer.
DEBUG_HOST = localhost

DEBUG_ARGS = --host $(DEBUG_HOST) --gdb-port $(GDB_PORT)
DEBUG_ARGS += --openocd-port $(OPENOCD_PORT)

# Use user provided OpenOCD if specified
ifdef OPENOCD_PATH
DEBUG_ARGS += --openocd $(OPENOCD_PATH)
endif

# Use GDB from toolchain if one is specified
ifdef CORTEX_TOOLCHAIN_BIN
DEBUG_ARGS += --gdb $(CORTEX_TOOLCHAIN_BIN)/arm-none-eabi-gdb
endif

# Select OpenOCD Interface
ifdef OPENOCD_INTERFACE
DEBUG_ARGS += --interface $(OPENOCD_INTERFACE)
endif

# Use a device file as the GDB server if one is proved, useful for Black Magic
# Probe. Note that the debug script will not attach to the target automatically,
# the user needs to perform the scan and attach manually after GDB opens.
ifdef GDB_FILE
DEBUG_ARGS += --gdb-file $(GDB_FILE)
endif


#============================================================================


# Define programs and commands.

# CORTEX_TOOLCHAIN_BIN must be specified as an environment variable or argument
# to make

SHELL = sh
ifdef CORTEX_TOOLCHAIN_BIN
# If a toolchain path is provided, use it
CC = $(CORTEX_TOOLCHAIN_BIN)/arm-none-eabi-gcc
LD = $(CORTEX_TOOLCHAIN_BIN)/arm-none-eabi-gcc
AS = $(CORTEX_TOOLCHAIN_BIN)/arm-none-eabi-as
OBJCOPY = $(CORTEX_TOOLCHAIN_BIN)/arm-none-eabi-objcopy
OBJDUMP = $(CORTEX_TOOLCHAIN_BIN)/arm-none-eabi-objdump
SIZE = $(CORTEX_TOOLCHAIN_BIN)/arm-none-eabi-size
NM = $(CORTEX_TOOLCHAIN_BIN)/arm-none-eabi-nm
else
# If no toolchain path was provided, try to find a toolchain on the PATH
CC = arm-none-eabi-gcc
LD = arm-none-eabi-gcc
AS = arm-none-eabi-as
OBJCOPY = arm-none-eabi-objcopy
OBJDUMP = arm-none-eabi-objdump
SIZE = arm-none-eabi-size
NM = arm-none-eabi-nm
endif
REMOVE = rm -f
COPY = cp

# Define Messages
# English
MSG_ERRORS_NONE = Errors: none
MSG_LINKING = Linking:
MSG_COMPILING = Compiling:
MSG_ASSEMBLING = Assembling:
MSG_CLEANING = Cleaning project:
MSG_PROGRAMMING = Uploading to Target:
MSG_RESET = Resetting Target:
MSG_DEBUGGING = Starting Debugger:


# Define all object files.
OBJ = $(patsubst $(SRCDIR)/%.S,$(OBJDIR)/%.o,$(patsubst $(SRCDIR)/%.cpp,$(OBJDIR)/%.o,$(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(SRC))))

# Define all dependancy files.
DEP = $(OBJ:%.o=%.d)

# Define all list files.
LST = $(OBJ:%.o=%.lst)

# Combine all necessary flags and optional flags.
# Add target processor to flags.
ALL_CFLAGS = -D$(PTYPE) -I. $(CFLAGS)
ALL_ASFLAGS = -D$(PTYPE) -I. -x assembler-with-cpp $(ASFLAGS)

# Define compilers
COMPILE.c = $(CC) $(ALL_CFLAGS) -c
COMPILE.cpp = $(CC) $(ALL_CFLAGS) -c



# Default target.
build: $(OBJDIR) elf

all: gccversion clean build program

elf: $(OBJDIR)/$(TARGET).elf

$(OBJDIR):
	@mkdir -p $@

# Display compiler version information.
gccversion : 
	@$(CC) --version

# Display information about build.
info:
	@echo CFLAGS=$(CFLAGS)
	@echo SRC=$(SRC)
	@echo OBJ=$(OBJ)

# Program the device.  
program: $(OBJDIR)/$(TARGET).elf
	@echo
	@echo $(MSG_PROGRAMMING)
	$(DEBUG_CMD) $(DEBUG_ARGS) --upload --reset --no-debug $^

# Upload binary to target.
upload: $(OBJDIR)/$(TARGET).elf
	@echo
	@echo $(MSG_PROGRAMMING)
	$(DEBUG_CMD) $(DEBUG_ARGS) --upload --no-debug $^

# Reset the device.
reset:
	@echo
	@echo $(MSG_RESET)
	$(DEBUG_CMD) $(DEBUG_ARGS) --reset --no-debug
	
# Reset the device and leave halted.
halt:
	@echo
	@echo $(MSG_RESET)
	$(DEBUG_CMD) $(DEBUG_ARGS) --reset halt --no-debug

# Launch a debugging session.
debug: $(OBJDIR)/$(TARGET).elf
	$(DEBUG_CMD) $(DEBUG_ARGS) $^

# Link: create ELF output file from object files.
.SECONDARY : $(OBJDIR)/$(TARGET).elf
.PRECIOUS : $(OBJ)
$(OBJDIR)/%.elf: $(OBJ)
	$(shell mkdir -p $(@D) >/dev/null)
	@echo
	@echo $(MSG_LINKING) $@
	$(LD) $^ --output $@ $(LDFLAGS)

# Compile: create object files from C source files.
$(OBJDIR)/%.o : $(SRCDIR)/%.c
	$(shell mkdir -p $(@D) >/dev/null)
	@echo
	@echo $(MSG_COMPILING) $<
	$(COMPILE.c) "$(abspath $<)" -o $@

# Assemble: create object files from assembler source files.
$(OBJDIR)/%.o:    $(SRCDIR)/%.s
	$(AS) $< -o $@

# Compile: create object files from C++ source files.
$(OBJDIR)/%.o : $(SRCDIR)/%.cpp
	$(shell mkdir -p $(@D) >/dev/null)
	@echo
	@echo $(MSG_COMPILING) $<
	$(COMPILE.cpp) "$(abspath $<)" -o $@

$(OBJDIR)/.depend:  $(SRC) $(OBJDIR)
	$(COMPILE.c) -MM $(SRC)  | \
	sed -E 's#^(.*\.o: *)$(SRCDIR)/(.*/)?(.*\.(c|cpp|S))#$(OBJDIR)/\2\1$(SRCDIR)/\2\3#' > $@

include $(OBJDIR)/.depend

# Target: clean project.
clean:
	@echo $(MSG_CLEANING)
	$(REMOVE) -rf $(OBJDIR)/*

# Listing of phony targets.
.PHONY : all gccversion build elf clean clean_list program debug upload reset
