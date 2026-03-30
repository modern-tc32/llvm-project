// RUN: %clang_cc1 -triple tc32 -O2 -emit-llvm -S -o - %s | FileCheck %s

typedef int (*timer_cb_t)(void *);

struct timer_evt {
  struct timer_evt *next;
  void *data;
  timer_cb_t cb;
  int timeout;
};

extern struct timer_evt *g_head;

void ev_timer_executeCB(void) {
  struct timer_evt *evt = g_head;

  while (evt) {
    if (evt->timeout == 0)
      (void)evt->cb(evt->data);
    evt = evt->next;
  }
}

void ev_timer_process(int armed) {
  if (armed)
    ev_timer_executeCB();
}

// CHECK-LABEL: define{{.*}}noinline{{.*}} void @ev_timer_executeCB(
// CHECK-LABEL: define{{.*}} void @ev_timer_process(
// CHECK: call void @ev_timer_executeCB()
// CHECK-NOT: call i32 %
