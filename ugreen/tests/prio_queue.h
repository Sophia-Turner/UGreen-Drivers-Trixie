#ifndef _CHIP_PRIO_QUEUE_H
#define _CHIP_PRIO_QUEUE_H

#include <linux/workqueue.h>
#include <linux/list.h>
#include <linux/spinlock.h>
#include <linux/wait.h>
#include <linux/atomic.h>
#include <linux/completion.h>
#include <linux/kref.h>

#define PRIO_LEVELS 3 // 高、中、低三个优先级

struct io_request {
    struct kref refcount;  
    struct list_head list;
    int priority;      // 0=最高优先级
    struct completion done;
    size_t data_len;
    int result;
    bool is_sync; 
    char data[];
};

struct prio_queue {
    struct list_head queues[PRIO_LEVELS];
    spinlock_t lock;
    atomic_t count;
    wait_queue_head_t wq;
    atomic_t io_in_progress;
};

struct chip_device {
    struct prio_queue pq;
    struct workqueue_struct *wq;
    struct delayed_work dwork;
    struct mutex io_lock;
    struct proc_dir_entry *proc_entry;
};
void req_get(struct io_request *req);

void req_put(struct io_request *req);
// 外部接口
int prio_queue_init(struct prio_queue *pq);
void prio_queue_cleanup(struct prio_queue *pq);
int prio_queue_put(struct prio_queue *pq, struct io_request *req);
struct io_request *prio_queue_get(struct prio_queue *pq);

#endif // _CHIP_PRIO_QUEUE_H