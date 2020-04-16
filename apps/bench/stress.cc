extern "C" {
#include <base/log.h>
#undef min
#undef max
}

#include "thread.h"
#include "sync.h"
#include "timer.h"
#include "fake_worker.h"

#include <iostream>
#include <chrono>
#include <fstream>
#include <signal.h>


namespace {

int threads;
uint64_t n;
std::string worker_spec;
std::ofstream myfile;


void MainHandler(void *arg) {
  rt::WaitGroup wg(1);
  uint64_t cnt[threads] = {};
  myfile.open("counters.csv");

  for (int i = 0; i < threads; ++i) {
    rt::Spawn([&,i](){
      auto *w = FakeWorkerFactory(worker_spec);
      if (w == nullptr) {
        std::cerr << "Failed to create worker." << std::endl;
        exit(1);
      }

      while (true) {
        w->Work(n);
        cnt[i]++;
        rt::Yield();
      }
    });
  }

  rt::Spawn([&](){
    uint64_t last_total = 0;
    auto last = std::chrono::steady_clock::now();
    while (1) {
      rt::Sleep(rt::kSeconds);
      auto now = std::chrono::steady_clock::now();
      uint64_t total = 0;
      double duration = std::chrono::duration_cast<
        std::chrono::duration<double>>(now - last).count();
      double us_d = std::chrono::duration_cast<std::chrono::microseconds>(now - last).count();
      for (int i = 0; i < threads; i++) total += cnt[i];
      log_info("%f - %f", static_cast<double>(total - last_total) / duration, static_cast<double>(total - last_total) / us_d);
      last_total = total;
      last = now;
      for(int i = 0; i < threads;i++)
        myfile << cnt[i] << ",";
      myfile << std::endl;

    }
  });

  // never returns
  wg.Wait();
}

void handle_sigint(int sig) 
{ 
    printf("Caught signal %d\n", sig); 
    myfile.close();
    exit(0);
} 

} // anonymous namespace

int main(int argc, char *argv[]) {
  int ret;

  if (argc != 5) {
    std::cerr << "usage: [config_file] [#threads] [#n] [worker_spec]"
              << std::endl;
    return -EINVAL;
  }
  signal(SIGINT, handle_sigint); 

  threads = std::stoi(argv[2], nullptr, 0);
  n = std::stoul(argv[3], nullptr, 0);
  worker_spec = std::string(argv[4]);

  ret = runtime_init(argv[1], MainHandler, NULL);
  if (ret) {
    printf("failed to start runtime\n");
    return ret;
  }

  return 0;
}
