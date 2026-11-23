---
title: "Double Linked List For Generic Data"
tags: background
toc: true
---


## Introduction

Allen Newell과 J. C. Shaw는 문제의 증명을 기호 논리를 통해 찾는 프로그램인 Logical Theorist (LT)를 개발하는 과정에서 연결 리스트를 제안하였다. LT를 구현하기 위해서는 표현식이나 논리 문제를 해결하는데 사용한 프로세스의 리스트를 다룰 수 있어야 했고, 이를 위한 요구사항을 해결하기 위해 개발한 언어인 Information Processing Language (IPL)이 리스트를 표현한 방식이 연결 리스트 (Linked list)였던 것이다. 이때 그 요구사항을 간략히 정리하면 다음과 같다[1].

-   Flexibility of Memory Management
    1.  No restrictions on the number of different lists of items of information
    2.  No restrictions on the nature of the items in a list
    3.  Possible to add, delete, insert and rearrange items of information in a list
-   Flexibility in the Specification of Processes
    1.  No limitation on the size and complexity of hierarchies of definitions
    2.  No restriction on the number of references in the instructions or on what is referenced
    3.  Possible to define processes implicitly (e.g., by recursion)
    4.  Unnecessary to have single integrated plan or set of conventions for the form of informations

상기의 요구사항을 읽어보면 Flexibility of Memory Mangement가 제네릭 데이터를 다루기 위한 연결 리스트와 연관이 있음을 알 수 있다.

이렇게 Allen Newell과 J. C. Shaw이 개발한 IPL은 다음과 같이 연결 리스트를 표현하였다[1].

```
    +--------+    +--------+    +--------+
--->|Location|--->|Location|--->|Location|---> (-1)
    +--------+    +--------+    +--------+
        |              |             |
        V              V             V
      element       element       element
```

위 그림에서 각 Location은 다음 Location을 가리키며, 리스트의 끝은 -1로 표현한다. 이러한 리스트는 Location만으로 순회가 가능하며, Location을 통해 element에 접근할 수 있다. 이는 다음과 같은 노드 구조와는 다른 것으로 보인다[1].

```C
struct node {
    void *element;

    struct node *next;
};
```

그러나 IPL에서 표현한 연결 리스트는 현대에도 게임이나 커널 분야에서 사용되고 있으며, 흔히 intrusive linked list라고 불린다. 본 글에서는 이러한 방식의 연결 리스트에 대한 사용예를 살펴보고, 간단한 구현 예시를 제공할 것이다.


## Intrusive Linked List

Intrusive Linked List는 연결 리스트에서 한 노드와 그 다음 노드 간 연결을 위한 타입이 존재하여 이 타입이 임의의 다른 타입에 멤버 변수로 속해있는 경우를 의미한다. 이를 C 언어로 표현하면 다음과 같다[2].

```C
struct element {
    /* ... */

    struct node list;
};

struct node {
    struct node *next;
};
```

위 코드와 그 앞에서의 C 코드 (Node structure of Non-intrusive linked list)를 비교해보면 어떤 차이가 있는지 알 수 있을 것이다.

상기의 코드와 같은 구현 방식은 제네릭 데이터를 linked list로 다루고자 할 때 메모리 할당 횟수를 감소시킨다는 장점이 있다. Non-intrusive linked list의 경우에는 노드 구조체와 데이터 구조체를 할당해야 하므로 두 번의 메모리 할당이 필요하다. 반면 intrusive linked list는 데이터 구조체만 할당하면 된다. 노드 구조체는 이미 데이터 구조체 내에 포함되어 있기 때문이다. 이러한 할당 횟수의 감소는 에러 처리 횟수의 감소를 의미하는 것이기도 하다.

다만, 현대에 intrusive linked list를 구현할 때 circular하게 구현하는 경우가 있다. 이는 NULL check가 필요없도록 만들어 branch prediction miss로 인한 성능 저하를 감소시킬 수 있다. 이는 Doom 3와 리눅스 커널의 코드를 보면 보다 명확히 보일 것이다.


### Doom 3's list structure

Doom 3는 C++로 작성되었지만 intrusive linked list 구현부를 이해하기에 큰 어려움은 없다. 여기서는 gpfault에서 작성한 글 [2]을 기반으로 하여 살펴볼 것이다.

Doom 3는 다음과 같이 리스트 타입을 구현한다[3].

```C++
// neo/idlib/containers/LinkList.h

template< class type >
class idLinkList {
public:
                                                idLinkList();
                                                ~idLinkList();

        bool				IsListEmpty( void ) const;
        bool				InList( void ) const;
        int					Num( void ) const;
        void				Clear( void );

        void				InsertBefore( idLinkList &node );
        void				InsertAfter( idLinkList &node );
        void				AddToEnd( idLinkList &node );
        void				AddToFront( idLinkList &node );

        void				Remove( void );

        type *				Next( void ) const;
        type *				Prev( void ) const;

        type *				Owner( void ) const;
        void				SetOwner( type *object );

        idLinkList *		ListHead( void ) const;
        idLinkList *		NextNode( void ) const;
        idLinkList *		PrevNode( void ) const;

private:
        idLinkList *		head;
        idLinkList *		next;
        idLinkList *		prev;
        type *				owner;
};
```

위 코드에서 눈여겨 봐야하는 것은 `private` 부분이다. 그럼 각 노드는 이전 노드와 다음 노드에 대한 정보 뿐만 아니라 head 노드와 그 노드를 가진 데이터에 대한 정보를 가지고 있음을 알 수 있다. 이때 head 노드는 리스트에 대한 handle의 의미를 가진다. 다음 코드를 읽어보자[3].

```C++
// neo/idlib/containers/LinkList.h


/*
================
idLinkList<type>::idLinkList

Node is initialized to be the head of an empty list
================
*/
template< class type >
idLinkList<type>::idLinkList() {
        owner	= NULL;
        head	= this;
        next	= this;
        prev	= this;
}
```

위 코드를 읽어보면 head 노드는 `owner` 없이 리스트를 핸들링하기 위한 노드임을 알 수 있다. 이를 그림으로 나타내면 다음과 같다.

```

              +---------+
     +--------| head    |-------------+
     |  +---->|         |<---------+  |
     |  |     +---------+          |  |
     |  |                          |  |
     V  |                          |  V
+---------+                     +---------+
| node2   |<--------------------|  node1  |
|         |-------------------->|         |
+---------+                     +---------+

```

상기의 그림을 살펴보면 NULL을 가리키는 노드가 없음을 알 수 있다. 즉, NULL check를 할 필요가 없는 것이다. 아래 코드를 보자[3].

```C++
// neo/idlib/containers/LinkList.h

/*
================
idLinkList<type>::Remove

Removes node from list
================
*/
template< class type >
void idLinkList<type>::Remove( void ) {
        prev->next = next;
        next->prev = prev;

        next = this;
        prev = this;
        head = this;
}

// ...

/*
================
idLinkList<type>::InsertAfter

Places the node after the existing node in the list.  If the existing node is the head,
then the new node is placed at the beginning of the list.
================
*/
template< class type >
void idLinkList<type>::InsertAfter( idLinkList &node ) {
        Remove();

        prev		= &node;
        next		= node.next;
        node.next	= this;
        next->prev	= this;
        head		= node.head;
}
```

위 코드를 보면 포인터 값을 바꾸는 것만으로 node insertion을 수행하는 것을 볼 수 있다. 이렇게 NULL check가 필요 없도록 만듦으로써 branch prediction miss로 인한 성능 저하를 감소시킬 수 있는 것이다.


### Linux kernel's list structure

리눅스 커널 코드를 읽다보면 list_head 구조체를 많이 접하게 된다. 이는 리눅스 커널에서 제공하는 연결 리스트에 관한 구조체이며, 그 구조는 앞서 살펴본 Doom 3의 것과 유사하다. 여기서는 리눅스 커널 5.15.30 버전을 기반으로 살펴볼 것이다.

리눅스 커널은 다음과 같은 노드 구조체를 제공한다[4].

```C
/* include/linux/types.h */

struct list_head {
    struct list_head *next, *prev;
};
```

위 코드를 통해 Doom 3의 노드가 가지고 있는 정보에 비해 적은 정보를 가지고 있음을 알 수 있다. 리눅스 커널은 head 노드는 별도로 관리하고, owner는 container_of 매크로를 이용하여 얻는다. 그래서 next와 prev만으로도 리스트를 핸들링할 수 있는 것이다.

리눅스 커널은 노드 삽입을 일반화한 함수를 정의하고 이를 이용하여 list_add_tail과 같은 함수를 구현한다[4].

```C
/* include/linux/list.h */


/*
  * Insert a new entry between two known consecutive entries.
  *
  * This is only for internal list manipulation where we know
  * the prev/next entries already!
  */
static inline void __list_add(struct list_head *new,
                              struct list_head *prev,
                              struct list_head *next)
{
        if (!__list_add_valid(new, prev, next))
                return;

        next->prev = new;
        new->next = next;
        new->prev = prev;
        WRITE_ONCE(prev->next, new);
}

```

위 코드를 통해 알 수 있는 것은 모든 노드가 어딘가를 가리키고 있기 때문에 노드 삽입이 두 노드 사이에 한 노드를 삽입하는 것으로 일반화할 수 있다는 것이다. 즉, 리스트의 처음 또는 리스트의 끝에 삽입하는 경우를 생각할 필요가 없다.

노드를 삭제하는 것은 Doom 3와 거의 동일하므로 생략하겠다.


### Implementation example of intrusive linked list

여기서는 상기에 살펴본 것에 대한 구현 예시를 제공한다. 이를 통해 지금까지 다룬 것이 어떻게 동작할지에 대한 감을 잡을 수 있을 것이다.

여기서 구현할 것은 `int` 타입의 key를 데이터로 하는 리스트이다. 이때 삽입 방식은 끝에 붙이거나 오름차순 정렬을 제공한다. 이에 대한 연결 리스트 구현은 다음과 같다.

```C
#ifndef __LIST_NODE_H__
#define __LIST_NODE_H__

#include <stdio.h>
#include <stddef.h>

struct list_node {
    struct list_node *head;	/*
                                 * first node and handle
                                 * for the list
                                 */
    struct list_node *prev;
    struct list_node *next;
    void *obj;			/* object holding this node */
};

inline void list_node_remove(struct list_node *node)
{
    node->next->prev = node->prev;
    node->prev->next = node->next;
    node->head = node;
    node->prev = node;
    node->next = node;
}

void list_node_clear(struct list_node *list,
                     void (*destroy)(void *));

inline void list_node_insert(struct list_node *prev,
                             struct list_node *node,
                             struct list_node *next)
{
    node->next = next;
    node->prev = prev;
    prev->next = node;
    next->prev = node;
    node->head = prev->head;
}

void list_node_add_tail(struct list_node *list,
                        struct list_node *node);

inline void list_node_init(struct list_node *node,
                           void *obj)
{
    node->head = node;
    node->prev = node;
    node->next = node;
    node->obj = obj;
}

#define LIST_HEAD_NODE_INIT(headp)		\
    list_node_init((headp), NULL)

#define LIST_NODE_FOREACH(listp, currp, nextp)		\
    for ((currp) = (listp)->head->next,			\
             (nextp) = (currp)->next;			\
         (currp) != (listp)->head;			\
         (currp) = (nextp), (nextp) = (nextp)->next)


#endif

```

```C
#include <stdio.h>
#include "list_node.h"

extern inline void list_node_remove(struct list_node *node);

void list_node_clear(struct list_node *list,
                     void (*destroy)(void *))
{
    struct list_node *curr, *next;

    LIST_NODE_FOREACH(list, curr, next) {
        list_node_remove(curr);
        destroy(curr->obj);
    }
}

extern inline void list_node_insert(struct list_node *prev,
                                    struct list_node *node,
                                    struct list_node *next);

void list_node_add_tail(struct list_node *list,
                        struct list_node *node)
{
    list_node_insert(list->prev, node, list);
}

extern inline void list_node_init(struct list_node *node,
                                  void *obj);
```

위 구현을 살펴보면 `inline`된 함수들이 있음을 알 수 있다. 함수를 inlining하는 것은 함수 호출 비용을 줄일 수 있다는 장점이 있지만, instruction cache를 소모한다는 단점도 가지고 있다. 다만, 위 코드에서의 `inline`은 컴파일러에 대한 요청일 뿐임을 기억하라. 컴파일러는 요청에 따라 확인할 것이고 inlining이 필요하다면 할 것이다. 이제 key 리스트를 구현한 예제를 읽어보자.

```C
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include "list_node.h"

struct key {
    unsigned int key;
    struct list_node klist;
};

typedef struct list_node keylist_t;

#define KEYLIST_INIT(headp) LIST_HEAD_NODE_INIT(headp)

static struct key *key_init(unsigned int key);
static void key_add_tail(keylist_t *list, struct key *k);
static void key_add_asc(keylist_t *list, struct key *k);
static void key_print(keylist_t *list);
static void key_destroy(void *k);
static void key_remove(keylist_t *list, unsigned int key);
static void keylist_destroy(keylist_t *list);

int main()
{
    struct key *k;
    keylist_t keylist;

    KEYLIST_INIT(&keylist);

    k = key_init(42);
    key_add_tail(&keylist, k);
    key_print(&keylist);

    k = key_init(23);
    key_add_tail(&keylist, k);
    key_print(&keylist);

    k = key_init(32);
    key_add_tail(&keylist, k);
    key_print(&keylist);

    key_remove(&keylist, 24);
    key_print(&keylist);

    k = key_init(255);
    key_add_tail(&keylist, k);
    key_print(&keylist);

    key_remove(&keylist, 32);
    key_print(&keylist);

    k = key_init(12);
    key_add_tail(&keylist, k);
    key_print(&keylist);

    k = key_init(12);
    key_add_tail(&keylist, k);
    key_print(&keylist);


    keylist_destroy(&keylist);
    return 0;
}

static struct key *key_init(unsigned int key)
{
    struct key *k;

    k = malloc(sizeof(*k));
    if (k == NULL) {
        perror("malloc");
        return NULL;
    }

    k->key = key;
    list_node_init(&(k->klist), k);
    return k;
}

static void key_add_tail(keylist_t *list, struct key *k)
{
    list_node_add_tail(list, &(k->klist));
}

static void key_add_asc(keylist_t *list,
                        struct key *k)
{
    struct list_node *curr, *next;
    struct key *kk;

    LIST_NODE_FOREACH(list, curr, next) {
        kk = curr->obj;
        if (kk->key >= k->key) {
            list_node_insert(curr->prev, &(k->klist), curr);
            return ;
        }
    }
    key_add_tail(list, k);
}

static struct key *key_search(keylist_t *list,
                              unsigned int key)
{
    struct list_node *curr, *next;
    struct key *kk;

    LIST_NODE_FOREACH(list, curr, next) {
        kk = curr->obj;
        if (kk->key == key)
            return kk;
    }
    return NULL;
}

static void key_print(keylist_t *list)
{
    size_t cnt;
    struct list_node *curr, *next;
    struct key *k;

    printf("\n\n---< Keys >---\n");
    cnt = 0;
    LIST_NODE_FOREACH(list, curr, next) {
        k = curr->obj;
        printf("key: %d, ", k->key);
        cnt++;
    }
    printf("-- [%ld]\n", cnt);
    printf("--------------\n");
}

static void key_destroy(void *k)
{
    free(k);
}

static void key_remove(keylist_t *list, unsigned int key)
{
    struct key *k;

    k = key_search(list, key);
    if (k) {
        list_node_remove(&(k->klist));
        key_destroy(k);
    }
}

static void keylist_destroy(keylist_t *list)
{
    list_node_clear(list, key_destroy);
}
```

위 코드는 head 노드를 어떻게 다루어야 하는지를 설명한다. 즉, 상기의 그림과 같이 리스트를 핸들링하기 위한 노드라는 것이다.


## Conclusion

정리하면, intrusive linked list는 할당 횟수 (그리고 에러 처리 횟수)를 감소시킬 수 있다는 장점이 있고, circular하게 구성함으로써 NULL check를 없앨 수 있으며 branch prediction miss 감소와 노드 삽입 일반화가 가능하다. 물론, non-intrusive linked list가 쓰이지 않는 것은 아니다. Glibc의 tcache 구현에서는 single linked list를 구현할 때 non-intrusive한 방식으로 구현한다. 중요한 것은 어떤 구현 방식이 현재 상황에 더 적합한지 따져보는 것이다.


## References

1. Allen Newell and J. C. Shaw, "Programming the Logic Theory Machine," Proceedings of the Western Joint Computer Conference, pp. 230 &#x2013; 240, 1957.
2. gpfault, Dec. 2014, "Intrusive Linked List in Doom 3," gpfault.net. [Online]. Available: https://gpfault.net/posts/intrusive-lists-doom3.txt.html
3. Edgars, 2012, "ABOUT," [Online]. Available: https://github.com/Edgarins29/Doom3
4. Torvalds, Mar. 2022, "linux kernel," [Online]. Available: https://github.com/torvalds/linux
