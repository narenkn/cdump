# C-Dump
This utility can dump your C/C++ variables into FSDB. The advantage is that one can debug easier.

## Interface
```c++
#define cDUMP_PushHier(hier)
#define cDUMP_PopHier()
#define cDUMP_PushArray(hier, sz)
#define cDUMP_PopArray()
#define cDUMP_int64(var, ...)
#define cDUMP_int32(var, ...)
#define cDUMP_int16(var, ...)
#define cDUMP_int8(var, ...)
#define cDUMP_char(var, ...)
#define cDUMP_bool(var, ...)
#define cDUMP_float(var, ...)
#define cDUMP_double(var, ...)
#define cDUMP_var(var, ...)
#define cDUMP_Arr1D(var)
```

## Example
The code was plucked out of existing code. I haven't had the opportunity to create a working example. I would be very happy you could do it. Let me know if you need further support from me...
