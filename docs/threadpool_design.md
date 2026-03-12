# HSM 线程池设计方案

## 1. 架构概述

### 1.1 设计目标

```
┌─────────────────────────────────────────────────────────────┐
│                    HSM 线程池架构图                          │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│   ┌─────────────┐     ┌─────────────┐     ┌─────────────┐  │
│   │   HSM 1     │     │   HSM 2     │     │   HSM N     │  │
│   │  (worker1)  │     │  (worker2)  │     │  (workerN)  │  │
│   └──────┬──────┘     └──────┬──────┘     └──────┬──────┘  │
│          │                   │                   │          │
│          └───────────────────┼───────────────────┘          │
│                              │                              │
│                              ▼                              │
│   ┌─────────────────────────────────────────────────────┐  │
│   │              线程池 (Thread Pool)                    │  │
│   │  ┌─────────────────────────────────────────────┐   │  │
│   │  │           工作队列 (Work Queue)              │   │  │
│   │  │  ┌─────┐  ┌─────┐  ┌─────┐      ┌─────┐    │   │  │
│   │  │  │Work1│→│Work2│→│Work3│→...→ │WorkN│    │   │  │
│   │  │  └─────┘  └─────┘  └─────┘      └─────┘    │   │  │
│   │  └─────────────────────────────────────────────┘   │  │
│   │                      │                              │  │
│   │          ┌───────────┼───────────┐                  │  │
│   │          ▼           ▼           ▼                  │  │
│   │   ┌─────────┐   ┌─────────┐   ┌─────────┐          │  │
│   │   │Worker 1 │   │Worker 2 │   │Worker N │          │  │
│   │   │ Thread  │   │ Thread  │   │ Thread  │          │  │
│   │   └─────────┘   └─────────┘   └─────────┘          │  │
│   └─────────────────────────────────────────────────────┘  │
│                              │                              │
│                              ▼                              │
│   ┌─────────────────────────────────────────────────────┐  │
│   │              完成通知 (Completion)                   │  │
│   │         回调函数 / HSM 事件通知                      │  │
│   └─────────────────────────────────────────────────────┘  │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

### 1.2 核心特性

| 特性 | 说明 |
|------|------|
| **轻量级** | 静态内存池，无动态分配 |
| **可配置** | 支持 1-16 个工作者线程 |
| **HSM集成** | 工作完成自动发送事件到HSM |
| **线程安全** | 互斥锁+条件变量保护共享数据 |
| **统计信息** | 提交/完成/取消/错误计数 |

---

## 2. 数据结构设计

### 2.1 工作项状态机

```
                    ┌─────────────┐
                    │   PENDING   │ ← 初始状态
                    │   (等待中)   │
                    └──────┬──────┘
                           │ submit_work()
                           ▼
                    ┌─────────────┐
         ┌─────────│   RUNNING   │ ← 执行中
         │         │   (运行中)   │
         │         └──────┬──────┘
         │                │ work_func() 返回
         │                ▼
         │    ┌───────────────────────┐
         │    │                       │
         ▼    ▼                       │
   ┌─────────────┐            ┌─────────────┐
   │  COMPLETED  │            │    ERROR    │
   │   (已完成)   │            │   (错误)    │
   └─────────────┘            └─────────────┘
         ▲
         │ cancel_work()
         │
   ┌─────────────┐
   │  CANCELLED  │
   │   (已取消)   │
   └─────────────┘
```

### 2.2 核心数据结构

```c
/* 工作项状态 */
enum osa_work_state {
    OSA_WORK_PENDING,      /* 等待执行 */
    OSA_WORK_RUNNING,      /* 正在执行 */
    OSA_WORK_COMPLETED,    /* 成功完成 */
    OSA_WORK_CANCELLED,    /* 已取消 */
    OSA_WORK_ERROR,        /* 执行错误 */
};

/* 工作项结构 */
struct osa_work {
    uint32_t id;                          /* 唯一ID */
    enum osa_work_state state;            /* 当前状态 */
    
    /* 工作函数和上下文 */
    int (*work_func)(void *ctx);          /* 实际工作函数 */
    void *work_ctx;                       /* 工作函数上下文 */
    
    /* 完成回调 */
    void (*complete_cb)(struct osa_work *work, int result, void *ctx);
    void *complete_ctx;                   /* 回调上下文 */
    
    /* HSM集成 - 目标HSM接收完成事件 */
    struct osa_hsm_active *target_hsm;    /* 目标HSM */
    int completion_signal;                /* 完成信号 */
    
    int result;                           /* 执行结果 */
    struct osa_work *next;                /* 链表指针 */
    struct osa_work *prev;
};
```

### 2.3 线程池结构

```c
struct osa_threadpool {
    char *name;                       /* 线程池名称 */
    
    /* Linux POSIX线程 */
    pthread_t *workers;               /* 工作者线程数组 */
    pthread_mutex_t lock;             /* 互斥锁 */
    pthread_cond_t work_available;    /* 工作可用条件变量 */
    pthread_cond_t work_completed;    /* 工作完成条件变量 */
    
    /* 工作队列 */
    struct osa_work *work_queue_head; /* 队列头 */
    struct osa_work *work_queue_tail; /* 队列尾 */
    
    uint32_t num_workers;             /* 工作者数量 */
    uint32_t queue_size;              /* 队列大小 */
    bool shutdown;                    /* 关闭标志 */
    
    /* 统计信息 */
    struct osa_threadpool_stats {
        uint32_t total_submitted;     /* 总提交数 */
        uint32_t total_completed;     /* 总完成数 */
        uint32_t total_cancelled;     /* 总取消数 */
        uint32_t total_errors;        /* 总错误数 */
        uint32_t queue_depth;         /* 当前队列深度 */
        uint32_t active_workers;      /* 活跃工作者数 */
    } stats;
};
```

---

## 3. 内存管理策略

### 3.1 静态工作池

```
┌─────────────────────────────────────────────────────────┐
│              静态工作池 (32个预分配槽位)                  │
├─────────────────────────────────────────────────────────┤
│                                                         │
│   槽位0    槽位1    槽位2    ...    槽位30   槽位31    │
│  ┌─────┐ ┌─────┐ ┌─────┐         ┌─────┐ ┌─────┐     │
│  │Work │ │Work │ │Work │   ...   │Work │ │Work │     │
│  │  0  │ │  1  │ │  2  │         │ 30  │ │ 31  │     │
│  └──┬──┘ └──┬──┘ └──┬──┘         └──┬──┘ └──┬──┘     │
│     │       │       │               │       │         │
│     ▼       ▼       ▼               ▼       ▼         │
│   [使用中] [空闲]  [使用中]  ...   [空闲]  [使用中]    │
│                                                         │
│   状态: COMPLETED = 空闲可用                            │
│   状态: PENDING/RUNNING = 已分配                        │
│                                                         │
└─────────────────────────────────────────────────────────┘
```

**优点：**
- 无堆内存分配，避免内存碎片
- 确定性内存使用，适合嵌入式系统
- O(1) 分配/释放时间

---

## 4. 工作流程

### 4.1 工作提交流程

```
┌─────────┐     ┌─────────────────┐     ┌─────────────────┐
│  调用者  │     │   线程池 (主线程)  │     │   工作队列      │
└────┬────┘     └────────┬────────┘     └────────┬────────┘
     │                   │                       │
     │ submit_work(work) │                       │
     │──────────────────>│                       │
     │                   │                       │
     │                   │ 1. 获取互斥锁          │
     │                   │ 2. 检查队列是否已满     │
     │                   │ 3. 分配工作ID          │
     │                   │ 4. 添加到队列尾部       │
     │                   │ 5. 更新统计信息         │
     │                   │ 6. 发送条件信号         │
     │                   │ 7. 释放互斥锁          │
     │                   │                       │
     │                   │──────────────────────>│
     │                   │                       │
     │     返回 0        │                       │
     │<──────────────────│                       │
     │                   │                       │
```

### 4.2 工作执行流程

```
┌─────────────────┐     ┌─────────────────┐     ┌─────────────────┐
│   工作者线程     │     │   工作队列       │     │   完成处理       │
└────────┬────────┘     └────────┬────────┘     └────────┬────────┘
         │                       │                       │
         │ 1. 等待 work_available │                       │
         │<──────────────────────│                       │
         │                       │                       │
         │ 2. 获取互斥锁          │                       │
         │ 3. 从队列头部取出工作   │                       │
         │ 4. 更新状态为 RUNNING  │                       │
         │ 5. 释放互斥锁          │                       │
         │                       │                       │
         │──────────────────────>│                       │
         │                       │                       │
         │ 6. 执行 work_func()    │                       │
         │    (在工作者线程中)     │                       │
         │                       │                       │
         │ 7. 获取互斥锁          │                       │
         │ 8. 更新状态为 COMPLETED│                       │
         │ 9. 调用 complete_cb()  │                       │
         │ 10. 发送HSM事件(如需要)│                       │
         │ 11. 发送 work_completed│                       │
         │ 12. 释放互斥锁         │                       │
         │                       │                       │
         │──────────────────────────────────────────────>│
         │                       │                       │
         │                       │                       │
```

---

## 5. HSM 集成机制

### 5.1 异步工作模式

```
┌─────────────┐                    ┌─────────────┐
│   HSM 主线程 │                    │  线程池工作者  │
└──────┬──────┘                    └──────┬──────┘
       │                                  │
       │ 1. 提交异步工作                   │
       │    submit_work_to_hsm()          │
       │─────────────────────────────────>│
       │                                  │
       │    立即返回，HSM继续执行          │
       │<─────────────────────────────────│
       │                                  │
       │    HSM 处理其他事件...            │
       │                                  │
       │                                  │ 2. 执行耗时操作
       │                                  │    (在后台线程)
       │                                  │
       │                                  │ 3. 工作完成
       │                                  │
       │ 4. 发送完成事件              │
       │<─────────────────────────────────│\SIG_WORK_COMPLETE
       │
       │
       │ 5. HSM 状态转换
       │    idle → working → idle
       │
```

### 5.2 状态机集成示例

```c
/* HSM 状态处理函数 */
static int state_working_handler(struct hsm *hsm, struct hsm_event *event)
{
    switch (event->signal) {
    case HSM_SIG_ENTRY:
        /* 进入工作状态，提交异步任务 */
        work = osa_work_alloc();
        osa_work_init(work, heavy_computation, ctx, NULL, NULL);
        osa_threadpool_submit_work_to_hsm(pool, work, hsm, SIG_WORK_COMPLETE);
        break;
        
    case SIG_WORK_COMPLETE:
        /* 异步任务完成，获取结果 */
        work = (struct osa_work *)event->data;
        result = work->result;
        osa_work_free(work);
        
        /* 转换回空闲状态 */
        STATE_TRANSIT(&state_idle);
        break;
    }
}
```

---

## 6. API 接口

### 6.1 线程池管理

```c
/* 初始化线程池 */
int osa_threadpool_init(struct osa_threadpool *pool, 
                        const char *name, 
                        uint32_t num_workers, 
                        uint32_t queue_size);

/* 启动线程池 */
int osa_threadpool_start(struct osa_threadpool *pool);

/* 停止线程池 */
int osa_threadpool_stop(struct osa_threadpool *pool);
```

### 6.2 工作提交

```c
/* 提交普通工作 */
int osa_threadpool_submit_work(struct osa_threadpool *pool, 
                               struct osa_work *work);

/* 提交带HSM通知的工作 */
int osa_threadpool_submit_work_to_hsm(struct osa_threadpool *pool, 
                                      struct osa_work *work,
                                      struct osa_hsm_active *target_hsm, 
                                      int completion_signal);

/* 取消待处理工作 */
int osa_threadpool_cancel_work(struct osa_threadpool *pool, 
                               uint32_t work_id);
```

### 6.3 工作项管理

```c
/* 从静态池分配工作项 */
struct osa_work* osa_work_alloc(void);

/* 释放工作项回静态池 */
void osa_work_free(struct osa_work *work);

/* 初始化工作项 */
void osa_work_init(struct osa_work *work,
                   int (*work_func)(void *ctx),
                   void *work_ctx,
                   void (*complete_cb)(struct osa_work *work, int result, void *ctx),
                   void *complete_ctx);
```

---

## 7. 配置参数

| 参数 | 默认值 | 范围 | 说明 |
|------|--------|------|------|
| `OSA_THREADPOOL_DEFAULT_WORKERS` | 4 | 1-16 | 默认工作者线程数 |
| `OSA_THREADPOOL_MAX_WORKERS` | 16 | - | 最大工作者线程数 |
| `OSA_THREADPOOL_QUEUE_SIZE` | 64 | - | 默认队列大小 |
| `OSA_THREADPOOL_STACK_SIZE` | 8KB | - | 线程栈大小 |
| `OSA_THREADPOOL_PRIORITY` | 5 | - | 线程优先级 |
| `OSA_WORK_POOL_SIZE` | 32 | - | 静态工作池大小 |

---

## 8. 测试验证

### 8.1 单元测试结果

```
=== Thread Pool Test ===

[tp|core] Thread pool 'test_pool' initialized with 2 workers, queue size 16
[tp|core] Worker thread started
[tp|core] Worker thread started
[tp|core] Thread pool 'test_pool' started with 2 workers
Thread pool started with 2 workers

Submitting 5 work items...
[tp|core] Work id:1 submitted, queue depth:1
Submitted work 1 (id=1)
[tp|core] Work id:2 submitted, queue depth:1
[tp|core] Executing work id:1
[tp|core] Executing work id:2
[WORK] Starting work item 2
[WORK] Starting work item 1
...

=== Statistics ===
Submitted:  5
Completed:  5
Cancelled:  0
Errors:     0
Queue:      0
Active:     0

Work count: 5
[tp|core] Thread pool stopped

=== Test Complete ===
```

### 8.2 验证要点

- ✅ 线程池初始化和启动
- ✅ 多工作项并发执行
- ✅ 队列管理（深度跟踪）
- ✅ 完成回调触发
- ✅ 统计信息准确
- ✅ 资源清理

---

## 9. 文件结构

```
core/
├── osa_threadpool.h    # 线程池头文件
├── osa_threadpool.c    # 线程池实现

example_linux/
├── hsm_threadpool_example.c  # HSM集成示例
├── threadpool_test.c         # 单元测试
└── makefile                  # 构建配置
```

---

## 10. 使用示例

### 10.1 基本使用

```c
#include "osa_threadpool.h"

/* 定义工作函数 */
int my_work_func(void *ctx)
{
    int *data = (int *)ctx;
    /* 执行耗时操作 */
    do_heavy_work(*data);
    return 0;  /* 返回结果 */
}

/* 使用线程池 */
void example(void)
{
    struct osa_threadpool pool;
    struct osa_work work;
    int my_data = 42;
    
    /* 1. 初始化并启动线程池 */
    osa_threadpool_init(&pool, "my_pool", 4, 64);
    osa_threadpool_start(&pool);
    
    /* 2. 初始化工作项 */
    osa_work_init(&work, my_work_func, &my_data, NULL, NULL);
    
    /* 3. 提交工作 */
    osa_threadpool_submit_work(&pool, &work);
    
    /* 4. 等待完成 */
    osa_threadpool_wait_for_work(&pool, &work, -1);
    
    /* 5. 停止线程池 */
    osa_threadpool_stop(&pool);
}
```

### 10.2 HSM 集成使用

```c
/* HSM 状态处理 */
static int state_idle_handler(struct my_hsm *hsm, struct hsm_event *event)
{
    struct osa_work *work;
    
    switch (event->signal) {
    case SIG_START_ASYNC_WORK:
        /* 分配并初始化工作项 */
        work = osa_work_alloc();
        osa_work_init(work, async_operation, hsm, NULL, NULL);
        
        /* 提交到线程池，完成后发送 SIG_WORK_DONE 到 HSM */
        osa_threadpool_submit_work_to_hsm(
            hsm->threadpool, 
            work,
            &hsm->super,
            SIG_WORK_DONE
        );
        
        /* 转换到等待状态 */
        STATE_TRANSIT(&state_waiting);
        break;
    }
}

static int state_waiting_handler(struct my_hsm *hsm, struct hsm_event *event)
{
    struct osa_work *work;
    
    switch (event->signal) {
    case SIG_WORK_DONE:
        /* 异步工作完成 */
        work = (struct osa_work *)event->data;
        
        /* 处理结果 */
        if (work->result == 0) {
            /* 成功 */
        } else {
            /* 失败 */
        }
        
        /* 释放工作项 */
        osa_work_free(work);
        
        /* 返回空闲状态 */
        STATE_TRANSIT(&state_idle);
        break;
    }
}
```

---

## 11. 总结

本线程池设计方案实现了：

1. **轻量级架构** - 静态内存池，无动态分配
2. **HSM深度集成** - 异步工作完成自动通知HSM
3. **灵活配置** - 支持1-16个工作者线程
4. **完整功能** - 提交、取消、等待、统计
5. **线程安全** - 完善的同步机制
6. **易于使用** - 简洁的API设计

适用于需要在HSM框架中执行异步操作的嵌入式应用场景。
