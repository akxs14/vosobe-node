#include "../include/node_monitor.h"

int main(void)
{
  NodeMonitor* nm = new NodeMonitor();
  nm->Loop();
  return 0;
}
