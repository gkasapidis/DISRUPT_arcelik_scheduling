#Compilers
CC = g++
LINKER = g++
RM = rm


#COMPILER FLAGS
#CFLAGS is given from make input
#CFLAGS = -std=c++11 -O3
#DFLAGS = -std=c++11 -g -O0

#OBJPATH is given from make input and it should be the path for the .o files
#EXECPATH is given from make input and it should be the path for the final executable file
SOURCES=CObject.cpp Graph.cpp ListObject.cpp MachineUnav.cpp OperationClass.cpp \
		JobClass.cpp MachineClass.cpp ProcessStepClass.cpp StageClass.cpp SolutionClass.cpp \
		OperationFrequencyMapClass.cpp ProbabilisticMoveClass.cpp LSSolutionClass.cpp\
		simplePool.cpp SSPool.cpp Pool.cpp ProblemInterface.cpp MachOpTable.cpp
OBJECTS=$(SOURCES:%.cpp=$(OBJPATH)/%.o)
HEADERS=Graph.h
FLAGS=$(CFLAGS)
EXECUTABLE=shopscheduling

#$(info $$EXECPATH is [${EXECPATH}])
#$(info $$OBJPATH is [${OBJPATH}])

all: $(OBJECTS) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(FLAGS) $(HEADERS) $(EXECUTABLE).cpp -o $(EXECPATH)/$(EXECUTABLE) $(OBJECTS)

$(OBJPATH)/%.o: %.cpp
	$(CC) $(FLAGS) -c $< -o $@

clean:
	  $(RM) $(EXECPATH)/shopscheduling *.o $(OBJPATH)/*.o
