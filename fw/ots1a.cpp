target::gpio_a::Peripheral *LED_PORT = &target::GPIOA;
int LED_PIN = 1;

target::gpio_a::Peripheral *RF_PORT = &target::GPIOA;
int RF_PIN = 0;


class TxDriver: public ookey::tx::Encoder {
    virtual void setRfPin(bool state) {
        RF_PORT->BSRR = (0x10000 | state) << RF_PIN;
    }
    virtual void setTimerInterrupt(bool enabled) {
        target::TIM16.CR1.setCEN(enabled);
    }
};

TxDriver txDriver;

class LedOffTimer : public genericTimer::Timer
{
  public:
    void onTimer()
    {
        LED_PORT->BRR = 1 << LED_PIN;
    }
};

class TxTimer : public genericTimer::Timer
{
    LedOffTimer ledOffTimer;
  public:
    void onTimer()
    {
        LED_PORT->BSRR = 1 << LED_PIN;
        if (!txDriver.busy())
        {
            txDriver.send((unsigned char *)"I'm OOK!", 8);
            //txDriver.send(NULL, 0);
        }
        ledOffTimer.start(10);
        start(10);
    }
};

void interruptHandlerTIM16()
{
    txDriver.handleTimerInterrupt();
    target::TIM16.SR.setUIF(0);
}

TxTimer txTimer;

void initApplication()
{
    target::RCC.AHBENR.setIOPAEN(true);
    target::RCC.AHBENR.setIOPBEN(true);
    target::RCC.APB2ENR.setTIM16EN(1);

    target::NVIC.ISER.setSETENA(1 << target::interrupts::External::TIM16);

    LED_PORT->MODER.setMODER(LED_PIN, 1);
    RF_PORT->MODER.setMODER(RF_PIN, 1);

    target::TIM16.DIER.setUIE(1);
    target::TIM16.ARR.setARR(4000);
    target::TIM16.PSC.setPSC(0);    

    txDriver.init(0x1234);
    txTimer.onTimer();
}
