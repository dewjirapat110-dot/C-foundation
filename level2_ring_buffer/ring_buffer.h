// ============================================================
// ring_buffer.h
// Public API ของ circular FIFO buffer
// ============================================================
//
// Ring buffer คืออะไร?
// = "ห้องพักคอย" สำหรับเก็บข้อมูลที่มาเร็วกว่าที่เราจะ process ได้ทัน
//
// ภาพในใจ: นึกถึง CD ที่เล่นวนซ้ำ — ถึงปลายแล้วกลับไปต้นใหม่
// แทนที่จะใช้ array ตรงๆ ที่เต็มแล้วใช้ต่อไม่ได้
//
// ใช้ทำอะไรในงาน firmware จริง?
// = บัฟเฟอร์ระหว่าง interrupt (ที่รับข้อมูลจาก UART/SPI/I2C)
//   กับ main loop (ที่ process ข้อมูล)
//
// ทำไมต้องเป็น ring (ไม่ใช่ array ธรรมดา)?
// = เพื่อใช้ memory คงที่ ไม่ต้อง malloc/free
//   และไม่มี "พื้นที่หมด" ตราบใดที่ producer ไม่เร็วกว่า consumer
// ============================================================

#ifndef RING_BUFFER_H
#define RING_BUFFER_H

#include <stdint.h>
#include <stdbool.h>
// ↑ stdbool.h ให้ type bool และค่า true/false (มาตั้งแต่ C99)

// ขนาดของ buffer — กำหนดตอน compile time
// ใน firmware จริงมักเลือกเป็น power of 2 (8, 16, 32, 64...)
// เพราะถ้าเป็น power of 2 การทำ modulo (%) สามารถ optimize เป็น AND ได้
// (size & (mask)) เร็วกว่า (size % capacity) มาก
#define RING_BUFFER_SIZE 16

// ============================================================
// โครงสร้างของ ring buffer
// ============================================================
//
// ใช้ "head" และ "tail" สองตัวบอกตำแหน่ง:
//   - tail (write index) ชี้ตำแหน่งที่ "จะเขียน byte ถัดไป"
//   - head (read index)  ชี้ตำแหน่งที่ "จะอ่าน byte ถัดไป"
//
// ภาพ buffer ขนาด 8:
//   index:  [0][1][2][3][4][5][6][7]
//   data:   [A][B][C][D][_][_][_][_]
//                         ^head        ← ผิด! head อยู่ที่ A
//                                        แก้: head=0, tail=4
//   จริงๆ:  head=0 (ชี้ที่ A), tail=4 (ชี้ที่ช่องว่างถัดไป)
//
// volatile บอก compiler ว่า:
//   "ค่าตัวแปรนี้อาจถูกเปลี่ยนจากภายนอก (interrupt) ได้ตลอดเวลา"
//   ห้าม optimize ด้วยการเก็บไว้ใน register หรือสมมติว่าค่าไม่เปลี่ยน
//   ต้องอ่านจาก memory ใหม่ทุกครั้ง
//
// ทำไมต้อง volatile?
// = เพราะใน firmware จริง interrupt handler (ISR) จะมาแก้ค่า tail
//   ในขณะที่ main loop กำลังอ่าน head อยู่
//   ถ้าไม่ใส่ volatile, compiler อาจ cache ค่าไว้ใน CPU register
//   แล้วไม่เห็นว่ามันเปลี่ยน — เกิด bug ที่ debug ยากมาก
typedef struct
{
    volatile uint8_t  buffer[RING_BUFFER_SIZE];  // พื้นที่เก็บข้อมูลจริง (static array)
    volatile uint16_t head;                       // index สำหรับอ่าน
    volatile uint16_t tail;                       // index สำหรับเขียน
    volatile uint16_t count;                      // จำนวน byte ที่อยู่ใน buffer
} RingBuffer;

// ============================================================
// API
// ============================================================

// เริ่มต้น buffer ให้ว่างเปล่า
// ต้องเรียกก่อนใช้งานทุกครั้ง — ไม่งั้นค่าใน head/tail/count จะเป็น garbage
void rb_init(RingBuffer *rb);

// เขียน 1 byte ลง buffer
// คืน true ถ้าสำเร็จ, false ถ้า buffer เต็ม (overrun)
//
// ทำไมไม่ overwrite ของเก่า?
// = เพราะนั่นจะทำให้ข้อมูลที่ยังไม่ได้อ่านหายไปเงียบๆ
//   ในงาน firmware เราอยากรู้เลยว่า buffer overrun (จะได้แก้ไข)
bool rb_write(RingBuffer *rb, uint8_t byte);

// อ่าน 1 byte ออกจาก buffer
// เก็บค่าไว้ใน *out_byte
// คืน true ถ้าสำเร็จ, false ถ้า buffer ว่าง (underrun)
bool rb_read(RingBuffer *rb, uint8_t *out_byte);

// เช็คสถานะ
bool     rb_is_empty(const RingBuffer *rb);
bool     rb_is_full (const RingBuffer *rb);
uint16_t rb_count   (const RingBuffer *rb);  // มี byte อยู่กี่ตัว
uint16_t rb_free    (const RingBuffer *rb);  // ยังเขียนได้อีกกี่ตัว

#endif // RING_BUFFER_H
