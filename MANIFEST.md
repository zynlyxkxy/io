# 来源清单（MANIFEST）

本仓是**外网开发复刻仓**，源出公司内网 GitLab 的 `io`。只保留源码；预编译依赖不进 git。

## 内网版本

| 项 | 值 |
|---|---|
| 内网仓库 | `git@172.16.10.36:bonder_series/icb3000/io_template.git` |
| 分支 | `develop` |
| 导出时 commit | `8557a5bfbd2c43aa508d80111164c2c72aa07a00` |
| 导出时间 | 2026-11-12 |

## 被排除的预编译依赖

- `basiclibrary_output/`

这些目录原本是 git 子模块或打包好的运行时，含大体积二进制（单个文件可超过 GitHub 100 MB 上限），
因此被 `.gitignore` 排除。把对应的 deps 包解压到仓库根目录即可还原（见 Release 页）。

## 原来的 .gitmodules（已从本仓移除）

```ini
[submodule "basiclibrary_output"]
	path = basiclibrary_output
	url = git@172.16.10.36:2006/platform_v2_product/basiclibrary_output.git
```
