#include "fcfs.h"
#include "sjf.h"
#include "srtn.h"
#include "rr.h"
#include "priority.h"
#include "mlfq.h"
#include "simulator.h"
#include "testsets.h"
#include "hrrn.h"

#include <functional>
#include <memory>
#include <iostream>
#include <iomanip>
#include <vector>

std::vector<Process> makeTestSet() {
  std::vector<Process> procs;
  auto add = [&](int pid, const std::string& name, std::uint64_t arrival,
                 std::uint64_t burst, int priority,
                 std::vector<IoBlock> io = {}) {
    Process p;
    p.pid = pid;
    p.name = name;
    p.arrivalTime = arrival;
    p.burstTime = burst;
    p.remainingTime = burst;
    p.priority = priority;
    p.dynamicPriority = priority;
    p.ioBlocks = std::move(io);
    procs.push_back(p);
  };
  add(1, "P1", 0, 8, 3);
  add(2, "P2", 1, 4, 1);
  add(3, "P3", 2, 9, 4);
  add(4, "P4", 3, 5, 2);
  return procs;
}

std::vector<Process> makeIoTestSet() {
  std::vector<Process> procs;
  auto add = [&](int pid, const std::string& name,
                 std::uint64_t arrival, std::uint64_t burst,
                 int priority,
                 std::vector<IoBlock> io = {}) {
    Process p;
    p.pid = pid; p.name = name;
    p.arrivalTime = arrival;
    p.burstTime = burst;
    p.remainingTime = burst;
    p.priority = priority;
    p.dynamicPriority = priority;
    p.ioBlocks = std::move(io);
    procs.push_back(p);
  };
  add(1, "CPU1", 0, 20, 2);
  add(2, "IO1",  1,  2, 1, {{1, 5}, {1, 5}, {0, 0}});
  add(3, "IO2",  2,  2, 1, {{1, 5}, {1, 5}, {0, 0}});
  add(4, "CPU2", 3, 10, 3);
  return procs;
}

std::vector<Process> makeHrrnTestSet() {
  std::vector<Process> procs;
  auto add = [&](int pid, const std::string& name, std::uint64_t arrival,
                 std::uint64_t burst, int priority) {
    Process p;
    p.pid = pid; p.name = name;
    p.arrivalTime = arrival;
    p.burstTime = burst;
    p.remainingTime = burst;
    p.priority = priority;
    p.dynamicPriority = priority;
    procs.push_back(p);
  };
  add(1, "A",  0, 4, 1);
  add(2, "L",  1, 6, 1);
  add(3, "S1", 4, 2, 1);
  add(4, "S2", 5, 1, 1);
  return procs;
}
std::vector<Process> makeConvoySet() {
  std::vector<Process> procs;
  auto add = [&](int pid, const std::string& name, std::uint64_t arrival,
                 std::uint64_t burst, int priority,
                 std::vector<IoBlock> io = {}) {
    Process p;
    p.pid = pid; p.name = name;
    p.arrivalTime = arrival;
    p.burstTime = burst;
    p.remainingTime = burst;
    p.priority = priority;
    p.dynamicPriority = priority;
    p.ioBlocks = std::move(io);
    procs.push_back(p);
  };
  add(1, "CPU1", 0, 20, 2);
  add(2, "IO1",  1,  6, 1, {{1, 4}, {2, 4}, {3, 4}, {4, 4}, {5, 4}});
  add(3, "IO2",  2,  6, 1, {{1, 4}, {2, 4}, {3, 4}, {4, 4}, {5, 4}});
  add(4, "CPU2", 3, 12, 3);
  return procs;
}
void printResult(const SimResult& r) {
  std::cout << std::left << std::setw(30) << r.algorithm
            << " | wait=" << std::setw(7) << std::fixed << std::setprecision(2) << r.avgWaiting
            << " | turn=" << std::setw(7) << r.avgTurnaround
            << " | resp=" << std::setw(7) << r.avgResponse
            << " | CPU=" << std::setw(6) << r.cpuUtilization << "%"
            << " | CS=" << std::setw(4) << r.contextSwitches
            << " | done=" << r.throughput
            << " | ovh=" << std::setw(6) << std::fixed << std::setprecision(2)
            << r.overheadPercent << "%"
            << "\n";
}

void printGantt(const SimResult& r) {
  std::cout << "Gantt (" << r.algorithm << "):\n";
  for (auto& [pid, span] : r.gantt) {
    std::cout << "  [" << span.first << "-" << span.second << ") ";
if (pid == -1) std::cout << "IDLE\n";
else if (pid == -2) std::cout << "CS\n";
else std::cout << "P" << pid << "\n";
  }
  std::cout << "\n";
}
// Печатает Гант, используя имена процессов вместо "P" + pid.
// switchCost нужен, чтобы понимать: pid == -2 это CS.
void printGanttNamed(const SimResult& r, const std::vector<Process>& procs) {
  std::cout << "Gantt (" << r.algorithm << "):\n";
  for (auto& [pid, span] : r.gantt) {
    std::cout << "  [" << span.first << "-" << span.second << ") ";
    if (pid == -1) {
      std::cout << "IDLE\n";
    } else if (pid == -2) {
      std::cout << "CS\n";
    } else {
      // ищем имя процесса
      std::string name = "?";
      for (const auto& p : procs) {
        if (p.pid == pid) { name = p.name; break; }
      }
      std::cout << name << "\n";
    }
  }
  std::cout << "\n";
}
using Factory = std::function<std::unique_ptr<Scheduler>(const std::vector<Process>&)>;

template <class T, class... Args>
Factory factory(Args... args) {
    return [args...](const std::vector<Process>& s) {
        return std::make_unique<T>(s, args...);
    };
}

// metric: 0 = avgWaiting, 1 = avgTurnaround, 2 = avgResponse, 3 = cpuUtilization
double getMetric(const SimResult& r, int metric) {
    switch (metric) {
        case 0: return r.avgWaiting;
        case 1: return r.avgTurnaround;
        case 2: return r.avgResponse;
        default: return r.cpuUtilization;
    }
}

void runExperiment10(int metric, const std::string& metricName) {
    std::vector<std::pair<std::string, Factory>> algos = {
        {"FCFS",       factory<FcfsScheduler>()},
        {"SJF",        factory<SjfScheduler>()},
        {"SRTN",       factory<SrtnScheduler>()},
        {"HRRN",       factory<HrrnScheduler>()},
        {"RR q=4",     factory<RrScheduler>(4)},
        {"Prio",       factory<PriorityScheduler>(false, false)},
        {"Prio+aging", factory<PriorityScheduler>(true, true)},
        {"MLFQ",       factory<MlfqScheduler>()},
    };
    const int sizes[5] = {12, 14, 16, 18, 20};

    std::cout << "\n" << metricName << "\n";
    std::cout << std::left << std::setw(12) << "Algorithm" << std::right;
    for (int i = 1; i <= 5; ++i)
        std::cout << std::setw(9) << ("set " + std::to_string(i));
    std::cout << std::setw(9) << "average" << "\n";

    for (auto& [label, make] : algos) {
        std::cout << std::left << std::setw(12) << label << std::right;
        double sum = 0;
        for (unsigned seed = 1; seed <= 5; ++seed) {
            // загружаем сохранённый набор — так воспроизводимо на любом компиляторе
            std::vector<Process> set;
            loadSet("sets/set" + std::to_string(seed) + ".txt", set);
            (void)sizes;

            auto sched = make(set);
            SimResult r = runSimulation(*sched);
            double v = getMetric(r, metric);
            sum += v;
            std::cout << std::setw(9) << std::fixed << std::setprecision(2) << v;
        }
        std::cout << std::setw(9) << sum / 5 << "\n";
    }
}
int main() {
  std::cout << "Case 1: CPU-bound\n";
  {
    auto set1 = makeTestSet();
    std::vector<std::unique_ptr<Scheduler>> scheds;
    scheds.push_back(std::make_unique<FcfsScheduler>(set1));
    scheds.push_back(std::make_unique<SjfScheduler>(set1));
    scheds.push_back(std::make_unique<SrtnScheduler>(set1));
    scheds.push_back(std::make_unique<RrScheduler>(set1, 1));
    scheds.push_back(std::make_unique<RrScheduler>(set1, 2));
    scheds.push_back(std::make_unique<RrScheduler>(set1, 4));
    scheds.push_back(std::make_unique<PriorityScheduler>(set1, false, false));
    scheds.push_back(std::make_unique<PriorityScheduler>(set1, true, false));
    scheds.push_back(std::make_unique<PriorityScheduler>(set1, true, true));
    scheds.push_back(std::make_unique<MlfqScheduler>(set1));
    for (auto& s : scheds) printResult(runSimulation(*s));
  }

  std::cout << "\nCase 2. Mixed loading: I/O + CPU\n";
  {
    auto set2 = makeIoTestSet();
    std::vector<std::unique_ptr<Scheduler>> scheds;
    scheds.push_back(std::make_unique<FcfsScheduler>(set2));
    scheds.push_back(std::make_unique<SrtnScheduler>(set2));
    scheds.push_back(std::make_unique<RrScheduler>(set2, 4));
    scheds.push_back(std::make_unique<PriorityScheduler>(set2, true, true));
    scheds.push_back(std::make_unique<MlfqScheduler>(set2));
    for (auto& s : scheds) printResult(runSimulation(*s));
  }

  // Задание 3: SJF vs HRRN на наборе A/L/S1/S2
  {
    auto set = makeHrrnTestSet();
    std::cout << "\nСравнение SJF и HRRN:\n";
    SjfScheduler  sjf(set);
    HrrnScheduler hrrn(set);
    SimResult r1 = runSimulation(sjf);
    SimResult r2 = runSimulation(hrrn);
    printResult(r1);
    printResult(r2);
    std::cout << "\nSJF Gantt:\n";
    printGanttNamed(r1, sjf.processes());
    std::cout << "\nHRRN Gantt:\n";
    printGanttNamed(r2, hrrn.processes());
  }

  // Диаграмма Ганта для RR (q=2)
  {
    auto set = makeTestSet();
    RrScheduler rr(set, 2);
    SimResult r = runSimulation(rr);
    std::cout << "\n";
    printGantt(r);
  }

  // Задание 8: таблица по процессам
  {
    auto set = makeTestSet();
    RrScheduler rr(set, 2);
    SimResult r = runSimulation(rr);
    std::cout << "\nПо процессам, RR (q=2):\n";
    printProcessTable(rr.processes());
  }

  // Задание 13: сохранение/загрузка
  {
    auto set = makeTestSet();
    saveSet("sets/set_basic.txt", set);

    std::vector<Process> loaded;
    if (!loadSet("sets/set_basic.txt", loaded)) {
        std::cerr << "Не удалось загрузить sets/set_basic.txt\n";
        return 1;
    }

    FcfsScheduler a(set), b(loaded);
    std::cout << "\nПроверка сохранения/загрузки (FCFS):\n";
    printResult(runSimulation(a));
    printResult(runSimulation(b));
  }
// Задание 4: накладные расходы на переключение
{
    auto set = makeTestSet();
    std::cout << "\nВлияние switchCost на RR (набор makeTestSet)\n";
    std::cout << "q  cost  wait   turn   resp   CPU%   ovh%   CS\n";
    for (std::uint64_t q : {1, 2, 4, 8}) {
        for (std::uint64_t cost : {0, 1}) {
            RrScheduler rr(set, q);
            SimResult r = runSimulation(rr, 100000, cost);
            std::cout << std::setw(2) << q << "  "
                      << std::setw(3) << cost << "  "
                      << std::fixed << std::setprecision(2)
                      << std::setw(6) << r.avgWaiting << " "
                      << std::setw(6) << r.avgTurnaround << " "
                      << std::setw(6) << r.avgResponse << " "
                      << std::setw(6) << r.cpuUtilization << " "
                      << std::setw(6) << r.overheadPercent << " "
                      << std::setw(4) << r.contextSwitches << "\n";
        }
    }
}
//Задание 5
{
  auto set = makeTestSet();
  std::cout << "\nВлияние кванта в RR (набор makeTestSet)\n";
  std::cout << "q   wait   turn   resp   CS\n";
  for (std::uint64_t q : {1, 2, 4, 8, 16}) {
    RrScheduler rr(set, q);
    SimResult r = runSimulation(rr);
    std::cout << std::setw(2) << q << "  " << std::fixed << std::setprecision(2)
              << r.avgWaiting << "  " << r.avgTurnaround << "  " << r.avgResponse
              << "  " << std::setw(3) << r.contextSwitches << "  "
              << std::string(r.contextSwitches, '#') << "\n";
  }
}
// Задание 6: convoy effect — FCFS vs MLFQ
{
    auto set = makeConvoySet();
    std::cout << "\nЗадание 6. Convoy effect (I/O + CPU)\n";

    std::vector<std::unique_ptr<Scheduler>> scheds;
    scheds.push_back(std::make_unique<FcfsScheduler>(set));
    scheds.push_back(std::make_unique<SrtnScheduler>(set));
    scheds.push_back(std::make_unique<RrScheduler>(set, 4));
    scheds.push_back(std::make_unique<PriorityScheduler>(set, true, true));
    scheds.push_back(std::make_unique<MlfqScheduler>(set));
    for (auto& s : scheds) printResult(runSimulation(*s));
}

// Задание 6: таблицы по процессам для FCFS и MLFQ
{
    auto set = makeConvoySet();

    FcfsScheduler fcfs(set);
    SimResult r1 = runSimulation(fcfs);
    std::cout << "\nFCFS, по процессам:\n";
    printProcessTable(fcfs.processes());
    std::cout << "FCFS Gantt:\n";
    printGantt(r1);

    MlfqScheduler mlfq(set);
    SimResult r2 = runSimulation(mlfq);
    std::cout << "\nMLFQ, по процессам:\n";
    printProcessTable(mlfq.processes());
    std::cout << "MLFQ Gantt:\n";
    printGantt(r2);
    {
    saveSet("sets/set_convoy.txt", makeConvoySet());
}
}
// Задание 10: сохраняем 5 случайных наборов
{
    const int sizes[5] = {12, 14, 16, 18, 20};
    for (unsigned seed = 1; seed <= 5; ++seed) {
        saveSet("sets/set" + std::to_string(seed) + ".txt",
                makeRandomSet(seed, sizes[seed - 1]));
    }
    std::cout << "\nНаборы sets/set1..set5.txt сохранены.\n";
}
// Задание 10: сравнение алгоритмов по 5 наборам
runExperiment10(0, "Average waiting time");
runExperiment10(1, "Average turnaround time");
runExperiment10(2, "Average response time");
runExperiment10(3, "CPU utilization %");
  return 0;
}
