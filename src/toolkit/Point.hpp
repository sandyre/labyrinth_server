//
//  Point.hpp
//  labyrinth_serv_xcode
//
//  Created by Aleksandr Borzikh on 15.07.17.
//  Copyright © 2017 hate-red. All rights reserved.
//

#ifndef Point_h
#define Point_h

#include <ostream>
#include <cmath>


template<typename T = float>
struct Point
{
    Point(T X = T(), T Y = T())
    : x(X),
      y(Y)
    { }

    T Distance(const Point<T>& b) const
    { return std::sqrt((b.x - x) * (b.x - x) + (b.y - y) * (b.y - y)); }
    
    T Distance(const Point<T>&& b) const
    { return Distance(b); }

    bool operator==(const Point<T>& b) const
    { return x == b.x && y == b.y; }

    bool operator==(const Point<T>&& b) const
    { return operator==(b); }

    bool operator!=(const Point<T>& b) const
    { return !operator==(b); }

    bool operator!=(const Point<T>&& b) const
    { return !operator==(b); }

    T x, y;
};

template<typename T>
std::ostream& operator <<(std::ostream& stream,const Point<T>& pt)
{ return stream << "{ " << pt.x << "; " << pt.y << " }"; }

#endif /* Point_h */
