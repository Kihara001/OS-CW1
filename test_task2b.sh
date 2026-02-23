#!/bin/bash
DURATION=40
MONITOR_TIME=25

echo "=== Task 2B Test ==="
echo ""

# Kill ALL old busy processes first
pkill busy 2>/dev/null
sleep 2

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

# Create users if needed
id alice &>/dev/null || useradd -m alice
id bob &>/dev/null || useradd -m bob

echo "UIDs: alice=$(id -u alice), bob=$(id -u bob)"
echo ""
echo "Test: alice=10 processes, bob=1 process"
echo "Duration: ${DURATION}s busy, ${MONITOR_TIME}s monitor"
echo ""

# Start bob FIRST
echo "Starting bob...200"
for i in $(seq 1 200); do
    su -s /bin/sh bob -c "/tmp/busy $DURATION" &
done


sleep 1

# Start alice
echo "Starting alice (200 processes)..."
for i in $(seq 1 200); do
    su -s /bin/sh alice -c "/tmp/busy $DURATION" &
done

# Wait for all to start
sleep 3

echo ""
echo "=== Process Check ==="
ALICE_PROCS=$(pgrep -u alice busy | wc -l)
BOB_PROCS=$(pgrep -u bob busy | wc -l)
echo "Running: alice=$ALICE_PROCS processes, bob=$BOB_PROCS processes"

if [ "$ALICE_PROCS" -lt 5 ] || [ "$BOB_PROCS" -lt 1 ]; then
    echo "ERROR: Not enough processes started!"
    echo "alice PIDs: $(pgrep -u alice busy)"
    echo "bob PIDs: $(pgrep -u bob busy)"
    exit 1
fi

echo ""
echo "=== Monitoring for ${MONITOR_TIME}s ==="
./monitor.exe $MONITOR_TIME

echo ""
echo "=== Final Process Check ==="
echo "Still running: alice=$(pgrep -u alice busy | wc -l), bob=$(pgrep -u bob busy | wc -l)"

echo ""
echo "=== Expected Results ==="
echo "Task 2B SUCCESS: alice and bob have SIMILAR CPU times (~50% each)"
echo "Task 2B FAIL:    alice has ~10x more than bob"
echo ""
