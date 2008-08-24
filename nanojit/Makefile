
include ../rules.mk

CXXFLAGS=${NANOJIT_CXXFLAGS}

SRC_ALL  = ${wildcard *.cpp}
SRC_UNUSED = NativeThumb.cpp
SRC = $(filter-out ${SRC_UNUSED}, ${SRC_ALL}) 

OBJ=${patsubst %.cpp, %.o, ${SRC}}

LIBNAME = libnanojit.a

all: ${LIBNAME}

clean:
	rm ${OBJ} ${LIBNAME}

%.o: %.cpp
	g++ ${CXXFLAGS} -c $<

${LIBNAME}: ${OBJ}
	ar rcs $@ $^

.PHONY: clean
