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
#include "ws2812.pio.h"
#include "ledtriz.h"

#define I2C_PORT i2c1
#define I2C_SDA 14
#define I2C_SCL 15
#define endereco 0x3C
#define GREEN 11
#define BLUE 12
#define RED 13
#define BUZZER_A 21
#define BUZZER_B 10
#define BUTTON_A 5
#define BUTTON_B 6
#define VRX_PIN 26   
#define VRY_PIN 27   
#define JOYSTICK_BUTTON 22


uint32_t last_time=0,last_time2=0,last_time3=0,last_time4=0,time_teste=0,start_time;
ssd1306_t ssd; // Inicializa a estrutura do display
bool relogio_ativo=0,cor=1,pressed=0,start=0,no_menu=1,no_relogio=0;
bool relogio_executando=0,ajustando_hora=0,alarme_ativo=0;
//uint8_t teste1,teste2;
uint8_t rec_pos=0; //Vai de 0 a 2
uint8_t inicio_display=0; //A partir de qual string vai desenhar em opt_list, só pode ser 0 ou 1 com as 4 opções
uint8_t selected=0; //0=Cronometro, 1=Temporizador, 2=Ajustar hora, 3=Alarme, 4 ou mais=Inválido
uint8_t valor_exibido[6];//Valores exibidos no cronômetro
uint32_t debug_aux=0;//Variável pra ajudar na depuração
struct repeating_timer timer1,timer2,timer3;
char tempo[8];
uint8_t relog[2]={0,0};//Guarda o valor das horas e minutos do relógio
char str_aux[6];

void draw_options();
void cronometro();
void temporizador();
void incrementar_cronometro();
void timer_set();
void temporizador_callback();
void ajustar_hora_callback();
void relogio_set();

void alarmer_buzzer(){
  ssd1306_fill(&ssd, 0);
  ssd1306_draw_string(&ssd,"APERTE B",30,25);
  ssd1306_send_data(&ssd);

  while(gpio_get(BUTTON_B) != 0 && pressed==1){
 pwm_set_gpio_level(BUZZER_A,3000);
 sleep_us(900);
 pwm_set_gpio_level(BUZZER_A,0);
 sleep_us(900);
 }
 while(gpio_get(BUTTON_B) == 0) sleep_us(1);//Debounce
 pressed=0;
 //gpio_set_irq_enabled_with_callback(BUTTON_B, GPIO_IRQ_EDGE_FALL, true, &temporizador_callback);
  ssd1306_fill(&ssd, 0);
  ssd1306_draw_string(&ssd,"0",30,0);
  ssd1306_send_data(&ssd);
 puts("Chegou ao fim");
}

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

bool timer_cronometro(struct repeating_timer *timer){
  //uint64_t elapsed = (time_us_64() - start_time) / 1000;  // Convertendo microssegundos para milissegundos
   // printf("Tempo: %llu ms\n", elapsed);
 if(start)
 {incrementar_cronometro(); return true;}
 return false;
}

void cronometro_callback(uint gpio, uint32_t events){
  uint32_t current_time = to_us_since_boot(get_absolute_time());
  // Verifica se passou tempo suficiente desde o último evento
  if (current_time - last_time2 > 300000) // 300 ms de debouncing
  {
    last_time2=current_time;
   if(gpio==BUTTON_B){
     start = !start;
     printf("Start = %d\n",start);
     if(start){
     //start_time = time_us_64();
     //Tive que colocar 71ms pra compensar o tempo gasto na função de incrementar cronômetro
     add_repeating_timer_ms(71, timer_cronometro, NULL, &timer1);
     }
    }
    else if(gpio == JOYSTICK_BUTTON){ //JOYSTICK_BUTTON
      puts("JOYSTICK Click");
      selected=1;
      start=0;
      gpio_set_irq_enabled_with_callback(BUTTON_B, GPIO_IRQ_EDGE_FALL, false, &cronometro_callback);
      gpio_set_irq_enabled_with_callback(BUTTON_A, GPIO_IRQ_EDGE_FALL, false, &cronometro_callback);
      gpio_set_irq_enabled_with_callback(JOYSTICK_BUTTON, GPIO_IRQ_EDGE_FALL, false, &cronometro_callback);
      gpio_set_irq_enabled_with_callback(BUTTON_B, GPIO_IRQ_EDGE_FALL, true, &interrupt);
     }
    else{ //Botão A
      for(uint8_t i=0;i<6;i++)
      {valor_exibido[i]=0;}
      ssd1306_fill(&ssd, 0);
      ssd1306_draw_string(&ssd,"00000.0",10,0);
     ssd1306_send_data(&ssd);
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

//Provavelmente dava pra usar recursão
void incrementar_cronometro(){
  //if(debug_aux == 1000){start=0; return;}
  //printf("Debug aux= %d\n",debug_aux);
  uint32_t current_time = to_ms_since_boot(get_absolute_time());
  if(valor_exibido[5]<9) valor_exibido[5]++;
  else
  {
   valor_exibido[5]=0;
   if(valor_exibido[4]<9)
     valor_exibido[4]++;
    else
    {
     valor_exibido[4]=0;
     if(valor_exibido[3]<9)
      valor_exibido[3]++;
     else
     {
      valor_exibido[3]=0;
      if(valor_exibido[2]<9)
      valor_exibido[2]++;
      else
      {
       valor_exibido[2]=0;
       if(valor_exibido[1]<9)
       valor_exibido[1]++;
       else
       {
        valor_exibido[1]=0;
        if(valor_exibido[0]<9)
        valor_exibido[0]++;
        else
        {
         puts("Valor maximo do cronometro atingido");
         for(uint8_t i=0;i<6;i++)
          {valor_exibido[i]=9;}
          start=0;
        }
       }
      }
     }
    }
  }
 //debug_aux++;
  ssd1306_fill(&ssd, 0);
  sprintf(tempo,"%d%d%d%d%d.%d",valor_exibido[0],valor_exibido[1],valor_exibido[2],valor_exibido[3],valor_exibido[4],valor_exibido[5]);
  ssd1306_draw_string(&ssd,tempo,10,0);
  ssd1306_send_data(&ssd);
  printf("Tempo da funcao: %d\n",current_time-time_teste);
   time_teste=current_time;
}

void cronometro(){
  start=0;
  for(uint8_t i=0;i<6;i++)
  {valor_exibido[i]=0;}
  selected=0; //Vai ser usada pra determinar quando sair da função
  ssd1306_fill(&ssd, 0);
  gpio_set_irq_enabled_with_callback(JOYSTICK_BUTTON, GPIO_IRQ_EDGE_FALL, true, &cronometro_callback);
  ssd1306_draw_string(&ssd,"00000.0",10,0);
  ssd1306_send_data(&ssd);
  gpio_set_irq_enabled_with_callback(BUTTON_B, GPIO_IRQ_EDGE_FALL, true, &cronometro_callback);
  gpio_set_irq_enabled_with_callback(BUTTON_A, GPIO_IRQ_EDGE_FALL, true, &cronometro_callback);
  
  while(1){
  //puts("Entrou no loop principal");
  if(selected == 1){
   selected=0;
   //gpio_set_irq_enabled_with_callback(JOYSTICK_BUTTON, GPIO_IRQ_EDGE_FALL, false, &interrupt);
   return;
  }
  /*
  if(start){
    //printf("No loop cr\n");
    ssd1306_fill(&ssd, 0);
    sprintf(tempo,"%d%d%d:%d%d%d",valor_exibido[0],valor_exibido[1],valor_exibido[2],valor_exibido[3],valor_exibido[4],valor_exibido[5]);
    ssd1306_draw_string(&ssd,tempo,10,0);
    ssd1306_send_data(&ssd);
    //sleep_ms(1);
    incrementar_cronometro();
  }
  */
 // current_time = to_ms_since_boot(get_absolute_time());
//if(current_time-time_teste>=1000){start=0; puts("Passou-se 1 segundo");}
  sleep_us(1);
 }
}

void selecionar_temporizador(uint16_t y){
  static uint16_t rapido=200;//Aumenta a velocidade quando o joystick é segurado por um tempo na mesma posição
  uint16_t current_time = to_ms_since_boot(get_absolute_time());
  if(current_time-last_time>=rapido){
    last_time=current_time;
     if(y>=3500){//joystick pra cima
       //last_time=current_time;
       if(rapido >= 50) rapido-=10;
       if(rec_pos<100)
       rec_pos++;
       else
       rec_pos=100;
       sprintf(tempo,"%d",rec_pos);
       ssd1306_fill(&ssd, 0);
       ssd1306_draw_string(&ssd,tempo,30,0);
       ssd1306_send_data(&ssd);
     }
     else if(y<=700){//Joystick pra baixo
       //last_time=current_time;
       if(rapido >= 50) rapido-=10;
       if(rec_pos>0)
       rec_pos--;
       else
       rec_pos=0;
       sprintf(tempo,"%d",rec_pos);
       ssd1306_fill(&ssd, 0);
       ssd1306_draw_string(&ssd,tempo,30,0);
       ssd1306_send_data(&ssd);
      }else rapido=200;
     }
   }

   void temporizador_callback(uint gpio, uint32_t events){
    uint32_t current_time = to_us_since_boot(get_absolute_time());
    // Verifica se passou tempo suficiente desde o último evento
    if (current_time - last_time2 > 300000) // 300 ms de debouncing
    {
      last_time2=current_time;
     if(gpio==BUTTON_B){
       start = !start;
       printf("Start = %d\n",start);
       if(start){
       //add_repeating_timer_ms(100, timer_set, NULL, &timer1);
       timer_set();
       }// else{

        //limpar_matriz();
       //}
      }
      else if(gpio == JOYSTICK_BUTTON){ //JOYSTICK_BUTTON
        puts("JOYSTICK Click");
        selected=1;
        pressed=start=0;
        limpar_matriz();
        gpio_set_irq_enabled_with_callback(BUTTON_B, GPIO_IRQ_EDGE_FALL, true, &interrupt);
       }
      else{ //Botão A
       start=0;
       limpar_matriz();
      }
    }
  }

 bool timer_countdown(){
   uint32_t current_time = to_ms_since_boot(get_absolute_time()),i;
   if(!rec_pos && !pressed){
    //if(selected==1) return false;
    puts("Countdown");
    start=0; if(!no_menu)pressed=1; return false;
  }
   if(!start){return false;}
   else if(rec_pos>75){
     rec_pos--;
      ssd1306_fill(&ssd, 0);
      sprintf(tempo,"%d",rec_pos);
      ssd1306_draw_string(&ssd,tempo,30,0);
      ssd1306_send_data(&ssd);
     uint8_t aux=rec_pos-75; //Armazena o índice exato do LED a ser alterado
     uint32_t color_new = urgb_u32(0, 5, 5),color_old=urgb_u32(0, 5, 0);

     

     for(i=0;i<aux;i++)
     put_pixel(color_old);
     put_pixel(color_new);
    
     printf("Tempo da funcao: %d\n",current_time-time_teste);
     time_teste=current_time;
     return start;

    }else

    if(rec_pos>50){
      rec_pos--;
      ssd1306_fill(&ssd, 0);
      sprintf(tempo,"%d",rec_pos);
      ssd1306_draw_string(&ssd,tempo,30,0);
      ssd1306_send_data(&ssd);
      uint8_t aux=rec_pos-50; //Armazena o índice exato do LED a ser alterado
      uint32_t color_new = urgb_u32(5, 5, 0),color_old=urgb_u32(0, 5, 5);
 
      
 
      for(i=0;i<aux;i++)
      put_pixel(color_old);
      put_pixel(color_new);

      printf("Tempo da funcao: %d\n",current_time-time_teste);
      time_teste=current_time;
      return start;
    }else 

    if(rec_pos>25){
      rec_pos--;
      ssd1306_fill(&ssd, 0);
      sprintf(tempo,"%d",rec_pos);
      ssd1306_draw_string(&ssd,tempo,30,0);
      ssd1306_send_data(&ssd);
      uint8_t aux=rec_pos-25; //Armazena o índice exato do LED a ser alterado
      uint32_t color_new = urgb_u32(5, 0, 0),color_old=urgb_u32(5, 5, 0);


 
      for(i=0;i<aux;i++)
      put_pixel(color_old);
      put_pixel(color_new);

      printf("Tempo da funcao: %d\n",current_time-time_teste);
      time_teste=current_time;
      return start;

    }else{
      rec_pos--;
      ssd1306_fill(&ssd, 0);
      sprintf(tempo,"%d",rec_pos);
      ssd1306_draw_string(&ssd,tempo,30,0);
      ssd1306_send_data(&ssd);
      uint8_t aux=rec_pos; //Armazena o índice exato do LED a ser alterado
      uint32_t color_new = urgb_u32(0, 0, 0),color_old=urgb_u32(5, 0, 0);
 
      for(i=0;i<aux;i++)
      put_pixel(color_old);
      put_pixel(color_new);

      printf("Tempo da funcao: %d\n",current_time-time_teste);
      time_teste=current_time;
      return start;
    }
  }

 //Seta o estado inicial da matriz de LEDs do temporizador
 void timer_set(){
  //uint32_t current_time = to_ms_since_boot(get_absolute_time());
  if(rec_pos>75){
    uint8_t aux=rec_pos-75;
    uint8_t aux2 = set_one_led(0,5,0,aux);//verde
    set_leds_start(0,5,5,aux2);//ciano
  } else if(rec_pos>50){
    uint8_t aux=rec_pos-50;
    uint8_t aux2 = set_one_led(0,5,5,aux);//ciano
    set_leds_start(5,5,0,aux2);//amarelo
  } else if(rec_pos>25){
    uint8_t aux=rec_pos-25;
    uint8_t aux2 = set_one_led(5,5,0,aux);//amarelo
    set_leds_start(5,0,0,aux2);//vermelho
  } else{// <=25 Leds
    uint8_t aux2 = set_one_led(5,0,0,rec_pos);//vermelho
  }
 //971 ms pra compensar o tempo gasto pela função
  add_repeating_timer_ms(971, timer_countdown, NULL, &timer2);
 // printf("Tempo da funcao: %d\n",current_time-time_teste);
 // time_teste=current_time;
  
 }

//Até 100 segundos
void temporizador(){//A variável rec_pos vai definir o valor do temporizador
  no_menu=rec_pos=selected=0; //Vai ser usada pra determinar quando sair da função
  gpio_set_irq_enabled_with_callback(JOYSTICK_BUTTON, GPIO_IRQ_EDGE_FALL, true, &temporizador_callback);
  gpio_set_irq_enabled_with_callback(BUTTON_B, GPIO_IRQ_EDGE_FALL, true, &temporizador_callback);
  gpio_set_irq_enabled_with_callback(BUTTON_A, GPIO_IRQ_EDGE_FALL, true, &temporizador_callback);
  ssd1306_fill(&ssd, 0);
  ssd1306_draw_string(&ssd,"0",30,0);
  ssd1306_send_data(&ssd);
  
  while(1){
  if(selected == 1){
   selected=0;
   //gpio_set_irq_enabled_with_callback(JOYSTICK_BUTTON, GPIO_IRQ_EDGE_FALL, false, &interrupt);
   return;
  }

  if(pressed){ puts("ENTROU");
     gpio_set_irq_enabled_with_callback(BUTTON_B, GPIO_IRQ_EDGE_FALL, false, &temporizador_callback);
     alarmer_buzzer();
    }
  else gpio_set_irq_enabled_with_callback(BUTTON_B, GPIO_IRQ_EDGE_FALL, true, &temporizador_callback);
   adc_select_input(0);
   uint16_t vry_value = adc_read();

   if(!start)
   selecionar_temporizador(vry_value);
   sleep_us(1);
 }
}

bool atualizar_horario(){
 if(!ajustando_hora) //Só atualiza o tempo se não estiver ajustando a hora
 {
 if(relog[1]<59) relog[1]++;
 else{
  if(relog[0]<23){
    relog[0]++;
    relog[1]=0;
  }
  else{
    relog[0]=relog[1]=0;//Ao dar meia noite, zera o relógio
  }
 }
 puts("Relogio funcionando de boa");
 if(no_relogio){
  ssd1306_fill(&ssd, 0);
  sprintf(str_aux,"%d:%d",relog[0],relog[1]);
  ssd1306_draw_string(&ssd,str_aux,50,30);
  ssd1306_send_data(&ssd);
 }
}
 return 1;
}

void definir_horario(uint16_t y){
  static uint8_t rapido=200;
  uint32_t current_time = to_ms_since_boot(get_absolute_time());
  if(current_time-last_time4 >= rapido){
  last_time4=current_time;
  if(y>=3500){
   if(rapido > 30) rapido-=10;
    if(rec_pos==0 && relog[0]<23){
    relog[0]++;
   }else if(rec_pos==1 && relog[1]<59){
    relog[1]++;
   }
  }else if(y<=700){
    if(rapido > 30) rapido-=10;
    if(rec_pos==0 && relog[0]>0){
      relog[0]--;
     }else if(rec_pos==1 && relog[1]>0){
      relog[1]--;
     }
  } else rapido=200;
 }
}

void relogio_set_callback(uint gpio, uint32_t events){
  uint32_t current_time = to_us_since_boot(get_absolute_time());
  // Verifica se passou tempo suficiente desde o último evento
  if (current_time - last_time2 > 300000) // 300 ms de debouncing
  {
    last_time2=current_time;
   if(gpio==BUTTON_B){
     relogio_ativo = 1;
     ajustando_hora=0;
     printf("Please...\n");
     
    }
    else if(gpio == JOYSTICK_BUTTON){ //JOYSTICK_BUTTON
      puts("JOYSTICK Click");
      selected=1;
      ajustando_hora=pressed=start=0;
      gpio_set_irq_enabled_with_callback(BUTTON_B, GPIO_IRQ_EDGE_FALL, true, &interrupt);
     }
  }
}

void relogio_set(){
  static char escolhido[7]="HORA";
  relogio_ativo=rec_pos=0;
  ajustando_hora=1;
  gpio_set_irq_enabled_with_callback(JOYSTICK_BUTTON, GPIO_IRQ_EDGE_FALL, true, &relogio_set_callback);
  gpio_set_irq_enabled_with_callback(BUTTON_B, GPIO_IRQ_EDGE_FALL, false, &ajustar_hora_callback);
  //last_time3=to_us_since_boot(get_absolute_time());
  gpio_set_irq_enabled_with_callback(BUTTON_B, GPIO_IRQ_EDGE_FALL, true, &relogio_set_callback);
  ssd1306_draw_string(&ssd,escolhido,50,50);
  ssd1306_send_data(&ssd);
  //while(gpio_get(BUTTON_B)==0){sleep_us(10);}

  while(1){
    if(selected == 1)
      {start=0; return;}
    
  ssd1306_fill(&ssd, 0);
  ssd1306_draw_string(&ssd,escolhido,50,50);
  sprintf(str_aux,"%d:%d",relog[0],relog[1]);
  ssd1306_draw_string(&ssd,str_aux,50,30);
  ssd1306_send_data(&ssd);

  adc_select_input(0);
  uint16_t vry_value = adc_read();
  adc_select_input(1); 
  uint16_t vrx_value = adc_read();

  if(vrx_value>=3500 && !rec_pos){rec_pos=1; strcpy(escolhido,"MINUTO");}
  else if(vrx_value <= 700 && rec_pos) {rec_pos=0; strcpy(escolhido,"HORA");}

  definir_horario(vry_value);
  if(relogio_ativo){ printf("Ativado relogio\n");
     gpio_set_irq_enabled_with_callback(BUTTON_B, GPIO_IRQ_EDGE_FALL, true, &ajustar_hora_callback);
     if(!relogio_executando){add_repeating_timer_ms(1000,atualizar_horario,NULL,&timer3);relogio_executando=1;}
     selected=1;
     return;
    }
  //printf("rec_pos = %d\n",rec_pos);
  sleep_us(1);
  }
}

void ajustar_hora_callback(uint gpio, uint32_t events){
  uint32_t current_time = to_us_since_boot(get_absolute_time());
  // Verifica se passou tempo suficiente desde o último evento
  if (current_time - last_time2 > 300000) // 300 ms de debouncing
  {
    last_time2=current_time;
   if(gpio==BUTTON_B){
     start = 1;
     printf("Por favor...\n");
     
    }
    else if(gpio == JOYSTICK_BUTTON){ //JOYSTICK_BUTTON
      puts("JOYSTICK Click");
      selected=1;
      pressed=start=0;
      gpio_set_irq_enabled_with_callback(BUTTON_B, GPIO_IRQ_EDGE_FALL, true, &interrupt);
     }
    else{ //Botão A
     start=0;
    }
  }
}


void ajustar_hora(){//A variável rec_pos vai definir se está selecionado a hora ou minuto
  while(gpio_get(BUTTON_B)==0) sleep_us(10);
  no_relogio=1;
  ajustando_hora=0;
  //sleep_ms(100);
  start=rec_pos=selected=0; //Vai ser usada pra determinar quando sair da 
  gpio_set_irq_enabled_with_callback(JOYSTICK_BUTTON, GPIO_IRQ_EDGE_FALL, true, &ajustar_hora_callback);
  gpio_set_irq_enabled_with_callback(BUTTON_B, GPIO_IRQ_EDGE_FALL, true, &ajustar_hora_callback);

  ssd1306_fill(&ssd, 0);
  sprintf(str_aux,"%d:%d",relog[0],relog[1]);
  ssd1306_draw_string(&ssd,str_aux,50,30);
  ssd1306_send_data(&ssd);
  
  while(1){
  if(selected == 1){
   no_relogio=selected=0;
   return;
  }
  if(start){relogio_set();}
  sleep_us(1);
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
int64_t desligar_alarme(){
  //puts("Terminando alarme");
 pwm_set_gpio_level(BUZZER_B,0);
 alarme_ativo=0;
 puts("Fim do alarme");
 return 0;
}

int64_t alarme_comum(){
  puts("Iniciando alarme");

 pwm_set_gpio_level(BUZZER_B,5000);
 add_alarm_in_ms(2000,desligar_alarme,NULL,false);
 return 0;
}

void meu_alarme_callback(uint gpio, uint32_t events){
  uint32_t current_time = to_us_since_boot(get_absolute_time());
  // Verifica se passou tempo suficiente desde o último evento
  if (current_time - last_time2 > 300000) // 300 ms de debouncing
  {
    last_time2=current_time;
   if(gpio==BUTTON_B && !alarme_ativo){
     alarme_ativo = 1;
     printf("Alarme definido\n");
     add_alarm_in_ms(valor_exibido[0]*1000, alarme_comum, NULL, false);
    }
    else if(gpio == JOYSTICK_BUTTON){ //JOYSTICK_BUTTON
      puts("JOYSTICK Click");
      selected=1;
      pressed=start=0;
      gpio_set_irq_enabled_with_callback(BUTTON_B, GPIO_IRQ_EDGE_FALL, true, &interrupt);
     }
  }
}

void alarme_joystick(uint16_t y){
  static uint8_t rapido=200;
  uint16_t current_time = to_ms_since_boot(get_absolute_time());
  if(current_time-last_time3>=rapido){
    if(y>=3500){ last_time3=current_time;
      if(valor_exibido[0] < 100) valor_exibido[0]++;

      sprintf(str_aux,"%d",valor_exibido[0]);
      ssd1306_fill(&ssd, 0);
      ssd1306_draw_string(&ssd,str_aux,60,30);
      ssd1306_send_data(&ssd);
      if(rapido>30) rapido-=10;
    }
    else if(y<=700){ last_time3=current_time;
      if(valor_exibido[0] > 1) valor_exibido[0]--;
      sprintf(str_aux,"%d",valor_exibido[0]);
      ssd1306_fill(&ssd, 0);
      ssd1306_draw_string(&ssd,str_aux,60,30);
      ssd1306_send_data(&ssd);
      if(rapido>50) rapido-=10;
  } else rapido=200;
 }
}

void colocar_alarme(){
  selected=0;
  valor_exibido[0]=1;
  sprintf(str_aux,"%d",valor_exibido[0]);
  ssd1306_fill(&ssd, 0);
  ssd1306_draw_string(&ssd,str_aux,60,30);
  ssd1306_send_data(&ssd);
  adc_select_input(0);
  gpio_set_irq_enabled_with_callback(JOYSTICK_BUTTON, GPIO_IRQ_EDGE_FALL, true, &meu_alarme_callback);
  if(!alarme_ativo){
    gpio_set_irq_enabled_with_callback(BUTTON_B, GPIO_IRQ_EDGE_FALL, true, &meu_alarme_callback);
  while (1){
    if(selected == 1){
      selected=0;
      return;
     }
     if(alarme_ativo){add_alarm_in_ms(valor_exibido[0]*1000, alarme_comum, NULL, false); return;}
    uint16_t vry_value = adc_read();
    alarme_joystick(vry_value);
  }
 }
 else{
  ssd1306_fill(&ssd, 0);
  ssd1306_draw_string(&ssd,"ATIVO",45,30);
  ssd1306_send_data(&ssd);
  sleep_ms(800);
  return;
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
  
  PIO pio = pio0;
    int sm = 0;
    uint offset = pio_add_program(pio, &ws2812_program);

    ws2812_program_init(pio, sm, offset, WS2812_PIN, 800000, IS_RGBW);

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

    pwm_init_gpio(BUZZER_A, 4096);
    pwm_init_gpio(BUZZER_B, 20000);

    limpar_matriz();
    draw_options();
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
     //gpio_set_irq_enabled_with_callback(JOYSTICK_BUTTON, GPIO_IRQ_EDGE_FALL, true, &interrupt);
     switch(selected){
      case 0: cronometro(); break;
      case 1: temporizador(); break;
      case 2: ajustar_hora(); break;
      default: colocar_alarme();
    }
   inicio_display=rec_pos=selected=0;
   gpio_set_irq_enabled_with_callback(JOYSTICK_BUTTON, GPIO_IRQ_EDGE_FALL, false, &interrupt);
   draw_options();
   gpio_set_irq_enabled_with_callback(BUTTON_B, GPIO_IRQ_EDGE_FALL, true, &interrupt);
   //pressed=0;
   no_menu=1;
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
