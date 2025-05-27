# Process System with IPC Communication

🚦⚙️This project implements a multi-process system with interprocess communication (IPC). The system starts with a parent process (P) responsible for overall coordination, including spawning and terminating child processes (C1, C2, ... Ck) and sending messages.

---

### Parent Process (P):
Reads a command file with timestamped instructions to spawn (S) or terminate (T) child processes. It also accesses a large text file used as a message source.

### Child Processes:
Receive text lines randomly sent by P and print them to standard output. They remain blocked while waiting for messages.

### Communication Mechanisms:
Uses a semaphore array (size M, configured at start) to limit active children and coordinate communication. Each child process is assigned a semaphore for synchronization. Shared memory is used for exchanging data between P and children.

### Behavior:
In each cycle, P executes commands from the command file if any and sends a randomly selected text line to a randomly chosen active child. When terminating a child, P sends a special IPC message to signal exit. The parent collects exit codes and can reuse semaphores for new children.

### Child termination:
Upon termination, a child reports the number of messages received and total active time (based on timestamps).

### How to Run

```python
make
 . / parent <CF. txt> <text . txt> <M>
