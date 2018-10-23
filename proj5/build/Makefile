# Makefile for CMSC 23700 Final Project (Projects 5 and 6)
# Autumn 2017
#

TARGET =	proj5

SHELL =		/bin/sh
OS =		$(shell uname -s)

# the location of doxygen depends on the system that you are running,
# but you may have to edit this path.
#
ifeq ($(OS),Darwin)
  # standard location for MacLab machines; installed from Mac Ports
  DOXYGEN =	/usr/local/bin/doxygen
else
  DOXYGEN =	/usr/bin/doxygen
endif

COMMON_DIR =	$(shell (cd ../../common; pwd))
BUILD_DIR =	$(shell pwd)
SHADERS_DIR =	$(BUILD_DIR)/../shaders/
DATA_DIR =	$(BUILD_DIR)/../data/

CPPFLAGS =	-I$(COMMON_DIR)/include -I/usr/local/include \
		-DSHADER_DIR=\"$(SHADERS_DIR)\"  -DDATA_DIR=\"$(DATA_DIR)\"
CXXFLAGS =	-g -Wall -pedantic
LDFLAGS =	-L$(COMMON_DIR)/lib -L/usr/local/lib
LIBS =		-lcs237 -lglfw -lpng

ifeq ($(OS),Darwin)
  CPPFLAGS	+= -I/opt/local/include
  LDFLAGS	+= -L/opt/local/lib
  LIBS		+= -framework OpenGL -framework Foundation
  CXX		= clang++ -std=c++11
else
  LDFLAGS	+= -L/usr/lib/x86_64-linux-gnu/mesa/ \
                   -Wl,-rpath=/usr/local/lib,-rpath=$(COMMON_DIR)/lib
  LIBS		+= -lGL -lm
  CXX		= g++ -std=c++11
endif

# where to find the source code
#
VPATH =		../src

SRCS =		$(wildcard ../src/*.cxx)
INCLUDES =	$(wildcard ../src/*.hxx)
DOC_SRCS =	$(SRCS) $(INCLUDES) ../main-page

COMMON_SRCS =	$(wildcard $(COMMON_DIR)/cs237-lib/*.cxx) \
		$(wildcard $(COMMON_DIR)/json/*.cxx) \
		$(wildcard $(COMMON_DIR)/obj/*.cxx) \
		$(wildcard $(COMMON_DIR)/tqt/*.cxx)
COMMON_INCLUDES = $(wildcard $(COMMON_DIR)/include/*.hxx)

OBJS =		$(notdir $(SRCS:.cxx=.o))

$(TARGET):	$(OBJS) $(COMMON_DIR)/lib/libcs237.a .depend
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS) $(LDFLAGS) $(LIBS)

%.o : %.cxx
	$(CXX) -c $(CPPFLAGS) $(CXXFLAGS) $< -o $@

$(COMMON_DIR)/lib/libcs237.a:	$(COMMON_INCLUDES) $(COMMON_SRCS)
	(cd $(COMMON_DIR)/build; $(MAKE))

# include-file dependency information
#
.depend:	$(SRCS) $(INCLUDES)
	- $(CXX) $(CPPFLAGS) -MM $(SRCS) > .depend

ifneq ($(MAKECMDGOALS),clean)
sinclude .depend
endif

# generate documentation from sources using doxygen
#
.PHONY:		doc
doc:		../doc

../doc:		$(DOC_SRCS)
	(cd ..; $(DOXYGEN) Doxyfile)
	- touch ../doc

# cleanup by removing generated files
#
.PHONY:		clean
clean:
		rm -rf *.o $(TARGET) ../doc .depend

