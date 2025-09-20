#pragma once

#define HPS_VERSION_MAJOR 0
#define HPS_VERSION_MINOR 0
#define HPS_VERSION_PATCH 3
#define HPS_VERSION "0.0.3"

namespace hps {
constexpr int version_major = HPS_VERSION_MAJOR;
constexpr int version_minor = HPS_VERSION_MINOR;
constexpr int version_patch = HPS_VERSION_PATCH;
constexpr auto version_string = HPS_VERSION;
}
