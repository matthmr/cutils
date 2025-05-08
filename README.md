# cutils

Common utils C utilities used in many of my projects.

## mem/

Memory utitlies.

### dat

Memory utilities as to data: sized and incremental strings, sized and/or typed
generic memory, bit-fields, global register-type aliases (`uint`, `uchar`, ...), 
linked list types &c.

### mgr

A simple memory manager. Implements particular `mm_freek` function to the whole
memory of some pointer but keep some other given an offset.

This MM tries to keep allocations as close as possible, favoring smaller memory
addresses.

Also implements a crude refcounter for those who need with `mm_use`.

## err

Error and panic framework.

## macros

Useful miscellaneous macros plus error control macros (`__defer` and such).

## debug

Debugging messages. Can be toggled in compilation by `-DDEBUG`.
