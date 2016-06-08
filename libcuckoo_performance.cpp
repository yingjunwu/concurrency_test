#include <cassert>
#include <cstdint>
#include <vector>
#include <thread>
#include <iostream>
#include <chrono>

#include "libcuckoo/cuckoohash_map.hh"

using std::chrono::high_resolution_clock;
using std::chrono::system_clock;
using std::chrono::milliseconds;
using std::chrono::microseconds;
using std::chrono::nanoseconds;


void PinToCore(size_t core) {
  cpu_set_t cpuset;
  CPU_ZERO(&cpuset);
  CPU_SET(core, &cpuset);
  int ret = pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
  if (ret != 0) {
    assert(false);
  }
}


// Fast random number generator
class fast_random {
 public:
  fast_random(unsigned long seed) : seed(0) { set_seed0(seed); }

  inline unsigned long next() {
    return ((unsigned long)next(32) << 32) + next(32);
  }

  inline uint32_t next_u32() { return next(32); }

  inline uint16_t next_u16() { return (uint16_t)next(16); }

  /** [0.0, 1.0) */
  inline double next_uniform() {
    return (((unsigned long)next(26) << 27) + next(27)) / (double)(1L << 53);
  }

  inline char next_char() { return next(8) % 256; }

  inline char next_readable_char() {
    static const char readables[] =
        "0123456789@ABCDEFGHIJKLMNOPQRSTUVWXYZ_abcdefghijklmnopqrstuvwxyz";
    return readables[next(6)];
  }

  inline std::string next_string(size_t len) {
    std::string s(len, 0);
    for (size_t i = 0; i < len; i++) s[i] = next_char();
    return s;
  }

  inline std::string next_readable_string(size_t len) {
    std::string s(len, 0);
    for (size_t i = 0; i < len; i++) s[i] = next_readable_char();
    return s;
  }

  inline unsigned long get_seed() { return seed; }

  inline void set_seed(unsigned long seed) { this->seed = seed; }

 private:
  inline void set_seed0(unsigned long seed) {
    this->seed = (seed ^ 0x5DEECE66DL) & ((1L << 48) - 1);
  }

  inline unsigned long next(unsigned int bits) {
    seed = (seed * 0x5DEECE66DL + 0xBL) & ((1L << 48) - 1);
    return (unsigned long)(seed >> (48 - bits));
  }

  unsigned long seed;
};

class TimeMeasurer{
public:
  TimeMeasurer(){}
  ~TimeMeasurer(){}

  void StartTimer(){
    start_time_ = high_resolution_clock::now();
  }

  void EndTimer(){
    end_time_ = high_resolution_clock::now();
  }

  long long GetElapsedMilliSeconds(){
    return std::chrono::duration_cast<milliseconds>(end_time_ - start_time_).count();
  }

private:
  TimeMeasurer(const TimeMeasurer&);
  TimeMeasurer& operator=(const TimeMeasurer&);

private:
  system_clock::time_point start_time_;
  system_clock::time_point end_time_;
};

const int kInitTupleCount = 10000000;
const int time_duration = 10;
bool is_running = false;
int *operation_counts = nullptr;

void RunWorkerThread(const int &thread_id, cuckoohash_map<int64_t, int64_t> *my_map, const int read_percent) {
  PinToCore(thread_id);
  fast_random rand_gen(thread_id);
  int64_t sum = 0;
  int operation_count = 0;
  // int read_operation_count = 0;
  // int write_operation_count = 0;
  while (true) {
    if (is_running == false) {
      break;
    }  
    if (rand_gen.next() % 100 < read_percent) {
      sum += (*my_map)[rand_gen.next() % kInitTupleCount];
      // ++read_operation_count;
    } else {
      (*my_map)[rand_gen.next() % kInitTupleCount] = 100;
      // ++write_operation_count;
    }
    ++operation_count;
  }
  operation_counts[thread_id] = operation_count;
  // printf("read count = %d, write count = %d\n", read_operation_count, write_operation_count);
}

void RunWorkload(const int &thread_count, const int &read_percent) {
  operation_counts = new int[thread_count];

  cuckoohash_map<int64_t, int64_t> my_map;  
  // populate.
  for (int i = 0; i < kInitTupleCount; ++i) {
    my_map[i] = 100;
  }

  is_running = true;
  std::vector<std::thread> worker_threads;
  for (int i = 0; i < thread_count; ++i) {
    worker_threads.push_back(std::move(std::thread(RunWorkerThread, i, &my_map, read_percent)));
  }
  std::this_thread::sleep_for(std::chrono::seconds(time_duration));
  is_running = false;
  for (int i = 0; i < thread_count; ++i) {
    worker_threads.at(i).join();
  }

  int total_count = 0;
  for (int i = 0; i < thread_count; ++i) {
    total_count += operation_counts[i];
  }

  printf("thread count = %d, read percentage = %d, throughput = %.1f M ops\n", thread_count, read_percent, total_count * 1.0 / time_duration / 1000 / 1000);

  delete[] operation_counts;
  operation_counts = nullptr;
}

int main() {
  for (int read_percent = 0; read_percent <= 100; read_percent += 20) {
    RunWorkload(1, read_percent);
    for (int thread_count = 8; thread_count <= 40; thread_count += 8) {
      RunWorkload(thread_count, read_percent);
    }
  }
}
