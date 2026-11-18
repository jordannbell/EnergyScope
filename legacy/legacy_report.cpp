// Old C-style API, kept so existing callers keep compiling. All the work is
// done by energy::legacy::ReportBuilder.

#include "legacy_report.h"

#include <cstddef>
#include <cstring>
#include <string>

#include "report_builder.hpp"

namespace {

// Still shared by every call, exactly like the old global arrays: results
// accumulate between calls. (Kept for now so the refactoring changes nothing;
// the characterization tests pin this.)
energy::legacy::ReportBuilder g_builder;

}  // namespace

void lr_reset(void) { g_builder.reset(); }

int lr_compute_report(const char *csv, char *out, int outSize) {
    if (out == nullptr || outSize <= 0) return -1;

    g_builder.addCsv(csv != nullptr ? csv : "");

    // A site line that does not fit in the caller's buffer is dropped; a
    // later, shorter line may still fit (old behaviour).
    std::string result;
    for (const std::string &line : g_builder.siteLines()) {
        if (result.size() + line.size() < static_cast<std::size_t>(outSize)) {
            result += line;
        }
    }
    std::memcpy(out, result.c_str(), result.size() + 1);
    return static_cast<int>(result.size());
}
