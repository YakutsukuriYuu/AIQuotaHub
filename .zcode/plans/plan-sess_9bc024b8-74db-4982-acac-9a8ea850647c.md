# v0.2 迭代计划：自定义提供商 + 动态 UI + 明暗双主题美化

## 目标（对应你的三个问题）

1. **自定义 URL + Key**：提供商不再写死，用户可添加任意 HTTP 数据源
2. **界面动态化**：运行期增删改/启停提供商，卡片自动重建
3. **美化**：跟随 macOS 系统明暗的双主题 + 卡片重设计（自绘圆环进度）

## 一、解析模板化（解决问题 1 的核心）

`ProviderConfig` 扩展：
- `parserTemplate`：`glm_quota` / `deepseek_balance` / `moonshot_balance` / `openai_costs` / `custom_json`
- `source`：`builtin`（可停用、可改 Key，不可删）/ `user`（可删改）
- `fields`：custom_json 时存字段路径映射；模板型时作高级覆盖

新增解析器（均为纯函数 + 单测夹具）：
- **MoonshotParser**：`GET /v1/users/me/balance` → `data.available_balance`（Kimi）
- **OpenAiCostsParser**：`GET /v1/organization/costs` 聚合当月 bucket → "本月花费"（需 Admin Key）；endpoint 支持 `{month_start}` 占位符，fetch 时替换为当月 1 号时间戳
- **CustomJsonParser**：按用户填的路径映射解析（quotasArray / used / limit / reset / balance 路径）

`HttpProvider` 通用化：一个类按模板绑定解析函数，**吸收并删除** GlmProvider/DeepSeekProvider；工厂简化为 `type: demo | http`。内置 providers.json 补全 glm / deepseek / kimi / openai 四个模板源 + demo。

## 二、提供商管理（解决问题 2）

- 新增 `ProviderManager`（core）：持有生效配置 → 持久化用户层 JSON（`~/Library/Application Support/AIQuotaHub/providers.json`，与内置合并、同 id 覆盖）→ 重建 Provider 实例 → 发 `providersChanged()`
- 设置对话框重构为**提供商管理页**：每行一个提供商（名称、模板、启用开关、Key 状态、编辑/删除）；「添加提供商」对话框：名称、模板下拉（带说明）、URL（按模板预填）、API Key（入钥匙串）、刷新间隔、**高级折叠区**（字段路径映射）
- MainWindow 监听 `providersChanged()` → 重建卡片/侧栏/调度器

## 三、美化：跟随系统明暗（解决问题 3）

- 新增 `Theme` 模块：深/浅两套调色板 token；`QStyleHints::colorScheme()` 检测 + `colorSchemeChanged` 实时切换；生成全套 QSS（窗口/工具栏/列表/对话框/输入框/按钮/滚动区/菜单）
- 新增 `RingProgress` 自绘控件：圆环弧 + 中心大数字百分比（QPainter 抗锯齿，状态色驱动）
- `ProviderCard` 重设计：左侧 4px 状态色条 + 双圆环（5h/周）+ 重置时间 + 余额行 + hover 提亮 + 失败灰化
- 托盘图标颜色改取 Theme 状态色

## 文件变动

| 新增 | 删除 | 大改 |
|---|---|---|
| MoonshotParser / OpenAiCostsParser / CustomJsonParser (h+cpp) | GlmProvider (h+cpp) | SettingsDialog（→提供商管理）|
| HttpProvider (h+cpp) | DeepSeekProvider (h+cpp) | ProviderCard（圆环重设计）|
| ProviderManager (h+cpp) | | MainWindow（动态重建）|
| Theme / RingProgress (h+cpp) | | providers.json（模板字段+新源）|
| AddProviderDialog (h+cpp) | | README |
| fixtures: kimi_balance / openai_costs | | |

## 实施顺序（5 次提交）

1. `feat(parsers): 解析模板化 + Kimi/OpenAI/自定义JSON 解析器`——先写解析器和单测，ctest 绿
2. `feat(core): HttpProvider 通用化 + ProviderManager + 用户层持久化`——编译绿
3. `feat(ui): 提供商管理页（运行期增删改启停）`——编译绿
4. `feat(theme): 明暗双主题 + 圆环进度卡片重设计`——编译零警告
5. `docs: README v0.2`——ctest 全绿 + offscreen 冒烟后提交

## 验证

- 步骤 1 后 ctest（新增 6+ 解析用例）；步骤 5 前完整 ctest + offscreen 冒烟 6 秒
- **推送到 GitHub**：等你在 https://github.com/settings/ssh/new 注册公钥后告诉我，我把 4+5 个提交一起推上去

## 边界说明

- 用户自定义源只保证"模板匹配"的响应能解析；字段漂移靠高级覆盖的路径映射兜底
- OpenAI 模板需要 Admin API Key（普通 sk- key 无权限查 costs），设置页会注明
- 卡片拖拽排序、图标头像不在本轮（如需下轮加）