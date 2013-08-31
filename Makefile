SOCKSTREAM_SOURCES = sockstreambuf.cc sockstream.cc

EXAMPLE = example
EXAMPLE_SOURCES = example.cc $(SOCKSTREAM_SOURCES)

CXX = g++
CXXFLAGS = -ansi -Wall -pedantic
LDFLAGS =

ALL=$(EXAMPLE)

.PHONY: clean

all: $(ALL)

clean:
	rm -f $(ALL)

$(EXAMPLE): $(EXAMPLE_SOURCES)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)
