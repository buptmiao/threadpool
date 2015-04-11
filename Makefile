# File Makefile

files := $(shell ls)
srcs  := $(filter %.cc, $(files))
objs  := $(patsubst %.cc, %.o, $(srcs))

CXX := g++
CXXFLAGS := -Wall -O
LDFLAGS := -lpthread

all : main
.PHONY : all

main : $(objs)
	$(CXX)  $^ -o $@ $(LDFLAGS)

%.o : %.cc %.h

.PHONY : log_clean clean print
print :
	echo $(objs)
    
log_clean:
	-rm *_log

clean:
	-rm main *.o *~
