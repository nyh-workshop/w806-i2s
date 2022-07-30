
#include <stdio.h>
#include "wm_hal.h"
#include <cmath>

I2S_HandleTypeDef hi2s;
DMA_HandleTypeDef hdma_i2s_tx;
DMA_HandleTypeDef hdma_i2s_rx;

extern "C" void Error_Handler(void);

static void GPIO_Init(void);
static void I2S_Init(void);
static void DMA_Init(void);
void Error_Handler(void);

constexpr uint32_t I2S_BUFFER_SIZE = 512; 

uint32_t tx_buf[I2S_BUFFER_SIZE];

struct i2sBuffer
{
    void fillBuffer(uint32_t* buffer, int16_t (*fPtr)(), uint32_t startIndex, uint32_t endIndex)
    {
        for(uint32_t i = startIndex; i < endIndex; i++)
        {
            int16_t temp = fPtr();
            //buffer[i] = (temp & 0x0000ffff) | ( (temp & 0x0000ffff) << 16 );
            buffer[i] = temp & 0x0000ffff;
        }
    };
    volatile uint8_t bEmptyFlag = 0;
    volatile uint8_t bSelect = 0;
};

struct i2sBuffer i2sB;

struct preCalculateSineTable1024
{
    constexpr preCalculateSineTable1024() : table()
    {
        for (uint32_t i = 0; i < 1024; i++)
        {
            table[i] = (int16_t)((float)32767.0 * std::sin(((float)i / (float)(1024.0) * 2.0f * M_PI)));
        }
    }
    int16_t table[1024];
};

static preCalculateSineTable1024 sine1024table;

// 440Hz sine wave:
static uint32_t accumulator = 0;
static uint32_t phase = 42852281;

int16_t generateSineWaveSample()
{
    accumulator += phase;
    return sine1024table.table[accumulator >> 22];
}

int main(void)
{
    SystemClock_Config(CPU_CLK_160M);
    printf("enter main\r\n");

    i2sB.bSelect = 0;
    i2sB.bEmptyFlag = 0;

    memset(tx_buf, 0x00, sizeof(tx_buf));

    //i2sB.fillBuffer(tx_buf, &generateSineWaveSample, 0, I2S_BUFFER_SIZE);
    
    HAL_Init();
    GPIO_Init();
    DMA_Init();
    I2S_Init();

    HAL_I2S_Transmit_DMA(&hi2s, (uint32_t*)tx_buf, I2S_BUFFER_SIZE * 2);
	
	while (1)
    {
		if(i2sB.bEmptyFlag)
        {
            if(!i2sB.bSelect)
            {
                //printf("<");
                i2sB.fillBuffer(tx_buf, &generateSineWaveSample, 0, I2S_BUFFER_SIZE / 2);
            }
            else
            {
                //printf(">");
                i2sB.fillBuffer(tx_buf, &generateSineWaveSample, I2S_BUFFER_SIZE / 2, I2S_BUFFER_SIZE);
            }
            i2sB.bEmptyFlag = 0;
        }
    }
}

static void GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    
    __HAL_RCC_GPIO_CLK_ENABLE();

    GPIO_InitStruct.Pin = GPIO_PIN_3 | GPIO_PIN_4;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4, GPIO_PIN_RESET);    
}

static void I2S_Init(void)
{
    hi2s.Instance = I2S;
    hi2s.Init.Mode = I2S_MODE_MASTER;
    hi2s.Init.Standard = I2S_STANDARD_PHILIPS;
    hi2s.Init.DataFormat = I2S_DATAFORMAT_16B;
    hi2s.Init.MCLKOutput = I2S_MCLKOUTPUT_ENABLE;
    hi2s.Init.AudioFreq = I2S_AUDIOFREQ_44K;
    hi2s.Init.Channel = I2S_CHANNEL_STEREO;
    hi2s.Init.ChannelSel = I2S_CHANNELSEL_LEFT;
    if (HAL_I2S_Init(&hi2s) != HAL_OK)
    {
        Error_Handler();
    }
}

static void DMA_Init(void)
{
    __HAL_RCC_DMA_CLK_ENABLE();
    
    HAL_NVIC_SetPriority(DMA_Channel0_IRQn, 0);
    HAL_NVIC_EnableIRQ(DMA_Channel0_IRQn);
}

void HAL_I2S_TxHalfCpltCallback(I2S_HandleTypeDef *hi2s)
{
    //printf("tx halfcplt\r\n");
    HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_3);
    i2sB.bSelect = 0;
    i2sB.bEmptyFlag = 1;
}

void HAL_I2S_TxCpltCallback(I2S_HandleTypeDef *hi2s)
{
    //printf("tx cplt\r\n");
    HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_4);
    i2sB.bSelect = 1;
    i2sB.bEmptyFlag = 1;
}

// void HAL_I2S_RxHalfCpltCallback(I2S_HandleTypeDef *hi2s)
// {
//     printf("rx halfcplt\r\n");
// }

// void HAL_I2S_RxCpltCallback(I2S_HandleTypeDef *hi2s)
// {
//     printf("rx cmplt\r\n");
// }

void HAL_I2S_ErrorCallback(I2S_HandleTypeDef *hi2s)
{
    printf("err cb\r\n");
}

void Error_Handler(void)
{
    while (1)
    {
        printf("error handler\r\n");
    }
}

void assert_failed(uint8_t *file, uint32_t line)
{
    printf("Wrong parameters value: file %s on line %d\r\n", file, line);
}