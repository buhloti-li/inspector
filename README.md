# Industrial Workcell Runtime System

工业工作单元运行时系统 — 基于 Qt6/C++17 的工业自动化拾取工作站完整解决方案。

## 功能概览

| 模块 | 说明 |
|------|------|
| **WorkcellController** | 核心控制器，管理状态机、任务调度、自动模式 |
| **DeviceManager** | 设备抽象层，支持相机/机器人/PLC 驱动注册与管理 |
| **VisionPipeline** | 3D 点云视觉处理，物体检测（可扩展 ONNX 模型） |
| **GraspPlanner** | 抓取规划，生成多候选位姿并评分排序 |
| **TrajectoryPlanner** | 运动轨迹规划，完整的 Pick-and-Place 路径生成 |
| **WorkflowEngine** | 节点图工作流引擎，支持条件分支 |
| **RecoveryManager** | 故障恢复管理，可配置的错误处理策略链 |
| **WorkcellServer** | TCP 服务器，JSON 协议，实时状态推送 |
| **Mobile Client** | iOS/Android 移动监控客户端（Qt Quick） |

## 项目结构

```
├── src/
│   ├── workcellcontroller.h/.cpp    # 核心控制器
│   ├── device/
│   │   ├── idevicedriver.h          # 设备驱动接口
│   │   ├── icameradriver.h          # 相机驱动接口
│   │   ├── irobotdriver.h           # 机器人驱动接口
│   │   ├── iplcdriver.h             # PLC 驱动接口
│   │   ├── devicemanager.h/.cpp     # 设备管理器
│   │   └── sim/                     # 模拟驱动
│   │       ├── simcameradriver.h/.cpp
│   │       └── simrobotdriver.h/.cpp
│   ├── vision/
│   │   └── visionpipeline.h/.cpp    # 视觉管线
│   ├── planning/
│   │   ├── graspplanner.h/.cpp      # 抓取规划
│   │   └── trajectoryplanner.h/.cpp # 轨迹规划
│   ├── orchestration/
│   │   ├── workflowengine.h/.cpp    # 工作流引擎
│   │   └── recoverymanager.h/.cpp   # 故障恢复
│   ├── server/
│   │   └── workcellserver.h/.cpp    # TCP 服务器
│   └── mobile/
│       ├── mobileclient.h/.cpp              # 网络客户端
│       ├── workcellstatusprovider.h/.cpp     # 状态提供者
│       ├── remotecommandservice.h/.cpp       # 远程命令
│       ├── alertnotificationservice.h/.cpp   # 告警通知
│       └── devicelistmodel.h/.cpp            # 设备列表模型
├── tests/                           # 15 个测试套件，480+ 测试用例
├── server/main.cpp                  # 服务器入口
├── mobile/                          # iOS/Android 移动 App
│   ├── main.cpp
│   ├── CMakeLists.txt
│   ├── qml/                         # Qt Quick 界面
│   └── ios/                         # iOS 配置
├── deploy/
│   ├── setup.sh                     # 一键部署脚本
│   └── industrial-server.service    # systemd 服务
├── Dockerfile                       # Docker 容器化
├── CMakeLists.txt                   # 构建配置
└── .github/workflows/ci.yml        # CI/CD
```

## 环境要求

- **C++ 17** 或更高
- **Qt 6.5+**（Core, Network, Test 模块）
- **CMake 3.21+**
- **Ninja**（推荐）或 Make

### Ubuntu / Debian

```bash
sudo apt-get install -y cmake ninja-build g++ qt6-base-dev qt6-base-dev-tools
```

### macOS

```bash
brew install cmake ninja qt@6
```

## 编译与测试

```bash
# 配置
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug

# 编译（含服务器）
cmake -S . -B build -G Ninja -DBUILD_SERVER=ON
cmake --build build --parallel

# 运行全部测试
ctest --test-dir build --output-on-failure
```

### 测试覆盖

| 测试套件 | 说明 |
|----------|------|
| runtime_tests | 核心控制器 |
| test_devicemanager | 设备管理 |
| test_simdrivers | 模拟驱动 |
| test_visionpipeline | 视觉管线 |
| test_graspplanner | 抓取规划 |
| test_trajectoryplanner | 轨迹规划 |
| test_workflowengine | 工作流引擎 |
| test_recoverymanager | 故障恢复 |
| test_integration_pickcycle | 集成测试 |
| test_mobileclient | 移动客户端 |
| test_workcellstatusprovider | 状态提供者 |
| test_remotecommandservice | 远程命令 |
| test_alertnotificationservice | 告警通知 |
| test_devicelistmodel | 设备列表 |
| test_workcellserver | TCP 服务器 |

共计 **15 个测试套件，480+ 测试用例，100% 通过**。

## 服务器运行

```bash
# 使用模拟设备启动
./build/industrial-server --sim --port 9600

# 指定端口
./build/industrial-server --port 8080
```

### TCP 协议

服务器使用换行分隔的 JSON 协议：

**请求格式：**
```json
{"cmd":"status","params":{},"id":1}
```

**响应格式：**
```json
{"id":1,"status":"ok","data":{"state":"Idle","cycleCount":0}}
```

**推送格式（状态广播）：**
```json
{"event":"statusUpdate","data":{"state":"Running","cycleCount":42}}
```

### 支持的命令

| 命令 | 参数 | 说明 |
|------|------|------|
| `status` | 无 | 查询当前状态 |
| `startCycle` | 无 | 启动拾取循环 |
| `stopCycle` | 无 | 停止循环 |
| `emergencyStop` | 无 | 紧急停机 |
| `loadRecipe` | `{"recipe":"name"}` | 加载配方 |
| `setAutoMode` | `{"enabled":true}` | 切换自动模式 |
| `reset` | 无 | 重置控制器 |

## 服务器部署

### 方式一：一键脚本（推荐）

在 Ubuntu 22.04/24.04 服务器上以 root 执行：

```bash
chmod +x deploy/setup.sh
sudo ./deploy/setup.sh
```

脚本会自动：安装依赖 → 编译 → 测试 → 安装 → 配置 systemd → 配置防火墙 → 启动服务

### 方式二：Docker

```bash
docker build -t industrial-server .
docker run -d -p 9600:9600 --name workcell industrial-server
```

### 服务管理

```bash
systemctl status industrial-server   # 查看状态
systemctl restart industrial-server  # 重启
journalctl -u industrial-server -f   # 查看日志
```

## 移动 App（iOS）

需要 Qt 6.5+ with Qt Quick 模块和 Xcode：

```bash
cmake -S . -B build-ios -G Xcode \
    -DCMAKE_SYSTEM_NAME=iOS \
    -DCMAKE_OSX_ARCHITECTURES=arm64 \
    -DCMAKE_PREFIX_PATH=/path/to/qt/ios \
    -DBUILD_MOBILE_APP=ON

cmake --build build-ios --config Release
```

## 架构

```
┌─────────────────────────────────────────────┐
│              Mobile App (iOS)                │
│   Dashboard │ Devices │ Control │ Alerts     │
└────────────────────┬────────────────────────┘
                     │ TCP/JSON
┌────────────────────┴────────────────────────┐
│            WorkcellServer (:9600)            │
└────────────────────┬────────────────────────┘
                     │
┌────────────────────┴────────────────────────┐
│           WorkcellController                 │
│  ┌──────────┐ ┌───────────┐ ┌────────────┐  │
│  │ Workflow  │ │ Recovery  │ │   Auto     │  │
│  │  Engine   │ │  Manager  │ │   Mode     │  │
│  └────┬─────┘ └─────┬─────┘ └────────────┘  │
│       │              │                        │
│  ┌────┴──────────────┴─────────────────────┐ │
│  │        Planning Layer                    │ │
│  │  GraspPlanner  │  TrajectoryPlanner      │ │
│  └────────────────┴────────────────────────┘ │
│  ┌─────────────────────────────────────────┐ │
│  │        Vision Pipeline                   │ │
│  │  Capture → Filter → Cluster → Detect     │ │
│  └─────────────────────────────────────────┘ │
└────────────────────┬────────────────────────┘
                     │
┌────────────────────┴────────────────────────┐
│           DeviceManager                      │
│  ┌──────────┐ ┌──────────┐ ┌──────────┐     │
│  │  Camera   │ │  Robot   │ │   PLC    │     │
│  │  Driver   │ │  Driver  │ │  Driver  │     │
│  └──────────┘ └──────────┘ └──────────┘     │
└─────────────────────────────────────────────┘
```

## License

MIT
