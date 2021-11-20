# example library build makefile - for Watcom C\C++
# wbcbz7 l5.ll.zozl

# target name
TARGET   = example

# build platform and system
BUILD    = dos
SYSTEM   = pmode

# debug level (0 - no debug, 2 - defualt debug build and so on, check wpp386 help for more info)
DLEVEL   = 0

# include paths
INCLUDE  = ../include

# compiler/linker paths
AS       = nasm.exe
CC       = wpp386.exe
LD       = wlink.exe

# compiler flags
CFLAGS   = -5r -fp5 -fpi87 -zp16 -oneatx -ei -s -d$(DLEVEL) -i=$(INCLUDE) -bt=$(BUILD)
LFLAGS   = -f obj -l $<.lst

# object files
OBJS     = dpmi.obj rtctimer.obj mycpuid.obj tgaload.obj bmpload.obj vga.obj
		   
OBJSTR   = file {$(OBJS)}


# aaand finally - build rules!
all: $(OBJS) $(TARGET).lib

$(TARGET).lib : $(OBJS) .symbolic
	%create $(TARGET).ls
	for %i in ($(OBJS)) do @%append $(TARGET).ls +%i
	
	wlib -n $(TARGET).lib
	wlib    $(TARGET).lib @$(TARGET).ls
	del     $(TARGET).ls

.cpp.obj:
	$(CC) $< $(CFLAGS)

# note: enable DJGPP DPMI functions implementation
dpmi.obj : dpmi.cpp
	$(CC) $< $(CFLAGS) -D_DPMI_DJGPP_COMPATIBILITY
	
# note: -zu switch makes wpp386 assume SS != DGROUP (critical for ISRs)
rtctimer.obj : rtctimer.cpp
	$(CC) $< $(CFLAGS) -zu
	
.asm.obj:
	$(AS) $< $(CFLAGS)
	
# clean all
clean: .symbolic
	del *.$(O)
	del *.lib
	del *.err
	del *.lst