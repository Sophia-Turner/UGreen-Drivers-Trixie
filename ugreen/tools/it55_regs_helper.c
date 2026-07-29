#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/uaccess.h>
#include <linux/string.h>
#include "ec_driver.h"

#define PROC_ENTRY_NAME "ec_mem"  // /proc入口名称
#define MAX_INPUT_LEN 32          // 最大输入长度

// 全局变量存储当前操作状态
static struct {
    u8 address;     // 当前操作的EC地址
    u8 value;       // 当前值(用于读取/写入)
    bool is_read;   // 标记当前是读操作还是写操作
} ec_state;

// 显示操作界面和结果
static int proc_show(struct seq_file *m, void *v)
{
    seq_puts(m, "EC Memory Access Interface\n");
    seq_puts(m, "=========================\n");
    seq_printf(m, "Current address: 0x%02x\n", ec_state.address);
    
    if (ec_state.is_read) {
        seq_printf(m, "Last read value: 0x%02x\n", ec_state.value);
    } else {
        seq_printf(m, "Last written value: 0x%02x\n", ec_state.value);
    }
    
    seq_puts(m, "\nUsage:\n");
    seq_puts(m, "  read <hex_addr>  - Read from EC memory\n");
    seq_puts(m, "  write <hex_addr> <hex_value> - Write to EC memory\n");
    seq_puts(m, "Example:\n");
    seq_puts(m, "  echo \"read 0x9e\" > /proc/ec_mem\n");
    seq_puts(m, "  echo \"write 0x9e 0x80\" > /proc/ec_mem\n");
    
    return 0;
}

// 打开/proc文件
static int proc_open(struct inode *inode, struct file *file)
{
    return single_open(file, proc_show, NULL);
}

// 处理用户输入
static ssize_t proc_write(struct file *file, const char __user *buf, 
                         size_t count, loff_t *ppos)
{
    char input[MAX_INPUT_LEN] = {0};
    char cmd[10] = {0};
    unsigned long addr = 0, val = 0;
    int ret = 0;
    
    // 复制用户输入
    if (copy_from_user(input, buf, min(count, sizeof(input) - 1)))
        return -EFAULT;
    
    // 解析输入
    if (sscanf(input, "%9s %lx %lx", cmd, &addr, &val) < 2) {
        pr_err("Invalid input format\n");
        return -EINVAL;
    }
    
    // 检查地址范围
    if (addr > 0xFF) {
        pr_err("Address out of range (0x00-0xFF)\n");
        return -EINVAL;
    }
    
    ec_state.address = (u8)addr;
    
    // 处理读命令
    if (strcmp(cmd, "read") == 0) {
        ret = ec_read_memory(ec_state.address, &ec_state.value);
        if (ret) {
            pr_err("Failed to read from EC 0x%02x, error %d\n", 
                  ec_state.address, ret);
            return ret;
        }
        ec_state.is_read = true;
        pr_info("Read EC 0x%02x: 0x%02x\n", 
               ec_state.address, ec_state.value);
    } 
    // 处理写命令
    else if (strcmp(cmd, "write") == 0) {
        if (val > 0xFF) {
            pr_err("Value out of range (0x00-0xFF)\n");
            return -EINVAL;
        }
        
        ret = ec_write_memory(ec_state.address, (u8)val);
        if (ret) {
            pr_err("Failed to write to EC 0x%02x, error %d\n", 
                  ec_state.address, ret);
            return ret;
        }
        ec_state.value = (u8)val;
        ec_state.is_read = false;
        pr_info("Write EC 0x%02x: 0x%02x\n", 
               ec_state.address, ec_state.value);
    } else {
        pr_err("Unknown command: %s\n", cmd);
        return -EINVAL;
    }
    
    return count;
}

// 文件操作结构
static const struct proc_ops proc_fops = {
    .proc_open = proc_open,
    .proc_read = seq_read,
    .proc_write = proc_write,
    .proc_release = single_release,
};

// 模块初始化
static int __init ec_test_init(void)
{
    // 初始化状态
    ec_state.address = 0x9e;  // 默认地址
    ec_state.value = 0;
    ec_state.is_read = true;
    
    // 创建/proc入口
    if (!proc_create(PROC_ENTRY_NAME, 0666, NULL, &proc_fops)) {
        pr_err("Failed to create /proc/%s\n", PROC_ENTRY_NAME);
        return -ENOMEM;
    }
    
    pr_info("EC memory test module loaded\n");
    return 0;
}

// 模块退出
static void __exit ec_test_exit(void)
{
    remove_proc_entry(PROC_ENTRY_NAME, NULL);
    pr_info("EC memory test module unloaded\n");
}

module_init(ec_test_init);
module_exit(ec_test_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("drivers@ugreen");
MODULE_DESCRIPTION("it55 Memory Access  Module");