# WUROS
WUROS is a basic OS developed as a project for CSC 159 Operating System Pragmatics. The development team consists of William Guo, Rian Allen, Laxmi Soujanya Chaturvedula, and myself. 

# Environment
This environment is referred to as SPEDE (System Programmers Educational Development Environment). The SPEDE Virtual Machine is a 64-bit x86 Linux Operating System. 
SPEDE was originally developed at CSUS with contributions over a number of years from
professors, teachers, and students as well. Originally, this environment consisted of physical hardware to
develop and run code, but is now run using virtual machines and the environment has now been updated to use
modern C compilers, debuggers, and tools.

# Why SPEDE?
Developing an Operating System is not a simple task. It is a specialized discipline that requires deep
understanding of computer hardware and system architectures, and the ability to put computer science and
engineering skills into practice.
SPEDE helps facilitate some of the tedious steps to get from system power-up to runninng (and more importantly: debugging) the code.
Within the SPEDE Virtual Machine, all of the development tools needed to write code, compile,
debug, and run the operating system are available. It is a flexible environment that can be built upon and
developed further.

# Keyboard Shortcuts
Shortcut | Description
--- | ---
`ESC` | When pressed three times consecutively, the program will exit.
`ALT` + `0-9` | Switches to the selected TTY. 
`CTRL` + `-` | Reduces kernel log level.
`CTRL` + `+` | Increases kernel log level.
`CTRL` + `n` | Creates a new process.
`CTRL` + `q` | Deletes the current active process.

# Shell Commands
Command | Description
--- | --- | ---
`exit` | Exits the current process.
`lock` | Takes a mutex lock that may block other shells. 
`sleep` | Puts the process to sleep.
`time` | Displays the current system time.
