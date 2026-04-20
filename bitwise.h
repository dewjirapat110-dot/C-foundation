// ============================================================
// bitwise.h
// Public API ของ library สำหรับจัดการ bit ใน register
// ============================================================
//
// ทำไมต้องมีไฟล์ .h ?
// - เพื่อแยก "interface" (ฟังก์ชันมีอะไรให้ใช้บ้าง)
//   ออกจาก "implementation" (ข้างในทำงานยังไง)
// - คนที่จะใช้ library นี้แค่ #include "bitwise.h" ก็พอ
//   ไม่ต้องสนใจว่าข้างใน .c เขียนยังไง
// ============================================================

#ifndef BITWISE_H
#define BITWISE_H
// ↑ Header guard — ป้องกันไม่ให้ไฟล์นี้ถูก include ซ้ำ
//   ถ้าถูก include หลายครั้ง compiler จะ error ว่ามี declaration ซ้ำ

#include <stdint.h>
// ↑ ดึง type uint8_t, uint16_t, uint32_t มาใช้
//   พวกนี้คือ "fixed-width type" — ขนาดชัดเจนทุก platform
//   uint8_t  = 1 byte (8 bits)  เหมาะกับ register 8-bit
//   uint16_t = 2 bytes (16 bits) เหมาะกับ register 16-bit
//   uint32_t = 4 bytes (32 bits) เหมาะกับ register 32-bit เช่น GPIO ของ STM32

// ============================================================
// API สำหรับ register 8-bit
// ============================================================
// ใช้ pointer (uint8_t*) เพราะเราต้อง "แก้ค่า" ของ register
// ไม่ใช่แค่ "อ่าน" — ถ้าใช้ uint8_t reg ธรรมดา จะแก้แค่ copy
// (ยกเว้น read_bit ที่ใช้ค่าตรงๆ ได้ เพราะแค่อ่านอย่างเดียว)
void    set_bit_u8   (uint8_t *reg, uint8_t pos);  // เซ็ต bit ที่ตำแหน่ง pos เป็น 1
void    clear_bit_u8 (uint8_t *reg, uint8_t pos);  // ล้าง bit ที่ตำแหน่ง pos เป็น 0
void    toggle_bit_u8(uint8_t *reg, uint8_t pos);  // กลับค่า bit (0→1, 1→0)
uint8_t read_bit_u8  (uint8_t  reg, uint8_t pos);  // อ่านค่า bit ที่ตำแหน่ง pos (return 0 หรือ 1)

// ============================================================
// API สำหรับ register 16-bit
// ============================================================
void    set_bit_u16   (uint16_t *reg, uint8_t pos);
void    clear_bit_u16 (uint16_t *reg, uint8_t pos);
void    toggle_bit_u16(uint16_t *reg, uint8_t pos);
uint8_t read_bit_u16  (uint16_t  reg, uint8_t pos);

// ============================================================
// API สำหรับ register 32-bit
// ============================================================
// 32-bit คือขนาดที่พบบ่อยที่สุดใน MCU สมัยใหม่ เช่น STM32, ESP32
void    set_bit_u32   (uint32_t *reg, uint8_t pos);
void    clear_bit_u32 (uint32_t *reg, uint8_t pos);
void    toggle_bit_u32(uint32_t *reg, uint8_t pos);
uint8_t read_bit_u32  (uint32_t  reg, uint8_t pos);

#endif // BITWISE_H
