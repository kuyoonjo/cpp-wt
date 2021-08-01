#include <chrono>
#include <cstdio>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <worker_thread.h>

using namespace std::chrono_literals;

class Worker : public wt::Worker {
public:
  std::string name;
  explicit Worker(std::string name) : wt::Worker(), name(name) {}
  void onMessage(size_t from_mailbox_id, wt::buffer_t buffer) override {
    std::printf("%s with id %lu from %lu: ", name.c_str(), id, from_mailbox_id);
    for (auto n : buffer)
      std::printf("%02x", n);
    std::printf("\n");
  }

  void onInit() override { std::printf("%s init.\n", name.c_str()); }

  void onExit() override { std::printf("%s exited.\n", name.c_str()); }
};

class Worker2 : public wt::Worker {
public:
  std::string name;
  std::shared_ptr<Worker> next;
  explicit Worker2(std::string name, Worker *next)
      : wt::Worker(), name(name), next(next) {}
  void onMessage(size_t from_mailbox_id, wt::buffer_t buffer) override {
    postMessage(next->id, buffer);
  }
};

int main() {
  wt::Mailbox master;
  wt::Mailbox master2;
  std::cout << "master: " << master.id << std::endl;
  std::cout << "master2: " << master2.id << std::endl;
  auto w1 = Worker("W1");
  auto w2 = Worker2("W2", &w1);
  for (int i = 0;; ++i) {
    if (i < 10) {
      wt::buffer_t m1(1);
      wt::buffer_t m2 = {1};
      master.postMessage(w1.id, m1);
      master.postMessage(w2.id, m2);
      std::this_thread::sleep_for(1s);
    } else {
      w1.exit();
      w2.exit();
    }
  }
  return 0;
}