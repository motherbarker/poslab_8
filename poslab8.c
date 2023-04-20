/*
 * File:   poslab8.c
 * Author: Carlos Daniel Valdez Coreas
 * Descripción: Se envia un caracter desde el microcontrolador hacia la
 * computadora y se muestra en la hiperterminal. Asimismo, se recibe un caracter
 * desde la hiperterminal y se muestra en el puerto B
 * Created on 19 de abril de 2023
 */
// PIC16F887 Configuration Bit Settings

// 'C' source line config statements

/*---------------------------- CONFIGURATION BITS ----------------------------*/
// CONFIG1
#pragma config FOSC = INTRC_NOCLKOUT// Oscillator Selection bits (INTOSCIO oscillator: I/O function on RA6/OSC2/CLKOUT pin, I/O function on RA7/OSC1/CLKIN)
#pragma config WDTE = OFF       // Watchdog Timer Enable bit (WDT disabled and can be enabled by SWDTEN bit of the WDTCON register)
#pragma config PWRTE = OFF      // Power-up Timer Enable bit (PWRT disabled)
#pragma config MCLRE = OFF      // RE3/MCLR pin function select bit (RE3/MCLR pin function is digital input, MCLR internally tied to VDD)
#pragma config CP = OFF         // Code Protection bit (Program memory code protection is disabled)
#pragma config CPD = OFF        // Data Code Protection bit (Data memory code protection is disabled)
#pragma config BOREN = OFF      // Brown Out Reset Selection bits (BOR disabled)
#pragma config IESO = OFF       // Internal External Switchover bit (Internal/External Switchover mode is disabled)
#pragma config FCMEN = OFF      // Fail-Safe Clock Monitor Enabled bit (Fail-Safe Clock Monitor is disabled)
#pragma config LVP = OFF        // Low Voltage Programming Enable bit (RB3 pin has digital I/O, HV on MCLR must be used for programming)

// CONFIG2
#pragma config BOR4V = BOR40V   // Brown-out Reset Selection bit (Brown-out Reset set to 4.0V)
#pragma config WRT = OFF        // Flash Program Memory Self Write Enable bits (Write protection off)


/*--------------------------------- LIBRARIES --------------------------------*/
#include <xc.h>
#include <stdint.h>
#include <PIC16F887.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>   //libreria para manipular cadenas

/*
 * Constantes
 */
#define _XTAL_FREQ 1000000  //frecuencia 1MHz

/*
 * Variables
 */

uint8_t bandera;
uint8_t pot;
char    select;
char    rx;
char    pot_val [];

/*
 * Prototipos de funciones
 */

void setup(void);
void print (unsigned char *word);   //muestra caracteres en la terminal
void show (char select);            //pasa el valor del pot

/*
 * Interrupciones
 */

void __interrupt() isr (void)
{
    if(PIR1bits.RCIF)       //chequea si hay datos recibidos
    {
        rx = RCREG;
    }
    
    if (PIR1bits.ADIF)      //interrupción ADC
    {
        if (ADCON0bits.CHS == 0b0000)
        {
            pot = ADRESH;
        }        
        __delay_us(50);
        PIR1bits.ADIF = 0;
    }
}

/*
 * Main
 */

void main (void)
{
    setup();
    while(1)
    {
        if (ADCON0bits.GO == 0)     //no hay conversión
        {
            ADCON0bits.GO = 1;      //se inicia conversión
        }
        print("\r ********************** \r");      //menú
        print("\r ¿Qué deseas hacer? \r");
        print("\r 1. Leer potenciómetro  \r");
        print("\r 2. Enviar ASCII  \r");
        print("\r ********************** \r");
        
        bandera = 1;
        
        while (bandera)     //mientras flag == 1, revisamos datos
        {
            while (PIR1bits.RCIF == 0);     //esperar dato
            
            select = rx;
            
            switch (select)
            {
                case ('1'):
                    print("\r El potenciómetro tiene un valor de \r");
                    itoa(pot_val,pot,10);       //convertir int a string
                    show(pot_val);
                    print ("\r ********** \r");
                    bandera = 0;        //apagamos flag
                    break;
                    
                case ('2'):
                    print("\r Ingrese el caracter ASCII que desea enviar \r");
                    while (PIR1bits.RCIF == 0);     //espera
                    PORTB = rx;         //se envia el valor al PORTA
                    show(rx);           //se muestra en pantalla
                    print("\r *********** \r");
                    bandera = 0;        //apagamos flag
                    break;
                    
                default:
                    print("\r :-( \r");
            }
        }
    }
    return;
}

/*
 * Funciones
 */

void setup(void)
{
    // configuración de entradas y salidas
    ANSEL = 0b00000001;         //AN0 como entrada analógica
    ANSELH = 0;                 // entradas digitales
    
    TRISB = 0;      //PORTB output
    PORTB = 0;      //se limpia el puerto
    
    // Configuración del oscilador
    OSCCONbits.IRCF = 0b0100;   //1MHz
    OSCCONbits.SCS = 1;         //internal oscilator

    // Configuración de comunicación serial
    //SYNC = 0, BRGH = 1, BRG16 = 1, SPBRG=25 <- Valores de la tabla 12-5
    TXSTAbits.SYNC = 0;     // Comunicación asíncrona full dúplex
    TXSTAbits.BRGH = 1;     // Baud rate de alta velocidad
    BAUDCTLbits.BRG16 = 1;  // 16 bits para baud rate
    
    SPBRG = 25;
    SPBRGH = 0;             // baud rate 9600 %error = 0.16
    
    RCSTAbits.SPEN = 1;     // se habilita comunicación
    TXSTAbits.TX9 = 0;      // no se utiliza el noveno bit
    TXSTAbits.TXEN = 1;     // se habilita el transmisor
    RCSTAbits.CREN = 1;     // se habilita el receptor
    
    // Configuración del ADC
    ADCON1bits.ADFM = 0;        //justificación a la izquierda
    ADCON1bits.VCFG0 = 0;       //Vref en VSS y VDD
    ADCON1bits.VCFG1 = 0;
    
    ADCON0bits.ADCS = 0b01;     //FOSC/8
    ADCON0bits.CHS = 0;         // AN0
    ADCON0bits.ADON = 1;        // se enciende el ADC
    __delay_us(50);
    
    // Configuración de las interrupciones
    
    PIR1bits.ADIF = 0;      // bandera del ADC en 0
    PIE1bits.ADIE = 1;      // Se habilitan interrupciones del ADC
    INTCONbits.PEIE = 1;    // Se habilitan interrupciones en los puertos
    INTCONbits.GIE = 1;     // se habilitan interrupciones globales
    PIE1bits.RCIE = 1;      // se habilitan interrupciones de recepción      
    
}

void print (unsigned char *word)
{
    while (*word != '\0')
    {
        while (TXIF != 1);
        TXREG = *word;
        *word++;
    }
    return;
}

void show(char select)
{
    while (TXSTAbits.TRMT == 0);
    TXREG = select;
}

