# Industrial Runtime Qt Prototype

本仓库包含一个面向工业抓取场景的 Qt C++ 原型实现，包含：
- `WorkcellController`：任务生命周期、失败重试降级、急停恢复、统计追溯。
- 自动化测试（Qt Test）：映射关键测试用例。
- 详细测试用例文档与官网资料下载记录。

## 构建（需要Qt6）
```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

## CI
项目已提供 GitHub Actions 工作流：每次 push / pull request 会自动安装 Qt6、编译并执行 `ctest`。

## GitHub CLI 认证
可参考 `docs/gh-token-配置指南.md`，完成 `gh + GitHub Token` 配置，以便读取 Actions 结果并驱动自动修复流程。
