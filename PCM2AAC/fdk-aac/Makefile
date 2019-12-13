CC = gcc
CXX = g++
CFLAG = -g -Wno-deprecated
LIBS  = -lfdk-aac
EXCUTABLE_FILE = demo

SOURCE=$(wildcard ./*.cpp )
INCLUDE = -I ./

OBJECTS=$(patsubst %.cpp,%.o,$(SOURCE))
$(EXCUTABLE_FILE) : $(OBJECTS)
	$(CXX) $(INCLUDE) $(CFLAG) $^ -o $@ $(LIBS)
$(OBJECTS):%.o : %.cpp
	$(CXX) $(INCLUDE) -c $(CFLAG) $< -o $@ $(LIBS)

.PHONY:clean
clean:
	rm -rf $(OBJECTS) $(EXCUTABLE_FILE)
