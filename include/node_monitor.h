// #define _MAC
#define _LINUX

#pragma once

#include <cmath>
#include <string>
#include <map>
#include <iostream>
#include <fstream>
#include <sstream>
#include <msgpack/rpc/client.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <ifaddrs.h>

#ifdef _LINUX
  #include <sys/types.h>
  #include <sys/sysctl.h>
  #include <sys/sysinfo.h> 
#endif

#ifdef _MAC
  #include <mach/vm_statistics.h>
  #include <mach/mach_types.h>
  #include <mach/mach_init.h>
  #include <mach/mach_host.h>
  #include <mach/mach_error.h>
#endif

#include "hypervisor_detector.h"

using namespace std;

class NodeMonitor
{
public:
  NodeMonitor(int mainLoopDelaySecs = 2);

  ~NodeMonitor();
/*
 * API methods
 */
  void ReadSettingsFile(string setF = "vosdemon.txt");

  void Loop(bool infinite = false, long iterations = 10);

  map<string,double> GetMetrics();

  void SendMetrics();

  string RequestID();

  int IsHypervisorRegistered();

  int RegisterHypervisor();
private:
  string settingFile;

  map<string, string> settingsMap;
  map<string, double> metricsMap;

  msgpack::rpc::client* client;

  int loopWaitSecs;

#ifdef _MAC
  unsigned long previousTotalTicks;
  unsigned long previousIdleTicks;
#endif

#ifdef _LINUX
  long long lastTotalUser;
  long long lastTotalUserLow;
  long long lastTotalSys;
  long long lastTotalIdle;
#endif

private:
  void StartClient();

  void AddSettingToMap(string key, string value);

  void AddSettingToFile(string key, string value);

  bool HasID();

  //Total CPU usage in %
  double GetCPUUsage();

  //Free RAM in bytes
  double GetFreeRAM();

  string GetIPAddress();

// auxiliary methods
private:
  template<class T> std::string to_string(const T& t);

  std::vector<string> inline StringSplit(const string &source, const char *delimiter = " ", bool keepEmpty = false);

#ifdef _MAC
  double CalculateCPULoadMac(unsigned long idleTicks, unsigned long totalTicks);
#endif

#ifdef _LINUX
  void ReadProcStat();
#endif
};
