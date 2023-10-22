#pragma once

#include "config-utils/shared/config-utils.hpp"
#include <vector>
#include <string>

DECLARE_CONFIG(ModConfig,
    CONFIG_VALUE(History, std::vector<std::string>, "History of played hashes", {});
    CONFIG_VALUE(FilterDuplicates, bool, "Filter duplicates", false);
    CONFIG_VALUE(Length, int, "Max History Length (after next map)", 100);
)