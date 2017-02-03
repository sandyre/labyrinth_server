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
    
    struct Cell
    {
        int16_t x = 0, y = 0;
        bool  bVisited = false;
        bool  bRightOpen = false;
        bool  bLeftOpen = false;
        bool  bUpOpen = false;
        bool  bBotOpen = false;
    };
    
    /*
     Labyrinth generation algorithm taken from:
     http://ru.stackoverflow.com/questions/482663/Простой-алгоритм-генерации-лабиринта-на-c-c
     */
    std::vector<std::vector<Cell>> labyrinth(m_stSettings.nChunkWidth,
                                             std::vector<Cell>(m_stSettings.nChunkHeight));
    
    m_oRandGen = std::mt19937(settings.nSeed);
    m_oRandDistr = std::uniform_int_distribution<>(0, m_stSettings.nChunkWidth);
    
    int16_t start_x = m_oRandDistr(m_oRandGen) % m_stSettings.nChunkWidth;
    int16_t start_y = m_oRandDistr(m_oRandGen) % m_stSettings.nChunkHeight;
    
    labyrinth[start_x][start_y].bVisited = true;
    
    for(auto i = 0; i < labyrinth.size(); ++i)
    {
        for(auto j = 0; j < labyrinth[i].size(); ++j)
        {
            labyrinth[i][j].x = i;
            labyrinth[i][j].y = j;
        }
    }
    
    std::stack<Cell> path;
    path.push(labyrinth[start_x][start_y]);
    
    while(!path.empty())
    {
        Cell _cell = path.top();
        std::vector<Cell> nextStep;
        
        if (_cell.x > 0 && (labyrinth[_cell.x - 1][_cell.y].bVisited == false))
            nextStep.push_back(labyrinth[_cell.x - 1][_cell.y]);
        if (_cell.x < m_stSettings.nChunkWidth - 1 && (labyrinth[_cell.x + 1][_cell.y].bVisited == false))
            nextStep.push_back(labyrinth[_cell.x + 1][_cell.y]);
        if (_cell.y > 0 && (labyrinth[_cell.x][_cell.y - 1].bVisited == false))
            nextStep.push_back(labyrinth[_cell.x][_cell.y - 1]);
        if (_cell.y < m_stSettings.nChunkHeight - 1 && (labyrinth[_cell.x][_cell.y + 1].bVisited == false))
            nextStep.push_back(labyrinth[_cell.x][_cell.y + 1]);
        
        if (!nextStep.empty())
        {
                //выбираем сторону из возможных вариантов
            Cell next = nextStep[m_oRandDistr(m_oRandGen) % nextStep.size()];
            
                //Открываем сторону, в которую пошли на ячейках
            if (next.x != _cell.x)
            {
                if (_cell.x - next.x > 0)
                {
                    labyrinth[_cell.x][_cell.y].bLeftOpen = true;
                    labyrinth[next.x][next.y].bRightOpen = true;
                }
                else
                {
                    labyrinth[_cell.x][_cell.y].bRightOpen = true;
                    labyrinth[next.x][next.y].bLeftOpen = true;
                }
            }
            if (next.y != _cell.y)
            {
                if (_cell.y - next.y > 0)
                {
                    labyrinth[_cell.x][_cell.y].bUpOpen = true;
                    labyrinth[next.x][next.y].bBotOpen = true;
                }
                else
                {
                    labyrinth[_cell.x][_cell.y].bBotOpen = true;
                    labyrinth[next.x][next.y].bUpOpen = true;
                }
            }
            
            labyrinth[next.x][next.y].bVisited = true;
            path.push(next);
            
        }
        else
        {
                //если пойти никуда нельзя, возвращаемся к предыдущему узлу
            path.pop();
        }
    }
    
    m_oMap = GameMap::Map(m_stSettings.nChunkWidth * 3, std::vector<MapBlockType>(m_stSettings.nChunkHeight*3,
                                                                                  MapBlockType::NOBLOCK));
    
    for(auto i = 0; i < labyrinth.size(); ++i)
    {
        for(auto j = 0; j < labyrinth[i].size(); ++j)
        {
            if(!labyrinth[i][j].bUpOpen)
            {
                m_oMap[3 * i][3 * j + 1] = MapBlockType::WALL;
            }
            if(!labyrinth[i][j].bLeftOpen)
            {
                m_oMap[3 * i + 1][3 * j] = MapBlockType::WALL;
            }
            if(!labyrinth[i][j].bRightOpen)
            {
                m_oMap[3 * i + 1][3 * j + 2] = MapBlockType::WALL;
            }
            if(!labyrinth[i][j].bBotOpen)
            {
                m_oMap[3 * i + 2][3 * j + 1] = MapBlockType::WALL;
            }
        }
    }
}

const GameMap::Map&
GameMap::GetMap() const
{
    return m_oMap;
}

Vec2
GameMap::GetRandomPosition()
{
    Vec2 position;

    do
    {
        position.x = m_oRandDistr(m_oRandGen) % m_oMap.size();
        position.y = m_oRandDistr(m_oRandGen) % m_oMap[position.x].size();
    } while(m_oMap[position.x][position.y] != GameMap::MapBlockType::NOBLOCK);
    return position;
}
