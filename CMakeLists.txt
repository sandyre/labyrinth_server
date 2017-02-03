cmake_minimum_required(VERSION 2.8.12 FATAL_ERROR)

project(labyrinth_server)

set(CMAKE_COLOR_MAKEFILE ON)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -lPocoNet")

set(SOURCES
	main.cpp
	gamemap.cpp
	gameserver.cpp
	gameworld.cpp
	masterserver.cpp
	)

set(HEADERS
	netpacket.hpp
	gameworld.hpp
	item.hpp
	masterserver.hpp
	monster.hpp
	globals.h
	gameserver.hpp
	gamemap.hpp
	construction.hpp
	)

add_executable(master_serv ${SOURCES})