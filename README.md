# MaCAD

一个基于**自建渲染视口**的参数化三维 CAD 平台。几何内核采用 **OpenCASCADE (OCCT)**,渲染完全构建在 **bgfx** 之上(不使用 Qt、不使用 OCCT 的 AIS / OpenGL 查看器)。

> **进度** — M1–M4 已实现,M5 完整装配已实现,M6 插件动态加载已实现(基础)。
> 详细里程碑与状态见 📍 [docs/ROADMAP.md](docs/ROADMAP.md)。

## 技术栈

C++20 · OpenCASCADE 8.0.0 · bgfx · Dear ImGui · GLFW · glm · spdlog · CMake(Ninja Multi-Config)

## 架构

```
app  ──▶ ui · render · geometry · plugin · sketch · core
geometry ──▶ core + OCCT      (唯一引入 OCCT 的层)
render/ui ──▶ core            (禁止引入 OCCT)
```

几何与渲染通过唯一的数据契约 [`core::MeshData`](src/core/MeshData.hpp) 解耦:任意 `TopoDS_Shape`
经 [`geometry::Tessellator`](src/geometry/Tessellator.cpp) 三角化为 `MeshData`,再由
[`render::Mesh`](src/render/Mesh.cpp) 上传到 GPU。所有用户操作统一走
[`ICommand`](src/plugin/ICommand.hpp) 接口——插件(M6)也复用同一套。

| 层 | 路径 | 职责 |
|-----|------|------|
| core | `src/core` | 日志、类型、`MeshData`、参数表、装配数据类型 |
| geometry | `src/geometry` | OCCT 封装:基本体、三角化、草图→实体 |
| sketch | `src/sketch` | 2D 参数化草图模型 + 约束求解器 |
| render | `src/render` | bgfx 设备、相机、网格缓冲、着色器 |
| ui | `src/ui` | ImGui 层 + 面板(工具栏/特征树/草图视图/装配等) |
| plugin | `src/plugin` | 命令与插件接口、注册表、命令栈(撤销/重做) |
| app | `src/app` | 窗口、主循环、各层装配 |

## 依赖

`bgfx.cmake`、GLFW、glm、spdlog、Dear ImGui 由 CMake `FetchContent` 自动拉取
(见 [`cmake/Dependencies.cmake`](cmake/Dependencies.cmake))。

**OCCT** 体量大,单独处理(见 [`cmake/OCCT.cmake`](cmake/OCCT.cmake)),通过 `MACAD_OCCT_SOURCE` 提供两种模式:

- `prebuilt`(默认,推荐):使用已安装的 OCCT SDK。将环境变量或 CMake 缓存变量
  `OpenCASCADE_DIR` 指向 SDK 的 `cmake/` 目录(即包含 `OpenCASCADEConfig.cmake` 的目录)。
- `fetch`:通过 FetchContent 从源码编译 OCCT。**首次构建预计 30–60 分钟以上。**

## 构建(Windows · Visual Studio 2022 · Ninja)

> ⚠️ 生成器是 **Ninja Multi-Config**,它不会自动设置编译器环境。
> 请在 **「VS 2022 开发者 PowerShell / 开发者命令提示符」** 中执行(或直接在 VS 里构建),
> 否则会报 `cstdint` / `string` 找不到——那是缺少 MSVC 的 `INCLUDE` 环境,并非代码错误。

```powershell
# 1. 指向预编译的 OCCT 8.0.0 SDK
#    (从 V8_0_0 的 GitHub Release 下载 occt-combined-with-debug-no-pch.zip,
#     解压后确保 <根目录>\cmake\OpenCASCADEConfig.cmake 存在)
$env:OpenCASCADE_DIR = "C:\OpenCASCADE\opencascade-8.0.0-vc14-64\cmake"

# 2. 配置 + 构建(注意:配置预设名是 MaCAD,构建预设名是 x64-debug)
cmake --preset MaCAD
cmake --build --preset x64-debug

# 3. 运行(无需手动设置 PATH)
.\build\MaCAD\bin\Debug\MaCAD.exe
```

**关于运行时 DLL**:构建后会有一个 POST_BUILD 步骤,自动把 OCCT 工具包 DLL 与第三方依赖
(jemalloc、freetype 等,来自同级的 `3rdparty-vc14-64`)拷到 exe 旁边,因此可**直接双击运行**,
无需配置 PATH。`run.bat` 仍可用作备选。

如需从源码编译 OCCT,在 `CMakePresets.json` 的缓存变量中设置 `MACAD_OCCT_SOURCE=fetch`。

### 首次搭建踩坑记录

- **bgfx 子模块**:bgfx(bgfx/bx/bimg)作为 `bgfx.cmake` 的 git 子模块拉取。
  网络不稳时 CMake 内的 clone 会失败。可预先在 `.deps/bgfx.cmake` 放一份
  本地副本(带子模块的浅克隆),[`cmake/Dependencies.cmake`](cmake/Dependencies.cmake) 会自动识别复用。
- **着色器包含路径**:源码构建下 `BGFX_SHADER_INCLUDE_PATH` 为空,shaderc 找不到
  `bgfx_shader.sh`;已通过 `MACAD_BGFX_SHADER_INCLUDE` 显式传入(见
  [`cmake/Shaders.cmake`](cmake/Shaders.cmake))。
- **渲染后端**:当前内嵌 DX11(`s_5_0`)着色器 profile 并强制 Direct3D11 后端;
  多后端需在 [`cmake/Shaders.cmake`](cmake/Shaders.cmake) 中编译更多 profile。
- **构建提速**:已关闭未用的 bgfx 工具(geometry/texture/viewers)、开启 `/MP` 并行与
  `BGFX_AMALGAMATED`;首次构建仍需编译一次 shaderc(glslang/spirv),属一次性成本。

## 你应当看到的画面

视口中央一个带光照的立方体,可用鼠标轨道旋转(左键拖拽)、平移(中键/右键拖拽)、
缩放(滚轮)。ImGui 提供:**工具栏**(含 *Create Box* / *Sketch* 等命令)、**特征树**、
**Components / Mates**(装配)、**Parameters**(参数)、**Plugins**(插件)、**Stats**
(后端、FPS、顶点/三角形数)等面板。

## 路线图

M2 草图 + 约束求解器 · M3 拉伸/旋转特征 + 特征树 · M4 参数系统 + 依赖重算 ·
M5 多实例装配 + 配合求解器 · M6 插件动态加载。各层模块与接口边界均已就位。
完整状态见 [docs/ROADMAP.md](docs/ROADMAP.md)。
