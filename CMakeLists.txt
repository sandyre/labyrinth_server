cmake_minimum_required(VERSION 2.8.12 FATAL_ERROR)

project(labyrinth_server)

set(POCO_PREFIX      "/usr/local/")
set(POCO_INCLUDE_DIR "${POCO_PREFIX}/include")
set(POCO_LIB_DIR     "${POCO_PREFIX}/lib")
set(POCO_LIBS
	"${POCO_LIB_DIR}/libPocoNet.a"
	"${POCO_LIB_DIR}/libPocoDataMySQL.a"
	"${POCO_LIB_DIR}/libPocoData.a"
	"${POCO_LIB_DIR}/libPocoFoundation.a")

set(CMAKE_COLOR_MAKEFILE ON)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -pthread -lmysqlclient")

set(SOURCES
	src/main.cpp
	src/GameServersController.cpp
	src/masterserver.cpp

	src/gameserver/gameserver.cpp
    src/gameserver/ConnectionsManager.cpp
    src/gameserver/PlayerConnection.cpp
	src/gameserver/gamelogic/construction.cpp
	src/gameserver/gamelogic/effect.cpp
	src/gameserver/gamelogic/gamemap.cpp
	src/gameserver/gamelogic/gameobject.cpp
	src/gameserver/gamelogic/gameworld.cpp
	src/gameserver/gamelogic/item.cpp
	src/gameserver/gamelogic/mapblock.cpp
	src/gameserver/gamelogic/units/hero.cpp
	src/gameserver/gamelogic/units/mage.cpp
	src/gameserver/gamelogic/units/monster.cpp
	src/gameserver/gamelogic/units/priest.cpp
	src/gameserver/gamelogic/units/rogue.cpp
	src/gameserver/gamelogic/units/unit.cpp
	src/gameserver/gamelogic/units/warrior.cpp

	src/services/DatabaseAccessor.cpp
	src/services/system_monitor.cpp

	src/toolkit/named_logger.cpp
	)

add_executable(labyrinth_server ${SOURCES})

target_include_directories(labyrinth_server PRIVATE "${POCO_INCLUDE_DIR}")
target_link_libraries(labyrinth_server "${POCO_LIBS}")
