# Makefile for CMSC 23700 Common Code Library
# Autumn 2017
#
# COPYRIGHT (c) 2017 John Reppy (http://cs.uchicago.edu/~jhr)
# All rights reserved.

SHELL =		/bin/sh
OS =		$(shell uname -s)

# the location of doxygen depends on the system that you are running,
# so you may have to edit this path.
#
ifeq ($(OS),Darwin)
  # standard location for MacLab machines
  DOXYGEN =	/usr/local/bin/doxygen
else
  DOXYGEN =	/usr/bin/doxygen
endif

BUILD_DIR =	$(shell pwd)
COMMON_DIR =	..
CS237_LIB_DIR =	$(COMMON_DIR)/cs237-lib
JSON_LIB_DIR =	$(COMMON_DIR)/json
OBJ_LIB_DIR =	$(COMMON_DIR)/obj
TQT_LIB_DIR =	$(COMMON_DIR)/tqt

# prefix of target library
TARGET_PREFIX =	$(COMMON_DIR)/lib/libcs237

CPPFLAGS =	-I$(COMMON_DIR)/include -I$(COMMON_DIR)/obj -I/usr/local/include
LDFLAGS =	-L/usr/local/lib
CXXFLAGS =	-O2 -g -Wall -pedantic

ifeq ($(OS),Darwin)
  TARGET        = $(TARGET_PREFIX).a
  CPPFLAGS	+= -I/opt/local/include
  CXX		= clang++ -std=c++11
else
  TARGET        = $(TARGET_PREFIX).so
  CPPFLAGS      += -I$(COMMON_DIR)/include -I$(COMMON_DIR)/obj -I/usr/local/include
  CXX		= g++ -std=c++11
  CXXFLAGS      += -fPIC
endif

VPATH =		$(CS237_LIB_DIR):$(JSON_LIB_DIR):$(OBJ_LIB_DIR):$(TQT_LIB_DIR)

DIRSRCS =	$(wildcard $(CS237_LIB_DIR)/*.cxx) \
 		$(wildcard $(JSON_LIB_DIR)/*.cxx) \
 		$(wildcard $(OBJ_LIB_DIR)/*.cxx) \
 		$(wildcard $(TQT_LIB_DIR)/*.cxx)

# source files without directory
SRCS =		$(notdir $(DIRSRCS))

INCLUDES =	$(wildcard ../include/*.hxx)  $(wildcard ../obj/*.hxx)

OBJS =		$(SRCS:.cxx=.o)

ifeq ($(OS),Darwin)
$(TARGET):	$(OBJS) .depend
	libtool -static -o $(TARGET) $(OBJS)
else
$(TARGET):	$(OBJS) .depend
	$(CXX) -shared -o $(TARGET) $(OBJS)
endif

%.o : %.cxx
	$(CXX) -c $(CPPFLAGS) $(CXXFLAGS) $< -o $@

# include-file dependency information
#
.depend:	$(DIRSRCS) $(INCLUDES)
	- $(CXX) $(CPPFLAGS) -MM $(DIRSRCS) > .depend

ifneq ($(MAKECMDGOALS),clean)
sinclude .depend
endif

# cleanup by removing generated files
#
.PHONY:		clean
clean:
		rm -rf *.o $(TARGET_PREFIX).* .depend
