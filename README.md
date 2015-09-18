# CASTLE
This repository contains implementation of a lock-based internal Binary Search Tree.<br>
The algorithm is described in our paper *CASTLE: Fast Concurrent Internal Binary Search Tree using Edge-Based Locking* published in PPoPP'15<br>
The technical report is available in the papers directory<br>
Binary file is in bin directory<br>
Source files are in src directory<br>
How to compile?<br>
1. Change the Makefile appropriately and run make<br>
How to run?<br>
`$./bin/castle.o numOfThreads readPercentage insertPercentage deletePercentage durationInSeconds maximumKeySize initialSeed hotKeyFrequency constantFactor sortPercent keySpread keyDistribution distributionParameter`<br>
Example: `$./bin/castle.o 64 70 20 10 1 10000 0 100 1 0 s z 1.0`<br>
Output is a semicolon separated line with the last entry denoting throughput in millions of operations per second<br>
Required Libraries:<br>
* GSL to create random numbers
* Intel "Threading Building Blocks(TBB)" atomic library(freely available)
* C++ std library can be used by commenting out "#define TBB" in header.h file
* Some memory allocator like jemalloc, tcmalloc, tbbmalloc,etc

Any questions?

contact - arunmoezhi at gmail dot com
