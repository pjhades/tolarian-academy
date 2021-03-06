# SystemTap from Beginner to Beginner

# Install on Archlinux
See [https://wiki.archlinux.org/index.php/SystemTap](https://wiki.archlinux.org/index.php/SystemTap).

Install `systemtap-git` from AUR instead of `systemtap`, otherwise compile errors will occur.

After installation, `groupadd` the groups `stapusr`, `stapsys`, `stapdev`, and
add the user that will run the scripts into those groups. Otherwise you'll need
root privilege.

* `stapdev` can run scripts AND modules, grant effective root privilege, should only be set on trusted users
* `stapusr` can only run modules from `/lib/modules/$(uname -r)/systemtap/`

# Hello World

```stap
probe begin
{
    print("hello world\n")
    exit()
}
```

* `stap x.stp` compile and run
* `stap -r $(uname -r) x.stp -m hello` compile the script to `hello.ko` kernel module
* `staprun hello.ko` run the compiled kernel module

# Cross Compiling
SystemTap can compile scripts from one machine (host machine) to modules that will be run
on different machines (target machines). The host machine should have corresponding kernel
packages installed. See [https://sourceware.org/systemtap/SystemTap_Beginners_Guide/cross-compiling.html](https://sourceware.org/systemtap/SystemTap_Beginners_Guide/cross-compiling.html).

# Tapset
Located at '/usr/share/systemtap/tapset'. SystemTap will find and replace the tapsets
used with their definitions in the library before compiling the script to C.
