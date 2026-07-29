#include "prio_queue.h"
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/version.h>

static void req_release(struct kref *ref)
{
    struct io_request *req = container_of(ref, struct io_request, refcount);
    kfree(req);
}

 void req_get(struct io_request *req)
{
    kref_get(&req->refcount);
}

 void req_put(struct io_request *req)
{
    kref_put(&req->refcount, req_release);
}
int prio_queue_init(struct prio_queue *pq)
{
    int i;
    
    spin_lock_init(&pq->lock);
    for (i = 0; i < PRIO_LEVELS; i++)
        INIT_LIST_HEAD(&pq->queues[i]);
    
    init_waitqueue_head(&pq->wq);
    atomic_set(&pq->count, 0);
    atomic_set(&pq->io_in_progress, 0);
    
    return 0;
}

void prio_queue_cleanup(struct prio_queue *pq)
{
    struct io_request *req;
    int i;
    
    spin_lock(&pq->lock);
    for (i = 0; i < PRIO_LEVELS; i++) {
        while (!list_empty(&pq->queues[i])) {
            req = list_first_entry(&pq->queues[i], struct io_request, list);
            list_del(&req->list);
            kfree(req);
        }
    }
    spin_unlock(&pq->lock);
}

int prio_queue_put(struct prio_queue *pq, struct io_request *req)
{
    unsigned long flags;
    
    if (req->priority >= PRIO_LEVELS || req->priority < 0)
        return -EINVAL;
    
    spin_lock_irqsave(&pq->lock, flags);
    list_add_tail(&req->list, &pq->queues[req->priority]);
    atomic_inc(&pq->count);
    spin_unlock_irqrestore(&pq->lock, flags);
    
   // wake_up(&pq->wq);
    return 0;
}

struct io_request *prio_queue_get(struct prio_queue *pq)
{
    unsigned long flags;
    struct io_request *req = NULL;
    int level;
    
    spin_lock_irqsave(&pq->lock, flags);
    
    for (level = 0; level < PRIO_LEVELS; level++) {
        if (!list_empty(&pq->queues[level])) {
            req = list_first_entry(&pq->queues[level], 
                                 struct io_request, list);
            req_get(req);
            list_del(&req->list);
            atomic_dec(&pq->count);
            break;
        }
    }
    
    spin_unlock_irqrestore(&pq->lock, flags);
   
    return req;
}