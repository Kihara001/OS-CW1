#ifndef UTILS_H
#define UTILS_H

#include <sys/types.h>

typedef struct {
    char username[64];
    long long cpu_ms;
} UserResult;

/* Scan /proc and record/update per-PID baselines and latest CPU ticks */
void scan_procs(void);

/* Aggregate per-user deltas and return sorted results. Caller frees *out. */
void collect_results(long ticks_per_sec, UserResult **out, int *count);

int cmp_results_desc(const void *a, const void *b);

void cleanup_tracking(void);

#endif
