// ============================================================
// ring_buffer.c
// Implementation ของ ring buffer ที่ประกาศไว้ใน ring_buffer.h
// ============================================================

#include "ring_buffer.h"

// ============================================================
// rb_init — รีเซ็ต buffer ให้ว่างเปล่า
// ============================================================
//
// ตั้ง head, tail, count = 0 ทั้งหมด
// เป็น "สถานะเริ่มต้น" ที่ฟังก์ชันอื่นๆ คาดหวัง
//
// ไม่ต้องเคลียร์ buffer[] เพราะข้อมูลที่ยังไม่ได้เขียน
// ถูกถือว่า "ไม่มี" จาก count = 0 อยู่แล้ว
void rb_init(RingBuffer *rb)
{
    rb->head  = 0;
    rb->tail  = 0;
    rb->count = 0;
}

// ============================================================
// rb_write — เขียน 1 byte ลง buffer
// ============================================================
//
// ขั้นตอน:
//   1. เช็คว่าเต็มไหม → ถ้าเต็ม return false
//   2. เขียน byte ลงตำแหน่ง tail
//   3. เลื่อน tail ไป 1 ตำแหน่ง (ถ้าถึงปลายให้กลับไป 0)
//   4. เพิ่ม count
//
// ภาพ:
//   ก่อน:  [A][B][_][_][_]   tail=2, count=2
//   write(C):
//   หลัง:  [A][B][C][_][_]   tail=3, count=3
bool rb_write(RingBuffer *rb, uint8_t byte)
{
    // เช็คเต็ม — ป้องกัน overwrite ข้อมูลที่ยังอ่านไม่ครบ
    if (rb_is_full(rb))
    {
        return false;  // overrun
    }

    // เขียนลงตำแหน่งปัจจุบันของ tail
    rb->buffer[rb->tail] = byte;

    // เลื่อน tail ไปข้างหน้า 1 ช่อง — ถ้าถึงปลาย ให้วนกลับ 0
    //
    // ใช้ modulo (%) เพื่อทำการ "wrap-around"
    // เช่น size = 8, tail = 7 → (7+1) % 8 = 0
    //
    // ถ้า RING_BUFFER_SIZE เป็น power of 2 compiler อาจ optimize
    // เป็น (tail + 1) & (RING_BUFFER_SIZE - 1) ให้เอง — เร็วกว่ามาก
    rb->tail = (rb->tail + 1) % RING_BUFFER_SIZE;

    rb->count++;

    return true;
}

// ============================================================
// rb_read — อ่าน 1 byte ออกจาก buffer
// ============================================================
//
// ขั้นตอน:
//   1. เช็คว่าว่างไหม → ถ้าว่าง return false
//   2. อ่าน byte จากตำแหน่ง head ใส่ใน *out_byte
//   3. เลื่อน head ไป 1 ตำแหน่ง (ถ้าถึงปลายให้กลับไป 0)
//   4. ลด count
//
// สังเกตว่า "อ่าน" ไม่ได้ลบข้อมูลออก — แค่เลื่อน head ผ่านเฉยๆ
// ข้อมูลเก่ายังอยู่ใน array แต่จะถูก write ใหม่ทับไปเอง
bool rb_read(RingBuffer *rb, uint8_t *out_byte)
{
    if (rb_is_empty(rb))
    {
        return false;  // underrun
    }

    *out_byte = rb->buffer[rb->head];
    rb->head  = (rb->head + 1) % RING_BUFFER_SIZE;
    rb->count--;

    return true;
}

// ============================================================
// Helper functions — เช็คสถานะ
// ============================================================
//
// ใช้ count แทนการเปรียบเทียบ head กับ tail โดยตรง
// เพราะถ้าใช้แค่ head/tail จะแยกไม่ออกระหว่าง "ว่าง" กับ "เต็ม"
// (ทั้งสองสถานะ head == tail เหมือนกัน)
//
// มีอีกวิธีคือใช้ "phantom slot" — เสีย 1 ช่องเพื่อแยก 2 สถานะ
// แต่การมี count ตรงๆ อ่านง่ายและชัดเจนกว่า

bool rb_is_empty(const RingBuffer *rb)
{
    return rb->count == 0;
}

bool rb_is_full(const RingBuffer *rb)
{
    return rb->count == RING_BUFFER_SIZE;
}

uint16_t rb_count(const RingBuffer *rb)
{
    return rb->count;
}

uint16_t rb_free(const RingBuffer *rb)
{
    return RING_BUFFER_SIZE - rb->count;
}
