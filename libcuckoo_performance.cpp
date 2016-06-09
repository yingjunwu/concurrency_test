#include <cassert>
#include <cstdint>
#include <vector>
#include <thread>
#include <cstdio>
#include <chrono>
#include <atomic>

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

std::atomic<int64_t> max_tuple_id;
const int time_duration = 10;
bool is_running = false;
int *operation_counts = nullptr;
FILE * pFile;

void RunWorkerThread(const int &thread_id, cuckoohash_map<int64_t, int64_t> *my_map, const std::vector<int> config) {
  PinToCore(thread_id);
  fast_random rand_gen(thread_id);
  int operation_count = 0;
  // int read_operation_count = 0;
  // int write_operation_count = 0;
  int64_t value = 0;
  while (true) {
    if (is_running == false) {
      break;
    }
    int rand_num = rand_gen.next();
    if (rand_num % 100 < config[0]) {  
      int64_t my_id = max_tuple_id.fetch_add(1, std::memory_order_relaxed);
      bool ret = my_map->insert(my_id, 100);
      if (ret == false) {
        fprintf(stderr, "insert failed!\n");
      }

    }
    else if (rand_num % 100 < config[0] + config[1]) {
        int64_t key = rand_gen.next() % max_tuple_id;
        bool ret = my_map->find(key, value);
        if (ret == false) {
          fprintf(stderr, "read failed! key = %lu, max_tuple_id = %lu\n", key, max_tuple_id.load());
        }
        // ++read_operation_count;
    } else {
      int64_t key = rand_gen.next() % max_tuple_id;
      bool ret = my_map->update(key, 100);
      if (ret == false) {
        fprintf(stderr, "update failed! key = %lu, max_tuple_id = %lu\n", key, max_tuple_id.load());
      }
      // ++write_operation_count;
    }
    ++operation_count;
  }
  operation_counts[thread_id] = operation_count;
  // printf("read count = %d, write count = %d\n", read_operation_count, write_operation_count);
}

void RunWorkload(const int &thread_count, const std::vector<int> config) {
  operation_counts = new int[thread_count];

  cuckoohash_map<int64_t, int64_t> my_map;  
  max_tuple_id = 1000;
  // populate.
  for (int i = 0; i < max_tuple_id; ++i) {
    my_map[i] = 100;
  }

  is_running = true;
  std::vector<std::thread> worker_threads;
  for (int i = 0; i < thread_count; ++i) {
    worker_threads.push_back(std::move(std::thread(RunWorkerThread, i, &my_map, config)));
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

  printf("thread count = %d, read = %d, write = %d, insert = %d, throughput = %.1f M ops\n", thread_count, config[0], config[1], config[2], total_count * 1.0 / time_duration / 1000 / 1000);

  fprintf(pFile, "thread count = %d, read = %d, write = %d, insert = %d, throughput = %.1f M ops\n", thread_count, config[0], config[1], config[2], total_count * 1.0 / time_duration / 1000 / 1000);
  
  fflush(pFile);

  delete[] operation_counts;
  operation_counts = nullptr;
}

int main() {
  pFile = fopen("libcuckoo.log", "w");
  
  std::vector<std::vector<int>> configs;
  // configs.push_back({0,0,100});
  // configs.push_back({0,20,80});
  // configs.push_back({0,80,20});
  // configs.push_back({20,0,80});
  configs.push_back({80,0,20});
  for (auto config : configs) {
    RunWorkload(1, config);
      for (int thread_count = 8; thread_count <= 40; thread_count += 8) {
        RunWorkload(thread_count, config);
      }
  }
      
  fclose(pFile);
}
