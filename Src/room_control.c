/**
 ******************************************************************************
 * @file           : room_control.c
 * @author         : Sam C
 * @brief          : Room control driver for STM32L476RGTx
 ******************************************************************************
 */
#include "room_control.h"

#include "gpio.h"    // Para controlar LEDs y leer el botón (aunque el botón es por EXTI)
#include "systick.h" // Para obtener ticks y manejar retardos/tiempos
#include "uart.h"    // Para enviar mensajes
#include "tim.h"     // Para controlar el PWM


static volatile uint32_t g_door_open_tick = 0;
static volatile uint8_t g_door_open = 0;
static volatile uint32_t g_last_button_tick = 0;
static volatile uint32_t g_PWM_set = 0;
static volatile uint32_t g_PWM_set_tick = 0;
static volatile uint32_t g_set_tick = 0;
static volatile uint32_t duty_handler = 0; //maneja el duty al usar el comando 'g'
static volatile uint32_t cdc = 0; //duty en room control
static volatile uint32_t g_set = 0; //duty en room control
void room_control_app_init(void)
{
    gpio_write_pin(EXTERNAL_LED_ONOFF_PORT, EXTERNAL_LED_ONOFF_PIN, GPIO_PIN_RESET);
    g_door_open = 0;
    g_door_open_tick = 0;

    tim3_ch1_pwm_set_duty_cycle(20); // Lámpara al 20%
}

void room_control_on_button_press(void)
{
    uint32_t now = systick_get_tick();
    if (now - g_last_button_tick < 50) return;  // Anti-rebote de 50 ms
    g_last_button_tick = now;

    uart2_send_string("Evento: Botón presionado - Abriendo puerta.\r\n");

    gpio_write_pin(EXTERNAL_LED_ONOFF_PORT, EXTERNAL_LED_ONOFF_PIN, GPIO_PIN_SET);
    cdc = current_duty_get();
    tim3_ch1_pwm_set_duty_cycle(100);
    g_door_open_tick = now;
    g_PWM_set_tick = now;
    g_door_open = 1;
    g_PWM_set = 1;


}

void room_control_on_uart_receive(char cmd)
{
    switch (cmd) {
        case '1':
            tim3_ch1_pwm_set_duty_cycle(100);
            uart2_send_string("Lámpara: brillo al 100%.\r\n");
            break;

        case '2':
            tim3_ch1_pwm_set_duty_cycle(70);
            uart2_send_string("Lámpara: brillo al 70%.\r\n");
            break;

        case '3':
            tim3_ch1_pwm_set_duty_cycle(50);
            uart2_send_string("Lámpara: brillo al 50%.\r\n");
            break;

        case '4':
            tim3_ch1_pwm_set_duty_cycle(20);
            uart2_send_string("Lámpara: brillo al 20%.\r\n");
            break;

        case '0':
            tim3_ch1_pwm_set_duty_cycle(0);
            uart2_send_string("Lámpara apagada.\r\n");
            break;

        case 'o':
        case 'O':
            gpio_write_pin(EXTERNAL_LED_ONOFF_PORT, EXTERNAL_LED_ONOFF_PIN, GPIO_PIN_SET);
            g_door_open_tick = systick_get_tick();
            g_door_open = 1;
            uart2_send_string("Puerta abierta remotamente.\r\n");
            break;

        case 'c':
        case 'C':
            gpio_write_pin(EXTERNAL_LED_ONOFF_PORT, EXTERNAL_LED_ONOFF_PIN, GPIO_PIN_RESET);
            g_door_open = 0;
            uart2_send_string("Puerta cerrada remotamente.\r\n");
            break;
        case '?': //logica para '?'
            uart2_send_string("'1'-'4': Ajustar brillo lámpara (100%, 70%, 50%, 20%)\r\n");
            uart2_send_string("'0'   : Apagar lámpara\r\n");
            uart2_send_string("'o'   : Abrir puerta\r\n");
            uart2_send_string("'c'   : Cerrar puerta\r\n");
            uart2_send_string("'s'   : Estado del sistema\r\n");
            uart2_send_string("'?'   : Ayuda\r\n");
            break;
        case 'g': //logica para '?'
            tim3_ch1_pwm_set_duty_cycle(0);
            g_set = 1 ;
            g_set_tick = systick_get_tick();
            break;
        default:
            uart2_send_string("Comando desconocido.\r\n");
            break;
    }
}

void room_control_tick(void)
{
    if (g_door_open && (systick_get_tick() - g_door_open_tick >= 3000)) {
        gpio_write_pin(EXTERNAL_LED_ONOFF_PORT, EXTERNAL_LED_ONOFF_PIN, GPIO_PIN_RESET);
        uart2_send_string("Puerta cerrada automáticamente tras 3 segundos.\r\n");
        g_door_open = 0;
    }
    if (g_PWM_set && (systick_get_tick() - g_PWM_set_tick >= 10000)) {
        tim3_ch1_pwm_set_duty_cycle(cdc);
        uart2_send_string("PWM puesto al estado anterior despues de 10 segundos.\r\n");
        g_PWM_set = 0;
    }
    if (g_set && (systick_get_tick() - g_set_tick >= 500)) {
        duty_handler+= 10;
        tim3_ch1_pwm_set_duty_cycle(duty_handler);
        uart2_send_string("duty + 10%\r\n");
        g_set_tick = systick_get_tick();
        if (duty_handler == 100){g_set = 0;}
    }
}
