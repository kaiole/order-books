#pragma once

#include "output/latency_record.h"

#include <span>
#include <string_view>

void writeLatencies(std::string_view path,
                    std::span<const LatencyRecord> records);
