#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "inc/ssd1306.h"
#include "inc/font.h"
#include "hardware/clocks.h"
#include "hardware/pio.h"
#include "hardware/timer.h"
#include "hardware/adc.h"     
#include "hardware/pwm.h"

#define I2C_PORT i2c1
#define I2C_SDA 14
#define I2C_SCL 15
#define endereco 0x3C
#define GREEN 11
#define BLUE 12
#define RED 13
#define BUTTON_A 5
#define BUTTON_B 6
#define VRX_PIN 26   
#define VRY_PIN 27   
#define JOYSTICK_BUTTON 22

uint32_t last_time=0,last_time2=0;
ssd1306_t ssd; // Inicializa a estrutura do display
bool leds_ativos=1,cor=1,pressed=0;
uint8_t teste1,teste2;
uint8_t rec_pos=0; //Vai de 0 a 2
uint8_t inicio_display=0; //A partir de qual string vai desenhar em opt_list, só pode ser 0 ou 1 com as 4 opções
uint8_t selected=0; //0=Cronometro, 1=Temporizador, 2=Ajustar hora, 3=Alarme, 4 ou mais=Inválido

void draw_options();
void cronometro();
void temporizador();

//Rotina de interrupção
void interrupt(uint gpio, uint32_t events)
{
    // Obtem o tempo atual em microssegundos
    uint32_t current_time = to_us_since_boot(get_absolute_time());
    // Verifica se passou tempo suficiente desde o último evento
    if (current_time - last_time2 > 300000) // 300 ms de debouncing
    {
      last_time2=current_time;
      if(gpio == BUTTON_B){
        puts("BUTTON B Click");
        pressed=1;
     }
     else{ //JOYSTICK_BUTTON
      puts("JOYSTICK Click");
      selected=1;
      gpio_set_irq_enabled_with_callback(BUTTON_B, GPIO_IRQ_EDGE_FALL, true, &interrupt);
     }
    }
}

uint pwm_init_gpio(uint gpio, uint wrap) {
  gpio_set_function(gpio, GPIO_FUNC_PWM);

  uint slice_num = pwm_gpio_to_slice_num(gpio);
  pwm_set_wrap(slice_num, wrap);
  
  pwm_set_enabled(slice_num, true);  
  return slice_num;  
}

void cronometro(){
  selected=0; //Vai ser usada pra determinar quando sair da função
  ssd1306_fill(&ssd, 0);
  //gpio_set_irq_enabled_with_callback(JOYSTICK_BUTTON, GPIO_IRQ_EDGE_FALL, true, &interrupt);
  ssd1306_draw_string(&ssd,"CRONOS",30,0);
  ssd1306_send_data(&ssd);
  
  while(1){
  if(selected == 1){
   selected=0;
   //gpio_set_irq_enabled_with_callback(JOYSTICK_BUTTON, GPIO_IRQ_EDGE_FALL, false, &interrupt);
   return;
  }
  sleep_ms(50);
 }
}

void temporizador(){
  selected=0; //Vai ser usada pra determinar quando sair da função
  ssd1306_fill(&ssd, 0);
  //gpio_set_irq_enabled_with_callback(JOYSTICK_BUTTON, GPIO_IRQ_EDGE_FALL, true, &interrupt);
  ssd1306_draw_string(&ssd,"TEMPORIZADOR",30,0);
  ssd1306_send_data(&ssd);
  
  while(1){
  if(selected == 1){
   selected=0;
   //gpio_set_irq_enabled_with_callback(JOYSTICK_BUTTON, GPIO_IRQ_EDGE_FALL, false, &interrupt);
   return;
  }
  sleep_ms(50);
 }
}

void draw_options(){
  ssd1306_fill(&ssd, 0);
  if(rec_pos != 0)
  ssd1306_draw_string(&ssd,opt_list[inicio_display],achar_coordenadas_X(opt_list[inicio_display]),1);
  if(rec_pos != 1)
  ssd1306_draw_string(&ssd,opt_list[inicio_display+1],achar_coordenadas_X(opt_list[inicio_display+1]),27);
  if(rec_pos != 2)
  ssd1306_draw_string(&ssd,opt_list[inicio_display+2],achar_coordenadas_X(opt_list[inicio_display+2]),54);

  draw_background_rectangle(&ssd,rec_pos,selected);
  ssd1306_send_data(&ssd);
}

void comando_joystick(uint16_t y){
 uint16_t current_time = to_ms_since_boot(get_absolute_time());
 if(current_time-last_time>=200){
    if(y>=3500){
      last_time=current_time;
      if(rec_pos<=0){
        rec_pos=0;
        if(inicio_display>0)
        inicio_display--;
      }
      else rec_pos--;

     if(selected <= 0) selected=0;
     else selected--;
     draw_options();
    }
    else if(y<=700){
      last_time=current_time;
      if(rec_pos >= 2){
      rec_pos=2;
      if(inicio_display<1)
      inicio_display++;
    }
      else rec_pos++;

      if(selected >= 3) selected=3;
     else selected++;
     draw_options();
    }
  }
}

int main()
{
  uint16_t red_level,blue_level;
  uint32_t current_time;
  i2c_init(I2C_PORT, 400 * 1000);

  gpio_set_function(I2C_SDA, GPIO_FUNC_I2C); // Set the GPIO pin function to I2C
  gpio_set_function(I2C_SCL, GPIO_FUNC_I2C); // Set the GPIO pin function to I2C
  gpio_pull_up(I2C_SDA); // Pull up the data line
  gpio_pull_up(I2C_SCL); // Pull up the clock line
  ssd1306_init(&ssd, WIDTH, HEIGHT, false, endereco, I2C_PORT); // Inicializa o display
  ssd1306_config(&ssd); // Configura o display

  adc_init();
  adc_gpio_init(VRX_PIN); 
  adc_gpio_init(VRY_PIN); 
  gpio_init(JOYSTICK_BUTTON);
  gpio_set_dir(JOYSTICK_BUTTON, GPIO_IN);
  gpio_pull_up(JOYSTICK_BUTTON); 

    stdio_init_all();

    gpio_init(GREEN);              
    gpio_set_dir(GREEN, GPIO_OUT);
    gpio_put(GREEN,0);
    
    gpio_init(BUTTON_A);
    gpio_set_dir(BUTTON_A, GPIO_IN); // Configura o pino como entrada
    gpio_pull_up(BUTTON_A);          // Habilita o pull-up interno

    gpio_init(BUTTON_B);
    gpio_set_dir(BUTTON_B, GPIO_IN); // Configura o pino como entrada
    gpio_pull_up(BUTTON_B);

    pwm_init_gpio(RED, 4096);
    pwm_init_gpio(BLUE, 4096); 


    //A partir daqui começa meu projeto final
    ssd1306_draw_string(&ssd,"BEM VINDO",30,0);
    ssd1306_draw_string(&ssd,"AO RELOGIO",25,27);
    ssd1306_draw_string(&ssd,"BITDOGLAB!",25,54);
    ssd1306_send_data(&ssd);
    ssd1306_fill(&ssd, 0);
    //sleep_ms(3000);

    ssd1306_draw_string(&ssd,"SELECIONE",30,0);
    ssd1306_draw_string(&ssd,"O QUE",45,27);
    ssd1306_draw_string(&ssd,"DESEJA",40,54);
    ssd1306_send_data(&ssd);
    //ssd1306_fill(&ssd, 0);
    draw_options();
   // sleep_ms(3000);
   gpio_set_irq_enabled_with_callback(BUTTON_B, GPIO_IRQ_EDGE_FALL, true, &interrupt);
  while (true)
  {
    adc_select_input(0);
   uint16_t vry_value = adc_read();
   adc_select_input(1); 
   uint16_t vrx_value = adc_read();
   
   comando_joystick(vry_value);
   if(pressed){
     pressed=0;
     gpio_set_irq_enabled_with_callback(BUTTON_B, GPIO_IRQ_EDGE_FALL, false, &interrupt);
     gpio_set_irq_enabled_with_callback(JOYSTICK_BUTTON, GPIO_IRQ_EDGE_FALL, true, &interrupt);
     switch(selected){
      case 0: cronometro(); break;
      case 1: temporizador(); break;
    }
   rec_pos=selected=0;
   gpio_set_irq_enabled_with_callback(JOYSTICK_BUTTON, GPIO_IRQ_EDGE_FALL, false, &interrupt);
   draw_options();
   gpio_set_irq_enabled_with_callback(BUTTON_B, GPIO_IRQ_EDGE_FALL, true, &interrupt);
   }
   /*
   current_time = to_ms_since_boot(get_absolute_time());
   if(current_time - last_time2 >= 1000){
   printf("valor em vry: %u\nvalor em vrx: %u\n\n",vry_value,vrx_value);
    last_time2=current_time;
   }
   */

   sleep_ms(50);
  }
  
}
