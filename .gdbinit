file pastel_dbg.bin
target remote localhost:1234
layout src
# break *&_start
# break real_start
break loader_load_elf
