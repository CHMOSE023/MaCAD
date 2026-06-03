# MaCAD 路线图 / Roadmap

> 参数化三维 CAD 平台。本文件是项目的**活文档**,随代码演进更新。

## 1. 目标与范围

最终目标:支持**草图、约束、特征树、参数驱动建模、装配与插件扩展**的参数化 3D CAD 平台。

- **几何内核**:OpenCASCADE (OCCT) 8.0.0 —— 仅用其建模内核。
- **渲染**:bgfx 自建渲染管线(D3D11/Vulkan/...),**不使用** OCCT AIS / OpenGL Viewer。
- **UI**:Dear ImGui + GLFW(无 Qt)。
- **数学/日志**:glm、spdlog。
- **构建**:C++20 + VS + CMake(FetchContent 管理依赖)。

分层依赖规则(编译期强制):
```
app → { ui, render, geometry, core, plugin }
geometry  ← 唯一直接 include OCCT 的层
render/ui ← 不得 include OCCT,仅通过 core::MeshData 等纯数据契约通信
```

## 2. 模块结构

```
src/
├─ core/      日志(Log)、强类型(Types)、MeshData;Parameter/Document 接口(预留)
├─ geometry/  OCCT 封装:Shape / Primitives(MakeBox) / Tessellator(BRepMesh→MeshData)
├─ render/    bgfx:Renderer / Camera(轨道相机) / Mesh / shaders(*.sc)
├─ ui/        ImGuiLayer + Panels(特征树/工具栏/FPS 占位)
├─ plugin/    IPlugin / ICommand / PluginRegistry(静态注册;动态加载预留)
└─ app/       Application(主循环) + main.cpp
cmake/        Dependencies / OCCT / Shaders 辅助
third_party/  imgui_bgfx(vendored 的 bgfx ImGui 渲染后端)
```

## 3. 里程碑状态

> 状态依据:逐文件阅读源码核实,且这些文件均已编译进成功的构建。
> ✅=已实现并接入 · ◑=部分实现 · ▢=未开始。**"已实现"指代码完成且可编译,不等于已逐项运行验证正确性**。

### ✅ M1 — 渲染管线 + 几何内核打通
OCCT 建 box → 三角化 → bgfx(D3D11)渲染 → ImGui 面板;鼠标可轨道旋转/缩放,工具栏 "Create Box" 可参数化重建。已运行验证。

### ✅ M2 — 2D 草图 + 约束求解器
- `sketch::Sketch`:点/线/圆/弧 + 9 种约束(重合/水平/垂直/平行/相切/相等/距离/半径/角度),扁平 DOF 变量模型。
- `sketch::Solver`:Gauss-Newton + 稠密线性解(部分主元)+ 有限差分雅可比。
- `ui::SketchView`(~590 行):草图模式下的绘制/拾取/拖拽/选择;草图模式切换已接入 `Application`。

### ✅ M3 — 拉伸/旋转特征 + 特征树
- `geometry::SketchToShape`:闭合轮廓 → `BRepPrimAPI_MakePrism`(拉伸)/ `MakeRevol`(旋转)。
- `Application` 的 `Feature` 列表 + `onExtrude/onRevolve/rebuildFeature`;Panels 中 **Feature Tree** 面板(选择/显示)。
- 待补:布尔特征(凸台/切除/打孔)、特征重排。

### ✅ M4 — 参数系统与依赖重算
- `core::ParameterTable`:命名参数 + `resolve()`(数值/参数名解析,特征尺寸可用参数表达式驱动)。
- `recompute()` 遍历特征重建;`plugin::CommandStack` 撤销/重做(Ctrl+Z/Y + Undo/Redo 按钮)。
- 待补:完整表达式求值、参数间依赖 DAG 与脏标记拓扑重算。

### ✅ M5 — 装配(多实例组件 + 迭代配合求解器)
- **组件实例** `core::Component`:同一零件(feature)可多次实例化,各自 6-DOF 位姿 + 接地(grounded)锚点;`Components` 面板可添加/删除/接地/编辑位姿。
- **配合** `core::Mate`:Coincident(重合)、Distance(轴向距离)、Concentric(同轴)、Parallel(平行)、Angle(角度),数值可用参数表达式驱动;`Mates` 面板增删改 + 选轴。
- **迭代求解器** `Application::solveMates()`:朝向 pass(确定性)+ 平移 pass(Gauss-Seidel 松弛,最多 200 次/容差 1e-7),接地为固定帧;输出收敛状态 + 残差 + 平移 DOF 估计(欠/完全/过约束)。
- **渲染**:装配模式下按各组件求解后的世界矩阵复用零件 mesh 绘制多实例。
- 关键文件:`core/AsmTypes.hpp`、`app/Application.cpp`(solveMates)、`ui/Panels.cpp`(Components/Mates 面板)。
- 旧的特征级 `AsmConstraint`/Assembly 面板保留(单实例定位),新组件系统为完整装配主线。
- 待补(M5+):旋转配合的完整 6-DOF 数值求解、装配树层级、零件多文档。

### ✅(基础) M6 — 插件动态加载
- `PluginRegistry::loadPluginLibrary`:`LoadLibraryW`/`dlopen` + `GetProcAddress` 解析 `macadCreatePlugin` 工厂(跨平台);`PluginApi.hpp` 定义 ABI。
- 待补:稳定 ABI 版本协商、插件命令/面板的完整注册与生命周期管理。

## 4. 贯穿设计的复用点
- **数据契约 `core::MeshData`**:任何 `TopoDS_Shape`(草图/特征/装配)都复用同一 `Tessellator` + `Mesh` 上传路径。
- **命令模式 `ICommand`**:服务于工具栏按钮、插件、撤销/重做、参数重算触发的统一入口。

 