#include "OLED.h"

#include "main.h"
#include <string.h>

#define OLED_WIDTH 128
#define OLED_HEIGHT 64
#define OLED_PAGE_COUNT (OLED_HEIGHT / 8)
#define OLED_I2C_ADDRESS 0x78

#define OLED_SCL_GPIO_PORT GPIOB
#define OLED_SCL_GPIO_PIN GPIO_PIN_13
#define OLED_SDA_GPIO_PORT GPIOB
#define OLED_SDA_GPIO_PIN GPIO_PIN_14

typedef struct
{
    char character;
    uint8_t rows[7];
} OLED_Glyph_t;

static uint8_t display_buffer[OLED_PAGE_COUNT][OLED_WIDTH];

/* Minimal 5x7 glyph set used by the home screen. */
static const OLED_Glyph_t glyphs[] = {
    {' ', {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}},
    {'%', {0x19, 0x1A, 0x04, 0x08, 0x0B, 0x13, 0x00}},
    {'0', {0x0E, 0x11, 0x13, 0x15, 0x19, 0x11, 0x0E}},
    {'1', {0x04, 0x0C, 0x14, 0x04, 0x04, 0x04, 0x1F}},
    {'2', {0x0E, 0x11, 0x01, 0x02, 0x04, 0x08, 0x1F}},
    {'3', {0x1E, 0x01, 0x01, 0x0E, 0x01, 0x01, 0x1E}},
    {'4', {0x02, 0x06, 0x0A, 0x12, 0x1F, 0x02, 0x02}},
    {'5', {0x1F, 0x10, 0x10, 0x1E, 0x01, 0x01, 0x1E}},
    {'6', {0x0E, 0x10, 0x10, 0x1E, 0x11, 0x11, 0x0E}},
    {'7', {0x1F, 0x01, 0x02, 0x04, 0x08, 0x08, 0x08}},
    {'8', {0x0E, 0x11, 0x11, 0x0E, 0x11, 0x11, 0x0E}},
    {'9', {0x0E, 0x11, 0x11, 0x0F, 0x01, 0x01, 0x0E}},
    {':', {0x00, 0x04, 0x04, 0x00, 0x04, 0x04, 0x00}},
    {'D', {0x1E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x1E}},
    {'F', {0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x10}},
    {'O', {0x0E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E}},
};

static void OLED_I2C_Delay(void)
{
    volatile uint8_t delay_count;

    for (delay_count = 0; delay_count < 8; delay_count++)
    {
        __NOP();
    }
}

static void OLED_WriteSCL(GPIO_PinState state)
{
    HAL_GPIO_WritePin(OLED_SCL_GPIO_PORT, OLED_SCL_GPIO_PIN, state);
    OLED_I2C_Delay();
}

static void OLED_WriteSDA(GPIO_PinState state)
{
    HAL_GPIO_WritePin(OLED_SDA_GPIO_PORT, OLED_SDA_GPIO_PIN, state);
    OLED_I2C_Delay();
}

static void OLED_I2C_Start(void)
{
    OLED_WriteSDA(GPIO_PIN_SET);
    OLED_WriteSCL(GPIO_PIN_SET);
    OLED_WriteSDA(GPIO_PIN_RESET);
    OLED_WriteSCL(GPIO_PIN_RESET);
}

static void OLED_I2C_Stop(void)
{
    OLED_WriteSDA(GPIO_PIN_RESET);
    OLED_WriteSCL(GPIO_PIN_SET);
    OLED_WriteSDA(GPIO_PIN_SET);
}

static void OLED_I2C_WriteByte(uint8_t value)
{
    uint8_t bit_index;

    for (bit_index = 0; bit_index < 8; bit_index++)
    {
        OLED_WriteSDA(
            (value & (0x80U >> bit_index)) != 0U
                ? GPIO_PIN_SET
                : GPIO_PIN_RESET);
        OLED_WriteSCL(GPIO_PIN_SET);
        OLED_WriteSCL(GPIO_PIN_RESET);
    }

    OLED_WriteSDA(GPIO_PIN_SET);
    OLED_WriteSCL(GPIO_PIN_SET);
    OLED_WriteSCL(GPIO_PIN_RESET);
}

static void OLED_WritePacket(uint8_t control, uint8_t value)
{
    OLED_I2C_Start();
    OLED_I2C_WriteByte(OLED_I2C_ADDRESS);
    OLED_I2C_WriteByte(control);
    OLED_I2C_WriteByte(value);
    OLED_I2C_Stop();
}

static void OLED_WriteCommand(uint8_t command)
{
    OLED_WritePacket(0x00, command);
}

static void OLED_WriteData(uint8_t data)
{
    OLED_WritePacket(0x40, data);
}

static const OLED_Glyph_t *OLED_FindGlyph(char character)
{
    uint8_t glyph_index;

    for (glyph_index = 0;
         glyph_index < sizeof(glyphs) / sizeof(glyphs[0]);
         glyph_index++)
    {
        if (glyphs[glyph_index].character == character)
        {
            return &glyphs[glyph_index];
        }
    }

    return &glyphs[0];
}

static void OLED_DrawGlyph(
    int16_t x,
    int16_t y,
    const OLED_Glyph_t *glyph,
    uint8_t font_size)
{
    uint8_t row;
    uint8_t column;

    for (row = 0; row < 7; row++)
    {
        for (column = 0; column < 5; column++)
        {
            if ((glyph->rows[row] & (0x10U >> column)) == 0U)
            {
                continue;
            }

            if (font_size == OLED_8X16)
            {
                OLED_DrawPoint(x + column, y + row * 2);
                OLED_DrawPoint(x + column, y + row * 2 + 1);
            }
            else
            {
                OLED_DrawPoint(x + column, y + row);
            }
        }
    }
}

void OLED_Init(void)
{
    HAL_Delay(100);

    OLED_WriteCommand(0xAE);
    OLED_WriteCommand(0xD5);
    OLED_WriteCommand(0x80);
    OLED_WriteCommand(0xA8);
    OLED_WriteCommand(0x3F);
    OLED_WriteCommand(0xD3);
    OLED_WriteCommand(0x00);
    OLED_WriteCommand(0x40);
    OLED_WriteCommand(0xA1);
    OLED_WriteCommand(0xC8);
    OLED_WriteCommand(0xDA);
    OLED_WriteCommand(0x12);
    OLED_WriteCommand(0x81);
    OLED_WriteCommand(0xCF);
    OLED_WriteCommand(0xD9);
    OLED_WriteCommand(0xF1);
    OLED_WriteCommand(0xDB);
    OLED_WriteCommand(0x30);
    OLED_WriteCommand(0xA4);
    OLED_WriteCommand(0xA6);
    OLED_WriteCommand(0x8D);
    OLED_WriteCommand(0x14);
    OLED_WriteCommand(0xAF);

    OLED_Clear();
    OLED_Update();
}

void OLED_Clear(void)
{
    memset(display_buffer, 0, sizeof(display_buffer));
}

void OLED_Update(void)
{
    uint8_t page;
    uint8_t column;

    for (page = 0; page < OLED_PAGE_COUNT; page++)
    {
        OLED_WriteCommand(0xB0U | page);
        OLED_WriteCommand(0x00);
        OLED_WriteCommand(0x10);

        for (column = 0; column < OLED_WIDTH; column++)
        {
            OLED_WriteData(display_buffer[page][column]);
        }
    }
}

void OLED_DrawPoint(int16_t x, int16_t y)
{
    if (x < 0 || x >= OLED_WIDTH || y < 0 || y >= OLED_HEIGHT)
    {
        return;
    }

    display_buffer[y / 8][x] |= (uint8_t)(1U << (y % 8));
}

void OLED_DrawRectangle(
    int16_t x,
    int16_t y,
    uint8_t width,
    uint8_t height,
    uint8_t filled)
{
    int16_t current_x;
    int16_t current_y;

    for (current_y = y; current_y < y + height; current_y++)
    {
        for (current_x = x; current_x < x + width; current_x++)
        {
            if (filled == OLED_FILLED ||
                current_x == x || current_x == x + width - 1 ||
                current_y == y || current_y == y + height - 1)
            {
                OLED_DrawPoint(current_x, current_y);
            }
        }
    }
}

void OLED_ShowString(
    int16_t x,
    int16_t y,
    const char *string,
    uint8_t font_size)
{
    int16_t cursor_x = x;

    while (*string != '\0')
    {
        OLED_DrawGlyph(cursor_x, y, OLED_FindGlyph(*string), font_size);
        cursor_x += font_size;
        string++;
    }
}
