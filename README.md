# HSM (Hierarchical State Machine) Framework

一个轻量级、跨平台的分层状态机框架，支持Linux和嵌入式RTOS环境。

## 概述

HSM（Hierarchical State Machine）是一个分层状态机框架，旨在提供一种结构化的方法来管理复杂系统的状态转换。它支持状态嵌套（子状态）、事件驱动的处理方式，并提供了操作系统抽象层（OSA）以实现跨平台兼容性。

主要特点：
- 支持层次化状态建模
- 事件驱动架构
- 跨平台支持（Linux/RTOS）
- 线程安全的事件队列机制
- 可扩展的定时器功能

## 目录结构

```
.
├── core/                 # 核心库文件
│   ├── hsm.h             # HSM 核心定义
│   ├── osa_hsm.c         # OS 抽象层实现
│   ├── osa_hsm.h         # OS 抽象层接口
│   ├── osa_timer.c       # 定时器实现
│   └── osa_timer.h       # 定时器接口
└── example_linux/        # Linux 示例
    ├── hsm_simulate.h    # 示例头文件
    ├── hsm_smuilate.c    # 示例实现
    └── makefile          # 构建脚本
```

## 安装与编译

### Linux 环境要求
- GCC 编译器
- POSIX 消息队列支持 (`-lrt`)

### 编译步骤

1. 确保系统已挂载消息队列文件系统：
   ```bash
   sudo mkdir -p /dev/mqueue
   sudo mount -t mqueue none /dev/mqueue
   ```

2. 进入示例目录并编译：
   ```bash
   cd example_linux
   make
   ```

3. 运行示例：
   ```bash
   ./bin/app.bin
   ```

4. 清理编译文件：
   ```bash
   make clean
   ```

## 主要API

### 核心宏定义

- `STATE_ENTRY(a)` - 在状态处理器中声明入口指针
- `STATE_INIT()` - 触发当前状态的初始化事件
- `STATE_DISPATCH(s, e)` - 将事件派发到指定状态
- `STATE_TRANSIT(s)` - 转移到新的状态
- `STATE_SUPER(e)` - 将事件传递给父状态处理

### 核心函数

- `osa_hsm_active_init()` - 初始化HSM实例
- `osa_hsm_active_start()` - 启动HSM任务/线程
- `osa_hsm_active_event_post()` - 向HSM发送事件
- `osa_hsm_active_period()` - 设置HSM周期性事件

## 使用示例

### 定义状态处理器

```c
int my_state_handler(void* entry, const struct hsm_event* event) {
    STATE_ENTRY(entry);

    switch (event->signal) {
        case HSM_SIG_ENTRY:
            printf("Entering state: %s\n", STATE_NAME());
            break;

        case HSM_SIG_EXIT:
            printf("Exiting state: %s\n", STATE_NAME());
            break;

        case HSM_USER_SIG:
            // 处理用户自定义事件
            STATE_TRANSIT(&another_state);  // 转移到另一个状态
            break;

        default:
            STATE_SUPER(event);  // 委托给父状态处理
            break;
    }

    return 0;
}
```

### 创建和启动HSM

```c
#include "osa_hsm.h"

// 定义状态
struct hsm_state my_state = {
    .handler = my_state_handler,
    .parent = NULL,
    .name = "MyState"
};

int main() {
    struct osa_hsm_active hsm;

    // 初始化HSM
    osa_hsm_active_init(&hsm, &my_state);
    hsm.name = "ExampleHSM";

    // 启动HSM（栈大小4096字节，优先级50，消息队列大小10）
    osa_hsm_active_start(&hsm, 4096, 50, 10);

    // 发送事件到HSM
    struct hsm_event event = {
        .signal = HSM_USER_SIG,
        .data = NULL,
        .size = 0
    };
    osa_hsm_active_event_post(&hsm, &event, 0);

    return 0;
}
```

## 跨平台支持

该框架设计为跨平台兼容：

- **Linux**: 使用POSIX线程（`pthread`）和消息队列（`mqueue`）
- **RTOS**: 通过适配层使用RTT、FreeRTOS或其他实时操作系统的相应功能

在Linux平台上，需要链接实时库（`-lrt`）以支持POSIX消息队列功能。

## 设计原理

HSM框架实现了状态模式的高级形式，支持：

1. **状态继承** - 子状态可以继承父状态的行为
2. **状态嵌套** - 状态可以包含子状态形成层次结构
3. **事件驱动** - 系统响应外部事件进行状态转换
4. **历史状态** - 记录之前的状态以便返回

## 注意事项

- 在Linux环境下，必须确保挂载了`/dev/mqueue`才能使用消息队列功能
- 事件发送是线程安全的，但在状态处理器内部访问共享资源时需要额外的同步机制
- 消息队列名称会使用HSM名称或进程ID，避免不同HSM实例之间的冲突

## 维护者

- destin.zhang@quectel.com

## 许可证

该项目采用 Apache 2.0 许可证。