/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#include "../base/KernelProxy.h"
#include "../console/JSPipeMount.h"
#include "gtest/gtest.h"
#include <list>
#include <pthread.h>


#define CHECK(x) { \
    ASSERT_NE(reinterpret_cast<void*>(NULL), x); \
} while (0)

// Preprocessor macro to simplify dealing with several calls expecting a
// const void* followed by a count.
#define STRING_PAIR(s) s, (sizeof(s)-1)

class JSOutboundPipeBridgeMock : public JSOutboundPipeBridge {
public:
  ~JSOutboundPipeBridgeMock() {
    EXPECT_TRUE(expectations_.size() == 0);
  }

  void Post(const void *buf, size_t count) {
    EXPECT_TRUE(expectations_.size() > 0);
    std::string expected_msg = expectations_.front();
    expectations_.pop_front();
    std::string msg = std::string(reinterpret_cast<const char*>(buf), count);
    EXPECT_EQ(expected_msg, msg);
  }

  void Expect(const std::string& msg) {
    expectations_.push_back(msg);
  }

private:
  std::list<std::string> expectations_;
};

TEST(JSPipeMountTest, ConstructDestruct) {
  JSPipeMount *mount = new JSPipeMount;
  delete mount;
}

TEST(JSPipeMountTest, Write) {
  JSPipeMount mount;
  JSOutboundPipeBridgeMock bridge;
  mount.set_outbound_bridge(&bridge);

  bridge.Expect("JSPipeMount:123:hello");
  bridge.Expect("JSPipeMount:123:there!");

  struct stat st;
  EXPECT_EQ(mount.Creat("/123", 0, &st), 0);

  EXPECT_EQ(mount.Write(st.st_ino, 0, "hello", 5), 5);
  EXPECT_EQ(mount.Write(st.st_ino, 0, "there!", 6), 6);
}

TEST(JSPipeMountTest, WritePrefix) {
  JSPipeMount mount;
  JSOutboundPipeBridgeMock bridge;
  mount.set_outbound_bridge(&bridge);
  mount.set_prefix("foo");

  bridge.Expect("foo:1776:bar");

  struct stat st;
  EXPECT_EQ(mount.Creat("/1776", 0, &st), 0);

  EXPECT_EQ(mount.Write(st.st_ino, 0, STRING_PAIR("bar")), 3);
}

TEST(JSPipeMountTest, Isatty) {
  JSPipeMount mount;
  EXPECT_EQ(mount.Isatty(1234), 1);
  mount.set_is_tty(0);
  EXPECT_EQ(mount.Isatty(1234), 0);
  mount.set_is_tty(1);
  EXPECT_EQ(mount.Isatty(1234), 1);
}

TEST(JSPipeMountTest, Read) {
  JSPipeMount mount;

  struct stat st;
  EXPECT_EQ(mount.Creat("/1234", 0, &st), 0);

  EXPECT_TRUE(mount.Receive(STRING_PAIR("JSPipeMount:1234:Testing 123!")));
  EXPECT_FALSE(mount.Receive(STRING_PAIR("Random:1234:ABCDEFGH")));
  EXPECT_TRUE(mount.Receive(STRING_PAIR("JSPipeMount:1234:ABCDEFGH")));

  char buffer[40];
  EXPECT_EQ(mount.Read(st.st_ino, 0, buffer, 7), 7);
  EXPECT_TRUE(memcmp(buffer, STRING_PAIR("Testing")) == 0);
  EXPECT_EQ(mount.Read(st.st_ino, 0, buffer, 8), 8);
  EXPECT_TRUE(memcmp(buffer, STRING_PAIR(" 123!ABC")) == 0);
  EXPECT_EQ(mount.Read(st.st_ino, 0, buffer, 40), 5);
  EXPECT_TRUE(memcmp(buffer, STRING_PAIR("DEFGH")) == 0);
}

TEST(JSPipeMountTest, ReadPrefix) {
  JSPipeMount mount;
  mount.set_prefix("foo");

  struct stat st;
  EXPECT_EQ(mount.Creat("/1234", 0, &st), 0);

  EXPECT_FALSE(mount.Receive(STRING_PAIR("JSPipeMount:1234:Testing 123!")));
  EXPECT_TRUE(mount.Receive(STRING_PAIR("foo:1234:ABCDEFGH")));

  char buffer[40];
  EXPECT_EQ(mount.Read(st.st_ino, 0, buffer, 40), 8);
  EXPECT_TRUE(memcmp(buffer, STRING_PAIR("ABCDEFGH")) == 0);
}

TEST(JSPipeMountTest, CornerCaseMessages) {
  JSPipeMount mount;

  struct stat st;
  EXPECT_EQ(mount.Creat("/1234", 0, &st), 0);

  EXPECT_TRUE(mount.Receive(STRING_PAIR("JSPipeMount:1234:")));
  EXPECT_TRUE(mount.Receive(STRING_PAIR("JSPipeMount:1:")));
  EXPECT_FALSE(mount.Receive(STRING_PAIR("JSPipeMount:ABC:123")));
  EXPECT_FALSE(mount.Receive(STRING_PAIR("JSPipeMount::123")));
  EXPECT_FALSE(mount.Receive(STRING_PAIR("")));
  EXPECT_FALSE(mount.Receive(STRING_PAIR("JSPipeMount123:123:")));
}

TEST(JSPipeMountTest, CornerCasePaths) {
  JSPipeMount mount;

  struct stat st;
  EXPECT_NE(mount.Creat("1234", 0, &st), 0);
  EXPECT_NE(mount.Creat("/abc", 0, &st), 0);
  EXPECT_NE(mount.Creat("//123", 0, &st), 0);
  EXPECT_NE(mount.Creat("/dev/pipe/123", 0, &st), 0);
}

static void *Blocking_sender(void *arg) {
  JSPipeMount *mount = reinterpret_cast<JSPipeMount*>(arg);

  usleep(50 * 1000);  // 50 ms
  EXPECT_TRUE(mount->Receive(STRING_PAIR("JSPipeMount:1234:Testing")));
  usleep(50 * 1000);  // 50 ms
  EXPECT_TRUE(mount->Receive(STRING_PAIR("JSPipeMount:1234:ABC")));

  return 0;
}

TEST(JSPipeMountTest, Blocking) {
  JSPipeMount mount;

  struct stat st;
  EXPECT_EQ(mount.Creat("/1234", 0, &st), 0);

  pthread_t thread_id;
  pthread_create(&thread_id, 0, Blocking_sender, &mount);
  char buffer[40];
  EXPECT_EQ(mount.Read(st.st_ino, 0, buffer, 7), 7);
  EXPECT_TRUE(memcmp(buffer, STRING_PAIR("Testing")) == 0);
  EXPECT_EQ(mount.Read(st.st_ino, 0, buffer, 3), 3);
  EXPECT_TRUE(memcmp(buffer, STRING_PAIR("ABC")) == 0);

  void *ret;
  pthread_join(thread_id, &ret);
  assert(ret == 0);
}

TEST(JSPipeMountTest, Select) {
  KernelProxy* kp = KernelProxy::KPInstance();
  JSPipeMount* mount = new JSPipeMount();
  kp->mount("/pipe", mount);

  int pipe = kp->open("/pipe/1234", O_RDONLY, 0600);
  EXPECT_GT(pipe, 0);

  fd_set readfds, writefds, exceptfds;
  FD_ZERO(&readfds);
  FD_ZERO(&writefds);
  FD_ZERO(&exceptfds);

  // Pipe is not readable to begin with.
  struct timeval timeout = { 0, 0 };
  FD_SET(pipe, &readfds);
  FD_SET(pipe, &writefds);
  FD_SET(pipe, &exceptfds);
  EXPECT_EQ(1, kp->select(pipe + 1, &readfds, &writefds, &exceptfds, &timeout));
  EXPECT_FALSE(FD_ISSET(pipe, &readfds));
  EXPECT_TRUE(FD_ISSET(pipe, &writefds));
  EXPECT_FALSE(FD_ISSET(pipe, &exceptfds));

  // After sending some data, it is readable.
  EXPECT_TRUE(mount->Receive(STRING_PAIR("JSPipeMount:1234:hello")));
  FD_SET(pipe, &readfds);
  FD_SET(pipe, &writefds);
  FD_SET(pipe, &exceptfds);
  EXPECT_EQ(2, kp->select(pipe + 1, &readfds, &writefds, &exceptfds, &timeout));
  EXPECT_TRUE(FD_ISSET(pipe, &readfds));
  EXPECT_TRUE(FD_ISSET(pipe, &writefds));
  EXPECT_FALSE(FD_ISSET(pipe, &exceptfds));

  // Consume the data.
  char buf[256];
  EXPECT_EQ(5, kp->read(pipe, buf, sizeof(buf)));
  EXPECT_EQ(0, memcmp(buf, "hello", 5));

  // Test that we wake up when the pipe becomes readable.
  pthread_t thread_id;
  pthread_create(&thread_id, 0, Blocking_sender, mount);
  FD_SET(pipe, &readfds);
  EXPECT_EQ(1, kp->select(pipe + 1, &readfds, NULL, NULL, NULL));
  EXPECT_TRUE(FD_ISSET(pipe, &readfds));

  void* ret;
  pthread_join(thread_id, &ret);
  assert(ret == 0);

  kp->umount("/pipe");
}
