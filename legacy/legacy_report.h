#ifndef LEGACY_REPORT_H
#define LEGACY_REPORT_H

/*
 * Per-site statistics report.
 *
 * csv: text with one measurement per line, "site,timestamp,kw".
 * out: receives one line per site:
 *      "<site>: n=<count> min=<kw> max=<kw> avg=<kw> kwh=<energy>\n"
 *
 * Returns the length of the text written to out, or -1 if outSize <= 0.
 */
int lr_compute_report(const char *csv, char *out, int outSize);

/* Forgets everything seen so far. */
void lr_reset(void);

#endif
