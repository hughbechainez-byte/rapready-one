#pragma once

#include <array>

namespace rapready::parameters
{
inline constexpr auto amount = "amount";
inline constexpr auto filter = "filter";
inline constexpr auto expansion = "expansion";
inline constexpr auto compression = "compression";
inline constexpr auto deEss = "deess";
inline constexpr auto saturation = "saturation";
inline constexpr auto limiter = "limiter";
inline constexpr auto theme = "theme";

inline constexpr std::array<const char*, 6> stageIds{
    filter, expansion, compression, deEss, saturation, limiter,
};

inline constexpr std::array<const char*, 10> eqIds{
    "eq80", "eq160", "eq315", "eq630", "eq1250",
    "eq2500", "eq4000", "eq6300", "eq10000", "eq16000",
};

inline constexpr std::array<float, 10> eqFrequenciesHz{
    80.0f, 160.0f, 315.0f, 630.0f, 1250.0f,
    2500.0f, 4000.0f, 6300.0f, 10000.0f, 16000.0f,
};

inline constexpr std::array<const char*, 17> audioStateIds{
    amount,
    filter,
    expansion,
    compression,
    deEss,
    saturation,
    limiter,
    "eq80",
    "eq160",
    "eq315",
    "eq630",
    "eq1250",
    "eq2500",
    "eq4000",
    "eq6300",
    "eq10000",
    "eq16000",
};

using AudioSnapshot = std::array<float, audioStateIds.size()>;
} // namespace rapready::parameters
