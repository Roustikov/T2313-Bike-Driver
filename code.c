#include <tiny2313.h>
#include <delay.h>

#define FRONT OCR1B
#define REAR OCR1A
#define STOPS OCR0A

#define FRONT_PORT DDRB.4
#define REAR_PORT DDRB.3
#define STOPS_PORT DDRB.2

#define STROBE_DELAY 100
#define COP_DELAY 50
#define CONTROL_PIN PIND.2

int EARLY_PWM_STATE = 0;

enum mode
{
    pulse,
    dummy1,
    full,
    dummy2,
    cop_light,
    dummy3,
    off,  
    dummy4,
    strobe
}mode;

void set(int value)
{
    FRONT = value;
    REAR = value;
    STOPS = value;
}

void shutdown()
{
    FRONT_PORT = 0;
    REAR_PORT = 0;
    STOPS_PORT = 0;
}

void activate()
{
    FRONT_PORT = 1;
    REAR_PORT = 1;
    STOPS_PORT = 1;
}

void pulse_mode()
{
    int t = 3, min = 5;
    float pwm = min;
    float mlt = 1.015;
    set(min); 
    
    activate();
    
    while(pwm < 255)
    {
    if(mode!=pulse)
        return;
             
      pwm*=mlt;
      if(pwm > 255)
        break;
      set(pwm);    
      delay_ms(t);
    }               
    
    while(pwm > min)
    {
    if(mode!=pulse)
        return;
     
      pwm/=mlt;    
      if(pwm < min)
        break;
      set(pwm);
      delay_ms(t);
    }
}

void full_mode()
{
    activate();
    set(255);
}

void off_mode()
{       
    shutdown();
    //sleep_enable();
    //powerdown();
}

void strobe_mode(int delay)
{
    EARLY_PWM_STATE = FRONT;
    set(255);
    
    activate();   
    delay_ms(delay);
    shutdown();
    delay_ms(delay);
    
    set(EARLY_PWM_STATE);
}

void cop_light_mode()
{        
    int i=0;
    set(0);  
    
    for(i=0; i<7; i++)
    {        
        if(mode == cop_light)
            strobe_mode(COP_DELAY);
        else
            return;
    }
    
    shutdown();
    
    for(i=0; i<3; i++)
    {
        if(mode == cop_light)
            delay_ms(300);
        else
            return;
    }
}

void mode_switch()
{
    mode++;
    if(mode > off)
    {
        mode = pulse;
    }
}

// External Interrupt 0 service routine
interrupt [EXT_INT0] void ext_int0_isr(void)
{
    delay_ms(150);
    
    if(CONTROL_PIN == 1)
    {       
        mode--;
        while(CONTROL_PIN == 1)
        {
            strobe_mode(STROBE_DELAY);
        }
    }
    else
    {
        mode_switch();
    }      
}

void main(void)
{
// Declare your local variables here

// Crystal Oscillator division factor: 1
#pragma optsize-
CLKPR=(1<<CLKPCE);
CLKPR=(0<<CLKPCE) | (0<<CLKPS3) | (0<<CLKPS2) | (0<<CLKPS1) | (0<<CLKPS0);
#ifdef _OPTIMIZE_SIZE_
#pragma optsize+
#endif

DDRD.2 = 0;
PORTB.2 = 0;

DDRB.2 = 1;
DDRB.3 = 1;
DDRB.4 = 1;

// Timer/Counter 0 initialization
// Clock source: System Clock
// Clock value: 500,000 kHz
// Mode: Fast PWM top=0xFF
// OC0A output: Non-Inverted PWM
// OC0B output: Disconnected
// Timer Period: 0,512 ms
// Output Pulse(s):
// OC0A Period: 0,512 ms Width: 0 us
TCCR0A=(1<<COM0A1) | (0<<COM0A0) | (0<<COM0B1) | (0<<COM0B0) | (1<<WGM01) | (1<<WGM00);
TCCR0B=(0<<WGM02) | (0<<CS02) | (0<<CS01) | (1<<CS00);
TCNT0=0x00;
OCR0A=0x00;
OCR0B=0x00;

// Timer/Counter 1 initialization
// Clock source: System Clock
// Clock value: 500,000 kHz
// Mode: Fast PWM top=0x00FF
// OC1A output: Non-Inverted PWM
// OC1B output: Non-Inverted PWM
// Noise Canceler: Off
// Input Capture on Falling Edge
// Timer Period: 0,512 ms
// Output Pulse(s):
// OC1A Period: 0,512 ms Width: 0 us
// OC1B Period: 0,512 ms Width: 0 us
// Timer1 Overflow Interrupt: Off
// Input Capture Interrupt: Off
// Compare A Match Interrupt: Off
// Compare B Match Interrupt: Off
TCCR1A=(1<<COM1A1) | (0<<COM1A0) | (1<<COM1B1) | (0<<COM1B0) | (0<<WGM11) | (1<<WGM10);
TCCR1B=(0<<ICNC1) | (0<<ICES1) | (0<<WGM13) | (1<<WGM12) | (0<<CS12) | (0<<CS11) | (1<<CS10);
TCNT1H=0x00;
TCNT1L=0x00;
ICR1H=0x00;
ICR1L=0x00;
OCR1AH=0x00;
OCR1AL=0x00;
OCR1BH=0x00;
OCR1BL=0x00;

// External Interrupt(s) initialization
// INT0: On
// INT0 Mode: Rising Edge
// INT1: Off
// Interrupt on any change on pins PCINT0-7: Off
GIMSK=(0<<INT1) | (1<<INT0) | (0<<PCIE);
MCUCR=(0<<ISC11) | (0<<ISC10) | (1<<ISC01) | (1<<ISC00);
EIFR=(0<<INTF1) | (1<<INTF0) | (0<<PCIF);

// Analog Comparator initialization
// Analog Comparator: Off
// The Analog Comparator's positive input is
// connected to the AIN0 pin
// The Analog Comparator's negative input is
// connected to the AIN1 pin
ACSR=(1<<ACD) | (0<<ACBG) | (0<<ACO) | (0<<ACI) | (0<<ACIE) | (0<<ACIC) | (0<<ACIS1) | (0<<ACIS0);
// Digital input buffer on AIN0: On
// Digital input buffer on AIN1: On
DIDR=(0<<AIN0D) | (0<<AIN1D);

// Global enable interrupts
#asm("sei")

    while (1)
    {   
        if(mode%2!=0)
        {
            mode_switch();
        } 
            
        while(mode == pulse)
        {   
            pulse_mode();
        }
        
        while(mode == full)
        {   
            full_mode();
        }
        
        while(mode == cop_light)
        {              
            cop_light_mode();
        }
        
        while(mode == strobe)
        {              
            strobe_mode(STROBE_DELAY);    
        }  
        
        while(mode == off)
        {   
            off_mode();
        }
    }
}
