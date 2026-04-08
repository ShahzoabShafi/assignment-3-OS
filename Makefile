CC = gcc
framesize ?= 18
varmemsize ?= 10

CFLAGS = -Wall -Wextra -std=c11 -DFRAME_STORE_SIZE=$(framesize) -DVAR_STORE_SIZE=$(varmemsize) -pthread
OBJS = shell.o interpreter.o shellmemory.o pcb.o queue.o schedule_policy.o program.o

mysh: $(OBJS)
	$(CC) $(CFLAGS) -o mysh $(OBJS)

shell.o: shell.c shell.h interpreter.h shellmemory.h
	$(CC) $(CFLAGS) -c shell.c

interpreter.o: interpreter.c interpreter.h shell.h shellmemory.h pcb.h queue.h schedule_policy.h program.h
	$(CC) $(CFLAGS) -c interpreter.c

shellmemory.o: shellmemory.c shellmemory.h
	$(CC) $(CFLAGS) -c shellmemory.c

pcb.o: pcb.c pcb.h program.h shellmemory.h
	$(CC) $(CFLAGS) -c pcb.c

queue.o: queue.c queue.h pcb.h schedule_policy.h
	$(CC) $(CFLAGS) -c queue.c

schedule_policy.o: schedule_policy.c schedule_policy.h interpreter.h queue.h
	$(CC) $(CFLAGS) -c schedule_policy.c

program.o: program.c program.h shell.h shellmemory.h
	$(CC) $(CFLAGS) -c program.c

clean:
	rm -f mysh *.o
	rm -rf backing_store
	rm -f prog[0-9] prog[0-9][0-9]