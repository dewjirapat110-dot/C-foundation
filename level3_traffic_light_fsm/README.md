# Level 3 — traffic_light_fsm

> 🟠 **State Machine**
> จุดเปลี่ยนจาก "เขียน code" ไปสู่ "ออกแบบระบบ"

---

## ทำไม Level นี้ถึงสำคัญมาก

**Firmware ส่วนใหญ่คือ state machine**

- เครื่องชงกาแฟ: รอปุ่ม → ตั้งอุณหภูมิ → ชง → เสร็จ
- ปุ่มไฟห้อง: OFF → กดทีเดียว → ON → กดอีกที → OFF
- Protocol UART: idle → header → data → checksum → complete
- Motor controller: stopped → accelerating → running → decelerating → stopped

ถ้าคุณเขียน firmware โดยไม่รู้จัก FSM pattern → จะลงเอยด้วย `if-else` ซ้อน 5-6 ชั้น ที่แก้ไม่ได้

**FSM บังคับให้เราคิดเป็น "สถานะ" — ทำให้ complexity จัดการได้**

---

## FSM คืออะไร

ระบบที่อยู่ใน **"สถานะเดียว"** จากชุดสถานะจำกัด และมีกฎการ **"เปลี่ยนสถานะ"** ที่ชัดเจน

```
┌──────┐  ครบเวลา  ┌──────┐  ครบเวลา  ┌──────┐
│ RED  │ ────────→ │GREEN │ ────────→ │YELLOW│
└──────┘           └──────┘           └──┬───┘
   ↑                                      │
   └──────────────── ครบเวลา ─────────────┘
```

ในตัวอย่างไฟจราจร:
- มี 3 state: RED, GREEN, YELLOW
- แต่ละ state มีระยะเวลาของมัน
- เมื่อครบเวลา → เปลี่ยนไป state ถัดไป

---

## สิ่งที่โปรเจ็คนี้ทำ

FSM จำลองไฟจราจรที่ใช้ **state table + function pointer** — pattern ที่สะอาดและขยายง่าย

---

## โครงสร้างไฟล์

```
level3_traffic_light_fsm/
├── fsm.h       ← enum state, struct, API
├── fsm.c       ← state table + FSM engine
└── main.c      ← simulation loop
```

---

## Concept ที่ได้ฝึก

### 1. `enum` สำหรับนิยาม state

```c
typedef enum {
    STATE_RED    = 0,
    STATE_GREEN  = 1,
    STATE_YELLOW = 2,
    STATE_COUNT
} TrafficState;
```

- 💡 **Trick:** `STATE_COUNT` ที่ปลาย enum = **จำนวน state อัตโนมัติ**
- ใช้เป็น array index ได้ตรงๆ (เริ่มจาก 0)

### 2. State table with designated initializer

```c
static const StateConfig state_table[STATE_COUNT] = {
    [STATE_RED] = {
        .name       = "RED",
        .duration   = 5,
        .on_enter   = on_enter_red,
        .next_state = STATE_GREEN
    },
    [STATE_GREEN] = { ... },
    [STATE_YELLOW] = { ... }
};
```

**ทำไม designated initializer ดีกว่า?**

เปรียบเทียบ:

```c
// ❌ แบบธรรมดา — เรียง index ต้องเป๊ะ
{ "RED", 5, on_enter_red, STATE_GREEN },
{ "GREEN", 5, on_enter_green, STATE_YELLOW },
{ "YELLOW", 2, on_enter_yellow, STATE_RED }
```

ถ้าวันหนึ่งเพิ่ม `STATE_BLINK` ตรงกลาง enum → array index เลื่อน → **bug แบบเงียบๆ**

```c
// ✓ Designated initializer — จับคู่ชื่อ index ให้เอง
[STATE_RED]    = { ... }
[STATE_GREEN]  = { ... }
[STATE_BLINK]  = { ... }  ← เพิ่มตรงนี้ก็ไม่กระทบของเดิม
[STATE_YELLOW] = { ... }
```

### 3. Function pointer — หัวใจของ pattern นี้

```c
typedef struct {
    const char *name;
    uint32_t    duration;
    void      (*on_enter)(void);   // ← function pointer
    TrafficState next_state;
} StateConfig;
```

Function pointer = **"ตัวแปรที่เก็บที่อยู่ของฟังก์ชัน"**

```c
// แทนที่จะ...
if (state == RED)         on_enter_red();
else if (state == GREEN)  on_enter_green();
else if (state == YELLOW) on_enter_yellow();

// เราทำแค่...
state_table[state].on_enter();
```

เพิ่ม state 10 ตัว = เพิ่มแถวใน table 10 แถว — **engine code เท่าเดิม**

### 4. FSM engine ที่ "ไม่รู้" ว่ากำลังรัน state อะไร

```c
void fsm_tick(FSM *fsm) {
    const StateConfig *cfg = &state_table[fsm->current_state];

    fsm->tick_count++;

    if (fsm->tick_count >= cfg->duration) {
        fsm->tick_count    = 0;
        fsm->current_state = cfg->next_state;
        state_table[fsm->current_state].on_enter();
    }
}
```

- ไม่มี `if-else` เกี่ยวกับ state
- ไม่มี `switch` ระบุชื่อ state
- **ทุกอย่างอยู่ใน state table ทั้งหมด**

นี่คือ **"data-driven design"** — พฤติกรรมของระบบกำหนดด้วยข้อมูล ไม่ใช่ code

---

## วิธี Build และ Run

```powershell
gcc -Wall -Wextra fsm.c main.c -o traffic_light_fsm
./traffic_light_fsm
```

---

## ตัวอย่างผลลัพธ์

```
[tick  1] state = RED
[tick  2] state = RED
...
[tick  5] state = RED         [Hardware] >>> LED แดง OFF | LED เขียว ON
[tick  6] state = GREEN
[tick  7] state = GREEN
...
[tick 10] state = GREEN       [Hardware] >>> LED แดง OFF | LED เขียว OFF | LED เหลือง ON
[tick 11] state = YELLOW
[tick 12] state = YELLOW      [Hardware] >>> LED แดง ON
[tick 13] state = RED
...
```

---

## เชื่อมโยงกับงานจริง

Pattern นี้ใช้ได้กับแทบทุก firmware ที่คุณจะเขียนในอนาคต:

```c
// ตัวอย่าง: Button debouncer FSM
typedef enum {
    BTN_IDLE,
    BTN_PRESSED,
    BTN_HELD,
    BTN_RELEASED
} ButtonState;

// ตัวอย่าง: UART protocol parser FSM (= Level 5!)
typedef enum {
    PARSER_WAIT_HEADER,
    PARSER_WAIT_LENGTH,
    PARSER_READ_DATA,
    PARSER_WAIT_CHECKSUM
} ParserState;

// ตัวอย่าง: Motor control FSM
typedef enum {
    MOTOR_STOPPED,
    MOTOR_ACCELERATING,
    MOTOR_RUNNING,
    MOTOR_DECELERATING,
    MOTOR_FAULT
} MotorState;
```

ทุกตัวใช้ pattern เดียวกัน: enum + struct + function pointer + state table

---

## Skills Unlocked ✓

- [x] `enum` แทน magic number
- [x] `struct` ห่อ config ของแต่ละ state
- [x] **Function pointer** — เก็บที่อยู่ฟังก์ชันไว้ใน struct
- [x] **State table pattern** — data-driven FSM
- [x] **Designated initializer** — `[INDEX] = {...}`
- [x] `const` สำหรับตารางที่ไม่เปลี่ยน (ได้ประโยชน์ flash memory บน MCU)
- [x] FSM context struct — ทำให้สร้างหลาย instance ได้

---

## → Level ถัดไป

Level 4 จะพาไปสู่ **memory mastery** — สร้าง allocator ของเราเอง โดยไม่ใช้ `malloc` เลย แล้วเจอเรื่องสำคัญที่มักมองข้าม: **alignment**
