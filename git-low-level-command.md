create an object in git with content 'xxx', whose key
is made of the sha-1 hash of its content plus a header

```
echo 'xxx' | git hash-object -w --stdin
```

inspect git object

```
git cat-file -p <hash> # pretty print the content
git cat-file -s <hash> # size
git cat-file -t <hash> # type
```

update the index

```
git update-index test.txt
git update-index new.txt
#  you can specify a particular version with SHA-1
git update-index --add --cacheinfo 100644 <hash> test.txt
```

create tree object from current index

```
git write-tree
```

read tree object into the index. `--prefix=dir` will cause
the tree named by `<hash>` be read under directory `dir`,
so after this if you do a `git status` you'll see a new
directory `dir` added with the named tree object as its content.

```
git read-tree --prefix=dir <hash>
```

commit a tree object

```
git commit-tree -m 'commit log' <hash>
#  commit the tree <hash> with <commit> as its predecessor
git commit-tree -m 'commit log' -p <commit> <hash>
```
