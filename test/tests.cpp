/*
 * 	Unit test routines for Familyline
 *
 *  Copyright (C) 2017 Arthur M
 *
 */
#include <gtest/gtest.h>

#include <common/logger.hpp>

#include "utils.hpp"

int main(int argc, char* argv[])
{
    ::testing::InitGoogleTest(&argc, argv);

    familyline::LoggerService::createLogger(stderr, familyline::LogType::Warning);

    return RUN_ALL_TESTS();
}
