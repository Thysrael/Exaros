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

在开发完成后，请先在 `main` 上 `pull` 来更新 `main`（其他人有可能已经更新到了 `main`），然后再在你的分支上 `merge main`，最后再在 `main` 上 `merge` 你的分支，避免你无法在 `main` 分支上解决冲突。  

`commit message` 风格不做约束，建议合并前最后一次的 `commit` 和分支名相同。

---



## Doc

文档应该更新在 `doc` 中：

- 文件名：全小写 + 下划线
- 图片路径：统一在 `doc/img/`
- 图片命名：应当为 `file_name-num` 的形式，如 `boot-3.png` 。

---



## Comment
下载 `doxdocgen` 插件，然后在需要注释的文件或者函数上输入 `/**\n` 即可自动生成。
对于代码中的注释，有如下规范：

- 可以使用中文，也可以是英文。
- 必须为每一个函数写一个 `/**/` 样式的注释（vscode 可以自动补全），写在定义前而不是声明前。
- 必须为每个全局变量写一个 `/**/` 样式的注释。
- 必须为每一个头文件写 `/**/` 注释。
- 建议为分支循环语句每个条件写 `//` 注释。
- 建议为较难理解的点写 `//` 注释。
