#pragma once

/**
 * Parse and show parameters
 *
 * Copyright (C) 2020 Arthur M.
 */

#include <vector>
#include <string>
#include <cstdio>
#include <optional>

#include <client/graphical/device.hpp>

#include <fmt/format.h>

struct ParamInfo {
    int width = 640;
    int height = 480;

    FILE* log_device = stderr;

    std::string renderer = "opengl";
    std::vector<std::unique_ptr<familyline::graphics::Device>> devices;
    
    std::optional<std::string> mapFile;
    std::optional<std::string> inputFile;

    std::optional<std::string> serverAddress;
};

/**
 * Parse the parameter list, return the parameter info
 *
 * Note that, in some cases, this function will terminate
 * the program
 */
[[nodiscard]]
ParamInfo parse_params(const std::vector<std::string>& params);
