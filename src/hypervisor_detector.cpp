#include "../include/hypervisor_detector.h"

HypervisorDetector::HypervisorDetector()
{
}

int HypervisorDetector::Connect(string hyperVURI)
{
  localConn = virConnectOpen(hyperVURI.c_str());

  if(localConn == NULL) 
  {
    cout<<"failed to connect to qemu:///system"<<endl;
    return -1;
  }
  nodeinfo = GetNodeInfo();
  return 0;
}

void HypervisorDetector::Disconnect()
{
  virConnectClose(localConn);
}

string HypervisorDetector::Model()
{
  virErrorPtr error;
  const char* type;

  type = virConnectGetType(localConn);
  if (type == NULL) {
    error = virSaveLastError();
    fprintf(stderr, "virConnectGetType failed: %s\n", error->message);
    virFreeError(error);
  }
  string s_type(type);

  return s_type;
}

virNodeInfo HypervisorDetector::GetNodeInfo()
{
  virErrorPtr error;
  virNodeInfo tmpNodeInfo;

  if (virNodeGetInfo(localConn, &tmpNodeInfo) < 0) {
    error = virSaveLastError();
    fprintf(stderr, "virNodeGetInfo failed: %s\n", error->message);
    virFreeError(error);
  }
  return tmpNodeInfo;
}


