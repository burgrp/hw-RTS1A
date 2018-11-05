#include <limits.h>

namespace ookey
{
namespace rx
{

const int timeSamplesCount = 8;

class Decoder : public applicationEvents::EventHandler
{

  unsigned char buffer[1 + 256 + 4];
  int bitTime;
  int bitCounter;
  int rxEventId;

  unsigned short timeSamples[timeSamplesCount];
  int timeSampleIdx = 0;

  virtual void setTimerInterrupt(int time) = 0;
  virtual void setRfPinInterrupt(bool enabled) = 0;
  virtual void dataReceived(unsigned char *data, int len) = 0;

  void mark()
  {
    target::GPIOA.ODR.setODR(1, 1);
    for (volatile int c = 0; c < 50; c++)
      ;
    target::GPIOA.ODR.setODR(1, 0);
  }

  void onEvent()
  {
    mark();
    dataReceived(&buffer[1], buffer[0]);
  }

public:
  void init()
  {
    target::GPIOA.MODER.setMODER(1, 1);
    rxEventId = applicationEvents::createEventId();
    handle(rxEventId);
  }

  void listen()
  {
    setTimerInterrupt(0);
    setRfPinInterrupt(true);
  }

  void handleTimerInterrupt(bool val)
  {

    if (bitCounter < 0)
    {
      setTimerInterrupt(bitTime);
      bitCounter++;
    }
    else
    {
      mark();

      volatile int byteIdx = bitCounter >> 3;
      volatile int bitIdx = bitCounter & 0x07;      

      if ((byteIdx == 1 && buffer[0] != 0xDF) || (byteIdx == 2 && buffer[1] != 0x00)) {
        listen();
      } else {
        if (byteIdx > 2 && byteIdx == 2 + buffer[2] + 4)
        {
          int calculatedCrc = 0x55;
          int len = buffer[2];
          for (int c = 0; c < len; c++)
          {
            calculatedCrc += buffer[c + 2 + 1];
          }
          int bufferCrc = buffer[2 + 1 + len] | buffer[2 + 1 + len + 1] << 8;
          if (bufferCrc == calculatedCrc)
          {
            applicationEvents::schedule(rxEventId);
            setTimerInterrupt(0);
          } else {
            listen();
          }
        }
        else
        {
          if (bitIdx == 0)
          {
            buffer[byteIdx] = 0;
          }
          buffer[byteIdx] |= val << bitIdx;
          bitCounter++;
        }
      }
    }
  }

  void getTimes(int offset, int &min, int &max)
  {
    for (int c = 0; c<timeSamplesCount>> 1; c++)
    {
      int t = timeSamples[(timeSampleIdx - c - offset) & (timeSamplesCount - 1)];
      if (t > max)
      {
        max = t;
      }
      if (t < min)
      {
        min = t;
      }
    }
  }

  void handleRfPinInterrupt(int time)
  {
    timeSamples[timeSampleIdx] = time;

    int fastMin = INT_MAX;
    int fastMax = INT_MIN;
    int slowMin = INT_MAX;
    int slowMax = INT_MIN;
    getTimes(-timeSamplesCount >> 1, fastMin, fastMax);
    getTimes(0, slowMin, slowMax);

    if (
        fastMax * 5 < slowMin * 4 &&
        fastMin * 5 > slowMax * 2)
    {
      bitTime = fastMin + fastMax >> 1;
      bitCounter = -1;
      setRfPinInterrupt(false);
      setTimerInterrupt(5 * bitTime >> 3);
    }

    volatile int x = fastMin + fastMax + slowMin + slowMax;
    timeSampleIdx = (timeSampleIdx + 1) & (timeSamplesCount - 1);
  }
};

} // namespace rx
} // namespace ookey