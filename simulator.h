#pragma once

#include "scheduler.h"
#include <vector>
#include <utility>
#include <string>
#include <cstdint>

// Результаты прогона
struct SimResult {
std::uint64_t overheadTicks = 0;   // сколько тактов ушло на переключения
double overheadPercent = 0;        // процент от общего времени
  std::string algorithm;
  double avgWaiting = 0;
  double avgTurnaround = 0;
  double avgResponse = 0;
  double cpuUtilization = 0;
  int contextSwitches = 0;
  int throughput = 0;
  std::uint64_t totalTicks = 0;
  std::vector<std::pair<int, std::pair<std::uint64_t, std::uint64_t>>> gantt;
};

// Прогон одного планировщика
SimResult runSimulation(Scheduler& sched,
                        std::uint64_t maxTicks = 100000,
                        std::uint64_t switchCost = 0);
