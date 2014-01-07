skiplist
========

C++ STL-style skiplist

I wrote this to find out if a skiplist was faster than the STL map, slightly
under a decade ago.

At the time, I recall it was - the current std::map in GCC appears to be
marginally faster. I suspect that C++14 improvements would let me add a bit
of speed to this - whether that'd be enough to get it into the lead I don't
know.

The tests don't do much iteration, because they're really benchmarks, and a
skiplist ought to be zillions of times faster than a red-black tree at
traversal.
