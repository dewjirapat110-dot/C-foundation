# Level 2 — ring_buffer

> 🟡 **Data Structure ที่ใช้จริงใน Firmware**
> หัวใจของ serial communication ทุกระบบ

---

## ทำไมต้องเรียนเรื่องนี้

ใน firmware คุณจะเจอสถานการณ์แบบนี้บ่อยมาก:

```
Hardware (UART/SPI/I2C) ส่งข้อมูลมา "เร็วมาก" ทาง interrupt
     ↓
Main loop ต้อง process ข้อมูล "ช้ากว่า" (ต้อง parse, ทำ logic ฯลฯ)
```

ถ้าไม่มีที่พัก byte ตรงกลาง → **data loss** ไปทันที

**Ring buffer คือห้องพักคอยตรงนั้น**

---

## Ring buffer คืออะไร

Fixed-size FIFO (First In, First Out) ที่ **วนซ้ำได้** — เหมือนเข็มนาฬิกาที่ครบรอบแล้วกลับมาเริ่มใหม่

```
Index:   [ 0 ][ 1 ][ 2 ][ 3 ][ 4 ][ 5 ][ 6 ][ 7 ]
Data:    [ A ][ B ][ C ][ D ][   ][   ][   ][   ]
           ↑                   ↑
         head                tail
         (read next)         (write next)
```

- เขียน → ใส่ที่ `tail`, แล้วเลื่อน tail ไปข้างหน้า
- อ่าน → หยิบจาก `head`, แล้วเลื่อน head ไปข้างหน้า
- ถ้าเลื่อนถึงปลาย → กลับไปที่ 0 (**wrap around**)

---

## สิ่งที่โปรเจ็คนี้ทำ

Library ที่ให้ ring buffer ขนาด 16 byte พร้อม API ครบชุด

| ฟังก์ชัน | ทำอะไร |
|--------|-------|
| `rb_init(rb)`            | เริ่มต้น buffer ให้ว่าง |
| `rb_write(rb, byte)`     | เขียน 1 byte (คืน false ถ้าเต็ม) |
| `rb_read(rb, &byte)`     | อ่าน 1 byte (คืน false ถ้าว่าง) |
| `rb_is_empty(rb)`        | เช็คว่าง |
| `rb_is_full(rb)`         | เช็คเต็ม |
| `rb_count(rb)`           | มีกี่ byte อยู่ใน buffer |
| `rb_free(rb)`            | เขียนได้อีกกี่ byte |

---

## โครงสร้างไฟล์

```
level2_ring_buffer/
├── ring_buffer.h
├── ring_buffer.c
└── main.c
```

---

## Concept ที่ได้ฝึก

### `volatile` — keyword ที่สำคัญมากใน embedded

```c
typedef struct {
    volatile uint8_t  buffer[RING_BUFFER_SIZE];
    volatile uint16_t head;
    volatile uint16_t tail;
    volatile uint16_t count;
} RingBuffer;
```

`volatile` บอก compiler ว่า:

> "ตัวแปรนี้อาจถูกเปลี่ยนจากที่อื่น (เช่น interrupt) — ห้าม optimize"

ถ้าไม่มี `volatile` compiler อาจจะ:
- เก็บค่าไว้ใน CPU register และไม่ reload จาก RAM
- สมมติว่าค่าไม่เปลี่ยนในบางสถานการณ์
- ลบการอ่าน/เขียนที่ดู "ไม่จำเป็น" ออก

→ ใน **single-thread PC** ไม่มีปัญหา
→ ใน **interrupt-driven firmware** = bug ที่ debug ยากที่สุดในโลก

### Modulo สำหรับ wrap-around

```c
rb->tail = (rb->tail + 1) % RING_BUFFER_SIZE;
```

ตรงนี้คือที่ "ring" ของ ring buffer เกิดขึ้น
- tail = 14 → 15
- tail = 15 → 16 → **กลับเป็น 0**

💡 **เคล็ดลับ performance:** ถ้า `RING_BUFFER_SIZE` เป็น **power of 2** (16, 32, 64…) compiler จะ optimize `%` เป็น `&` ที่เร็วกว่ามาก

### แยก "ว่าง" กับ "เต็ม" ด้วย `count`

ถ้าใช้แค่ head/tail:
- `head == tail` → อาจหมายถึง "ว่าง" **หรือ** "เต็ม" — แยกไม่ออก

วิธีแก้:
1. **เก็บ count ด้วย** (วิธีของเรา) — อ่านง่าย
2. **เสีย 1 ช่อง** (phantom slot) — เต็ม = `(tail+1) % size == head`

---

## วิธี Build และ Run

```powershell
gcc -Wall -Wextra ring_buffer.c main.c -o ring_buffer
./ring_buffer
```

---

## เชื่อมโยงกับงานจริง

ใน firmware จริง (เช่น STM32) หน้าตาแบบนี้:

```c
// Global ring buffer สำหรับ UART1 receive
RingBuffer uart1_rx;

// Interrupt handler — ถูกเรียกอัตโนมัติทุกครั้งที่ UART รับ byte
void USART1_IRQHandler(void) {
    if (USART1->SR & USART_SR_RXNE) {           // มี byte พร้อมอ่าน
        uint8_t byte = USART1->DR;              // อ่านจาก hardware register
        rb_write(&uart1_rx, byte);              // เก็บใส่ ring buffer ← Level 2!
    }
}

// Main loop
int main(void) {
    rb_init(&uart1_rx);
    init_uart();

    while (1) {
        uint8_t byte;
        if (rb_read(&uart1_rx, &byte)) {        // มีข้อมูลรอ process
            process_byte(byte);                 // parse, validate, act
        }

        // ทำงานอื่นๆ ต่อได้เลย — ไม่ต้องรอ UART
    }
}
```

**Producer** (ISR) กับ **Consumer** (main loop) ทำงานคนละความเร็ว แต่ไม่สูญข้อมูล
Ring buffer คือตัวกลางที่ทำให้เป็นไปได้

---

## Edge cases ที่ต้องระวัง (และทดสอบใน main.c)

- ✅ เขียนจนเต็ม → การเขียนครั้งถัดไปต้อง fail (ไม่ overwrite)
- ✅ อ่านจนว่าง → การอ่านครั้งถัดไปต้อง fail
- ✅ Wrap-around → head/tail วนกลับถูกต้อง
- ✅ เขียน-อ่านสลับกันจำนวนมาก → FIFO order ยังถูก

---

## Skills Unlocked ✓

- [x] Circular buffer logic ด้วย modulo
- [x] Static memory allocation (ไม่มี `malloc`)
- [x] `volatile` keyword และเหตุผลที่ใช้
- [x] Edge case thinking — full, empty, overrun, underrun
- [x] Producer-consumer pattern
- [x] FIFO concept — หัวใจของ serial communication

---

## → Level ถัดไป

Level 3 จะเปลี่ยนจาก "data structure thinking" ไปสู่ **"system design thinking"** — สร้าง **Finite State Machine** ที่เป็น pattern หลักของ firmware ทุกตัว
