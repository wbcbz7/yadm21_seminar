# example exexutable build makefile - for Watcom C\C++
# wbcbz7 l5.ll.zozl

# target name
TARGET   = vbetest

# build platform and system
BUILD    = dos
SYSTEM   = pmode

# debug symbols property (we'll leave all debug symbols here)
DEBUG    = all

# debug level (0 - no debug, 2 - default debug build and so on, check wpp386 help for more info)
DLEVEL   = 0

# include paths
INCLUDE  = ..\include\

# compiler/linker paths
AS       = nasm.exe
CC       = wcc386.exe
CPPC     = wpp386.exe
LD       = wlink.exe

# compiler flags (note -xst enables exception handling for C++, you can safely omit it if you're not using Watcom's STL)
CFLAGS   = -5r -fp5 -fpi87 -zp16 -oneatx -ei -s -d$(DLEVEL) -i=$(INCLUDE) -bt=$(BUILD)
AFLAGS   = -f obj -l $<.lst

# object files
OBJS     = $(TARGET).obj

# library files  
LIBS     = ../lib/example.lib

# ----------------------------------------------
# aaand finally - build rules!
all: $(TARGET).exe

$(TARGET).exe: $(OBJS) .symbolic
	%write $(TARGET).lnk debug 	$(DEBUG)
	%write $(TARGET).lnk name  	$(TARGET)
	%write $(TARGET).lnk file    {$(OBJS)}
	%write $(TARGET).lnk library {$(LIBS)}
	%write $(TARGET).lnk system  $(SYSTEM)
	$(LD) @$(TARGET).lnk $(LFLAGS)
	del $(TARGET).lnk

.c.obj:
	$(CC) $< $(CFLAGS)
	
.cpp.obj:
	$(CPPC) $< $(CFLAGS)
	
.asm.obj:
	$(AS) $< $(AFLAGS)
	
# clean all
clean: .symbolic
	del *.obj
	del *.lib
	del *.err
	del *.lst