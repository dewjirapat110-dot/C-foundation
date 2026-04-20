<<<<<<< HEAD
# C Foundation for Embedded Systems

โปรเจ็คฝึก C programming แบบ level-by-level เพื่อสร้างพื้นฐานที่แข็งแกร่งสำหรับงาน **embedded systems / firmware development**

ทุก Level ต่อยอดจาก Level ก่อนหน้า และรวมกันเป็น "ภาพใหญ่" ของ firmware ที่สมบูรณ์

---

## ภาพรวม Roadmap

| Level | Project | Concept หลัก | Skill ที่ได้ |
|-------|---------|--------------|--------------|
| 🟢 1 | [`bitwise_toolkit`](./level1_bitwise_toolkit) | Bit & Memory Mastery | set/clear/toggle/read bit, fixed-width types |
| 🟡 2 | [`ring_buffer`](./level2_ring_buffer) | Data Structure ใน Firmware | Circular FIFO, `volatile`, interrupt safety |
| 🟠 3 | [`traffic_light_fsm`](./level3_traffic_light_fsm) | State Machine | `enum` + `struct` + function pointer + state table |
| 🔴 4 | [`static_allocator`](./level4_static_allocator) | Memory & Pointer Mastery | Memory pool, alignment, `void*` |
| 🔵 5 | [`packet_parser`](./level5_packet_parser) | Protocol & Parser | Binary protocol, checksum, FSM-based parser |
| ⚫ 6 | [`cooperative_scheduler`](./level6_cooperative_scheduler) | Bare-metal Simulation | Task scheduling, priority, round-robin |

---

## ภาพใหญ่ — ทำไมลำดับนี้

Firmware ในชีวิตจริงมีโครงสร้างเป็นชั้นๆ แบบนี้

```
┌───────────────────────────────────────┐
│      Application Logic                │
├───────────────────────────────────────┤
│      Task Scheduler         ← Level 6 │
├───────────────────────────────────────┤
│      Protocol / Parser      ← Level 5 │
├───────────────────────────────────────┤
│      State Machine (FSM)    ← Level 3 │
├───────────────────────────────────────┤
│      Memory Management      ← Level 4 │
├───────────────────────────────────────┤
│      Data Buffer (Ring)     ← Level 2 │
├───────────────────────────────────────┤
│      Hardware Registers     ← Level 1 │
└───────────────────────────────────────┘
```

เราสร้างจาก **ชั้นล่างขึ้นบน** ทีละชั้น ทำให้แต่ละ Level มี "ฐาน" ที่มั่นคงเสมอ

---

## 6 Level เชื่อมโยงกันยังไงในงานจริง

ลองนึกภาพ firmware ที่อ่านข้อมูลจาก GPS module ผ่าน UART

```
GPS module
    │  ส่ง byte มาทาง UART
    ▼
┌───────────────────────────────────────────┐
│ UART Interrupt Handler                    │
│   อ่านจาก UART register                   │  ← Level 1
│   (GPIO/peripheral bit manipulation)      │
└──────────────┬────────────────────────────┘
               │
               │ write byte ลง
               ▼
┌───────────────────────────────────────────┐
│ Ring Buffer                               │  ← Level 2
│   (interrupt-safe FIFO)                   │
└──────────────┬────────────────────────────┘
               │
               │ main loop มาอ่าน
               ▼
┌───────────────────────────────────────────┐
│ Packet Parser (FSM)                       │  ← Level 5 (ใช้ pattern Level 3)
│   อ่านทีละ byte, ประกอบเป็น packet        │
└──────────────┬────────────────────────────┘
               │
               │ เมื่อ packet สมบูรณ์
               ▼
┌───────────────────────────────────────────┐
│ Static Allocator                          │  ← Level 4
│   จอง memory สำหรับ structure ที่ parse   │
└──────────────┬────────────────────────────┘
               │
               │ ส่งต่อให้
               ▼
┌───────────────────────────────────────────┐
│ Application State Machine                 │  ← Level 3
│   ตัดสินใจตาม state ปัจจุบัน               │
└──────────────┬────────────────────────────┘
               │
               │ task pattern ทั้งหมดจัดการโดย
               ▼
┌───────────────────────────────────────────┐
│ Cooperative Scheduler                     │  ← Level 6
│   จัดลำดับ task ทั้งหมดบน CPU ตัวเดียว    │
└───────────────────────────────────────────┘
```

นี่คือ firmware จริงๆ ที่ engineer สาย embedded เขียนทุกวัน

---

## โครงสร้าง Directory

```
c_foundation/
├── README.md                              ← ไฟล์นี้
├── level1_bitwise_toolkit/
│   ├── bitwise.h
│   ├── bitwise.c
│   ├── main.c
│   └── README.md
├── level2_ring_buffer/
│   ├── ring_buffer.h
│   ├── ring_buffer.c
│   ├── main.c
│   └── README.md
├── level3_traffic_light_fsm/
│   ├── fsm.h
│   ├── fsm.c
│   ├── main.c
│   └── README.md
├── level4_static_allocator/
│   ├── allocator.h
│   ├── allocator.c
│   ├── main.c
│   └── README.md
├── level5_packet_parser/
│   ├── parser.h
│   ├── parser.c
│   ├── main.c
│   └── README.md
└── level6_cooperative_scheduler/
    ├── scheduler.h
    ├── scheduler.c
    ├── main.c
    └── README.md
=======
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
>>>>>>> 377c29824590773fcfabffad0903691ba5510472
```

---

<<<<<<< HEAD
## วิธี Build และ Run (Windows + PowerShell)

แต่ละ Level มี README แยกของตัวเอง แต่รูปแบบเหมือนกันหมด

```powershell
# เข้าโฟลเดอร์ของ Level ที่ต้องการ
cd level1_bitwise_toolkit

# Compile (PowerShell ไม่รองรับ && จึงแยกบรรทัด)
gcc -Wall -Wextra bitwise.c main.c -o bitwise_toolkit

# Run
./bitwise_toolkit
```

### ทำไมใช้ `-Wall -Wextra`?

สองตัวนี้เปิด warning เพิ่มเติมของ GCC:

- `-Wall`   — warning พื้นฐานที่ควรสนใจทุกครั้ง
- `-Wextra` — warning เสริมที่มักช่วยจับ bug ก่อนเกิด

ใน embedded project ส่วนใหญ่บังคับให้ compile ด้วยธงพวกนี้แล้ว "ห้ามมี warning" เลย

---

## แนวคิดที่ปรากฏซ้ำทุก Level

ทั้ง 6 Level ใช้แนวคิดพื้นฐานชุดเดียวกันซ้ำๆ คุ้นเคยกับสิ่งเหล่านี้ = คุ้นเคยกับ C สำหรับงาน embedded

### 1. Fixed-width types จาก `<stdint.h>`
`uint8_t`, `uint16_t`, `uint32_t` แทน `int`, `short`, `long` — ขนาดชัดเจนทุก platform

### 2. `struct` ห่อข้อมูลที่ใช้ร่วมกัน
แทนที่จะใช้ global variable หลายตัว → ห่อเข้า struct ตัวเดียว

### 3. Separate `.h` / `.c` files
`.h` = interface (คนอื่นใช้), `.c` = implementation (ซ่อนรายละเอียด)

### 4. Header guard (`#ifndef ... #define ... #endif`)
ป้องกัน double include — ทำเสมอในทุก `.h`

### 5. `const` ถ้าไม่เปลี่ยน
บอก compiler (และคนอ่าน) ว่าค่านี้ห้ามแก้ — ช่วยจับ bug + optimize ได้

### 6. Return value สื่อสถานะ (`bool`, enum)
แทน `return void` หรือใช้ magic number — อ่านง่ายและปลอดภัยกว่า

### 7. ไม่ใช้ `malloc` เลย
ทุกอย่างเป็น static allocation — deterministic และปลอดภัยใน real-time system

---

## ทำไมเรียน C เพื่อ Embedded ถึงต้องทำแบบนี้

การอ่าน textbook หรือฟังคลิปได้ความรู้แค่ "ผิวๆ" เพราะ:

- **อ่านรู้เรื่อง ≠ เขียนได้** — ต้องได้แตะของจริงถึงจะเข้าใจกับดัก
- **Concept ไม่ยึดในหัว** ถ้าไม่ได้ใช้แก้ปัญหาเอง
- **Edge case ที่น่ากลัวที่สุด** ในงาน firmware (race condition, alignment, buffer overflow) มักไม่ปรากฏในตัวอย่างสั้นๆ

การทำ project ทั้ง 6 Level นี้จบ = คุณได้แตะปัญหาจริง และได้เห็นวิธีคิดของคนทำ firmware

---

## เส้นทางถัดไป

หลังจบ 6 Level นี้ ทักษะที่แข็งแกร่งพอสำหรับ:

1. **เริ่มใช้ RTOS** เช่น FreeRTOS — จะเข้าใจ task/queue/semaphore ง่ายขึ้นมาก เพราะเคยทำของ primitive version เองแล้ว
2. **อ่าน code base จริง** ของ driver ใน Linux kernel, Zephyr, nuttx ได้
3. **เขียน driver เอง** สำหรับ peripheral ที่ chipset ไม่ support
4. **ทำ bootloader** — เพราะเข้าใจ memory layout และ low-level boot sequence
5. **Debug HardFault** บน ARM Cortex-M — เพราะเข้าใจ alignment และ stack ดีแล้ว

---

## License

Educational use. เขียนขึ้นเพื่อการเรียนรู้ส่วนตัวของ ดิว (Dew)
=======
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
>>>>>>> 377c29824590773fcfabffad0903691ba5510472
