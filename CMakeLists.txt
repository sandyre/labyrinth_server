cmake_minimum_required(VERSION 2.8.12 FATAL_ERROR)

project(labyrinth_server)

set(CMAKE_COLOR_MAKEFILE ON)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -lPocoNet")

set(SOURCES
	src/main.cpp
	src/gamemap.cpp
	src/gameserver.cpp
	src/gameworld.cpp
	src/masterserver.cpp
	)

set(HEADERS
	src/netpacket.hpp
	src/gameworld.hpp
	src/item.hpp
	src/masterserver.hpp
	src/monster.hpp
	src/globals.h
	src/gameserver.hpp
	src/gamemap.hpp
	src/construction.hpp
	)

add_executable(master_serv ${SOURCES})
