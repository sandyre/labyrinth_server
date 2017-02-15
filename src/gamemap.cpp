//
//  gamemap.cpp
//  labyrinth_server
//
//  Created by Aleksandr Borzikh on 02.02.17.
//  Copyright © 2017 sandyre. All rights reserved.
//

#include "gamemap.hpp"

#include <random>
#include <stack>

GameMap::GameMap()
{
    
}

GameMap::~GameMap()
{
    
}

GameMap::GameMap(const GameMap::Settings& settings)
{
    m_stSettings = settings;
    m_oRandGen = std::mt19937(m_stSettings.nSeed);
    m_oRandDistr = std::uniform_int_distribution<>(0, 1000);
    m_oMap.resize(m_stSettings.nMapSize*m_stSettings.nRoomSize + 2,
                  std::vector<MapBlockType>(m_stSettings.nMapSize*m_stSettings.nRoomSize + 2, MapBlockType::NOBLOCK));
    
    struct Cell {
        Cell(uint16_t _x, uint16_t _y, MapBlockType _type) {
            x = _x;
            y = _y;
            type = _type;
        }
        
        uint16_t x;
        uint16_t y;
        MapBlockType type;
    };
    
    struct Room {
        Point2 coord;
        std::vector<std::vector<MapBlockType>> cells;
    };
    
    std::vector<std::vector<Room>> rooms(m_stSettings.nMapSize, std::vector<Room>(m_stSettings.nMapSize));
    
    bool red = false;
    int n = m_stSettings.nRoomSize;
    
    for (size_t i = 0; i < m_stSettings.nMapSize; i++)
    {
        if (m_stSettings.nMapSize % 2 != 1) {
            red = !red;
        }
        
        for (size_t j = 0; j < m_stSettings.nMapSize; j++)
        {
            rooms[i][j].cells.resize(n, std::vector<MapBlockType>(n, MapBlockType::WALL));
            
                // PRIM GENERATION
            std::vector<Cell> list;
            uint16_t x, y;
            
            x = m_oRandDistr(m_oRandGen) % n;
            y = m_oRandDistr(m_oRandGen) % n;
            
            rooms[i][j].cells[x][y] = MapBlockType::NOBLOCK;
            if (x > 0)
            {
                rooms[i][j].cells[x - 1][y] = MapBlockType::BORDER;
                list.push_back(Cell(x - 1, y, MapBlockType::BORDER));
            }
            if (x < n - 1)
            {
                rooms[i][j].cells[x + 1][y] = MapBlockType::BORDER;
                list.push_back(Cell(x + 1, y, MapBlockType::BORDER));
            }
            if (y > 0)
            {
                rooms[i][j].cells[x][y - 1] = MapBlockType::BORDER;
                list.push_back(Cell(x, y - 1, MapBlockType::BORDER));
            }
            if (y < n - 1)
            {
                rooms[i][j].cells[x][y + 1] = MapBlockType::BORDER;
                list.push_back(Cell(x, y + 1, MapBlockType::BORDER));
            }
            
            while (list.size())
            {
                int rand = m_oRandDistr(m_oRandGen) % list.size();
                Cell cell = list[rand];
                std::vector<Cell> neighbours;
                int count = 0;
                x = cell.x;
                y = cell.y;
                
                if (x > 0)
                {
                    if (rooms[i][j].cells[x - 1][y] == MapBlockType::NOBLOCK)
                    {
                        count++;
                    }
                    else
                    {
                        neighbours.push_back(Cell(x - 1, y, rooms[i][j].cells[x - 1][y]));
                    }
                }
                if (x < n - 1)
                {
                    if (rooms[i][j].cells[x + 1][y] == MapBlockType::NOBLOCK)
                    {
                        count++;
                    }
                    else
                    {
                        neighbours.push_back(Cell(x + 1, y, rooms[i][j].cells[x + 1][y]));
                    }
                }
                if (y < n - 1)
                {
                    if (rooms[i][j].cells[x][y + 1] == MapBlockType::NOBLOCK)
                    {
                        count++;
                    }
                    else
                    {
                        neighbours.push_back(Cell(x, y + 1, rooms[i][j].cells[x][y + 1]));
                    }
                }
                if (y > 0)
                {
                    if (rooms[i][j].cells[x][y - 1] == MapBlockType::NOBLOCK)
                    {
                        count++;
                    }
                    else
                    {
                        neighbours.push_back(Cell(x, y - 1, rooms[i][j].cells[x][y - 1]));
                    }
                }
                if (count != 1)
                {
                    list.erase(list.begin() + rand);
                    rooms[i][j].cells[x][y] = MapBlockType::WALL;
                    continue;
                }
                rooms[i][j].cells[x][y] = MapBlockType::NOBLOCK;
                list.erase(list.begin() + rand);
                for (auto& c : neighbours) {
                    if (c.type == MapBlockType::WALL) {
                        list.push_back(c);
                    }
                }
            }
            
            for (size_t k = 0; k < n; k++)
            {
                for (size_t p = 0; p < n; p++)
                {
                    if (rooms[i][j].cells[k][p] != MapBlockType::NOBLOCK)
                    {
                        if (m_oRandDistr(m_oRandGen) > 900)
                        {
                            rooms[i][j].cells[k][p] = MapBlockType::NOBLOCK;
                        }
                    }
                }
            }
            
            if (red) {
                for (size_t k = 0; k < n; k++)
                {
                    rooms[i][j].cells[k][0] = MapBlockType::NOBLOCK;
                    rooms[i][j].cells[k][n - 1] = MapBlockType::NOBLOCK;
                    rooms[i][j].cells[0][k] = MapBlockType::NOBLOCK;
                    rooms[i][j].cells[n - 1][k] = MapBlockType::NOBLOCK;
                }
            }
            red = !red;
            
            for (size_t k = 0; k < n; k++)
            {
                for (size_t p = 0; p < n; p++)
                {
                    m_oMap[i* n + k + 1][j * n + p + 1] = rooms[i][j].cells[p][k];
                }
            }
        }
    }
    int size = m_stSettings.nMapSize * m_stSettings.nRoomSize + 2;
    for (size_t i = 0; i < size; i++)
    {
        m_oMap[i][0] = MapBlockType::BORDER;
        m_oMap[i][size - 1] = MapBlockType::BORDER;
        m_oMap[0][i] = MapBlockType::BORDER;
        m_oMap[size - 1][i] = MapBlockType::BORDER;
    }
}

const GameMap::Map&
GameMap::GetMap() const
{
    return m_oMap;
}

Point2
GameMap::GetRandomPosition()
{
    Point2 position;
    
    do
    {
        position.x = m_oRandDistr(m_oRandGen) % m_oMap.size();
        position.y = m_oRandDistr(m_oRandGen) % m_oMap[position.x].size();
    } while(m_oMap[position.x][position.y] != GameMap::MapBlockType::NOBLOCK);
    return position;
}
