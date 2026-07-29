#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>

#define NUM_THREADS 10
#define TEST_DURATION 30 // 测试持续时间(秒)
#define MAX_REQUESTS 100000

const char *proc_files[] = {
    "/proc/chip_io/high",
    "/proc/chip_io/normal",
    "/proc/chip_io/low"
};

// 测试统计
typedef struct {
    int requests_sent;
    int requests_failed;
    long total_latency_ns;
} ThreadStats;

// 全局统计
typedef struct {
    ThreadStats prio_stats[3]; // 0=high, 1=normal, 2=low
    int running;
} GlobalStats;

GlobalStats stats;

// 获取当前时间(纳秒)
long long get_nanos() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000000000LL + ts.tv_nsec;
}

// 测试线程函数
void *test_thread(void *arg) {
    int priority = *(int *)arg;
    char buf[32];
    long long start_time;
    
    while (__atomic_load_n(&stats.running, __ATOMIC_RELAXED)) {
        // 准备测试数据
        snprintf(buf, sizeof(buf), "thread%ld_%d", (long)pthread_self(), rand()%1000);
        
        // 同步写入测试
        start_time = get_nanos();
        int fd = open(proc_files[priority], O_WRONLY | O_SYNC);
        if (fd < 0) {
            __atomic_fetch_add(&stats.prio_stats[priority].requests_failed, 1, __ATOMIC_RELAXED);
            continue;
        }
        
        if (write(fd, buf, strlen(buf)) < 0) {
            __atomic_fetch_add(&stats.prio_stats[priority].requests_failed, 1, __ATOMIC_RELAXED);
        } else {
            __atomic_fetch_add(&stats.prio_stats[priority].requests_sent, 1, __ATOMIC_RELAXED);
            long latency = get_nanos() - start_time;
            __atomic_fetch_add(&stats.prio_stats[priority].total_latency_ns, latency, __ATOMIC_RELAXED);
        }
        close(fd);
        
        // 异步写入测试(随机进行)
        if (rand() % 5 == 0) { // 20%概率测试异步
            fd = open(proc_files[priority], O_WRONLY);
            if (fd >= 0) {
                write(fd, buf, strlen(buf));
                close(fd);
            }
        }
        
        // 随机延迟(0-10ms)
        usleep(rand() % 10000);
    }
    return NULL;
}

// 监控线程(定期打印状态)
void *monitor_thread(void *arg) {
    time_t start = time(NULL);
    
    while (__atomic_load_n(&stats.running, __ATOMIC_RELAXED)) {
        system("cat /proc/chip_io/status");
        printf("\n=== 测试统计 ===\n");
        printf("持续时间: %ld秒\n", time(NULL) - start);
        
        for (int i = 0; i < 3; i++) {
            ThreadStats *s = &stats.prio_stats[i];
            printf("优先级 %d: 请求数=%d 失败=%d 平均延迟=%.2fms\n",
                  i, s->requests_sent, s->requests_failed,
                  s->requests_sent ? (s->total_latency_ns / 1000000.0 / s->requests_sent) : 0);
        }
        printf("================\n\n");
        sleep(5);
    }
    return NULL;
}

int main() {
    pthread_t threads[NUM_THREADS];
    pthread_t monitor;
    int priorities[NUM_THREADS];
    
    // 初始化
    memset(&stats, 0, sizeof(stats));
    stats.running = 1;
    
    // 创建监控线程
    pthread_create(&monitor, NULL, monitor_thread, NULL);
    
    // 创建测试线程
    for (int i = 0; i < NUM_THREADS; i++) {
        priorities[i] = i % 3; // 均匀分配优先级
        pthread_create(&threads[i], NULL, test_thread, &priorities[i]);
    }
    
    // 运行测试
    sleep(TEST_DURATION);
    
    // 停止测试
    __atomic_store_n(&stats.running, 0, __ATOMIC_RELAXED);
    
    // 等待线程结束
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }
    pthread_join(monitor, NULL);
    
    // 最终统计
    printf("\n=== 最终测试结果 ===\n");
    for (int i = 0; i < 3; i++) {
        ThreadStats *s = &stats.prio_stats[i];
        printf("优先级 %d:\n", i);
        printf("  总请求数: %d\n", s->requests_sent);
        printf("  失败数: %d\n", s->requests_failed);
        printf("  平均延迟: %.2fms\n", 
              s->requests_sent ? (s->total_latency_ns / 1000000.0 / s->requests_sent) : 0);
        printf("  QPS: %.2f\n", s->requests_sent / (double)TEST_DURATION);
    }
    
    // 边界条件测试
    printf("\n=== 边界条件测试 ===\n");
    
    // 测试超大请求
    char *big_data = malloc(1024 * 1024);
    memset(big_data, 'A', 1024 * 1024);
    int fd = open("/proc/chip_io/high", O_WRONLY);
    printf("写入1MB数据结果: %zd\n", write(fd, big_data, 1024 * 1024));
    close(fd);
    free(big_data);
    
    // 测试空请求
    fd = open("/proc/chip_io/high", O_WRONLY);
    printf("写入0字节结果: %zd\n", write(fd, NULL, 0));
    close(fd);
    
    return 0;
}
