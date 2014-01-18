CFLAGS += -std=c99
CFLAGS += -std=gnu99 
CPPFLAGS += -g

PROCESSOR=$(shell uname -p)
ifeq  ("$(PROCESSOR)", "x86_64")
RPATHDIR=/usr/lib64
else
RPATHDIR=/usr/lib
endif

LDFLAGS += -s -Wl,-rpath=$(RPATHDIR),-Bsymbolic 

LD = g++


PROGRAM_NAME = lvasrmodule

CSOURCES = \
	lvasrmodule.c

DEP_LIBS = \
	-llv_lvspeechport -lpython2.6


COMPILE_C = gcc
COMPILE_CPP = g++

INC_DIR += -I/usr/include -I/usr/include/python2.6
LIB_DIR += -L$(RPATHDIR)

CPPOBJECTS = $(CPPSOURCES:.cpp=.o)
COBJECTS = $(CSOURCES:.c=.o)


$(PROGRAM_NAME): $(CPPOBJECTS) $(COBJECTS)
	@echo Linking ...
	$(LD) $(LDFLAGS) $(LIB_DIR) -o $@ $(COBJECTS) $(CPPOBJECTS) $(DEP_LIBS) 

%.o : %.c
	@echo Compiling  $<... 
	$(COMPILE_C) $(CFLAGS) -MMD -MP $(INC_DIR) -o $@ -c $< 

%.o : %.cpp
	@echo Compiling  $<... 
	$(COMPILE_CPP) $(CPPFLAGS) -MMD -MP $(INC_DIR) -o $@ -c $<      
clean:
	@echo Cleaning... 
	@rm -f $(CPPOBJECTS) $(COBJECTS) $(PROGRAM_NAME) 
