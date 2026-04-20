// ============================================================
// parser.h
// Public API ของ binary packet protocol parser
// ============================================================
//
// Protocol format (รูปแบบ packet):
//
//   [0xAA] [LEN] [DATA_0] [DATA_1] ... [DATA_N-1] [CHECKSUM]
//    1B     1B    LEN bytes                         1B
//
//   0xAA     = header byte (sync byte) — ใช้ "หาจุดเริ่ม" ของ packet
//   LEN      = จำนวน byte ใน data field (0 ถึง MAX_DATA_LEN)
//   DATA     = payload จริง
//   CHECKSUM = XOR ของ DATA ทุก byte
//
// ตัวอย่าง packet ที่ valid:
//   0xAA  0x03  0x01 0x02 0x03  0x00
//   hdr   len   ←---data---→   checksum
//                              (0x01 ^ 0x02 ^ 0x03 = 0x00)
//
// ทำไม protocol ต้องเป็นแบบนี้?
// = เพราะใน UART/SPI byte stream ไม่มี "ขอบเขต" ของ packet
//   เราจะรู้ได้ยังไงว่า "packet ใหม่เริ่มตรงไหน"?
//
//   คำตอบ: ใช้ header byte คงที่ (0xAA) เป็น marker
//   ถ้า byte ที่รับมาไม่ใช่ 0xAA → ยังไม่เริ่ม packet → ทิ้ง
//   ถ้าเจอ 0xAA → เริ่มอ่าน length → อ่าน data → อ่าน checksum
//
// Parser ของเราก็คือ FSM (ย้อนกลับไป Level 3!)
//   รัฐเริ่มต้น: WAIT_HEADER → เมื่อเจอ 0xAA → GOT_LENGTH
//   GOT_LENGTH → อ่าน len → READING_DATA
//   READING_DATA → เก็บ data ทีละ byte → เมื่อครบ → VERIFY_CHECKSUM
//   VERIFY_CHECKSUM → เช็ค XOR → COMPLETE หรือ ERROR
// ============================================================

#ifndef PARSER_H
#define PARSER_H

#include <stdint.h>
#include <stdbool.h>

#define PACKET_HEADER   0xAAU   // sync byte
#define MAX_DATA_LEN    16      // ขนาด data field สูงสุด

// ============================================================
// Packet — โครงสร้างของ packet ที่ parse เสร็จแล้ว
// ============================================================
//
// __attribute__((packed)) บอก compiler ว่า:
//   "อย่าใส่ padding ระหว่าง field"
//
// ทำไมต้อง packed?
// = ปกติ compiler จะแอบเพิ่ม padding เพื่อให้ field ต่อไปอยู่ใน address
//   ที่ aligned ให้ CPU อ่านเร็วขึ้น (เกี่ยวกับเรื่อง alignment ใน Level 4!)
//
//   แต่ถ้าเราจะ "ทาบ" struct นี้ลงบน byte stream ที่รับมาจาก UART
//   เราต้องการให้ layout ของ field ตรงกับ byte stream เป๊ะๆ
//   → padding จะทำให้ layout เพี้ยน → ข้อมูลอ่านผิด
//
// ข้อเสีย: การเข้าถึง field ที่ไม่ aligned จะช้ากว่า (หรือ crash ใน CPU บางตัว)
// → ในงานจริงบางทีเลือก "copy byte-by-byte จาก raw buffer มาใส่ Packet ปกติ"
//    ปลอดภัยกว่า packed
typedef struct __attribute__((packed))
{
    uint8_t header;                   // ต้องเป็น 0xAA
    uint8_t length;                   // จำนวน byte ใน data
    uint8_t data[MAX_DATA_LEN];       // payload
    uint8_t checksum;                 // XOR ของ data
} Packet;

// ============================================================
// Parser state — FSM ของ parser
// ============================================================
typedef enum
{
    PS_WAIT_HEADER,         // รอ 0xAA
    PS_WAIT_LENGTH,         // รอ byte ต่อไปเป็น length
    PS_READING_DATA,        // กำลังสะสม data bytes
    PS_WAIT_CHECKSUM        // รอ byte checksum
} ParserState;

// ============================================================
// Result code — ผลลัพธ์จากการ feed byte 1 ตัว
// ============================================================
typedef enum
{
    PR_IN_PROGRESS,   // ยังไม่ครบ packet — ป้อน byte ต่อไป
    PR_COMPLETE,      // packet สมบูรณ์! อ่าน checksum ถูกต้องด้วย
    PR_BAD_CHECKSUM,  // packet ครบแต่ checksum ผิด — ทิ้งได้
    PR_BAD_LENGTH     // length เกินกว่า MAX_DATA_LEN — packet เสีย
} ParseResult;

// ============================================================
// Parser — โครงสร้างของ parser instance
// ============================================================
typedef struct
{
    ParserState state;                      // อยู่รัฐไหนของ FSM
    Packet      packet;                     // buffer สำหรับเก็บ packet ที่กำลัง parse
    uint8_t     data_index;                 // นับว่าเก็บ data มาแล้วกี่ byte
} Parser;

// ============================================================
// API
// ============================================================

// เริ่มต้น parser ให้พร้อมรับ packet ใหม่
void parser_init(Parser *p);

// ป้อน 1 byte เข้า parser
// คืนค่า ParseResult บอกสถานะ
//
// วิธีใช้ใน main loop:
//   while (rb_read(&uart_rx, &byte))
//   {
//       ParseResult r = parser_feed(&parser, byte);
//       if (r == PR_COMPLETE)  handle_packet(&parser.packet);
//       if (r == PR_BAD_...)   log_error();
//   }
ParseResult parser_feed(Parser *p, uint8_t byte);

// คำนวณ checksum (XOR ทุก byte) — ใช้ทั้งฝั่งส่งและฝั่งรับ
uint8_t packet_checksum(const uint8_t *data, uint8_t length);

#endif // PARSER_H
