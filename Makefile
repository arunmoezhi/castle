TBB_INC=${HOME}/tbb/tbb42_20140601oss/include
TBB_LIB=${HOME}/tbb/tbb42_20140601oss/build/linux_intel64_gcc_cc4.6.3_libc2.14.90_kernel3.6.11_release
TBBFLAGS=-I$(TBB_INC) -Wl,-rpath,$(TBB_LIB) -L$(TBB_LIB) -ltbb
GSL_INC=${HOME}/gsl/installpath/include
GSL_LIB=${HOME}/gsl/installpath/lib
GSLFLAGS=-I$(GSL_INC) -Wl,-rpath,$(GSL_LIB) -L$(GSL_LIB) -lgsl -lgslcblas -lm
JEMALLOC_INC=${HOME}/jemalloc/installpath/include
JEMALLOC_LIB=${HOME}/jemalloc/installpath/lib
JEMALLOCFLAGS=-I$(JEMALLOC_INC) -Wl,-rpath,$(JEMALLOC_LIB) -L$(JEMALLOC_LIB) -ljemalloc
CC=g++482
CFLAGS= -O3 -lrt -lpthread -std=gnu++0x -march=native 
SRC1= ./src/Test.cpp
OBJ1= ./bin/castle.o
all: 
	$(CC) $(CFLAGS) $(TBBFLAGS) $(GSLFLAGS) $(JEMALLOCFLAGS) -o $(OBJ1) $(SRC1)
clean:
	rm -rf ./bin/*.*

