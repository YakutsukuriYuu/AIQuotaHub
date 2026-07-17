# AI Quota Hub — 第一迭代实施计划（M1+M2：可运行的仪表盘，git 全程管理）

## 交付目标

在当前空目录 `/Users/yakutsukuriyuu/Documents/zcode/qt` 下从零构建 **macOS 原生 Qt Widgets 应用 `AIQuotaHub.app`**，目录即项目根并初始化为 git 仓库：

- 卡片式仪表盘：每卡片显示 5小时额度 / 周额度进度条 + 重置时间 + API 余额，按用量着色（<60% 绿 / 60–85% 黄 / >85% 红 / 失败灰）
- 真实数据源：**GLM 智谱**（官方配额 API）+ **DeepSeek**（官方余额 API）两个适配器
- **Demo 适配器**：生成模拟数据，无 API Key 也能看到完整界面效果
- 菜单栏托盘图标（显示最紧张的额度）+ 手动/定时自动刷新（失败指数退避）
- 设置对话框：填 API Key，存 **macOS 钥匙串**（不落盘明文）
- 解析逻辑带 **Qt Test 单元测试**（真实响应样例做夹具，不依赖网络和真 Key）

不在本轮（后续里程碑）：Kimi/OpenAI/Claude 适配器、SQLite 历史 + Qt Charts 趋势详情页、macdeployqt 打包、NetworkAuth（需 MaintenanceTool 补装）。

## git 管理策略

- **P0 第一步先 `git init`**，后续每个阶段完成即提交，共 4 次里程碑提交：
  1. `chore: 项目骨架（CMake/presets/gitignore/空窗口）`
  2. `feat(core): 数据模型 + HTTP 客户端 + GLM/DeepSeek 解析（测试全绿）`
  3. `feat(ui): 仪表盘卡片 + 托盘 + 设置页 + 钥匙串`
  4. `docs: README 构建运行说明`（含最终验证后修正）
- `.gitignore`：`build/`、`*.user`、`.DS_Store`、`.qtcreator/`、macOS 无关产物
- 只提交，不关联远程、不 push

## 技术要点

- Qt 6.11.1，模块：Core / Gui / Widgets / Network / Test（Charts、Sql 后续里程碑再加）
- CMake + Ninja，用 Qt 自带工具链绝对路径；**CMakePresets.json** 固化配置
- endpoint、认证头、JSON 字段路径写进 `resources/providers.json` → 接口变动改配置不重编译
- 解析函数写成纯函数（QByteArray → Model），与网络层分离，可单测

## 文件清单（约 23 个文件）

```
.gitignore
CMakeLists.txt                    # MACOSX_BUNDLE，find_package(Qt6 Widgets Network Test)
CMakePresets.json                 # 固化 Ninja/Qt 路径
README.md
resources/providers.json          # GLM/DeepSeek 端点+字段映射
resources/fixtures/*.json         # 测试夹具（GLM配额、DeepSeek余额真实响应样例）
src/main.cpp
src/core/
  Models.h/.cpp                   # QuotaWindow(5h/Weekly) / ApiUsage / ProviderSnapshot
  Provider.h/.cpp                 # 抽象接口 + 注册表
  HttpJsonClient.h/.cpp           # QNAM 异步 GET + Bearer + JSON
  ProvidersConfig.h/.cpp          # 加载 providers.json
  RefreshScheduler.h/.cpp         # QTimer 定时 + 失败退避(×2, 上限15min)
  CredentialStore.h/.cpp          # Security framework 钥匙串封装
src/providers/
  GlmProvider.h/.cpp
  DeepSeekProvider.h/.cpp
  DemoProvider.h/.cpp             # 模拟数据
src/ui/
  MainWindow.h/.cpp               # 侧栏 + 卡片滚动区
  ProviderCard.h/.cpp             # 进度条/余额/状态色
  SettingsDialog.h/.cpp           # Key 录入 → 钥匙串
  TrayIcon.h/.cpp
tests/
  CMakeLists.txt
  test_parsers.cpp
```

## 实施顺序（与提交对应）

1. **P0**：git init → CMakeLists + presets + .gitignore + 空窗口 → 编译通过 → **提交①**
2. **P1**：Models → HttpJsonClient → GLM/DeepSeek 解析 → **ctest 全绿** → **提交②**
3. **P2**：ProviderCard → MainWindow → Scheduler → SettingsDialog + 钥匙串 → TrayIcon → **提交③**
4. **P3**：完整构建 → ctest → offscreen 冒烟启动 → README → **提交④**

## 验证命令

- `~/Qt/Tools/CMake/CMake.app/Contents/bin/cmake --preset default` + `cmake --build --preset default`
- `ctest --test-dir build`
- `QT_QPA_PLATFORM=offscreen ./build/AIQuotaHub.app/Contents/MacOS/AIQuotaHub` 冒烟（数秒后主动结束）

## 你需要知道的

- 构建、提交全程无需你操作；跑真实数据需在 app 设置页填 GLM/DeepSeek 的 API Key（没有也不影响构建和 Demo 演示）
- git 只做本地提交；要关联 GitHub/Gitee 远程仓库的话随时说