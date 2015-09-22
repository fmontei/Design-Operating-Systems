I. SETUP

Type the following commands into the terminal:

1) sudo apt-get install linux-headers-$(uname -r) build-essential
2) Must type sudo -s in terminal to have root privileges
3) Must initialize project in /root/

II. RUN PROGRAM

Type the following commands into the terminal: 

1) make
2) insmod mod1.ko

Confirm that the probe has been inserted by typing:

3) lsmod | head -n 5

You should see mod1 listed.

Type the following to see the message "Probe inserted  by Felipe":

4) dmesg

Next, trigger the Pre_Handler and Post_Handler by typing:

5) mkdir temp

Again, type:

6) dmesg

You should see the calls to Felipe's Pre_Handler and Felipe's Post_Handler
and a counter being incremented.

To remove the probe, type:

7) rmmod mod1.ko

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

VI. HELPFUL LINKS

a) Tutorial: http://opensourceforu.efytimes.com/2011/04/kernel-debugging-using-kprobe-and-jprobe/
b) struct pt_regs src code: http://lxr.free-electrons.com/source/arch/x86/include/asm/ptrace.h


