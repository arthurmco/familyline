#pragma once

/**
 * This class allow us to select a function to run during the
 * game loop
 */

#include <functional>


/**
 * This function allows us to run different functions
 * in the game loop, without the loop itself needing to
 * know it
 */
class LoopRunner
{
private:
    /**
     * The function we need to run
     *
     * If this function returns false, the loop must be stopped
     */
    std::function<bool()> fn_;

    /**
     * The function we loaded
     *
     * We first store them, and swap them before running, so
     * that we do not crash because we loaded a loop run
     * function in the middle of another loop run function
     */
    std::function<bool()> loadedfn_;

public:
    void load(std::function<bool()> fn) { loadedfn_ = fn; }

    bool run()
    {
        fn_ = loadedfn_;
        return fn_();
    }
};

