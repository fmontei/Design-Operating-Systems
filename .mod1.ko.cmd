cmd_/root/kprobe/mod1.ko := ld -r -m elf_x86_64 -T ./scripts/module-common.lds --build-id  -o /root/kprobe/mod1.ko /root/kprobe/mod1.o /root/kprobe/mod1.mod.o
