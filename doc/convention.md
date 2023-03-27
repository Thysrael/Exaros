# Convention

## Code Style

- 文件名：全小写 + 下划线
- 变量名：小驼峰
- 函数名：小驼峰
- 宏：全大写 + 下划线
- 私有变量或者函数：允许使用双下划线 `__`

----



## Git Branch

维护 `main` 一个稳定分支。

`main` 分支为 `branch` 的主分支，每次开发新的特征，请在 `main` 分支上执行

```shell
git checkout -b branch_name
```

关于 `branch_name` 应当遵循如下风格：

```shell
developer_name-feat/doc/fix-content
```

示例如下（`content` 的内容可以是中文）：

```shell
qs-doc-boot_sbi
```

在开发完成后，请先在 `dev` 上 `pull` 来更新 `dev`（其他人有可能已经更新到了 `dev`），然后再在你的分支上 `merge dev`，最后再在 `dev` 上 `merge` 你的分支，避免你无法在 `dev` 分支上解决冲突。  
