/*
    Cursor information class

    Copyright (C) 2016 Arthur M
*/

#include <client/input/input_manager.hpp>
#include <client/input/input_service.hpp>

#ifndef CURSOR_HPP
#define CURSOR_HPP

namespace familyline::input
{
class Cursor
{
private:
    static Cursor* cursor;

    HumanListenerHandler _listener;

    int _x = -1, _y = -1;

public:
    static Cursor* GetInstance()
    {
        if (!cursor)
            cursor = new Cursor{};

        return cursor;
    }

    Cursor();
    void GetPositions(int& x, int& y);
};

}  // namespace familyline::input

#endif /* end of include guard: CURSOR_HPP */
