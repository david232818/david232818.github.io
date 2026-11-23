---
title: FreeBSD debugging environment
tags: background
toc: true
---


## Introduction
본 글에서는 [1]을 정리하여 Windows 11 WSL2상에서 FreeBSD 원격 디버깅 환경을 구성하는 방법을 다룬다. 이때 host는 Windows 11 WSL2, guest는 FreeBSD이다. 비록 [1]은 FreeBSD 13.0을 사용하였지만, 이 글을 쓰는 시점에는 13.0 관련 파일을 다운받을 수 없었으므로 여기서는 13.1을 사용하였다. 또한, 여기서는 working directory를 /mnt/c/Users/Username/working/directory/FreeBSD/13.0/이라고 가정하여 설명한다.

## FreeBSD configuration for GDB
FreeBSD는 리눅스와 OS ABI가 다르기 때문에 FreeBSD 커널 이미지의 symbols를 일반적으로 설치된 (e.g., sudo apt install gdb -y) GDB를 사용하여 읽는 것은 OS ABI handler 관련 오류를 발생시킬 수 있다. 따라서 GDB에 FreeBSD configuration을 적용하여 빌드할 필요가 있다.


그럼 GDB 소스를 다운받고 tarball을 풀자.

```bash
(host)$ wget http://ftp.gnu.org/gnu/gdb/gdb-10.2.tar.xz
(host)$ tar -xf gdb-10.2.tar.xz
```


위 작업이 정상적으로 완료되었다면, gdb-10.2 directory로 들어가서 FreeBSD configuration을 해주자.

```bash
(host)$ cd gdb-10.2
(host)$ ./configure --target=x86_64-portbld-freebsd13.0 --prefix /mnt/c/Users/Username/working/directory/FreeBSD/13.0/usr
```


위 설정 작업이 완료되면 GDB를 빌드할 수 있다. 이때 clock skew가 탐지됨으로 인한 경고가 표시될 수 있지만, 오류가 아니라면 넘어가도록 한다.

```bash
(host)$ make -j8
(host)$ make install
```

빌드가 정상적으로 완료되었다면, /mnt/c/Users/Username/working/directory/FreeBSD/13.0/usr/bin에 x86_64-portbld-freebsd13.0-gdb가 생성되어 있는 것을 확인할 수 있을 것이다. 이제 다시 working directory로 돌아가자.

## Setup FreeBSD in QEMU
본 섹션은 QEMU에 FreeBSD를 올리고 원격 디버깅을 하기 위한 SSH와 serial 설정을 다룬다. 이를 위해 FreeBSD 이미지를 다운받아야 한다. 상기에 언급하였듯이, 여기서는 13.1을 사용한다.

```bash
(host)$ wget http://ftp.freebsd.org/pub/FreeBSD/releases/VM-IMAGES/13.1-RELEASE/amd64/Latest/FreeBSD-13.1-RELEASE-amd64.qcow2.xz
```


이제 다운 받은 이미지를 QEMU에 올려서 FreeBSD를 실행시키자.

```bash
(host)$ xz -d FreeBSD-13.1-RELEASE-amd64.qcow2.xz
(host)$ qemu-img resize FreeBSD-13.1-RELEASE-amd64.qcow2 20G
(host)$ sudo qemu-system-x86_64 -m 1024 -hda FreeBSD-13.1-RELEASE-amd64.qcow2 -machine accel=kvm:tcg -cpu host
```

FreeBSD가 부팅되었다면, 다음과 같이 SSH와 serial을 설정하자.

```bash
(guest)# echo 'console="comconsole"' >> /boot/loader.conf
(guest)# echo 'sshd_enable="YES"' >> /etc/rc.conf
(guest)# vi /etc/ssh/sshd_config  # enable root login
(guest)# passwd  # change root password
```

이때 sshd_config 파일은 다음과 같이 설정되어 있으면 된다.

```
#       $OpenBSD: sshd_config,v 1.104 2021/07/02 05:11:21 dtucker Exp $
#       $FreeBSD$

# This is the sshd server system-wide configuration file.  See
# sshd_config(5) for more information.

# This sshd was compiled with PATH=/usr/bin:/bin:/usr/sbin:/sbin

# The strategy used for options in the default sshd_config shipped with
# OpenSSH is to specify options with their default value where
# possible, but leave them commented.  Uncommented options override the
# default value.

# Note that some of FreeBSD's defaults differ from OpenBSD's, and
# FreeBSD has a few additional options.

#Port 22
#AddressFamily any
#ListenAddress 0.0.0.0
#ListenAddress ::

#HostKey /etc/ssh/ssh_host_rsa_key
#HostKey /etc/ssh/ssh_host_ecdsa_key
#HostKey /etc/ssh/ssh_host_ed25519_key

# Ciphers and keying
#RekeyLimit default none

# Logging
#SyslogFacility AUTH
#LogLevel INFO

# Authentication:

#LoginGraceTime 2m
PermitRootLogin yes
#StrictModes yes
#MaxAuthTries 6
#MaxSessions 10

#PubkeyAuthentication yes

# The default is to check both .ssh/authorized_keys and .ssh/authorized_keys2
# but this is overridden so installations will only check .ssh/authorized_keys
AuthorizedKeysFile      .ssh/authorized_keys

#AuthorizedPrincipalsFile none

#AuthorizedKeysCommand none
#AuthorizedKeysCommandUser nobody

# For this to work you will also need host keys in /etc/ssh/ssh_known_hosts
#HostbasedAuthentication no
# Change to yes if you don't trust ~/.ssh/known_hosts for
# HostbasedAuthentication
#IgnoreUserKnownHosts no
# Don't read the user's ~/.rhosts and ~/.shosts files
#IgnoreRhosts yes

# Change to yes to enable built-in password authentication.
PasswordAuthentication yes
#PermitEmptyPasswords no

# Change to no to disable PAM authentication
#KbdInteractiveAuthentication yes

# Kerberos options
#KerberosAuthentication no
#KerberosOrLocalPasswd yes
#KerberosTicketCleanup yes
#KerberosGetAFSToken no

# GSSAPI options
#GSSAPIAuthentication no
#GSSAPICleanupCredentials yes

# Set this to 'no' to disable PAM authentication, account processing,
# and session processing. If this is enabled, PAM authentication will
# be allowed through the KbdInteractiveAuthentication and
# PasswordAuthentication.  Depending on your PAM configuration,
# PAM authentication via KbdInteractiveAuthentication may bypass
# the setting of "PermitRootLogin without-password".
# If you just want the PAM account and session checks to run without
# PAM authentication, then enable this but set PasswordAuthentication
# and KbdInteractiveAuthentication to 'no'.
#UsePAM yes

#AllowAgentForwarding yes
#AllowTcpForwarding yes
#GatewayPorts no
#X11Forwarding yes
#X11DisplayOffset 10
#X11UseLocalhost yes
#PermitTTY yes
#PrintMotd yes
#PrintLastLog yes
#TCPKeepAlive yes
#PermitUserEnvironment no
#Compression delayed
#ClientAliveInterval 0
#ClientAliveCountMax 3
#UseDNS yes
#PidFile /var/run/sshd.pid
#MaxStartups 10:30:100
#PermitTunnel no
#ChrootDirectory none
#UseBlacklist no
#VersionAddendum FreeBSD-20211221

# no default banner path
#Banner none

# override default of no subsystems
Subsystem       sftp    /usr/libexec/sftp-server

# Example of overriding settings on a per-user basis
#Match User anoncvs
#       X11Forwarding no
#       AllowTcpForwarding no
#       PermitTTY no
#       ForceCommand cvs server
```

위 파일에서 중요한 것은 "PermitRootLogin yes"와 "PasswordAuthentication yes"이다.


[1]은 다음 절차로 루트 파일 시스템 크기를 재설정하는 것을 설명한다. 그러나 여기서는 이에 대한 필요성을 느낄 수 없어 건너뛰도록 하겠다. 다음은 packetdrill 설치이다.

```bash
(guest)# pkg install git bison vim-console
(guest)# git clone https://github.com/nplab/packetdrill.git
(guest)# cd packetdrill/gtests/net/packetdrill/
(guest)# ./configure && make
(guest)# sysctl -w vm.old_mlock=1
(guest)# echo "vm.old_mlock=1" >> /etc/sysctl.c
```

만약 위 패키지를 설치했음에도 명령어를 찾을 수 없다는 오류가 표시된다면 해당 명령어를 다시 설치해주면 된다. 예를 들어, git 명령어를 찾을 수 없다는 오류가 표시된다면,

```bash
(guest)# pkg install git
```

으로 다시 설치해주면 되는 것이다.

## FreeBSD debugging environment
이제 디버깅을 위한 소스 코드와 symbols를 다운받고 QEMU상에서 구동되는 FreeBSD에 GDB를 붙일 준비가 되었다.


먼저 FreeBSD 소스 코드를 다운받고 tarball을 풀자.

```bash
(host)$ wget http://ftp.freebsd.org/pub/FreeBSD/releases/amd64/13.1-RELEASE/src.txz
(host)$ wget http://ftp.freebsd.org/pub/FreeBSD/releases/amd64/13.1-RELEASE/kernel-dbg.txz
(host)$ tar -xf src.txz
(host)$ tar -xf kernel-dbg.txz
```

위 작업은 상당한 시간을 요하며, 정상적으로 완료되었다면 다음과 같이 생성된 디렉터리들을 확인할 수 있다.

```bash
(host)$ ls ./usr/
bin  include  lib  share  src
(host)$
```


다음은 FreeBSD symbols를 다운받기 위해 네트워크를 설정하는 단계이다.

```bash
(host)$ sudo qemu-system-x86_64 -m 1024 -hda FreeBSD-13.1-RELEASE-amd64.qcow2 -nographic -echr 17 -device virtio-net-pci,netdev=n0 -netdev tap,id=n0 -s
(guest)# ifconfig vtnet0 192.168.0.213
```

```bash
(host)$ sudo ifconfig tap0 192.168.0.42
(host)$ scp -P 22 root@192.168.0.213:/usr/lib/debug/boot/kernel/kernel.debug ./
```

위 작업이 (비록, 경고가 표시됨에도) 완료되었다면 kernel.debug 파일이 생성되었음을 확인할 수 있다. 이제 kernel.debug에 있는 symbols를 GDB에 로드하고 QEMU에 붙여서 디버깅을 해보자.

```bash
(host)$ /mnt/c/Users/Username/working/directory/FreeBSD/13.0/usr/bin/x86_64-portbld-freebsd13.0-gdb kernel.debug
(host)$ usr/bin/x86_64-portbld-freebsd13.0-gdb kernel.debug
GNU gdb (GDB) 10.2
Copyright (C) 2021 Free Software Foundation, Inc.
License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>
This is free software: you are free to change and redistribute it.
There is NO WARRANTY, to the extent permitted by law.
Type "show copying" and "show warranty" for details.
This GDB was configured as "--host=x86_64-pc-linux-gnu --target=x86_64-portbld-freebsd13.0".
Type "show configuration" for configuration details.
For bug reporting instructions, please see:
<https://www.gnu.org/software/gdb/bugs/>.
Find the GDB manual and other documentation resources online at:
    <http://www.gnu.org/software/gdb/documentation/>.

For help, type "help".
Type "apropos word" to search for commands related to "word"...
Reading symbols from kernel.debug...
(gdb) target remote :1234
Remote debugging using :1234
warning: remote target does not support file transfer, attempting to access files from local filesystem.
warning: Unable to find dynamic linker breakpoint function.
GDB will be unable to debug shared library initializers
and track explicitly loaded dynamic code.
acpi_cpu_c1 () at /usr/src/sys/x86/x86/cpu_machdep.c:246
246     /usr/src/sys/x86/x86/cpu_machdep.c: No such file or directory.
(gdb)
```

위 화면에서 continue 명령어를 입력하면 제어권이 GDB에서 FreeBSD로 돌아가고, Ctrl+C 등으로 인터럽트를 발생시키면 제어권을 GDB가 얻게 된다.


이제 디버깅 환경을 구성한 것 같지만, 아직 문제가 남아있다. 위의 GDB 메시지들을 살펴보면 "/usr/src/sys/x86/x86/cpu_machdep.c: No such file or directory"가 있음을 알 수 있다. 이는 GDB에서 소스 코드를 인식하지 못했다는 것으로 소스 코드 레벨 디버깅이 안됨을 의미한다. 따라서 이 문제를 해결할 필요성은 분명히 있다.


먼저 에러 메시지를 살펴보면 소스 코드를 working directory가 아닌 root directory의 usr에서 찾고 있음을 알 수 있다. 따라서 GDB를 구동할 때 working directory의 usr를 소스 코드를 찾는 경로로 등록시켜 주어야 한다. 이는 다음과 같이 할 수 있다.

```bash
(host)$ /mnt/c/Users/Username/working/directory/FreeBSD/13.0/usr/bin/x86_64-portbld-freebsd13.0-gdb --dir=/mnt/c/Users/Username/working/directory/FreeBSD/13.0/usr/src/ kernel.debug
```

## FreeBSD debugging environment test
지금까지 FreeBSD 원격 디버깅 환경을 구성하는 방법을 알아보았다. 본 섹션에서는 정말로 함수 중단점이 걸리는지 테스트하는 방법을 다룬다.


FreeBSD는 notification subsystem으로 kqueue를 제공한다. 사용자는 이를 이용하여 특정 파일 등을 모니터링할 수 있다. Kqeueu를 이용하여 파일을 모니터링하는 예제 코드는 다음과 같다[2].

```C
#include <sys/event.h>
#include <err.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[])
{
        struct kevent event;
        struct kevent triggered;
        int kq, fd, ret;

        if (argc != 2)
                err(EXIT_FAILURE, "Usage: %s <path>\n", argv[0]);

        fd = open(argv[1], O_RDONLY);
        if (fd == -1)
                err(EXIT_FAILURE, "Failed to open %s\n", argv[1]);

        kq = kqueue();
        if (kq == -1)
                err(EXIT_FAILURE, "kqueue() failed\n");

        EV_SET(&event, fd, EVFILT_VNODE, EV_ADD | EV_CLEAR, NOTE_WRITE,
                0, NULL);

        ret = kevent(kq, &event, 1, NULL, 0, NULL);
        if (ret == -1)
                err(EXIT_FAILURE, "kevent register\n");

        if (event.flags & EV_ERROR)
                err(EXIT_FAILURE, "Event error: %s\n", strerror(event.data));

        for ( ; ; ) {
                ret = kevent(kq, NULL, 0, &triggered, 1, NULL);
                if (ret == -1) {
                        err(EXIT_FAILURE, "kevent wait\n");
                } else if (ret > 0) {
                        printf("Something happend..\n");
                }
        }
        return 0;
}
```

위 코드를 실행하는 방법은 위 코드를 컴파일한 실행 파일을 백그라운드로 구동하고 모니터링 하는 파일에 데이터를 쓰는 것이다.


하지만 여기서는 디버깅 환경을 테스트해야 하므로 위 코드를 실행하기 전에 kqueue_kevent()에 중단점을 걸자.

```bash
(gdb) b kqueue_
kqueue_acquire          kqueue_expand           kqueue_poll
kqueue_add_filteropts   kqueue_fill_kinfo       kqueue_register
kqueue_close            kqueue_fo_release       kqueue_stat
kqueue_del_filteropts   kqueue_ioctl            kqueue_task
kqueue_drain            kqueue_kevent
kqueue_drain_schedtask  kqueue_kqfilter
(gdb) b kqueue_kevent
Breakpoint 1 at 0xffffffff80bcad82: file /usr/src/sys/kern/kern_event.c, line 1320.
(gdb) c
Continuing.

```

그리고 상기의 테스트 코드를 실행하면 다음과 같이 중단점이 트리거되는 것을 확인할 수 있다.

```bash
(guest)# touch ttt_kqueue.txt
(guest)# ./ttt_kqueue ttt_kqueue.txt &
[1] 856
(guest)#
```

```bash
Breakpoint 2, kqueue_kevent (kq=kq@entry=0xfffff80003b28200,
    td=td@entry=0xfffffe004acac020, nchanges=nchanges@entry=1,
    nevents=nevents@entry=0, k_ops=k_ops@entry=0xfffffe008a10ade0,
    timeout=0x0, timeout@entry=0xfffff80003b28200)
    at /usr/src/sys/kern/kern_event.c:1320
    at /usr/src/sys/kern/kern_event.c:1320
1320            if (nchanges < 0)
(gdb) list
1315    {
1316            struct kevent keva[KQ_NEVENTS];
1317            struct kevent *kevp, *changes;
1318            int i, n, nerrors, error;
1319
1320            if (nchanges < 0)
1321                    return (EINVAL);
1322
1323            nerrors = 0;
1324            while (nchanges > 0) {
(gdb)
```

## Conclusion
이렇게 Windows 11 WSL2에서 FreeBSD 13.1을 원격으로 디버깅하는 환경을 구성하는 방법에 대해 알아보았다. 여기서 구성한 디버깅 환경은 최소한의 것이기 때문에 GDB 확장 등을 이용하여 보다 좋은 디버깅 환경을 구축할 수도 있을 것이다.

## References
1. chenshuo, "Cross debug FreeBSD kernel on Linux host with QEMU," 2021. [Online]. Available: https://github.com/chenshuo/notes/wiki/Cross-debug-FreeBSD-kernel-on-Linux-host-with-QEMU, [Accessed Oct. 7, 2023].
2. "KQUEUE(2)," FreeBSD Manual Pages, 2022. [Online]. Available: https://man.freebsd.org/cgi/man.cgi?query=kqueue&sektion=2, [Accessed Oct. 7, 2023].
