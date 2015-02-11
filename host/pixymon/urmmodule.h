#ifndef URMMODULE
#define URMMODULE


#include "monmodule.h"
//#include "qqueue.h"
#include "pixytypes.h"

// color connected components
class UrmModule : public MonModule
{
public:
    UrmModule(Interpreter *interpreter);
    ~UrmModule();

    virtual bool render(uint32_t fourcc, const void *args[]);
    virtual bool command(const QStringList &argv);
    virtual void paramChange();

private:

    void rls(const Frame8 *frame);
    void handleLine(uint8_t *line, uint16_t width);

    //uint32_t m_crc;
    //Qqueue *m_qq;
    uint8_t m_renderMode;
};

#endif // URMMODULE

