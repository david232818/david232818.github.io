---
title: Dive into SOME/IP
tags: background
toc: true
---


## Introduction
Scalable service-Oriented MiddlewarE over IP (SOME/IP)는 remote procedure calls (RPC), event notifications를 지원하는 automotive (차량용)/embedded communication protocol과 그 기반에 존재하는 serialization/wire format이다. 즉, 프로토콜과 데이터 표현 및 전송 형식을 모두 포함한다. SOME/IP는 기존의 RPC 메커니즘과는 다른, 임베디드 시스템이나 차량 등에 사용되는 시스템의 요구사항을 충족시키기 위해 제안되었다[1].


비록 SOME/IP가 데이터 표현 및 전송 형식을 포함하지만, 여기서는 프로토콜 부분만을 다룰 것이다. 그리고 본 글에서와 같이 프로토콜을 사용하기 위해 분석할 때 가장 먼저 봐야 하는 것은 다음 두 가지이다:

 - Addressing method
 - Error Handling


따라서 본 글에서는 AUTOSAR의 SOME/IP Protocol Specification [1]을 기반으로 SOME/IP에서의 addressing method와 error handling을 알아볼 것이다. 그리고 Server - Client 예제를 살펴볼 것이다.

## SOME/IP Overview
SOME/IP는 임베디드 시스템이나 차량 등에 사용되는 프로토콜이며, 주로 TCP와 UDP 위에서 동작한다. 그리고 이를 SOME/IP의 TCP 또는 UDP binding이라고 표현한다. 이를 그림으로 표현하면 다음과 같다[1]:

```
[      SOME/IP    ]
[        TCP      ]
[         IP      ]
[ Data link layer ]
[  Physical layer ]
```

### Resource identification
SOME/IP는 기존의 RPC 메커니즘을 임베디드 시스템, 차량 등에 맞게 구성한 것이므로 원격으로 호출할 프로시저를 식별하기 위한 것을 주소라고 볼 수 있다. 또한, SOME/IP는 event notification을 지원하므로 이것을 식별하는 것도 주소의 목표에 포함될 수 있을 것이다.


SOME/IP에서 식별해야 하는 경우의 수와 각각을 식별하기 위해 제공하는 것은 다음과 같이 나타낼 수 있다[1]:

 - Message ID: The RPC call to a method of an application or to identify an event
 - Request ID: Multiple parallel uses of the same method
 - Request ID: Multiple Service-Instances reside on different ECUs
 - Instance ID: Multiple Service-Instances of one or more Services reside on one single ECU

그럼 이들을 하나씩 알아보겠다.


*Message ID*는 SOME/IP header에 포함되며, Service ID와 Method ID로 구성된다. 이는 32 비트 값이어야만 하고, 전체 시스템 (e.g., the vehicle)에서 유일해야 한 것으로 가정된다. 이때 Service ID는 2^16개의 services를 구분하고, Method ID는 2^16개의 service elements (methods) 또는 events를 구분한다. 여기서 Method ID는 구분하는 종류가 두 개이기 때문에 service elements와 events 필드를 나누는 것이 일반적이다[1]. 

```
Message ID:
    +-----< Service ID >-----+----< Method ID >-------+
    |       Service ID       | methods   |   events   |
    +------------------------+------------------------+
      2 ^ 16 services          2 ^ 16 service elements or events
```

SOME/IP에서 event는 적어도 하나의 eventgroup에 속해 있어야만 하고, eventgroup도 적어도 하나의 event를 포함해야 한다. 여기서 eventgroup이란 service 내에서 subscription을 허용하기 위한 events와 notification events의 logical grouping이다[1].

```
+--------< Eventgroup >--------+
|  event1, event2, ...         |
+------------------------------+
```


*Request ID*는 SOME/IP header에 포함되며, server와 client가 같은 method를 병렬적으로 사용할 수 있도록 만든다. Request ID는 request-response 쌍마다 유일한 값이어야만 한다. 그리고 Request ID가 사용되는 상황에서 응답 메시지를 전송할 때 송신자는 응답 메시지에 Request ID를 복사해야만 한다[1].

```
    +-------------[ Request ID 1 ]----[ Request sender ]
    |
    V                
[ Provider ] <----[ Request ID 2 ]----[ Request sender ]
    ^
    |
    +-------------[ Request ID 3 ]----[ Request sender ]

```


*Instance ID*는 같은 Service에서의 Service-Instances를 식별하지만, 헤더에 포함되어 있지 않다. 이때 서로 다른 Service의 몇몇 Service-Instaces가 TCP 계층의 포트를 공유하는 경우에는, 한 ECU에 있는 같은 서비스의 여러 Service-Instances는 다른 포트를 사용해야만 한다[1].

### Error handling
SOME/IP에서 오류 처리는 application 또는 communication layer에 의해 수행될 수 있다. SOME/IP는 오류 처리를 위한 다음과 같은 두 가지 메커니즘을 지원한다[1]:

 - Return Codes in the Response Messages of methods
 - Explicit Error Messages


*Return Code*는 응답 메시지에서 transport application 오류를 전송하거나 해당 method의 응답 데이터를 전송하는 데에만 사용되어야 한다. 그리고 유념해야 할 것은 SOME/IP에서 이것이 "오류"로 처리되지는 않는다는 것이다. 즉, 이들의 메시지 타입은 여전히 "응답"이다. Return Code는 오직 응답 메시지 (메시지 타입이 "응답", "오류")에서만 사용 가능하며, 다른 메시지 타입의 메시지는 이 필드를 0x00으로 설정해야만 한다[1].


*Explicit Error Messages*는 transport application 오류를 전송하거나, 응답 데이터 또는 generic한 SOME/IP 오류를 전송할 때에만 사용되어야 한다. 만약 보다 세부적인 오류 사항을 전송해야 한다면, Error Message (메시지 타입이 "오류")의 payload를 채워서 전송하면 된다. 이는 서버에서 발생할 수 있는 application별 오류를 처리하는데 사용될 수 있다. 그리고 스위치와 같이 통신의 중간 단계에 존재하는 장비에서 발생하는 오류를 처리하는데에도 사용될 수 있다[1].

## SOME/IP Example: Server - Client
이제 SOME/IP 관련 예제를 살펴보겠다. 여기서는 SOME/IP 구현 중 하나인 vsomeip [2]를 사용할 것이다. 이 라이브러리를 설치하는 방법은 vsomeip [2]의 README 파일을 참고하라.


예제 코드를 살펴보기 이전에 vsomeip의 코딩 스타일에 대해 언급하고 진행하겠다. Vsomeip에는 `vsomeip::application`가 존재하고 이를 초기화한 후에 event별 callback을 등록하는 방식으로 application이 구현된다. 이때 client는 서버에 요청할 method와 서버로부터 받은 응답을 처리하는 callback을 작성하게 되고, server는 제공할 서비스에 대한 주소를 할당한 후에 client에서와 입장만 바뀐 callback 함수를 작성하게 된다.

```
Client:
    <vsomeip::application> init
    register callbacks
    run request / response thread
    
Server:
    <vsomeip::application> init
    allocate service ids
    register callbacks
    offer service
```


그럼 이제 예제 코드를 살펴보겠다. 여기서 살펴볼 예제 코드는 COVESA가 작성한 vsomeip in 10 minutes [3]에 나오는 예제 코드이다. 먼저 client부터 살펴보면 다음과 같다:

```C++
#include <iomanip>
#include <iostream>
#include <sstream>

#include <condition_variable>
#include <thread>

#include <vsomeip/vsomeip.hpp>

#define SAMPLE_SERVICE_ID 0x1234
#define SAMPLE_INSTANCE_ID 0x5678
#define SAMPLE_METHOD_ID 0x0421

std::shared_ptr< vsomeip::application > app;
std::mutex mutex;
std::condition_variable condition;

void run() {
  std::unique_lock<std::mutex> its_lock(mutex);
  condition.wait(its_lock);

  std::shared_ptr< vsomeip::message > request;
  request = vsomeip::runtime::get()->create_request();
  request->set_service(SAMPLE_SERVICE_ID);
  request->set_instance(SAMPLE_INSTANCE_ID);
  request->set_method(SAMPLE_METHOD_ID);

  std::shared_ptr< vsomeip::payload > its_payload = vsomeip::runtime::get()->create_payload();
  std::vector< vsomeip::byte_t > its_payload_data;
  for (vsomeip::byte_t i=0; i<10; i++) {
      its_payload_data.push_back(i % 256);
  }
  its_payload->set_data(its_payload_data);
  request->set_payload(its_payload);
  app->send(request);
}

void on_message(const std::shared_ptr<vsomeip::message> &_response) {

  std::shared_ptr<vsomeip::payload> its_payload = _response->get_payload();
  vsomeip::length_t l = its_payload->get_length();

  // Get payload
  std::stringstream ss;
  for (vsomeip::length_t i=0; i<l; i++) {
     ss << std::setw(2) << std::setfill('0') << std::hex
        << (int)*(its_payload->get_data()+i) << " ";
  }

  std::cout << "CLIENT: Received message with Client/Session ["
      << std::setw(4) << std::setfill('0') << std::hex << _response->get_client() << "/"
      << std::setw(4) << std::setfill('0') << std::hex << _response->get_session() << "] "
      << ss.str() << std::endl;
}

void on_availability(vsomeip::service_t _service, vsomeip::instance_t _instance, bool _is_available) {
    std::cout << "CLIENT: Service ["
            << std::setw(4) << std::setfill('0') << std::hex << _service << "." << _instance
            << "] is "
            << (_is_available ? "available." : "NOT available.")
            << std::endl;
    condition.notify_one();
}

int main() {

    // Generate vsomeip::application and init
    app = vsomeip::runtime::get()->create_application("Hello");
    app->init();
    
    // Register callback: availability handler
    app->register_availability_handler(SAMPLE_SERVICE_ID, SAMPLE_INSTANCE_ID, on_availability);
    app->request_service(SAMPLE_SERVICE_ID, SAMPLE_INSTANCE_ID);
    
    // Register callback: message handler
    app->register_message_handler(SAMPLE_SERVICE_ID, SAMPLE_INSTANCE_ID, SAMPLE_METHOD_ID, on_message);
    
    std::thread sender(run);
    app->start();
}
```

위 코드에서 `on_availability()`, `on_message()`는 여기서 관심을 갖는 부분이 아니다. 여기서 첫 번째로 관심을 갖는 부분은 `main()`이다. `main()`을 살펴보면 다음과 같은 순서로 동작함을 알 수 있다:

```
1. Generate `vsomeip::application` and initialize
2. Register callback
3. run sender thread
```

여기서 마지막 부분 (`std::thread sender(run);`)이 payload를 구성하고, server에게 request를 전송한다. 그럼 server는 client가 호출을 요청한 프로시저를 식별하기 위해 client 측에서 전송한 Service ID, Instance ID, Method ID를 확인할 것이다. 그리고 이것으로 프로시저가 식별된다면, 해당 프로시저를 호출하고 client에게 응답 메시지를 전송할 것이다. 그럼 server 코드를 살펴보겠다.

```C++
#include <iomanip>
#include <iostream>
#include <sstream>

#include <vsomeip/vsomeip.hpp>

#define SAMPLE_SERVICE_ID 0x1234
#define SAMPLE_INSTANCE_ID 0x5678
#define SAMPLE_METHOD_ID 0x0421

std::shared_ptr<vsomeip::application> app;

void on_message(const std::shared_ptr<vsomeip::message> &_request) {

    std::shared_ptr<vsomeip::payload> its_payload = _request->get_payload();
    vsomeip::length_t l = its_payload->get_length();

    // Get payload
    std::stringstream ss;
    for (vsomeip::length_t i=0; i<l; i++) {
       ss << std::setw(2) << std::setfill('0') << std::hex
          << (int)*(its_payload->get_data()+i) << " ";
    }

    std::cout << "SERVICE: Received message with Client/Session ["
        << std::setw(4) << std::setfill('0') << std::hex << _request->get_client() << "/"
        << std::setw(4) << std::setfill('0') << std::hex << _request->get_session() << "] "
        << ss.str() << std::endl;

    // Create response
    std::shared_ptr<vsomeip::message> its_response = vsomeip::runtime::get()->create_response(_request);
    its_payload = vsomeip::runtime::get()->create_payload();
    std::vector<vsomeip::byte_t> its_payload_data;
    for (int i=9; i>=0; i--) {
        its_payload_data.push_back(i % 256);
    }
    its_payload->set_data(its_payload_data);
    its_response->set_payload(its_payload);
    app->send(its_response);
}

int main() {

   // Generate vsomeip::application and initialize
   app = vsomeip::runtime::get()->create_application("World");
   app->init();
   
   // Allocate service id and register callback: message handler
   app->register_message_handler(SAMPLE_SERVICE_ID, SAMPLE_INSTANCE_ID, SAMPLE_METHOD_ID, on_message);
   
   app->offer_service(SAMPLE_SERVICE_ID, SAMPLE_INSTANCE_ID);
   app->start();
}
```

앞서 client 코드를 분석했을 때와 마찬가지로, server 코드의 `main()`을 보면 다음과 같은 순서로 동작함을 알 수 있다:

```
1. Generate vsomeip::application and initialize
2. Allocate service id and register callback
3. offer service
```

만약 client가 할당된 service ids로 request를 전송한다면 `on_message()`가 호출되어 client에게 응답 메시지를 전송할 것이다.

## Conclusion
지금까지 SOME/IP에서 원격으로 호출할 프로시저를 어떻게 식별하는지와 오류 처리를 살펴보았고, vsomeip 구현을 사용하여 그 코딩 스타일을 살펴보았다. 그리고 프로토콜이 식별하고자 하는 것의 주소 표현 방식을 중점적으로 살펴보았을 때 코드의 흐름을 더 쉽게 유추할 수 있을 것이다.

## References
1. "SOME/IP Protocol Specification," AUTOSAR, 2022.
2. COVESA, "vsomeip," 2024. [Online]. Available: https://github.com/COVESA/vsomeip, [Accessed Feb. 04, 2024].
3. COVESA, "vsomeip in 10 minutes," 2024. [Online]. Available: https://github.com/COVESA/vsomeip/wiki/vsomeip-in-10-minutes, [Accessed Feb. 04, 2024].
