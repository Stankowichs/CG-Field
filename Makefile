CXX := g++
CXXFLAGS := -std=c++17 -Wall -Wextra
LDFLAGS := -lfreeglut -lopengl32 -lglu32 -lgdi32 -lwinmm

TARGET := CGProject.exe
SRCS := CGProject.cpp Logic.cpp Render.cpp Audio.cpp
OBJS := $(SRCS:.cpp=.o)

.PHONY: all run clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(OBJS) -o $@ $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

run: $(TARGET)
	.\$(TARGET)

clean:
	del /Q $(OBJS) $(TARGET) 2>nul || exit 0
