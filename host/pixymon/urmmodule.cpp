#include <stdio.h>
#include "urmmodule.h"
#include "interpreter.h"
#include "renderer.h"
//#include "qqueue.h"
#include "calc.h"

// declare module
MON_MODULE(UrmModule)


UrmModule::UrmModule(Interpreter *interpreter) : MonModule(interpreter)
{

    //m_qq = new Qqueue(); //??
    m_interpreter->m_pixymonParameters->addCheckbox("Enable MotionDetection",true,"Turn feature on/off.","uRobotics");
    m_interpreter->m_pixymonParameters->add("Pixel-change treshold",PT_INT8,15,"0-255 (4-20 recommended. Depends on light conditions etc.).", "uRobotics");
    m_interpreter->m_pixymonParameters->add("Maximum # of pixel changes",PT_INT16,3000,"1-32000 - Choose a lower value for objects far away. \nHigher value = more cpu-demanding","uRobotics");
    m_interpreter->m_pixymonParameters->add("Vector StdDev treshold",PT_INT8,60,"0-255 (Try 1-20 for slow objects, 21-255 for faster objects).\nOnly active with Median Vector Disabled!", "uRobotics");
    m_interpreter->m_pixymonParameters->addCheckbox("Enable Median Vector",true,"Turn feature on/off. \nLess cpu-demanding","uRobotics");
    m_interpreter->m_pixymonParameters->addCheckbox("Enable BlackWhite",false,"Turn feature on/off.","uRobotics");
    m_interpreter->m_pixymonParameters->addCheckbox("Separate Objects Continuously",false,"If unchecked, the objects will be separated after an image is processed (tracking lags 1 frame behind). \n Continuous separation is roughly twice as cpu-demanding.","uRobotics");

    m_renderMode = pixymonParameter("Motion detection render mode").toUInt();

}

UrmModule::~UrmModule()
{
    //delete m_qq;
}

bool UrmModule::render(uint32_t fourcc, const void *args[])
{
    if(fourcc==FOURCC('B','A','8','1'))
    {
        m_renderer->renderBA81(*(uint8_t *)args[0], *(uint16_t *)args[1], *(uint16_t *)args[2], *(uint32_t *)args[3], (uint8_t *)args[4]);
        return true;
    }
    return false;
}

bool UrmModule::command(const QStringList &argv)
{
    return false;
}

void UrmModule::paramChange()
{
    try{
        if(m_interpreter->m_pixymonParameters->value("Maximum # of pixel changes").toUInt() < 1)
            m_interpreter->m_pixymonParameters->value("Maximum # of pixel changes").setValue(1);
        if(m_interpreter->m_pixymonParameters->value("Maximum # of pixel changes").toUInt() > 32000)
            m_interpreter->m_pixymonParameters->value("Maximum # of pixel changes").setValue(32000);
    }catch(...)
    {
        //qDebug() << "wrong input";
    }

    m_renderer->setParameters(
                m_interpreter->m_pixymonParameters->value("Pixel-change treshold").toUInt(),
                m_interpreter->m_pixymonParameters->value("Vector StdDev treshold").toUInt(),
                m_interpreter->m_pixymonParameters->value("Maximum # of pixel changes").toUInt(),
                m_interpreter->m_pixymonParameters->value("Enable MotionDetection").toBool(),
                m_interpreter->m_pixymonParameters->value("Enable Median Vector").toBool(),
                m_interpreter->m_pixymonParameters->value("Enable BlackWhite").toBool(),
                m_interpreter->m_pixymonParameters->value("Separate Objects Continuously").toBool());
}

void UrmModule::rls(const Frame8 *frame)
{
    // ???
//    uint32_t y;
//    Qval eof(0, 0, 0, 0xffff);

//    for (y=1; y<(uint32_t)frame->m_height; y++)
//        handleLine(frame->m_pixels+y*frame->m_width, frame->m_width);

//    indicate end of frame
//    m_qq->enqueue(&eof);
}

void UrmModule::handleLine(uint8_t *line, uint16_t width)
{
    //??
}
