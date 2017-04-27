#CROSS_COMPILE	:= /opt/buildroot-gcc463/usr/bin/mipsel-linux-
CC		:= $(CROSS_COMPILE)gcc
LD		:= $(CROSS_COMPILE)gcc
AR		:= $(CROSS_COMPILE)ar
STRIP		:= $(CROSS_COMPILE)strip

CFLAGS		:= 
LDFLAGS		:= 
LIBS            :=

CFLAGS		+= -Wall -DXXX 
LDFLAGS		+= -pthread

SRC 	:= $(wildcard *.c)
OBJS	:= $(SRC:%.c=%.o)

EXEC	:= test

all:  $(OBJS) $(EXEC)

$(EXEC): $(OBJS) 
	$(CC) $< -o $@ $(LDFLAGS) $(LIBS)
.PHONY : clean
clean:
	-rm -rf *~ *.o 
	-rm -rf $(EXEC)
