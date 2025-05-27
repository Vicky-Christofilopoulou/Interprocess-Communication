SOURCE_PARENT = parent.c child.c functions.c shared_memory.c

OBJS_PARENT = parent.o child.o functions.o shared_memory.o

OUT_PARENT = parent

CC = gcc
FLAGS = -c -g
LIBS = -pthread

all: $(OUT_PARENT)

$(OUT_PARENT): $(OBJS_PARENT)
	$(CC) $(OBJS_PARENT) -o $(OUT_PARENT) $(LIBS)

parent.o: parent.c shared_memory.h functions.h
	$(CC) $(FLAGS) parent.c

child.o: child.c shared_memory.h functions.h
	$(CC) $(FLAGS) child.c

functions.o: functions.c shared_memory.h functions.h
	$(CC) $(FLAGS) functions.c

shared_memory.o: shared_memory.c shared_memory.h
	$(CC) $(FLAGS) shared_memory.c

clean:
	rm -f *.o $(OUT_PARENT)
