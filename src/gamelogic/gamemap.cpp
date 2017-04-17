//
//  gamemap.cpp
//  labyrinth_serv_xcode
//
//  Created by Aleksandr Borzikh on 15.04.17.
//  Copyright © 2017 hate-red. All rights reserved.
//

#include "gamemap.hpp"

#include <random>
#include "mapblock.hpp"
#include "globals.h"
#include "gameworld.hpp"

GameMap::GameMap()
{
    
}

GameMap::~GameMap()
{
    
}

void
GameMap::GenerateMap(const Configuration& settings, GameWorld * world)
{
    auto m_oRandGen = std::mt19937(settings.nSeed);
    auto m_oRandDistr = std::uniform_real_distribution<float>(0, 1000);
    
    std::vector<std::vector<MapBlockType>> tmp_map(settings.nMapSize * settings.nRoomSize + 2,
                                                   std::vector<MapBlockType>(settings.nMapSize * settings.nRoomSize + 2,
                                                                             MapBlockType::NOBLOCK));
    
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
    
    std::vector<std::vector<Room>> rooms(settings.nMapSize, std::vector<Room>(settings.nMapSize));
    
    bool red = false;
    int n = settings.nRoomSize;
    
    for (size_t i = 0; i < settings.nMapSize; i++)
    {
        if (settings.nMapSize % 2 != 1) {
            red = !red;
        }
        
        for (size_t j = 0; j < settings.nMapSize; j++)
        {
            rooms[i][j].cells.resize(n, std::vector<MapBlockType>(n, MapBlockType::WALL));
            
                // PRIM GENERATION
            std::vector<Cell> list;
            uint16_t x, y;
            
            x = (int)m_oRandDistr(m_oRandGen) % n;
            y = (int)m_oRandDistr(m_oRandGen) % n;
            
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
                int rand = (int)m_oRandDistr(m_oRandGen) % list.size();
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
                    tmp_map[i* n + k + 1][j * n + p + 1] = rooms[i][j].cells[p][k];
                }
            }
        }
    }
    int size = settings.nMapSize * settings.nRoomSize + 2;
    for (size_t i = 0; i < size; i++)
    {
        tmp_map[i][0] = MapBlockType::BORDER;
        tmp_map[i][size - 1] = MapBlockType::BORDER;
        tmp_map[0][i] = MapBlockType::BORDER;
        tmp_map[size - 1][i] = MapBlockType::BORDER;
    }
    
        // create floor
    uint32_t current_block_uid = 1;
    for(auto i = size-1; i >= 0; --i)
    {
        for(auto j = size-1; j >= 0; --j)
        {
            auto block = new NoBlock();
            
            Point2 log_coords(i,j);
            block->SetGameWorld(world);
            block->SetUID(current_block_uid);
            block->SetLogicalPosition(log_coords);
            
            world->m_apoObjects.emplace_back(block);
            
            ++current_block_uid;
        }
    }
    
    for(auto i = size-1; i >= 0; --i)
    {
        for(auto j = size-1; j >= 0; --j)
        {
            if(tmp_map[i][j] == MapBlockType::WALL)
            {
                auto block = new WallBlock();
                
                Point2 log_coords(i,j);
                block->SetGameWorld(world);
                block->SetUID(current_block_uid);
                block->SetLogicalPosition(log_coords);
                
                world->m_apoObjects.emplace_back(block);
                
                ++current_block_uid;
            }
            else if(tmp_map[i][j] == MapBlockType::BORDER)
            {
                auto block = new BorderBlock();
                
                Point2 log_coords(i,j);
                block->SetGameWorld(world);
                block->SetUID(current_block_uid);
                block->SetLogicalPosition(log_coords);
                
                world->m_apoObjects.emplace_back(block);
                
                ++current_block_uid;
            }
        }
    }
}
