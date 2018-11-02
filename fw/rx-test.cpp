target::gpio_b_f::Peripheral *LED_PORT = &target::GPIOB;
int LED_PIN = 7;

target::gpio_a::Peripheral *RF_PORT = &target::GPIOA;
int RF_PIN = 0;

class RxDecoder : public ookey::rx::Decoder
{

    virtual void setTimerInterrupt(int time)
    {
        target::TIM16.DIER.setUIE(0);
        target::TIM16.CNT.setCNT(0);
        target::TIM16.ARR.setARR(time == 0 ? 0xFFFF : time);
        target::TIM16.SR.setUIF(0);
        target::TIM16.DIER.setUIE(time > 0);
    }

    virtual void setRfPinInterrupt(bool enabled)
    {
        target::EXTI.IMR.setMR(RF_PIN, enabled);
    }

    virtual void dataReceived(unsigned char* data, int len) {
        LED_PORT->ODR.setODR(LED_PIN, !LED_PORT->IDR.getIDR(LED_PIN));
        listen();
    }
};

RxDecoder rxDecoder;

void interruptHandlerTIM16()
{
    rxDecoder.handleTimerInterrupt(RF_PORT->IDR.getIDR(RF_PIN));
    target::TIM16.SR.setUIF(0);
}

void interruptHandlerEXTI0_1()
{
    if (target::EXTI.PR.getPR(RF_PIN))
    {
        target::EXTI.PR.setPR(RF_PIN, 1);
        int time = target::TIM16.CNT;
        target::TIM16.CNT = 0;
        rxDecoder.handleRfPinInterrupt(time);
    }
}

void initApplication()
{
    target::RCC.AHBENR.setIOPAEN(true);
    target::RCC.AHBENR.setIOPBEN(true);
    target::RCC.APB2ENR.setTIM16EN(1);

    target::NVIC.ISER.setSETENA(1 << target::interrupts::External::TIM16);

    LED_PORT->MODER.setMODER(LED_PIN, 1);

    target::TIM16.PSC.setPSC(0);
    target::TIM16.CR1.setCEN(1);

    RF_PORT->PUPDR.setPUPDR(RF_PIN, 1);
    RF_PORT->MODER.setMODER(RF_PIN, 0);
    target::SYSCFG.EXTICR1.setEXTI(0, 0);
    target::EXTI.RTSR.setTR(RF_PIN, 1);

    target::NVIC.ISER.setSETENA(1 << target::interrupts::External::EXTI0_1);

    rxDecoder.init();
    rxDecoder.listen();
}