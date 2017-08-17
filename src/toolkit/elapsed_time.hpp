//
//  elapsed_time.hpp
//  labyrinth_serv_xcode
//
//  Created by Aleksandr Borzikh on 06.07.17.
//  Copyright © 2017 hate-red. All rights reserved.
//

#ifndef elapsed_time_h
#define elapsed_time_h

#include <chrono>


class ElapsedTime
{
public:
    ElapsedTime()
    : _started(std::chrono::steady_clock::now())
    { }

    void Reset()
    { _started = std::chrono::steady_clock::now(); }

    template<typename T>
    T Elapsed() const
    { return std::chrono::duration_cast<T>(std::chrono::steady_clock::now() - _started); }

private:
    std::chrono::steady_clock::time_point _started;
};

#endif /* elapsed_time_h */
