# Compiler and flags
CC = g++
CCFLAGS = -Wall -DDFT_SIZE=512 -DDECIM=16

# Source files
SRCS = fft.cpp fftInput.cpp main.cpp micInput.cpp fileInput.cpp

# Object files
OBJS = $(SRCS:.cpp=.o)

# Output executable
TARGET = main

# Default target
all: $(TARGET)

# SFML configuration
SFML_PACKAGES = sfml-graphics sfml-window sfml-system sfml-audio
SFML_CCFLAGS = $(shell pkg-config --cflags $(SFML_PACKAGES))
SFML_LIBS = $(shell pkg-config --libs $(SFML_PACKAGES))

# Debug flag
DEBUG ?= 0
ifeq ($(DEBUG), 1)
	CCFLAGS += -g3
	LDFLAGS += -g3
else
	CCFLAGS += -O3
	LDFLAGS += -O3
endif

# Update CCFLAGS and LDFLAGS
CCFLAGS += $(SFML_CCFLAGS)
LDFLAGS += $(SFML_LIBS)

# Link target
$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

# Compile target
%.o: %.cpp
	$(CC) $(CCFLAGS) -c $< -o $@

# Clean up build files
clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean