#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "utils.h"

struct proc_snapshot {
    pid_t pid;
    uid_t uid;
    unsigned long utime;
    unsigned long stime;
};

struct user_total {
    uid_t uid;
    char username[64];
    long long total_ticks;
};

static struct proc_snapshot initial_snapshot[MAX_PROCS];
static int initial_count = 0;

static struct user_total users[MAX_USERS];
static int user_count = 0;

static long clock_ticks;

/* Find or add user in users array */
static int find_or_add_user(uid_t uid)
{
    for (int i = 0; i < user_count; i++) {
        if (users[i].uid == uid)
            return i;
    }
    if (user_count >= MAX_USERS)
        return -1;
    
    users[user_count].uid = uid;
    users[user_count].total_ticks = 0;
    get_username(uid, users[user_count].username, sizeof(users[user_count].username));
    return user_count++;
}

/* Find initial snapshot for a PID */
static struct proc_snapshot *find_initial(pid_t pid)
{
    for (int i = 0; i < initial_count; i++) {
        if (initial_snapshot[i].pid == pid)
            return &initial_snapshot[i];
    }
    return NULL;
}

/* Take initial snapshot of all processes */
static void take_initial_snapshot(void)
{
    pid_t pids[MAX_PROCS];
    int count = get_all_pids(pids, MAX_PROCS);
    
    initial_count = 0;
    for (int i = 0; i < count && initial_count < MAX_PROCS; i++) {
        unsigned long utime, stime;
        uid_t uid = get_proc_uid(pids[i]);
        
        if (uid == (uid_t)-1)
            continue;
        if (read_proc_stat(pids[i], &utime, &stime) < 0)
            continue;
        
        initial_snapshot[initial_count].pid = pids[i];
        initial_snapshot[initial_count].uid = uid;
        initial_snapshot[initial_count].utime = utime;
        initial_snapshot[initial_count].stime = stime;
        initial_count++;
    }
}

/* Calculate final CPU usage */
static void calculate_final_usage(void)
{
    pid_t pids[MAX_PROCS];
    int count = get_all_pids(pids, MAX_PROCS);
    
    for (int i = 0; i < count; i++) {
        unsigned long utime, stime;
        uid_t uid = get_proc_uid(pids[i]);
        
        if (uid == (uid_t)-1)
            continue;
        if (read_proc_stat(pids[i], &utime, &stime) < 0)
            continue;
        
        /* Find initial values for this PID */
        struct proc_snapshot *init = find_initial(pids[i]);
        unsigned long init_utime = 0, init_stime = 0;
        
        if (init && init->uid == uid) {
            init_utime = init->utime;
            init_stime = init->stime;
        }
        
        /* Calculate delta */
        long long delta_utime = (long long)utime - (long long)init_utime;
        long long delta_stime = (long long)stime - (long long)init_stime;
        
        if (delta_utime < 0) delta_utime = 0;
        if (delta_stime < 0) delta_stime = 0;
        
        long long delta_total = delta_utime + delta_stime;
        
        if (delta_total > 0) {
            int idx = find_or_add_user(uid);
            if (idx >= 0) {
                users[idx].total_ticks += delta_total;
            }
        }
    }
}

/* Compare function for qsort (descending order) */
static int compare_users(const void *a, const void *b)
{
    const struct user_total *ua = (const struct user_total *)a;
    const struct user_total *ub = (const struct user_total *)b;
    
    if (ub->total_ticks > ua->total_ticks) return 1;
    if (ub->total_ticks < ua->total_ticks) return -1;
    return 0;
}

/* Print final summary */
static void print_summary(void)
{
    /* Sort users by CPU time (descending) */
    qsort(users, user_count, sizeof(struct user_total), compare_users);
    
    printf("Rank\tUser\t\tCPU Time (milliseconds)\n");
    printf("----------------------------------------\n");
    
    int rank = 1;
    for (int i = 0; i < user_count; i++) {
        if (users[i].total_ticks > 0) {
            /* Convert ticks to milliseconds */
            long long ms = (users[i].total_ticks * 1000) / clock_ticks;
            printf("%d\t%-15s\t%lld\n", rank++, users[i].username, ms);
        }
    }
}

int main(int argc, char *argv[])
{
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <seconds>\n", argv[0]);
        return 1;
    }
    
    int duration = atoi(argv[1]);
    if (duration <= 0) {
        fprintf(stderr, "Invalid duration: %s\n", argv[1]);
        return 1;
    }
    
    clock_ticks = get_clock_ticks();
    
    /* Take initial snapshot */
    take_initial_snapshot();
    
    /* Run for specified duration, sampling every second */
    for (int i = 0; i < duration; i++) {
        sleep(1);
    }
    
    /* Calculate final usage and print */
    calculate_final_usage();
    print_summary();
    
    return 0;
}
