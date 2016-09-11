PROJ := oss.cgi
OBJS := Oss.o Room.o Mac.o
SRCS := $(OBJS:.o=.c)
H    := $(OBJS:.o=.h)

CC   := gcc
CF   := -Wall -O3 -fasm -fomit-frame-pointer -ffast-math -funroll-loops -fasm -fomit-frame-pointer -ffast-math -funroll-loops -pedantic -ansi

default: $(PROJ)

$(PROJ): $(OBJS)
	$(CC) $(CF) -o $@ $(OBJS)

.c.o:
	$(CC) $(CF) -c $< -D NEIL #DEBUG

.PHONY: clean
clean:
	-rm $(OBJS)
