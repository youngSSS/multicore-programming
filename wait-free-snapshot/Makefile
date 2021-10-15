.SUFFIXES: .cpp .o

CC=g++

SRCDIR=src/
INC=include/
LIBS=lib/

# SRCS:=$(wildcard src/*.cpp)
# OBJS:=$(SRCS:.cpp=.o)

# main source file
TARGET_SRC:=$(SRCDIR)main.cpp
TARGET_OBJ:=$(SRCDIR)main.o

# Include more files if you write another source file.
# SRCS_FOR_LIB:= \
# 	$(SRCDIR)db_api.cpp \
# 	$(SRCDIR)index_manager.cpp \
# 	$(SRCDIR)file.cpp \
# 	$(SRCDIR)buffer_manager.cpp \
# 	$(SRCDIR)lock_manager.cpp \
# 	$(SRCDIR)transaction_manager.cpp \
# 	$(SRCDIR)log_manager.cpp

OBJS_FOR_LIB:=$(SRCS_FOR_LIB:.cpp=.o)

CFLAGS+= -g -fPIC -I $(INC)

TARGET=run

all: $(TARGET)

$(TARGET): $(TARGET_OBJ)
	$(CC) $(CFLAGS) $< -o $@ -L $(LIBS) -lpthread

%.o: %.cpp
	$(CC) $(CFLAGS) $^ -c -o $@ -lpthread

clean:
	rm $(TARGET) $(TARGET_OBJ) $(OBJS_FOR_LIB) $(LIBS)*
