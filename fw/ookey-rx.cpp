#include <limits.h>

namespace ookey
{
namespace rx
{

const int timeSamplesCount = 8;

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

  void mark() {
      target::GPIOA.ODR.setODR(1, 1);
      for (volatile int c = 0; c < 50; c++)
        ;
      target::GPIOA.ODR.setODR(1, 0);   
  }

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

  // int getAvgTime(int offset) {
  //   int sum = 0;
  //   for (int c = 0; c < 4; c++) {
  //     sum += timeSamples[(timeSampleIdx - c - offset) & (timeSamplesCount - 1)];
  //   }
  //   return sum >> 2;
  // }

  void getTimes(int offset, int& min, int& max) {
    for (int c = 0; c < timeSamplesCount >> 1; c++) {
      int t = timeSamples[(timeSampleIdx - c - offset) & (timeSamplesCount - 1)];
      if (t > max) {
        max = t;
      }
      if (t < min) {
        min = t;
      }
    }
  }

  void handleRfPinInterrupt(int time)
  {
    timeSamples[timeSampleIdx] = time;

    // volatile int avgFast = getAvgTime(-4);
    // volatile int avgSlow = getAvgTime(0);

    // // preamble pattern:
    // // are last 4 pulses approximately two times wider than previous 4 pulses?
    // volatile int avgFastByTwo = avgFast << 1;
    // volatile int diff = avgFastByTwo - avgSlow;
    // if (diff < 0) {
    //   diff = -diff;
    // }

    // if (diff < avgFast >> 2) {
    //   mark();
    //   diff++;
    // }

    int fastMin = INT_MAX;
    int fastMax = INT_MIN;
    int slowMin = INT_MAX;
    int slowMax = INT_MIN;
    getTimes(-timeSamplesCount >> 1, fastMin, fastMax);
    getTimes(0, slowMin, slowMax);

    if (
      fastMax * 5 < slowMin * 4 &&
      fastMin * 5 > slowMax * 2
    ) {
      bitTime = (fastMin + fastMax) >> 2;
      mark();
    }

    volatile int x  = fastMin + fastMax + slowMin + slowMax;
    timeSampleIdx = (timeSampleIdx + 1) & (timeSamplesCount - 1);

  }
};

} // namespace rx
} // namespace ookey