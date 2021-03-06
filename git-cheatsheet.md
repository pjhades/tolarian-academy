# Config
* `git config --global user.name xxxx`
* `git config --global user.email xxxx`
* `git config --global core.editor vim`
* `git config --list`


# Basics
* `git rm /path/to/file` 从 index 和 tree 中删除文件
* `git reset /path/to/file` 撤銷暫存。`reset` file 用於將 index 恢復到某個狀態，默認是 HEAD，與 `checkout` 配合可檢出文件
* `git reset --hard HEAD~1` 强制将 `HEAD` 回退一个 commit，此操作会同时修改 index 和 tree，没有暂存的修改都会被丢弃
* `git checkout -- /path/to/file` 撤銷修改。`checkout` file 用於從 index 中恢復或更新文件到 tree
* `git clean -i` 清除未跟蹤文件，交互式操作
* `git clean -n` 不刪除，只顯示要執行的操作
* `git reflog` 顯示 ref 的更新記錄，其中的 ref 可用於其他命令中指定某個對象
* `git log -p` 打印帶 diff 的日誌
* `git log -p -3` 只顯式最近 3 條日誌
* `git push origin tag` 向 origin 上傳 `tag` 標籤的內容
* `git push origin --tags` 上傳所有 tag 數據
* `git push --delete origin local-branch` 从 remote 上删除 `local-branch`
* `git push origin :local-branch` 同上


# Rebase
```
git rebase master
```
应用 master 分支的修改到当前分支上


```
git rebase -i HEAD~5
```
交互式地从 `HEAD` 开始 squash 5 个 commit。在随后打开的文本编辑器中可以选择 squash 的方式。此命令一般用来将压缩多个 commit。
需要注意的是，如果在交互模式下选择 `squash` 方式，即使用一个 commit 并合并 commit message 到前一个 commit，那么此 commit 必须拥有“前一个 commit”，
否则 rebase 会失败，可以用 `rebase --abort` 命令来取消。注意看清楚交互模式下列出的各个 commit。

此命令也可以用来分解一个大 commit。例如要分解 `HEAD` 前一个 commit，可以 `git rebase -i HEAD~1` 或者 `git rebase -i HEAD^`（假设 `HEAD` 此时只有一个 parent）。
然后在文本编辑器中将前一个 commit 设置为 `edit` 表示要修改该 commit。然后 git 会位于该 commit 之后，用 `git reset HEAD~1` 将该 commit 的修改重新放入 working tree。
重新安排 commit 之后，`git rebase --continue` 即可完成 commit 的分解。


# Submodules
* `git submodule add https://github.com/xxx/xxx.git` 新增 submodule
* `git submodule init` 在 index 中根據 `.gitmodules` 做初始化
* `git submodule update` clone submodule 到本地
* `git submodule update --init --recursive` update 同時初始化，遞歸 clone


# Remote & Tracking Branch
Tracking branch 表示 branch 之间存在直接的關聯，一般用来跟踪远程的 branch。
此時 git 知道與哪個服務器進行通信，例如可以直接 `git pull` 來更新本地 branch。
當用 `git clone` 克隆 repo 時，git 會自動創建一個叫做 `master` 的本地 branch，跟蹤遠程的 `origin/master`。
Tracking branch 可在命令中用 `@{upstream}` 或 `@{u}` 簡寫。

* `git checkout --track remote/branch` 創建並切換到一個本地 branch `branch`，跟蹤 `remote` 上的 `branch`
* `git checkout -b bbb remote/branch` 實現同樣效果，但本地 branch 重新命名為 `bbb`
* `git branch -u remote/branch` 設置當前 branch 的 upstream 為 `remote/branch`
* `git branch -vv` 查看本地 branch 的詳細信息，包括提交版本 ahead/behind、跟蹤哪個遠程 branch 等。注意此命令不會與服務器通信，
所以列出的數據都來自最近一次的 `fetch` 操作
* `git push origin --delete hotfix` 刪除遠程 branch `hotfix`
* `git branch --track fox` 创建一个新 branch 叫做 `fox`，跟踪当前所在的本地 branch。`--track` 和以前的 `--set-upstream` 等价但后者已标为 deprecated。
详见 `man git-branch`

參考 [https://git-scm.com/book/it/v2/Git-Branching-Remote-Branches](https://git-scm.com/book/it/v2/Git-Branching-Remote-Branches)


# Stash
用于临时保存尚未提交的修改。

* `git stash` 暂存当前未提交修改
* `git stash list` 列出所有 stash
* `git stash drop stash@{3}` 弃掉指定的 stash
* `git stash clear` 清空所有 stash
* `git stash apply stash@{1}` 将指定的 stash 写入当前工作目录


# Checkout
`git checkout <commit>`，可以将 `HEAD` 转为 detached state，即将 `HEAD` 指向历史中的某个 commit，
并以该 commit 为起点更新 index 和 tree。所谓 detached state 表示 `HEAD` 此时指向的 commit 没有其他任何 ref。
通常 `HEAD` 指向的 commit 会同时被另一个 branch 来 ref，但在 detached state 下，如果后续操作中更改了 `HEAD`，
则没有任何 ref 指向之前的 commit。此时可以通过 `git reflog` 来找到之前的 commit 对象。

`git checkout branch` 如果当前找不到名叫 `branch` 的 branch，但能且仅能在一个 remote 中找到同名的 branch，
就创建一个新的 branch 作为 tracking branch，相当于 `git checkout -b branch --track remote/branch`


# Ref
* `HEAD^2` 表示找第 2 个 parent
* `HEAD~2` 表示找 parent 的 parent
