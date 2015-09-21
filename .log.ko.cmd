cmd_/root/kprobe/log.ko := ld -r -m elf_x86_64 -T ./scripts/module-common.lds --build-id  -o /root/kprobe/log.ko /root/kprobe/log.o /root/kprobe/log.mod.o
