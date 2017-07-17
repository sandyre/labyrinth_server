//
//  Random.hpp
//  labyrinth_serv_xcode
//
//  Created by Aleksandr Borzikh on 15.07.17.
//  Copyright © 2017 hate-red. All rights reserved.
//

#ifndef Random_h
#define Random_h

#include <random>


template<typename T, typename U>
class RandomGenerator
{
public:
    RandomGenerator(int min, int max, int seed = 0)
    : _generator(seed),
      _distribution(min, max)
    { }

    int NextInt()
    { return _distribution(_generator); }

private:
    T _generator;
    U _distribution;
};

#endif /* Random_h */
