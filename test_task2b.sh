#!/bin/bash
DURATION=25

echo "=== Task 2B Test ==="

# Compile busy
cat > /tmp/busy.c << 'BUSY'
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
int main(int argc, char *argv[]) {
    int seconds = 10;
    if (argc > 1) seconds = atoi(argv[1]);
    time_t end = time(NULL) + seconds;
    volatile long x = 0;
    while (time(NULL) < end) { x++; }
    return 0;
}
BUSY

gcc -o /tmp/busy /tmp/busy.c
chmod +x /tmp/busy

# Create users
id alice &>/dev/null || useradd -m alice
id bob &>/dev/null || useradd -m bob

echo "Starting: alice=10 processes, bob=1 process"

# Start processes using su -s /bin/sh
for i in $(seq 1 10); do
    su -s /bin/sh alice -c "/tmp/busy $DURATION" &
done
su -s /bin/sh bob -c "/tmp/busy $DURATION" &

sleep 2
echo "Processes running:"
ps aux | grep busy | grep -v grep

echo "Monitoring..."
./monitor.exe $((DURATION - 3))

echo "=== Done ==="

