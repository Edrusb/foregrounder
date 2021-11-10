# Description
Foregrounder is a small C program to use as foreground of a daemon (or set of daemons) that cannot work in foreground.
The main/initial target was to support such daemon (or set of daemons) in a docker container 
so they can run in background, while foregrounder holds the container alive as process number 1.
Upon customer stop request, foregrounder will run the specified command to properly stop the daemons.

# Requirements
- Ansi C compiler (clang, gcc)
- linker (ld)
- the "make" program

# Compilation
run "make" 

# Usage
run "./foregrounder"
