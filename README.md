# MaCAD

MaCAD 是一个基于 **OpenCASCADE (OCCT)**、**bgfx** 和 **Dear ImGui** 的参数化三维 CAD 原型项目。

项目使用自建 bgfx 渲染视口，不使用 Qt，也不使用 OCCT 的 AIS / OpenGL 查看器。当前构建系统为 **CMakePresets.json + Ninja Multi-Config**，依赖主要由 **vcpkg** 提供。

详细里程碑见 [docs/ROADMAP.md](docs/ROADMAP.md)。

## 技术栈

- C++20
- CMake + Ninja Multi-Config
- Visual Studio 2026 / MSVC
- OpenCASCADE 8.0.0
- bgfx / bx / bimg
- Dear ImGui
- GLFW
- glm
- spdlog

## 目录结构

```text
src/app        程序入口、窗口、主循环、各模块装配
src/core       日志、基础类型、MeshData、参数表、装配数据类型
src/geometry   OCCT 封装、基本体、草图转实体、三角化
src/render     bgfx 设备、相机、GPU 网格、shader 程序
src/sketch     2D 草图模型和约束求解器
src/ui         ImGui 层、工具栏、面板、草图视图
src/plugin     命令接口、插件接口、注册表、命令栈
plugins/sample 示例插件
third_party    项目内维护的第三方适配代码
```

模块依赖大致如下：

```text
app      -> ui + render + geometry + plugin + sketch + core
geometry -> core + OCCT
render   -> core + bgfx
ui       -> core + plugin + sketch + ImGui + GLFW
```

几何和渲染之间通过 [core::MeshData](src/core/MeshData.hpp) 解耦。OCCT 生成的 `TopoDS_Shape` 会先由 [geometry::Tessellator](src/geometry/Tessellator.cpp) 转成 `MeshData`，再由 [render::Mesh](src/render/Mesh.cpp) 上传到 GPU。

## 依赖安装

### vcpkg

当前项目直接依赖以下 vcpkg 包：

- `glm`
- `spdlog[fmt]`
- `glfw3`
- `imgui[glfw-binding]`
- `bgfx[multithreaded,tools]`

先设置 `VCPKG_ROOT`，指向你的 vcpkg 根目录：

```powershell
$env:VCPKG_ROOT = "E:\dev\vcpkg"
```

安装命令：

```powershell
& "$env:VCPKG_DIR\vcpkg.exe" install `
  glm:x64-windows `
  spdlog[fmt]:x64-windows `
  glfw3:x64-windows `
  imgui[glfw-binding]:x64-windows `
  "bgfx[multithreaded,tools]:x64-windows"
```

这里列的是直接依赖。vcpkg 会自动安装传递依赖，例如 `fmt`、`lodepng`、`miniz`、`tinyexr`、`libsquish`、`glslang`、`spirv-tools`、`spirv-headers`、`spirv-cross`、`meshoptimizer`、`cgltf` 等。通常不需要手动单独安装这些传递依赖。

`bgfx[tools]` 必须安装，因为构建时会调用 `shaderc.exe` 生成内嵌 shader 头文件。缺少它时，CMake 会报：

```text
bgfx shader compiler was not found
```

### OpenCASCADE

当前预设使用本地预编译 OpenCASCADE，不通过 vcpkg 安装。

需要设置 `OpenCASCADE_DIR`，指向包含 `OpenCASCADEConfig.cmake` 的目录。例如：

```powershell
$env:OpenCASCADE_DIR = "C:\OpenCASCADE\opencascade-8.0.0-vc14-64\cmake"
```

也可以在系统环境变量中设置 `OpenCASCADE_DIR`，或者在 `CMakePresets.json` 中配置对应路径。

## 构建

建议在 **Visual Studio 2026 Developer PowerShell** 或 **Developer Command Prompt** 中执行命令。普通 PowerShell / cmd 可能没有加载 MSVC 和 Windows SDK 环境，会出现类似错误：

```text
Cannot open include file: 'string'
cannot open file 'kernel32.lib'
```

这类错误通常不是源码问题，而是没有进入 VS 开发者环境。

### 配置

```powershell
cmake --preset MaCAD
```

### 编译 Debug

```powershell
cmake --build --preset x64-debug
```

如果输出：

```text
ninja: no work to do.
```

说明目标已经是最新状态，不需要重新编译。

### 编译 Release

```powershell
cmake --build --preset x64-release
```

## 运行

Debug 可执行文件：

```powershell
.\build\MaCAD\bin\Debug\MaCAD.exe
```

Release 可执行文件：

```powershell
.\build\MaCAD\bin\Release\MaCAD.exe
```

也可以在 Visual Studio 的 CMake 视图中选择 `MaCAD.exe` 启动调试。

## 常见问题

### `bgfx shader compiler was not found`

原因：没有安装 `bgfx[tools]`，或者 CMake 缓存里仍保留旧的 `shaderc_EXECUTABLE-NOTFOUND`。

处理：

```powershell
& "$env:VCPKG_ROOT\vcpkg.exe" install "bgfx[multithreaded,tools]:x64-windows" --recurse
cmake --preset MaCAD
```

如果仍然报错，可以删除 `build/MaCAD` 后重新配置。

### `Cannot open include file: 'string'`

原因：没有使用 VS 开发者命令行，MSVC 标准库 include 路径没有加载。

处理：打开 **Visual Studio 2026 Developer PowerShell** 后重新执行构建命令。

### `cannot open file 'kernel32.lib'`

原因：Windows SDK lib 路径没有加载，通常也是没有使用 VS 开发者命令行。

处理：打开 **Visual Studio 2026 Developer PowerShell** 后重新配置和构建。

### vcpkg 下载失败

如果安装依赖时出现 SSL 或代理错误，先确认代理可用。必要时在当前终端设置：

```cmd
set HTTP_PROXY=http://127.0.0.1:6789
set HTTPS_PROXY=http://127.0.0.1:6789
```

然后重新执行 vcpkg 安装命令。

## 当前界面

程序启动后应显示一个 bgfx 渲染视口和多个 ImGui 面板。当前功能包括基础模型显示、草图入口、参数面板、装配相关面板、插件加载入口和统计信息面板。

## 备注

- 当前 shader profile 使用 DX11，对应 bgfx 的 `s_5_0` / `dx11` 输出。
- 构建目录由 `CMakePresets.json` 指定为 `build/MaCAD`。
- 不建议手动修改 `build/` 内文件；配置或依赖变化后重新运行 CMake preset。
