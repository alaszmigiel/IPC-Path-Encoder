# IPC File Processor

This project demonstrates an **Inter-Process Communication (IPC) system** implementing a **Producer-Consumer** model utilizing **shared memory** and **semaphores** to enable communication between multiple processes.

Program consists of three main processes:

- **Process 1:** Reads a directory path from the user and sends file paths to Process 2.
- **Process 2:** Encodes received paths into a hexadecimal format and forwards them to Process 3.
- **Process 3:** Stores the received encoded paths in a file.

### Signal Controls

Through **signal handling**, the user can:
- **Terminate** the entire application (`SIGUSR1`) by sending it to **any process**.
- **Pause** (`SIGQUIT`) and **resume** (`SIGCONT`) execution, sent **to worker processes**.
- **Enable or disable encoding** (`SIGUSR2`), sent **to worker processes**.

## Installation & Usage

### 1. Clone the repository:
```sh
git clone https://github.com/alaszmigiel/IPC-File-Processor.git
cd IPC-File-Processor
```

### 2. Compile the program:
```sh
make
```

### 3. Run the program:
```sh
make run
```

### 4. Control execution with signals:

Open a **new terminal window** and use the following commands to control the process:

```sh
kill -SIGUSR1 <pid>   # Terminate the entire application (can be sent to any process)
kill -SIGQUIT <worker_pid>  # Pause execution (sent to worker processes)
kill -SIGCONT <worker_pid>  # Resume execution (sent to worker processes)
kill -SIGUSR2 <worker_pid>  # Toggle encoding mode (sent to worker processes)
```

### 5. Clean up (Remove compiled files):

To remove all generated object files (`.o`) and binaries, run:

```sh
make clean
```

## System Requirements

This program is designed to run on **Linux** or **macOS**. 
If you are using **Windows**, you need to run it inside a **Linux virtual machine** (e.g., using WSL or VirtualBox).
