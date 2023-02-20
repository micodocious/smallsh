# the compiler: gcc for C program, define as g++ for C++
CC = gcc

# compiler flags:
#  -g    adds debugging information to the executable file
#  -Wall turns on most, but not all, compiler warnings
CFLAGS  = -std=c99 -Wall -Wextra -Wpedantic -Werror

# the build target executable:
TARGET = smallsh

all: $(TARGET)

$(TARGET): $(TARGET).c
	$(CC) $(CFLAGS) -o $(TARGET) $(TARGET).c

clean:
	$(RM) $(TARGET)
	$(RM) junk
	$(RM) mytestresults
	$(RM) *.o
