#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <pwd.h>
#include <ctype.h>
#include "utils.h"

long get_clock_ticks(void)
{
    return sysconf(_SC_CLK_TCK);
}

int read_proc_stat(pid_t pid, unsigned long *utime, unsigned long *stime)
{
    char path[64];
    FILE *fp;
    char buf[1024];
    
    snprintf(path, sizeof(path), "/proc/%d/stat", pid);
    fp = fopen(path, "r");
    if (!fp)
        return -1;
    
    if (!fgets(buf, sizeof(buf), fp)) {
        fclose(fp);
        return -1;
    }
    fclose(fp);
    
    /* Find the closing parenthesis of comm field (field 2) */
    char *p = strrchr(buf, ')');
    if (!p)
        return -1;
    p += 2; /* Skip ") " */
    
    /* Now parse fields starting from field 3 */
    /* Fields: state(3), ppid(4), pgrp(5), session(6), tty_nr(7), tpgid(8),
       flags(9), minflt(10), cminflt(11), majflt(12), cmajflt(13),
       utime(14), stime(15) */
    char state;
    int ppid, pgrp, session, tty_nr, tpgid;
    unsigned long flags, minflt, cminflt, majflt, cmajflt;
    
    int ret = sscanf(p, "%c %d %d %d %d %d %lu %lu %lu %lu %lu %lu %lu",
                     &state, &ppid, &pgrp, &session, &tty_nr, &tpgid,
                     &flags, &minflt, &cminflt, &majflt, &cmajflt,
                     utime, stime);
    
    if (ret != 13)
        return -1;
    
    return 0;
}

uid_t get_proc_uid(pid_t pid)
{
    char path[64];
    struct stat st;
    
    snprintf(path, sizeof(path), "/proc/%d", pid);
    if (stat(path, &st) < 0)
        return (uid_t)-1;
    
    return st.st_uid;
}

void get_username(uid_t uid, char *buf, size_t buflen)
{
    struct passwd *pw = getpwuid(uid);
    if (pw) {
        strncpy(buf, pw->pw_name, buflen - 1);
        buf[buflen - 1] = '\0';
    } else {
        snprintf(buf, buflen, "%u", uid);
    }
}

int get_all_pids(pid_t *pids, int max_pids)
{
    DIR *dir;
    struct dirent *entry;
    int count = 0;
    
    dir = opendir("/proc");
    if (!dir)
        return -1;
    
    while ((entry = readdir(dir)) != NULL && count < max_pids) {
        /* Check if directory name is a number (PID) */
        int is_pid = 1;
        for (int i = 0; entry->d_name[i]; i++) {
            if (!isdigit(entry->d_name[i])) {
                is_pid = 0;
                break;
            }
        }
        if (is_pid && entry->d_name[0] != '\0') {
            pids[count++] = atoi(entry->d_name);
        }
    }
    
    closedir(dir);
    return count;
}
