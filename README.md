# PastelOS

PastelOS is my toy operating system for x86.
It's written from scratch in C99 and some assembly to glue it together.
Hopefully I will factor out all the x86-specific code at some point to make it platform-independent.

## The idea

Keep in mind that this is a toy OS, so I am making a lot of assumptions about how software is interacting with the kernel.
This also means that security may not always be a given.
I learn best by doing, so I will only discover the issues with my plans when I get there. :)

There's not much present yet, but this is a rough outline of how the project is going to be structured:

The kernel is a microkernel, i.e., most things actually happen in user-space. It only handles...

- [x] Physical memory management (it sucks, but it works)
- [x] Virtual memory management (ditto)
- [x] Scheduling
- [ ] IPC, via...
    - [ ] message passing, possibly akin to UNIX signals?
    - [ ] memory mapping
    - [ ] UNIX-style pipes (<3)
- [ ] File system management
    - [ ] Probably some ramdisk driver
- [ ] Access management for...
    - [ ] hardware (i.e., letting drivers claim hardware for themselves)
    - [ ] files
    - [ ] pipes
    - [ ] messages?

The user-space is where all the interesting bits are supposed to live:

- [ ] VGA textmode graphics (happening in the kernel right now)
- [ ] Keyboard input
- [ ] The actual file system drivers
    - The ramdisk driver (probably?) can't live here, since the kernel needs *some* help
      loading the rest of the system.
    - [ ] Probably FAT to start with.
