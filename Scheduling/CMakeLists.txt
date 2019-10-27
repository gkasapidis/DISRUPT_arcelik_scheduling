cmake_minimum_required(VERSION 3.9)
project(Scheduling)

add_executable(Scheduling
        BufferClass.cpp
        BufferClass.h
        CandidateList.h
        CObject.cpp
        CObject.h
        console_colors.h
        Graph.cpp
        Graph.h
        hash_tuple.h
        JobClass.cpp
        JobClass.h
        ListObject.cpp
        ListObject.h
        LSCriticalMoves.cpp
        LSSolutionClass.cpp
        LSSolutionClass.h
        MachineClass.cpp
        MachineClass.h
        MachineUnav.cpp
        MachineUnav.h
        MachOpTable.cpp
        MachOpTable.h
        OperationClass.cpp
        OperationClass.h
        Parameters.h
        Pool.cpp
        Pool.h
        PoolItemClass.h
        ProbabilisticMoveClass.cpp
        ProbabilisticMoveClass.h
        ProblemInterface.cpp
        ProblemInterface.h
        ProcessStep.h
        ProcessStepClass.cpp
        Resource.h
        shopscheduling.cpp
        simplePool.cpp
        simplePool.h
        SolutionClass.cpp
        SolutionClass.h
        SSPool.cpp
        SSPool.h
        StageClass.cpp
        StageClass.h
        DataClasses/ProductionOrder.h
        DataClasses/Product.h
        DataClasses/ProductFamily.h
        DataClasses/ProductionLine.h
        string_util.h DataClasses/ProblemDataset.h DataClasses/ProblemDataset.cpp
        db_utils.h db_utils.cpp
        csv_utils.h csv_utils.cpp DataClasses/event.h)



#CMAKE TWEAKS

set(CMAKE_CXX_STANDARD 11)

MESSAGE("BUILD TYPE:        " ${CMAKE_BUILD_TYPE})

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

#GCC Flags
set(CMAKE_CXX_FLAGS "-Wall -Wextra -std=c++11")
set(CMAKE_CXX_FLAGS_DEBUG "-no-pie -g -pg -O0 -lm")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -lm")
set(CMAKE_CXX_FLAGS_WINDOWSRELEASE "-O3 -static -static-libgcc -static-libstdc++")


#Set WindowsRelease Custom Commands
if (CMAKE_BUILD_TYPE STREQUAL "WindowsRelease")
    MESSAGE("Setting Windows Release Flags")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_WINDOWSRELEASE}")
    set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME "${PROJECT_NAME}.exe")
endif (CMAKE_BUILD_TYPE STREQUAL  "WindowsRelease")


#Generic Flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -lmysqlclient -lz")

target_link_libraries(Scheduling mysqlclient)


MESSAGE("COMPILER FLAGS:" ${CMAKE_CXX_FLAGS})
MESSAGE("TARGET:" ${TARGET})
#MESSAGE("COMPILER LINK_EXECUTABLE FLAGS:" ${CMAKE_CXX_LINK_EXECUTABLE})
#MESSAGE("COMPILER EXE_LINKER_FLAGS:" ${CMAKE_EXE_LINKER_FLAGS})
