CC = gcc
OBJS = eurovision.o map.o country.o judge.o main.o
EXEC = eurovision
DEBUG_FLAG = # now empty, assign -g for debug
COMP_FLAG = -std=c99 -Wall -Werror


$(EXEC) : $(OBJS)
	$(CC) $(DEBUG_FLAG) $(OBJS) -o $@ -L. -lmtm
eurovision.o: eurovision.c map.h country.h judge.h eurovision.h list.h
	$(CC) -c $(DEBUG_FLAG) $(COMP_FLAG) $*.c
map.o: map.c map.h
	$(CC) -c $(DEBUG_FLAG) $(COMP_FLAG) $*.c
country.o: country.c map.h country.h
	$(CC) -c $(DEBUG_FLAG) $(COMP_FLAG) $*.c
judge.o: judge.c map.h judge.h
	$(CC) -c $(DEBUG_FLAG) $(COMP_FLAG) $*.c
main.o: main.c
	$(CC) -c $(DEBUG_FLAG) $(COMP_FLAG) $*.c

clean:
	rm -f $(OBJS) $(EXEC)
