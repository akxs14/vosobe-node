#include "../include/node_monitor.h"

NodeMonitor::NodeMonitor(int mainLoopDelay)
{
  int registration_res;
  loopWaitSecs = mainLoopDelay;

#ifdef _MAC
  previousTotalTicks  = 0;
  previousIdleTicks   = 0;
#endif

#ifdef _LINUX
  lastTotalUser     = 0;
  lastTotalUserLow  = 0;
  lastTotalSys      = 0;
  lastTotalIdle     = 0;
#endif

  ReadSettingsFile();
  StartClient();

  if(!HasID()) {
    string new_id = RequestID();
    AddSettingToMap("id", new_id);
    AddSettingToFile("id", new_id);
  }    

  // 1 -> registered
  // 0 -> not registered
  if(!IsHypervisorRegistered())
    registration_res = RegisterHypervisor();
}

NodeMonitor::~NodeMonitor()
{
  delete this->client;
}

/*
 *  Node Monitor API methods
 */
void NodeMonitor::Loop(bool infinite, long iterations)
{
  while(iterations > 0) {
    SendMetrics();
    sleep(2);
    if(infinite) iterations--;
  }
}

int NodeMonitor::IsHypervisorRegistered()
{
  msgpack::rpc::future f = this->client->call("is_hypervisor_registered",
                                                        settingsMap["id"]);

  int result = f.get<int>();
  return result;
}

int NodeMonitor::RegisterHypervisor()
{
  HypervisorDetector *hd = new HypervisorDetector();
  int connected = hd->Connect();

  if(!connected)
  {
    msgpack::rpc::future f = this->client->call("register_hypervisor",
                                                hd->Model(),
                                                hd->Memory(),
                                                hd->CPUs(),
                                                hd->MHz(),
                                                hd->NUMANodes(),
                                                hd->CPUSockets(),
                                                hd->CoresPerSocket(),
                                                hd->ThreadsPerCore(),
                                                settingsMap["id"]);

    int result = f.get<int>();
    return result;    
  }
  else
    return -1;
}

map<string, double> NodeMonitor::GetMetrics()
{
  map<string, double> tempMap;
  tempMap["freeRAM"]  = GetFreeRAM();
  tempMap["usageCPU"] = GetCPUUsage();
  return tempMap;
}

void NodeMonitor::SendMetrics()
{
  metricsMap = GetMetrics();
  msgpack::rpc::future f = this->client->call("receive_stats",
                                              settingsMap["id"],
                                              settingsMap["ip_addr"],
                                              metricsMap["usageCPU"],
                                              metricsMap["freeRAM"]);
  int result = f.get<int>();
}
 
string NodeMonitor::RequestID()
{
  msgpack::rpc::future f = this->client->call("get_new_id",
                                              settingsMap["ip_addr"]);
  int new_id = f.get<int>();
  return to_string(new_id);
}

/*
 *  Private methods
 */
void NodeMonitor::StartClient()
{
  this->client = new msgpack::rpc::client(settingsMap["ip"],
    atoi(settingsMap["port"].c_str()));
}

//Checks whether an ID has been loaded from the settings file,
//hence the client has an assigned ID.
bool NodeMonitor::HasID()
{
  return (!(settingsMap.find("id") == settingsMap.end()));
}

void NodeMonitor::ReadSettingsFile(string setF)
{
  settingFile = setF;
  string line;
  vector<string> tokenizedLine;
  ifstream settings;
  settings.open(settingFile.c_str());

  if(settings.is_open())
    while(settings.good()) {
      getline(settings, line);
      tokenizedLine = StringSplit(line);
      AddSettingToMap(tokenizedLine[0], tokenizedLine[1]);
    }
  // AddSettingToMap("ip_addr", "127.0.0.1");
  AddSettingToMap("ip_addr", GetIPAddress());
  settings.close();
}

void NodeMonitor::AddSettingToFile(string key, string value)
{
  ofstream setFile;
  setFile.open(settingFile.c_str(), ios_base::app);
  setFile <<endl<<key<<" "<<value;
  setFile.close();
}

void NodeMonitor::AddSettingToMap(string key, string value)
{
  settingsMap[key] = value;
}

string NodeMonitor::GetIPAddress()
{
  struct ifaddrs * ifAddrStruct=NULL;
  struct ifaddrs * ifa=NULL;
  void * tmpAddrPtr=NULL;
  string ip_addr;

  getifaddrs(&ifAddrStruct);

  for (ifa = ifAddrStruct; ifa != NULL; ifa = ifa->ifa_next) {
    if (ifa ->ifa_addr->sa_family==AF_INET) { // check it is IP4
      // is a valid IP4 Address
      tmpAddrPtr=&((struct sockaddr_in *)ifa->ifa_addr)->sin_addr;
      char addressBuffer[INET_ADDRSTRLEN];
      inet_ntop(AF_INET, tmpAddrPtr, addressBuffer, INET_ADDRSTRLEN);
#ifdef _MAC
      if(!strcmp(ifa->ifa_name,"en0"))
        ip_addr = string(addressBuffer);  
#endif
#ifdef _LINUX
      if(!strcmp(ifa->ifa_name,"wlan0"))
        ip_addr = string(addressBuffer);
#endif
    }
  }
  if (ifAddrStruct!=NULL) 
    freeifaddrs(ifAddrStruct);

  return ip_addr;
}

/*
 *  Internal auxiliary methods
 */
template<class T> std::string NodeMonitor::to_string(const T& t)
{
  std::stringstream ss;
  ss << t;
  return ss.str();
}

vector<string> NodeMonitor::StringSplit(const string &source, const char *delimiter, bool keepEmpty)
{
  vector<string> results;
  size_t prev = 0;
  size_t next = 0;

  while ((next = source.find_first_of(delimiter, prev)) != string::npos) {
    if (keepEmpty || (next - prev != 0))
      results.push_back(source.substr(prev, next - prev));

    prev = next + 1;
  }
  if (prev < source.size())
    results.push_back(source.substr(prev));

  return results;
}

/*
 *  Method implementations for retrieving
 *  CPU Usage, free RAM in Linux systems
 */
 #ifdef _LINUX
double NodeMonitor::GetCPUUsage()
{
  double percent;
  FILE* file;
  unsigned long long totalUser, totalUserLow, totalSys, totalIdle, total;

  file = fopen("/proc/stat", "r");
  fscanf(file, "cpu %Ld %Ld %Ld %Ld", &totalUser, &totalUserLow,
    &totalSys, &totalIdle);
  fclose(file);

  if (totalUser < lastTotalUser || totalUserLow < lastTotalUserLow ||
    totalSys < lastTotalSys || totalIdle < lastTotalIdle){
      //Overflow detection. Just skip this value.
      percent = -1.0;
  }
  else{
    total = (totalUser - lastTotalUser) + (totalUserLow - lastTotalUserLow) +
      (totalSys - lastTotalSys);
    percent = total;
    total += (totalIdle - lastTotalIdle);
    percent /= total;
    percent *= 100;
  }
  lastTotalUser = totalUser;
  lastTotalUserLow = totalUserLow;
  lastTotalSys = totalSys;
  lastTotalIdle = totalIdle;

  return percent;
}

double NodeMonitor::GetFreeRAM()
{
  struct sysinfo memInfo;
  sysinfo (&memInfo);

  long freeMemory = memInfo.freeram;
  freeMemory *= memInfo.mem_unit;

  return freeMemory;
}

void NodeMonitor::ReadProcStat()
{
  FILE* file = fopen("/proc/stat", "r");
  fscanf(file, "cpu %Ld %Ld %Ld %Ld", &lastTotalUser, &lastTotalUserLow,
  &lastTotalSys, &lastTotalIdle);
  fclose(file);
}
#endif

/*
 *  Method implementations for retrieving
 *  CPU Usage and free RAM in MAC OS systems
 */
#ifdef _MAC
double NodeMonitor::GetCPUUsage()
{
  host_cpu_load_info_data_t cpuinfo;
  mach_msg_type_number_t count = HOST_CPU_LOAD_INFO_COUNT;
  double sysLoadPercentage;

  if(host_statistics(mach_host_self(), HOST_CPU_LOAD_INFO, (host_info_t)&cpuinfo, &count) == KERN_SUCCESS)
  {
    unsigned long totalTicks = 0;

    for(int i=0;i<CPU_STATE_MAX;i++)
      totalTicks += cpuinfo.cpu_ticks[i];

    sysLoadPercentage = CalculateCPULoadMac(cpuinfo.cpu_ticks[CPU_STATE_IDLE], totalTicks);
  }
  else
    return -1.0;

  return sysLoadPercentage * 100;
}

double NodeMonitor::GetFreeRAM()
{
  vm_size_t page_size;
  mach_port_t mach_port;
  mach_msg_type_number_t count;
  vm_statistics_data_t vm_stats;
  int64_t freeMemory;

  mach_port = mach_host_self();
  count     = sizeof(vm_stats) / sizeof(natural_t);

  if(KERN_SUCCESS == host_page_size(mach_port, &page_size) &&
    KERN_SUCCESS == host_statistics(mach_port, HOST_VM_INFO, (host_info_t)&vm_stats, &count))
      freeMemory = (int64_t)vm_stats.free_count * (int64_t)page_size;

  return freeMemory;
}

double NodeMonitor::CalculateCPULoadMac(unsigned long idleTicks, unsigned long totalTicks)
{
  unsigned long totalTicksSinceLastTime = totalTicks - previousTotalTicks;
  unsigned long idleTicksSinceLastTime = idleTicks - previousIdleTicks;
  double ret = 1.0 - ((totalTicksSinceLastTime > 0) ? ((double)idleTicksSinceLastTime / totalTicksSinceLastTime) : 0);

  previousTotalTicks = totalTicks;
  previousIdleTicks = idleTicks;

  return ret;
}
#endif