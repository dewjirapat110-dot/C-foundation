# Level 4 — static_allocator

> 🔴 **Memory & Pointer Mastery**
> ทำไม firmware engineer ไม่ใช้ `malloc`

---

## ทำไม embedded ถึงไม่ใช้ malloc

ใน PC การใช้ `malloc / free` เป็นเรื่องปกติ แต่ใน **firmware บน MCU** มันคือเรื่องใหญ่

### ปัญหา 1: Heap fragmentation

```
เริ่มต้น:  [─────── heap 1 KB ───────]

malloc(100) × 10 → [100][100][100][100][100][100][100][100][100][100]

free(index 1,3,5,7,9) → [100][   ][100][   ][100][   ][100][   ][100][   ]

malloc(500) → FAIL!
  (แม้ว่า free space รวม = 500 byte พอดี
   แต่ไม่มีก้อนต่อเนื่องขนาด 500 byte)
```

→ Memory พอ แต่ขอไม่ได้ — bug ที่เกิดช้าๆ หลังระบบรันไปนานๆ

### ปัญหา 2: Non-deterministic timing

`malloc` ใช้เวลาไม่แน่นอน (อาจหา free block นาน) → ใน real-time system ที่ต้องตอบสนองภายใน `X ms` เป๊ะ = **อันตราย**

### ปัญหา 3: MCU เล็กไม่มี heap

`malloc` ต้องการ runtime/OS support — MCU ขนาดเล็กอาจไม่มีเลย

---

## วิธีแก้: Static Memory Pool

จองพื้นที่ก้อนใหญ่ก้อนเดียวตอนเริ่มโปรแกรม แล้วเขียน allocator มาแบ่งใช้เอง

```
┌──────────────────────────────────────────┐
│  static uint8_t buffer[1024];            │  ← จองตอน compile time
└──────────────────────────────────────────┘
```

แบบที่เราจะทำคือ **bump allocator / linear allocator**:

```
┌────────────────────────────────────────┐
│████████████████████░░░░░░░░░░░░░░░░░░░│
└──────────────────▲─────────────────────┘
                   │
                   used pointer
                   (เพิ่มทีละก้อน)
```

- ขอ memory = เลื่อน pointer ไปข้างหน้า
- ไม่มี free ทีละก้อน → reset ทั้งก้อนเท่านั้น

### ข้อดี
- เร็วมาก (O(1))
- Deterministic 100%
- ไม่มี fragmentation

### ข้อจำกัด
- Free ทีละก้อนไม่ได้ (ต้อง reset ทั้งหมด)
- ไม่เหมาะถ้า lifetime ของแต่ละก้อนต่างกันมาก

---

## สิ่งที่โปรเจ็คนี้ทำ

Memory pool ขนาด 1 KB พร้อม alignment ที่ถูกต้อง

| ฟังก์ชัน | ทำอะไร |
|--------|-------|
| `pool_init(p)`        | เริ่มต้น pool |
| `pool_alloc(p, size)` | จอง memory ขนาด `size` byte (auto-align เป็น 4 byte) |
| `pool_reset(p)`       | รีเซ็ต pool ให้ว่างทั้งหมด |
| `pool_used(p)`        | ใช้ไปแล้วกี่ byte |
| `pool_free(p)`        | เหลืออีกกี่ byte |

---

## โครงสร้างไฟล์

```
level4_static_allocator/
├── allocator.h
├── allocator.c
└── main.c
```

---

## Concept ที่ได้ฝึก

### 1. `void*` — pointer ไปอะไรก็ได้

```c
void *pool_alloc(Pool *p, size_t size);
```

Allocator ไม่รู้ (และไม่สน) ว่าผู้ใช้จะเอา memory ไปเก็บ type อะไร
คืนเป็น `void*` → ผู้ใช้ `cast` เอาเอง

```c
int      *p_int    = (int *)pool_alloc(&pool, sizeof(int));
uint8_t  *p_byte   = (uint8_t *)pool_alloc(&pool, 1);
Sensor   *p_sensor = (Sensor *)pool_alloc(&pool, sizeof(Sensor));
```

นี่คือเหตุผลที่ `malloc` ก็คืน `void*` เช่นเดียวกัน

### 2. Pointer arithmetic

```c
void *ptr = p->buffer + p->used;
```

- `p->buffer` = pointer ไปต้น array (= `&buffer[0]`)
- บวก `p->used` (เป็น byte) → pointer ไปตำแหน่งว่างถัดไป

เพราะ `buffer` เป็น `uint8_t[]` → บวกเป็น byte ตรงๆ (ถ้าเป็น `uint32_t*` จะบวกทีละ 4 byte อัตโนมัติ)

### 3. **Alignment** — เรื่องสำคัญที่มักมองข้าม

```c
size_t aligned = (size + ALIGNMENT - 1) & ~(ALIGNMENT - 1);
```

**Alignment คืออะไร?**

CPU แต่ละตัวอยากอ่านข้อมูลจาก address ที่ "ลงตัว" ตามขนาด type:

| Type       | ขนาด | Alignment | Address ต้อง... |
|------------|------|-----------|-----------------|
| `uint8_t`  | 1    | 1         | ได้ทุก address |
| `uint16_t` | 2    | 2         | ลงตัวด้วย 2 |
| `uint32_t` | 4    | 4         | ลงตัวด้วย 4 |
| `uint64_t` | 8    | 8         | ลงตัวด้วย 8 |

ถ้าอ่าน `uint32_t` จาก address ที่ไม่ลงตัวด้วย 4:
- **ARM Cortex-M0**: crash → HardFault 💥
- **x86**: อ่านได้แต่ช้ากว่า (แตกเป็น 2 รอบ)

**สูตรปัดขึ้นให้ aligned:**
```c
aligned_size = (size + ALIGN - 1) & ~(ALIGN - 1);
```

ตัวอย่าง ALIGN = 4:
| Input size | `(size+3)` | `& ~3` | Result |
|-----------|------------|--------|--------|
| 1         | 4          | = 4    | **4**  |
| 3         | 6          | = 4    | **4**  |
| 5         | 8          | = 8    | **8**  |
| 8         | 11         | = 8    | **8**  |
| 9         | 12         | = 12   | **12** |

### 4. `size_t` — integer type สำหรับขนาด

```c
size_t size = sizeof(int);
```

`size_t` เป็น **unsigned integer** ที่มีขนาดใหญ่พอจะบรรจุขนาดของ object ที่ใหญ่ที่สุดบน platform นั้น
- บน 32-bit platform: ปกติ 4 byte
- บน 64-bit platform: ปกติ 8 byte

**กฎ:** ถ้าตัวแปรนั้นแทนขนาด/จำนวน byte → ใช้ `size_t`

---

## วิธี Build และ Run

```powershell
gcc -Wall -Wextra allocator.c main.c -o static_allocator
./static_allocator
```

---

## ตัวอย่างผลลัพธ์

```
=== Test 2: Alignment verification ===
  alloc(1) → offset 0   (คาดหวัง 0)
  alloc(3) → offset 4   (คาดหวัง 4)
  alloc(5) → offset 8   (คาดหวัง 8)
  alloc(9) → offset 16  (คาดหวัง 16)

  ทุก offset หาร 4 ลงตัวไหม?
    offset 0  % 4 = 0  ✓
    offset 4  % 4 = 0  ✓
    offset 8  % 4 = 0  ✓
    offset 16 % 4 = 0  ✓
```

---

## เชื่อมโยงกับงานจริง

### Use case 1: Parse packet buffer (Level 5!)

```c
Pool packet_pool;
pool_init(&packet_pool);

while (packet_received()) {
    Packet *pkt = pool_alloc(&packet_pool, sizeof(Packet));
    fill_packet_from_uart(pkt);
    process_packet(pkt);
}

// จบ frame → reset ทั้งหมดครั้งเดียว (O(1))
pool_reset(&packet_pool);
```

### Use case 2: FreeRTOS heap_1.c

FreeRTOS มี `heap_1.c` ที่เป็น bump allocator แบบเดียวกับที่เราเพิ่งเขียน — ใช้ในระบบที่ create task ครั้งเดียวตอน boot ไม่ delete เลย

### Use case 3: Arena allocator ในเกม

Game engine จำนวนมากใช้ pattern นี้สำหรับ per-frame allocation:
- Frame เริ่ม → alloc ได้อิสระ
- Frame จบ → reset ทั้งหมด

---

## Skills Unlocked ✓

- [x] `void *` — generic pointer
- [x] `size_t` — portable size type
- [x] Pointer arithmetic
- [x] **Alignment** — คำนวณและประโยชน์
- [x] Static pool allocation
- [x] Bump/linear allocator pattern
- [x] Exhaustion handling (return `NULL`)
- [x] `const` parameter สำหรับ read-only input

---

## → Level ถัดไป

Level 5 จะเป็น **"ทุกอย่างที่ผ่านมาบรรจบกัน"** — สร้าง **packet parser** ที่ใช้ FSM (Level 3) + struct layout ที่สำคัญเรื่อง alignment (Level 4) + ring buffer เป็นแหล่ง input (Level 2)
