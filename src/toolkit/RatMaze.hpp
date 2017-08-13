//
//  RatMaze.hpp
//  labyrinth_server
//
//  Created by Alexandr Borzykh on 26/07/2017.
//  Copyright © 2017 HATE|RED. All rights reserved.
//

#ifndef RatMaze_hpp
#define RatMaze_hpp

#include "Point.hpp"
#include "optional.hpp"

#include <deque>

using Matrix = std::vector<std::vector<uint8_t>>;


namespace
{

    enum Direction
    {
        UP,
        DOWN,
        LEFT,
        RIGHT
    };

    Direction ComputeDirection(const Point<>& a, const Point<>& b)
    {
        if(a.x > b.x)
            return LEFT;
        else if(a.x < b.x)
            return RIGHT;
        else if(a.y > b.y)
            return DOWN;

        return UP;
    }

    bool inRange(Matrix& matrix, Point<> pt)
    { return matrix.size() > pt.x && pt.x >= 0 && matrix[pt.x].size() > pt.y && pt.y >= 0; }

    bool
    RatMazeImpl(Matrix& matrix, Point<> current, Point<> dst, Matrix& solution, int recursion_depth)
    {
        if(recursion_depth < 0)
            return false;

        if(current == dst)
        {
            solution[current.x][current.y] = 1;
            return true;
        }

        if(inRange(matrix, current)
           && matrix[current.x][current.y] == 1)
        {
            solution[current.x][current.y] = 1;

            enum Direction preferredDirection = ComputeDirection(current, dst);

            switch(preferredDirection)
            {
                case RIGHT:
                {
                    if(RatMazeImpl(matrix, { current.x + 1, current.y }, dst, solution, recursion_depth-1))
                        return true;
                    break;
                }

                case LEFT:
                {
                    if(RatMazeImpl(matrix, { current.x - 1, current.y }, dst, solution, recursion_depth-1))
                        return true;
                    break;
                }

                case UP:
                {
                    if(RatMazeImpl(matrix, { current.x, current.y + 1 }, dst, solution, recursion_depth-1))
                        return true;
                    break;
                }

                case DOWN:
                {
                    if(RatMazeImpl(matrix, { current.x, current.y - 1 }, dst, solution, recursion_depth-1))
                        return true;
                    break;
                }
            }

            if(RatMazeImpl(matrix, { current.x + 1, current.y }, dst, solution, recursion_depth-1))
                return true;
            else if(RatMazeImpl(matrix, { current.x - 1, current.y }, dst, solution, recursion_depth-1))
                return true;
            else if(RatMazeImpl(matrix, { current.x - 1, current.y }, dst, solution, recursion_depth-1))
                return true;
            else if(RatMazeImpl(matrix, { current.x, current.y - 1 }, dst, solution, recursion_depth-1))
                return true;

            solution[current.x][current.y] = 0;
            return false;
        }

       return false;
    }

    std::deque<Point<>>
    MatrixToPath(Matrix& solution, Point<> src, Point<> dest)
    {
        Point<> current = src;
        std::deque<Point<>> path;
        while(current != dest)
        {
            path.push_back(current);
            solution[current.x][current.y] = 0;

            if(const Point<> left = Point<>(current.x - 1, current.y);
               inRange(solution, left) && solution[left.x][left.y] == 1)
                current = left;
            else if(const Point<> right = Point<>(current.x + 1, current.y);
               inRange(solution, right) && solution[right.x][right.y] == 1)
                current = right;
            else if(const Point<> upper = Point<>(current.x, current.y + 1);
               inRange(solution, upper) && solution[upper.x][upper.y] == 1)
                current = upper;
            else if(const Point<> bottom = Point<>(current.x, current.y - 1);
               inRange(solution, bottom) && solution[bottom.x][bottom.y] == 1)
                current = bottom;
        }
        path.push_back(dest);

        return path;
    }

    std::experimental::optional<std::deque<Point<>>>
    RatMazeInterface(Matrix& matrix, const Point<>& src, const Point<>& dst)
    {
        Matrix solution(matrix.size(), std::vector<uint8_t>(matrix[0].size(), 0));
        if(!RatMazeImpl(matrix, src, dst, solution, 64))
            return std::experimental::optional<std::deque<Point<>>>();

        return MatrixToPath(solution, src, dst);
    }

}

std::experimental::optional<std::deque<Point<>>>
RatMaze(Matrix& matrix, const Point<>& src, const Point<>& dst)
{ return RatMazeInterface(matrix, src, dst); }

#endif /* RatMaze_hpp */
