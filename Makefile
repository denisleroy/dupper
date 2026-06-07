TARGET = dupper

CXXFLAGS = -g -O0 -std=c++11

SOURCES = \
	context.cc \
	main.cc	\
	md5.cc	\
	md5_comparator.cc \
	scanner.cc \
	size_comparator.cc

OBJECTS = $(SOURCES:.cc=.o)

HEADERS = $(wildcard *.h)

%.o: %.cc $(HEADERS)
	$(CXX) -c $(CXXFLAGS) $< -o $@

all: $(TARGET)

$(TARGET): $(OBJECTS) $(SOURCES)
	$(CXX) $(OBJECTS) -o $(TARGET)

clean:
	rm -f $(TARGET) $(OBJECTS)
