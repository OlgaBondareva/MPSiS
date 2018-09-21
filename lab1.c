#include <msp430.h> 

/*
 * main.c
 */

int buttonS1State = 0;

void setupLeds()
{
	P1DIR = BIT0; // OUT 1 LED
	P1OUT = 0;

	P8DIR = (BIT1 | BIT2); // OUT 2 and 3 LED
	P8OUT = 0;
}

int getButtonS1State()
{
	return (P1IN & BIT7) == 0;
}

void turnOffLeds()
{
	P1OUT &= ~BIT0;  //led1
	P8OUT &= ~BIT1;  //led2
	P8OUT &= ~BIT2;  //led3
	buttonS1State = 0;
	while(getButtonS1State() == 1);
}

void setupS1Button()
{
	P1REN |= BIT7;
	P1OUT |= BIT7;
}

int customDelay(long delayValue)
{
	volatile int i = 0;
	for(i = 0; i++ < delayValue;)
	{
		if(buttonS1State && getButtonS1State())
		{
			turnOffLeds();
			return 1;
		}
	}
	return 0;
}

int main(void) {
	WDTCTL = WDTPW | WDTHOLD; // Stop watchdog timer

	const long delayValue = 16598;

	setupLeds();
	setupS1Button();

	for(;;)
	{
		if (getButtonS1State())
		{
			if(!buttonS1State)
			{
				while(getButtonS1State() == 1);
				buttonS1State = 1;
				P1OUT |= BIT0;  // turn led1
				if(customDelay(delayValue))
				{
					continue;
				}
				P8OUT |= BIT1;  // turn led2
				if(customDelay(delayValue))
				{
					continue;
				}
				P8OUT |= BIT2;  // turn led3
				while(getButtonS1State() == 1);
			}
			else
			{
				turnOffLeds();
			}
		}
	}
	return 0;
}
