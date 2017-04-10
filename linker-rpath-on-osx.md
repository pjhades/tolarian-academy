# RPATH on MAC OS X


RPATH 用于指定动态链接库的搜索位置。cmake >=2.8.12 默认在链接时使用 RPATH。
此时可以在 CMakeLists.txt 脚本中进行各种编译时和安装后的 RPATH 设置。

## 链接时的搜索顺序

### 大多数 unix
1. RPATH - a list of directories which is linked into the executable, supported on most UNIX systems. It is ignored if RUNPATH is present.
2. `LD_LIBRARY_PATH` - an environment variable which holds a list of directories
3. RUNPATH - same as RPATH, but searched after `LD_LIBRARY_PATH`, supported only on most recent UNIX systems, e.g. on most current Linux systems
4. /etc/ld.so.conf - configuration file for ld.so which lists additional library directories
5. Builtin directories - basically /lib and /usr/lib

### OSX

* `DYLD_LIBRARY_PATH` - an environment variable which holds a list of directories
* RPATH - a list of directories which is linked into the executable. These can contain @loader_path and @executable_path.
* Builtin directories - /lib /usr/lib
* `DYLD_FALLBACK_LIBRARY_PATH` - an environment variable which holds a list of directories



## 为什么需要 builtin 目录之外的其他搜索位置？

因为

* 用户可能把动态库安装到自定义的位置（需指定 `LD_LIBRARY_PATH`），或者
* 在系统中同时安装同一库的不同版本，根据 executable 选择不同版本（需指定 RPATH）

所以 linker 在链接时，需要用一种方式将「去何处搜索动态库」记录在 target 中，以便 loader 加载时使用。

* linux
  - 用 ld 选项 --rpath 设置 RPATH，此信息会写入 ELF header .dynamic
  - 可以将 RPATH 设置为绝对路径，但显然很不方便，因为不能保证所有用户都将动态库装到同一位置
  - 可以指定相对于 $ORIGIN 变量的相对路径，即相对于 binary 所在的位置
* cmake
  - 使用 CMAKE_INSTALL_RPATH 指定安装后的 RPATH
  - cmake 提供了两种 RPATH
    - 构建目录（build tree）
    - 安装目录（installed dir）




## cmake 中的 RPATH 设置

一个库的 install name 就是一个路径，告诉 linker 在运行时如何找到这个库。

OSX 中 dyld linker 提供了几个变量用于在 install name 中指定链接时的相对路径：

* `@executable_path`，加载这个 lib 的 executable 所在位置，适用于嵌入 executable 中的动态库
* `@loader_path`，加载这个 object 的文件的位置，适用于嵌入 plugin 的动态库，而此情况下 `@executable_path` 不能用，因为
  - plugin 安装位置未知，无法通过 app executable 来查找
  - plugin 会被多个 app 使用，无法通过某个确定的 app executable 来查找
* `@rpath`，run path list，指定一个搜索列表，加载时依次在其中查找动态库
  - 更加灵活，编译一个库时，不必像前两者那样局限与某一种具体情况
  - 库的使用者在编译时可根据自己来指定 RPATH 的取值

cmake 默认情况下的设置：

* 在 build tree 中，cmake 默认将 RPATH 设置为库的绝对路径
* 在 install tree 中，cmake 将 RPATH 清空

相关 cmake 变量：

* `MACOSX_RPATH`，打开后（cmake >=2.8.12 默认打开），RPATH 将增加 `@rpath/` 前缀
* `CMAKE_MACOSX_RPATH`，初始化所有 target 的 `MACOSX_RPATH`
* `INSTALL_RPATH`，分号分隔列表，指定 installed targets 使用的 RPATH
* `CMAKE_INSTALL_RPATH`，初始化所有 target 的 `INSTALL_RPATH`
* `INSTALL_NAME_DIR`，设置 install name 的目录部分
* `CMAKE_SKIP_BUILD_PATH`，让 cmake 不使用 build tree 作为 RPATH
* `CMAKE_BUILD_WITH_INSTALL_RPATH`，cmake 默认将 build tree 作为 executable 的 RPATH，安装后执行需要 relink。此变量让 cmake 用 install path 作为 RPATH
* `INSTALL_RPATH_USE_LINK_PATH`，将所有 linker search path 上的目录加入 `INSTALL_RPATH`
* `CMAKE_INSTALL_RPATH_USE_LINK_PATH`，初始化所有 target 的 `INSTALL_RPATH_USE_LINK_PATH`

一般 `CMAKE_XXXX` 变量都时用来对所有 target 的对应变量做初始化的，如果要对某个 target
单独设置，可用 `set_target_properties` 命令，例如

```
set_target_properties (
    target_name
    PROPERTIES
    INSTALL_NAME_DIR "/wwww/yyyyy/jjjjj"
)
```


## OSX 可用以下命令控制链接相关信息

* `otool -L /path/to/exe` 列出某个 object 依赖的动态库
* `otool -D /path/to/lib` 列出某个库的 install name
* `otool -l /path/to/exe` 列出某个 object 的链接信息
* `otool -l /path/to/exe | grep -A5 RPATH` 在链接信息中查看 RPATH
* `install_name_tool -id "new_install_name" /path/to/lib` 修改 install name
* `install_name_tool -change "old_install_name" "new_install_name" /path/to/program`


## 参考

1. [https://cmake.org/Wiki/CMake_RPATH_handling](https://cmake.org/Wiki/CMake_RPATH_handling)
2. [https://www.mikeash.com/pyblog/friday-qa-2009-11-06-linking-and-install-names.html](https://www.mikeash.com/pyblog/friday-qa-2009-11-06-linking-and-install-names.html)
