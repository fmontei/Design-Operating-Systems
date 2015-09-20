cmd_/root/kprobe/sysmon_uid.ko := ld -r -m elf_x86_64 -T ./scripts/module-common.lds --build-id  -o /root/kprobe/sysmon_uid.ko /root/kprobe/sysmon_uid.o /root/kprobe/sysmon_uid.mod.o
