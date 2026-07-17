# AI Quota Hub

macOS 桌面应用：一站式查看各家 AI 的**订阅额度**（5 小时窗 / 周窗）与 **API 余额/用量**。

基于 Qt 6.11.1（Widgets + C++20），数据卡片式仪表盘 + 菜单栏托盘驻留。

## 当前功能（v0.1）

- 卡片仪表盘：5 小时额度 / 周额度进度条、重置时间、API 余额；按用量着色（<60% 绿 / 60–85% 黄 / >85% 红 / 失败灰）
- 数据源适配器：
  - **GLM 智谱**：`GET https://open.bigmodel.cn/api/monitor/usage/quota/limit`（配额）
  - **DeepSeek**：`GET https://api.deepseek.com/user/balance`（余额）
  - **Demo 演示**：模拟数据，无 Key 也能看界面效果
- 定时自动刷新（每源可配间隔），失败指数退避（×2，上限 15 分钟）
- 菜单栏托盘：图标颜色 = 当前最紧张的额度；菜单含摘要 / 立即刷新 / 退出；关窗不退出
- 设置对话框录入 API Key，存 **macOS 钥匙串**（不落盘明文）
- 解析器单元测试（Qt Test + 响应夹具，不依赖网络和真实 Key）

## 构建

无需全局安装 cmake/ninja，使用 Qt 自带的工具链（路径已固化在 `CMakePresets.json`）：

```bash
# 配置（仅需一次）
~/Qt/Tools/CMake/CMake.app/Contents/bin/cmake --preset default

# 构建
~/Qt/Tools/CMake/CMake.app/Contents/bin/cmake --build build

# 测试
~/Qt/Tools/CMake/CMake.app/Contents/bin/ctest --test-dir build --output-on-failure
```

产物：`build/AIQuotaHub.app`

## 运行

```bash
open build/AIQuotaHub.app
```

首次运行：工具栏「设置」中填入 GLM / DeepSeek 的 API Key，保存后自动重刷。
未填 Key 的源显示「未配置 API Key」；Demo 源始终有模拟数据。

## 配置（providers.json）

所有接口地址、认证头、刷新间隔、字段映射都在 `resources/providers.json`，**接口变动时改配置即可，不用改代码**。

加载优先级（高 → 低）：

1. 环境变量 `$AIQUOTAHUB_PROVIDERS_JSON`
2. `~/Library/Application Support/AIQuotaHub/providers.json`（用户覆盖）
3. App 包内 `Contents/Resources/providers.json`（随包默认）

新增提供商：在 JSON 加配置 + 在 `src/providers/` 写一个 `Provider` 子类（参考 `GlmProvider`，解析写成纯函数便于测试），并在 `MainWindow.cpp` 的 `createProvider()` 工厂注册。

## 目录结构

```
src/
  main.cpp
  core/        # Models（统一数据模型）、Provider 抽象、HttpJsonClient、
               # ProvidersConfig（JSON 配置）、RefreshScheduler（定时+退避）、
               # CredentialStore（macOS 钥匙串）
  providers/   # Glm / DeepSeek / Demo 适配器；*Parser.cpp 为纯函数解析器
  ui/          # MainWindow、ProviderCard、SettingsDialog、TrayIcon
resources/     # providers.json
tests/         # Qt Test 解析测试 + fixtures 夹具
```

## 已知边界 / 后续路线

- **GLM 配额字段名按公开资料实现并做了候选容忍**；首次用真实 Key 请求后如有偏差，改 `providers.json` 的 `fields` 或 `GlmParser.cpp` 的候选键即可
- M3：Kimi（月之暗面余额 + 订阅额度待验证）、OpenAI（需 Admin Key）、SQLite 历史 + Qt Charts 趋势页
- M4：Claude 本地日志（`~/.claude`）解析、macdeployqt 打包分发；如需 OAuth 流程，用 `~/Qt/MaintenanceTool.app` 补装 Qt Network Authorization
