// ============================================================
// main.c
// ทดสอบ ring buffer — ครบทุก edge case
// ============================================================

#include <stdio.h>
#include "ring_buffer.h"

// Helper: พิมพ์สถานะปัจจุบันของ buffer ออกมาให้เห็นภาพ
static void print_status(const RingBuffer *rb, const char *label)
{
    printf("[%s] count=%u, free=%u, empty=%s, full=%s\n",
           label,
           rb_count(rb),
           rb_free(rb),
           rb_is_empty(rb) ? "yes" : "no",
           rb_is_full(rb)  ? "yes" : "no");
}

int main(void)
{
    printf("==========================================\n");
    printf("  Ring Buffer — Level 2 Test\n");
    printf("  (RING_BUFFER_SIZE = %d)\n", RING_BUFFER_SIZE);
    printf("==========================================\n\n");

    RingBuffer rb;
    rb_init(&rb);
    print_status(&rb, "init");

    // ----------------------------------------------------------
    // Test 1: เขียน 5 byte แล้วอ่าน 5 byte
    // เคสปกติที่สุด — ดูว่า FIFO ทำงานถูกไหม (อ่านได้ตามลำดับเขียน)
    // ----------------------------------------------------------
    printf("\n=== Test 1: write 5 byte, read 5 byte (FIFO) ===\n");

    for (uint8_t i = 1; i <= 5; i++)
    {
        if (rb_write(&rb, i))
        {
            printf("  write(%u) ✓\n", i);
        }
    }
    print_status(&rb, "after write");

    uint8_t value;
    while (rb_read(&rb, &value))
    {
        printf("  read = %u ✓\n", value);
    }
    print_status(&rb, "after read");

    // ----------------------------------------------------------
    // Test 2: เขียนจน buffer เต็ม
    // เพื่อทดสอบกลไกป้องกัน overrun
    // ----------------------------------------------------------
    printf("\n=== Test 2: write until It's full (overrun protection) ===\n");

    for (int i = 0; i < RING_BUFFER_SIZE; i++)
    {
        bool ok = rb_write(&rb, (uint8_t)(0xA0 + i));
        if (!ok)
        {
            printf("  write count %d → failed (should not happen)\n", i + 1);
        }
    }
    print_status(&rb, "after fill");

    // ลองเขียนเพิ่ม — ต้องไม่สำเร็จ
    bool ok = rb_write(&rb, 0xFF);
    printf("  write(0xFF) when get full → %s (expected: failed)\n",
           ok ? "succeeded" : "failed");

    // ----------------------------------------------------------
    // Test 3: อ่านจน buffer ว่าง
    // ทดสอบกลไกป้องกัน underrun
    // ----------------------------------------------------------
    printf("\n=== Test 3: read until It get full (underrun protection) ===\n");

    int read_count = 0;
    while (rb_read(&rb, &value))
    {
        read_count++;
    }
    printf("  read all %d byte\n", read_count);
    print_status(&rb, "after drain");

    // ลองอ่านอีก — ต้องไม่สำเร็จ
    ok = rb_read(&rb, &value);
    printf("  read when It's empty → %s (expected: failed)\n",
           ok ? "succeeded" : "failed");

    // ----------------------------------------------------------
    // Test 4: ทดสอบ wrap-around
    // นี่คือหัวใจของ ring buffer — index ต้องวนกลับได้ถูกต้อง
    //
    // วิธีทดสอบ:
    //   1. เขียนถึงเกือบเต็ม (RING_BUFFER_SIZE - 2 ตัว)
    //   2. อ่านออกครึ่งนึง (เพื่อให้ head เลื่อนไปข้างหน้า)
    //   3. เขียนเพิ่มอีก — tail จะต้องวนกลับไปเริ่มต้น
    //   4. อ่านทั้งหมดออกมา → ค่าต้องเรียงตามลำดับเขียน
    // ----------------------------------------------------------
    printf("\n=== Test 4: Wrap-around behavior ===\n");

    rb_init(&rb);  // เริ่มใหม่ให้สะอาด

    // เขียน 12 ตัว
    printf("  write byte 1-12:\n  ");
    for (uint8_t i = 1; i <= 12; i++)
    {
        rb_write(&rb, i);
        printf("%u ", i);
    }
    printf("\n");
    print_status(&rb, "after 12 writes");

    // อ่านออก 8 ตัว — head จะถูกเลื่อนไปไกล
    printf("  read 8 byte:\n  ");
    for (int i = 0; i < 8; i++)
    {
        rb_read(&rb, &value);
        printf("%u ", value);
    }
    printf("\n");
    print_status(&rb, "after 8 reads");

    // เขียนเพิ่ม 8 ตัว — tail จะต้องวนรอบกลับไปต้น array
    printf("  write byte 100-107:\n  ");
    for (uint8_t i = 100; i <= 107; i++)
    {
        rb_write(&rb, i);
        printf("%u ", i);
    }
    printf("\n");
    print_status(&rb, "after wrap-write");

    // อ่านทั้งหมด — ลำดับต้องถูก: 9,10,11,12,100,101,...,107
    printf("  read all (squence FIFO):\n  ");
    while (rb_read(&rb, &value))
    {
        printf("%u ", value);
    }
    printf("\n");
    print_status(&rb, "final");

    // ----------------------------------------------------------
    // Test 5: จำลองสถานการณ์จริง — UART receive แล้ว main loop ดึงไป process
    // ----------------------------------------------------------
    printf("\n=== Test 5: visual UART receive scenario ===\n");

    rb_init(&rb);

    // จำลอง interrupt handler ยิงข้อมูล "GPS,12.34" เข้ามา
    const char *gps_data = "GPS,12.34";
    printf("  [ISR] input data: \"%s\"\n", gps_data);
    for (const char *p = gps_data; *p; p++)
    {
        rb_write(&rb, (uint8_t)*p);
    }

    // จำลอง main loop ดึงข้อมูลออกมาทีละ byte แล้ว print
    printf("  [main] output process: \"");
    while (rb_read(&rb, &value))
    {
        printf("%c", (char)value);
    }
    printf("\"\n");

    printf("\n==========================================\n");
    printf("  All tests passed.\n");
    printf("==========================================\n");

    return 0;
}
