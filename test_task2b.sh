#!/bin/bash
DURATION=30
MONITOR_TIME=20

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

# Show UIDs
echo "UIDs: alice=$(id -u alice), bob=$(id -u bob)"

echo "Starting: alice=10 processes, bob=1 process (duration=${DURATION}s)"

# Start bob FIRST
su -s /bin/sh bob -c "/tmp/busy $DURATION" &
BOB_PID=$!
sleep 1

# Start alice's processes
for i in $(seq 1 10); do
    su -s /bin/sh alice -c "/tmp/busy $DURATION" &
done

# Wait for processes to actually start
sleep 3

echo ""
echo "Processes running:"
ps aux | grep busy | grep -v grep
echo ""

ALICE_COUNT=$(ps aux | grep busy | grep alice | grep -v grep | wc -l)
BOB_COUNT=$(ps aux | grep busy | grep bob | grep -v grep | wc -l)
echo "Count: alice=$ALICE_COUNT, bob=$BOB_COUNT"

if [ "$ALICE_COUNT" -eq 0 ] || [ "$BOB_COUNT" -eq 0 ]; then
    echo "ERROR: Not all processes started!"
    exit 1
fi

echo ""
echo "Monitoring for ${MONITOR_TIME}s..."
./monitor.exe $MONITOR_TIME

echo ""
echo "=== Done ==="
echo "Expected: alice and bob should have SIMILAR CPU times if Task 2B works"
