This is the old and widely used memory bandwidth benchmark, with some minor
adjustments.

The original source code was downloaded from:
https://www.cs.virginia.edu/stream/

This version has been updated to provide a simple Makefile, errorcode exits,
a quieter output mode and the ability to specify the "N" value at runtime.

This code has been used to reproduce an issue found during hardware
qualification.  For large enough CPUs, the size of "N" required to
ensure that the array does not fit within the cache appears to always
cause a "Failed Validation" error:

e.g:
```
./stream_omp quiet 471443940    # works
./stream_omp quiet 471443941    # fails
```

```
Failed Validation on array c[]
        Expected  : 144991110792234369024.000000
        Observed  : 144991112242145492992.000000
```
