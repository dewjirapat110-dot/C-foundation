# Level 1 — bitwise_toolkit

> 🟢 **Bit & Memory Mastery**
> พื้นฐานของทุกอย่างในงาน embedded

---

## ทำไมต้องเริ่มด้วยเรื่องนี้

ในโลก embedded "ทุกอย่างคือ bit"

เวลาจะสั่งเปิด LED, อ่านปุ่ม, เปิด UART, ส่งข้อมูล SPI, ตั้ง timer...
ทุกอย่างจบลงที่ **bit ใน peripheral register**

```c
// เปิด LED ที่ขา PA5 ของ STM32
GPIOA->ODR |= (1 << 5);

// ปิด timer interrupt
TIM2->DIER &= ~(1 << 0);

// เช็คว่า UART รับ byte พร้อมอ่านหรือยัง
if (USART1->SR & (1 << 5)) { ... }
```

ถ้าคุณอ่านโค้ดข้างบนนี้ไม่ได้ = คุณยังไม่พร้อมทำ firmware จริง

Level 1 ฝึกให้คุณ **อ่านและเขียน code แบบนี้ได้อย่างอัตโนมัติ**

---

## สิ่งที่โปรเจ็คนี้ทำ

Library ที่ให้ฟังก์ชัน 4 ตัว สำหรับจัดการ bit ใน register

| ฟังก์ชัน | ทำอะไร |
|--------|-------|
| `set_bit(reg, pos)`    | บังคับให้ bit ที่ตำแหน่ง pos เป็น **1** |
| `clear_bit(reg, pos)`  | บังคับให้ bit ที่ตำแหน่ง pos เป็น **0** |
| `toggle_bit(reg, pos)` | **กลับค่า** bit (0 → 1, 1 → 0) |
| `read_bit(reg, pos)`   | **อ่านค่า** bit (คืน 0 หรือ 1) |

รองรับ register 3 ขนาด: 8-bit, 16-bit, 32-bit

---

## โครงสร้างไฟล์

```
level1_bitwise_toolkit/
├── bitwise.h      ← public API
├── bitwise.c      ← implementation
└── main.c         ← test cases
```

---

## Concept ที่ได้ฝึก

### Bitwise operators

| Operator | ใช้ทำ | กฎสำคัญ |
|----------|------|---------|
| `\|` (OR)  | set bit   | `x \| 1 = 1` |
| `&` (AND)  | clear bit | `x & 0 = 0` |
| `^` (XOR)  | toggle    | `x ^ 1 = ~x` |
| `~` (NOT)  | invert    | flip ทุก bit |
| `<<`       | shift ซ้าย | สร้าง mask |
| `>>`       | shift ขวา  | อ่าน bit |

### สูตรหลักที่ใช้ซ้ำๆ

```c
// Set:    force bit = 1
reg |=  (1 << pos);

// Clear:  force bit = 0
reg &= ~(1 << pos);

// Toggle: flip bit
reg ^=  (1 << pos);

// Read:   get bit value (0 หรือ 1)
(reg >> pos) & 1
```

สูตร 4 บรรทัดนี้คือ **DNA ของ firmware engineer** — ท่องจนออกมาโดยไม่ต้องคิด

### Fixed-width types

ใช้ `uint8_t`, `uint16_t`, `uint32_t` จาก `<stdint.h>` แทน `int`, `short`, `long`

```c
// ❌ ผิด — ขนาดไม่แน่นอน (PC: 4 byte, MCU 16-bit: 2 byte)
int register_value;

// ✓ ถูก — 4 byte เสมอทุก platform
uint32_t register_value;
```

ใน embedded การรู้ขนาด type **ต้องแน่นอน 100%** เพราะเราทาบลงบน hardware register ที่มีขนาดตายตัว

---

## วิธี Build และ Run

```powershell
gcc -Wall -Wextra bitwise.c main.c -o bitwise_toolkit
./bitwise_toolkit
```

---

## ตัวอย่างผลลัพธ์

```
=== Test 1: 8-bit register (เช่น GPIO 8 ขา) ===
เริ่มต้น:        0000 0000  (0x00)
set bit 0:      0000 0001  (0x01)
set bit 3, 7:   1000 1001  (0x89)
clear bit 0:    1000 1000  (0x88)
toggle bit 3:   1000 0000  (0x80)
read bit 7  = 1   (คาดหวัง 1)
read bit 5  = 0   (คาดหวัง 0)
```

---

## เชื่อมโยงกับงานจริง

สิ่งที่ฝึกใน Level 1 นี้ = **สิ่งที่ driver ทุกตัวใน firmware ทำ**

```c
// Driver ของ GPIO (เลียนแบบ STM32 HAL)
void gpio_set_pin(GPIO_TypeDef *port, uint8_t pin) {
    port->ODR |= (1 << pin);           // ← set_bit
}

void gpio_reset_pin(GPIO_TypeDef *port, uint8_t pin) {
    port->ODR &= ~(1 << pin);          // ← clear_bit
}

uint8_t gpio_read_pin(GPIO_TypeDef *port, uint8_t pin) {
    return (port->IDR >> pin) & 1;     // ← read_bit
}
```

เห็นไหมครับ — code ของ STM32 HAL ที่ดูน่ากลัว จริงๆ แล้วก็คือ 4 สูตรเดิมที่คุณฝึกมา

---

## Skills Unlocked ✓

- [x] Bitwise operators (`|`, `&`, `^`, `~`, `<<`, `>>`)
- [x] Fixed-width types จาก `<stdint.h>`
- [x] Register-level thinking — มอง memory เป็นกลุ่ม bit
- [x] Writing reusable C library ที่แยก `.h` กับ `.c`
- [x] Header guard ป้องกัน double include
- [x] Function prototype / declaration

---

## → Level ถัดไป

หลังจากจัดการ bit ระดับ hardware ได้แล้ว Level 2 จะพาไปสู่ **data structure แรกในงาน firmware** — **ring buffer** ที่เป็นหัวใจของทุก serial communication
