CPPC=c++
CPPFLAGS=--std=c++17
LDFLAGS=
LIBS=
INCLUDES=
OUT=main
GARBAGE=*.o
CPPSRCS=$(wildcard *.cpp)
OBJS=$(CPPSRCS:.cpp=.o)

all: $(OUT)

###########################################

$(OUT): $(OBJS)
	$(CXX) -o $@ $^ $ $(CPPFLAGS)

###########################################

.PHONY: clean ultraclean

clean: 
	rm -f $(GARBAGE)

ultraclean: clean
	rm -f $(OUT)

