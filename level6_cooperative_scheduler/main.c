// ============================================================
// main.c
// จำลองการทำงานของ scheduler กับ task จริงๆ ใน firmware
//
// Task ที่ลงทะเบียน (เลียนแบบ use case จริง):
//   1. LED blink        — priority 1, ทุก 500 ms
//   2. sensor read      — priority 2, ทุก 100 ms
//   3. status report    — priority 3, ทุก 1000 ms
//
// ใน MCU จริง task พวกนี้คืองานที่ firmware ทำพร้อมๆ กัน
// โดยไม่ต้องมี RTOS
// ============================================================

#include <stdio.h>
#include "scheduler.h"

// ============================================================
// Global pointer เพื่อให้ task function เข้าถึง current_tick ได้
// ============================================================
//
// เพราะ task function ต้องเป็น void(void) จะส่ง scheduler ให้ไม่ได้
// ใช้ global pointer เป็นทางออกแบบง่าย
//
// ใน firmware จริง หลาย pattern ที่ใช้:
//   - global variable (ง่ายสุด)
//   - callback context (ซับซ้อนกว่าแต่ flexible)
//   - service locator / dependency injection
static Scheduler *g_sched = NULL;

// ============================================================
// Task functions — ฟังก์ชันที่จะถูก scheduler เรียก
// ============================================================
// ใน firmware จริงฟังก์ชันเหล่านี้จะไป control hardware
// (GPIO, ADC, UART...) แต่ในตัวจำลอง print แทน

static void task_led_blink(void)
{
    static bool led_on = false;    // state เก็บใน static
    led_on = !led_on;
    printf("  [T=%5u ms] LED blink       → LED %s\n",
           g_sched->current_tick,
           led_on ? "ON " : "OFF");
}

static void task_sensor_read(void)
{
    static int counter = 0;
    counter++;
    // จำลองค่า sensor ที่เปลี่ยนไป
    int fake_temp = 250 + (counter % 50);  // 25.0 - 29.9°C
    printf("  [T=%5u ms] sensor_read     → temp = %.1f°C\n",
           g_sched->current_tick,
           fake_temp / 10.0);
}

static void task_status_report(void)
{
    printf("  [T=%5u ms] status_report   → system healthy, uptime OK\n",
           g_sched->current_tick);
}

// ============================================================
// task bonus: heartbeat — ทุก 250 ms — เพื่อแสดง priority ชัดขึ้น
// ============================================================
static void task_heartbeat(void)
{
    printf("  [T=%5u ms] heartbeat       → ♥\n",
           g_sched->current_tick);
}

int main(void)
{
    printf("==================================================\n");
    printf("  Cooperative Scheduler — Level 6 Simulation\n");
    printf("==================================================\n\n");

    // ----------------------------------------------------------
    // Setup
    // ----------------------------------------------------------
    Scheduler sched;
    scheduler_init(&sched);
    g_sched = &sched;   // ให้ task function มองเห็น

    // ลงทะเบียน task ทั้งหมด
    // priority: เลขน้อย = สำคัญมาก = รันก่อนเมื่อชน tick เดียวกัน
    scheduler_add_task(&sched, "led_blink",      task_led_blink,      500, 1);
    scheduler_add_task(&sched, "sensor_read",    task_sensor_read,    100, 2);
    scheduler_add_task(&sched, "status_report",  task_status_report, 1000, 3);
    scheduler_add_task(&sched, "heartbeat",      task_heartbeat,      250, 2);

    printf("Tasks registered:\n");
    for (uint8_t i = 0; i < sched.task_count; i++)
    {
        printf("  - %-14s  period=%4u ms  priority=%u\n",
               sched.tasks[i].name,
               sched.tasks[i].period_ms,
               sched.tasks[i].priority);
    }

    // ----------------------------------------------------------
    // รัน 2 วินาที (2000 tick)
    // ----------------------------------------------------------
    printf("\n---- เริ่มรัน scheduler 2 วินาที ----\n\n");
    scheduler_run(&sched, 2000);

    // ----------------------------------------------------------
    // สาธิต disable / enable task
    // ----------------------------------------------------------
    printf("\n---- Pause sensor_read แล้วรันต่อ 1 วินาที ----\n\n");
    scheduler_disable_task(&sched, "sensor_read");
    scheduler_run(&sched, 1000);

    printf("\n---- Resume sensor_read แล้วรันต่อ 1 วินาที ----\n\n");
    scheduler_enable_task(&sched, "sensor_read");
    scheduler_run(&sched, 1000);

    printf("\n==================================================\n");
    printf("  Simulation complete (current_tick = %u ms)\n",
           sched.current_tick);
    printf("==================================================\n");

    // ----------------------------------------------------------
    // สรุปแนวคิดที่ Level 6 เอามารวมกับทุกอย่างที่ผ่านมา
    // ----------------------------------------------------------
    printf("\nScheduler pattern นี้ใช้ concept จาก:\n");
    printf("  - Level 1 (uint32_t)          — tick counter, period\n");
    printf("  - Level 3 (struct + function) — Task struct ถือ function pointer\n");
    printf("  - Level 4 (fixed array)       — MAX_TASKS = static allocation\n");
    printf("\nใน firmware จริง + ring buffer + FSM + parser = ระบบที่สมบูรณ์\n");

    return 0;
}
