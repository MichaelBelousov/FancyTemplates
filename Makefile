CPPC=c++
CPPFLAGS=--std=c++17
OPTMZ=-O2
LDFLAGS=
LIBS=
INCLUDES=
OUT=main
CPPSRCS=$(wildcard *.cpp)
OBJS=$(CPPSRCS:.cpp=.o)
DEPS=$(OBJS:.o=.d)

all: _prepare $(OUT)

###########################################

%.d: %.c
	@$(CPP) $(CFLAGS) $< -MM -MT  $(A:.d=.o) >$@

$(OUT): $(OBJS)
	$(CXX) -o $@ $^ $ $(CPPFLAGS)

debug: ultraclean _debug all

_debug:
	$(eval CPPFLAGS += -g -DDEBUG)
	$(eval OPTMZ = -O0)

_prepare:
	$(eval CPPFLAGS += $(OPTMZ))

###########################################

.PHONY: clean
clean: 
	rm -f $(OBJS) $(DEPS)

.PHONY: ultraclean
ultraclean: clean
	rm -f $(OUT)

