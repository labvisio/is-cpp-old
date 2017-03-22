is
===================

Easy to use IoT messaging middleware C++ implementation. 

Installing Dependencies
-----------------
On linux run the **install** script to install all the dependencies. 
**(!Tested only on Ubuntu 14.04 and 16.04!)**.

```shell
cd scripts
chmod 755 install
./install
```

Using the library
-----------------
Everything is header only, just include the headers in your project, for instance if
you copy the files to **/usr/local/include/is** (this is done automatically by the install script).

```c++
#include <is/is.hpp>
// Everything will be under the is namespace
```
Will include all the necessary files to use the library.

The messaging layer is implemented using the the 
[amqp 0.9.1](https://www.rabbitmq.com/specification.html) protocol, 
requiring a broker to work. We recommend using [RabbitMQ](https://www.rabbitmq.com/).

The broker can be easily instantiated with [Docker](https://www.docker.com/) with the following command:
```c++
docker run -d -m 512M -p 15672:15672 -p 5672:5672 picoreti/rabbitmq:latest
```
To install docker run: 
```shell
curl -sSL https://get.docker.com/ | sh
```

Messages and Serialization
----------
A number of standard messages are provided. (See the **msgs** folder)
New messages can be defined using the **IS_DEFINE_MSG** macro.

```c++
struct Entity {
  std::string id;
  std::string type;
  std::vector<std::string> services;
    
  IS_DEFINE_MSG(id, type, services);
};
```  
Messages can be serialized/deserialized according to the 
[MessagePack](http://msgpack.org/) binary format by using the 
**is::msgpack** helper.

The message defined above with the following values:  
```c++
  Entity entity { "1", "camera", { "set_fps", "set_delay" } }; 
```
will be equivalent to the following JSON object but on binary form. 

```javascript
["1", "camera", ["set_fps", "set_delay"]] 
// Binary Form: 93 a1 31 a6 63 61 6d 65 72 61 92 a7 73 65 74 5f 66 70 73 a9 73 65 74 5f 64 65 6c 61 79 
```

Publish/Subscribe Pattern Example
------------------

```c++
#include <is/is.hpp>

int main(int argc, char* argv[]) {
  /* 
      Creates a connection to the broker running on localhost at port 5672, 
    with credentials guest:guest (username:password respectively).
  */
  auto is = is::connect("amqp://guest:guest@localhost:5672");
  
  /*
      Create a subscription to the topic "device.temperature". 
    A tag representing the subscription is returned. The tag can later
    be used to consume messages.   
  */
  auto tag = is.subscribe("device.temperature");

  /*
      Publishes a message to the "device.temperature" topic. The messages 
    is a simple float value and its being serialized according to the msgpack 
    specification.   
  */
  is.publish("device.temperature", is::msgpack(33.7f));
  
  /*
      Consume a message from the "device.temperature" subscription
    and deserialize it.
  */
  auto message = is.consume(tag);
  auto temperature = is::msgpack<float>(message);
  
  assert(temperature == 33.7f);
}
```

Request/Reply Pattern Example
------------------

Service provider example:

```c++
#include <is/is.hpp>

is::Reply increment(is::Request req) {
  auto value = is::msgpack<int>(req);
  return is::msgpack(value + 1);
}

int main(int argc, char* argv[]) {
  std::string uri = "amqp://guest:guest@localhost:5672";

  /*
      Spawn the service provider with entity name "math" on a different 
    thread where it will create a new connection to the broker and advertise 
    the "increment" service. 
  */
  auto thread = is::advertise(uri, "math", {
    { "increment", increment }
    /* ... other services ... */
  });

  thread.join();
}
```

Service client example:

```c++
#include <is/is.hpp>

using namespace std::chrono_literals;

int main(int argc, char* argv[]) {
  std::string uri = "amqp://guest:guest@localhost:5672";
  
  auto is = is::connect(uri);

  // Creates a service client using the same connection.
  auto client = is::make_client(is);

  /*
      Request the "increment" service from the "math" entity. 
    Requests are asynchronous and can arrive at any order, therefore
    this method returns a request id that can be used to correlate 
    requests with replies.  
  */
  auto req_id = client.request("math.increment", is::msgpack(0));

  /*
      Block waiting for the reply with id="req_id" with 1s timeout. The 
    policy "discard_others" will dicard any other reply received. This 
    will not happen since we only made one request.
  */
  auto reply = client.receive_for(1s, req_id, is::policy::discard_others);

  if (reply == nullptr) {
    is::log::error("Request {} timeout!", req_id);
  } else {
    is::log::info("Reply: {}", is::msgpack<int>(reply));
  }
}
```

Pipeline Pattern Example
------------------

The request/reply pattern can be easily composed in other to form a pipeline. 
For instance, the **increment** service example can be modified using the 
pipeline pattern to increment the value twice before sending the reply 
to the client by just changing the following line.

```c++
client.request("math.increment", is::msgpack(0));
```

to 

```c++
client.request("math.increment;math.increment", is::msgpack(0));
```