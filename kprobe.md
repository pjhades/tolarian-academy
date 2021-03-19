# kprobe ftrace interface

## `kernel/trace/trace_kprobe.c`

`init_kprobe_trace` is called to initialize the tracefs interface for kprobes:
- `/sys/kernel/tracing/kprobe_events` and
- `/sys/kernel/tracing/kprobe_profile`

The file operations on `kprobe_events` are

```C
static const struct file_operations kprobe_events_ops = {
    .owner          = THIS_MODULE,
    .open           = probes_open,
    .read           = seq_read,
    .llseek         = seq_lseek,
    .release        = seq_release,
    .write      = probes_write,
};
```

The file is accesses as a sequential file.

- Open: check file access mode, if mode is write and truncate, like
```
>/sys/kernel/tracing/kprobe_events
```
call `release_all_trace_kprobes()` first. Then delegate to `seq_open()`.

- Read: delegate to `seq_read`.
- Seek: delegate to `seq_lseek`.
- Release: delegate to `seq_release`.
- Write: call `trace_parse_run_command()` to parse the content written and create the probe.


## `kernel/trace/trace.c`

Now probe parsing and creation are done in `trace_parse_run_command()`.
The probe string is copied in chunks into a kernel buffer of 4096 bytes.
Then in each 4906-byte chunk, each line is run via `trace_run_command()` and a corresponding probe is created via create_trace_probe(). Function create_trace_probe() does the actual parsing and creates a struct trace_kprobe object,
which is registered by calling `register_trace_kprobe()`.


## `kernel/trace/trace_output.c`

Function `register_trace_kprobe()` calls `register_kprobe_event()` to register an event.
The `struct trace_event` objects are hashed based on their types into a global hashtable `event_hash`.
Also all those objects are collected in the list `ftrace_event_list`. Function `trace_search_list()`
assigns a type for the event and tells which list the event object should be appended to.
