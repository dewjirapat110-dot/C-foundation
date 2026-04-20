// ============================================================
// parser.c
// Implementation ของ binary packet parser
// ใช้แนวคิด FSM เดียวกับ Level 3
// ============================================================

#include "parser.h"

// ============================================================
// packet_checksum — XOR ทุก byte ของ data
// ============================================================
//
// XOR เป็นวิธี checksum ที่ง่ายและเร็วมาก
//   0 ^ 0 = 0
//   0 ^ 1 = 1
//   1 ^ 0 = 1
//   1 ^ 1 = 0
//
// คุณสมบัติสำคัญ: a ^ a = 0 และ a ^ 0 = a
// → ถ้า XOR ทุก byte รวมถึง checksum ผลลัพธ์ต้องเป็น 0
//   (ถ้า data ครบถ้วนไม่เพี้ยน)
//
// ข้อจำกัดของ XOR checksum:
// = ถ้ามี bit สองตัวในตำแหน่งเดียวกันแต่คนละ byte ถูก flip พร้อมกัน
//   → XOR ยังคงเหมือนเดิม → ตรวจไม่เจอ
// → ในงานจริงที่ต้องการความเชื่อถือสูง ใช้ CRC-8/16/32 แทน
//   (ซับซ้อนกว่า แต่จับ error ได้ดีกว่ามาก)
uint8_t packet_checksum(const uint8_t *data, uint8_t length)
{
    uint8_t sum = 0;
    for (uint8_t i = 0; i < length; i++)
    {
        sum ^= data[i];
    }
    return sum;
}

// ============================================================
// parser_init — เริ่มต้น parser
// ============================================================
void parser_init(Parser *p)
{
    p->state      = PS_WAIT_HEADER;
    p->data_index = 0;

    // ไม่จำเป็นต้องเคลียร์ packet — จะถูกเขียนทับเมื่อ parse packet ใหม่
    // แต่เขียนเคลียร์ก็ไม่เสียหาย (เพื่อ debug ง่ายขึ้น)
    p->packet.header   = 0;
    p->packet.length   = 0;
    p->packet.checksum = 0;
}

// ============================================================
// parser_feed — ป้อน 1 byte แล้วเดิน FSM
// ============================================================
//
// นี่คือหัวใจของ protocol parser
// แต่ละ state รับ byte 1 ตัว แล้วตัดสินใจว่าจะ:
//   - เก็บค่า (transition ไป state ใหม่)
//   - เพิกเฉย (stay ใน state เดิม)
//   - หรือ reset ถ้าเจอ error
ParseResult parser_feed(Parser *p, uint8_t byte)
{
    switch (p->state)
    {
    // ---------------------------------------------------
    // WAIT_HEADER — รอ sync byte (0xAA)
    // ---------------------------------------------------
    // ถ้า byte ไม่ใช่ 0xAA → ทิ้งและอยู่ state เดิม
    // นี่คือวิธี "resync" กับ byte stream ได้เอง
    // ถ้าเริ่มต้นรับ stream กลางทาง → ทิ้งจน
    // เจอ 0xAA ตัวต่อไป → เริ่ม packet ถัดไป
    case PS_WAIT_HEADER:
        if (byte == PACKET_HEADER)
        {
            p->packet.header = byte;
            p->state         = PS_WAIT_LENGTH;
        }
        return PR_IN_PROGRESS;

    // ---------------------------------------------------
    // WAIT_LENGTH — byte นี้ = จำนวน data bytes
    // ---------------------------------------------------
    case PS_WAIT_LENGTH:
        // เช็คก่อน — ป้องกัน buffer overflow ถ้า length ใหญ่เกิน
        if (byte > MAX_DATA_LEN)
        {
            parser_init(p);       // reset
            return PR_BAD_LENGTH;
        }

        p->packet.length = byte;
        p->data_index    = 0;

        // ถ้า length = 0 (packet ว่าง) → ข้ามไปรอ checksum เลย
        if (byte == 0)
        {
            p->state = PS_WAIT_CHECKSUM;
        }
        else
        {
            p->state = PS_READING_DATA;
        }
        return PR_IN_PROGRESS;

    // ---------------------------------------------------
    // READING_DATA — สะสม data bytes
    // ---------------------------------------------------
    case PS_READING_DATA:
        p->packet.data[p->data_index] = byte;
        p->data_index++;

        // ครบ length แล้วหรือยัง?
        if (p->data_index >= p->packet.length)
        {
            p->state = PS_WAIT_CHECKSUM;
        }
        return PR_IN_PROGRESS;

    // ---------------------------------------------------
    // WAIT_CHECKSUM — byte สุดท้ายของ packet
    // ---------------------------------------------------
    case PS_WAIT_CHECKSUM:
        p->packet.checksum = byte;

        // คำนวณ checksum จาก data ที่เก็บมา
        uint8_t expected = packet_checksum(p->packet.data, p->packet.length);

        // รีเซ็ต state เพื่อรอ packet ถัดไป (ไม่ว่าผลจะเป็นอย่างไร)
        p->state      = PS_WAIT_HEADER;
        p->data_index = 0;

        if (expected == byte)
        {
            return PR_COMPLETE;       // ผ่าน! packet valid
        }
        else
        {
            return PR_BAD_CHECKSUM;   // ข้อมูลเพี้ยน
        }

    // ---------------------------------------------------
    // Default — ไม่ควรเกิด แต่ป้องกันไว้
    // ---------------------------------------------------
    default:
        parser_init(p);
        return PR_IN_PROGRESS;
    }
}
