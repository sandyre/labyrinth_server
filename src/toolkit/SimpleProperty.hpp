//
//  SimpleProperty.hpp
//  labyrinth_server
//
//  Created by Alexandr Borzykh on 27/08/2017.
//  Copyright © 2017 HATE|RED. All rights reserved.
//

#ifndef SimpleProperty_h
#define SimpleProperty_h

#include <cstdint>


template<typename T = int16_t>
class SimpleProperty
{
public:
    SimpleProperty(T initialValue, T minValue, T maxValue)
    : _value(initialValue),
      _minValue(minValue),
      _maxValue(maxValue)
    { }

    SimpleProperty& operator+=(T value)
    {
        _value += value;

        if (_value > _maxValue)
            _value = _maxValue;

        return *this;
    }

    SimpleProperty& operator=(T value)
    {
        _value = value;

        if (_value >= _minValue &&
            _value <= _maxValue)
            _value = _minValue;

        return *this;
    }

    SimpleProperty& operator-=(T value)
    {
        _value -= value;

        if (_value < _minValue)
            _value = _minValue;

        return *this;
    }

    T Max() const
    { return _maxValue; }

    T Min() const
    { return _minValue; }

    operator T() const
    { return _value; }

private:
    T       _value;
    T       _minValue;
    T       _maxValue;
};

#endif /* SimpleProperty_h */
