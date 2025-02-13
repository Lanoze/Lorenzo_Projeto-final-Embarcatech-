/*
 * Por: Wilton Lacerda Silva
 *    Comunicação serial com I2C
 *  
 * Uso da interface I2C para comunicação com o Display OLED
 * 
 * Estudo da biblioteca ssd1306 com PicoW na Placa BitDogLab.
 *  
 * Este programa escreve uma mensagem no display OLED.
 * 
 * 
*/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "inc/ssd1306.h"
#include "inc/font.h"
#include "hardware/clocks.h"
#include "hardware/pio.h"
//#include "ws2812.pio.h"
#include "hardware/timer.h"
#include "hardware/adc.h"     
#include "hardware/pwm.h"

#define I2C_PORT i2c1
#define I2C_SDA 14
#define I2C_SCL 15
#define endereco 0x3C

#define IS_RGBW false
#define NUM_PIXELS 25
//#define WS2812_PIN 7
#define tempo 100
#define GREEN 11
#define BLUE 12
#define RED 13
#define BUTTON_A 5
#define BUTTON_B 6
#define VRX_PIN 26   
#define VRY_PIN 27   
#define JOYSTICK_BUTTON 22

uint32_t last_time=0;
ssd1306_t ssd; // Inicializa a estrutura do display
char c;

uint pwm_init_gpio(uint gpio, uint wrap) {
  gpio_set_function(gpio, GPIO_FUNC_PWM);

  uint slice_num = pwm_gpio_to_slice_num(gpio);
  pwm_set_wrap(slice_num, wrap);
  
  pwm_set_enabled(slice_num, true);  
  return slice_num;  
}

//Rotina de interrupção, o botão A decrementa e o botão B incrementa
void interrupt(uint gpio, uint32_t events)
{
    // Obtem o tempo atual em microssegundos
    uint32_t current_time = to_us_since_boot(get_absolute_time());
    // Verifica se passou tempo suficiente desde o último evento
    if (current_time - last_time > 300000) // 300 ms de debouncing
    {
      bool ligou;
      last_time=current_time;
      ssd1306_fill(&ssd, 0);
      ssd1306_draw_char(&ssd,c,58,32);
      if(gpio==BUTTON_B){//Verifica se foi o botao B (incremento) ou o botao A (decremento)
        ligou=gpio_get(BLUE);
        gpio_put(BLUE,!gpio_get(BLUE));
        if(!ligou){
         puts("LED azul ligado!");
        }
        else{
         puts("LED azul desligado!");
        }
      }else{
        ligou=gpio_get(GREEN);
        gpio_put(GREEN,!gpio_get(GREEN));
        if(!ligou){
         puts("LED verde ligado!");
        }
        else{
         puts("LED verde desligado!");
        }
      }
      ssd1306_draw_string(&ssd, "lorem", 5, 10);
      ssd1306_rect(&ssd, 3, 3, 122, 58, 1, 0);
      ssd1306_send_data(&ssd);
    }
}

int main()
{
  uint8_t teste1,teste2; //O  propósito principal dessa variável é facilitar a depuração
  uint pwm_wrap = 4096;
  uint32_t current_time;
  // I2C Initialisation. Using it at 400Khz.
  i2c_init(I2C_PORT, 400 * 1000);

  gpio_set_function(I2C_SDA, GPIO_FUNC_I2C); // Set the GPIO pin function to I2C
  gpio_set_function(I2C_SCL, GPIO_FUNC_I2C); // Set the GPIO pin function to I2C
  gpio_pull_up(I2C_SDA); // Pull up the data line
  gpio_pull_up(I2C_SCL); // Pull up the clock line
  ssd1306_init(&ssd, WIDTH, HEIGHT, false, endereco, I2C_PORT); // Inicializa o display
  ssd1306_config(&ssd); // Configura o display
  ssd1306_send_data(&ssd); // Envia os dados para o display

  adc_init();
  adc_gpio_init(VRX_PIN); 
  adc_gpio_init(VRY_PIN); 
  gpio_init(JOYSTICK_BUTTON);
  gpio_set_dir(JOYSTICK_BUTTON, GPIO_IN);
  gpio_pull_up(JOYSTICK_BUTTON); 

  // Limpa o display. O display inicia com todos os pixels apagados.
  ssd1306_fill(&ssd, false);
  ssd1306_send_data(&ssd);

    stdio_init_all();

    gpio_init(RED);              // Inicializa o pino do LED
    gpio_set_dir(RED, GPIO_OUT); // Configura o pino como saida
    gpio_put(RED,0);

    gpio_init(BLUE);              
    gpio_set_dir(BLUE, GPIO_OUT);
    gpio_put(BLUE,0);

    gpio_init(GREEN);              
    gpio_set_dir(GREEN, GPIO_OUT);
    gpio_put(GREEN,0);
    
    gpio_init(BUTTON_A);
    gpio_set_dir(BUTTON_A, GPIO_IN); // Configura o pino como entrada
    gpio_pull_up(BUTTON_A);          // Habilita o pull-up interno

    gpio_init(BUTTON_B);
    gpio_set_dir(BUTTON_B, GPIO_IN); // Configura o pino como entrada
    gpio_pull_up(BUTTON_B);

    pwm_init_gpio(RED, pwm_wrap);
    pwm_init_gpio(BLUE, pwm_wrap); 

    //Só é possivel ter uma unica função de interrupção
    gpio_set_irq_enabled_with_callback(BUTTON_B, GPIO_IRQ_EDGE_FALL, true, &interrupt);
    gpio_set_irq_enabled_with_callback(BUTTON_A, GPIO_IRQ_EDGE_FALL, true, &interrupt);

  //bool cor = true;
  while (true)
  {
   adc_select_input(0);
   uint16_t vry_value = adc_read();
   adc_select_input(1); 
   uint16_t vrx_value = adc_read();

   ssd1306_fill(&ssd, 1); // Limpa o display
   ssd1306_rect(&ssd, 3, 3, 122, 58, 0, 1); // Desenha um retângulo
   //ssd1306_draw_char(&ssd,'A',58,32);
   current_time = to_ms_since_boot(get_absolute_time());
   if(current_time - last_time >= 1000){
   //printf("vry: %u\n",vry_value);
   printf("valor em y: %u\nvalor em x: %u\n\n",teste1,teste2);
    last_time=current_time;
   }
   //O .0 é importante pra indicar que quero um numero fracionario (Caso contrario resulta em 0)
   //O 120 serve pra compensar o fato do display ter eixo Y com sentido pra baixo e o joystick
   //ter o eixo y com sentido pra cima
   teste1=120-(uint8_t)round(120*(vry_value/4095.0));
   teste2=(uint8_t)round(120*(vrx_value/4095.0));
   //Coloca um limite máximo e mínimo
   if(teste1 < 37) teste1=37;
   else if(teste1 > 87) teste1=87;

   //if(teste2 >= 57 && teste2 < 59)teste2=58;

   ssd1306_draw_square(&ssd,teste2,teste1-34); //O -34 é pra compensar o drift
   ssd1306_send_data(&ssd);
   sleep_ms(10);
  }
}