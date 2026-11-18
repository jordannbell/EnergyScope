#pragma once

#include <afxwin.h>

#include <cstdint>

// Unix time (seconds since 1970) -> text in local time, using a strftime
// style format such as L"%Y-%m-%d %H:%M". Values CTime cannot represent are
// shown as the raw number instead of crashing.
CString formatUnixTime(std::int64_t unixTime, const wchar_t* format);
