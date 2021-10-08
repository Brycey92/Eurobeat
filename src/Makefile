SHELL=/bin/sh

BIN=../build/eurobeat

OS=$(shell uname)
ifeq ($(findstring CYGWIN,$(OS)),CYGWIN)
	OS=Cygwin
endif
ifeq ($(findstring WINNT,$(OS)),WINNT)
	OS=Cygwin
endif

ifeq ($(OS), Cygwin)
	NOCYGWIN= -mno-cygwin
	#fix for recently introduced problem 
	SHELL=c:/cygwin/bin/bash.exe
endif

#consider all *.c as sources  
SOURCES.cpp := $(wildcard *.cpp)


CC=g++
LINK=g++
CFLAGS=$(NOCYGWIN) -c -Ilib -Ilib/portaudio/include/ -MMD -MP -fpic -W -Wall
MORECFLAGS=
LINKFLAGS=-g -lm -L. $()
LINKLIBS=-lportaudio -lsndfile
OBJS=$(SOURCES.cpp:.cpp=.o)

debug : MORECFLAGS=-g -Og -Wundef -Winit-self
pedantic : MORECFLAGS=-g -Og -Wfloat-equal -Wundef -Winit-self -Wshadow -Wpointer-arith -Wcast-align -Wstrict-prototypes -Wstrict-overflow=5 -Waggregate-return -Wunreachable-code #-Wwrite-strings -fmessage-length=0
release : MORECFLAGS=-O2

%.o : %.cpp
	$(CC) $(CFLAGS) $(MORECFLAGS) -o $@ $<

#.SUFFIXES:
#.SUFFIXES: .d .o .h .cpp
#.cpp.o: ; $(CC) $(CFLAGS) $(MORECFLAGS) $*.cpp 

.PHONY: clean

%.d: %.cpp
	@set -e; rm -f $@; \
	$(CC) -M $(CFLAGS) $< > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

all release debug pedantic : $(BIN)

old:
	g++ -g -o eurobeat eurobeat.h util.cpp audioutil.cpp canutil.cpp -Iportaudio/include/ -lportaudio -lsndfile

$(BIN): $(OBJS)
	$(LINK) $(LINKFLAGS) $(FLAGS) -o $(BIN) $(OBJS) $(LINKLIBS)

clean:
	rm -f $(BIN) $(OBJS) *.d *~

include $(sources:.cpp=.d)
