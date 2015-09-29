Files:
a) sysmon.c -- module
b) to_file.c -- user-space program that writes string provided to it via
   run.sh to a file called sysmon.backup
c) sysmon.backup -- all logging saved here
d) /proc/sysmon_log -- temporary log (cleared every one second)
e) /proc/sysmon_uid -- UID to track
f) /proc/sysmon_toggle -- toggle logging on/off
   (in addition, run.sh internally passes "flush" to /proc/sysmon_toggle to
    tell the module to clear sysmon_log)
g) rogue.c -- a rogue user-space program that opens a test.dat file and writes messages iteratively to that file. Most function calls included are system calls. 

I. SETUP

Type the following commands into the terminal:

1) sudo apt-get install linux-headers-$(uname -r) build-essential
2) Must type sudo -s in terminal to have root privileges
3) Must initialize project in /root/

Note: modify lines 43 and 44 in run.sh to the directory in which these
files are placed. Specifically, change:

line 43: cd /root/Design-Operating-Systems/
line 44: make -C /root/Design-Operating-Systems/

II. RUN PROGRAM - USING SCRIPT
A convenient script has been provided to make running this module easy.

Make sure you have privileges to run run.sh. If not, type: 
chmod u+x run.sh

Afterward, type the following:
1) sudo ./run.sh
2) If a log file (sysmon.backup) already exists, you will be prompted to 
   delete it (to create a new one) or keep it (and add new entries to it).
3) Type in a valid UID: e.g., 0 (root) or 1000.
4) ctrl+c to terminate the module and see the contents of sysmon.backup.

III. PRINT OUT LOG -- SYSMON_LOG PROC FILE

1) Make sure you are superuser (sudo -s)
2) Go to /proc directory
3) Type cat sysmon_log (or alternatively, gedit sysmon_log)

IV. DISABLE/ENABLE LOGGING -- SYSMON_TOGGLE PROC FILE (DEFAULT is enabled)

1) Make sure you are superuser (sudo -s)
2) Go to /proc directory
3) Type echo 0 > sysmon_toggle to disable logging
4) Type echo 1 > sysmon_toggle to enable logging
5) Typing dmesg shows a message indicating whether logging was enabled/disabled

V. CHANGE UID -- SYSMON_UID PROC FILE

1) Make sure you are superuser (sudo -s)
2) Go to /proc directory
3) Type echo uid > sysmon_uid to set current uid to uid
   (To find out what your uid is, type id -u <username> or simply id for the current user's id)
4) Typing dmesg shows a message indicating whether current uid has been changed

VI. RUN the rogue program

1) Make sure you are superuser (sudo -s)
2) Go to /root directory
3) Make sure no test.dat exists in the directory
3) Type gcc rogue.c -o rogue to compile
4) Type ./rogue to run
5) rm test.dat

When you also run our interposer (if rogue is run in the current directory, UID will be 0) to keep track of the log, you can see tons of write() system call (along with the PID for rogue program) printed out during the execution of rogue.
Example when PID is 5290 (more info in the project description):
RAX: 1 (__NR_write), Arg0: 3, Arg1: 6295648, UID: 0, PID: 5290, TGID: 5290
RAX: 1 (__NR_write), Arg0: 3, Arg1: 6295648, UID: 0, PID: 5290, TGID: 5290
RAX: 1 (__NR_write), Arg0: 3, Arg1: 6295648, UID: 0, PID: 5290, TGID: 5290
RAX: 1 (__NR_write), Arg0: 3, Arg1: 6295648, UID: 0, PID: 5290, TGID: 5290
RAX: 1 (__NR_write), Arg0: 3, Arg1: 6295648, UID: 0, PID: 5290, TGID: 5290
RAX: 1 (__NR_write), Arg0: 3, Arg1: 6295648, UID: 0, PID: 5290, TGID: 5290
RAX: 1 (__NR_write), Arg0: 3, Arg1: 6295648, UID: 0, PID: 5290, TGID: 5290
RAX: 1 (__NR_write), Arg0: 3, Arg1: 6295648, UID: 0, PID: 5290, TGID: 5290
RAX: 1 (__NR_write), Arg0: 3, Arg1: 6295648, UID: 0, PID: 5290, TGID: 5290
RAX: 1 (__NR_write), Arg0: 3, Arg1: 6295648, UID: 0, PID: 5290, TGID: 5290
RAX: 1 (__NR_write), Arg0: 3, Arg1: 6295648, UID: 0, PID: 5290, TGID: 5290
RAX: 1 (__NR_write), Arg0: 3, Arg1: 6295648, UID: 0, PID: 5290, TGID: 5290
RAX: 1 (__NR_write), Arg0: 3, Arg1: 6295648, UID: 0, PID: 5290, TGID: 5290
RAX: 1 (__NR_write), Arg0: 3, Arg1: 6295648, UID: 0, PID: 5290, TGID: 5290
RAX: 1 (__NR_write), Arg0: 3, Arg1: 6295648, UID: 0, PID: 5290, TGID: 5290
RAX: 1 (__NR_write), Arg0: 3, Arg1: 6295648, UID: 0, PID: 5290, TGID: 5290
RAX: 1 (__NR_write), Arg0: 3, Arg1: 6295648, UID: 0, PID: 5290, TGID: 5290
RAX: 1 (__NR_write), Arg0: 3, Arg1: 6295648, UID: 0, PID: 5290, TGID: 5290
RAX: 1 (__NR_write), Arg0: 3, Arg1: 6295648, UID: 0, PID: 5290, TGID: 5290
RAX: 1 (__NR_write), Arg0: 3, Arg1: 6295648, UID: 0, PID: 5290, TGID: 5290
RAX: 1 (__NR_write), Arg0: 3, Arg1: 6295648, UID: 0, PID: 5290, TGID: 5290
RAX: 1 (__NR_write), Arg0: 3, Arg1: 6295648, UID: 0, PID: 5290, TGID: 5290
RAX: 1 (__NR_write), Arg0: 3, Arg1: 6295648, UID: 0, PID: 5290, TGID: 5290
RAX: 1 (__NR_write), Arg0: 3, Arg1: 6295648, UID: 0, PID: 5290, TGID: 5290
RAX: 1 (__NR_write), Arg0: 3, Arg1: 6295648, UID: 0, PID: 5290, TGID: 5290
RAX: 1 (__NR_write), Arg0: 3, Arg1: 6295648, UID: 0, PID: 5290, TGID: 5290
RAX: 1 (__NR_write), Arg0: 3, Arg1: 6295648, UID: 0, PID: 5290, TGID: 5290
RAX: 1 (__NR_write), Arg0: 3, Arg1: 6295648, UID: 0, PID: 5290, TGID: 5290
RAX: 1 (__NR_write), Arg0: 3, Arg1: 6295648, UID: 0, PID: 5290, TGID: 5290
RAX: 1 (__NR_write), Arg0: 3, Arg1: 6295648, UID: 0, PID: 5290, TGID: 5290
RAX: 1 (__NR_write), Arg0: 3, Arg1: 6295648, UID: 0, PID: 5290, TGID: 5290
RAX: 1 (__NR_write), Arg0: 3, Arg1: 6295648, UID: 0, PID: 5290, TGID: 5290
RAX: 1 (__NR_write), Arg0: 3, Arg1: 6295648, UID: 0, PID: 5290, TGID: 5290
RAX: 1 (__NR_write), Arg0: 3, Arg1: 6295648, UID: 0, PID: 5290, TGID: 5290
RAX: 1 (__NR_write), Arg0: 3, Arg1: 6295648, UID: 0, PID: 5290, TGID: 5290

VII. HELPFUL LINKS

a) Tutorial: http://opensourceforu.efytimes.com/2011/04/kernel-debugging-using-kprobe-and-jprobe/
b) struct pt_regs src code: http://lxr.free-electrons.com/source/arch/x86/include/asm/ptrace.h
c) http://ecee.colorado.edu/~siewerts/extra/code/example_code_archive/a102_code/EXAMPLES/Cooperstein-Drivers/s_14/lab4_proc_sig_solB.c


