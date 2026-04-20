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
```

---

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
