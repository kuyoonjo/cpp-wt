#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <utility>

#if defined(__cpp_lib_hardware_interference_size) &&                           \
    !defined(USING_HARDWARE_DESTRUCTIVE_INTERFERENCE_SIZE)
#define __cpp_lib_hardware_interference_size_back                              \
  __cpp_lib_hardware_interference_size
#undef __cpp_lib_hardware_interference_size
#endif

#include <rigtorp/MPMCQueue.h>

#ifdef __cpp_lib_hardware_interference_size_back
#define __cpp_lib_hardware_interference_size                                   \
  __cpp_lib_hardware_interference_size_back
#undef __cpp_lib_hardware_interference_size_back
#endif

#include <cstdio>
#include <map>
#include <thread>
#include <vector>

namespace wt {
using buffer_t = std::vector<uint8_t>;

struct message_t {
  size_t mailbox_id;
  buffer_t buffer;
};

using message_ptr = std::shared_ptr<message_t>;

using queue_t = rigtorp::MPMCQueue<message_ptr>;

constexpr int DEFAULT_QUEUE_SIZE = 1024;
constexpr int MASTER_QUEUE_SIZE = 1024;

class Mailbox {
public:
  const size_t &id = m_id;
  const std::shared_ptr<queue_t> &queue = m_queue;
  static size_t last_mailbox_id;
  static std::map<size_t, std::shared_ptr<Mailbox>> mailboxes;

  explicit Mailbox(size_t queue_size = DEFAULT_QUEUE_SIZE) {
    m_id = ++last_mailbox_id;
    m_queue = std::make_shared<queue_t>(queue_size);
    mailboxes.emplace(m_id, this);
  }

  bool postMessage(size_t to_mailbox_id, buffer_t buffer) {
    message_t message{id, std::move(buffer)};
    auto ptr = std::make_shared<message_t>(message);
    auto to = mailboxes.find(to_mailbox_id);
    if (to != mailboxes.end())
      return to->second->queue->try_push(ptr);
    return false;
  }

  bool receiveMessage() {
    message_ptr message;
    if (queue->try_pop(message)) {
      onMessage(message->mailbox_id, message->buffer);
      return true;
    }
    return false;
  }

protected:
  virtual void onMessage(size_t from_mailbox_id, buffer_t buffer) {}

private:
  size_t m_id;
  std::shared_ptr<queue_t> m_queue;
};

inline size_t Mailbox::last_mailbox_id = 0;
inline std::map<size_t, std::shared_ptr<Mailbox>> Mailbox::mailboxes = {};

class Worker : public Mailbox {
public:
  explicit Worker(size_t queue_size = DEFAULT_QUEUE_SIZE)
      : Mailbox(queue_size) {
    m_thread = std::make_shared<std::thread>([this]() {
      onInit();
      while (true) {
        if (m_exit_flag) {
          onExit();
          return;
        }
        receiveMessage();
      }
    });
  };

  void exit() { m_exit_flag = true; }

protected:
  virtual void onInit() {}
  virtual void onExit() {}

private:
  std::atomic_bool m_exit_flag = false;
  std::shared_ptr<std::thread> m_thread = nullptr;
};

} // namespace wt