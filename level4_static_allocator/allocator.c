// ============================================================
// allocator.c
// Implementation ของ static memory pool allocator
// ============================================================

#include "allocator.h"

// ============================================================
// Alignment — เรื่องที่สำคัญมากใน embedded แต่หลายคนมองข้าม
// ============================================================
//
// alignment คืออะไร?
// = "address ที่ใช้เก็บข้อมูล type นี้ ต้องหารด้วยจำนวนนี้ลงตัว"
//
// ตัวอย่าง:
//   uint8_t   ต้อง align 1 byte  (ทุก address ใช้ได้)
//   uint16_t  ต้อง align 2 bytes (address ต้องเป็นเลขคู่)
//   uint32_t  ต้อง align 4 bytes (address ต้องลงตัวด้วย 4)
//
// ทำไมต้อง align?
// = CPU บาง architecture (เช่น ARM Cortex-M0) จะ "crash" ถ้าอ่าน
//   uint32_t จาก address ที่ไม่ลงตัวด้วย 4
//   = เกิด HardFault — debug ยากมาก
//
//   ส่วน CPU บางตัว (x86) อ่านได้ แต่ "ช้ากว่า" เพราะต้องอ่าน 2 ครั้ง
//   = performance ลดลง
//
// วิธีคำนวณ aligned size:
//   aligned = (size + (align - 1)) & ~(align - 1)
//
//   ตัวอย่าง: align = 4
//     size = 1 → aligned = (1 + 3) & ~3 = 4  ← ปัดขึ้นเป็นก้อนละ 4
//     size = 5 → aligned = (5 + 3) & ~3 = 8
//     size = 8 → aligned = (8 + 3) & ~3 = 8  ← พอดีแล้ว
//
// เราใช้ alignment = 4 byte เพราะเป็นค่าที่ปลอดภัยสำหรับ MCU 32-bit ส่วนใหญ่
// (ถ้า portable มาก ๆ ใช้ alignof(max_align_t) แต่ที่นี่ทำง่ายๆ)
#define ALIGNMENT 4

// ============================================================
// pool_init — เริ่มต้น pool
// ============================================================
void pool_init(Pool *p)
{
    p->used     = 0;
    p->capacity = POOL_SIZE;

    // ไม่ต้องเคลียร์ buffer[] — เพราะ used = 0 หมายถึง "ยังไม่มีข้อมูล"
    // และ pool_alloc รอบใหม่จะเขียนทับเอง
    //
    // ใน hardware จริง buffer[] เป็น static array
    // จะถูก initialize เป็น 0 อัตโนมัติตอนโหลดโปรแกรม (.bss section)
}

// ============================================================
// pool_alloc — จอง memory ขนาด size byte
// ============================================================
//
// ขั้นตอน:
//   1. ปัด size ขึ้นเป็น aligned size
//   2. เช็คว่าพื้นที่เหลือพอไหม (ใช้ aligned size — ไม่ใช่ size!)
//   3. คำนวณ pointer ของก้อน memory ที่จะคืน (= base + used)
//   4. เลื่อน used ไปข้างหน้าตาม aligned size
//   5. คืน pointer
//
// ภาพ pool ก่อน-หลัง alloc:
//   ก่อน:   [████████____________]   used = 8
//                    ^
//                    used pointer
//
//   alloc(5) → aligned = 8
//   หลัง:   [████████████████____]   used = 16
//                    ^       ^
//                    return  used pointer ใหม่
//                    pointer
void *pool_alloc(Pool *p, size_t size)
{
    // ขอ size = 0 → คืน NULL (ไม่มีอะไรให้จอง)
    // ป้องกันพฤติกรรมแปลกๆ
    if (size == 0)
    {
        return NULL;
    }

    // ปัดขึ้นให้เป็นทวีคูณของ ALIGNMENT
    //
    // (size + ALIGNMENT - 1) → ทำให้ตัวเลขโดดข้าม "ขั้น" ถัดไป
    // & ~(ALIGNMENT - 1)     → ปัดลงให้ลงตัวด้วย ALIGNMENT
    //
    // ตัวอย่าง ALIGNMENT = 4:
    //   ~(4-1) = ~3 = ...11111100  ← mask ที่ทำให้ 2 bit ล่างเป็น 0
    //
    //   size=1: (1+3) & ~3 = 4 & ~3 = 4    ✓
    //   size=5: (5+3) & ~3 = 8 & ~3 = 8    ✓
    //   size=8: (8+3) & ~3 = 11 & ~3 = 8   ✓
    size_t aligned_size = (size + ALIGNMENT - 1) & ~(ALIGNMENT - 1);

    // เช็คว่าพื้นที่เหลือพอ
    // สำคัญ: ใช้ aligned_size ไม่ใช่ size
    // ไม่งั้นจะเช็คผิด แล้วเขียนเกินขอบ buffer
    if (p->used + aligned_size > p->capacity)
    {
        return NULL;   // out of memory
    }

    // คำนวณ pointer ของก้อนใหม่
    //
    // p->buffer คือ pointer ไปต้น array (== &buffer[0])
    // บวก used = pointer ไปยังตำแหน่งว่างถัดไป
    //
    // pointer arithmetic บน uint8_t* → บวกเป็น byte ตรงๆ
    // (ถ้าเป็น uint32_t* จะบวกทีละ 4 byte อัตโนมัติ)
    void *ptr = p->buffer + p->used;

    // เลื่อน used ไปข้างหน้า
    p->used += aligned_size;

    return ptr;
}

// ============================================================
// pool_reset — รีเซ็ต pool ให้ว่าง
// ============================================================
//
// ใช้ตอนไหน?
// = ตอนเริ่มรอบงานใหม่ที่ไม่ต้องเก็บของเดิมไว้แล้ว
//   เช่น ใน game frame loop, parse packet ใหม่, หรือ reset ระบบ
//
// หมายเหตุ: เมื่อเรียก pool_reset แล้ว pointer ที่จองไปก่อนหน้านี้
//          ทั้งหมดจะ "ใช้ไม่ได้" อีกต่อไป (dangling pointer)
//          ผู้ใช้ต้องระวัง
void pool_reset(Pool *p)
{
    p->used = 0;
}

// ============================================================
// Status query
// ============================================================

size_t pool_used(const Pool *p)
{
    return p->used;
}

size_t pool_free(const Pool *p)
{
    return p->capacity - p->used;
}
