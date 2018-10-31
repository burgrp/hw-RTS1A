namespace ookey
{
namespace rx
{

const int timeSamplesCount = 15;

class Decoder
{

  unsigned char buffer[256];
  int bitTime;
  int bitCounter;

  unsigned short timeSamples[timeSamplesCount];
  int timeSampleIdx = 0;

  void listen()
  {
    setTimer(0);
    enableRfPinInterrupt(true);
  }

  void receive()
  {
    bitCounter = -1;
    enableRfPinInterrupt(false);
    setTimer(bitTime / 8);
  }

  virtual void setTimer(int time) = 0;
  virtual void enableRfPinInterrupt(bool enabled) = 0;

public:
  void init()
  {
    listen();
    target::GPIOA.MODER.setMODER(1, 1);
  }

  void handleTimerInterrupt(bool val)
  {

    if (bitCounter < 0)
    {
      setTimer(bitTime);
    }
    else
    {

      target::GPIOA.ODR.setODR(1, 1);
      for (volatile int c = 0; c < 50; c++)
        ;
      target::GPIOA.ODR.setODR(1, 0);

      volatile int byteIdx = bitCounter >> 3;
      volatile int bitIdx = bitCounter & 0x07;
      if (bitIdx == 0)
      {
        buffer[byteIdx] = 0;
      }
      buffer[byteIdx] |= val << bitIdx;
      if (byteIdx == 10)
      {
        volatile int x = 1;
        x++;
        listen();
      }
    }

    bitCounter++;
  }

  void handleRfPinInterrupt(int time)
  {
    timeSamples[timeSampleIdx++] = time;

    if (timeSampleIdx == timeSamplesCount)
    {
      timeSampleIdx = 0;
    }

    int sum = 0;
    for (int c = 0; c < timeSamplesCount; c++)
    {
      sum += timeSamples[c];
    }
    int avg = sum / timeSamplesCount;
    int maxDev = 0;
    for (int c = 0; c < timeSamplesCount; c++)
    {
      int dev = timeSamples[c] - avg;
      if (dev < 0)
      {
        dev = -dev;
      }
      if (dev > maxDev)
      {
        maxDev = dev;
      }
    }

    if (maxDev < avg / 16)
    {
      bitTime = avg;
      receive();
    }
  }
};

} // namespace rx
} // namespace ookey