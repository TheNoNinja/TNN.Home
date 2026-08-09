#include "test.h"
#include "loop/loop.h"

#include <signal.h>
#include <unistd.h>

void loop_test_timeout(void)
{
    Loop l = loop_create();
    l.now_ms = 1000;
    TEST_EXPECT(loop_timeout_ms(&l) == -1);
    const u32 a = loop_timer_add(&l);
    const u32 b = loop_timer_add(&l);
    loop_timer_start(&l, a, 500);
    loop_timer_start(&l, b, 200);
    TEST_EXPECT(loop_timeout_ms(&l) == 200);

    l.now_ms = 1300;
    TEST_EXPECT(loop_timeout_ms(&l) == 0);
    loop_timer_stop(&l, b);
    TEST_EXPECT(loop_timeout_ms(&l) == 200);
}

void loop_test_timer_fires(void)
{
    Loop l = loop_create();
    l.now_ms = 1000;
    const u32 t = loop_timer_add(&l);
    loop_timer_start(&l, t, 10);
    
    l.now_ms = 1009;
    loop_fire_timers(&l);
    TEST_EXPECT(!loop_timer_fired(&l, t));

    l.now_ms = 1010;
    loop_fire_timers(&l);
    TEST_EXPECT(loop_timer_fired(&l, t));
    TEST_EXPECT(loop_timeout_ms(&l) == -1);

    loop_fire_timers(&l);
    TEST_EXPECT(!loop_timer_fired(&l, t));
}

void loop_test_pipe(void)
{
    int fds[2];
    TEST_EXPECT(pipe(fds) == 0);
    Loop l = loop_create();
    loop_watch(&l, fds[0]);
    const u32 t = loop_timer_add(&l);

    loop_timer_start(&l, t, 1);
    TEST_EXPECT(loop_wait(&l));
    TEST_EXPECT(loop_timer_fired(&l, t) && !loop_readable(&l, fds[0]));
    TEST_EXPECT(write(fds[1], "x", 1) == 1);
    TEST_EXPECT(loop_wait(&l));
    TEST_EXPECT(loop_readable(&l, fds[0]) && !loop_timer_fired(&l, t));
    close(fds[0]);
    close(fds[1]);
}

void loop_test_signal(void)
{
    Loop l = loop_create();
    loop_catch_signals();
    raise(SIGTERM);
    TEST_EXPECT(!loop_wait(&l) && l.stop_signal == SIGTERM);

    const u32 t = loop_timer_add(&l);
    loop_timer_start(&l, t, 0);
    TEST_EXPECT(loop_wait(&l) && loop_timer_fired(&l, t));
}