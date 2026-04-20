// ============================================================
// main.c
// จำลองการทำงานของ FSM ไฟจราจร
//
// ใน hardware จริง: loop นี้จะถูกแทนด้วย timer interrupt
// (เช่น ทุก 1 วินาทีเรียก fsm_tick() หนึ่งครั้ง)
// ============================================================

#include <stdio.h>
#include "fsm.h"

#define TOTAL_TICKS 30   // จำลอง 30 tick

int main(void)
{
    printf("==================================================\n");
    printf("  Traffic Light FSM — Level 3 Simulation\n");
    printf("==================================================\n\n");

    printf("ตั้งเวลา: RED=5 tick, GREEN=5 tick, YELLOW=2 tick\n");
    printf("ลำดับ:    RED -> GREEN -> YELLOW -> RED ...\n\n");

    // สร้าง FSM 1 ตัว — เริ่มต้นที่ RED
    FSM traffic_light;

    printf("---- Start (tick 0) ----\n");
    fsm_init(&traffic_light, STATE_RED);

    // จำลอง tick ทีละครั้ง — เหมือน timer interrupt
    for (int t = 1; t <= TOTAL_TICKS; t++)
    {
        printf("[tick %2d] state = %-7s ",
               t,
               fsm_current_name(&traffic_light));

        fsm_tick(&traffic_light);

        // ถ้าไม่มี state transition จะมาตรงนี้พร้อม newline ปกติ
        // ถ้ามี transition, on_enter ได้ print ไปแล้ว
        if (traffic_light.tick_count != 0)
        {
            // ยังอยู่ใน state เดิม
            printf("\n");
        }
        // ถ้า tick_count = 0 หมายถึงพึ่งเข้า state ใหม่
        // → on_enter ใน fsm_tick ได้ print ไปแล้ว ไม่ต้อง print เพิ่ม
    }

    printf("\n==================================================\n");
    printf("  Simulation complete (%d ticks)\n", TOTAL_TICKS);
    printf("==================================================\n");

    // ----------------------------------------------------------
    // สาธิต: สร้าง FSM อีก 1 ตัว — แสดงว่าใช้ตารางเดียวกัน
    // จำลองสี่แยกที่มีไฟ 2 ทิศทาง
    // ----------------------------------------------------------
    printf("\n=== Bonus: create 2 FSM and the same state_table ===\n\n");

    FSM intersection_north;
    FSM intersection_east;

    printf(">>> north:\n");
    fsm_init(&intersection_north, STATE_RED);

    printf(">>> east:\n");
    fsm_init(&intersection_east, STATE_GREEN);  // อีกฝั่งเริ่มที่เขียว

    printf("\n  both machines work independently\n");
    printf("  utilize the same state_table  → to save memory\n");

    return 0;
}
