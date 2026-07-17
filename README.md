# AI Quota Hub

macOS 桌面应用：一站式查看各家 AI 的**订阅额度**（5 小时窗 / 周窗）与 **API 余额/用量**。

基于 Qt 6.11.1（Widgets + C++20），圆环卡片仪表盘 + 菜单栏托盘驻留，跟随系统明/暗主题。

## 功能（v0.2）

### 仪表盘
- 圆环进度卡片：5 小时额度 / 周额度（环内百分比 + 用量小字）、重置时间、API 余额；左侧状态色条（<60% 绿 / 60–85% 黄 / >85% 红 / 失败灰），hover 提亮
- **跟随 macOS 系统外观的明/暗双主题**，实时切换
- 菜单栏托盘：图标颜色 = 最紧张的额度；摘要 / 立即刷新 / 退出；关窗不退出
- 定时自动刷新（每源可配间隔），失败指数退避（×2，上限 15 分钟）

### 提供商：模板预设 + 自定义
内置 5 个数据源（设置页可启停）：

| 源 | 模板 | 数据 |
|---|---|---|
| GLM 智谱 | `glm_quota` | 5 小时/周配额 |
| DeepSeek | `deepseek_balance` | 余额 |
| Kimi 月之暗面 | `moonshot_balance` | 余额 |
| OpenAI | `openai_costs` | 本月花费（需 **Admin Key**） |
| Demo 演示 | — | 模拟数据 |

**自定义添加**：设置页「＋ 添加提供商」→ 选解析模板 → 填名称/URL/API Key/刷新间隔即可接入任意"Bearer Key + JSON"接口；`custom_json` 模板还可在高级区自定义 JSON 字段路径映射（quotasArray/used/limit/reset/balance/currency）。

- 用户自建源保存在 `~/Library/Application Support/AIQuotaHub/providers.json`（可编辑/删除）
- API Key 一律存 **macOS 钥匙串**，不落盘
- 接口字段漂移时：模板源可改 JSON 里的 `fields` 覆盖，无需改代码

## 构建

无需全局安装 cmake/ninja，使用 Qt 自带工具链（已固化在 `CMakePresets.json`）：

```bash
# 配置（仅需一次）
~/Qt/Tools/CMake/CMake.app/Contents/bin/cmake --preset default

# 构建
~/Qt/Tools/CMake/CMake.app/Contents/bin/cmake --build build

# 测试（12 个解析用例）
~/Qt/Tools/CMake/CMake.app/Contents/bin/ctest --test-dir build --output-on-failure
```

产物：`build/AIQuotaHub.app`，运行 `open build/AIQuotaHub.app`。

## 架构

```
src/
  main.cpp
  core/        # Models（统一数据模型）、Provider 抽象、ProviderManager（配置+实例生命周期）、
               # ProvidersConfig（内置+用户层合并加载/保存）、HttpJsonClient、
               # RefreshScheduler（定时+退避）、CredentialStore（macOS 钥匙串）
  providers/   # HttpProvider（按模板分发解析）、DemoProvider；
               # *Parser.cpp 均为纯函数解析器（glm/deepseek/moonshot/openai/custom）
  ui/          # MainWindow、ProviderCard、RingProgress、Theme（明暗双主题）、
               # SettingsDialog（提供商管理）、AddProviderDialog、TrayIcon
resources/     # providers.json（内置源定义，打进 App 包 Resources）
tests/         # Qt Test 解析测试 + fixtures 夹具
```

数据流：`providers.json`（内置）+ 用户层 JSON → `ProviderManager` 合并 → `HttpProvider.fetch()` → 模板纯函数解析 → `ProviderSnapshot` → 卡片/托盘。

## 已知边界 / 后续路线

- GLM 配额字段名按公开资料实现并做候选容忍；真实响应若有偏差，改 `GlmParser.cpp` 候选键或 JSON `fields` 即可
- OpenAI 模板需要组织 **Admin API Key**（普通 sk- key 无权限查 costs）
- Kimi 编程订阅的 5h 配额暂无公开 API（余额已支持；配额待官方接口或抓包验证）
- 路线：Claude 本地日志（`~/.claude`）解析、SQLite 历史 + Qt Charts 趋势详情页、卡片拖拽排序、macdeployqt 打包分发
