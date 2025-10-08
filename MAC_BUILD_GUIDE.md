# 在 Mac 上构建 Windows DLL 的指南

> 虽然无法直接在 Mac 上编译 Windows DLL，但有多种方法可以实现

## 🚫 为什么不能直接在 Mac 上编译

### 技术限制
- ❌ Visual Studio 不支持 macOS
- ❌ Windows API（`windows.h`、GDI+）不存在于 macOS
- ❌ Windows 系统 DLL（`KERNEL32.DLL` 等）无法在 macOS 上链接
- ❌ 交叉编译 Windows DLL 非常复杂且不可靠

## ✅ 可行方案对比

| 方案 | 难度 | 成本 | 推荐度 | 适用场景 |
|------|------|------|--------|----------|
| GitHub Actions | ⭐ | 免费 | ⭐⭐⭐⭐⭐ | 开源项目、团队协作 |
| Parallels Desktop | ⭐⭐ | $99/年 | ⭐⭐⭐⭐⭐ | 个人开发、频繁编译 |
| VirtualBox | ⭐⭐⭐ | 免费 | ⭐⭐⭐⭐ | 个人开发、预算有限 |
| 远程 Windows 机器 | ⭐⭐ | 按需 | ⭐⭐⭐ | 偶尔编译 |
| Azure Pipelines | ⭐⭐ | 免费 | ⭐⭐⭐⭐ | 私有项目、企业 |

---

## 方案 1：GitHub Actions（推荐）

### 优点
- ✅ **完全自动化** - 提交代码自动编译
- ✅ **零成本** - 公开仓库免费
- ✅ **无需本地 Windows 环境**
- ✅ **可重复构建**

### 设置步骤

#### 1. 推送代码到 GitHub

```bash
cd ~/Documents/XMW/electron-vlc-demo

# 初始化 Git（如果还没有）
git init
git add .
git commit -m "Initial commit with WinVLCBridge"

# 创建 GitHub 仓库并推送
git remote add origin https://github.com/你的用户名/electron-vlc-demo.git
git push -u origin master
```

#### 2. 工作流已配置

我已经创建了 `.github/workflows/build-windows-dll.yml`，它会：
- 监听 `WinVLCBridge/` 目录的变化
- 自动在 Windows 环境编译
- 上传编译产物

#### 3. 触发构建

**自动触发：**
```bash
# 修改代码后
git add WinVLCBridge/
git commit -m "Update WinVLCBridge"
git push
```

**手动触发：**
1. 访问 GitHub 仓库
2. 进入 Actions 标签
3. 选择 "Build WinVLCBridge DLL"
4. 点击 "Run workflow"

#### 4. 下载编译产物

1. 进入 GitHub Actions 页面
2. 找到完成的工作流运行
3. 下载 "WinVLCBridge-Release-x64" 产物

```bash
# 解压后得到
WinVLCBridge-Release-x64/
├── WinVLCBridge.dll
├── libvlc.dll
├── libvlccore.dll
└── plugins/
```

### 创建 Release

打标签自动创建 GitHub Release：

```bash
git tag -a v1.0.0 -m "Release version 1.0.0"
git push origin v1.0.0
```

DLL 会自动附加到 Release 中。

---

## 方案 2：Parallels Desktop + Windows（最佳体验）

### 优点
- ✅ **性能优秀** - 接近原生 Windows
- ✅ **无缝集成** - 共享文件夹、剪贴板
- ✅ **即时编译** - 无需等待 CI

### 设置步骤

#### 1. 安装 Parallels Desktop

下载：https://www.parallels.com/

#### 2. 创建 Windows 虚拟机

- 下载 Windows 11 ISO（或使用 Parallels 自动下载）
- 创建虚拟机（建议分配 4+ GB 内存）

#### 3. 安装开发工具

在 Windows 虚拟机中：

```powershell
# 下载 Visual Studio Community
# https://visualstudio.microsoft.com/downloads/

# 安装时勾选：
# - 使用 C++ 的桌面开发
# - Windows 10/11 SDK
```

#### 4. 配置共享文件夹

Parallels Desktop 会自动共享 Mac 文件夹：

```
Windows 中的路径：
Z:\Users\kuo\Documents\XMW\electron-vlc-demo
```

#### 5. 在 Windows 中编译

```cmd
# 在 Windows 虚拟机中
cd Z:\Users\kuo\Documents\XMW\electron-vlc-demo\WinVLCBridge
build.bat release
```

#### 6. 在 Mac 中使用

编译完成后，DLL 立即在 Mac 上可见：

```bash
# 在 Mac 上
ls ~/Documents/XMW/electron-vlc-demo/WinVLCBridge/build/bin/Release/
# WinVLCBridge.dll  libvlc.dll  libvlccore.dll  plugins/
```

---

## 方案 3：VirtualBox + Windows（免费方案）

### 优点
- ✅ **完全免费**
- ✅ **跨平台**

### 缺点
- ⚠️ 性能较差
- ⚠️ 集成度不如 Parallels

### 设置步骤

#### 1. 安装 VirtualBox

```bash
brew install --cask virtualbox
```

或从官网下载：https://www.virtualbox.org/

#### 2. 下载 Windows ISO

从微软官网下载：https://www.microsoft.com/software-download/windows11

#### 3. 创建虚拟机

- 内存：4 GB+
- 磁盘：50 GB+
- 处理器：2 核+

#### 4. 设置共享文件夹

VirtualBox → 设置 → 共享文件夹：

```
文件夹路径：~/Documents/XMW/electron-vlc-demo
文件夹名称：electron-vlc-demo
自动挂载：勾选
```

在 Windows 中访问：`\\VBOXSVR\electron-vlc-demo`

#### 5. 编译

```cmd
cd \\VBOXSVR\electron-vlc-demo\WinVLCBridge
build.bat release
```

---

## 方案 4：远程 Windows 机器

### 使用云服务器

#### AWS EC2 Windows 实例

```bash
# 1. 在 AWS 创建 Windows Server 实例
# 2. 使用 Microsoft Remote Desktop 连接

# 3. 上传代码
scp -r WinVLCBridge/ user@ec2-instance:/path/

# 4. 远程编译
# 5. 下载 DLL
scp user@ec2-instance:/path/WinVLCBridge/build/bin/Release/*.dll ./
```

#### Azure Virtual Machines

类似 AWS，但可能有更好的 Visual Studio 集成。

### 使用实体 Windows 机器

如果你有备用的 Windows 电脑：

```bash
# 在 Mac 上
rsync -avz WinVLCBridge/ windows-pc:/path/to/project/

# SSH 到 Windows（需要安装 OpenSSH）
ssh user@windows-pc
cd /path/to/project/WinVLCBridge
build.bat release

# 下载 DLL
rsync -avz windows-pc:/path/to/project/WinVLCBridge/build/bin/Release/ ./output/
```

---

## 方案 5：使用 UTM（Apple Silicon 专用）

如果你使用 M1/M2/M3 Mac：

### 1. 安装 UTM

```bash
brew install --cask utm
```

或从 App Store 购买（支持开发者）。

### 2. 创建 Windows ARM 虚拟机

- 下载 Windows 11 ARM ISO
- 在 UTM 中创建虚拟机
- 性能优于 x86 模拟

### 3. 注意事项

⚠️ **重要**：Windows ARM 需要：
- Visual Studio 2022（支持 ARM64）
- 可能需要调整 CMakeLists.txt 目标平台

```cmake
# 在 CMakeLists.txt 中
if(CMAKE_SYSTEM_PROCESSOR MATCHES "arm64|ARM64")
    set(CMAKE_GENERATOR_PLATFORM ARM64)
endif()
```

---

## 推荐工作流程

### 日常开发（推荐）

```mermaid
graph LR
    A[Mac 上编写代码] --> B[提交到 Git]
    B --> C[GitHub Actions 自动编译]
    C --> D[下载 DLL]
    D --> E[在 Mac 上测试 Electron 应用]
```

**优点：** 无需本地 Windows 环境

### 频繁编译（推荐）

```mermaid
graph LR
    A[Mac 上编写代码] --> B[在 Parallels 中编译]
    B --> C[立即在 Mac 上测试]
```

**优点：** 快速迭代

---

## 实际示例：完整工作流

### 使用 GitHub Actions

```bash
# 在 Mac 上

# 1. 修改代码
cd ~/Documents/XMW/electron-vlc-demo
code WinVLCBridge/WinVLCBridge.cpp

# 2. 提交变更
git add WinVLCBridge/
git commit -m "Fix: 修复矩形绘制问题"
git push

# 3. 等待 GitHub Actions（约 5-10 分钟）
# 在浏览器中查看进度：
open https://github.com/你的用户名/electron-vlc-demo/actions

# 4. 下载编译产物
# 从 Actions 页面下载，或使用 GitHub CLI：
gh run download

# 5. 解压并使用
unzip WinVLCBridge-Release-x64.zip -d WinVLCBridge/bin/
```

### 使用 Parallels Desktop

```bash
# 在 Mac 上

# 1. 修改代码
cd ~/Documents/XMW/electron-vlc-demo
code WinVLCBridge/WinVLCBridge.cpp

# 2. 在 Parallels 中编译
# （在 Windows 虚拟机中打开终端）
cd Z:\Users\kuo\Documents\XMW\electron-vlc-demo\WinVLCBridge
build.bat release

# 3. 立即在 Mac 上使用
ls build/bin/Release/
# WinVLCBridge.dll 已存在！
```

---

## 常见问题

### Q: GitHub Actions 编译失败怎么办？

**A:** 查看日志：

1. 进入 Actions 页面
2. 点击失败的工作流
3. 查看具体错误信息

常见问题：
- VLC SDK 下载失败 → 检查网络或手动上传 SDK
- CMake 配置失败 → 检查 CMakeLists.txt 路径
- 链接错误 → 确保 VLC SDK 结构正确

### Q: 虚拟机性能太慢怎么办？

**A:** 优化设置：

1. **增加内存**：4 GB → 8 GB
2. **增加 CPU 核心**：2 核 → 4 核
3. **启用嵌套虚拟化**（Parallels/VMware）
4. **使用 SSD** 存储虚拟机文件

### Q: 可以用 Docker 吗？

**A:** 不推荐。理论上可以用 Wine + MinGW，但：

- ❌ 配置极其复杂
- ❌ 兼容性问题多
- ❌ 调试困难
- ❌ 不支持 Visual Studio

### Q: 编译产物可以在所有 Windows 版本运行吗？

**A:** 理论上可以，但需要：

- ✅ Visual C++ 运行库（2015-2022）
- ✅ Windows 7 或更高版本
- ✅ 相应的 VLC DLL

---

## 总结对比

| 方案 | 设置时间 | 编译时间 | 成本 | 适合场景 |
|------|----------|----------|------|----------|
| **GitHub Actions** | 30 分钟 | 5-10 分钟 | 免费 | ⭐ 偶尔编译、开源项目 |
| **Parallels** | 1 小时 | 1-2 分钟 | $99/年 | ⭐⭐⭐ 频繁编译、商业项目 |
| **VirtualBox** | 2 小时 | 2-3 分钟 | 免费 | ⭐⭐ 学习、预算有限 |
| **远程机器** | 可变 | 可变 | 可变 | 偶尔使用 |

---

## 快速决策树

```
需要在 Mac 上构建 Windows DLL？
│
├─ 开源项目？
│  └─ 是 → 使用 GitHub Actions ✅
│
├─ 频繁编译（每天多次）？
│  └─ 是 → 使用 Parallels Desktop ✅
│
├─ 预算有限？
│  └─ 是 → 使用 VirtualBox 或 GitHub Actions ✅
│
└─ 偶尔编译（每月几次）？
   └─ 是 → 使用 GitHub Actions 或远程机器 ✅
```

---

## 下一步

1. **选择方案** - 根据你的需求选择上述方案之一
2. **按照指南设置** - 跟随相应的设置步骤
3. **测试编译** - 运行一次完整的编译流程
4. **集成到工作流** - 将编译集成到日常开发

**推荐起点：** 如果你的项目是开源的，从 GitHub Actions 开始最简单！

---

**提示：** 项目已包含 `.github/workflows/build-windows-dll.yml`，只需推送到 GitHub 即可使用！

