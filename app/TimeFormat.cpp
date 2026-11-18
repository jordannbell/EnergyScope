#include "TimeFormat.h"

#include <afxdisp.h>

namespace {
constexpr std::int64_t kMaxDisplayableTime = 32503680000;  // year 3000
}

CString formatUnixTime(std::int64_t unixTime, const wchar_t* format) {
    // CTime asserts on out-of-range values.
    if (unixTime < 0 || unixTime > kMaxDisplayableTime) {
        CString raw;
        raw.Format(L"%lld", unixTime);
        return raw;
    }
    return CTime(static_cast<__time64_t>(unixTime)).Format(format);
}
