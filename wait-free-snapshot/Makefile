.SUFFIXES: .cpp .o

CC=g++

SRCDIR=src/
INC=include/

# main source file
TARGET_SRC:=$(SRCDIR)main.cpp
TARGET_OBJ:=$(SRCDIR)main.o

CFLAGS+= -g -fPIC -I $(INC)

TARGET=run

all: $(TARGET)

$(TARGET): $(TARGET_OBJ)
	$(CC) $(CFLAGS) $< -o $@ -lboost_system -lboost_thread-mt -lpthread

%.o: %.cpp
	$(CC) $(CFLAGS) $^ -c -o $@ 

clean:
	rm $(TARGET) $(TARGET_OBJ)

