#!/bin/bash
# Task 2B Test Script - All in one
# Usage: ./test_task2b.sh

set -e

DURATION=20

echo "=== Task 2B: Equitable Scheduling Test ==="

# Create busy.c
cat > /tmp/busy.c << 'BUSY_EOF'
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int main(int argc, char *argv[])
{
    int seconds = 10;
    if (argc > 1)
        seconds = atoi(argv[1]);
    
    time_t end = time(NULL) + seconds;
    volatile long x = 0;
    
    while (time(NULL) < end) {
        x++;
    }
    
    return 0;
}
BUSY_EOF

# Compile busy
gcc -o /tmp/busy /tmp/busy.c

# Create test users (if they don't exist)
id alice &>/dev/null || useradd -m alice
id bob &>/dev/null || useradd -m bob

echo ""
echo "Test setup:"
echo "  alice: 10 CPU-intensive processes"
echo "  bob:   1 CPU-intensive process"
echo "  Duration: ${DURATION} seconds"
echo ""
echo "Expected result (Task 2B working):"
echo "  Both users should have ~50% CPU time each"
echo ""
echo "Starting test..."

# Start alice's processes (10)
for i in $(seq 1 10); do
    su alice -c "/tmp/busy $DURATION" &
done

# Start bob's process (1)
su bob -c "/tmp/busy $DURATION" &

# Wait a moment for processes to start
sleep 1

# Show process count
echo ""
echo "Running processes:"
echo "  alice: $(pgrep -u alice | wc -l) processes"
echo "  bob:   $(pgrep -u bob | wc -l) processes"
echo ""

# Monitor CPU usage
echo "Monitoring for $DURATION seconds..."
echo ""

# Simple monitoring using /proc
sleep $((DURATION - 1))

# Wait for all background processes
wait 2>/dev/null

# Get results using /proc/stat info or our monitor
echo ""
echo "=== Results ==="

# If monitor.exe exists, use it
if [ -f "./monitor.exe" ]; then
    # Run another quick test with monitor
    for i in $(seq 1 10); do
        su alice -c "/tmp/busy 10" &
    done
    su bob -c "/tmp/busy 10" &
    ./monitor.exe 10
else
    echo "monitor.exe not found - showing basic stats"
    echo ""
    # Show CPU time from /proc
    for user in alice bob; do
        total=0
        for pid in $(pgrep -u $user 2>/dev/null); do
            if [ -f /proc/$pid/stat ]; then
                times=$(awk '{print $14+$15}' /proc/$pid/stat 2>/dev/null || echo 0)
                total=$((total + times))
            fi
        done
        echo "$user: $total ticks"
    done
fi

echo ""
echo "=== Test Complete ==="
echo ""
echo "If alice and bob have similar CPU times, Task 2B is working!"
echo "If alice has ~10x more than bob, Task 2B is NOT working."

# Cleanup
rm -f /tmp/busy /tmp/busy.c
