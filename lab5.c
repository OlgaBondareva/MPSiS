#include <msp430.h> 

int mode = 0;

unsigned char number[12] [8] = { {0x00,0xf0,0x90,0x90,0x90,0x90,0x90,0xf0}, // 0
								 {0x00,0x80,0x80,0x80,0x80,0x80,0x80,0x80}, // 1
								 {0x00,0xf0,0x80,0x80,0xf0,0x10,0x10,0xf0}, // 2
								 {0x00,0xf0,0x80,0x80,0xf0,0x80,0x80,0xf0}, // 3
								 {0x00,0x90,0x90,0x90,0xf0,0x80,0x80,0x80}, // 4
								 {0x00,0xf0,0x10,0x10,0xf0,0x80,0x80,0xf0}, // 5
								 {0x00,0xf0,0x10,0x10,0xf0,0x90,0x90,0xf0}, // 6
								 {0x00,0xf0,0x80,0x80,0x80,0x40,0x20,0x10}, // 7
								 {0x00,0xf0,0x90,0x90,0xf0,0x90,0x90,0xf0}, // 8
								 {0x00,0xf0,0x90,0x90,0xf0,0x80,0x80,0xf0}, // 9
								 {0x00,0x00,0x00,0x00,0xf0,0x00,0x00,0x00}, // -
								 {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}};// пустота

// работа с акселерометром
char cma3000_SPI(unsigned char byte1, unsigned char byte2) {
    char indata;

    P3OUT &= ~BIT5;				//P3.5 SET "0" IS START SPI OPERATION
    indata = UCA0RXBUF;
    while(!(UCA0IFG & UCTXIFG));//WAIT TXIFG == TXBUF IS READY FOR NEW DATA
    UCA0TXBUF = byte1; 			//START SPI TRANSMIT. SEND FIRST BYTE
    while(!(UCA0IFG & UCRXIFG));//WAIT RXIFG == RXBUF HAVE NEW DATA
    indata = UCA0RXBUF;
    while(!(UCA0IFG & UCTXIFG));//WAIT TXIFG == TXBUF IS READY FOR NEW DATA
    UCA0TXBUF = byte2; 			//START SPI TRANSMIT. SEND SECOND BYTE
    while(!(UCA0IFG & UCRXIFG));//WAIT RXIFG == RXBUF HAVE NEW DATA
    indata = UCA0RXBUF; 		//READ SPI DATA FROM ACCEL. IN 2 BYTE IN READ COMMAND
    while(UCA0STAT & UCBUSY);	//WAIT UNTIL USCI_A0 SPI INTERFACE IS NO LONGER BUSY
    P3OUT |= BIT5; 				//P3.5 SET "1" IS STOP SPI OPERATION
    return indata;
}

// передача на lcd
void DOGS102_SPI(unsigned char byte) {
	while (!(UCB1IFG & UCTXIFG));	// Ожидаем готовности TXBUF к приему данных.
	P7OUT &= ~BIT4;					// CS=0, Начало SPI операции.
	UCB1TXBUF = byte;				// Начало передачи.
	while (UCB1STAT & UCBUSY);		// Ожидаем пока интерфейс занят
	P7OUT |= BIT4;					// CS=1
}

void SetPos(char row, char page) {
	P5OUT &= ~BIT6;					// Режим команды

	char low = row & 0xF;
	char high = row >> 4;

	DOGS102_SPI(low);
	DOGS102_SPI(0x10 | high);

	page &= 0xF;
	DOGS102_SPI(0xB0 | page);
}

void SetData(char data) {
    P5OUT |= BIT6;				// Режим данных
    DOGS102_SPI(data);
}

void SetCmd(char cmd) {
	P5OUT &= ~BIT6;				// Режим команды
	DOGS102_SPI(cmd);
}

void PutSymbol(int symbol, int position) {
	char page, row;

	switch(position) {
		case 0: page = -1; break;
		case 1: page = 0; break;
		case 2: page = 1; break;
		case 3: page = 2; break;
	}

	if (!mode) {
		row = 132;
	} else {
		row = 102;
	}
	int i, j = 0;
	for (i = row; i > row - 8; i--, j++) {
		SetPos(i, page);
		int data = number[symbol][j];
		SetData(data);
	}
}

int main(void) {
    WDTCTL = WDTPW | WDTHOLD;	// Stop watchdog timer

    TA0CCR0 = 0x2000;
    TA0CTL = TASSEL__SMCLK + MC__UP + TACLR + ID__1;

    TA1CCR0 = 0xFFFF;
    TA1CCTL0 = CCIE;
    TA1CTL = TASSEL__SMCLK + MC__UP + TACLR + ID__4;

	P1DIR &= ~BIT7;
	P1REN |= BIT7;
	P1OUT |= BIT7;
	P1IE |= BIT7;
	P1IES |= BIT7;
	P1IFG &= ~BIT7;

	// Сигнал прерывания акселерометра
	P2DIR  &= ~BIT5;            //P2.5(CMA3000 PIN INT) INPUT
	P2OUT  |=  BIT5;            //P2.5(CMA3000 PIN INT) PULL-UP RESISTOR
	P2REN  |=  BIT5;            //P2.5(CMA3000 PIN INT) ENABLE RESISTOR
	P2IE   |=  BIT5;            //P2.5(CMA3000 PIN INT) INTERRUPT ENABLE
	P2IES  &= ~BIT5;            //P2.5(CMA3000 PIN INT) EDGE FOR INTERRUPT : LOW-TO-HIGH
	P2IFG  &= ~BIT5;            //P2.5(CMA3000 PIN INT) CLEAR INT FLAG
	// Выбор устройства
	P3DIR  |=  BIT5;            //P3.5(CMA3000 PIN CSB) SET AS OUTPUT
	P3OUT  |=  BIT5;            //P3.5(CMA3000 PIN CSB) SET "1" IS DISABLE CMA3000
	// Синхросигнал
	P2DIR  |=  BIT7;            //P3.5(CMA3000 PIN SCK) SET AS OUTPUT
	P2SEL  |=  BIT7;            //DEVICE MODE : P2.7 IS UCA0CLK
	// Линия передачи по SPI и Линия приема по SPI
	P3DIR  |= (BIT3 | BIT6);    //P3.5 & P3.6(CMA3000 PIN MOSI, PWR) SET AS OUTPUT
	P3DIR  &= ~BIT4;            //P3.4(CMA3000 PIN MISO) SET AS INPUT
	P3SEL  |= (BIT3 | BIT4);    //DEVICE MODE : P3.3 - UCA0SIMO, P3.4 - UCA0SOMI
	P3OUT  |= BIT6;             //P3.6(CMA3000 PIN PWR) SET "1" IS POWER CMA3000

    P5DIR	|= BIT6 | BIT7;// CD и RST устанавливаем на выход.
    P7DIR	|= BIT4 | BIT6;// CS и ENA устанавливаем на выход.
    P7OUT	|= BIT4 | BIT6;// CS & ENA no select on bkLED.
    P4DIR	|= BIT1 | BIT3; // SCK и SDA устанавливаем на выход.
    P4SEL	|= BIT1 | BIT3;// Режим устройста для SCK и SDA = UCB1SIMO & UCB1CLK

    P5OUT	&= ~BIT7;// Сброс при нуле.
    __delay_cycles(25000);
    P5OUT	|= BIT7;// Снимаем флаг сброса.
    __delay_cycles(125000);

    // для акселерометра
    UCA0CTL1 |= UCSWRST;
    UCA0CTL0 |= UCSYNC | UCMST | UCMSB | UCCKPH; // sync Master  MSB
    UCA0CTL1 |= UCSWRST | UCSSEL__SMCLK; // выбор источника ТИ
    UCA0BR0 = 0x30; // младший байт делителя частоты
    UCA0BR1 = 0;
    UCA0MCTL = 0;
    UCA0CTL1 &= ~UCSWRST;

    // для ЖКИ
    UCB1CTL1 |= UCSWRST;// Сброс логики интерфейса.
    UCB1CTL0 |= UCSYNC | UCMST | UCMSB | UCCKPH;// Синхронный режим, тактирование генератором USCI.
    UCB1CTL1 |= UCSWRST | UCSSEL__SMCLK;
    UCB1BR0	= 0x30;// Деление частоты, младшая и старшая части.
    UCB1BR1	= 0;
    UCB1CTL1 &= ~UCSWRST;// Снимаем бит сброса и разрешаем работу модуля

    P5OUT &= ~BIT6; // Режим: команда
    DOGS102_SPI(0x2F); //Power On
    DOGS102_SPI(0xAF); //Display On

    int i, j;
    for (i = 30; i < 132; ++i) {
        for (j = 0; j < 8; ++j) {
        	SetPos(i, j);
        	SetData(0);
        }
    }

    cma3000_SPI(0x4, 0);
    __delay_cycles(1250);
    cma3000_SPI(0xA, BIT7 | BIT4 | BIT2);
    __delay_cycles(25000);
    __bis_SR_register(GIE);
    __no_operation();
	return 0;
}

#pragma vector= TIMER0_A0_VECTOR
__interrupt void TIMER0_AO_ISR(void) {
	if(!(P1IN & BIT7)) {
			PutSymbol(11,0);
			PutSymbol(11,1);
			PutSymbol(11,2);
			PutSymbol(11,3);

			// меняем режим адресации
			if(mode) {
				SetCmd(0xA0);
			}
			else {
				SetCmd(0xA1);
			}
			mode = !mode;
	}
	P1IE |= BIT7;
	TA0CCTL0 &= ~CCIE;
	TA0CCTL0 &= ~CCIFG;
}

#pragma vector = PORT1_VECTOR
__interrupt void PORT1_ISR(void) {
	P1IE &= ~BIT7;
    TA0CCTL0 = CCIE;
    TA0CTL |= TACLR;
    P1IFG &= ~BIT7;
}

#pragma vector = TIMER1_A0_VECTOR
__interrupt void TIMER1_AO_ISR(void) {
	int symbol, i;
	signed char value;
	value = cma3000_SPI(0x18, 0); //0x18 - X
	if(value < 0) {
		PutSymbol(10,0);
		value *= -1;
	}
	else {
		PutSymbol(11,0);
	}

	for ( i = 3; i > 0; i--) {
		symbol = value % 10;
		value /= 10;
		PutSymbol(symbol,i);
	}
	P2IFG &= ~BIT5;
}
