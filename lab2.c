#include <msp430.h>

volatile int mode = 0;

void led3Off ()
{
	P8OUT &= ~BIT2;  //led3
}

void led3On()
{
	P8OUT |= BIT2;
}

int main(void)
{
	// Переводим сторожевой таймер в интервальный режим
	// WDTPW - макрос, определяющий пароль сторожевого таймера
	// WDTSSELx - Выбор источника счетного сигнала (0 - fACLK)
	// WDTTMSEL - Выбор режима (сторожевой или интервальный)
	// WDTCNTCL - Очистка регистра счетчика
	// WDTISx - Делитель частоты
	WDTCTL = WDTPW + WDTTMSEL + WDTCNTCL + WDTIS2 + WDTSSEL0;	//1000ms

	led3Off();

	P2REN |= BIT2;		// Разрешение подтягивающего резистора для Р2.2
	P1REN |= BIT7;		// Разрешение подтягивающего резистора для Р1.7

	P8DIR |= BIT2;      // P8.2 set as output (led3)
	P2DIR &= !(BIT2); 	// Р2.2 (S2) set as input
	P1DIR &= !(BIT7); 	// Р1.7 (S1) set as input
	P2OUT |= BIT2;
	P1OUT |= BIT7;
	P8OUT |= !BIT2;

	P1IFG &= ~BIT7;  	// Clear interrupt flag for P1.7
	P1IES |= BIT7;		// Falling Edge for P1.7
	P1IE |= BIT7;		// Enable interrupt for P1.7

	P2IFG &= ~BIT2;		// Clear interrupt flag for P2.2
	P2IES |= BIT2;		// Falling Edge for P2.2
    P2IE |= BIT2;		// Enable interrupt for P2.2

  __bis_SR_register(LPM0_bits + GIE);       // Enter LPM0 (режим низкого энергопотребления), enable interrupts
  __no_operation();                         // For debugger

  return 0;
}

#pragma vector=PORT2_VECTOR
__interrupt void PORT2_ISR(void)
{
	P2IE &= ~BIT2;		// Запрещаем прерывания
	P2IES ^= BIT2;		// Фронт/Спад

	led3On();

	SFRIE1 |=  WDTIE;   // Разрешаем прерывания от сторожевого таймера
	P2IE |= BIT2;		// Разрешаем прерывания
	P2IFG &= ~BIT2;		// Очищаем флаг прерывания P2.2
}

#pragma vector=PORT1_VECTOR
__interrupt void PORT1_ISR(void)
{
		P1IE &= ~BIT7; 			// Запрещаем прерывания

		mode++;
		if (mode == 1)
		{
			WDTCTL = WDTPW + WDTTMSEL + WDTCNTCL + WDTIS2 + WDTSSEL0 + WDTIS0; //250ms
		}
		if (mode == 2)
		{
			WDTCTL = WDTPW + WDTTMSEL + WDTCNTCL + WDTIS2 + WDTIS1 + WDTSSEL0; //16ms
		}
		if (mode == 3)
		{
			WDTCTL = WDTPW + WDTTMSEL + WDTCNTCL + WDTIS2 + WDTIS1 + WDTIS0 + WDTSSEL0; //1.9ms
			mode = 0;
		}

		P1IE |= BIT7; 			// Разрешаем прерывания
		P1IFG &= ~BIT7;			// Очищаем флаг прерывания P1.7
}

#pragma vector = WDT_VECTOR
__interrupt void watchdog_timer(void){
	SFRIFG1 &= ~BIT0;	// Очищаем флаг прерывания от таймера

	SFRIE1 |=  !WDTIE;  // Запрещаем прерывания от сторожевого таймера
	led3Off();
}


