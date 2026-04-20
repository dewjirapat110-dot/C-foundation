// ============================================================
// scheduler.h
// Public API ของ cooperative task scheduler
// ============================================================
//
// Cooperative scheduler คืออะไร?
// = ระบบที่รัน "หลาย task" บน CPU ตัวเดียว โดยไม่มี OS
//   แต่ละ task เป็นฟังก์ชันธรรมดาที่รันแล้วจบ (ไม่ blocking)
//   scheduler มีหน้าที่ "เรียก task ที่ถึงเวลา" ในทุก tick
//
// Cooperative vs Preemptive:
//   - Cooperative (ของเรา): task จบเองเสมอ scheduler รอ
//     → เรียบง่าย predictable ใช้ memory น้อย
//     → แต่ task ที่ใช้เวลานานจะหน่วงทุกอย่าง
//   - Preemptive (FreeRTOS, Linux): OS สามารถ "หยุด" task กลางคันได้
//     → ซับซ้อนกว่า ต้องใช้ hardware timer + context switch
//     → แต่รองรับ real-time deadline ได้ดีกว่า
//
// ใน MCU ขนาดเล็ก cooperative scheduler คือ pattern ยอดนิยมมาก
// เพราะ:
//   1. ไม่ต้องพึ่ง RTOS → ประหยัด flash + RAM
//   2. Debug ง่ายกว่ามาก (ไม่มี context switch แบบ unpredictable)
//   3. เพียงพอกับงาน sensor / communication / state machine ส่วนใหญ่
//
// ------------------------------------------------------------
// แนวคิดการทำงาน:
//
//   tick counter เพิ่มทุก 1ms (หรือตามที่ตั้ง)
//   ทุก tick scheduler ตรวจทุก task:
//     ถ้า (current_tick - last_run) >= period → ถึงเวลารัน
//
//   เรียงรัน task ตาม priority (เลขน้อย = สำคัญมาก)
//
//   แต่ละ task ต้อง "สั้นพอ" ไม่ hog CPU
// ============================================================

#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <stdint.h>
#include <stdbool.h>

#define MAX_TASKS 8    // จำนวน task สูงสุดที่ scheduler รองรับ

// ============================================================
// TaskFn — type ของฟังก์ชัน task
// ============================================================
//
// typedef function pointer เพื่อให้อ่าน struct ง่ายขึ้น
// แทนที่จะเขียน  void (*function)(void);  ทุกที่
// เราเขียน  TaskFn function;
//
// ฟังก์ชัน task ต้องเป็น void(void) — ไม่รับ arg และไม่คืนค่า
// ถ้าต้องการ state ภายใน → ใช้ static variable ในฟังก์ชัน
typedef void (*TaskFn)(void);

// ============================================================
// Task — ข้อมูลของ task 1 ตัว
// ============================================================
typedef struct
{
    TaskFn   function;       // ฟังก์ชันที่จะเรียก
    uint32_t period_ms;      // ห่างกันกี่ ms ต่อรอบ
    uint32_t last_run_ms;    // รันล่าสุดเมื่อ tick ไหน
    uint8_t  priority;       // ตัวเลขน้อย = สำคัญมาก (รันก่อน)
    bool     enabled;        // true = active, false = pause ชั่วคราว
    const char *name;        // ชื่อสำหรับ debug print
} Task;

// ============================================================
// Scheduler — ตัว scheduler (ห่อ task ทุกตัวไว้)
// ============================================================
typedef struct
{
    Task     tasks[MAX_TASKS];
    uint8_t  task_count;
    uint32_t current_tick;    // นาฬิกาของระบบ (นับ ms)
} Scheduler;

// ============================================================
// API
// ============================================================

// เริ่มต้น scheduler ให้ว่าง
void scheduler_init(Scheduler *s);

// ลงทะเบียน task ใหม่
// คืน true ถ้าสำเร็จ, false ถ้าเต็ม (เกิน MAX_TASKS)
bool scheduler_add_task(Scheduler *s,
                        const char *name,
                        TaskFn fn,
                        uint32_t period_ms,
                        uint8_t priority);

// 1 tick ของระบบ — scheduler ตรวจและรัน task ที่ถึงเวลา
// ควรถูกเรียกทุก 1 ms (จาก timer interrupt)
void scheduler_tick(Scheduler *s);

// รัน scheduler ให้ "เดินเครื่อง" เป็นเวลา duration_ms ms
// = จำลอง main loop ของ firmware
void scheduler_run(Scheduler *s, uint32_t duration_ms);

// เปิด/ปิด task ที่ลงทะเบียนไว้
void scheduler_enable_task (Scheduler *s, const char *name);
void scheduler_disable_task(Scheduler *s, const char *name);

#endif // SCHEDULER_H
