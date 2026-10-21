CXX      = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2
TARGET   = calc

all: $(TARGET)
	./$(TARGET) tests.txt

$(TARGET): main.cpp
	$(CXX) $(CXXFLAGS) -o $(TARGET) main.cpp

clean:
	rm -f $(TARGET)

.PHONY: all clean