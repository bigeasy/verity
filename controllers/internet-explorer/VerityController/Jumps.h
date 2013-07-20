#define IsOkay(err, branch) do { if (err != S_OK) { goto branch; } } while (0)
#define IsAllocated(ptr, err, branch) do { if (ptr) { err = E_OUTOFMEMORY; goto branch; } } while (0)
#define Done(branch) do { goto branch; } while (0)
