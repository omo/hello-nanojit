
include ./rules.mk

CXXFLAGS = ${NANOJIT_CXXFLAGS} -I.
LDFLAGS	 = -Lnanojit/  -lnanojit

EXENAME = hello
SRC=hello.cpp jstracer.cpp
OBJ=${patsubst %.cpp, %.o, ${SRC}}

all: ${EXENAME} libs

%o: %.cpp
	g++ ${CXXFLAGS} -c $<

libs:
	cd nanojit; make

${EXENAME}: ${OBJ}
	g++ $^ ${LDFLAGS} -o $@

.PHONY: lib
