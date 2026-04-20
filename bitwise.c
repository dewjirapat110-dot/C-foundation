// ============================================================
// bitwise.c
// Implementation ของฟังก์ชันทั้งหมดที่ประกาศไว้ใน bitwise.h
// ============================================================
//
// แนวคิดหลัก: ทุกฟังก์ชันใช้ "bitmask" ในการทำงาน
//
// bitmask คืออะไร?
// = uint ที่มี bit ที่เราสนใจเป็น 1 เพียงตัวเดียว ที่เหลือเป็น 0
//
// สร้าง mask ได้ยังไง?
// = (1 << pos)
//
// ตัวอย่าง: ถ้า pos = 3
//   1            = 0000 0001
//   1 << 3       = 0000 1000   ← bit ที่ 3 เป็น 1 ตัวเดียว
// ============================================================

#include "bitwise.h"

// ============================================================
// 8-bit operations
// ============================================================

// SET: บังคับให้ bit ที่ตำแหน่ง pos กลายเป็น 1
//
// ใช้ OR (|) เพราะกฎ OR คือ:
//   x | 0 = x  (bit อื่นไม่เปลี่ยน)
//   x | 1 = 1  (bit ที่เราต้องการกลายเป็น 1)
//
// ตัวอย่าง: reg = 0101 0010, pos = 3
//   reg     = 0101 0010
//   1 << 3  = 0000 1000
//   ผลลัพธ์   = 0101 1010   ← bit 3 ขึ้นเป็น 1
void set_bit_u8(uint8_t *reg, uint8_t pos)
{
    *reg |= (uint8_t)(1U << pos);
    // หมายเหตุ: cast เป็น (uint8_t) เพื่อความชัดเจนเรื่อง type
    //          และใช้ 1U (unsigned 1) เพื่อหลีกเลี่ยง undefined behavior
    //          ที่อาจเกิดเมื่อ shift integer แบบ signed
}

// CLEAR: บังคับให้ bit ที่ตำแหน่ง pos กลายเป็น 0
//
// ใช้ AND (&) กับ NOT (~) ของ mask
// กฎ AND:
//   x & 1 = x  (bit อื่นไม่เปลี่ยน)
//   x & 0 = 0  (bit ที่เราต้องการกลายเป็น 0)
//
// ตัวอย่าง: reg = 0101 1010, pos = 3
//   reg          = 0101 1010
//   1 << 3       = 0000 1000
//   ~(1 << 3)    = 1111 0111   ← invert mask
//   reg & ~mask  = 0101 0010   ← bit 3 ลงเป็น 0
void clear_bit_u8(uint8_t *reg, uint8_t pos)
{
    *reg &= (uint8_t)~(1U << pos);
}

// TOGGLE: กลับค่า bit (0→1, 1→0)
//
// ใช้ XOR (^) เพราะกฎ XOR คือ:
//   x ^ 0 = x   (bit อื่นไม่เปลี่ยน)
//   x ^ 1 = ~x  (bit ที่เราต้องการถูก flip)
//
// ตัวอย่าง: reg = 0101 0010, pos = 1
//   reg     = 0101 0010
//   1 << 1  = 0000 0010
//   ผลลัพธ์   = 0101 0000   ← bit 1 จาก 1 → 0
void toggle_bit_u8(uint8_t *reg, uint8_t pos)
{
    *reg ^= (uint8_t)(1U << pos);
}

// READ: อ่านค่า bit ที่ตำแหน่ง pos (return 0 หรือ 1)
//
// ขั้นตอน:
//   1. shift bit ที่เราสนใจไปอยู่ตำแหน่ง 0 (LSB)
//   2. AND กับ 1 เพื่อตัดเอาแค่ bit เดียว (bit อื่นกลายเป็น 0)
//
// ตัวอย่าง: reg = 0101 1010, pos = 4
//   reg >> 4    = 0000 0101   ← bit 4 ของเดิมตอนนี้อยู่ที่ตำแหน่ง 0
//   & 1         = 0000 0001   ← ตัดเหลือแค่ bit เดียว = 1
uint8_t read_bit_u8(uint8_t reg, uint8_t pos)
{
    return (uint8_t)((reg >> pos) & 1U);
}

// ============================================================
// 16-bit operations
// ============================================================
// Logic เหมือนเดิมทุกอย่าง เปลี่ยนแค่ขนาด type

void set_bit_u16(uint16_t *reg, uint8_t pos)
{
    *reg |= (uint16_t)(1U << pos);
}

void clear_bit_u16(uint16_t *reg, uint8_t pos)
{
    *reg &= (uint16_t)~(1U << pos);
}

void toggle_bit_u16(uint16_t *reg, uint8_t pos)
{
    *reg ^= (uint16_t)(1U << pos);
}

uint8_t read_bit_u16(uint16_t reg, uint8_t pos)
{
    return (uint8_t)((reg >> pos) & 1U);
}

// ============================================================
// 32-bit operations
// ============================================================
// สำคัญ: ใช้ 1UL (unsigned long) แทน 1U
// เพราะถ้า pos สูง (เช่น 31) แล้วใช้แค่ 1U อาจจะ overflow
// ใน platform ที่ int = 16-bit (ไม่ใช่ปัญหากับ PC แต่อาจเป็นปัญหากับ MCU เก่าๆ)
// ใช้ 1UL ปลอดภัยที่สุด

void set_bit_u32(uint32_t *reg, uint8_t pos)
{
    *reg |= (1UL << pos);
}

void clear_bit_u32(uint32_t *reg, uint8_t pos)
{
    *reg &= ~(1UL << pos);
}

void toggle_bit_u32(uint32_t *reg, uint8_t pos)
{
    *reg ^= (1UL << pos);
}

uint8_t read_bit_u32(uint32_t reg, uint8_t pos)
{
    return (uint8_t)((reg >> pos) & 1UL);
}
