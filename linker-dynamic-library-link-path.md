# Preparation

```bash
$ cat libadd.h
int add(int, int);

$ cat libadd.c
int add(int x, int y)
{
    return x + y;
}

$ cat add.c
# include <libadd.h>
# include <stdio.h>
int main()
{
    printf("%d\n", add(2, 3));
    return 0;
}
```

# Compile and Link Normally

```bash
$ gcc -shared -fPIC -o libadd.so libadd.c

$ gcc -I. -L. -ladd -o add add.c

$ ldd add
    linux-vdso.so.1 (0x00007ffd70927000)
    libadd.so => not found
    libc.so.6 => /usr/lib/libc.so.6 (0x00007f506b37e000)
    /lib64/ld-linux-x86-64.so.2 (0x00007f506b71c000)

$ ./add
./add: error while loading shared libraries: libadd.so: cannot open shared object file: No such file or directory
```

This does not work at runtime because `ld.so` does not see `libadd.so` in the default search paths.


# Solutions
These solutions are listed in the order in which `ld.so` searches for libraries at runtime.

## Use `ld -rpath`
The `-rpath` options of `ld` adds directories to the runtime search path, which can be used
when compile and link some specific program. We pass this option to `ld` via `gcc -Wl,-rpath=xxx` when
buiding the executable `add`.

```bash
$ gcc -I. -L. -ladd -Wl,-rpath=. -o add add.c

$ ldd add
    linux-vdso.so.1 (0x00007fff44dc7000)
    libadd.so => ./libadd.so (0x00007f625f3c7000)
    libc.so.6 => /usr/lib/libc.so.6 (0x00007f625f029000)
    /lib64/ld-linux-x86-64.so.2 (0x00007f625f5c9000)

$ ./add
5
```

## Use `LD_RUN_PATH`
This environment variable is used when `-rpath` is absent.

```bash
$ LD_RUN_PATH=. gcc -I. -L. -ladd -o add add.c

$ ldd add
    linux-vdso.so.1 (0x00007ffc856f7000)
    libadd.so => ./libadd.so (0x00007f1cd8776000)
    libc.so.6 => /usr/lib/libc.so.6 (0x00007f1cd83d8000)
    /lib64/ld-linux-x86-64.so.2 (0x00007f1cd8978000)

$ ./add
5
```

## Use `LD_LIBRARY_PATH`
This environment variable provides search paths for the runtime linker to
find shared libraries needed.

```bash
$ LD_LIBRARY_PATH=. ./add
5
```

This can be useful if you want to temporarily test a new version of shared library.
But it can be a problem as the libraries in `LD_LIBRARY_PATH` can override the real one,
possibly causing security problems.

For example, we can create a `libbadguy.so` library whose malicious `add()` function
actually does the multiplication. We put the library in the parent directory, and copy the
real addition version to `/usr/lib64` because the default search paths have lower priority
than `LD_LIBRARY_PATH`. Then we can just use `LD_LIBRARY_PATH` to let the malicious version
take the place.

```bash
$ cat libbadguy.c
int add(int x, int y)
{
    return x * y;
}

$ gcc -shared -fPIC -o ../libadd.so libbadguy.c    # put the malicious version in the parent directory

$ sudo mv libadd.so /usr/lib64    # copy the real version to the system path

$ gcc -I. -L. -ladd -o add add.c    # compile it normally in hope for using the real version

$ ./add    # it seems work
5

$ LD_LIBRARY_PATH=.. ./add    # but it can be replaced
6
```

Since this can be dangerous, `ld.so` ignores environment variables like `LD_LIBRARY_PATH`
if the executable is going to run in secure-execution mode, where one typical case is that
the executable has setuid bit set.

```bash
$ sudo chown root:root add    # now add is root's property

$ sudo chmod u+s add    # enable suid

$ ./add
5

$ LD_LIBRARY_PATH=.. ./add    # this time it is ignored
5
```

## Use `/etc/ld.so.conf`
We can add the path to the libraries in `/etc/ld.so.conf` or a file in `/etc/ld.so.conf.d/`.
This search happens AFTER the search of the default directories.
