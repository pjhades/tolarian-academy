# Scenario in Question

- A Python app prints logs in different priority levels via `logging` package
- The app runs in Docker
- Container is started with journald log driver enabled

Now, `journalctl` does not differentiate log levels, everything is reported as error.


# Implementation of journald driver in Docker (moby)

The path of journald driver registration:

1. `init()` from `daemon/logger/journald/journald.go`
1. `RegisterLogDriver()` from `daemon/logger/factory.go`
1. `register()` from `daemon/logger/factory.go`. This function registers `New()`, whose job is to create a `Logger` interface implementer.
   The driver uses go-systemd package to communicate with jouranld. When a `Logger` is created,
   a connection is established with domain socket `/run/systemd/journal/socket`, to which
   the log messages will be sent.

The path of log copying:

1. `InitializeStdio()` from `container/container.go` This is called by containerd.
2. `startLogging()` from `container/container.go`
3. `StartLogger()` from `container/container.go` It gets the `New()` function registered by driver as `initDriver()`
   1. `GetLogDriver()` from `daemon/logger/factory.go`
      1. `get()` from `daemon/logger/factory.go` This function returns the registered `New()`
   2. `initDriver()` from `container/container.go` The `initDriver()` function is actually the driver `New()`
4. `NewCopier()` from `daemon/logger/copier.go` This function creates a copier, source: two `BytesPipe` objects for stdout and stderr respectively; destination: the log driver
   1. `StdoutPipe()` and `StderrPipe()` from `container/stream/streams.go`
5. `Run()` from `daemon/logger/copier.go` For each source, add a goroutine `copySrc()` to a `sync.WaitGroup`. `copySrc()` block(?) reads from the `BytesPipe` (stdout and stderr), splits the read data by newline and creates a `Message` where `Line` is the message line, `Source` is either stdout or stderr. A timestamp is also attached to the message.
   1. `Log()` from `daemon/logger/journald/journald.go` This sends the `Message` struct to journald. The `Source` field of `Message` will be checked: if it's stdout, message will be sent to journald at info level, otherwise if it's stderr, sent at error level.

So it turns out that the log driver behaves as a proxy between the container and journald.
In the journald log driver there's no mechanism to translate the original log level.

A possible reason is that the log messages are in text but journals are in binary.
There isn't any format or protocol between the container and the driver,
so the driver doesn't understand log levels.
