# Level 6 — cooperative_scheduler

> ⚫ **Bare-metal Simulation**
> Capstone — ทุก Level ก่อนหน้าบรรจบกันเป็นระบบที่รันจริง

---

## ทำไม Level นี้เป็นจุดสิ้นสุดที่สมบูรณ์

ถึงจุดนี้คุณสร้าง **component** ที่ firmware ต้องใช้ครบแล้ว
- bit manipulation (Level 1)
- ring buffer (Level 2)
- state machine (Level 3)
- memory allocator (Level 4)
- protocol parser (Level 5)

แต่ component เหล่านี้ในระบบจริง **ไม่ได้รันเดี่ยวๆ** — ทั้งหมดต้องรันพร้อมกันบน CPU ตัวเดียว

**Level 6 คือ "ผู้จัดคิว" ที่ทำให้ทุกอย่างรันพร้อมกันได้**

---

## Cooperative Scheduler คืออะไร

ระบบที่รัน **หลาย task** บน CPU ตัวเดียว **โดยไม่มี OS**

### หลักการทำงาน

```
Timer interrupt ทุก 1 ms
       │
       ▼
  tick counter += 1
       │
       ▼
  ตรวจทุก task:
    "ถึงเวลารันไหม?"
       │
       ▼
  รัน task ตาม priority
  (task ทำงานเสร็จเร็ว → return)
       │
       ▼
  รอ tick ถัดไป
```

### Cooperative vs Preemptive

| | **Cooperative** (ของเรา) | **Preemptive** (FreeRTOS, Linux) |
|---|---|---|
| Task หยุดเมื่อไหร่ | จบเอง (return) | OS หยุดให้ได้ทุกเมื่อ |
| ใช้ memory | น้อยมาก | ต้องมี stack ต่อ task |
| Debug | ง่าย (flow ชัดเจน) | ยาก (race condition) |
| Real-time guarantee | ขึ้นกับ task ที่ยาวที่สุด | OS รับประกัน deadline ได้ |
| ต้องการ hardware timer + context switch | ไม่จำเป็น | จำเป็น |

ใน MCU ขนาดเล็ก cooperative scheduler เป็น pattern ที่นิยมมากเพราะ:
1. ไม่ต้องพึ่ง RTOS → ประหยัด flash + RAM
2. Debug ง่ายกว่า
3. เพียงพอสำหรับงานส่วนใหญ่ (sensor, comms, state machine)

---

## สิ่งที่โปรเจ็คนี้ทำ

Scheduler ที่รองรับ task พร้อมกันสูงสุด 8 ตัว แต่ละตัวมี:
- **Function pointer** (จาก Level 3!)
- **Period** — รันห่างกันกี่ ms ต่อรอบ
- **Priority** — ตัวเลขน้อย = สำคัญมาก (รันก่อน)
- **Enable/disable flag** — pause ชั่วคราวได้

| ฟังก์ชัน | ทำอะไร |
|--------|-------|
| `scheduler_init(s)`                       | เริ่มต้น scheduler |
| `scheduler_add_task(s, name, fn, period, prio)` | ลงทะเบียน task |
| `scheduler_tick(s)`                       | 1 tick (เรียกจาก timer IRQ) |
| `scheduler_run(s, duration_ms)`           | รัน duration ms (สำหรับจำลอง) |
| `scheduler_enable_task(s, name)`          | เปิด task |
| `scheduler_disable_task(s, name)`         | ปิด task |

---

## โครงสร้างไฟล์

```
level6_cooperative_scheduler/
├── scheduler.h
├── scheduler.c
└── main.c
```

---

## Concept ที่ได้ฝึก

### 1. Typedef function pointer

```c
typedef void (*TaskFn)(void);
```

เดิมต้องเขียนยาวๆ ทุกครั้ง:
```c
void (*function)(void);
```

`typedef` ทำให้อ่านง่ายขึ้น:
```c
TaskFn function;
```

### 2. Tick-based timing

```c
uint32_t elapsed = s->current_tick - t->last_run_ms;
if (elapsed >= t->period_ms) {
    t->last_run_ms = s->current_tick;
    t->function();
}
```

**สังเกต:** ไม่ใช้ `current_tick % period == 0`

เพราะถ้า scheduler ช้าไป 1 tick → **พลาดรอบทั้งรอบ**

วิธี "นับระยะห่างจากครั้งล่าสุด" ปลอดภัยกว่า — ถ้า lag ก็รันทันทีที่เป็นไปได้

### 3. Priority dispatch

```c
for (int prio = 0; prio < 256; prio++) {
    for (uint8_t i = 0; i < s->task_count; i++) {
        if (s->tasks[i].priority == prio) {
            // รันถ้าถึงเวลา
        }
    }
}
```

เมื่อ task หลายตัวครบเวลาพร้อมกัน → รันจาก priority น้อยก่อน

ตัวอย่างผลลัพธ์ที่ T=1000ms:
```
[T= 1000 ms] LED blink       ← priority 1 (รันก่อน)
[T= 1000 ms] sensor_read     ← priority 2
[T= 1000 ms] heartbeat       ← priority 2 (เรียงตามลำดับ add)
[T= 1000 ms] status_report   ← priority 3 (รันหลังสุด)
```

### 4. Cooperative contract

```c
// ✓ ถูก — task ทำงานเสร็จแล้ว return
void task_blink(void) {
    toggle_led();
}

// ✗ ผิด — task block, ทุก task อื่นรอ
void task_broken(void) {
    while (1) {
        check_sensor();
    }
}

// ✗ ผิด — task ใช้เวลานาน, task อื่นหน่วง
void task_too_slow(void) {
    delay_ms(500);  // ทุก task อื่นหายไป 500 ms!
}
```

**กฎเหล็ก:** ทุก task ต้องสั้นและไม่ block

---

## วิธี Build และ Run

```powershell
gcc -Wall -Wextra scheduler.c main.c -o cooperative_scheduler
./cooperative_scheduler
```

---

## ตัวอย่างผลลัพธ์

```
Tasks registered:
  - led_blink       period= 500 ms  priority=1
  - sensor_read     period= 100 ms  priority=2
  - status_report   period=1000 ms  priority=3
  - heartbeat       period= 250 ms  priority=2

---- เริ่มรัน scheduler 2 วินาที ----

  [T=  100 ms] sensor_read     → temp = 25.1°C
  [T=  200 ms] sensor_read     → temp = 25.2°C
  [T=  250 ms] heartbeat       → ♥
  [T=  300 ms] sensor_read     → temp = 25.3°C
  ...
  [T= 1000 ms] LED blink       → LED OFF       ← priority 1 ก่อน
  [T= 1000 ms] sensor_read     → temp = 26.0°C ← priority 2
  [T= 1000 ms] heartbeat       → ♥             ← priority 2 (ลำดับถัดไป)
  [T= 1000 ms] status_report   → system healthy ← priority 3 ทีหลังสุด
```

สังเกตว่าที่ `T=1000ms` มี 4 task ถึงเวลาพร้อมกัน — scheduler เรียงรันตาม priority ได้ถูกต้อง

---

## เชื่อมโยงกับทุก Level — ภาพที่สมบูรณ์

นี่คือ firmware จริงที่ใช้ทุก component ที่สร้างมา

```c
// === Global state ===
RingBuffer uart_rx;          // Level 2
Parser     protocol_parser;  // Level 5
Pool       packet_pool;      // Level 4
FSM        app_fsm;          // Level 3
Scheduler  sched;            // Level 6

// === Task: ดึง byte จาก ring buffer ป้อนให้ parser ===
void task_parse_uart(void) {
    uint8_t byte;
    while (rb_read(&uart_rx, &byte)) {                    // Level 2
        ParseResult r = parser_feed(&protocol_parser,    // Level 5
                                    byte);
        if (r == PR_COMPLETE) {
            Packet *pkt = pool_alloc(&packet_pool,        // Level 4
                                     sizeof(Packet));
            *pkt = protocol_parser.packet;
            handle_packet(pkt);
        }
    }
}

// === Task: ขับเคลื่อน state machine ===
void task_app_logic(void) {
    fsm_tick(&app_fsm);                                   // Level 3
}

// === Task: blink LED ===
void task_heartbeat(void) {
    GPIOA->ODR ^= (1 << 5);                               // Level 1
}

// === Main ===
int main(void) {
    // Init ทุก component
    rb_init(&uart_rx);
    parser_init(&protocol_parser);
    pool_init(&packet_pool);
    fsm_init(&app_fsm, STATE_IDLE);
    scheduler_init(&sched);

    // Register tasks
    scheduler_add_task(&sched, "parse",  task_parse_uart, 10,   1);
    scheduler_add_task(&sched, "app",    task_app_logic,  50,   2);
    scheduler_add_task(&sched, "heart",  task_heartbeat,  500,  3);

    // รัน forever
    while (1) {
        scheduler_tick(&sched);                           // Level 6
        // (ใน MCU จริงตรงนี้เรียกจาก SysTick IRQ)
    }
}
```

**ทุก Level = ชิ้นส่วนของระบบนี้** — ไม่มีชิ้นไหนเกิน ไม่มีชิ้นไหนขาด

---

## เส้นทางถัดไปหลังจบ Level 6

### ระยะใกล้
1. **ลองรันบน MCU จริง** — เอา code นี้ไป port ลง STM32 Blue Pill ($3)
2. **เพิ่ม task ของตัวเอง** — เช่น ADC reader, PWM controller, button handler
3. **ใช้ interrupt จริง** แทน simulation loop

### ระยะกลาง
4. **ขยายไปใช้ FreeRTOS** — จะรู้สึกคุ้นเคยมาก เพราะทุก concept เคยทำเองมาแล้ว
5. **เขียน custom driver** — เพราะเข้าใจ bit + register + timing แล้ว
6. **ทำ bootloader** — ใช้ memory layout ที่เข้าใจจาก Level 4

### ระยะยาว
7. **อ่าน Linux kernel driver** — syntax พวก `container_of`, `list_head` จะดูเป็นธรรมชาติ
8. **Contribute open-source** — Zephyr, ChibiOS, NuttX มีโปรเจ็คที่ต้องการคน

---

## Skills Unlocked ✓

- [x] `typedef` function pointer
- [x] Task struct + function pointer array
- [x] **Tick-based timing** (non-blocking)
- [x] **Priority-based dispatch**
- [x] **Cooperative contract** — ทำไมทุก task ต้องสั้น
- [x] การ design API ที่ scalable
- [x] **ภาพรวมของ firmware system** ที่สมบูรณ์
- [x] ความเชื่อมโยงระหว่าง cooperative scheduler กับ preemptive RTOS

---

## บทสรุป

หลังจบ 6 Level คุณไม่ได้แค่รู้ C — คุณ **คิดเป็น firmware engineer**

ก็คือ:
- เห็น bit ใน register = รู้ว่าต้อง set/clear ยังไง
- เห็น byte stream = รู้ว่าต้องใช้ ring buffer
- เห็น behavior ที่ซับซ้อน = รู้ว่าควรเป็น FSM
- เห็น memory = คิดเรื่อง alignment ก่อน
- เห็น protocol = รู้ว่า parser คือ FSM
- เห็นระบบที่ต้องทำหลายงาน = คิดเป็น task + scheduler

**นี่คือ foundation ที่แข็งแกร่งพอสำหรับงาน firmware professional**
