namespace ookey
{
namespace tx
{

const unsigned char PREAMBLE[2] = {0xFF, 0xFF};

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
        for (int c = 0; c < len; c++)
        {
            buffer[i++] = data[c];
        }

        size = i;
        counter = 0;
        timerEnable(true);
    }

    void tick()
    {
        int bitCnt = (counter >> 1) & 0x07;
        int byteCnt = (counter >> 4);
        if (byteCnt < size)
        {
            setRfPin((counter & 1) & ((buffer[byteCnt] >> bitCnt) & 1));

            counter++;
        }
        else
        {
            setRfPin(0);
            timerEnable(false);
        }
    }

}; // namespace tx

} // namespace tx
} // namespace ookey