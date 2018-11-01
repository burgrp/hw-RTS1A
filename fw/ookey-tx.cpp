namespace ookey
{
namespace tx
{

// 16 sync manchaster pulses
const unsigned char PREAMBLE[4] = {0xFF, 0xFF, 0xFF, 0xAA}; 

class Driver
{

    int counter;

    virtual void timerEnable(bool enabled) = 0;
    virtual void setRfPin(bool state) = 0;

    unsigned char buffer[256];
    int size;

  public:
    void init()
    {
    }

    bool busy()
    {
        return false;
    }

    void send(unsigned char *data, int len)
    {
        int i = 0;
        for (int c = 0; c < sizeof(PREAMBLE); c++)
        {
            buffer[i++] = PREAMBLE[c];
        }
        buffer[i++] = len;
        int crc = 0x55;
        for (int c = 0; c < len; c++)
        {
            buffer[i++] = data[c];
            crc += data[c];
        }
        buffer[i++] = crc & 0xFF;
        buffer[i++] = (crc >> 8) & 0xFF;
        

        size = i;
        counter = 0;
        timerEnable(true);
    }

    void tick()
    {
        int byteIdx = (counter >> 4);
        if (byteIdx < size)
        {
            int bitIdx = (counter >> 1) & 0x07;
            setRfPin(((buffer[byteIdx] >> bitIdx) ^ counter) & 1);

            counter++;
        }
        else
        {
            setRfPin(0);
            timerEnable(false);
        }
    }
};

} // namespace tx
} // namespace ookey