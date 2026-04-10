# 本地模型部署方案：48GB Mac + Hermes/OpenClaw Agent 省 Token 策略

> **详细安装与对接指南请看** → [local-model-setup-guide.md](./local-model-setup-guide.md)

## 一、问题分析

你当前的痛点：Hermes Agent token 消耗太快，云端 API 费用高。同时 Mac 上还跑着 Hermes、OpenClaw 等 agents。

核心思路：**不是用本地模型替代云端模型，而是让 Agent 智能分流——简单任务走本地免费模型，复杂任务才用云端付费模型。**

---

## 二、本地模型的缺陷（必须正视）

| 缺陷 | 具体表现 | 影响程度 |
|------|---------|---------|
| **推理能力弱** | 多文件上下文理解、复杂架构设计、边界情况处理远不如 Claude/GPT | 致命 |
| **速度慢** | 32B 模型在 Mac 上约 10-15 tok/s，云端 50-100+ tok/s | 中等 |
| **上下文窗口小** | 本地模型实际可用上下文约 8K-32K，超过后质量急剧下降 | 高 |
| **专业代码差** | IaC（Terraform）、K8s YAML、复杂 SQL 等生成质量低 | 高 |
| **无持续更新** | 无法像云端模型一样持续学习新框架/新 API | 中等 |
| **内存竞争** | 与其他程序争抢统一内存，可能导致系统变慢 | 中等 |

**结论：本地模型不能完全替代云端模型，但可以在其擅长的领域省下 40-60% 的 token 消耗。**

---

## 三、本地模型擅长什么（应该让它做这些）

| 任务类型 | 适合本地模型 | 适合云端模型 |
|---------|------------|------------|
| 代码补全/续写 | ✅ 非常适合 | ❌ 杀鸡用牛刀 |
| 简单 boilerplate 生成 | ✅ 完全够用 | ❌ 浪费 token |
| 代码格式化/重构简单函数 | ✅ | ❌ |
| 文本分类/提取 | ✅ | ❌ |
| 上下文压缩/摘要（Compaction） | ✅ 省大量 token | ❌ |
| 简单问答/文档查找 | ✅ | ❌ |
| 复杂多文件架构设计 | ❌ 质量差 | ✅ 必须用 |
| Debug 复杂逻辑 | ❌ | ✅ |
| 新框架/新 API 使用 | ❌ 知识过时 | ✅ |
| 长上下文推理（>32K tokens） | ❌ | ✅ |

---

## 四、最终推荐：Qwen 2.5 Coder 32B（Q4_K_M 量化）

### 为什么选这个模型

| 对比项 | Qwen 2.5 Coder 32B | Llama 3.3 70B | Qwen 3 32B |
|-------|-------------------|---------------|------------|
| 内存占用 | **~20GB** | ~42GB（太大） | ~20GB |
| 代码专项优化 | ✅ 专为代码训练 | ❌ 通用模型 | ❌ 通用模型 |
| HumanEval 分数 | ~89% | ~82% | ~75% |
| 48GB Mac 可行性 | ✅ 还剩 ~28GB 给系统和 agents | ❌ 只剩 6GB | ✅ 但代码能力弱 |
| 上下文窗口 | 32K | 128K | 32K |
| 智能评分 | 89/100 | 85/100 | 79/100 |

### 为什么不选 70B

你的 Mac 48GB 内存，还要跑 Hermes + OpenClaw agents。70B Q4 量化需要 ~42GB，留给系统的内存不到 6GB，会导致严重卡顿和 swap。**32B 是你的甜点档位。**

### 为什么不选更小的模型

7B/14B 模型虽然更快更省内存，但代码生成质量差距明显，无法有效承担"省 token"的任务分流职责——如果本地模型生成的代码经常需要云端模型返工，反而更浪费。

---

## 五、推荐部署方案：Ollama（最简方案）

### 为什么选 Ollama 而不是 MLX/vMLX/Rapid-MLX

| 工具 | 速度 | 易用性 | 生态兼容 | 推荐理由 |
|-----|------|-------|---------|---------|
| **Ollama 0.19+** | 82 tok/s（已内置 MLX 后端） | ⭐⭐⭐⭐⭐ 一行安装 | OpenAI API 兼容 | **首选：最简单，性能已经够用** |
| Rapid-MLX | 79 tok/s | ⭐⭐⭐ 需要 Python 环境 | OpenAI API 兼容 | 备选：如果你需要更精细控制 |
| vMLX | 60 tok/s 普通场景 / 长上下文优势 | ⭐⭐⭐ 功能多但复杂 | 自有 API | 长上下文场景特化 |

**Ollama 0.19+ 已经内置 MLX 后端，Apple Silicon 上的性能和原生 MLX 几乎一致，没必要折腾更复杂的方案。**

### 安装步骤

```bash
# 1. 安装 Ollama（如果还没装）
brew install ollama

# 2. 启动 Ollama 服务
ollama serve

# 3. 拉取推荐模型
ollama pull qwen2.5-coder:32b-instruct-q4_K_M

# 4. 验证模型可用
ollama run qwen2.5-coder:32b-instruct-q4_K_M "写一个 Python 快速排序"

# 5. 查看内存占用（应该在 ~20GB 左右）
ollama ps
```

### API 调用方式

Ollama 启动后默认在 `http://localhost:11434` 提供 OpenAI 兼容 API：

```bash
curl http://localhost:11434/v1/chat/completions \
  -H "Content-Type: application/json" \
  -d '{
    "model": "qwen2.5-coder:32b-instruct-q4_K_M",
    "messages": [{"role": "user", "content": "用 Go 写一个 HTTP server"}]
  }'
```

---

## 六、Agent 智能分流策略（核心省钱方案）

### 6.1 OpenClaw 配置 — 任务级模型分流

在 OpenClaw 中配置不同任务使用不同模型：

```yaml
# OpenClaw 配置示例
models:
  # 复杂推理任务 → 云端
  orchestrator:
    provider: anthropic
    model: claude-sonnet-4-20250514  # 或 opus，按需选择
  
  # 代码生成/补全 → 本地
  code_generation:
    provider: ollama
    model: qwen2.5-coder:32b-instruct-q4_K_M
    base_url: http://localhost:11434/v1
  
  # 上下文压缩/摘要 → 本地
  compaction:
    provider: ollama
    model: qwen2.5-coder:32b-instruct-q4_K_M
    base_url: http://localhost:11434/v1
  
  # 分类/格式化 → 本地
  classification:
    provider: ollama
    model: qwen2.5-coder:32b-instruct-q4_K_M
    base_url: http://localhost:11434/v1
```

### 6.2 Hermes Agent 配置

```yaml
# Hermes Agent 模型路由配置
routing:
  # 简单代码任务路由到本地
  - pattern: "code_completion|boilerplate|format|refactor_simple"
    model:
      provider: ollama
      name: qwen2.5-coder:32b-instruct-q4_K_M
      endpoint: http://localhost:11434/v1
  
  # 复杂任务保留云端
  - pattern: "architecture|debug_complex|multi_file|planning"
    model:
      provider: anthropic
      name: claude-sonnet-4-20250514
```

### 6.3 Compaction 策略（最大省 token 点）

Compaction（上下文压缩）是 token 消耗大户。将 compaction 任务路由到本地模型：

```yaml
# OpenClaw compaction 配置
compaction:
  enabled: true
  provider: ollama
  model: qwen2.5-coder:32b-instruct-q4_K_M
  max_context_tokens: 8000
  aggressive: true
```

这样每次 compaction 不再消耗云端 token，预计可节省 **20-30% 的总 token 消耗**。

---

## 七、内存管理建议

你的 48GB 分配建议：

| 用途 | 预计占用 | 说明 |
|-----|---------|-----|
| macOS 系统 | ~4GB | 基础开销 |
| Hermes Agent | ~2-4GB | 运行时内存 |
| OpenClaw Agent | ~2-4GB | 运行时内存 |
| Ollama + Qwen 2.5 Coder 32B | ~20GB | 模型加载后常驻 |
| 开发工具（IDE、浏览器等） | ~8-10GB | 日常开发 |
| **剩余缓冲** | **~6-10GB** | 系统缓存和弹性空间 |

### 优化技巧

```bash
# 设置 Ollama 模型空闲卸载时间（默认 5 分钟）
# 如果内存紧张，可以缩短到 2 分钟
export OLLAMA_KEEP_ALIVE=2m

# 或者在不需要时手动卸载模型
curl http://localhost:11434/api/generate -d '{"model": "qwen2.5-coder:32b-instruct-q4_K_M", "keep_alive": 0}'
```

---

## 八、预期效果

| 指标 | 优化前（纯云端） | 优化后（本地+云端分流） |
|-----|--------------|-------------------|
| Token 消耗 | 100% | **40-60%**（节省 40-60%） |
| 代码补全 | 云端 ~$0.01/次 | 本地 $0/次 |
| Compaction | 云端 ~$0.005/次 | 本地 $0/次 |
| 复杂推理 | 云端 | 云端（不变） |
| 响应速度（简单任务） | ~2s（网络延迟） | ~3-5s（本地推理） |
| 离线可用性 | ❌ | ✅（简单任务可离线） |

---

## 九、快速开始清单

- [ ] `brew install ollama`
- [ ] `ollama serve`（或设置开机自启）
- [ ] `ollama pull qwen2.5-coder:32b-instruct-q4_K_M`（下载 ~20GB）
- [ ] `ollama ps` 确认模型加载，检查内存占用
- [ ] 配置 OpenClaw 的 compaction 和 classification 指向本地模型
- [ ] 配置 Hermes 的代码补全任务指向本地模型
- [ ] 观察 1 周 token 消耗变化，根据实际情况调整分流策略

---

## 十、备选方案

如果发现 32B 模型在你的工作负载下内存压力大（经常 swap），可以降级到：

- **Qwen 2.5 Coder 14B（Q6_K）**：~12GB 内存，速度更快（~25 tok/s），代码能力仍然不错
- **Qwen 2.5 Coder 7B（Q8_0）**：~8GB 内存，速度最快（~40 tok/s），适合纯补全场景

逐步升级的路径：先跑 14B 验证方案可行 → 再切换到 32B 看是否值得。
