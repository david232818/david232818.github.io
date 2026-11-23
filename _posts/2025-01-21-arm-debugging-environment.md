---
title: ARM Debugging Environment Setup
tags: background
toc: true
---


## Introduction

ARM은 x86-64와 다른 아키텍처이므로 환경 구축이라는 문제가 추가된다. 그리고 여기에는 에뮬레이션이 필수적으로 요구되고 주로
QEMU가 쓰인다. 이에 여기서는 QEMU user static과 GDB로 어떻게 환경을 만들고 이용하는지를 알아볼 것이다.

## QEMU User Static

QEMU의 qemu-user-static 에뮬레이터는 에뮬레이션이 요구되는 프로그램을 이 명령어에 적절한 인자로 실행하는 것만으로
구동할 수 있다\[1\].

이 에뮬레이터에서 볼만한 옵션은 -L과 -g이다. 먼저 -L은 ELF 인터프리터 접두어 (ELF interpreter
prefix)를 설정한다. 즉, ELF 인터프리터 기능을 사용할 때 필요한 파일 (e.g., libc.so.6)을 검색할
디렉토리를 설정하는 것이다. 예를 들어, -L /usr/arm-linux-gnueabi/로 설정하면 이 디렉토리
내에서 libc.so.6 등을 찾는다. 다음으로 -g는 QEMU가 1234 포트로 GDB 연결을 기다리도록 만든다. 이를 이용하여
프로그램에 대한 디버깅을 할 수 있다\[1\].

그럼 qemu-user-static으로 ARM용으로 컴파일된 프로그램을 실행시키는 방법을 알아보자. 물론 첫 번째는 관련
프로그램을 설치하는 것이다. 단, 아래의 명령어는 우분투 기준임에 유의하라\[3\].

``` text
$ sudo apt-get install qemu-system -y
$ sudo apt-get install qemu-user-static -y
```

위와 같이 설치했다면 ARM ELF 인터프리터 기능을 사용하기 위한 파일을 받아야 한다. 이는 다음과 같은 명령어 중 하나로 할
수 있고, 여기서는 gcc-arm-linux-gnueabi를 설치했다고 가정한다. 단, 아래의 명령어는 우분투 기준임에
유의하라\[4, 5, 6\].

``` text
$ sudo apt install gcc-arm-linux-gnueabi -y
$ sudo apt install gcc-arm-none-eabi -y
$ sudo apt install gcc-arm-linux-gnueabihf -y
$ sudo apt install gcc-aarch64-linux-gnu -y
```

이것으로 우리는 다음을 설치했다.

  - 프로그램 구동을 위한 에뮬레이터
      - /usr/bin/qemu-\* (qemu-arm-static)
  - 프로그램에서 필요로 하는 ELF 인터프리터 기능
      - /usr/arm-linux-gnueabi/\*

이제 프로그램을 구동해보자. 이는 간단히 다음 명령어를 실행하여 할 수 있다. 단, 프로그램 이름은 hello라고 가정한다.

``` text
$ qemu-arm-static -L /usr/arm-linux-gnueabi/ ./hello
```

## GDB Multi-architecture

GDB는 x86-64에서 ARM 프로그램을 구동하는 것과 같은 상황에서도 디버깅할 수 있도록 gdb-multiarch를 제공한다.
여기서는 GDB 확장 중 하나인 GEF를 사용하고 있다는 가정 하에 설명할 것이다\[7\].

앞서 언급한 gdb-multiarch를 이용하여 ARM 프로그램을 디버깅할 때는 아키텍처를 ARM으로 설정해주어야 한다. 이는
다음과 같이 할 수 있다\[8\].

``` text
$ gdb-multiarch ./hello
gef➤  set architecture arm
```

그리고 상기에 설명하였듯이, qemu-user-static 에뮬레이터를 사용한 디버깅에서는 -g 옵션을 사용하여 원격 디버깅을
해야 하므로 gef-remote 명령어를 사용해야 한다. 그런데 현 상황과 같은 경우에는 gef-remote 명령어의
–qemu-user와 –qemu-binary 옵션을 사용해야 한다. 이때 후자는 타겟 프로그램명을 인자로 전달해주어야 한다.
아래 예시를 보면 보다 명확히 알 수 있을 것이다\[9\].

``` text
gef➤  gef-remote --qemu-user --qemu-binary ./hello localhost 1234
```

지금까지 설명한 것으로 ARM 프로그램을 디버깅하는 절차를 정리하면 다음과 같다.

``` text
[Target ARM Program]

$ qemu-arm-static -L /usr/arm-linux-gnueabi/ -g 1234 ./hello
```

``` text
[GDB]

$ gdb-multiarch ./hello
gef➤ set architecture arm
gef➤  gef-remote --qemu-user --qemu-binary ./hello localhost 1234
```

## References

1.  "qemu-user-static - QEMU User Emulator (static version)," ubuntu
    manuals. \[Online\]. Available:
    <https://manpages.ubuntu.com/manpages/bionic/man1/qemu-user-static.1.html>
2.  "RUNNING ARM BINARIES ON X86 WITH QEMU-USER," AZERIA. \[Online\].
    Available: <https://azeria-labs.com/arm-on-x86-qemu-user/>
3.  "Download QEMU," QEMU. \[Online\]. Available:
    <https://www.qemu.org/download/>
4.  "Cross-compiler," ARM Developer Hub. \[Online\]. Available:
    <https://learn.arm.com/install-guides/gcc/cross/>
5.  "GNU C compiler for the armel architecture," Canonical Ltd.
    \[Online\]. Available:
    <https://launchpad.net/ubuntu/noble/+package/gcc-arm-linux-gnueabi>
6.  Tharanga, Jun. 2012, "what is arm-linux-gcc and how to install this
    in ubuntu," STACK OVERFLOW. \[Online\]. Available:
    <https://stackoverflow.com/questions/11118051/what-is-arm-linux-gcc-and-how-to-install-this-in-ubuntu>
7.  "GNU Debugger (with support for multiple architectures)," Canonical
    Ltd. \[Online\]. Available:
    <https://launchpad.net/ubuntu/focal/+package/gdb-multiarch>
8.  "Command gef-remote," GEF-Legacy - GDB Enhanced Features (for
    GDB-Python2) documentation. \[Online\]. Available:
    <https://gef-legacy.readthedocs.io/en/latest/commands/gef-remote/>
9.  "gef-remote," GDB Enhanced Features documentation. \[Online\].
    Available: <https://hugsy.github.io/gef/commands/gef-remote/>
