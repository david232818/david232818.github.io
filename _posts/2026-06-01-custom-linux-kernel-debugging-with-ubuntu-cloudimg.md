---
title: "Custom Linux Kernel Debugging With Ubuntu Cloud Image"
tags: background
toc: true
---


## Introduction

CIDY님은 우분투 클라우드 이미지를 사용하여 커널 디버깅 환경을 구축하는 방법을 설명하였다. 이 방법은 우분투 클라우드 이미지를
다운받아서 QEMU로 구동하고, 이 이미지에서 사용하는 소스와 디버깅 심볼을 GDB에 등록하여 디버깅을 하는 방식이다. 그리고
BACK ON TOP님은 우분투에서 커널 소스를 빌드하여 우분투에 설치하는 방법을 설명하였다. 본 글에서는 이 두 글을 기반으로
우분투에 빌드한 커널을 설치하고 디버깅하는 방법을 정리한다\[1, 2\].

## Ubuntu Cloud Image Basic Setup

먼저 ubuntu라는 이름의 디렉토리를 생성하자. 여기서는 이 디렉토리를 루트로 하여 설명할 것이다. 이제 우분투 이미지를
다운받는다.

``` bash
cd ubuntu/img/
wget https://cloud-images.ubuntu.com/jammy/current/jammy-server-cloudimg-amd64.img
```

user-data.yaml 파일을 다음과 같이 작성하고

``` text
#cloud-config
ssh_pwauth: true
chpasswd:
  expire: false
  users:
    - name: ubuntu
      password: '1234'
      type: text
```

다음과 같이 이미지를 변환한다.

``` bash
cloud-localds seed.raw user-data.yaml
qemu-img convert -f raw -O qcow2 seed.raw seed.img
qemu-img resize jammy-server-cloudimg-amd64.img +45G
```

이때 45 기가 바이트나 주는 이유는 이후에 커널에 디버깅 정보를 포함하여 빌드할 때 상당한 용량을 요하기 때문이다. 그럼 다음과
같이 QEMU를 구동할 수 있다.

``` bash
sudo qemu-system-x86_64 -enable-kvm -m 4G -nographic \
      -netdev id=net00,type=user,hostfwd=tcp::2222-:22 \
      -device virtio-net-pci,netdev=net00 \
      -drive if=virtio,format=qcow2,file=jammy-server-cloudimg-amd64.img \
      -drive if=virtio,format=qcow2,file=seed.img
```

## Linux Kernel Compilation with Debian Package in QEMU

QEMU로 구동된 우분투에서 리눅스 커널 컴파일은 다음과 같이 할 수 있고, 여기서는 5.18 버전을 설치할 것이다\[3\].

``` bash
sudo apt update
sudo apt install build-essential libncurses5 libncurses5-dev bin86 libssl-dev bison flex libelf-dev -y
wget https://www.kernel.org/pub/linux/kernel/v5.x/linux-5.18.tar.gz
sudo tar zxf linux-5.18.tar.gz -C/usr/src
cd /usr/src/linux-5.18/
sudo cp /boot/config-5.15.0-179-generic ./.config
sudo make olddefconfig
sudo scripts/config --disable SYSTEM_TRUSTED_KEYS
sudo scripts/config --disable SYSTEM_REVOCATION_KEYS
sudo make -j8 bindeb-pkg DEB_BUILD_OPTIONS="nostrip debug"
```

위 make 명령어에서 bindeb-pkg는 (여기서는) 리눅스 커널을 .deb 패키지로 만들며,
DEB\_BUILDOPTIONS를 설정해서 디버깅 정보를 포함하도록 만든다. 이때 컴파일 초반에 터미널에서 질문이
뜰 것인데 엔터키를 입력하여 기본값으로 넘어가면 된다\[4\].

<br>

컴파일이 완료되면 다음과 같이 .deb 파일이 생성된다.

``` text
$ ls /usr/src/
linux-5.18
linux-headers-5.15.0-179
linux-headers-5.15.0-179-generic
linux-headers-5.18.0
linux-headers-5.18.0_5.18.0-1_amd64.deb
linux-image-5.18.0-dbg_5.18.0-1_amd64.deb
linux-image-5.18.0_5.18.0-1_amd64.deb
linux-libc-dev_5.18.0-1_amd64.deb
linux-upstream_5.18.0-1_amd64.buildinfo
linux-upstream_5.18.0-1_amd64.changes
$
```

그리고 다음과 같이 커널을 설치할 수 있다.

``` bash
cd /usr/src/
sudo dpkg -i *.deb
sudo update-grub
sudo reboot
```

## Debugging Environment Setup

이제 디버깅 정보가 있는 파일을 사용하여 환경을 구성해보자. 먼저 QEMU 게스트에서 디버깅 관련 파일을 /tmp 디렉토리로
복사한다.

``` bash
tar cfz /tmp/boot.tar.gz /boot/*.18.0
tar cfz /tmp/dbg.tar.gz /usr/src/linux-image-5.18.0-dbg_5.18.0-1_amd64.deb
```

그 다음, 호스트에서 다음과 같이 위 tarball 파일을 다운받는다.

``` bash
cd ubuntu/
scp -P 2222 ubuntu@localhost:/tmp/boot.tar.gz ./
scp -P 2222 ubuntu@localhost:/tmp/dbg.tar.gz ./
tar zxf boot.tar.gz             # boot directory is generated
tar zxf dbg.tar.gz              # usr directory is generated
```

앞서 우분투 게스트에서 설치한 커널이 5.18 버전이므로 호스트에도 다음과 같이 동일하게 구성한다.

``` bash
cd ubuntu/
wget https://www.kernel.org/pub/linux/kernel/v5.x/linux-5.18.tar.gz
tar zxf linux-5.18.tar.gz
cd usr/src/
sudo apt-get install ./linux-image-5.18.0-dbg_5.18.0-1_amd64.deb
cd ubuntu/linux-5.18/
cp /lib/debug/boot/vmlinux-5.18.0 ./vmlinux
cp -Rf /usr/lib/debug/lib/modules/5.18.0/* ./
cp ../boot/config-5.18.0 ./.config
make olddefconfig
make scripts_gdb
echo -e "set auto-load safe-path /\n" >> ~/.gdbinit
echo -e "add-auto-load-safe-path /\n" >> ~/.gdbinit
sudo bash -c 'echo -e "set auto-load safe-path /\n" >> /root/.gdbinit'
```

## Debugging Test

앞서 /boot 디렉토리 내 파일을 복사한 것으로 다음과 같이 QEMU 스크립트를 작성할 수 있다.

``` bash
#!/usr/bin/env bash
RAM="4G"
CPU_SOCKETS="1"
CPU_CORES="2"
CPU_THREADS="2"
args=(
  # kvm 켜기, RAM 크기 설정
  -enable-kvm -m "$RAM"
  # CPU 코어 설정
  -smp "$CPU_THREADS",cores="$CPU_CORES",sockets="$CPU_SOCKETS"
  # 사용할 kernel image
  -kernel ./boot/vmlinuz-5.18.0
  # 사용할 initial ramdisk
  -initrd ./boot/initrd.img-5.18.0
  # kernel command line, /dev/vda1을 mount하고, 시리얼 포트를 이용해 콘솔 연결, kaslr 꺼서 디버깅 편하도록
  -append "root=/dev/vda1 console=tty1 console=ttyS0 nokaslr"
    # host의 port 2222를 guest의 port 22로 포트 포워딩 
  -netdev id=net00,type=user,hostfwd=tcp::2222-:22 
  -device virtio-net-pci,netdev=net00
  # 사용할 drive
  -drive if=virtio,format=qcow2,file=./img/jammy-server-cloudimg-amd64.img
  -drive if=virtio,format=qcow2,file=./img/seed.img
    -nographic
  # vnc로 접속 가능하도록 설정
  # -vnc :1
  # qemu monitor 사용 가능하도록 설정
  # -monitor stdio
  # gdb에서 target remote :1234로 attach할 수 있음
  -s
)
qemu-system-x86_64 "${args[@]}" 2>&1
```

그리고 gdb를 QEMU에 연결하면 다음과 같이 소스 코드가 표시되면서 디버깅할 수 있음을 알 수 있다.

``` text
$ gdb -q ./vmlinux
Reading symbols from ./vmlinux...
------- tip of the day (disable with set show-tips off) -------
break-if-taken and break-if-not-taken commands sets breakpoints after a given jump instruction was taken or not
pwndbg> target remote :1234
Remote debugging using :1234
0xffffffff825a91bb in native_safe_halt () at ./arch/x86/include/asm/irqflags.h:52
52      }
Permission error when attempting to parse page tables with gdb-pt-dump.
Either change the kernel-vmmap setting, re-run GDB as root, or disable `ptrace_scope` (`echo 0 | sudo tee /proc/sys/kernel/yama/ptrace_scope`)   
Warning: Avoided exploring possible address 0xffffffff825a9010.
You can explicitly explore it with `vmmap-explore 0xffffffff825a9000`                                                                            
Warning: Avoided exploring possible address 0xffffffff83425140.
You can explicitly explore it with `vmmap-explore 0xffffffff83425000`                                                                            
Warning: Avoided exploring possible address 0xffffffff8258ea6a.
You can explicitly explore it with `vmmap-explore 0xffffffff8258e000`                                                                            
Warning: Avoided exploring possible address 0xffffed102134857e.
You can explicitly explore it with `vmmap-explore 0xffffed1021348000`                                                                            
Warning: Avoided exploring possible address 0xffffffff82c5aaa0.
You can explicitly explore it with `vmmap-explore 0xffffffff82c5a000`                                                                            
Warning: Avoided exploring possible address 0xffff888109a42beb.
You can explicitly explore it with `vmmap-explore 0xffff888109a42000`                                                                            
Warning: Avoided exploring possible address 0xffffffff8414c1a0.
You can explicitly explore it with `vmmap-explore 0xffffffff8414c000`                                                                            
Warning: Avoided exploring possible address 0xffffffff83407db0.
You can explicitly explore it with `vmmap-explore 0xffffffff83407000`                                                                            
Warning: Avoided exploring possible address 0xffff88810465a000.
You can explicitly explore it with `vmmap-explore 0xffff88810465a000`                                                                            
Warning: Avoided exploring possible address 0xffff888109a00000.
You can explicitly explore it with `vmmap-explore 0xffff888109a00000`                                                                            
Warning: Avoided exploring possible address 0xffffffff81072c05.
You can explicitly explore it with `vmmap-explore 0xffffffff81072000`                                                                            
LEGEND: STACK | HEAP | CODE | DATA | WX | RODATA
─────────────────────────────────────────────[ REGISTERS / show-flags off / show-compact-regs off ]──────────────────────────────────────────────
 RAX  0xffffffff825a9010 (default_idle) ◂— 0xffffffff825a9010
 RBX  0xffffffff83425140 (init_task) ◂— 0xffffffff83425140
 RCX  0xffffffff8258ea6a (rcu_eqs_enter.constprop+154) ◂— 0xffffffff8258ea6a
 RDX  0xffffed102134857e ◂— 0xffffed102134857e
 RDI  0xffffffff82c5aaa0 ◂— 0xffffffff82c5aaa0
 RSI  0xffffffff82c5aa60 ◂— 0xffffffff82c5aa60
 R8   1
 R9   0xffff888109a42beb ◂— 0xffff888109a42beb
 R10  0xffffed102134857d ◂— 0xffffed102134857d
 R11  1
 R12  0
 R13  0xffffffff8414c1a0 (__cpu_online_mask) ◂— 0xffffffff8414c1a0
 R14  0
 R15  0
 RBP  0xffffffff83407db0 (init_thread_union+32176) —▸ 0xffffffff83407dc0 (init_thread_union+32192) —▸ 0xffffffff83407de0 (init_thread_union+32224) —▸ 0xffffffff83407e90 (init_thread_union+32400) —▸ 0xffffffff83407ea8 (init_thread_union+32424) ◂— ...
 RSP  0xffffffff83407da8 (init_thread_union+32168) —▸ 0xffffffff825a901e (default_idle+14) ◂— 0xffffffff825a901e
 RIP  0xffffffff825a91bb (native_safe_halt+11) ◂— 0xffffffff825a91bb
 CR0  0x80050033 [ PE WP PG ]
 CR3  0x10465a000 [virtual: 0xffff88810465a000 ◂— 0xffff88810465a000]
 CR4  0x6f0 [ umip fsgsbase smep smap pke cet pks ]
 EFER 0xd01 [ NXE ]
 GS_BASE 0xffff888109a00000 ◂— 0xffff888109a00000
 FS_BASE 0
 CS   0x10    SS   0x18    DS   0x0    ES   0x0    FS   0x0    GS   0x0   
──────────────────────────────────────────────────────[ DISASM / x86-64 / set emulate on ]───────────────────────────────────────────────────────
 ► 0xffffffff825a91bb <native_safe_halt+11>    ret                                <default_idle+14>
    ↓
   0xffffffff825a901e <default_idle+14>        nop    
   0xffffffff825a901f <default_idle+15>        pop    rbp
   0xffffffff825a9020 <default_idle+16>        ret    

   0xffffffff825a9021 <default_idle+17>        int3   
   0xffffffff825a9022                          int3   
   0xffffffff825a9023                          int3   
   0xffffffff825a9024                          int3   
   0xffffffff825a9025                          int3   
   0xffffffff825a9026                          int3   
   0xffffffff825a9027                          int3   
────────────────────────────────────────────────────────────────[ SOURCE (CODE) ]────────────────────────────────────────────────────────────────
In file: ubuntu/linux-5.18/arch/x86/include/asm/irqflags.h:52
   47 
   48 static inline __cpuidle void native_safe_halt(void)
   49 {
   50         mds_idle_clear_cpu_buffers();
   51         asm volatile("sti; hlt": : :"memory");
 ► 52 }
   53 
   54 static inline __cpuidle void native_halt(void)
   55 {
   56         mds_idle_clear_cpu_buffers();
   57         asm volatile("hlt": : :"memory");
────────────────────────────────────────────────────────────────────[ STACK ]────────────────────────────────────────────────────────────────────
00:0000│ rsp 0xffffffff83407da8 (init_thread_union+32168) —▸ 0xffffffff825a901e (default_idle+14) ◂— 0xffffffff825a901e
01:0008│ rbp 0xffffffff83407db0 (init_thread_union+32176) —▸ 0xffffffff83407dc0 (init_thread_union+32192) —▸ 0xffffffff83407de0 (init_thread_union+32224) —▸ 0xffffffff83407e90 (init_thread_union+32400) —▸ 0xffffffff83407ea8 (init_thread_union+32424) ◂— ...
02:0010│+008 0xffffffff83407db8 (init_thread_union+32184) —▸ 0xffffffff81072c05 (arch_cpu_idle+21) ◂— 0xffffffff81072c05
03:0018│+010 0xffffffff83407dc0 (init_thread_union+32192) —▸ 0xffffffff83407de0 (init_thread_union+32224) —▸ 0xffffffff83407e90 (init_thread_union+32400) —▸ 0xffffffff83407ea8 (init_thread_union+32424) —▸ 0xffffffff83407ed0 (init_thread_union+32464) ◂— ...
04:0020│+018 0xffffffff83407dc8 (init_thread_union+32200) —▸ 0xffffffff825a93b3 (default_idle_call+99) ◂— 0xffffffff825a93b3
05:0028│+020 0xffffffff83407dd0 (init_thread_union+32208) ◂— 0xffffffff83407dd0
06:0030│+028 0xffffffff83407dd8 (init_thread_union+32216) —▸ 0xffffffff83425140 (init_task) ◂— 0xffffffff83425140
07:0038│+030 0xffffffff83407de0 (init_thread_union+32224) —▸ 0xffffffff83407e90 (init_thread_union+32400) —▸ 0xffffffff83407ea8 (init_thread_union+32424) —▸ 0xffffffff83407ed0 (init_thread_union+32464) —▸ 0xffffffff83407ee0 (init_thread_union+32480) ◂— ...
──────────────────────────────────────────────────────────────────[ BACKTRACE ]──────────────────────────────────────────────────────────────────
 ► 0 0xffffffff825a91bb native_safe_halt+11
   1 0xffffffff825a901e default_idle+14
   2 0xffffffff825a901e default_idle+14
   3 0xffffffff81072c05 arch_cpu_idle+21
   4 0xffffffff825a93b3 default_idle_call+99
   5 0xffffffff811b2578 do_idle+904
   6 0xffffffff811b2578 do_idle+904
   7 0xffffffff811b28f0 cpu_startup_entry+32
──────────────────────────────────────────────────────────────[ THREADS (2 TOTAL) ]──────────────────────────────────────────────────────────────
  ► 1   "" stopped: 0xffffffff825a91bb <native_safe_halt+11> 
    2   "" stopped: 0xffffffff825a91bb <native_safe_halt+11> 
─────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
pwndbg> 
```

이때 커널 모듈을 디버깅할 때는 GDB를 붙이기 전에 모듈을 로드하는 것이 좋은 것으로 보인다.

## References

1.  CIDY, "\[Linux Kernel\] Kernel Debugging Environment setup."
    orcinus-orca.tistory.com, Accessed: Jun. 01, 2026. \[Online\].
    Available: <https://orcinus-orca.tistory.com/246>
2.  BACK ON TOP, "\[Linux/Ubuntu\] 리눅스 커널 컴파일, 시스템 호출."
    hjrrkd.tistory.com, Accessed: Jun. 01, 2026. \[Online\]. Available:
    <https://hjrrkd.tistory.com/entry/LinuxUbuntu-%EB%A6%AC%EB%88%85%EC%8A%A4-%EC%BB%A4%EB%84%90-%EC%BB%B4%ED%8C%8C%EC%9D%BC-%EC%8B%9C%EC%8A%A4%ED%85%9C-%ED%98%B8%EC%B6%9C>
3.  ME74, "No rule to make target 'debian/canonical-revoked-certs.pem'."
    me74.tistory.com, Accessed: Jun. 01, 2026. \[Online\]. Available:
    <https://me74.tistory.com/145>
4.  "BuildADebianKernelPackage." wiki.debian.org, Accessed: Jun. 01,
    2026. \[Online\]. Available:
    <https://wiki.debian.org/BuildADebianKernelPackage>
