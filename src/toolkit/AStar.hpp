//
//  AStar.hpp
//  labyrinth_server
//
//  Created by Alexandr Borzykh on 15/08/2017.
//  Copyright © 2017 HATE|RED. All rights reserved.
//

#ifndef AStar_h
#define AStar_h

#include "Point.hpp"
#include "optional.hpp"

#include <deque>
#include <limits>
#include <set>
#include <vector>

template<typename T>
using Matrix = std::vector<std::vector<T>>;


namespace
{

    using Pair = std::pair<double, Point<>>;


    struct Cell
    {
        Cell()
        : Parent({ -1, -1 }),
        F(std::numeric_limits<double>::max()),
        G(std::numeric_limits<double>::max()),
        H(std::numeric_limits<double>::max())
        { }

        Point<>	Parent;
        double F, G, H;
    };


    double calculateHValue(const Point<>& a, const Point<>& b)
    { return std::abs(a.x - b.x) + std::abs(a.y - b.y); }


    bool isValid(const Matrix<int8_t>& map, const Point<>& pt)
    { return pt.x >= 0 && pt.x < map.size() && pt.y >= 0 && pt.y < map.size(); }


    std::experimental::optional<std::deque<Point<>>>
    AStarImpl(const Matrix<int8_t>& map, const Point<>& src, const Point<>& dst)
    {
        std::deque<Point<>> result;

        Matrix<int8_t> closedList(map.size(), std::vector<int8_t>(map.size(), 0));
        Matrix<Cell> cells(map.size(), std::vector<Cell>(map.size()));

        cells[src.x][src.y].F = 0.0;
        cells[src.x][src.y].G = 0.0;
        cells[src.x][src.y].H = 0.0;
        cells[src.x][src.y].Parent = src;

        std::set<Pair> openList;
        openList.insert(std::make_pair(0.0, src));

        bool foundDest = false;

        while (!openList.empty())
        {
            Pair p = *openList.begin();
            openList.erase(openList.begin());

            Point<> current = p.second;
            closedList[current.x][current.y] = true;

            double gN, hN, fN;

            // North way
            if (const Point<> nextPoint(current.x - 1, current.y);
                isValid(map, nextPoint))
            {
                if (nextPoint == dst)
                {
                    cells[nextPoint.x][nextPoint.y].Parent = current;
                    foundDest = true;
                    break;
                }
                else if (closedList[nextPoint.x][nextPoint.y] == false
                         && map[nextPoint.x][nextPoint.y] == 1)
                {
                    gN = cells[current.x][current.y].G + 1.0;
                    hN = calculateHValue(nextPoint, dst);
                    fN = gN + hN;

                    if (cells[nextPoint.x][nextPoint.y].F == std::numeric_limits<double>::max()
                        || cells[nextPoint.x][nextPoint.y].F > fN)
                    {
                        openList.insert(std::make_pair(fN, nextPoint));

                        cells[nextPoint.x][nextPoint.y].F = fN;
                        cells[nextPoint.x][nextPoint.y].G = gN;
                        cells[nextPoint.x][nextPoint.y].H = hN;
                        cells[nextPoint.x][nextPoint.y].Parent = current;
                    }
                }
            }
            // South way
            if (const Point<> nextPoint(current.x + 1, current.y);
                isValid(map, nextPoint))
            {
                if (nextPoint == dst)
                {
                    cells[nextPoint.x][nextPoint.y].Parent = current;
                    foundDest = true;
                    break;
                }
                else if (closedList[nextPoint.x][nextPoint.y] == false
                         && map[nextPoint.x][nextPoint.y] == 1)
                {
                    gN = cells[current.x][current.y].G + 1.0;
                    hN = calculateHValue(nextPoint, dst);
                    fN = gN + hN;

                    if (cells[nextPoint.x][nextPoint.y].F == std::numeric_limits<double>::max()
                        || cells[nextPoint.x][nextPoint.y].F > fN)
                    {
                        openList.insert(std::make_pair(fN, nextPoint));

                        cells[nextPoint.x][nextPoint.y].F = fN;
                        cells[nextPoint.x][nextPoint.y].G = gN;
                        cells[nextPoint.x][nextPoint.y].H = hN;
                        cells[nextPoint.x][nextPoint.y].Parent = current;
                    }
                }
            }
            // East way
            if (const Point<> nextPoint(current.x, current.y + 1);
                isValid(map, nextPoint))
            {
                if (nextPoint == dst)
                {
                    cells[nextPoint.x][nextPoint.y].Parent = current;
                    foundDest = true;
                    break;
                }
                else if (closedList[nextPoint.x][nextPoint.y] == false
                         && map[nextPoint.x][nextPoint.y] == 1)
                {
                    gN = cells[current.x][current.y].G + 1.0;
                    hN = calculateHValue(nextPoint, dst);
                    fN = gN + hN;

                    if (cells[nextPoint.x][nextPoint.y].F == std::numeric_limits<double>::max() 
                        || cells[nextPoint.x][nextPoint.y].F > fN)
                    {
                        openList.insert(std::make_pair(fN, nextPoint));
                        
                        cells[nextPoint.x][nextPoint.y].F = fN;
                        cells[nextPoint.x][nextPoint.y].G = gN;
                        cells[nextPoint.x][nextPoint.y].H = hN;
                        cells[nextPoint.x][nextPoint.y].Parent = current;
                    }
                }
            }
            // West way
            if (const Point<> nextPoint( current.x, current.y - 1 );
                isValid(map, nextPoint))
            {
                if (nextPoint == dst)
                {
                    cells[nextPoint.x][nextPoint.y].Parent = current;
                    foundDest = true;
                    break;
                }
                else if (closedList[nextPoint.x][nextPoint.y] == false
                         && map[nextPoint.x][nextPoint.y] == 1)
                {
                    gN = cells[current.x][current.y].G + 1.0;
                    hN = calculateHValue(nextPoint, dst);
                    fN = gN + hN;
                    
                    if (cells[nextPoint.x][nextPoint.y].F == std::numeric_limits<double>::max()
                        || cells[nextPoint.x][nextPoint.y].F > fN)
                    {
                        openList.insert(std::make_pair(fN, nextPoint));
                        
                        cells[nextPoint.x][nextPoint.y].F = fN;
                        cells[nextPoint.x][nextPoint.y].G = gN;
                        cells[nextPoint.x][nextPoint.y].H = hN;
                        cells[nextPoint.x][nextPoint.y].Parent = current;
                    }
                }
            }
        }
        
        if (!foundDest)
            return std::experimental::optional<std::deque<Point<>>>();
        
        Point<> current = dst;
        while (cells[current.x][current.y].Parent != current)
        {
            result.push_front(current);
            current = cells[current.x][current.y].Parent;
        }
        
        return result;
    }
    
}


std::experimental::optional<std::deque<Point<>>>
AStar(const Matrix<int8_t>& map, const Point<>& src, const Point<>& dst)
{ return AStarImpl(map, src, dst); }

#endif /* AStar_h */
