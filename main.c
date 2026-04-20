// ============================================================
// main.c
// ทดสอบ library bitwise — จำลอง register ขนาดต่างๆ
// ============================================================

#include <stdio.h>
#include <stdint.h>
#include "bitwise.h"

// Helper: พิมพ์ค่า uint8_t ในรูปแบบ binary เพื่อให้เห็นภาพ
//
// ทำไมถึงต้องเขียนเอง?
// เพราะ printf ไม่มี format specifier %b มาให้ใน C standard
// (มีแค่ %d %x %o ฯลฯ — ไม่มี binary)
static void print_binary_u8(uint8_t value)
{
    // วน 8 รอบ จาก bit สูงสุด (bit 7) ไปถึงต่ำสุด (bit 0)
    for (int i = 7; i >= 0; i--)
    {
        // shift bit ที่สนใจมาที่ตำแหน่ง 0 แล้ว AND 1 เพื่ออ่านค่า
        printf("%d", (value >> i) & 1);
        // ใส่ space กลางคำเพื่ออ่านง่าย: "1010 1010"
        if (i == 4) printf(" ");
    }
}

static void print_binary_u16(uint16_t value)
{
    // พิมพ์เป็นสองครึ่ง (high byte + low byte) ใช้ฟังก์ชันเดิมได้
    print_binary_u8((uint8_t)(value >> 8));   // high byte (bit 15..8)
    printf(" ");
    print_binary_u8((uint8_t)(value & 0xFF)); // low byte (bit 7..0)
}

static void print_binary_u32(uint32_t value)
{
    // พิมพ์ทีละ byte ทั้ง 4 byte
    for (int i = 3; i >= 0; i--)
    {
        print_binary_u8((uint8_t)((value >> (i * 8)) & 0xFF));
        if (i > 0) printf(" ");
    }
}

int main(void)
{
    printf("==========================================\n");
    printf("  Bitwise Toolkit — Level 1 Test\n");
    printf("==========================================\n\n");

    // ----------------------------------------------------------
    // Test 1: 8-bit register
    // จำลอง GPIO Output Data Register (ODR) ของ MCU 8 ขา
    // ----------------------------------------------------------
    printf("=== Test 1: 8-bit register (เช่น GPIO 8 ขา) ===\n");

    uint8_t gpio = 0x00;  // เริ่มต้น = 0000 0000 (ทุกขา LOW)
    printf("start:        ");
    print_binary_u8(gpio);
    printf("  (0x%02X)\n", gpio);

    // เปิด LED ที่ขา 0 (set bit 0 = 1)
    set_bit_u8(&gpio, 0);
    printf("set bit 0:      ");
    print_binary_u8(gpio);
    printf("  (0x%02X)\n", gpio);

    // เปิด LED ที่ขา 3 และ 7 ด้วย
    set_bit_u8(&gpio, 3);
    set_bit_u8(&gpio, 7);
    printf("set bit 3, 7:   ");
    print_binary_u8(gpio);
    printf("  (0x%02X)\n", gpio);

    // ปิด LED ที่ขา 0
    clear_bit_u8(&gpio, 0);
    printf("clear bit 0:    ");
    print_binary_u8(gpio);
    printf("  (0x%02X)\n", gpio);

    // toggle ขา 3 (ที่เปิดอยู่ → ปิด)
    toggle_bit_u8(&gpio, 3);
    printf("toggle bit 3:   ");
    print_binary_u8(gpio);
    printf("  (0x%02X)\n", gpio);

    // อ่านค่าขา 7 (ที่เปิดอยู่)
    printf("read bit 7  = %u   (expected 1)\n", read_bit_u8(gpio, 7));
    printf("read bit 5  = %u   (expected 0)\n", read_bit_u8(gpio, 5));

    // ----------------------------------------------------------
    // Test 2: 16-bit register
    // จำลอง timer counter
    // ----------------------------------------------------------
    printf("\n=== Test 2: 16-bit register ===\n");

    uint16_t timer_ctrl = 0x0000;
    printf("start:         ");
    print_binary_u16(timer_ctrl);
    printf("  (0x%04X)\n", timer_ctrl);

    // เปิด timer (bit 0) และเปิด interrupt (bit 8)
    set_bit_u16(&timer_ctrl, 0);
    set_bit_u16(&timer_ctrl, 8);
    printf("set bit 0, 8:    ");
    print_binary_u16(timer_ctrl);
    printf("  (0x%04X)\n", timer_ctrl);

    // toggle bit 15 (high bit)
    toggle_bit_u16(&timer_ctrl, 15);
    printf("toggle bit 15:   ");
    print_binary_u16(timer_ctrl);
    printf("  (0x%04X)\n", timer_ctrl);

    // ----------------------------------------------------------
    // Test 3: 32-bit register
    // จำลอง GPIO ของ STM32 (32-bit) — ขนาดที่พบบ่อยใน MCU สมัยใหม่
    // ----------------------------------------------------------
    printf("\n=== Test 3: 32-bit register ===\n");

    uint32_t gpio32 = 0x00000000;
    printf("start:                  ");
    print_binary_u32(gpio32);
    printf("\n");
    printf("                       (0x%08X)\n", gpio32);

    // เปิด bit 0, 5, 16, 31 (กระจายทั้ง 4 byte)
    set_bit_u32(&gpio32, 0);
    set_bit_u32(&gpio32, 5);
    set_bit_u32(&gpio32, 16);
    set_bit_u32(&gpio32, 31);
    printf("set bit 0, 5, 16, 31:  ");
    print_binary_u32(gpio32);
    printf("\n");
    printf("                       (0x%08X)\n", gpio32);

    // อ่านค่าทั้งหมด
    printf("read bit 31 = %u    (expected 1)\n", read_bit_u32(gpio32, 31));
    printf("read bit 16 = %u    (expected 1)\n", read_bit_u32(gpio32, 16));
    printf("read bit 10 = %u    (expected 0)\n", read_bit_u32(gpio32, 10));

    // ----------------------------------------------------------
    // Test 4: ตัวอย่างการใช้งานจริง — เลียนแบบ STM32 GPIO
    // ----------------------------------------------------------
    printf("\n=== Test 4: wrok as STM32 — turn on LED at PA5 ===\n");

    // ใน STM32 จริง: GPIOA->ODR |= (1 << 5);
    // ใน toolkit ของเรา: set_bit_u32(&GPIOA_ODR, 5);
    uint32_t GPIOA_ODR = 0x00000000;
    set_bit_u32(&GPIOA_ODR, 5);
    printf("LED on PA5 ON  → ODR = 0x%08X (bit 5 = %u)\n",
           GPIOA_ODR, read_bit_u32(GPIOA_ODR, 5));

    // ดับ LED
    clear_bit_u32(&GPIOA_ODR, 5);
    printf("LED on PA5 OFF → ODR = 0x%08X (bit 5 = %u)\n",
           GPIOA_ODR, read_bit_u32(GPIOA_ODR, 5));

    printf("\n==========================================\n");
    printf("  All tests passed.\n");
    printf("==========================================\n");

    return 0;
}
