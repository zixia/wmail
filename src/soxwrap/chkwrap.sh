#!/bin/sh

# Sanity check to make sure that everything got wrapped.

nm $* | sed -n '
/U getpeername/p
/U getsockname/p
/U accept/p
/U recvfrom/p
/U connect/p
/U listen/p
/U select/p
/U bind/p
/U close/p
/U dup/p
/U sendto/p
/U read/p
/U write/p
'
