// ============================================================
// scheduler.c
// Implementation ของ cooperative task scheduler
// ============================================================

#include <string.h>
#include "scheduler.h"

// ============================================================
// scheduler_init — เริ่มต้น scheduler
// ============================================================
void scheduler_init(Scheduler *s)
{
    s->task_count   = 0;
    s->current_tick = 0;

    // ไม่จำเป็นต้องเคลียร์ tasks[] — task_count = 0 บอกว่ายังไม่มี task
    // (ใช้ memset ก็ได้ถ้าอยากให้ชัดเจน)
}

// ============================================================
// scheduler_add_task — ลงทะเบียน task ใหม่
// ============================================================
bool scheduler_add_task(Scheduler *s,
                        const char *name,
                        TaskFn fn,
                        uint32_t period_ms,
                        uint8_t priority)
{
    // ตรวจว่าเต็มหรือยัง
    if (s->task_count >= MAX_TASKS)
    {
        return false;
    }

    // ตรวจ argument ที่ไม่ valid
    if (fn == NULL || period_ms == 0)
    {
        return false;
    }

    // ลงทะเบียน task ลงในช่องถัดไป
    Task *t = &s->tasks[s->task_count];
    t->function    = fn;
    t->period_ms   = period_ms;
    t->last_run_ms = 0;         // 0 = ยังไม่เคยรัน → จะรันครั้งแรกทันที
    t->priority    = priority;
    t->enabled     = true;
    t->name        = name;

    s->task_count++;
    return true;
}

// ============================================================
// scheduler_tick — หัวใจของ scheduler
// ============================================================
//
// ทุก tick (เรียกจาก timer interrupt หรือ simulation loop):
//   1. เพิ่ม current_tick
//   2. หา task ทั้งหมดที่ "ถึงเวลา" แล้ว
//   3. เรียงรันตาม priority (น้อยมาก่อน)
//
// การเลือกลำดับรันทำแบบง่าย: วน loop ทุก task ซ้ำๆ ตาม priority level
// ถ้า task เยอะมากๆ ควร sort ล่วงหน้า (ตอน add_task) เพื่อประหยัดเวลา
void scheduler_tick(Scheduler *s)
{
    s->current_tick++;

    // ค้นหา priority ต่ำสุด (สำคัญสุด) ที่มีในตอนนี้
    // และรัน task ที่ priority นั้นก่อน ตามด้วย level ถัดไป
    //
    // algorithm ง่ายๆ: วน priority จาก 0 ถึง 255
    // สำหรับแต่ละ level → หา task ที่ priority ตรงและถึงเวลา
    //
    // O(256 × N) — ยอมรับได้เมื่อ N น้อย (MAX_TASKS = 8)
    // ถ้า N เยอะ → ใช้ priority queue แทน
    for (int prio = 0; prio < 256; prio++)
    {
        for (uint8_t i = 0; i < s->task_count; i++)
        {
            Task *t = &s->tasks[i];

            // ข้ามถ้าถูก disable
            if (!t->enabled)
            {
                continue;
            }

            // ข้ามถ้า priority ไม่ตรง (รอบนี้)
            if (t->priority != prio)
            {
                continue;
            }

            // ถึงเวลารันหรือยัง?
            //
            // สำคัญ: ไม่ใช้  current_tick % period == 0
            // เพราะถ้า scheduler "ช้า" ไปหลาย tick → จะพลาดรอบ
            //
            // ใช้วิธี "นับระยะห่างจากครั้งล่าสุด" แทน
            // → ถ้า lag → รันทันทีเมื่อมีโอกาส (ปลอดภัยกว่า)
            uint32_t elapsed = s->current_tick - t->last_run_ms;
            if (elapsed >= t->period_ms)
            {
                t->last_run_ms = s->current_tick;
                t->function();
            }
        }
    }
}

// ============================================================
// scheduler_run — จำลอง main loop ของ firmware
// ============================================================
//
// ใน hardware จริง:
//   - timer interrupt ยิงทุก 1ms → เรียก scheduler_tick
//   - main loop แค่ while(1) {} — หรือ sleep รอ interrupt
//
// ในตัวจำลองนี้:
//   - ใช้ loop ธรรมดาแทน → เรียก tick ตาม duration ที่ต้องการ
void scheduler_run(Scheduler *s, uint32_t duration_ms)
{
    for (uint32_t i = 0; i < duration_ms; i++)
    {
        scheduler_tick(s);
    }
}

// ============================================================
// enable/disable task โดยหาตามชื่อ
// ============================================================
void scheduler_enable_task(Scheduler *s, const char *name)
{
    for (uint8_t i = 0; i < s->task_count; i++)
    {
        if (strcmp(s->tasks[i].name, name) == 0)
        {
            s->tasks[i].enabled = true;
            return;
        }
    }
}

void scheduler_disable_task(Scheduler *s, const char *name)
{
    for (uint8_t i = 0; i < s->task_count; i++)
    {
        if (strcmp(s->tasks[i].name, name) == 0)
        {
            s->tasks[i].enabled = false;
            return;
        }
    }
}
