// #define _MAC
#define _LINUX

#include <string>
#include <iostream>
#include <libvirt/libvirt.h>
#include <libvirt/virterror.h>
#include <msgpack/rpc/client.h>

using namespace std;

class HypervisorDetector {
public:
  HypervisorDetector();

private:
  virConnectPtr localConn;
  char *hostname;
  char *caps;
  int vcpus;
  virNodeInfo nodeinfo;
  int numnodes;
  const char* type;
  unsigned long virtVersion;
  unsigned long libvirtVersion;
  char *uri;
  int is_encrypted;
  int is_secured;
  virSecurityModel secmodel;

public:
  int Connect(string hyperVURI = "qemu:///system");
  void Disconnect();

  string HypervisorType();
  string Model(); 
  long long Memory() { return nodeinfo.memory; }
  int CPUs() { return nodeinfo.cpus; }
  int MHz() { return nodeinfo.mhz; }
  int NUMANodes() { return nodeinfo.nodes; }
  int CPUSockets() { return nodeinfo.sockets; }
  int CoresPerSocket() { return nodeinfo.cores; }
  int ThreadsPerCore() { return nodeinfo.threads; }

private:
  virNodeInfo GetNodeInfo();
};
