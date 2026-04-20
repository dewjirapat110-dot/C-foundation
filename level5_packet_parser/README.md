# Level 5 — packet_parser

> 🔵 **Protocol & Parser**
> ทุกอย่างที่ผ่านมาบรรจบกัน

---

## ทำไม Level นี้เป็นจุดรวม

ทุก serial protocol ใน firmware (UART, SPI, I2C) ส่งข้อมูลเป็น **binary packet** รูปแบบคล้ายๆ กัน:

```
[HEADER] [LENGTH] [DATA...] [CHECKSUM]
```

- NMEA sentences จาก GPS
- Modbus frames จาก industrial sensor
- MAVLink จาก drone
- Custom protocol จาก peripheral IC

→ เมื่อคุณ parse ได้ 1 อัน คุณ parse ได้ทุกอัน

Level นี้ทำให้ **ทุก concept จาก Level 1-4 มาเจอกัน**:
- `uint8_t` + bit manipulation (Level 1)
- Byte stream จาก ring buffer (Level 2)
- FSM pattern (Level 3) → parser **คือ** FSM
- Memory layout + alignment (Level 4) → `struct packed`

---

## Protocol ที่เราจะสร้าง

```
┌──────┬────────┬──────────────────────┬──────────┐
│ 0xAA │ LENGTH │ DATA (LENGTH bytes)  │ CHECKSUM │
└──────┴────────┴──────────────────────┴──────────┘
  1 B    1 B     0-16 bytes              1 B
```

- **0xAA** = sync byte — ใช้ "หาจุดเริ่ม" ของ packet
- **LENGTH** = จำนวน byte ใน DATA (0 ถึง 16)
- **DATA** = payload จริง
- **CHECKSUM** = `XOR` ของทุก byte ใน DATA

### ตัวอย่าง packet ที่ถูกต้อง
```
0xAA  0x03  0x01 0x02 0x03  0x00
 hdr  len   ←--- data ---→  checksum
                            (0x01 XOR 0x02 XOR 0x03 = 0x00)
```

---

## ทำไมต้องมี header byte?

ใน UART byte stream **ไม่มี "ขอบเขต" ของ packet**

```
...0x05 0x12 0xAA 0x03 0x01 0x02 0x03 0x00 0xFF 0x88...
              ↑
              เริ่ม packet ตรงไหน? parser จะรู้ได้ยังไง?
```

**คำตอบ:** ทิ้ง byte ที่ไม่ใช่ 0xAA ทั้งหมด → เจอ 0xAA แล้วเริ่ม parse

ถ้า stream เพี้ยน → parser จะ **resync** ตัวเองได้อัตโนมัติเมื่อเจอ 0xAA ตัวต่อไป

---

## สิ่งที่โปรเจ็คนี้ทำ

| ฟังก์ชัน | ทำอะไร |
|--------|-------|
| `parser_init(p)`           | เริ่มต้น parser |
| `parser_feed(p, byte)`     | ป้อน 1 byte → คืน `ParseResult` |
| `packet_checksum(data, n)` | คำนวณ XOR checksum |

**ParseResult:**
- `PR_IN_PROGRESS`  — ยังไม่ครบ packet, ป้อน byte ต่อไป
- `PR_COMPLETE`     — packet สมบูรณ์ ✓
- `PR_BAD_CHECKSUM` — packet ครบแต่ checksum ผิด ✗
- `PR_BAD_LENGTH`   — length เกิน MAX_DATA_LEN ✗

---

## โครงสร้างไฟล์

```
level5_packet_parser/
├── parser.h
├── parser.c
└── main.c
```

---

## Concept ที่ได้ฝึก

### 1. Parser = FSM (กลับมาใช้ Level 3!)

```
┌──────────────┐   byte == 0xAA   ┌──────────────┐
│ WAIT_HEADER  │ ───────────────→ │ WAIT_LENGTH  │
└──────────────┘                  └──────┬───────┘
       ↑                                 │ store length
       │                                 ▼
       │                          ┌──────────────┐
       │  complete/error          │ READING_DATA │
       │                          └──────┬───────┘
       │                                 │ data collected
       │                                 ▼
       │                          ┌──────────────┐
       └────────────────────────── WAIT_CHECKSUM │
                                  └──────────────┘
```

**ไม่มี `while` รอ byte** — feed ทีละ byte, state เดินทีละก้าว
→ เหมาะกับ interrupt-driven architecture เพราะไม่ block

### 2. `struct __attribute__((packed))`

```c
typedef struct __attribute__((packed)) {
    uint8_t header;
    uint8_t length;
    uint8_t data[MAX_DATA_LEN];
    uint8_t checksum;
} Packet;
```

**ปกติ** compiler ใส่ **padding** ระหว่าง field เพื่อ alignment (ดู Level 4)
**packed** บอก compiler ว่า "อย่าใส่ padding เลย"

ทำไมต้อง packed ใน protocol?
- เราต้องการ layout ตรงกับ byte stream เป๊ะๆ
- ถ้า compiler ใส่ padding → struct กับ byte stream ไม่ตรง → อ่านผิด

💡 **Trade-off:** packed struct อ่านได้ช้ากว่า (เพราะ field ไม่ aligned)
→ ในงานจริงบางทีเลือก "copy byte-by-byte จาก raw buffer มาใส่ struct ปกติ" แทน

### 3. XOR checksum

```c
uint8_t packet_checksum(const uint8_t *data, uint8_t length) {
    uint8_t sum = 0;
    for (uint8_t i = 0; i < length; i++) {
        sum ^= data[i];
    }
    return sum;
}
```

**คุณสมบัติสำคัญของ XOR:**
- `a ^ a = 0`
- `a ^ 0 = a`

→ XOR เป็น checksum ที่ **เร็วที่สุด** แต่ **อ่อนแอที่สุด**
→ ในงานจริงที่ต้องการความเชื่อถือสูงใช้ **CRC-8/16/32** แทน (ซับซ้อนกว่าแต่จับ error ได้ดีกว่ามาก)

---

## วิธี Build และ Run

```powershell
gcc -Wall -Wextra parser.c main.c -o packet_parser
./packet_parser
```

---

## ตัวอย่างผลลัพธ์

```
=== Test 4: Garbage before header (auto-resync) ===
  stream : 00 FF 12 34 AA 02 10 20 30 (9 bytes)
  result : COMPLETE ✓
  data   : 10 20 (2 bytes)

=== Test 5: Packet arriving in 2 chunks ===
  chunk 1: AA 04 A1 B2 (4 bytes)
  after chunk 1: IN_PROGRESS
  chunk 2: C3 D4 04 (3 bytes)
  after chunk 2: COMPLETE ✓
```

---

## เชื่อมโยงกับงานจริง

### UART receive pipeline ที่สมบูรณ์

```c
// Level 2: Ring buffer
RingBuffer uart_rx;

// Level 5: Parser
Parser parser;

// Level 2: Interrupt handler ใส่ byte ลง ring buffer
void USART1_IRQHandler(void) {
    uint8_t byte = USART1->DR;
    rb_write(&uart_rx, byte);
}

// Main loop
int main(void) {
    rb_init(&uart_rx);
    parser_init(&parser);

    while (1) {
        uint8_t byte;
        while (rb_read(&uart_rx, &byte)) {
            ParseResult result = parser_feed(&parser, byte);

            switch (result) {
            case PR_COMPLETE:
                handle_packet(&parser.packet);
                break;

            case PR_BAD_CHECKSUM:
            case PR_BAD_LENGTH:
                log_protocol_error();
                break;

            case PR_IN_PROGRESS:
                // ปกติ — ยังเก็บ byte ต่อไป
                break;
            }
        }
    }
}
```

**สังเกต:** ทั้งระบบใช้ non-blocking primitives ตลอด → latency predictable

---

## Edge cases ที่ทดสอบ (ใน main.c)

- ✅ Valid packet
- ✅ Corrupt checksum
- ✅ Invalid length (เกินขนาดที่รองรับ)
- ✅ Garbage ก่อน header (auto-resync)
- ✅ Packet ที่มาเป็น chunk (เหมือนใน UART จริง)
- ✅ Empty packet (length = 0)
- ✅ Multiple packet ติดกันใน stream เดียว

---

## Skills Unlocked ✓

- [x] `union` / packed struct สำหรับ protocol data
- [x] **FSM-based parser** — ประยุกต์ pattern Level 3
- [x] Byte-level protocol handling
- [x] Checksum / integrity verification
- [x] **Auto-resynchronization** — parser ฟื้นจาก error ได้เอง
- [x] Stream-based parsing (non-blocking)
- [x] การออกแบบ API ที่เชื่อมต่อ ring buffer + FSM ได้สวยงาม

---

## → Level ถัดไป

Level 6 เป็น **capstone** — สร้าง **cooperative scheduler** ที่รัน **ทุก Level ก่อนหน้า** พร้อมกันบน CPU ตัวเดียว = ภาพของ firmware system ที่สมบูรณ์
