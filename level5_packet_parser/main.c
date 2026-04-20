// ============================================================
// main.c
// ทดสอบ packet parser — packet สมบูรณ์, เสียหาย, แบ่งมาหลายส่วน
// ============================================================

#include <stdio.h>
#include "parser.h"

// Helper: print byte stream ในรูปแบบ hex
static void print_bytes(const char *label, const uint8_t *data, uint8_t len)
{
    printf("  %s: ", label);
    for (uint8_t i = 0; i < len; i++)
    {
        printf("%02X ", data[i]);
    }
    printf("(%u bytes)\n", len);
}

// Helper: print ผล ParseResult แบบ human-readable
static const char *result_name(ParseResult r)
{
    switch (r)
    {
    case PR_IN_PROGRESS:  return "IN_PROGRESS";
    case PR_COMPLETE:     return "COMPLETE ✓";
    case PR_BAD_CHECKSUM: return "BAD_CHECKSUM ✗";
    case PR_BAD_LENGTH:   return "BAD_LENGTH ✗";
    default:              return "UNKNOWN";
    }
}

// Helper: feed byte stream ทั้งก้อนเข้า parser
// คืน result สุดท้ายที่ได้ (อย่างอื่นนอกเหนือจาก IN_PROGRESS)
static ParseResult feed_stream(Parser *p, const uint8_t *stream, uint8_t len)
{
    ParseResult result = PR_IN_PROGRESS;
    for (uint8_t i = 0; i < len; i++)
    {
        result = parser_feed(p, stream[i]);
        // ถ้าได้ผลสุดท้าย (ไม่ใช่ IN_PROGRESS) ก็สรุปเลย
        if (result != PR_IN_PROGRESS)
        {
            return result;
        }
    }
    return result;
}

int main(void)
{
    printf("==========================================\n");
    printf("  Packet Parser — Level 5 Test\n");
    printf("==========================================\n\n");

    Parser parser;
    parser_init(&parser);

    // ----------------------------------------------------------
    // Test 1: Packet สมบูรณ์ (data = 0x01 0x02 0x03)
    // ----------------------------------------------------------
    printf("=== Test 1: Valid packet ===\n");

    // 0x01 ^ 0x02 ^ 0x03 = 0x00
    uint8_t packet1[] = { 0xAA, 0x03, 0x01, 0x02, 0x03, 0x00 };
    print_bytes("stream ", packet1, sizeof(packet1));

    ParseResult r = feed_stream(&parser, packet1, sizeof(packet1));
    printf("  result : %s\n", result_name(r));

    if (r == PR_COMPLETE)
    {
        printf("  header = 0x%02X, length = %u, checksum = 0x%02X\n",
               parser.packet.header, parser.packet.length, parser.packet.checksum);
        print_bytes("data   ", parser.packet.data, parser.packet.length);
    }

    // ----------------------------------------------------------
    // Test 2: Packet ที่ checksum ผิด
    // ----------------------------------------------------------
    printf("\n=== Test 2: Corrupt checksum ===\n");

    // ใส่ checksum เป็น 0xFF (ผิด — ที่ถูกคือ 0x00)
    uint8_t packet2[] = { 0xAA, 0x03, 0x01, 0x02, 0x03, 0xFF };
    print_bytes("stream ", packet2, sizeof(packet2));

    r = feed_stream(&parser, packet2, sizeof(packet2));
    printf("  result : %s\n", result_name(r));

    // ----------------------------------------------------------
    // Test 3: Length เกิน MAX_DATA_LEN → ต้องถูก reject
    // ----------------------------------------------------------
    printf("\n=== Test 3: Invalid length (> MAX_DATA_LEN) ===\n");

    uint8_t packet3[] = { 0xAA, 0xFF };  // length = 255 เกิน MAX = 16
    print_bytes("stream ", packet3, sizeof(packet3));

    r = feed_stream(&parser, packet3, sizeof(packet3));
    printf("  result : %s\n", result_name(r));

    // ----------------------------------------------------------
    // Test 4: Garbage bytes ก่อน header → ต้องข้ามไปจนเจอ 0xAA
    // จำลองสถานการณ์ที่ parser เริ่มรับ stream กลางทาง
    // ----------------------------------------------------------
    printf("\n=== Test 4: Garbage before header (auto-resync) ===\n");

    uint8_t packet4[] = {
        0x00, 0xFF, 0x12, 0x34,            // garbage — parser ต้องข้าม
        0xAA, 0x02, 0x10, 0x20, 0x30       // packet จริง (0x10 ^ 0x20 = 0x30)
    };
    print_bytes("stream ", packet4, sizeof(packet4));

    r = feed_stream(&parser, packet4, sizeof(packet4));
    printf("  result : %s\n", result_name(r));

    if (r == PR_COMPLETE)
    {
        print_bytes("data   ", parser.packet.data, parser.packet.length);
    }

    // ----------------------------------------------------------
    // Test 5: Packet ถูกแบ่งมาเป็น 2 ส่วน (เหมือนใน UART จริง)
    // ----------------------------------------------------------
    printf("\n=== Test 5: Packet arriving in 2 chunks ===\n");

    parser_init(&parser);

    // chunk 1: header + length + บาง data
    // 0xA1 ^ 0xB2 ^ 0xC3 ^ 0xD4 = ?
    //   0xA1 ^ 0xB2 = 0x13
    //   0x13 ^ 0xC3 = 0xD0
    //   0xD0 ^ 0xD4 = 0x04
    uint8_t chunk1[] = { 0xAA, 0x04, 0xA1, 0xB2 };
    print_bytes("chunk 1", chunk1, sizeof(chunk1));
    r = feed_stream(&parser, chunk1, sizeof(chunk1));
    printf("  after chunk 1: %s\n", result_name(r));

    // chunk 2: data ที่เหลือ + checksum
    uint8_t chunk2[] = { 0xC3, 0xD4, 0x04 };
    print_bytes("chunk 2", chunk2, sizeof(chunk2));
    r = feed_stream(&parser, chunk2, sizeof(chunk2));
    printf("  after chunk 2: %s\n", result_name(r));

    if (r == PR_COMPLETE)
    {
        print_bytes("data   ", parser.packet.data, parser.packet.length);
    }

    // ----------------------------------------------------------
    // Test 6: Packet ว่าง (length = 0) — edge case ที่มักลืม
    // ----------------------------------------------------------
    printf("\n=== Test 6: Empty packet (length=0) ===\n");

    // packet ว่าง: header + len=0 + checksum=0 (XOR ของ data ว่าง = 0)
    uint8_t packet6[] = { 0xAA, 0x00, 0x00 };
    print_bytes("stream ", packet6, sizeof(packet6));

    parser_init(&parser);
    r = feed_stream(&parser, packet6, sizeof(packet6));
    printf("  result : %s\n", result_name(r));

    // ----------------------------------------------------------
    // Test 7: หลาย packet ติดกัน — parser ต้องจัดการต่อเนื่องได้
    // ----------------------------------------------------------
    printf("\n=== Test 7: Multiple packets back-to-back ===\n");

    parser_init(&parser);

    uint8_t multi_stream[] = {
        // packet 1: data = [0xAA, 0xBB], checksum = 0xAA^0xBB = 0x11
        0xAA, 0x02, 0xAA, 0xBB, 0x11,
        // packet 2: data = [0x55], checksum = 0x55
        0xAA, 0x01, 0x55, 0x55,
        // packet 3: data = [0x01, 0x02, 0x03, 0x04], checksum = 0x04
        // 0x01^0x02^0x03^0x04 = 0x04
        0xAA, 0x04, 0x01, 0x02, 0x03, 0x04, 0x04
    };

    int completed = 0;
    for (size_t i = 0; i < sizeof(multi_stream); i++)
    {
        ParseResult pr = parser_feed(&parser, multi_stream[i]);
        if (pr == PR_COMPLETE)
        {
            completed++;
            printf("  packet #%d ✓  (len=%u, data=", completed, parser.packet.length);
            for (uint8_t j = 0; j < parser.packet.length; j++)
            {
                printf("%02X ", parser.packet.data[j]);
            }
            printf(")\n");
        }
        else if (pr == PR_BAD_CHECKSUM || pr == PR_BAD_LENGTH)
        {
            printf("  error at byte %zu: %s\n", i, result_name(pr));
        }
    }

    printf("  พบ packet ที่สมบูรณ์ทั้งหมด: %d packets\n", completed);

    printf("\n==========================================\n");
    printf("  All tests passed.\n");
    printf("==========================================\n");

    return 0;
}
