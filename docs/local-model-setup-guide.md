# 本地模型完整部署指南：安装、对接 Hermes Agent、任务分流

> 适用于：48GB 内存 Mac（Apple Silicon），同时运行 Hermes + OpenClaw 等 agents

---

## 目录

1. [模型最终推荐（更新版）](#一模型最终推荐更新版)
2. [Ollama 安装与配置（详细版）](#二ollama-安装与配置详细版)
3. [Hermes Agent 对接本地模型](#三hermes-agent-对接本地模型)
4. [任务分流策略：让 Agent 智能调度](#四任务分流策略让-agent-智能调度)
5. [完整配置文件示例](#五完整配置文件示例)
6. [内存管理与性能优化](#六内存管理与性能优化)
7. [故障排查](#七故障排查)

---

## 一、模型最终推荐（更新版）

经过更深入的调研，我对比了截至 2026 年 4 月的所有主流本地代码模型。**结论有更新**：

### 方案 A（首选推荐）：Qwen 2.5 Coder 32B Instruct

| 指标 | 数值 |
|------|------|
| 参数量 | 32B（Dense 架构） |
| 量化版本 | Q4_K_M |
| 磁盘占用 | ~20GB |
| 运行内存 | ~20-22GB |
| 上下文窗口 | 32K（实际可用 32,768） |
| HumanEval | 92.7% |
| SWE-bench Verified | 50.0% |
| 推理速度 | 10-15 tok/s（Mac Apple Silicon） |
| 许可证 | Apache 2.0 |

**为什么首选它：**
- 32B 级别代码能力最强，HumanEval 92.7% 接近 Claude Sonnet 水平
- 20GB 内存占用，48GB Mac 跑完 agents 后余量充足（还剩 ~26GB）
- 稳定成熟，Ollama 原生支持，tool calling 开箱即用
- 上下文 32K 对 agent 辅助任务完全够用

### 方案 B（激进方案）：Qwen3-Coder-Next 80B MoE

| 指标 | 数值 |
|------|------|
| 参数量 | 80B 总参数，**3B 激活参数**（MoE 架构） |
| 量化版本 | Q4_K_M |
| 磁盘占用 | ~48-52GB |
| 运行内存 | ~46-48GB |
| 上下文窗口 | 256K |
| SWE-bench Verified | 70.6% |
| 推理速度 | 20+ tok/s（因为只激活 3B 参数） |
| 许可证 | Apache 2.0 |

**潜力巨大但有风险：**
- SWE-bench 70.6% 远超方案 A 的 50%，接近 Claude Opus 水平
- MoE 架构只激活 3B 参数，推理速度反而比 32B Dense 更快
- **风险**：48GB 内存全部吃满，跑 agents 时可能 swap，导致整机卡顿
- **建议**：先跑方案 A 验证流程，闲时试方案 B 看系统能否承受

### 方案 C（轻量备选）：Qwen 2.5 Coder 14B

| 指标 | 数值 |
|------|------|
| 参数量 | 14B |
| 量化版本 | Q6_K |
| 磁盘占用 | ~12GB |
| 运行内存 | ~12-14GB |
| 上下文窗口 | 32K |
| HumanEval | ~85% |
| 推理速度 | ~25 tok/s |

**什么时候选它：**
- 方案 A 运行时发现内存紧张，频繁 swap
- 你需要同时开很多应用（IDE + 浏览器 + Docker + agents）
- 对代码质量要求不是特别高（纯补全/boilerplate 场景）

### 不推荐的模型及原因

| 模型 | 不推荐原因 |
|------|-----------|
| Llama 3.3 70B | ~42GB 内存，加 agents 必定爆内存 |
| DeepSeek Coder V2 Lite 16B | HumanEval 81%，代码能力不如 Qwen 2.5 Coder 32B |
| Qwen3 32B（通用版） | 通用模型，代码能力（HumanEval ~75%）远不如代码专用版 |
| CodeLlama 34B | 已过时，所有指标被 Qwen 2.5 Coder 全面超越 |

---

## 二、Ollama 安装与配置（详细版）

### 2.1 安装 Ollama

```bash
# 方法一：Homebrew（推荐，CLI 控制）
brew install ollama

# 方法二：直接下载（提供菜单栏图标 + 自动更新）
# 访问 https://ollama.com/download 下载 macOS 版本
```

安装后验证：

```bash
ollama --version
# 输出类似：ollama version is 0.19.x
```

### 2.2 启动 Ollama 服务

```bash
# 方法一：前台启动（看得到日志，调试用）
ollama serve

# 方法二：后台服务（Homebrew 安装的）
brew services start ollama

# 方法三：如果是直接下载安装的，点开 Ollama 应用即可自动启动服务
```

验证服务运行：

```bash
curl http://localhost:11434/v1/models
# 应该返回 JSON 响应（即使模型列表为空）
```

### 2.3 下载模型

```bash
# ===== 方案 A：Qwen 2.5 Coder 32B（首选，~20GB 下载）=====
ollama pull qwen2.5-coder:32b-instruct-q4_K_M

# ===== 方案 B：Qwen3-Coder-Next 80B MoE（激进，~48GB 下载）=====
ollama pull qwen3-coder-next:q4_K_M

# ===== 方案 C：Qwen 2.5 Coder 14B（轻量，~12GB 下载）=====
ollama pull qwen2.5-coder:14b-instruct-q6_K
```

下载需要一些时间，取决于网络速度。20GB 大约需要 10-30 分钟。

### 2.4 验证模型可用

```bash
# 运行一个简单测试
ollama run qwen2.5-coder:32b-instruct-q4_K_M "用 Python 写一个二分查找"

# 查看当前加载的模型和资源占用
ollama ps

# 你应该看到类似输出：
# NAME                                    SIZE      PROCESSOR    UNTIL
# qwen2.5-coder:32b-instruct-q4_K_M      20 GB     100% GPU     4 minutes from now
```

**关键检查点：**
- `PROCESSOR` 列应显示 `100% GPU`（Metal 加速正常）
- 如果显示 `100% CPU`，说明 Metal 没生效，参考故障排查章节
- `SIZE` 应该约 20GB

### 2.5 设置上下文长度（重要！）

**Ollama 默认上下文长度很低！** 48GB 以下的 Mac 默认只有 4096-32768 tokens。对于 agent 使用，系统提示 + 工具定义就可能占 4K-8K tokens，必须手动设置。

```bash
# 方法一：环境变量（推荐，全局生效）
# 在你的 shell 配置文件中添加（~/.zshrc 或 ~/.bashrc）
export OLLAMA_CONTEXT_LENGTH=32768

# 然后重启 Ollama 服务
brew services restart ollama

# 方法二：创建自定义模型（持久化上下文设置）
cat > /tmp/Modelfile << 'EOF'
FROM qwen2.5-coder:32b-instruct-q4_K_M
PARAMETER num_ctx 32768
EOF

ollama create qwen2.5-coder-32k -f /tmp/Modelfile
# 之后使用 qwen2.5-coder-32k 这个名字来调用
```

验证上下文设置：

```bash
ollama ps
# 查看 CONTEXT 列，应该显示 32768 而不是 4096
```

### 2.6 设置开机自启（可选）

```bash
# Homebrew 安装的
brew services start ollama

# 直接下载安装的：在系统设置 → 通用 → 登录项 中添加 Ollama
```

---

## 三、Hermes Agent 对接本地模型

Hermes Agent 有多种方式连接本地模型，从简单到高级。

### 3.1 方式一：完全使用本地模型（最省钱，适合简单任务）

#### 交互式设置

```bash
hermes model
# 选择：Custom endpoint (self-hosted / VLLM / etc.)
# API base URL: http://localhost:11434/v1
# API key: （直接回车跳过，本地不需要）
# Model name: qwen2.5-coder:32b-instruct-q4_K_M
# Context length: 32768
```

#### 手动编辑配置文件

配置文件路径：`~/.hermes/config.yaml`

```yaml
model:
  default: "qwen2.5-coder:32b-instruct-q4_K_M"
  provider: "ollama"
  base_url: "http://localhost:11434/v1"
  context_length: 32768
```

验证连接：

```bash
# 启动 Hermes，应该看到它连接到本地模型
hermes chat
# 在对话中输入一个简单问题测试
```

### 3.2 方式二：云端为主 + 本地辅助（推荐的省钱方案）

这是最优策略：主推理用云端（质量高），辅助任务用本地（省钱）。

#### 主模型：保留云端

```yaml
# ~/.hermes/config.yaml
model:
  default: "anthropic/claude-sonnet-4-20250514"
  provider: "anthropic"
  # 或者用 OpenRouter
  # provider: "openrouter"
```

#### 辅助任务：路由到本地

```yaml
# 辅助模型配置 — 将高频低复杂度任务路由到本地
auxiliary:
  # 上下文压缩（Compaction）— 最大 token 节省点
  # 每次压缩都会消耗大量 token，路由到本地后完全免费
  compression:
    provider: "main"  # 使用下方 web_extract 同样的本地端点
    timeout: 120      # 本地模型比云端慢，增加超时时间

  # 网页内容提取和摘要
  web_extract:
    provider: "main"
    model: "qwen2.5-coder:32b-instruct-q4_K_M"

# 压缩设置
compression:
  enabled: true
  threshold: 0.50
  target_ratio: 0.20
  protect_last_n: 20
  summary_model: "qwen2.5-coder:32b-instruct-q4_K_M"
  summary_provider: "main"
```

**注意**：辅助模型（auxiliary）的 `provider: "main"` 指的是使用自定义端点配置。如果你的主模型设为 Anthropic，需要额外配置让辅助任务走本地。具体做法见下方完整配置示例。

### 3.3 方式三：Smart Model Routing（简单任务自动走本地）

Hermes 内置了"智能路由"功能，可以把短/简单的请求自动发到便宜的模型：

```yaml
# ~/.hermes/config.yaml
smart_model_routing:
  enabled: true
  max_simple_chars: 160    # 用户输入少于 160 字符视为"简单"
  max_simple_words: 28     # 用户输入少于 28 个词视为"简单"
  cheap_model:
    provider: custom
    model: "qwen2.5-coder:32b-instruct-q4_K_M"
    base_url: "http://localhost:11434/v1"
```

这样，简短的问题（如"解释这个函数"、"格式化这段代码"）会自动走本地模型，复杂问题走云端。

### 3.4 方式四：Skill 级别模型路由（最精细的控制）

Hermes 的 Skill 系统允许为每个 Skill 指定不同的模型。

#### 创建使用本地模型的 Skill

在 `~/.hermes/skills/` 目录下创建 Skill：

```bash
mkdir -p ~/.hermes/skills/local-code-review
```

创建 `~/.hermes/skills/local-code-review/SKILL.md`：

```markdown
---
name: local-code-review
model: qwen2.5-coder:32b-instruct-q4_K_M
---

# 代码审查 Skill

你是一个代码审查助手。请审查提供的代码，指出：
1. 潜在的 bug
2. 代码风格问题
3. 性能优化建议

保持回复简洁，只列出关键问题。
```

#### 在对话中调用

```
# Hermes 会自动使用 Skill 中指定的本地模型
/skill local-code-review
```

#### 更多可以走本地的 Skill 示例

**代码补全 Skill** (`~/.hermes/skills/local-code-complete/SKILL.md`)：

```markdown
---
name: local-code-complete
model: qwen2.5-coder:32b-instruct-q4_K_M
---

# 代码补全

根据上下文补全代码。只输出补全的代码部分，不要解释。
```

**代码格式化 Skill** (`~/.hermes/skills/local-code-format/SKILL.md`)：

```markdown
---
name: local-code-format
model: qwen2.5-coder:32b-instruct-q4_K_M
---

# 代码格式化

将提供的代码格式化为符合项目规范的风格。保持功能不变。
```

**文档生成 Skill** (`~/.hermes/skills/local-doc-gen/SKILL.md`)：

```markdown
---
name: local-doc-gen
model: qwen2.5-coder:32b-instruct-q4_K_M
---

# 文档生成

为提供的代码生成文档注释（docstring）。使用项目的文档风格。
```

### 3.5 方式五：Delegation（子任务模型路由）

在 Hermes 的 delegate_task 中，你可以为子任务指定模型：

```yaml
# ~/.hermes/config.yaml
delegation:
  supervisor_model: "anthropic/claude-sonnet-4-20250514"    # 主编排 agent（云端）
  execution_model: "qwen2.5-coder:32b-instruct-q4_K_M"     # 子任务执行（本地）
```

这意味着：
- **supervisor（主脑）**：使用云端 Claude，负责理解需求、规划任务、做复杂决策
- **execution（执行者）**：使用本地 Qwen Coder，负责执行具体的代码编写、格式化等

### 3.6 对话中切换模型

在 Hermes 对话中，你随时可以用 `/model` 命令切换：

```
/model custom:qwen2.5-coder:32b-instruct-q4_K_M    # 切换到本地模型
/model anthropic:claude-sonnet-4                      # 切回云端
/model custom                                         # 自动检测本地模型
```

---

## 四、任务分流策略：让 Agent 智能调度

### 4.1 哪些任务应该走本地模型

| 任务类型 | 走本地 | 预计占总 token 比例 | 节省效果 |
|---------|-------|-------------------|---------|
| 上下文压缩（Compaction） | ✅ | 20-30% | 最大节省点 |
| 代码补全/续写 | ✅ | 15-20% | 高 |
| Boilerplate/模板代码生成 | ✅ | 10-15% | 高 |
| 代码格式化/简单重构 | ✅ | 5-10% | 中 |
| 文档/注释生成 | ✅ | 5-8% | 中 |
| 网页内容提取/摘要 | ✅ | 3-5% | 中 |
| 简单 Q&A | ✅ | 3-5% | 低 |

| 任务类型 | 走云端 | 原因 |
|---------|-------|------|
| 复杂架构设计 | ✅ | 需要强推理能力 |
| 多文件 Bug 调试 | ✅ | 需要长上下文理解 |
| 新框架/API 使用指导 | ✅ | 需要最新知识 |
| 安全审计 | ✅ | 不能出错 |
| 复杂 SQL/IaC 生成 | ✅ | 本地模型容易生成过时语法 |

### 4.2 分流的三层架构

```
┌─────────────────────────────────────────────┐
│           Layer 1: Smart Routing             │
│    短/简单请求 → 自动走本地模型               │
│    长/复杂请求 → 走云端模型                   │
└─────────────┬───────────────────────────────┘
              │
┌─────────────▼───────────────────────────────┐
│       Layer 2: Delegation Routing            │
│    supervisor（规划） → 云端 Claude            │
│    execution（执行） → 本地 Qwen Coder         │
└─────────────┬───────────────────────────────┘
              │
┌─────────────▼───────────────────────────────┐
│       Layer 3: Auxiliary Task Routing         │
│    压缩/摘要/提取 → 本地模型                  │
│    视觉/复杂分析 → 云端模型                   │
└─────────────────────────────────────────────┘
```

### 4.3 预期节省效果

| 策略 | 预计节省 token | 实施难度 |
|------|--------------|---------|
| 压缩（Compaction）走本地 | 20-30% | 简单（改配置） |
| Smart Routing 走本地 | 10-15% | 简单（改配置） |
| Delegation 执行模型走本地 | 10-20% | 中等 |
| Skill 级别路由 | 5-10% | 需要创建 Skills |
| **合计** | **40-65%** | |

---

## 五、完整配置文件示例

以下是一个完整的 `~/.hermes/config.yaml` 配置，实现了云端 + 本地混合部署：

```yaml
# =============================================================================
# ~/.hermes/config.yaml — 云端 + 本地混合部署配置
# =============================================================================

# ── 主模型：云端 Claude（用于复杂推理和规划）──
model:
  default: "anthropic/claude-sonnet-4-20250514"
  provider: "anthropic"
  # 如果用 OpenRouter：
  # default: "anthropic/claude-sonnet-4"
  # provider: "openrouter"
  # base_url: "https://openrouter.ai/api/v1"

# ── 智能路由：简单请求自动走本地 ──
smart_model_routing:
  enabled: true
  max_simple_chars: 160
  max_simple_words: 28
  cheap_model:
    provider: custom
    model: "qwen2.5-coder:32b-instruct-q4_K_M"
    base_url: "http://localhost:11434/v1"

# ── 子任务委派：执行用本地，规划用云端 ──
delegation:
  supervisor_model: "anthropic/claude-sonnet-4-20250514"
  execution_model: "qwen2.5-coder:32b-instruct-q4_K_M"

# ── 辅助模型配置 ──
auxiliary:
  vision:
    provider: "auto"
    model: ""
    timeout: 30
    download_timeout: 30
  web_extract:
    provider: "main"
    model: "qwen2.5-coder:32b-instruct-q4_K_M"

# ── 上下文压缩：用本地模型压缩（最大省钱点）──
compression:
  enabled: true
  threshold: 0.50
  target_ratio: 0.20
  protect_last_n: 20
  summary_model: "qwen2.5-coder:32b-instruct-q4_K_M"
  # 压缩用本地模型，增加超时
  # summary_provider: "main"

# ── 记忆系统 ──
memory:
  memory_enabled: true
  user_profile_enabled: true
  memory_char_limit: 2200
  user_char_limit: 1375
  nudge_interval: 10
  flush_min_turns: 6

# ── Agent 行为 ──
agent:
  max_turns: 60
  verbose: false
  reasoning_effort: "medium"

# ── 终端配置 ──
terminal:
  backend: "local"
  cwd: "."
  timeout: 180

# ── Skills ──
skills:
  creation_nudge_interval: 15
```

---

## 六、内存管理与性能优化

### 6.1 48GB 内存分配建议

```
┌─────────────────────────────────────────────┐
│              48GB 统一内存分配                 │
├─────────────────────────────────────────────┤
│ macOS 系统          │  ~4GB                  │
│ Hermes Agent        │  ~2-4GB                │
│ OpenClaw Agent      │  ~2-4GB                │
│ IDE + 浏览器        │  ~6-8GB                │
│ ────────────────────│────────────────────── │
│ Ollama + 模型       │  ~20GB (Qwen 32B)     │
│ ────────────────────│────────────────────── │
│ 剩余缓冲            │  ~8-14GB               │
└─────────────────────────────────────────────┘
```

### 6.2 关键环境变量

在 `~/.zshrc`（或 `~/.bashrc`）中添加：

```bash
# Ollama 上下文长度（token 数）
export OLLAMA_CONTEXT_LENGTH=32768

# 模型空闲后自动卸载时间（默认 5m，内存紧张时缩短）
export OLLAMA_KEEP_ALIVE=5m

# 如果想让 Ollama 监听所有接口（从其他设备访问）
# export OLLAMA_HOST=0.0.0.0

# 启用调试日志（排查问题时用）
# export OLLAMA_DEBUG=1
```

### 6.3 模型按需加载/卸载

```bash
# 查看当前加载的模型
ollama ps

# 手动卸载模型（释放内存给其他程序）
curl http://localhost:11434/api/generate \
  -d '{"model": "qwen2.5-coder:32b-instruct-q4_K_M", "keep_alive": 0}'

# 手动预加载模型（提前加载避免首次请求慢）
curl http://localhost:11434/api/generate \
  -d '{"model": "qwen2.5-coder:32b-instruct-q4_K_M", "keep_alive": "10m"}'
```

### 6.4 监控内存使用

```bash
# 实时监控内存
# 方法一：使用 Activity Monitor（活动监视器）
# 方法二：命令行
vm_stat 1    # 每秒刷新

# 查看 Ollama 进程内存
ps aux | grep ollama

# 如果看到大量 swap 使用，说明内存不够
sysctl vm.swapusage
```

### 6.5 内存紧张时的应急方案

如果发现经常 swap：

```bash
# 1. 缩短模型驻留时间
export OLLAMA_KEEP_ALIVE=2m

# 2. 降级到更小的模型
ollama pull qwen2.5-coder:14b-instruct-q6_K
# 然后修改 config.yaml 中的模型名

# 3. 减小上下文窗口
export OLLAMA_CONTEXT_LENGTH=16384
```

---

## 七、故障排查

### 7.1 "Connection refused" — Ollama 服务没启动

```bash
# 检查服务状态
brew services list | grep ollama

# 启动服务
brew services start ollama
# 或
ollama serve
```

### 7.2 "100% CPU" — Metal GPU 没有激活

```bash
# 检查 Ollama 二进制是否是 ARM 原生
file $(which ollama)
# 应该显示 "arm64"，如果显示 "x86_64" 说明在用 Rosetta 模拟

# 修复：重新安装 ARM 原生版本
brew reinstall ollama
```

### 7.3 Tool Calling 不工作 — 模型输出 JSON 而不是执行工具

Ollama 默认支持 tool calling，但需要确认：

```bash
# 检查模型是否支持 tool calling
ollama show qwen2.5-coder:32b-instruct-q4_K_M
# 在输出中查找 "tools" 相关信息
```

如果仍然不工作，创建自定义模型强制启用：

```bash
cat > /tmp/Modelfile << 'EOF'
FROM qwen2.5-coder:32b-instruct-q4_K_M
PARAMETER num_ctx 32768
TEMPLATE """{{ if .System }}<|im_start|>system
{{ .System }}<|im_end|>
{{ end }}{{ range .Messages }}<|im_start|>{{ .Role }}
{{ .Content }}<|im_end|>
{{ end }}<|im_start|>assistant
"""
EOF
ollama create qwen2.5-coder-agent -f /tmp/Modelfile
```

### 7.4 上下文不够 — "Context limit: 2048/4096"

```bash
# 方法一：环境变量
export OLLAMA_CONTEXT_LENGTH=32768
brew services restart ollama

# 方法二：在 Hermes config 中手动指定
# ~/.hermes/config.yaml
model:
  context_length: 32768
```

### 7.5 响应被截断 — 输出不完整

这通常是 `max_tokens` 设置太低导致的。在 Hermes 配置中增加：

```yaml
model:
  max_tokens: 8192  # 增大输出上限
```

### 7.6 Hermes 连接超时

本地模型比云端慢，需要增加超时：

```bash
# 在 ~/.hermes/.env 中设置
HERMES_STREAM_READ_TIMEOUT=1800
```

---

## 附录：一键启动脚本

创建 `~/start-local-ai.sh`：

```bash
#!/bin/bash

echo "=== 启动本地 AI 服务 ==="

# 设置环境变量
export OLLAMA_CONTEXT_LENGTH=32768
export OLLAMA_KEEP_ALIVE=5m

# 启动 Ollama（如果没运行）
if ! pgrep -x "ollama" > /dev/null; then
    echo "启动 Ollama 服务..."
    ollama serve &
    sleep 3
fi

# 预加载模型
echo "预加载 Qwen 2.5 Coder 32B..."
curl -s http://localhost:11434/api/generate \
  -d '{"model": "qwen2.5-coder:32b-instruct-q4_K_M", "keep_alive": "30m"}' \
  > /dev/null

# 验证
echo ""
echo "=== 模型状态 ==="
ollama ps
echo ""
echo "=== 内存使用 ==="
echo "Swap 使用: $(sysctl -n vm.swapusage 2>/dev/null || echo 'N/A')"
echo ""
echo "本地 AI 服务就绪！API 地址: http://localhost:11434/v1"
echo "Hermes 连接命令: hermes model → Custom endpoint → http://localhost:11434/v1"
```

```bash
chmod +x ~/start-local-ai.sh
```

---

## 附录：升级路径

```
第 1 周：安装 Ollama + Qwen 2.5 Coder 32B
         配置 compression 走本地
         观察 token 消耗变化
              ↓
第 2 周：启用 smart_model_routing
         创建常用 Skills 走本地
         观察内存和速度
              ↓
第 3 周：配置 delegation 执行模型走本地
         精调哪些任务走本地/云端
              ↓
如果内存充裕：尝试 Qwen3-Coder-Next 80B MoE
如果内存紧张：降级到 Qwen 2.5 Coder 14B
```
