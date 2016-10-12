CGILIB		= libcgi.a
CC		= gcc
CFLAGS		= -Wall -O2 -ansi -pedantic -march=pentium3
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
	rm -f $(CGILIB)
	rm -f $(OBJECTS)

