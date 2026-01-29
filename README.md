# xv6-riscv: MLFQ Scheduler + `getpinfo()` + `ps`

This project extends **xv6-riscv** with:
- a new system call **`getpinfo(struct pstat*)`** that returns a snapshot of process information,
- an updated user program **`ps`** that prints that information,
- a replacement of the default scheduling behavior with a **Multi-Level Feedback Queue (MLFQ)** scheduler.

## Features

### 1) `getpinfo(struct pstat*)` system call
Adds a system call that fills a `struct pstat` with process information for all entries in the process table.

Implementation highlights:
- Takes a user pointer (`argaddr`) to a `struct pstat`.
- Builds a **local** `struct pstat st` in kernel space, zeroes it with `memset`.
- Locks to produce a **consistent snapshot**:
  - acquires `wait_lock`,
  - then (per process) acquires `p->lock`.
- Copies the snapshot back to user space with `copyout(...)`.
- Returns `0` on success, `-1` on failure.

Also includes a helper to convert `enum procstate` to a printable string (`state_to_str()`).

### 2) `ps` user program (updated)
`ps` now:
- declares `struct pstat st;`
- calls `getpinfo(&st)`
- prints a table with the columns:PID PPID PRIO STATE SZ NAME


Only entries with `st.inuse[i] == 1` are printed.

### 3) MLFQ Scheduler
Replaces the default scheduling policy with **4 priority levels** (0 highest → 3 lowest):

- The scheduler always prefers runnable processes from **level 0**, then **1**, **2**, **3**.
- Inside each level, it uses **Round-Robin**.
- Uses an array `last_proc_idx[4]` to keep fair RR rotation per level.

#### Per-process scheduling fields
Each `struct proc` includes:
- `mlfq_level`   : current queue level (0..3)
- `quantum_left` : remaining ticks in the current quantum
- `waitticks`    : ticks spent runnable but not running (used for promotion)

New processes start at:
- `mlfq_level = 0`
- `quantum_left = quantum_for_level(0)`
- `waitticks = 0`

#### Time quanta per level
`quantum_for_level(level)` maps:
- L0: 4 ticks
- L1: 8 ticks
- L2: 16 ticks
- L3: 32 ticks

#### Timer-tick behavior (preemption + demotion + promotion)
xv6 normally yields every timer tick; this project changes that logic so yielding follows MLFQ rules:

On each timer tick:
1. Update waiting time for other runnable processes via:
   - `mlfq_update_waitticks(running)`
   - increments `waitticks` for RUNNABLE processes that are **not** running
   - **promotes** a process when  
     `waitticks >= 10 * quantum(current_level)`
2. Decrement the running process’s quantum:
   - `running->quantum_left--`
3. If the quantum expires:
   - **demote** (down to level 3)
   - reset `quantum_left` for the new level
   - reset `waitticks`
   - `yield()`
4. If the quantum is not expired but a higher-priority runnable process exists:
   - `higher_level_runnable(cur_level)` → `yield()` (preemption)

## Build & Run

From the xv6-riscv root directory:

```bash
make qemu
```

Run ps inside xv6:
```bash
ps
```

### Notes

This is implemented on xv6-riscv.

The MLFQ policy here is intentionally simple and deterministic:
fixed queues (0..3), fixed quanta (4/8/16/32), promotion threshold tied to 10 * quantum(level).

# Author
Giorgos Theodorou


