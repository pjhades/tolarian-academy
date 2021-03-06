#  Concepts
* blocks
  * 1k, 2k, 4k or 8k, divided into block groups
* superblock
  * `ext2_super_block`
  * used to access the rest of the data on the disk
  * 1024b
  * located at the offset 1024b from the start of the fs, in block 0 or 1, depending on block size `s_first_data_block`
* block group
  * superblock (same for all)
    * group descriptors (same for all)
    * array of `ext2_group_desc`
    * meta info of each group
    * used to locate block and inode bitmaps and inode table
  * block bitmap
    * which blocks in a group have been allocated
  * inode bitmap
    * which inodes in a group have been allocated
  * inode table
    * array of inodes
    * first few entries are reserved
      * `EXT2_BAD_INO`, 1, bad blocks inode
      * `EXT2_ROOT_INO`, 2, root directory inode
      * `EXT2_ACL_IDX_INO`, 3, ACL index inode (deprecated?)
      * `EXT2_ACL_DATA_INO`, 4, ACL data inode (deprecated?)
      * `EXT2_BOOT_LOADER_INO`, 5, boot loader inode
      * `EXT2_UNDEL_DIR_INO`, 6, undelete directory inode
    * `s_first_ino` denotes the first available inode
  * data blocks
* inode
  * `ext2_inode`
  * `i_block` holds pointers to blocks,  totally `EXT2_N_BLOCKS`
    * `EXT2_NDIR_BLOCKS` 12 direct blocks (file data)
    * `EXT2_IND_BLOCK` 1 singly indirect
    * `EXT2_DIND_BLOCK` 1 doubly indirect
    * `EXT2_TIND_BLOCK` 1 triply indirect
* dir entry
  * `ext2_dir_entry`
  * linked list or hashed index (directory inode `i_flags` has `EXT2_INDEX_FL`)


#  Disk Layout
A typical layout on a 20MB disk with 1KiB blocks:

```
    Block offset      Length              Description
    byte 0            512 bytes           boot record (if present)
    byte 512          512 bytes           additional boot record data (if present)

    -- block group 0, blocks 1 to 8192 --
    byte 1024         1024 bytes          superblock
    block 2           1 block             block group descriptor table
    block 3           1 block             block bitmap
    block 4           1 block             inode bitmap
    block 5           214 blocks          inode table
    block 219         7974 blocks         data blocks

    -- block group 1, blocks 8193 to 16384 --
    block 8193        1 block             superblock backup
    block 8194        1 block             block group descriptor table backup
    block 8195        1 block             block bitmap
    block 8196        1 block             inode bitmap
    block 8197        214 blocks          inode table
    block 8408        7974 blocks         data blocks

    -- block group 2, blocks 16385 to 24576 --
    block 16385       1 block             block bitmap    // note here sparse_super is enabled
    block 16386       1 block             inode bitmap
    block 16387       214 blocks          inode table
    block 16601       3879 blocks         data blocks
```


#  Indexed directory entries
The target of indexed directories is to reduce the time spent
on searching for a directory entry that matches our desired target name.
It takes `O(n)` time to traverse a series of blocks.

By adding **index blocks** amongst normal directory entry blocks,
we can achieve the goal that we only need to search in one block for our desired
directory entry.

Index blocks contain `dx_entry`s, which contain hash values and block numbers, specifying
the target block that contains our directory entry. The hash value in the index blocks are
less than or equal to all the hash values of the names its corresponding block.

Newly created empty directories have two blocks: an **index root block**, and an **empty directory entry block**.
We keep adding new index entries to the index block until it gets full, when we add one more (maximum is 1) level
of indirection, forming a tree structure (HTree). The indirection level is referenced as an **internal index block**.

As the designer Daniel Phillips said:
The index root has the following structure:

> `root: [fake dirents; header; index entries...]`
>
> The "fake dirents" are normal directory entries for "." and "..".  The
> rec_len of the latter is the entire remainder of the block, so that the
> root index block appears to older versions of Ext2 to have only the 
> two entries in it.
>
> The structure of an internal index node is similar:
>
> `node: [fake dirent; index entries...]`
>
> In this case the "fake dirent" is an empty entry that appears to take up
> the entire block.  In fact, in both the index root and nodes, this
> apparently empty space is filled with index entries.  Each index entry
> is 8 bytes long, and it happens that this is the same size as an empty
> directory entry:
>
> `entry: [hash, block]`


```
    Offset (bytes)  Size (bytes)    Description
    -- Fake Linked Directory Entry: . --
    0               4               inode: this directory
    4               2               rec_len: 12
    6               1               name_len: 1
    7               1               file_type: EXT2_FT_DIR=2
    8               1               name: .
    9               3               padding
    -- Fake Linked Directory Entry: .. --
    12              4               inode: parent directory
    16              2               rec_len: (blocksize - this entry's length(12))
    18              1               name_len: 2
    19              1               file_type: EXT2_FT_DIR=2    // as if it contains only . and ..
    20              2               name: ..
    22              2               padding
    -- Indexed Directory Root Information Structure --
    24              4               reserved, zero
    28              1               hash_version
    29              1               info_length
    30              1               indirect_levels
    31              1               reserved - unused flags
```

And right after the above structures, there're indexed directory entries till the end of the data blocks
or till all the files in the directory are indexed:

```
    Offset (bytes)  Size (bytes)    Description
    -- Indexed Directory Entry Count and Limit Structure
    0               2               limit  // max number of indexed directory entries
    2               2               count  // actual count
    -- A Bunch of Indexed Directory Entries,
    -- Sorted by Hash Value in Ascending Order
    Offset (bytes)  Size (bytes)    Description
    0               4               hash   // 32bit filename hash
    4               4               block
```


#  Others
Features are enabled with `mkfs.ext2 -O xxx`, where default values are in `/etc/mke2fs.conf`.
See `man mkfs.ext2` for details.

* `sparse_super`: superblocks and group descriptors should be stored only in groups 0, 1 and powers of 3, 5 and 7, enabled by default.
* `sparse_super2`: more extreme, store twice on the first and last block group.
* `dir_index`: enable indexed directory.

Check features: `tune2fs -l /path/to/dev`.



#  RTFSC
Various error handling code is omitted.

About the indexed directory, ext3 source code says in `/fs/ext3/namei.c`:

##  Search
```
    static struct buffer_head *ext3_find_entry(struct inode *dir,
    					struct qstr *entry,
    					struct ext3_dir_entry_2 **res_dir)
    {
    	// ...
    	if (is_dx(dir)) {
    		bh = ext3_dx_find_entry(dir, entry, res_dir, &err);
    		/*
    		 * On success, or if the error was file not found,
    		 * return.  Otherwise, fall back to doing a search the
    		 * old fashioned way.
    		 */
    		if (bh || (err != ERR_BAD_DX_DIR))
    			return bh;
    		dxtrace(printk("ext3_find_entry: dx failed, falling back\n"));
    	}
        // ...
    }
```

And we have function `ext3_dx_find_entry` in the same file:

```
    static struct buffer_head * ext3_dx_find_entry(struct inode *dir,
    			struct qstr *entry, struct ext3_dir_entry_2 **res_dir,
    			int *err)
    {
    	// ...
        // after this, frame will contain the found dx_entry with the
        // target hash value
    	if (!(frame = dx_probe(entry, dir, &hinfo, frames, err)))
    		return NULL;
    	do {
            // get that block
    		block = dx_get_block(frame->at);
    		if (!(bh = ext3_dir_bread (NULL, dir, block, 0, err)))
    			goto errout;
    
            // normal search
    		retval = search_dirblock(bh, dir, entry,
    					 block << EXT3_BLOCK_SIZE_BITS(sb),
    					 res_dir);
    
            // ...
    		/* Check to see if we should continue to search */
    		retval = ext3_htree_next_block(dir, hinfo.hash, frame,
    					       frames, NULL);
    		if (retval < 0) {
    			ext3_warning(sb, __func__,
    			     "error reading index page in directory #%lu",
    			     dir->i_ino);
    			*err = retval;
    			goto errout;
    		}
    	} while (retval == 1);

        // ...
    }
```

And the function `dx_probe`:

```
static struct dx_frame *
dx_probe(struct qstr *entry, struct inode *dir,
	 struct dx_hash_info *hinfo, struct dx_frame *frame_in, int *err)
{
	unsigned count, indirect;
	struct dx_entry *at, *entries, *p, *q, *m;
	struct dx_root *root;
	struct buffer_head *bh;
	struct dx_frame *frame = frame_in;
	u32 hash;

    // read a block
	frame->bh = NULL;
	if (!(bh = ext3_dir_bread(NULL, dir, 0, 0, err))) {
		*err = ERR_BAD_DX_DIR;
		goto fail;
	}
    // check infomation in the dx_root
	root = (struct dx_root *) bh->b_data;

    // ...
	hinfo->hash_version = root->info.hash_version;
	if (hinfo->hash_version <= DX_HASH_TEA)
		hinfo->hash_version += EXT3_SB(dir->i_sb)->s_hash_unsigned;
	hinfo->seed = EXT3_SB(dir->i_sb)->s_hash_seed;
	if (entry)
		ext3fs_dirhash(entry->name, entry->len, hinfo);
	hash = hinfo->hash;

    // ...
    // find indirect level to decide if we need to advance one more step
	if ((indirect = root->info.indirect_levels) > 1) {
		ext3_warning(dir->i_sb, __func__,
			     "Unimplemented inode hash depth: %#06x",
			     root->info.indirect_levels);
		brelse(bh);
		*err = ERR_BAD_DX_DIR;
		goto fail;
	}

    // jump over the dx_root, we reach the dx_entry items
	entries = (struct dx_entry *) (((char *)&root->info) +
				       root->info.info_length);
    // ...
    // now we find the dx_entry with the desired hash value
	while (1)
	{
		count = dx_get_count(entries);
        // ...
        // binary search to find the lower bound of the hash value
        // i.e. the target block
		p = entries + 1;
		q = entries + count - 1;
		while (p <= q)
		{
			m = p + (q - p)/2;
			dxtrace(printk("."));
			if (dx_get_hash(m) > hash)
				q = m - 1;
			else
				p = m + 1;
		}

        // ...
		at = p - 1;
		dxtrace(printk(" %x->%u\n", at == entries? 0: dx_get_hash(at), dx_get_block(at)));
		frame->bh = bh;
		frame->entries = entries;
		frame->at = at;
		if (!indirect--) return frame;

        // if there's an indirect level, read that block
		if (!(bh = ext3_dir_bread(NULL, dir, dx_get_block(at), 0, err))) {
			*err = ERR_BAD_DX_DIR;
			goto fail2;
		}
		at = entries = ((struct dx_node *) bh->b_data)->entries;
        // ...
		frame++;
		frame->bh = NULL;
	}
    // ...
	return NULL;
}
```


##  Inserting

First see function `ext3_add_entry`:
```
    static int ext3_add_entry (handle_t *handle, struct dentry *dentry,
    	struct inode *inode)
    {
        // ...
    	
        // call ext3_dx_add_entry for dir_index enabled directories
    	if (is_dx(dir)) {
    		retval = ext3_dx_add_entry(handle, dentry, inode);
    		if (!retval || (retval != ERR_BAD_DX_DIR))
    			return retval;
    		EXT3_I(dir)->i_flags &= ~EXT3_INDEX_FL;
    		dx_fallback++;
    		ext3_mark_inode_dirty(handle, dir);
    	}

        // ...
    }
```

Then we find `ext3_dx_add_entry`:
```
    static int ext3_dx_add_entry(handle_t *handle, struct dentry *dentry,
    			     struct inode *inode)
    {
    	struct dx_frame frames[2], *frame;
    	struct dx_entry *entries, *at;
    	struct dx_hash_info hinfo;
    	struct buffer_head * bh;
    	struct inode *dir = dentry->d_parent->d_inode;
    	struct super_block * sb = dir->i_sb;
    	struct ext3_dir_entry_2 *de;
    	int err;
    
        // call dx_probe first, same as ext3_dx_find_entry
        // this is to find the block that we may insert into
    	frame = dx_probe(&dentry->d_name, dir, &hinfo, frames, &err);
    	if (!frame)
    		return err;
    	entries = frame->entries;
    	at = frame->at;
    
        // read that block
    	if (!(bh = ext3_dir_bread(handle, dir, dx_get_block(frame->at), 0, &err)))
    		goto cleanup;
    
    	// ...
        // try to add the directory entry to that block
    	err = add_dirent_to_buf(handle, dentry, inode, NULL, bh);
        // ENOSPC error is returned in cases the block is full
    	if (err != -ENOSPC) {
    		bh = NULL;
    		goto cleanup;
    	}
    
        // ...
        // we reach the limit on the number of index entries
    	if (dx_get_count(entries) == dx_get_limit(entries)) {
    		u32 newblock;
    		unsigned icount = dx_get_count(entries);
    		int levels = frame - frames;
    		struct dx_entry *entries2;
    		struct dx_node *node2;
    		struct buffer_head *bh2;
    
            // ...

    		bh2 = ext3_append (handle, dir, &newblock, &err);
    		if (!(bh2))
    			goto cleanup;
    		node2 = (struct dx_node *)(bh2->b_data);
    		entries2 = node2->entries;
    		memset(&node2->fake, 0, sizeof(struct fake_dirent));
    		node2->fake.rec_len = ext3_rec_len_to_disk(sb->s_blocksize);
    		BUFFER_TRACE(frame->bh, "get_write_access");
    		err = ext3_journal_get_write_access(handle, frame->bh);
    		if (err)
    			goto journal_error;

    		if (levels) {
                // we need a new block
                // ...
    			memcpy ((char *) entries2, (char *) (entries + icount1),
    				icount2 * sizeof(struct dx_entry));
    			dx_set_count (entries, icount1);
    			dx_set_count (entries2, icount2);
    			dx_set_limit (entries2, dx_node_limit(dir));
    
    			/* Which index block gets the new entry? */
    			if (at - entries >= icount1) {
    				frame->at = at = at - entries - icount1 + entries2;
    				frame->entries = entries = entries2;
    				swap(frame->bh, bh2);
    			}
    			dx_insert_block (frames + 0, hash2, newblock);
    			// ...
    		} else {
                // add one more level of indirection
    			dxtrace(printk("Creating second level index...\n"));
    			memcpy((char *) entries2, (char *) entries,
    			       icount * sizeof(struct dx_entry));
    			dx_set_limit(entries2, dx_node_limit(dir));
    
    			/* Set up root */
    			dx_set_count(entries, 1);
    			dx_set_block(entries + 0, newblock);
    			((struct dx_root *) frames[0].bh->b_data)->info.indirect_levels = 1;
    
    			// ...
    		}
            // ...
    	}
        // ...
        // finally add the new item
    	err = add_dirent_to_buf(handle, dentry, inode, de, bh);
    	bh = NULL;
    	goto cleanup;
        // ...
    }
```


#  References
* [http://e2fsprogs.sourceforge.net/ext2intro.html](http://e2fsprogs.sourceforge.net/ext2intro.html)
* [http://www.nongnu.org/ext2-doc/ext2.html](http://www.nongnu.org/ext2-doc/ext2.html)
* [http://uranus.chrysocome.net/explore2fs/es2fs.htm](http://uranus.chrysocome.net/explore2fs/es2fs.htm)
* [https://lwn.net/2001/0412/a/index-format.php3](https://lwn.net/2001/0412/a/index-format.php3)
