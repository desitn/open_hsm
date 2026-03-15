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
- **Worker Pool - 异步任务卸载**
- **完整的平台抽象层（OSA）**

## 目录结构

```
.
├── core/                       # 核心库文件
│   ├── hsm.h                   # HSM 核心定义
│   ├── osa_hsm.c               # OS 抽象层 HSM 实现
│   ├── osa_hsm.h               # OS 抽象层 HSM 接口
│   ├── osa_worker.c            # Worker Pool 实现
│   └── osa_worker.h            # Worker Pool 接口
├── platform/                   # 平台抽象层
│   ├── osa_platform.h          # 平台抽象主接口
│   ├── osa_error.h             # 统一错误码系统
│   ├── osa_thread.h            # 线程抽象接口
│   ├── osa_sync.h              # 同步原语接口（互斥锁、信号量）
│   ├── osa_queue.h             # 消息队列抽象接口
│   ├── osa_timer.h             # 定时器抽象接口
│   ├── linux/                  # Linux 平台实现
│   │   ├── osa_thread.c        # Linux 线程实现
│   │   ├── osa_sync.c          # Linux 同步原语实现
│   │   ├── osa_queue.c         # Linux 消息队列实现
│   │   ├── osa_timer.c         # Linux 定时器实现
│   │   └── osa_error.c         # Linux 错误码实现
│   └── esp32/                  # ESP32 (FreeRTOS) 平台实现
│       ├── osa_thread.c        # FreeRTOS 任务封装
│       ├── osa_sync.c          # FreeRTOS 互斥锁/信号量
│       ├── osa_queue.c         # FreeRTOS 队列
│       ├── osa_timer.c         # FreeRTOS 定时器
│       └── osa_error.c         # ESP32 错误码实现
├── test/                       # 单元测试
│   ├── unity/                  # Unity 测试框架
│   │   ├── unity.h             # Unity 头文件
│   │   └── unity.c             # Unity 实现
│   ├── test_main.c             # 测试主入口
│   ├── test_error.c            # 错误码系统测试
│   ├── test_sync.c             # 同步原语测试
│   ├── test_worker.c           # Worker Pool 测试
│   └── makefile                # 测试构建脚本
├── examples/                   # 示例程序
│   ├── linux/                  # Linux 平台示例
│   │   ├── hsm_simulate.h      # 基础示例头文件
│   │   ├── hsm_simulate.c      # 基础 HSM 示例
│   │   ├── hsm_worker.c        # HSM + Worker Pool 集成示例
│   │   ├── worker_pool.c       # Worker Pool 功能演示
│   │   ├── error_code_demo.c   # 错误码系统演示
│   │   └── makefile            # 构建脚本
│   └── esp32/                  # ESP32 平台示例
│       ├── CMakeLists.txt      # ESP-IDF 项目配置
│       └── main/
│           ├── CMakeLists.txt  # 组件配置
│           └── hsm_esp32_example.c  # ESP32 HSM + Worker Pool 示例
└── docs/                       # 文档
    └── worker_design.md        # Worker Pool 设计文档
```

## 安装与编译

### Linux 环境要求
- GCC 编译器
- POSIX 线程支持 (`-lpthread`)
- POSIX 实时库支持 (`-lrt`)

### 编译步骤

#### 1. 运行单元测试
```bash
cd test
make
./bin/test_runner.bin
```

#### 2. 运行示例程序
```bash
cd examples/linux

# 编译所有示例
make

# 运行基础 HSM 示例
./bin/hsm_app.bin

# 运行 Worker Pool 示例
make worker
./bin/hsm_worker.bin

# 运行错误码演示
make error-demo
./bin/error_code_demo.bin
```

#### 3. 清理编译文件
```bash
make clean
```

### ESP32 编译

#### 环境要求
- ESP-IDF 开发框架（v4.0+）
- ESP32 开发板

#### 编译步骤
```bash
cd examples/esp32

# 设置 ESP-IDF 环境
. $HOME/esp/esp-idf/export.sh

# 配置项目（可选）
idf.py menuconfig

# 编译
idf.py build

# 烧录到设备
idf.py -p /dev/ttyUSB0 flash

# 查看串口输出
idf.py -p /dev/ttyUSB0 monitor
```

#### ESP32 示例功能
- LED 状态机控制（GPIO2）
- Worker Pool 异步任务处理
- 模拟按钮事件触发状态转换
- 内存使用统计

### 平台选择编译

默认自动检测 Linux 平台，也可以手动指定：
```bash
# Linux 平台（默认）
make PLATFORM=linux

# RTOS 平台（需要实现）
make PLATFORM=rtos
```

## 平台抽象层 (OSA)

框架提供完整的平台抽象层，使核心代码与平台无关：

### 线程抽象 (`osa_thread.h`)
```c
osa_thread_t thread;
struct osa_thread_param param = {
    .name = "my_thread",
    .stack_size = 4096,
    .priority = 5
};
osa_thread_create(&thread, &param, my_thread_func, arg);
```

### 同步原语 (`osa_sync.h`)
```c
// 互斥锁
osa_mutex_t mutex;
osa_mutex_create(&mutex);
osa_mutex_lock(mutex);
osa_mutex_unlock(mutex);

// 信号量
osa_sem_t sem;
osa_sem_create(&sem, 0, 1);  // 初始值0，最大值1
osa_sem_wait(sem, 100);      // 等待100ms
osa_sem_post(sem);
```

### 消息队列 (`osa_queue.h`)
```c
osa_queue_t queue;
osa_queue_create(&queue, "my_queue", sizeof(my_msg_t), 10);
osa_queue_send(queue, &msg, sizeof(msg), -1);  // 阻塞发送
osa_queue_receive(queue, &msg, sizeof(msg), 0); // 非阻塞接收
```

### 定时器 (`osa_timer.h`)
```c
osa_timer_t timer;
osa_timer_create(&timer, 1000, true, my_callback, arg); // 1秒周期定时器
osa_timer_stop(timer);
osa_timer_delete(timer);
```

## 主要API

### 核心宏定义

- `STATE_ENTRY(a)` - 在状态处理器中声明入口指针
- `STATE_NAME()` - 获取当前状态名称
- `STATE_ERROR()` - 获取最后错误码
- `STATE_INIT()` - 触发当前状态的初始化事件
- `STATE_DISPATCH(s, e)` - 将事件派发到指定状态
- `STATE_TRANSIT(s)` - 转移到新的状态
- `STATE_SUPER(e)` - 将事件传递给父状态处理

### HSM 核心函数

- `osa_hsm_active_init()` - 初始化HSM实例
- `osa_hsm_active_start()` - 启动HSM任务/线程
- `osa_hsm_active_event_post()` - 向HSM发送事件
- `osa_hsm_active_period()` - 设置HSM周期性事件

### Worker Pool API

- `osa_worker_pool_init()` - 初始化 Worker Pool
- `osa_worker_pool_start()` - 启动 Worker Pool
- `osa_worker_pool_stop()` - 停止 Worker Pool
- `osa_worker_pool_submit()` - 提交任务到 Worker Pool
- `osa_worker_pool_submit_to_hsm()` - 提交任务并在完成时通知 HSM
- `osa_worker_job_init()` - 初始化任务结构
- `osa_worker_job_alloc()` - 从静态池分配任务
- `osa_worker_job_free()` - 释放任务回静态池

### 错误码系统 API (`osa_error.h`)

统一错误码系统，替代直接使用整数返回值：

```c
// 错误码常量
OSA_OK                      // 成功 (0)
OSA_ERR_GENERIC             // 通用错误
OSA_ERR_INVALID_PARAM       // 无效参数
OSA_ERR_NULL_POINTER        // 空指针
OSA_ERR_NOT_INITIALIZED     // 未初始化
OSA_ERR_NO_MEMORY           // 内存不足
OSA_ERR_TIMEOUT             // 超时
OSA_ERR_MUTEX_LOCK          // 互斥锁错误
OSA_ERR_THREAD_CREATE       // 线程创建失败
OSA_ERR_QUEUE_FULL          // 队列满
OSA_ERR_WORKER_INIT         // Worker Pool 初始化失败
OSA_ERR_WORKER_QUEUE_FULL   // Worker Pool 队列满
OSA_ERR_HSM_INIT            // HSM 初始化失败
OSA_ERR_HSM_INVALID_STATE   // HSM 无效状态

// 辅助函数
const char* osa_error_string(int err);      // 获取错误描述
const char* osa_error_category(int err);    // 获取错误分类
bool osa_is_ok(int err);                    // 检查是否成功
bool osa_is_err(int err);                   // 检查是否错误
```

使用示例：
```c
int err = osa_worker_pool_init(&pool, "test", 2, 8);
if (osa_is_err(err)) {
    printf("Failed: %s (category: %s)\n", 
           osa_error_string(err), 
           osa_error_category(err));
    return -1;
}
```

## 使用示例

### 基础 HSM 示例

```c
#include "osa_hsm.h"

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
            STATE_TRANSIT(&another_state);
            break;

        default:
            STATE_SUPER(event);
            break;
    }
    return 0;
}

int main() {
    struct osa_hsm_active hsm;
    struct hsm_state my_state = {
        .handler = my_state_handler,
        .parent = NULL,
        .name = "MyState"
    };

    osa_hsm_active_init(&hsm, &my_state);
    hsm.name = "ExampleHSM";
    osa_hsm_active_start(&hsm, 4096, 50, 10);

    // 发送事件
    struct hsm_event event = { .signal = HSM_USER_SIG };
    osa_hsm_active_event_post(&hsm, &event, 0);

    return 0;
}
```

### Worker Pool 示例

```c
#include "osa_worker.h"

// 任务函数
int heavy_computation(void *ctx) {
    // 执行耗时操作
    return result;
}

// 完成回调
void on_complete(struct osa_worker_job *job, int result, void *ctx) {
    printf("Job %d completed with result %d\n", job->id, result);
}

int main() {
    struct osa_worker_pool pool;
    
    // 初始化 Worker Pool（4个线程，队列大小64）
    osa_worker_pool_init(&pool, "main_pool", 4, 64);
    osa_worker_pool_start(&pool);
    
    // 创建任务
    struct osa_worker_job *job = osa_worker_job_alloc();
    osa_worker_job_init(job, heavy_computation, ctx, on_complete, cb_ctx);
    
    // 提交任务
    osa_worker_pool_submit(&pool, job);
    
    // 或者提交到 HSM（完成时自动发送事件）
    osa_worker_pool_submit_to_hsm(&pool, job, &my_hsm, HSM_SIG_WORK_COMPLETE);
    
    osa_worker_pool_stop(&pool);
    return 0;
}
```

## 跨平台支持

该框架设计为跨平台兼容：

- **Linux**: 使用 POSIX 线程（`pthread`）、条件变量和 POSIX 定时器
- **RTOS**: 通过适配层使用 FreeRTOS、RT-Thread 或其他实时操作系统的相应功能

平台抽象层接口定义在 `platform/` 目录下，具体实现在 `platform/<platform>/` 目录中。

### 添加新平台支持

1. 在 `platform/` 下创建新目录（如 `platform/freertos/`）
2. 实现 `osa_thread.c`, `osa_sync.c`, `osa_queue.c`, `osa_timer.c`
3. 在编译时指定 `PLATFORM=freertos`

## 设计原理

HSM框架实现了状态模式的高级形式，支持：

1. **状态继承** - 子状态可以继承父状态的行为
2. **状态嵌套** - 状态可以包含子状态形成层次结构
3. **事件驱动** - 系统响应外部事件进行状态转换
4. **历史状态** - 记录之前的状态以便返回
5. **异步处理** - Worker Pool 支持将耗时操作卸载到后台线程

## 注意事项

- 事件发送是线程安全的，但在状态处理器内部访问共享资源时需要额外的同步机制
- Worker Pool 的任务结构使用静态分配，最大任务数由 `OSA_WORKER_POOL_QUEUE_SIZE` 定义
- 消息队列名称会使用 HSM 名称，避免不同 HSM 实例之间的冲突

## 文档

- [Worker Pool 设计文档](docs/worker_design.md) - 详细的设计说明和架构图

## 维护者

- destin.zhang@quectel.com

## 许可证

该项目采用 Apache 2.0 许可证。
