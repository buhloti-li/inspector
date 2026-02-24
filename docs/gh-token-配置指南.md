# 在环境里配置 `gh` + GitHub Token（实操版）

> 目标：让自动化代理可以读取 GitHub Actions 结果（run / jobs / logs），并据此自动修复。

## 1. 前提条件
- 你有仓库访问权限。
- 本地/容器已安装 `gh`（GitHub CLI）。
- 准备一个 GitHub Personal Access Token（建议 Fine-grained PAT）。

## 2. 如何创建 Fine-grained PAT（详细步骤）

### 2.1 打开创建页面
1. 登录 GitHub 账号。
2. 右上角头像 -> **Settings**。
3. 左侧滚动到最下方 -> **Developer settings**。
4. 进入 **Personal access tokens** -> **Fine-grained tokens**。
5. 点击 **Generate new token**。

### 2.2 填写 Token 基本信息
在创建表单中按以下建议填写：
- **Token name**：例如 `ci-fix-bot-gh-cli`。
- **Description**：例如 `Read Actions logs and push CI fixes`。
- **Expiration**：建议 30~90 天（不要设永久）。
- **Resource owner**：选择你的个人账号或目标组织。

### 2.3 选择仓库范围
- **Repository access** 推荐：`Only select repositories`。
- 勾选目标仓库（例如 `OWNER/REPO`）。
- 若需要跨多个仓库自动修复，再选择 `All repositories`（不推荐，权限过大）。

### 2.4 勾选最小权限（关键）
在 **Repository permissions** 中设置：
- **Actions**: `Read`
- **Contents**: `Read and write`（需要提交修复代码）
- **Pull requests**: `Read and write`（需要创建/更新 PR）
- （可选）**Metadata**: `Read`

> 如果你只读取日志，不推送代码：`Contents` 可以仅 `Read`。

### 2.5 生成并保存
1. 点击页面底部 **Generate token**。
2. 复制显示出来的 token（只显示一次）。
3. 保存到密码管理器；不要发到聊天、邮件或提交到仓库。

### 2.6 组织仓库额外步骤（如适用）
- 若仓库属于 Organization，可能需要组织管理员批准 Fine-grained PAT。
- 在 token 列表中查看状态是否为 `Pending approval`。
- 审批通过后再执行后续 `gh` 登录。

## 3. 在环境中配置认证

### 3.0 如何进入“隔离容器”终端
不同场景进入方式不同，核心原则是：`GITHUB_TOKEN` 必须设置在**执行代理命令的同一个终端会话**里。

- **场景 A：你在 Codex/Agent 的容器终端里操作**
  - 直接在该终端执行后续 `export` 和 `gh` 命令即可。
- **场景 B：你在 GitHub Codespaces**
  1. 打开仓库页面 -> **Code** -> **Codespaces** -> 创建/打开 Codespace。
  2. 进入后打开 Terminal（`Ctrl+\``）。
  3. 在该 Terminal 中执行本章命令。
- **场景 C：你在本地 Docker 容器**
  1. 先拿到容器名：`docker ps`。
  2. 进入容器：`docker exec -it <container_name> bash`。
  3. 在容器内执行本章命令。

> 注意：你在“本机 shell”里设置的环境变量，不会自动出现在另一个远端容器里。

### 3.1 配置环境变量（推荐）
```bash
export GITHUB_TOKEN='<你的token>'
```

### 3.1.1 让环境变量持久化（可选）
如果你希望重开终端后仍可用，可写入 shell 配置文件：

```bash
# Bash
echo "export GITHUB_TOKEN='<你的token>'" >> ~/.bashrc
source ~/.bashrc

# Zsh
echo "export GITHUB_TOKEN='<你的token>'" >> ~/.zshrc
source ~/.zshrc
```

验证变量是否生效：

```bash
test -n "$GITHUB_TOKEN" && echo "GITHUB_TOKEN is set" || echo "GITHUB_TOKEN is missing"
```

### 3.2 登录 gh（无交互方式）
```bash
printf '%s' "$GITHUB_TOKEN" | gh auth login --with-token
```

### 3.3 验证登录
```bash
gh auth status
```

## 4. 验证仓库与 CI 读取能力
假设仓库为 `OWNER/REPO`：

```bash
# 查看最近工作流运行
gh run list -R OWNER/REPO --limit 10

# 查看某个 run 详情（将 <RUN_ID> 替换成上一步ID）
gh run view <RUN_ID> -R OWNER/REPO

# 拉取失败日志
gh run view <RUN_ID> -R OWNER/REPO --log
```

如果需要只看失败任务：
```bash
gh run list -R OWNER/REPO --status failure --limit 20
```

## 5. 自动修复闭环（推荐流程）
1. 通过 `gh run list` 找到最新失败 run。
2. `gh run view --log` 提取失败堆栈/测试名。
3. 本地修改代码并补充测试。
4. 提交并推送，触发下一轮 Actions。
5. 再次读取 run 状态，直到通过。

## 6. 常见问题排查

### 6.1 `gh: authentication failed`
- 检查 token 是否过期；
- 检查 token 是否绑定了正确组织/仓库；
- 重新执行 `gh auth login --with-token`。

### 6.2 `HTTP 403` / `Resource not accessible`
- token 权限不足（尤其 Actions / Contents / PR）；
- 仓库是组织仓库，组织策略限制了 PAT；
- fork 场景下 GITHUB_TOKEN 默认权限较低。

### 6.3 能看 run 但不能推送
- 给 token 增加 `Contents: Write`；
- 确认分支保护规则（需要 PR 而非直推）。

## 7. 安全建议
- 不要把 token 写入仓库文件；
- 优先使用短期/最小权限 token；
- 在 CI 中通过 Secrets 注入（如 `GH_TOKEN`），不用明文；
- 定期轮换 token（例如每 30~90 天）。
