Enhanced Containers
===================

This project actually defines alternate C++ allocators and allocator wrappers
for STL containers.


## No Swap Allocator

This allocator will lock allocated memory to RAM so that it cannot be swapped
out to disk.

## Secure Allocator

This is really just a combination fo the No Swap Allocator and the Zero On
Release Allocator defined in such a way as to ensure that memory gets zeroed out
before the memory pages are unlocked and can be swapped out to disk again.  This
helps prevent accidental data leaks.  This allocator is intended for containers
that store things like passwords and private encryption keys.

## Zero On Release Allocator

This allocator will zero out the the allocated memory when that memory is being
released.  This combined with the No Swap Allocator (as done in the Secure
Allocator) will help prevent data leaks.
