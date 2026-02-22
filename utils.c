#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <ctype.h>
#include <sys/stat.h>
#include <pwd.h>
#include "utils.h"

/* --- Per-PID tracking --- */

typedef struct PidInfo {
    int pid;
    uid_t uid;
    long long baseline;
    long long latest;
    struct PidInfo *next;
} PidInfo;

#define PID_BUCKETS 4096
static PidInfo *pid_table[PID_BUCKETS];

static PidInfo *pid_find(int pid) {
    int idx = (unsigned)pid % PID_BUCKETS;
    for (PidInfo *p = pid_table[idx]; p; p = p->next)
        if (p->pid == pid) return p;
    return NULL;
}

static void pid_insert(int pid, uid_t uid, long long ticks) {
    int idx = (unsigned)pid % PID_BUCKETS;
    PidInfo *p = malloc(sizeof(PidInfo));
    p->pid = pid;
    p->uid = uid;
    p->baseline = ticks;
    p->latest = ticks;
    p->next = pid_table[idx];
    pid_table[idx] = p;
}

/* --- Helpers --- */

static long long read_cpu_ticks(int pid) {
    char path[64];
    snprintf(path, sizeof(path), "/proc/%d/stat", pid);
    FILE *f = fopen(path, "r");
    if (!f) return -1;

    char buf[2048];
    if (!fgets(buf, sizeof(buf), f)) { fclose(f); return -1; }
    fclose(f);

    /* Find last ')' to skip comm field safely */
    char *p = strrchr(buf, ')');
    if (!p) return -1;
    p += 2; /* skip ") " */

    /* Fields after comm: state(3), ppid(4), pgrp(5), session(6), tty_nr(7),
       tpgid(8), flags(9), minflt(10), cminflt(11), majflt(12), cmajflt(13),
       utime(14), stime(15) */
    unsigned long utime, stime;
    if (sscanf(p, "%*c %*d %*d %*d %*d %*d %*u %*u %*u %*u %*u %lu %lu",
               &utime, &stime) != 2)
        return -1;
    return (long long)utime + (long long)stime;
}

static uid_t get_proc_uid(int pid) {
    char path[64];
    struct stat st;
    snprintf(path, sizeof(path), "/proc/%d", pid);
    if (stat(path, &st) != 0) return (uid_t)-1;
    return st.st_uid;
}

/* --- Public API --- */

void scan_procs(void) {
    DIR *proc = opendir("/proc");
    if (!proc) return;

    struct dirent *ent;
    while ((ent = readdir(proc))) {
        /* Only numeric entries are PIDs */
        char *c = ent->d_name;
        while (*c && isdigit((unsigned char)*c)) c++;
        if (*c != '\0') continue;

        int pid = atoi(ent->d_name);
        long long ticks = read_cpu_ticks(pid);
        if (ticks < 0) continue;

        PidInfo *info = pid_find(pid);
        if (info) {
            info->latest = ticks;
        } else {
            uid_t uid = get_proc_uid(pid);
            if (uid == (uid_t)-1) continue;
            pid_insert(pid, uid, ticks);
        }
    }
    closedir(proc);
}

/* Per-UID accumulator */
typedef struct UidAcc {
    uid_t uid;
    long long ticks;
    struct UidAcc *next;
} UidAcc;

#define UID_BUCKETS 256

void collect_results(long ticks_per_sec, UserResult **out, int *count) {
    UidAcc *uid_table[UID_BUCKETS];
    memset(uid_table, 0, sizeof(uid_table));
    int n_users = 0;

    /* Sum (latest - baseline) per UID */
    for (int i = 0; i < PID_BUCKETS; i++) {
        for (PidInfo *p = pid_table[i]; p; p = p->next) {
            long long delta = p->latest - p->baseline;
            if (delta <= 0) continue;

            int idx = (unsigned)p->uid % UID_BUCKETS;
            UidAcc *u;
            for (u = uid_table[idx]; u; u = u->next) {
                if (u->uid == p->uid) break;
            }
            if (u) {
                u->ticks += delta;
            } else {
                u = malloc(sizeof(UidAcc));
                u->uid = p->uid;
                u->ticks = delta;
                u->next = uid_table[idx];
                uid_table[idx] = u;
                n_users++;
            }
        }
    }

    /* Build results array */
    UserResult *res = malloc(n_users * sizeof(UserResult));
    int ri = 0;
    for (int i = 0; i < UID_BUCKETS; i++) {
        UidAcc *u = uid_table[i];
        while (u) {
            struct passwd *pw = getpwuid(u->uid);
            if (pw)
                strncpy(res[ri].username, pw->pw_name, 63);
            else
                snprintf(res[ri].username, 63, "%u", (unsigned)u->uid);
            res[ri].username[63] = '\0';
            res[ri].cpu_ms = (u->ticks * 1000) / ticks_per_sec;
            ri++;
            UidAcc *next = u->next;
            free(u);
            u = next;
        }
    }

    *out = res;
    *count = n_users;
}

int cmp_results_desc(const void *a, const void *b) {
    const UserResult *ra = a, *rb = b;
    if (rb->cpu_ms > ra->cpu_ms) return 1;
    if (rb->cpu_ms < ra->cpu_ms) return -1;
    return strcmp(ra->username, rb->username);
}

void cleanup_tracking(void) {
    for (int i = 0; i < PID_BUCKETS; i++) {
        PidInfo *p = pid_table[i];
        while (p) { PidInfo *next = p->next; free(p); p = next; }
        pid_table[i] = NULL;
    }
}
