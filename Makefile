# File Makefile

files := $(shell ls)
srcs  := $(filter %.cpp, $(files))
objs  := $(patsubst %.cpp, %.o, $(srcs))

CXX := g++
CXXFLAGS := -Wall -O
LDFLAGS := -lpthread

all : main
.PHONY : all

main : $(objs)
	$(CXX)  $^ -o $@ $(LDFLAGS)

%.o : %.cpp %.h

.PHONY : log_clean clean print
print :
	echo $(objs)
    
log_clean:
	-rm *_log

clean:
	-rm main *.o *~
