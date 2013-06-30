#include <vector>
#include <string>
#include <iostream>
#include <mp/wavy.h>
#include <msgpack.hpp>
#include <msgpack/rpc/client.h>

using namespace std;
using namespace msgpack::rpc;
 
int main(void)
{ 
  msgpack::rpc::client c("127.0.0.1", 9090);
  int result = c.call("add", 1, 2).get<int>();
  cout << result << endl;   

  return 0;
}
