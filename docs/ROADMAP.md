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

### ✅ M1 — 渲染管线 + 几何内核打通(已完成)

OCCT 建 box → 三角化 → bgfx(D3D11)渲染 → ImGui 面板;鼠标可轨道旋转/缩放,工具栏 "Create Box" 可参数化重建。整条技术栈端到端验证通过。
  
### ▢ M2 — 2D 草图 + 约束求解器(下一步)
- 草图平面与 2D 几何(点/线/圆/弧),屏幕拾取与拖拽。
- 几何约束(重合/水平/垂直/平行/相切/相等)+ 尺寸约束(距离/半径/角度)。
- 求解器:从草图约束建立方程组,数值迭代(牛顿法/最小二乘)求解自由度。
- UI:草图模式工具栏、约束面板、欠/过约束状态提示。

### ▢ M3 — 拉伸/旋转特征 + 特征树
- 由草图轮廓生成实体:`BRepPrimAPI_MakePrism`(拉伸)、`MakeRevol`(旋转)。
- 布尔特征(`BRepAlgoAPI_*`):凸台/切除/打孔。
- 特征树数据模型 + 面板:增删改、重排、回溯重算。

### ▢ M4 — 参数系统与依赖重算
- 实现 `core::Parameter`:命名参数、表达式求值、参数间引用。
- 依赖图(DAG)+ 脏标记 + 拓扑重算:改一个尺寸,下游特征自动更新。
- 撤销/重做(基于 `ICommand`)。

### ▢ M5 — 装配
- 多文档/零件实例、装配树。
- 装配约束(贴合/对齐/同轴)与定位求解。

### ▢ M6 — 插件动态加载
- 动态库(.dll)发现/加载,`IPlugin` 入口,命令/面板注册。
- 稳定 ABI 边界与版本协商。

## 4. 贯穿设计的复用点
- **数据契约 `core::MeshData`**:任何 `TopoDS_Shape`(草图/特征/装配)都复用同一 `Tessellator` + `Mesh` 上传路径。
- **命令模式 `ICommand`**:服务于工具栏按钮、插件、撤销/重做、参数重算触发的统一入口。

 