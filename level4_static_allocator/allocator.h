// ============================================================
// allocator.h
// Public API ของ static memory pool allocator
// ============================================================
//
// ปัญหาที่ต้องการแก้:
// = ใน embedded system เราไม่อยากใช้ malloc() เลย
//
// ทำไม malloc ถึงไม่ดีใน embedded?
//   1. Heap fragmentation — ขอจอง/คืน memory บ่อยๆ ทำให้ heap แตกเป็นเสี่ยงๆ
//      สุดท้ายขอ memory ไม่ได้แม้ว่าจะมี free space รวมพอ
//   2. Non-deterministic — malloc ใช้เวลาไม่แน่นอน (อาจจะนาน)
//      ในระบบ real-time ที่ต้องตอบสนองภายในเวลาที่กำหนด → อันตราย
//   3. ต้องมี OS หรือ runtime ที่จัดการ heap → MCU เล็กๆ ไม่มี
//
// วิธีแก้ของเรา:
// = จองพื้นที่ก้อนใหญ่ก้อนเดียวตอนเริ่มโปรแกรม (static array)
//   แล้วเขียน allocator แบบง่ายๆ มาแบ่งใช้เอง
//
// แบบที่เราจะทำคือ "bump allocator" หรือ "linear allocator":
//   - มี pointer ตัวเดียวที่บอกว่า "ใช้ไปถึงไหนแล้ว"
//   - ขอ memory = เลื่อน pointer ไปข้างหน้า
//   - ไม่มี free ทีละก้อน → ต้อง reset ทั้งก้อนเท่านั้น
//
// ข้อดี: เร็วมาก ปลอดภัย เข้าใจง่าย
// ข้อจำกัด: ไม่เหมาะถ้าต้อง free แบบไม่เป็นลำดับ
//           (สำหรับ scenario นั้นต้องใช้ block allocator แทน)
// ============================================================

#ifndef ALLOCATOR_H
#define ALLOCATOR_H

#include <stdint.h>
#include <stddef.h>   // ดึง size_t มาใช้

#define POOL_SIZE 1024   // ขนาด pool 1KB — สำหรับ MCU ขนาดเล็กก็เหมาะ

// ============================================================
// Pool — โครงสร้างของ allocator
// ============================================================
//
// ทำไม buffer เป็น array (ไม่ใช่ pointer)?
// = เพราะเราต้องการให้ memory ถูกจองเป็น "ของ struct นี้" เลย
//   ไม่ต้องไปขอจาก heap อีก
//   ถ้าประกาศ Pool เป็น global / static → memory จะถูกจองตอนโหลดโปรแกรม
//   ใน MCU จะอยู่ในส่วน .bss section (RAM ที่ถูก initialize เป็น 0)
//
// ทำไม buffer เป็น uint8_t (ไม่ใช่ int หรือ char)?
// = เพราะเราต้องการมองเป็น "ก้อน byte" — ไม่สนใจว่าผู้ใช้จะใช้เก็บอะไร
//   uint8_t = byte ตรงๆ ขนาดชัดเจน
typedef struct
{
    uint8_t buffer[POOL_SIZE];   // พื้นที่จริงสำหรับเก็บข้อมูล
    size_t  used;                 // ใช้ไปกี่ byte แล้ว (offset จาก buffer[0])
    size_t  capacity;             // ขนาดทั้งหมด (สำคัญถ้าทำหลาย pool)
} Pool;

// ============================================================
// API
// ============================================================

// เริ่มต้น pool — ตั้ง used = 0, capacity = POOL_SIZE
void pool_init(Pool *p);

// ขอ memory ขนาด size byte
// คืน:
//   - pointer ไปยัง memory ที่จอง (ใช้ได้จริง)
//   - NULL ถ้า pool เต็ม
//
// ทำไมคืนเป็น void* (ไม่ใช่ uint8_t* หรืออื่นๆ)?
// = เพราะ allocator ไม่รู้ว่าผู้ใช้จะเอาไปเก็บ type อะไร
//   void* คือ "pointer ไปอะไรก็ได้" ผู้ใช้ cast เอาเอง
//   นี่คือเหตุผลที่ malloc() ก็คืน void* เช่นกัน
void *pool_alloc(Pool *p, size_t size);

// รีเซ็ต pool ให้กลับมาว่าง
// ทุกอย่างที่จองไปก่อนหน้า "หายไป" หมด
//
// หมายเหตุ: ไม่ต้องเคลียร์ buffer[] เพราะ pool_alloc รอบใหม่
//          จะเขียนทับเอง (เร็วกว่ามาก)
void pool_reset(Pool *p);

// ดูสถานะ
size_t pool_used(const Pool *p);   // ใช้ไปแล้วกี่ byte
size_t pool_free(const Pool *p);   // เหลืออีกกี่ byte

#endif // ALLOCATOR_H
