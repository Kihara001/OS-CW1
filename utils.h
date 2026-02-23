#ifndef UTILS_H
#define UTILS_H

#include <sys/types.h>

#define MAX_USERS 1024
#define MAX_PROCS 32768

struct proc_info {
    pid_t pid;
    uid_t uid;
    unsigned long utime;
    unsigned long stime;
};

struct user_cpu {
    uid_t uid;
    char username[64];
    long long cpu_time_ms;
};

/* Read CPU times (utime, stime) from /proc/<pid>/stat */
int read_proc_stat(pid_t pid, unsigned long *utime, unsigned long *stime);

/* Get UID of a process */
uid_t get_proc_uid(pid_t pid);

/* Get username from UID */
void get_username(uid_t uid, char *buf, size_t buflen);

/* Get all PIDs from /proc */
int get_all_pids(pid_t *pids, int max_pids);

/* Get clock ticks per second */
long get_clock_ticks(void);

#endif /* UTILS_H */
