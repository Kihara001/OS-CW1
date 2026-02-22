#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "utils.h"

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <seconds>\n", argv[0]);
        return 1;
    }

    int duration = atoi(argv[1]);
    if (duration <= 0) {
        fprintf(stderr, "Duration must be a positive integer.\n");
        return 1;
    }

    long ticks_per_sec = sysconf(_SC_CLK_TCK);

    /* Initial snapshot: records baselines for all existing processes */
    scan_procs();

    /* Monitor for the specified duration, scanning once per second */
    for (int i = 0; i < duration; i++) {
        sleep(1);
        scan_procs();
    }

    /* Aggregate and sort results */
    UserResult *results;
    int count;
    collect_results(ticks_per_sec, &results, &count);
    qsort(results, count, sizeof(UserResult), cmp_results_desc);

    /* Print ranked output */
    printf("%-6s %-20s %s\n", "Rank", "User", "CPU Time (milliseconds)");
    printf("----------------------------------------\n");
    int rank = 1;
    for (int i = 0; i < count; i++) {
        if (results[i].cpu_ms == 0) continue;
        printf("%-6d %-20s %lld\n", rank++, results[i].username, results[i].cpu_ms);
    }

    free(results);
    cleanup_tracking();
    return 0;
}
