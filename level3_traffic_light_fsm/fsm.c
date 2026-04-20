// ============================================================
// fsm.c
// Implementation ของ FSM engine + state table
// ============================================================

#include <stdio.h>
#include "fsm.h"

// ============================================================
// on_enter callbacks — รันทุกครั้งที่เข้า state ใหม่
// ============================================================
//
// ทำเป็น static เพราะเป็น "private" ใช้แค่ในไฟล์นี้
// ไม่ต้องการให้คนอื่นเรียกตรงๆ — ต้องผ่าน FSM engine เท่านั้น
//
// ในงานจริง ฟังก์ชันพวกนี้จะไป control hardware
// เช่น เปิด LED แดง, สั่ง motor, ส่งสัญญาณ ฯลฯ
// ในตัวอย่างนี้แค่ print ออกมาให้เห็นภาพ

static void on_enter_red(void)
{
    printf("    [Hardware] >>> LED แดง  ON  | LED เขียว OFF | LED เหลือง OFF\n");
}

static void on_enter_green(void)
{
    printf("    [Hardware] >>> LED แดง  OFF | LED เขียว ON  | LED เหลือง OFF\n");
}

static void on_enter_yellow(void)
{
    printf("    [Hardware] >>> LED แดง  OFF | LED เขียว OFF | LED เหลือง ON\n");
}

// ============================================================
// State table — หัวใจของ pattern นี้
// ============================================================
//
// ใช้ "designated initializer" — สังเกตวงเล็บใน [STATE_RED] = {...}
//
// ทำไมถึงดีกว่าเขียนเรียง index?
//
//   แบบธรรมดา (เสี่ยง):
//     {"RED", ...}, {"GREEN", ...}, {"YELLOW", ...}
//     → ถ้าวันหนึ่งเพิ่ม STATE_BLINK ตรงกลาง enum
//       ลำดับใน array จะ "เลื่อน" ทำให้ STATE_GREEN ชี้ผิดข้อมูล
//
//   แบบ designated initializer (ปลอดภัย):
//     [STATE_RED] = {...}
//     → compiler จับคู่ index ให้เอง
//       เพิ่ม state ใหม่ตรงไหนก็ได้ ไม่กระทบของเดิม
//
// const บอก compiler ว่าตารางนี้ไม่เปลี่ยนค่า
// → compiler อาจวางลง flash แทน RAM ใน MCU จริง
//   ช่วยประหยัด RAM ที่มีจำกัด
static const StateConfig state_table[STATE_COUNT] =
{
    [STATE_RED] = {
        .name       = "RED",
        .duration   = 5,                 // อยู่แดง 5 tick
        .on_enter   = on_enter_red,      // ← เก็บ "ที่อยู่ของฟังก์ชัน"
        .next_state = STATE_GREEN
    },
    [STATE_GREEN] = {
        .name       = "GREEN",
        .duration   = 5,
        .on_enter   = on_enter_green,
        .next_state = STATE_YELLOW
    },
    [STATE_YELLOW] = {
        .name       = "YELLOW",
        .duration   = 2,                 // เหลืองสั้นกว่า เป็น "transition"
        .on_enter   = on_enter_yellow,
        .next_state = STATE_RED
    }
};

// ============================================================
// fsm_init — เริ่มต้น FSM
// ============================================================
//
// 1. ตั้งสถานะปัจจุบัน
// 2. รีเซ็ต tick counter
// 3. เรียก on_enter ของ state เริ่มต้น (สำคัญ! ไม่งั้น hardware จะไม่ถูกเซ็ตค่า)
void fsm_init(FSM *fsm, TrafficState initial_state)
{
    fsm->current_state = initial_state;
    fsm->tick_count    = 0;

    // เรียก on_enter ของ state เริ่มต้น
    // สังเกตว่าเราไม่รู้ว่ามันคือ on_enter_red หรือ on_enter_green
    // engine แค่ตามไปเรียกที่ pointer ชี้อยู่
    state_table[initial_state].on_enter();
}

// ============================================================
// fsm_tick — engine ของ FSM
// ============================================================
//
// ความสวยงามอยู่ที่นี่: ฟังก์ชันนี้ไม่มี if-else เกี่ยวกับ state เลย
// ไม่ว่าจะมี state กี่ตัว (3, 30, 300) โค้ดนี้ก็เท่าเดิม
//
// ทุก logic ถูกย้ายไปอยู่ใน state_table แทน
// → เพิ่ม state ใหม่ = แค่เพิ่มแถวในตาราง ไม่ต้องแก้ engine เลย
void fsm_tick(FSM *fsm)
{
    // ดึง config ของ state ปัจจุบันจากตาราง
    // ใช้ pointer เพื่อหลีกเลี่ยงการ copy struct ทั้งก้อน
    const StateConfig *cfg = &state_table[fsm->current_state];

    fsm->tick_count++;

    // ถึงเวลาเปลี่ยน state หรือยัง?
    if (fsm->tick_count >= cfg->duration)
    {
        fsm->tick_count    = 0;                      // รีเซ็ต counter
        fsm->current_state = cfg->next_state;        // กระโดดไป state ใหม่

        // เรียก on_enter ของ state ใหม่
        // engine ไม่รู้ว่าเรียกฟังก์ชันอะไร — แค่ตามไปที่ pointer
        state_table[fsm->current_state].on_enter();
    }
}

// ============================================================
// fsm_current_name — สำหรับ debug
// ============================================================
const char *fsm_current_name(const FSM *fsm)
{
    return state_table[fsm->current_state].name;
}
