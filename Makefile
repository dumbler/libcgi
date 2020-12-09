CGILIB		= libcgi.a
CC		= gcc
# -g is for including debug symbols for gdb. remove this flag upon release
CFLAGS		= -Wall -O2 -ansi -pedantic
INCLUDES	= .
#DEFINES		= -DCGI_DEBUG
AR		= ar
ARFLAGS		= rc
SOURCES		= libcgi.c
HEADERS		= libcgi.h
OBJECTS		= $(SOURCES:.c=.o)

all: $(CGILIB)

$(CGILIB): $(OBJECTS)
	$(AR) $(ARFLAGS) $@ $(OBJECTS)

$(OBJECTS): $(SOURCES) $(HEADERS)
	$(CC) -c $(CFLAGS) -I$(INCLUDES) $(SOURCES) $(DEFINES)

clean:
	cd $(OUTPUT) && rm -f $(CGILIB)
	rm -f $(OBJECTS) $(CGILIB)

