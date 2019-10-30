# ftrace

- If `CONFIG_DYNAMIC_FTRACE` is enabled, when compiling the kernel, `gcc -pg` inserts extra function call to `mcount()`
- The call sites of all `mcount()` calls are recorded
- At boot time the calls are converted to NOPs
- Either `mcount()` or NOP will be inserted into the recorded call sites
  depending on if function or function graph tracer is enabled
- `trace_printk()` writes to a ring buffer, avoiding down sides of `printk()`
