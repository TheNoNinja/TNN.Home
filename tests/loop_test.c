#include "test.h"
#include "loop/loop.h"
#include "platform/platform.h"

#include <signal.h>

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
    TEST_EXPECT(!loop_has_timer_fired(&l, t));

    l.now_ms = 1010;
    loop_fire_timers(&l);
    TEST_EXPECT(loop_has_timer_fired(&l, t));
    TEST_EXPECT(loop_timeout_ms(&l) == -1);

    loop_fire_timers(&l);
    TEST_EXPECT(!loop_has_timer_fired(&l, t));
}

void loop_test_timer_repeats(void)
{
    Loop l = loop_create();
    l.now_ms = 1000;
    const u32 t = loop_timer_add(&l);
    loop_timer_repeat(&l, t, 10);
    l.now_ms = 1010;
    loop_fire_timers(&l);
    TEST_EXPECT(loop_has_timer_fired(&l, t));
    TEST_EXPECT(loop_timeout_ms(&l) == 10);
    
    l.now_ms = 1023;
    loop_fire_timers(&l);
    TEST_EXPECT(loop_has_timer_fired(&l, t));
    TEST_EXPECT(loop_timeout_ms(&l) == 7);

    l.now_ms = 1075;
    loop_fire_timers(&l);
    TEST_EXPECT(loop_has_timer_fired(&l, t));
    TEST_EXPECT(loop_timeout_ms(&l) == 10);
    loop_fire_timers(&l);
    TEST_EXPECT(!loop_has_timer_fired(&l, t));

    loop_timer_stop(&l, t);
    TEST_EXPECT(loop_timeout_ms(&l) == -1);
    loop_timer_start(&l, t, 5);
    l.now_ms = 1080;
    loop_fire_timers(&l);
    TEST_EXPECT(loop_has_timer_fired(&l, t) && loop_timeout_ms(&l) == -1);
}

void loop_test_pipe(void)
{
    PlatformHandle ends[2];
    TEST_EXPECT(platform_socket_pair(ends));
    Loop l = loop_create();
    loop_watch(&l, ends[0]);
    const u32 t = loop_timer_add(&l);

    loop_timer_start(&l, t, 1);
    TEST_EXPECT(loop_wait(&l));
    TEST_EXPECT(loop_has_timer_fired(&l, t) && !loop_is_readable(&l, ends[0]));
    TEST_EXPECT(platform_send(ends[1], STR("x")) == 1);
    TEST_EXPECT(loop_wait(&l));
    TEST_EXPECT(loop_is_readable(&l, ends[0]) && !loop_has_timer_fired(&l, t));
    platform_close(&ends[0]);
    platform_close(&ends[1]);
}

void loop_test_signal(void)
{
    Loop l = loop_create();
    platform_signals_catch();
    raise(SIGTERM);
    TEST_EXPECT(!loop_wait(&l) && l.stop_signal == SIGTERM);

    const u32 t = loop_timer_add(&l);
    loop_timer_start(&l, t, 0);
    TEST_EXPECT(loop_wait(&l) && loop_has_timer_fired(&l, t));
}

void loop_test_unwatch(void)
{
    PlatformHandle a[2];
    PlatformHandle b[2];
    TEST_EXPECT(platform_socket_pair(a) && platform_socket_pair(b));
    Loop l = loop_create();
    loop_watch(&l, a[0]);
    loop_watch(&l, b[0]);
    TEST_EXPECT(platform_send(a[1], STR("x")) == 1 && platform_send(b[1], STR("x")) == 1);
    TEST_EXPECT(loop_wait(&l));
    TEST_EXPECT(loop_is_readable(&l, a[0]) && loop_is_readable(&l, b[0]));

    loop_unwatch(&l, a[0]);
    TEST_EXPECT(l.watch_count == 1 && !loop_is_readable(&l, a[0]) && loop_is_readable(&l, b[0]));

    loop_watch(&l, a[0]);
    TEST_EXPECT(l.watch_count == 2 && !loop_is_readable(&l, a[0]));
    TEST_EXPECT(loop_wait(&l) && loop_is_readable(&l, a[0]));

    loop_unwatch(&l, a[1]);
    TEST_EXPECT(l.watch_count == 2);
    platform_close(&a[0]);
    platform_close(&a[1]);
    platform_close(&b[0]);
    platform_close(&b[1]);
}