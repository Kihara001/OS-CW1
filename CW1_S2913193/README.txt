Solution for Task 1
1. Complete Crash Prevention (Fallback Method)
Instead of performing risky queue re-searches or manual task evictions during a conflict, the scheduler simply returns NULL to delegate the process to the kernel's standard idle handler. This completely eliminates division-by-zero and null pointer errors.

2. Livelock Resolution (Peterson's Algorithm)
By requiring an "Intent" flag declaration before execution and using the CPU ID as a tie-breaker for simultaneous wake-ups, the implementation achieves robust mutual exclusion and prevents freezes (livelocks) caused by CPUs constantly yielding to each other.

Please look at pick_next_task_fair function in fair.c for details



Solution for Task 2A
The solution involves implementing a user-space monitoring program that measures the total CPU time consumed by each user over a specified period and outputs a ranked summary. The process consists of four main steps:

Initial Snapshot: At startup, scan all running processes and record their UID along with the initial CPU time (utime and stime) in an array.

Wait Duration: Pause the program for the duration specified by the command-line argument, using a loop that sleeps for 1 second in each iteration.

Calculate and Aggregate: After the specified duration, retrieve the CPU time for all processes again, calculate the difference from the initial snapshot, and accumulate the total CPU usage for each user (UID).

Sort and Output: Convert the accumulated CPU time (clock ticks) into milliseconds, sort the users in descending order based on their CPU usage, and print the final ranking.


Solution for Task 2B:
The solution involves shifting the fairness criterion of the Completely Fair Scheduler (CFS) from a "per-task" basis to a "per-user (UID)" basis. Here is a practical approach using the "dynamic adjustment of task weights or virtual runtimes":

1. Tracking User State (Data Structures):
Introduce a global array or hash table in fair.c to track the "cumulative CPU time" and the "number of runnable tasks" for each UID (≥ 1000).

2. Time Accounting:
Modify update_curr(), the core function where CFS calculates task execution time. When a task's executed time (delta_exec) is updated, add this value to the cumulative CPU time of the user who owns that task.

3. Enforcing Fairness (Scheduling Intervention):
Intervene in the CFS task selection process using one of the following methods to ensure equitable CPU distribution:

Dynamic vruntime Scaling: When updating a task's vruntime in update_curr(), scale it by multiplying it by the user's runnable task count or a penalty factor derived from their cumulative CPU time.  This rapidly increases the vruntime for users running many tasks, effectively pushing their tasks to the right side of the scheduler's Red-Black Tree.

4. Fulfilling the "No Unnecessary Throttling" Requirement:
Because this approach dynamically adjusts scheduling weights or virtual runtimes instead of forcefully sleeping or blocking tasks, it naturally allows all runnable tasks to fully utilize the CPU without restriction when there is no CPU contention.


Please look at update_curr function in fair.c for details. 

