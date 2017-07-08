cmake_minimum_required(VERSION 2.8.12 FATAL_ERROR)

project(labyrinth_server)

set(POCO_PREFIX      "/usr/local/")
set(POCO_INCLUDE_DIR "${POCO_PREFIX}/include")
set(POCO_LIB_DIR     "${POCO_PREFIX}/lib")
set(POCO_LIBS        "${POCO_LIB_DIR}/libPocoNet.a"
					 "${POCO_LIB_DIR}/libPocoData.a"
					 "${POCO_LIB_DIR}/libPocoDataMySQL.a"
                     "${POCO_LIB_DIR}/libPocoFoundation.a")
set(MYSQLCONNECTOR_LIB "/usr/local/lib/libmysqlcppconn-static.a")

set(CMAKE_COLOR_MAKEFILE ON)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")

set(SOURCES
	src/main.cpp
	src/gameserver.cpp
	src/utils/named_logger.cpp
  src/services/system_monitor.cpp
	src/masterserver.cpp
	src/gamelogic/construction.cpp
	src/gamelogic/effect.cpp
	src/gamelogic/gamemap.cpp
	src/gamelogic/gameobject.cpp
	src/gamelogic/gameworld.cpp
	src/gamelogic/item.cpp
	src/gamelogic/mapblock.cpp
	src/gamelogic/units/hero.cpp
	src/gamelogic/units/mage.cpp
	src/gamelogic/units/monster.cpp
	src/gamelogic/units/priest.cpp
	src/gamelogic/units/rogue.cpp
	src/gamelogic/units/unit.cpp
	src/gamelogic/units/warrior.cpp
	)

set(HEADERS
	src/gameserver.hpp
	src/utils/named_logger.hpp
  src/services/system_monitor.hpp
	src/masterserver.hpp
	src/player.hpp
	src/date.h
	src/globals.h
	src/gsnet_generated.h
	src/msnet_generated.h
	src/gamelogic/construction.hpp
	src/gamelogic/gameobject.hpp
	src/gamelogic/item.hpp
	src/gamelogic/mapblock.hpp
	src/gamelogic/effect.hpp
	src/gamelogic/gameworld.hpp
	src/gamelogic/gamemap.hpp
	src/gamelogic/units/hero.hpp
	src/gamelogic/units/mage.hpp
	src/gamelogic/units/monster.hpp
	src/gamelogic/units/priest.hpp
	src/gamelogic/units/unit.hpp
	src/gamelogic/units/warrior.hpp
	)

add_executable(labyrinth_server ${SOURCES})

target_include_directories(labyrinth_server PRIVATE "${POCO_INCLUDE_DIR}")
target_link_libraries(labyrinth_server "${POCO_LIBS}" "${MYSQLCONNECTOR_LIB}")
