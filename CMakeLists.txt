cmake_minimum_required(VERSION 2.8.12 FATAL_ERROR)

project(labyrinth_server)

find_library(LIBPOCONET_LOCATION libPocoNet.a)
find_library(LIBPOCOFOUND_LOCATION libPocoFoundation.a)
find_library(LIBFLAT_LOCATION libflatbuffers.a)

set(CMAKE_COLOR_MAKEFILE ON)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -pthread")

set(SOURCES
	src/main.cpp
	src/gamemap.cpp
	src/gameserver.cpp
	src/gameworld.cpp
	src/masterserver.cpp
	)

set(HEADERS
	src/gameworld.hpp
	src/item.hpp
	src/masterserver.hpp
	src/monster.hpp
	src/globals.h
	src/gameserver.hpp
	src/gamemap.hpp
	src/construction.hpp
	src/msnet_generated.h
	src/gsnet_generated.h
	src/player.hpp
	)

add_executable(master_serv ${SOURCES})
target_link_libraries(master_serv ${LIBPOCONET_LOCATION} ${LIBPOCOFOUND_LOCATION} ${LIBFLAT_LOCATION})
