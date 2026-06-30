#pragma once
// Include the CMake-generated version.h if available; otherwise fall back to
// static defaults so the header is always usable.
#if __has_include("rv1126b/version.h")
#  include "rv1126b/version.h"
#else
#  define RV1126B_SDK_VERSION_MAJOR    1
#  define RV1126B_SDK_VERSION_MINOR    0
#  define RV1126B_SDK_VERSION_PATCH    0
#  define RV1126B_SDK_VERSION_STRING   "1.0.0"
#endif
