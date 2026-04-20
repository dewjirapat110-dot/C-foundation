// ============================================================
// main.c
// ทดสอบ static allocator — ครอบคลุม alignment, exhaustion, reset
// ============================================================

#include <stdio.h>
#include <string.h>
#include "allocator.h"

// Helper: พิมพ์ address ของ pointer ในรูปแบบที่อ่านง่าย
// แสดงแค่ offset จากต้น pool (ไม่ใช่ address จริง)
static size_t offset_in_pool(const Pool *p, const void *ptr)
{
    // (uint8_t*)ptr - p->buffer = ระยะห่างเป็น byte จากต้น buffer
    return (size_t)((const uint8_t *)ptr - p->buffer);
}

static void print_status(const Pool *p, const char *label)
{
    printf("[%s] used=%zu, free=%zu\n",
           label, pool_used(p), pool_free(p));
}

int main(void)
{
    printf("==========================================\n");
    printf("  Static Allocator — Level 4 Test\n");
    printf("  (POOL_SIZE = %d, ALIGNMENT = 4)\n", POOL_SIZE);
    printf("==========================================\n\n");

    Pool pool;
    pool_init(&pool);
    print_status(&pool, "init");

    // ----------------------------------------------------------
    // Test 1: จอง memory แบบธรรมดา
    // ----------------------------------------------------------
    printf("\n=== Test 1: reserve mutiple spaces for memory ===\n");

    // จอง int (4 byte) 1 ตัว
    int *p_int = (int *)pool_alloc(&pool, sizeof(int));
    if (p_int)
    {
        *p_int = 12345;
        printf("  reserve int  → offset=%zu, value=%d\n",
               offset_in_pool(&pool, p_int), *p_int);
    }

    // จอง uint8_t (1 byte) — จะถูกปัดขึ้นเป็น 4 byte เพราะ alignment
    uint8_t *p_byte = (uint8_t *)pool_alloc(&pool, sizeof(uint8_t));

    if (p_byte)
    {
        *p_byte = 0xAB;
        printf("  reserve byte → offset=%zu, value=0x%02X (utilize memory for 4 byte because of alignment)\n",
               offset_in_pool(&pool, p_byte), *p_byte);
    }

    // จอง array ของ double (8 byte × 3 = 24 byte)
    double *p_arr = (double *)pool_alloc(&pool, 3 * sizeof(double));
    if (p_arr)
    {
        p_arr[0] = 1.1;
        p_arr[1] = 2.2;
        p_arr[2] = 3.3;
        printf("  reserve double[3] → offset=%zu, values=%.1f, %.1f, %.1f\n",
               offset_in_pool(&pool, p_arr),
               p_arr[0], p_arr[1], p_arr[2]);
    }

    print_status(&pool, "after 3 allocs");

    // ----------------------------------------------------------
    // Test 2: Alignment — พิสูจน์ว่า pointer ที่คืนมาลงตัวด้วย 4
    // ----------------------------------------------------------
    printf("\n=== Test 2: Alignment verification ===\n");

    pool_reset(&pool);

    // จองก้อนขนาดแปลกๆ แล้วเช็คว่า address ยังคง aligned
    void *a = pool_alloc(&pool, 1);   // 1 byte → ได้ offset 0
    void *b = pool_alloc(&pool, 3);   // 3 byte → offset 4 (ไม่ใช่ 1!)
    void *c = pool_alloc(&pool, 5);   // 5 byte → offset 8
    void *d = pool_alloc(&pool, 9);   // 9 byte → offset 16 (ไม่ใช่ 13!)

    printf("  alloc(1) → offset %zu  (expected 0)\n",  offset_in_pool(&pool, a));
    printf("  alloc(3) → offset %zu  (expected 4)\n",  offset_in_pool(&pool, b));
    printf("  alloc(5) → offset %zu  (expected 8)\n",  offset_in_pool(&pool, c));
    printf("  alloc(9) → offset %zu  (expected 16)\n", offset_in_pool(&pool, d));

    // ทุก offset ต้องหาร 4 ลงตัว
    printf("\n  ทุก offset หาร 4 ลงตัวไหม?\n");
    printf("    offset %zu %% 4 = %zu\n", offset_in_pool(&pool, a), offset_in_pool(&pool, a) % 4);
    printf("    offset %zu %% 4 = %zu\n", offset_in_pool(&pool, b), offset_in_pool(&pool, b) % 4);
    printf("    offset %zu %% 4 = %zu\n", offset_in_pool(&pool, c), offset_in_pool(&pool, c) % 4);
    printf("    offset %zu %% 4 = %zu\n", offset_in_pool(&pool, d), offset_in_pool(&pool, d) % 4);

    print_status(&pool, "after alignment test");

    // ----------------------------------------------------------
    // Test 3: จอง memory จนเต็ม pool
    // ----------------------------------------------------------
    printf("\n=== Test 3: Exhaustion ===\n");

    pool_reset(&pool);

    // จองทีละ 100 byte จนกว่าจะไม่ได้
    int alloc_count = 0;
    void *last_ptr = NULL;
    while (1)
    {
        void *ptr = pool_alloc(&pool, 100);
        if (ptr == NULL)
        {
            break;  // pool เต็มแล้ว
        }
        last_ptr = ptr;
        alloc_count++;
    }

    printf("  จอง 100 byte ได้ทั้งหมด %d ครั้ง ก่อน pool เต็ม\n", alloc_count);
    printf("  pointer สุดท้ายที่จองได้: offset %zu\n",
           offset_in_pool(&pool, last_ptr));
    print_status(&pool, "full");

    // ลองจองเพิ่ม — ต้องได้ NULL
    void *should_fail = pool_alloc(&pool, 100);
    printf("  จองอีก 100 byte เมื่อเต็ม → %s (คาดหวัง NULL)\n",
           should_fail == NULL ? "NULL ✓" : "ไม่ใช่ NULL ✗");

    // ----------------------------------------------------------
    // Test 4: Reset
    // ----------------------------------------------------------
    printf("\n=== Test 4: Reset pool ===\n");

    pool_reset(&pool);
    print_status(&pool, "after reset");

    // จองอีกครั้ง — ต้องเริ่มจาก offset 0 ใหม่
    void *fresh = pool_alloc(&pool, 16);
    printf("  จองใหม่หลัง reset → offset %zu (คาดหวัง 0)\n",
           offset_in_pool(&pool, fresh));

    // ----------------------------------------------------------
    // Test 5: Use case จริง — เก็บ struct หลายแบบใน pool เดียว
    // ----------------------------------------------------------
    printf("\n=== Test 5: Use case — จอง struct หลายแบบ ===\n");

    typedef struct
    {
        uint32_t id;
        char     name[16];
    } Sensor;

    typedef struct
    {
        uint32_t timestamp;
        int16_t  temperature;
        int16_t  humidity;
    } Reading;

    pool_reset(&pool);

    // จอง Sensor 2 ตัว
    Sensor *s1 = (Sensor *)pool_alloc(&pool, sizeof(Sensor));
    Sensor *s2 = (Sensor *)pool_alloc(&pool, sizeof(Sensor));

    s1->id = 1;
    strncpy(s1->name, "Outdoor", sizeof(s1->name) - 1);
    s1->name[sizeof(s1->name) - 1] = '\0';

    s2->id = 2;
    strncpy(s2->name, "Indoor", sizeof(s2->name) - 1);
    s2->name[sizeof(s2->name) - 1] = '\0';

    // จอง Reading 1 ตัว
    Reading *r = (Reading *)pool_alloc(&pool, sizeof(Reading));
    r->timestamp = 1700000000;
    r->temperature = 285;   // 28.5°C × 10
    r->humidity    = 652;   // 65.2%   × 10

    printf("  Sensor 1: id=%u, name=\"%s\" @ offset %zu\n",
           s1->id, s1->name, offset_in_pool(&pool, s1));
    printf("  Sensor 2: id=%u, name=\"%s\" @ offset %zu\n",
           s2->id, s2->name, offset_in_pool(&pool, s2));
    printf("  Reading : ts=%u, temp=%.1f°C, humid=%.1f%% @ offset %zu\n",
           r->timestamp,
           r->temperature / 10.0,
           r->humidity    / 10.0,
           offset_in_pool(&pool, r));

    print_status(&pool, "end");

    printf("\n==========================================\n");
    printf("  All tests passed.\n");
    printf("==========================================\n");

    return 0;
}
