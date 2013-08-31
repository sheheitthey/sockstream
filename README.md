sockstream
==========

A C++ iostream implemented with a socket.

I created this to demonstrate polymorphism of C++ iostreams, which I had
always heard of as an advantage but not seen examples of. Most of the work is
in sockstreambuf, to override the various buffer and I/O methods. Then
sockstream just uses the sockstreambuf by passing it to init. It also exposes
a few socket-specific operations.
