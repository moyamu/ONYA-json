####################################################################################################
## I, the creator of this work, hereby release it into the public domain. This applies worldwide.
## In case this is not legally possible: I grant anyone the right to use this work for any purpose,
## without any conditions, unless such conditions are required by law.
####################################################################################################

## ONYA:json - JSON parser
## Makefile

####################################################################################################
# Configuration

CXX = g++
CC = gcc

# release build:
CFLAGS = -O3 -Wall -Werror
CXXFLAGS = ${CFLAGS}
LDFLAGS = 

# maintainer:
#CFLAGS = -g -Wall -Werror -DPMU_=1
#CXXFLAGS = ${CFLAGS}
#LDFLAGS = -g 
#
#CFLAGS = -g -ftest-coverage -fprofile-arcs -pg -O3 -Wall -Werror
#CXXFLAGS = ${CFLAGS}
#LDFLAGS = -g -ftest-coverage -fprofile-arcs -pg

# additional Linker flags required for the tests (for Linux)
LDLIBS_RT = -lrt

####################################################################################################
# Main targets

.PHONY: all
all: tests.ok mktestdata jsonpp

.PHONY: help
help:
	@echo "make all          (default) Build everything and run the tests"
	@echo "make clean        Clean all generated files"
	@echo "make benchmark    Run the speed test"

.PHONY: clean
clean:
	rm -f tests.ok tests mktestdata testdata_*.json
	rm -f *.a *.o
	rm -f gmon.out *.gcno *.gcda *.gcov

.PHONY: tar
tar: ONYA-json.tar.gz
tar_files := json.cc json.h tests.cc _test.cc _test.h _pmu.cc _pmu.h mktestdata.cc Makefile
ONYA-json.tar.gz: ${tar_files}
	tar cfz $@ ${tar_files}

%.o: %.cc Makefile
	${CXX} ${CXXFLAGS} -c -o $@ $<

####################################################################################################
# Example programs

jsonpp: jsonpp.c
	$(CC) ${LDFLAGS} ${CFLAGS} -o $@ $<


####################################################################################################
# Tests

tests.ok: tests
	./tests && touch $@

tests_objs = tests.o json.o _pmu.o _test.o
tests: ${tests_objs}
	$(CXX) ${LDFLAGS} -o $@ ${tests_objs} ${LDLIBS_RT}

tests.o: tests.cc json.h _test.h _pmu.h

mktestdata: mktestdata.o
	${CXX} ${LDFLAGS} -o $@ mktestdata.o

.PHONY: benchmark
benchmark: tests testdata_900_2.json testdata_10_5.json
	./tests testdata_900_2.json testdata_10_5.json

testdata_900_2.json: mktestdata
	./mktestdata 900 900 >$@
testdata_10_5.json: mktestdata
	./mktestdata 10 10 10 10 10 >$@

####################################################################################################
# Dependencies (g++ -MM *.cc)

_pmu.o: _pmu.cc _pmu.h
_test.o: _test.cc _test.h
json.o: json.cc json.h _pmu.h
mktestdata.o: mktestdata.cc
tests.o: tests.cc _pmu.h _test.h json.h
