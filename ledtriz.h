#include "ws2812.pio.h"
#define IS_RGBW false
#define NUM_PIXELS 25
#define WS2812_PIN 7

static bool led_buffer[NUM_PIXELS]={1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1};

//Corrige o índice para que o LED certo seja acendido
uint correcao_index(int index){
    //Caso esteja numa linha ímpar
    if((index>=5 && index<10) || (index>=15 && index<20))
    return index<10 ? index+10:index-10;
    else
    return NUM_PIXELS-index-1;
   }

//Aparentemente essa função só funciona com vetor de 1 dimensão
static inline void put_pixel(uint32_t pixel_grb)
{
   pio_sm_put_blocking(pio0, 0, pixel_grb << 8u);
}

static inline uint32_t urgb_u32(uint8_t r, uint8_t g, uint8_t b)
{
   return ((uint32_t)(r) << 8) | ((uint32_t)(g) << 16) | (uint32_t)(b);
}

//Apaga toda a matriz de LEDs
void limpar_matriz(){
 uint32_t color = urgb_u32(0, 0, 0);
 for(int i=0;i<NUM_PIXELS;i++)
   put_pixel(color);
}

//Liga os LEDs na matriz que estiverem com '1'
uint8_t set_one_led(uint8_t r, uint8_t g, uint8_t b,uint8_t quantity)
{
   int i;
   // Define a cor com base nos parâmetros fornecidos
   uint32_t color = urgb_u32(r, g, b);

   // Define todos os LEDs com a cor especificada
   for (i = 0; i < quantity; i++)//Pesquena alteração pra ligar apenas a quantidade desejada
   {
      // if (led_buffer[i])
       //{
           put_pixel(color); // Liga o LED com um no buffer
      // }
      // else
      // {
      //     put_pixel(0);  // Desliga os LEDs com zero no buffer
      // }
   }
 return i;
}

//Permite selecionar o começo a partir do qual os LEDs serão acendidos
void set_leds_start(uint8_t r, uint8_t g, uint8_t b,uint8_t comeco)
{
   // Define a cor com base nos parâmetros fornecidos
   uint32_t color = urgb_u32(r, g, b);

   // Define todos os LEDs com a cor especificada
   for (; comeco < NUM_PIXELS; comeco++)//Pesquena alteração pra ligar apenas a quantidade desejada
   {
       //if (led_buffer[comeco])
      // {
           put_pixel(color); // Liga o LED com um no buffer
      // }
       //else
      // {
       //    put_pixel(0);  // Desliga os LEDs com zero no buffer
      // }
   }
}