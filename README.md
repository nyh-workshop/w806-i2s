# W806-i2s

Basic I2S and DMA demonstration for the W806 board. This demo generates a 440Hz sine wave using DDS method and with the sampling rate of 44.1kHz.

![image](https://user-images.githubusercontent.com/20377029/181915702-c964b774-8323-4f3b-bf5e-7765cfd731ab.png)

Wiring table to connect this to PCM5102 board (you can find these in AliExpress):

|W806|PCM5102|
|---|---
|PB8 (BCK)|BCK|
|PB9 (LRCK)|LCK|
|PB11 (DO)|DIN|

***Attention: Make sure the PCM5102 board's FMT pin is grounded! Else there will be no or wrong output!***

## Issues:
- In the current W806-SDK (*d2625af*) the "Transfer Complete" callback is disabled. Please re-enable it by uncommenting it in the "**wm-sdk-w806\platform\drivers\wm_i2s.c**" before compiling it:
```
static void I2S_DMATxCplt(DMA_HandleTypeDef *hdma)
{
    I2S_HandleTypeDef *hi2s = (I2S_HandleTypeDef *)((DMA_HandleTypeDef *)hdma)->Parent;
    
    if ((hdma->Init.Mode == DMA_MODE_NORMAL_SINGLE) || (hdma->Init.Mode == DMA_MODE_LINK_SINGLE))
    {
        CLEAR_BIT(hi2s->Instance->CR, I2S_CR_TXDMA_EN);
    //    __HAL_I2S_DISABLE_TX(hi2s);
        hi2s->TxXferCount = 0;
    //    hi2s->State = HAL_I2S_STATE_READY;
    }
//    HAL_I2S_TxCpltCallback(hi2s); // <- this callback has been disabled in the code.
}
```

- There are attempts to put the enums for "empty flag" (bEmptyFlag) and "buffer select" (bSelect), however the optimizer seemed to remove them from the O1 onwards. Using a basic volatile uint8_t fixes this temporarily.

- The sine table is precalculated using constexpr with math functions, and this requires a C++14 and beyond. In the **wm-sdk-w806\tools\W806\conf.mk**, please change the config so that it is using "***gnu++14***" instead:
```
CCXXFLAGS := -Wall \
    -DTLS_CONFIG_CPU_XT804=1 \
    -DGCC_COMPILE=1 \
    -mcpu=$(cputype) \
    $(optimization) \
    -std=gnu++14 \
    -c  \
    -mhard-float  \
    $(extra_flag)  \
    -fdata-sections  \
    -ffunction-sections
```

- The printf("<") (fill lower half buffer) and printf(">") (fill upper half buffer) is left there as debugging points. A sine wave that is correctly generated will show a uniform "<><><>" (fill lower, upper, lower, upper, etc...) instead of "<<<>>>" (or other variations of it).

- The pin toggles in the I2S DMA half-transfer and transfer complete callbacks are placed to check if these interrupts are fired correctly. The original idea is to put a short pulse there, however this pulse is too short to be seen, so toggling these would be much more easier to view. (rising and falling edges seen are when the interrupts are called)
