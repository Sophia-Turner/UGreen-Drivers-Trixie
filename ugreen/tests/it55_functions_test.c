#include "prio_queue.h"
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/uaccess.h>
#include <linux/delay.h>
#include <linux/module.h>
#include "ec_driver.h"
#define DRV_NAME "chip_prio"
#define PROC_NAME "chip_io"


#define EC_IT55_WDT_SWITCH 0x55
//	MSB：0x56，LSB：0x57
#define EC_IT55_WDT_TIME_MSB 0x56
#define EC_IT55_WDT_TIME_LSB 0x57


#define BKLT_PWM_Value 0xa5
static int wdt_get_counter(void)
{
	unsigned char tmp1, tmp2;
	ec_read_memory(EC_IT55_WDT_TIME_MSB,&tmp1);
	ec_read_memory(EC_IT55_WDT_TIME_LSB,&tmp2);
	return ((tmp1 << 8) & 0xff00) | (tmp2 & 0xff);
}
static int wdt_update(int value)
{
	ec_write_memory(EC_IT55_WDT_TIME_MSB, (u8)((value & 0xff00) >> 8));
	ec_write_memory(EC_IT55_WDT_TIME_LSB, value & 0xff);
	return 0;
}
static struct chip_device *chip_dev;
static u8 backlight = 0;
static int perform_chip_io(struct io_request *req)
{
	// 模拟芯片IO操作
	pr_info("Processing IO request, prio=%d, data_len=%zu\n", req->priority,
		req->data_len);

	// 模拟处理延迟
	switch (req->priority) {
	case 0:
        wdt_update(0xff);
        ec_write_memory( EC_IT55_WDT_SWITCH, 1);
        ec_write_memory( EC_IT55_WDT_SWITCH, 0);
        printk("######wdt %d",wdt_get_counter());
		break;
	case 1:
        backlight++;
        ec_write_memory(BKLT_PWM_Value,backlight );
		break;
	case 2:
        ec_write_memory(0x9e,1 );
        ec_write_memory(0x9e,0);
		break;
	}

	return 0; // 成功
}

static void chip_work_handler(struct work_struct *work)
{
	struct chip_device *dev =
		container_of(work, struct chip_device, dwork.work);
	struct io_request *req;

	while ((req = prio_queue_get(&dev->pq)) != NULL) {
		req->result = perform_chip_io(req);

		if (req->is_sync)
			complete(&req->done);
		else
			req_put(req);   // 异步请求在此释放
		req_put(req);  
	}

	//atomic_set(&dev->pq.io_in_progress, 0);
}
static int list_count(struct list_head *head)
{
	struct list_head *pos;
	int count = 0;

	list_for_each(pos, head) {
		count++;
	}

	return count;
}
static int chip_proc_show(struct seq_file *m, void *v)
{
	int level;

	seq_puts(m, "Chip Priority Queue Status\n");
	seq_printf(m, "Total pending requests: %d\n",
		   atomic_read(&chip_dev->pq.count));

	spin_lock(&chip_dev->pq.lock);
	for (level = 0; level < PRIO_LEVELS; level++) {
		seq_printf(m, "Priority %d: %d requests\n", level,
			   list_count(&chip_dev->pq.queues[level]));
	}
	spin_unlock(&chip_dev->pq.lock);

	return 0;
}

static int chip_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, chip_proc_show, NULL);
}

static ssize_t chip_proc_write(struct file *file, const char __user *buf,
			       size_t count, loff_t *ppos)
{
	struct io_request *req;
	int priority = 1; // 默认中等优先级
	char *data;
	bool is_sync = true; //(file->f_flags & O_SYNC);
	// 根据文件名确定优先级
	if (strstr(file->f_path.dentry->d_name.name, "high"))
		priority = 0;
	else if (strstr(file->f_path.dentry->d_name.name, "low"))
		priority = 2;

	// 一次性分配结构体+数据内存
	req = kmalloc(sizeof(*req) + count, GFP_KERNEL);
    if (!req) {
		pr_err("chip_proc_write: kmalloc failed\n");
		return -ENOMEM;
	} 
	kref_init(&req->refcount); 
	req->is_sync = is_sync;
	// 数据部分指针
	data = req->data;
	// 直接从用户空间拷贝到最终位置
	if (copy_from_user(data, buf, count)) {
		pr_err("chip_proc_write: copy_from_user failed, req=%px\n", req);
		req_put(req);  
		return -EFAULT;
	}

	INIT_LIST_HEAD(&req->list);
	req->priority = priority;
	req->data_len = count;
	init_completion(&req->done);
	if (prio_queue_put(&chip_dev->pq, req)) {
		pr_err("chip_proc_write: prio_queue_put failed, req=%px\n", req);
		req_put(req);  
		return -EAGAIN;
	}

	//if (!atomic_xchg(&chip_dev->pq.io_in_progress, 1))
	queue_delayed_work(chip_dev->wq, &chip_dev->dwork, 0);
	if (is_sync) {
		if (wait_for_completion_interruptible(&req->done)) {
			req_put(req);  
			return -ERESTARTSYS;
		}
		count = req->result;
	}

	// 异步操作立即返回，由工作队列负责释放
	if (!is_sync)
		return count;

	req_put(req);  
	return count;
}

static const struct proc_ops chip_proc_fops = {
	.proc_open = chip_proc_open,
	.proc_read = seq_read,
	.proc_write = chip_proc_write,
	.proc_lseek = seq_lseek,
	.proc_release = single_release,
};

static int __init chip_init(void)
{
	int ret = 0;

	chip_dev = kzalloc(sizeof(*chip_dev), GFP_KERNEL);
	if (!chip_dev)
		return -ENOMEM;

	prio_queue_init(&chip_dev->pq);
	mutex_init(&chip_dev->io_lock);

	// 创建有序工作队列确保串行化
	chip_dev->wq = alloc_ordered_workqueue(DRV_NAME, WQ_MEM_RECLAIM);
	if (!chip_dev->wq) {
		ret = -ENOMEM;
		goto err_wq;
	}

	INIT_DELAYED_WORK(&chip_dev->dwork, chip_work_handler);

	// 创建proc接口
	chip_dev->proc_entry = proc_mkdir(PROC_NAME, NULL);
	if (!chip_dev->proc_entry) {
		ret = -ENOMEM;
		goto err_proc;
	}

	proc_create("high", 0666, chip_dev->proc_entry, &chip_proc_fops);
	proc_create("normal", 0666, chip_dev->proc_entry, &chip_proc_fops);
	proc_create("low", 0666, chip_dev->proc_entry, &chip_proc_fops);
	proc_create("status", 0444, chip_dev->proc_entry, &chip_proc_fops);

	pr_info("Chip priority queue driver loaded\n");
	return 0;

err_proc:
	destroy_workqueue(chip_dev->wq);
err_wq:
	prio_queue_cleanup(&chip_dev->pq);
	kfree(chip_dev);
	return ret;
}

static void __exit chip_exit(void)
{
	remove_proc_entry("high", chip_dev->proc_entry);
	remove_proc_entry("normal", chip_dev->proc_entry);
	remove_proc_entry("low", chip_dev->proc_entry);
	remove_proc_entry("status", chip_dev->proc_entry);
	remove_proc_entry(PROC_NAME, NULL);

	cancel_delayed_work_sync(&chip_dev->dwork);
	destroy_workqueue(chip_dev->wq);
	prio_queue_cleanup(&chip_dev->pq);
	kfree(chip_dev);

	pr_info("Chip priority queue driver unloaded\n");
}

module_init(chip_init);
module_exit(chip_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("Priority queue chip driver");
MODULE_VERSION("1.0");