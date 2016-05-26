#********************************* MAKEFILE ***********************************
#**                            *******************                           **
#**                                                                          **
#** Project : FreePP Box (common lib)                                        **
#** Filename : Makefile                                                      **
#** Version : 1.0                                                            **
#** Date : OCT 31, 2007                                                      **
#**                                                                          **
#******************************************************************************
#**                                                                          **
#** Copyright (c) 2007 Browan Communications Inc.                            **
#** All rights reserved.                                                     **
#**                                                                          **
#******************************************************************************
#Version History:
#----------------
#Version : 1.0
#Date : OCT 31, 2007
#Revised By : jone.li
#Description : This is the makefile template .the HEAD
#Version : 1.1
#Date : NOV 05, 2007
#Revised By : jone.li
#Description : 1.We can remove the files which we dont want to compile in macro IGNORE_SOURCE
#              the subdir should define the same macro too.
#              2 Remove the LDFLAGS flag when we complile the files 
#


#!!!!!!!!!!!!!!!!! change 1
#CROSS_COMPILE=arm-linux-uclibc-
#------------------------------
HEADDIR := $(shell pwd)

CC := $(CROSS_COMPILE)gcc
CXX:= $(CROSS_COMPILE)g++
STRIP := $(CROSS_COMPILE)strip
AR :=$(CROSS_COMPILE)ar
CFLAGS :=-g -I.
CXXFLAGS := $(CFLAGS) 
LDFLAGS  := -L.
LIBS := 
STATICLIBS :=
#add the makefile into the dependencies
DEPS_FILTER := sed  '$$s,$$, Makefile,g'
#
#here you can define which files dont be compiled.(advanced)
#
IGNORE_SOURCE :=


#!!!!!!!!!!!!!!!!! change 2
FORMAT   := c cpp cxx
#!!!!!!!!!!!!!!!!! change 3
SUBDIRS  := 
#!!!!!!!!!!!!!!!!! change 4
TARGET   := example

#
#export the vars for the makefile in the sub dir
#
export CROSS_COMPILE CFLAGS CXXFLAGS LDFLAGS

#
#transfor the type to %.type
#
FORMAT_MODE = $(foreach format,$(FORMAT),%.$(format) )

#LOCAL_SOURCE := $(wildcard *.c) $(wildcard *.cpp) # method 1
LOCAL_SOURCE :=  $(filter-out $(IGNORE_SOURCE),$(foreach format,$(FORMAT),$(wildcard *.$(format)))) #method 2

#
#get the all source file in deferent dir
#
SOURCE_FILES := $(foreach dir, $(SUBDIRS),$(foreach format,$(FORMAT),$(wildcard $(dir)/*.$(format))) )
SOURCE_FILES += $(LOCAL_SOURCE)
SOURCE_FILES := $(filter-out $(IGNORE_SOURCE), $(SOURCE_FILES) )
#
#transfor the source file to obj file
#
#OBJ_FILES := $(patsubst %.c,%.o,$(SOURCE_FILES) ) #replace the C file
#OBJ_FILES := $(patsubst %.cpp,%.o,$(OBJ_FILES) )  #replace the CPP file  # method 1
OBJ_FILES := $(foreach format,$(FORMAT_MODE),$(filter %.o,$(patsubst $(format),%.o,$(SOURCE_FILES))) ) #method 2

#
#transfor the obj files fo dependencies files
#
LOCAL_DEPS := $(foreach format,$(FORMAT_MODE),$(filter %.d,$(patsubst $(format),%.d,$(LOCAL_SOURCE))) )
#
#*************we define  how to create the dependence file********************************************
#
%.d : %.c
	$(CC) $(CFLAGS)  -M  $< > $@.tmp
	$(DEPS_FILTER)  < $@.tmp >$@
	rm $@.tmp
%.d : %.cpp
	$(CXX) $(CFLAGS) -M  $< > $@.tmp
	$(DEPS_FILTER)  < $@.tmp >$@
	rm $@.tmp
%.d : %.cxx
	$(CXX) $(CFLAGS) -M  $< > $@.tmp
	$(DEPS_FILTER)  < $@.tmp >$@
	rm $@.tmp

#
#*************we define  how to complile the different format's source code***************************
#
%.o : %.cxx
	$(CXX) $(CFLAGS) -o $@ -c $<
#default type
%.o : %.c   
	$(CC) $(CFLAGS) -o $@ -c $<
#default type
%.o : %.cpp
	$(CXX) $(CFLAGS) -o $@ -c $<
#
#!!!!!!!!!!!!!!!!!!!!!!!!! you should define the compile rule  if you have the other source format !!! change 5
#
#.o : %.??
#*****************************************************************************************************
all: sub target 
include $(LOCAL_DEPS)
sub:
	for x in $(SUBDIRS); do\
		(cd $$x && $(MAKE) ); \
	done

release: target sharelib staticlib

target:$(TARGET) 
sharelib:lib$(TARGET).so
staticlib:lib$(TARGET).a

$(TARGET):$(OBJ_FILES)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) $(STATICLIBS)  -static
	$(STRIP) $@

#
#next is  compile mothod for  a .so/.a library,if you want do it :)
#
lib$(TARGET).so: $(OBJ_FILES)
	$(CC) $(CFLAGS) $(LDFLAGS) -shared -o $@ -Wl,-soname,$@ -lc $^ $(STATICLIBS)
	$(STRIP) $@

#
#next is compile mothod for a .a library
#
lib$(TARGET).a:$(OBJ_FILES)
	$(AR) -r $@ $^
	@chmod 766 $@
	$(STRIP) $@
install:
	
clean:
	for x in $(SUBDIRS); do (cd $$x && $(MAKE) clean ); done
	rm -rf $(TARGET) *.o *.d *.a *.so 

