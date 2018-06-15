# drey-vm-2

Drey is a virtual machine for hosting networked modern board games.

Drey executes a bytecode format compiled by another language, typically [scurry](https://github.com/pezipink/scurry)

The virtual machine and project has (or will have) the following properties

* Everything is written in C from the ground up with no 3rd party libraries (for the fun of it) except ZeroMQ 
* Various garbage collection and memory allocation data structures and algorithms (subject to change)
- [x] Dynamic typing with object->object dictionaries as the core type
- [x] First class lambda functions, partial application and closures
- [x] Transparent networking via ZeroMQ
- [x] "Flowroutines" (remote co-routines) orchestrate collecting responses from clients, with an automatic rollback facility for multi-stage choices  (implemented by cloning callstacks)
- [x] Fibers allow a limited form of apparent concurrency allowing more than one client to act at once
- [x] Combined inter-process and external sockets with ZeroMQ.
- [ ] Record and playback facility 
- [ ] First class data visiblity features control what clients can see, mostly automatically
- [ ] Only supports windows at the moment due to threading, but will support some nix later.
- [ ] remote debugging support

Work in progress,  a toy project and a re-write of the not-finished [drey-vm](https://github.com/pezipink/drey-vm)
