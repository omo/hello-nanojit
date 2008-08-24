
include ./rules.mk

CXXFLAGS = ${NANOJIT_CXXFLAGS} -I.
LDFLAGS	 = -Lnanojit/  -lnanojit

EXENAME = hello
SRC=hello.cpp jstracer.cpp
OBJ=${patsubst %.cpp, %.o, ${SRC}}

all: libs ${EXENAME}

%o: %.cpp
	g++ ${CXXFLAGS} -c $<

libs:
	cd nanojit; make

clean:
	cd nanojit;	 make clean
	-rm ${OBJ} ${EXENAME}

${EXENAME}: ${OBJ}
	g++ $^ ${LDFLAGS} -o $@

.PHONY: lib clean
