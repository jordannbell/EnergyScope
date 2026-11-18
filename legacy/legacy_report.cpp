/*
 * legacy_report.cpp
 * Per-site statistics report. Written in 2001 as a "temporary" tool.
 * Still used. Deliberately kept in the style of old code: see README.md.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "legacy_report.h"

#define MAXS 16
#define BUFSZ 65536

struct SiteStat {
    char name[32];
    int n;
    double mn, mx, sum, kwh;
    long lastTs;
    double lastKw;
};

static struct SiteStat g_sites[MAXS];
static int g_ns = 0;
static char g_buf[BUFSZ];

static int find_site(const char *name)
{
    int i;
    for (i = 0; i < g_ns; i++) {
        if (strcmp(g_sites[i].name, name) == 0)
            return i;
    }
    return -1;
}

void lr_reset(void)
{
    g_ns = 0;
}

int lr_compute_report(const char *csv, char *out, int outSize)
{
    char *line;
    int i;
    char tmp[512];

    if (outSize <= 0)
        return -1;
    out[0] = 0;

    strncpy(g_buf, csv, BUFSZ - 1);
    g_buf[BUFSZ - 1] = 0;

    line = strtok(g_buf, "\n");
    while (line != NULL) {
        char *c1 = strchr(line, ',');
        if (c1 != NULL) {
            char *c2 = strchr(c1 + 1, ',');
            if (c2 != NULL) {
                char name[64];
                int len = (int)(c1 - line);
                long ts;
                double kw;
                int idx;

                if (len > 31)
                    len = 31;
                strncpy(name, line, len);
                name[len] = 0;
                ts = atol(c1 + 1);
                kw = atof(c2 + 1);

                idx = find_site(name);
                if (idx < 0 && g_ns < MAXS) {
                    idx = g_ns++;
                    strcpy(g_sites[idx].name, name);
                    g_sites[idx].n = 0;
                    g_sites[idx].mn = kw;
                    g_sites[idx].mx = kw;
                    g_sites[idx].sum = 0;
                    g_sites[idx].kwh = 0;
                }
                if (idx >= 0) {
                    struct SiteStat *s = &g_sites[idx];
                    if (s->n > 0) {
                        long dt = ts - s->lastTs;
                        s->kwh += s->lastKw * dt / 3600.0;
                    }
                    s->n++;
                    if (kw < s->mn)
                        s->mn = kw;
                    if (kw > s->mx)
                        s->mx = kw;
                    s->sum += kw;
                    s->lastTs = ts;
                    s->lastKw = kw;
                }
            }
        }
        line = strtok(NULL, "\n");
    }

    for (i = 0; i < g_ns; i++) {
        sprintf(tmp, "%s: n=%d min=%.2f max=%.2f avg=%.2f kwh=%.2f\n",
                g_sites[i].name, g_sites[i].n, g_sites[i].mn, g_sites[i].mx,
                g_sites[i].sum / g_sites[i].n, g_sites[i].kwh);
        if ((int)strlen(out) + (int)strlen(tmp) < outSize)
            strcat(out, tmp);
    }
    return (int)strlen(out);
}
