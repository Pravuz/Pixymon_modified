//
// begin license header
//
// This file is part of Pixy CMUcam5 or "Pixy" for short
//
// All Pixy source code is provided under the terms of the
// GNU General Public License v2 (http://www.gnu.org/licenses/gpl-2.0.html).
// Those wishing to use Pixy source code, software and/or
// technologies under different licensing terms should contact us at
// cmucam@cs.cmu.edu. Such licensing terms are available for
// all portions of the Pixy codebase presented here.
//
// end license header
//

#include <QPainter>
#include <QFont>
#include "debug.h"
#include <QtDebug>
#include <QFile>
#include "renderer.h"
#include "videowidget.h"
#include "interpreter.h"
#include "dataexport.h"
#include "monmodule.h"
#include <chirp.hpp>
#include "calc.h"
#include <math.h>

Renderer::Renderer(VideoWidget *video, Interpreter *interpreter) : MonModule(interpreter), m_background(0, 0)
{
    m_video = video;
    m_interpreter = interpreter;

    m_rawFrame.m_pixels = new uint8_t[RAWFRAME_SIZE];
    m_rawFrame.m_height = 0;
    m_rawFrame.m_width = 0;

    firstImg = true;
    p_crossHair.m_x = CROSSHAIR_X;
    p_crossHair.m_y = CROSSHAIR_Y;
    q_crossHair.setX(CROSSHAIR_X);
    q_crossHair.setY(CROSSHAIR_Y);
    treshPixelChange = 15;
    vectorStdDev = 60;
    nOfP = 3000;
    mdMode = true;
    medianMode = true;
    bwMode = false;
    contObSepMode = false;

    vectorsToCrosshair.reserve(6);

    qRegisterMetaType<Point16>("Point16");

    connect(this, SIGNAL(image(QImage, uchar)), m_video, SLOT(handleImage(QImage, uchar)));
    connect(this, SIGNAL(drawRect(Point16,Point16,Point16,bool)), this, SLOT(drawObjRect(Point16, Point16,Point16,bool)),Qt::QueuedConnection);
    connect(this, SIGNAL(objCalc()), this, SLOT(objCalcs()),Qt::QueuedConnection);
    connect(this, SIGNAL(sepObj(int, int)), this, SLOT(separateObjects(int, int)),Qt::QueuedConnection);
    connect(this, SIGNAL(sepObjs()), this, SLOT(separateAllObjects()),Qt::QueuedConnection);
}

Renderer::~Renderer()
{
    delete[] m_rawFrame.m_pixels;
}

inline void Renderer::interpolateBayer(unsigned int width, unsigned int x, unsigned int y, unsigned char *pixel, unsigned int &r, unsigned int &g, unsigned int &b)
{
    if (y&1)
    {
        if (x&1)
        {
            r = *pixel;
            g = (*(pixel-1)+*(pixel+1)+*(pixel+width)+*(pixel-width))>>2;
            b = (*(pixel-width-1)+*(pixel-width+1)+*(pixel+width-1)+*(pixel+width+1))>>2;
        }
        else
        {
            r = (*(pixel-1)+*(pixel+1))>>1;
            g = *pixel;
            b = (*(pixel-width)+*(pixel+width))>>1;
        }
    }
    else
    {
        if (x&1)
        {
            r = (*(pixel-width)+*(pixel+width))>>1;
            g = *pixel;
            b = (*(pixel-1)+*(pixel+1))>>1;
        }
        else
        {
            r = (*(pixel-width-1)+*(pixel-width+1)+*(pixel+width-1)+*(pixel+width+1))>>2;
            g = (*(pixel-1)+*(pixel+1)+*(pixel+width)+*(pixel-width))>>2;
            b = *pixel;
        }
    }
}

void Renderer::setParameters(unsigned int treshPixelChange, unsigned int vectorStdDev, unsigned int nOfP, bool mdEnabled, bool medianEnabled, bool bwEnabled, bool contObSepEnabled)
{    
    if(!mdEnabled || medianMode != medianEnabled)
    {
        firstImg = false;
        vectorsToCrosshair.clear();
    }

    this->treshPixelChange = treshPixelChange;
    this->vectorStdDev = vectorStdDev;
    this->nOfP = nOfP;
    mdMode = mdEnabled;
    medianMode = medianEnabled;
    bwMode = bwEnabled;
    contObSepMode = contObSepEnabled;
}

void Renderer::separateObjects(const int x, const int y)
{
    if(!(315 > x && x > 2 && 195 > y && y > 2)) return; //pixel is out of bounds.
    bool noMatchFound = true;
    Point16 point;//curent pixel point.
    point.m_x = x;
    point.m_y = y;

    if(!m_detectedObjectList.empty())
    {
        for(QVector<m_detectedObject>::iterator i = m_detectedObjectList.begin(); noMatchFound && i != m_detectedObjectList.end(); i++)
        {
            if(i->sameObject(point)) noMatchFound = false;
        }
    }
    if(noMatchFound)
    {
        m_detectedObject xyb;
        //initial object values
        xyb.topLeft = xyb.bottomRight = point;
        xyb.objPixels = 1;

        m_detectedObjectList.append(xyb);
    }
}

void Renderer::separateAllObjects()
{
    bool noMatchFound = true;
    Point16 point;//curent pixel point.

    for(int y = 3; y < 195; y++) //only focus on motion within these boundaries.
        for(int x = 3; x < 315; x++)
        {
            noMatchFound = true;
            if(m_lastBwImg[x][y] == 0x00) continue; //skip this iteration, this pixel doesn't have a change.
            point.m_x = x;
            point.m_y = y;

            if(!m_detectedObjectList.empty())
            {
                for(QVector<m_detectedObject>::iterator i = m_detectedObjectList.begin(); noMatchFound && i != m_detectedObjectList.end(); i++)
                {
                    if(i->sameObject(point)) noMatchFound = false;
                }
            }
            if(noMatchFound)
            {
                m_detectedObject xyb;
                //initial object values
                xyb.topLeft = xyb.bottomRight = point;
                xyb.objPixels = 1;

                m_detectedObjectList.append(xyb);
            }
        }

    emit objCalc();
    //since all these signals are queued, we might end up with a situation where
    //m_detectedObjectList is cleared by objCalcs()while this thread is working.
    //so, we might have to copy and send the entire list to objCalc.
    //this will require alot of memory however, so hopefully we don't have to.
}

void Renderer::treshFilter(){}

void Renderer::objCalcs()
{

    // the following will propably never be the case. but u never know :p
    if(m_detectedObjectList.empty()) return;
    m_detectedObject m_largestObjPointer;
    m_largestObjPointer = m_detectedObjectList[0];
    for(int i = 1; i<m_detectedObjectList.size(); i++)
        if(m_largestObjPointer.objPixels < m_detectedObjectList[i].objPixels)
            m_largestObjPointer = m_detectedObjectList[i];

    // memory leakage deluxe if we don't clear this after each image is produced.
    // this is also the most reasonable (if not the only) place where we can safely clear the list.
    m_detectedObjectList.clear();

    m_largestObjPointer.midPoint.m_x = (m_largestObjPointer.bottomRight.m_x - m_largestObjPointer.topLeft.m_x)/2 + m_largestObjPointer.topLeft.m_x;
    m_largestObjPointer.midPoint.m_y = (m_largestObjPointer.topLeft.m_y - m_largestObjPointer.bottomRight.m_y)/2 + m_largestObjPointer.bottomRight.m_y;

    Point16 m_vector, m_meanOrMedianVector, m_varianceVector, m_stdDevVector;
    bool m_vectorApproved = false;

    m_vector.m_x = p_crossHair.m_x - m_largestObjPointer.midPoint.m_x;
    m_vector.m_y = p_crossHair.m_y - m_largestObjPointer.midPoint.m_y;

    if(!vectorsToCrosshair.empty())
    {
        if(!medianMode)
        {
            for(QList<Point16>::iterator i = vectorsToCrosshair.begin(); i != vectorsToCrosshair.end(); i++)
            {
                m_meanOrMedianVector.m_x += i->m_x;
                m_meanOrMedianVector.m_y += i->m_y;
            }
            m_meanOrMedianVector.m_x = m_meanOrMedianVector.m_x/vectorsToCrosshair.size();
            m_meanOrMedianVector.m_y = m_meanOrMedianVector.m_y/vectorsToCrosshair.size();

            for(QList<Point16>::iterator i = vectorsToCrosshair.begin(); i != vectorsToCrosshair.end(); i++)
            {
                m_varianceVector.m_x += (m_meanOrMedianVector.m_x - i->m_x)*(m_meanOrMedianVector.m_x - i->m_x);
                m_varianceVector.m_y += (m_meanOrMedianVector.m_y - i->m_y)*(m_meanOrMedianVector.m_y - i->m_y);
            }
            m_varianceVector.m_x = m_varianceVector.m_x/vectorsToCrosshair.size();
            m_varianceVector.m_y = m_varianceVector.m_y/vectorsToCrosshair.size();

            m_stdDevVector.m_x = (int)sqrt(m_varianceVector.m_x);
            m_stdDevVector.m_y = (int)sqrt(m_varianceVector.m_y);

            if(!(m_vector.m_x > (int)(m_meanOrMedianVector.m_x + (m_stdDevVector.m_x/2) + vectorStdDev)))
            {//This vector is approved for use
                m_vectorApproved = true;
            }//else; this vector deviates too much. but we still add it to the list for future references.

        }
        else
        {
            QList<int> medianOfVTC_x, medianOfVTC_y;
            for(QList<Point16>::iterator i = vectorsToCrosshair.begin(); i != vectorsToCrosshair.end(); i++)
            {
                medianOfVTC_x.append(i->m_x);
                medianOfVTC_y.append(i->m_y);
            }
            qSort(medianOfVTC_x.begin(),medianOfVTC_x.end());
            qSort(medianOfVTC_y.begin(),medianOfVTC_y.end());

            if(medianOfVTC_x.size() == 5){
                m_meanOrMedianVector.m_x = medianOfVTC_x.at(2);
                m_meanOrMedianVector.m_y = medianOfVTC_y.at(2);
            }
            m_vectorApproved = true;
        }
    }
    vectorsToCrosshair.append(m_vector);
    while(vectorsToCrosshair.size() > 5) vectorsToCrosshair.removeAt(0);


    if(!medianMode) emit drawRect(m_largestObjPointer.topLeft, m_largestObjPointer.bottomRight, m_largestObjPointer.midPoint, m_vectorApproved);
    else emit drawRect(m_largestObjPointer.topLeft, m_largestObjPointer.bottomRight, m_meanOrMedianVector, m_vectorApproved);
}

void Renderer::drawObjRect(const Point16 tL, const Point16 bR, const Point16 objMidOrVect, bool drawVector)
{
    QPoint q_tL(tL.m_x-1,tL.m_y+1), q_bR(bR.m_x+1,bR.m_y-1), q_objMid;
    if(medianMode)
    {
        q_objMid.setX(p_crossHair.m_x - objMidOrVect.m_x);
        q_objMid.setY(p_crossHair.m_y - objMidOrVect.m_y);
    }
    else
    {
        q_objMid.setX(objMidOrVect.m_x);
        q_objMid.setY(objMidOrVect.m_y);
    }

    QRect objRect(q_tL,q_bR);
    QPainter p;

    QImage img;
    img = m_background;

    p.begin(&img);
    if(drawVector)p.setBrush(QBrush(QColor(255, 0,0 , 90)));
    p.setPen(QPen(QColor(255, 0, 0, 150)));

    p.drawRect(objRect);
    if(drawVector) p.drawLine(q_objMid, q_crossHair);
    p.end();

    emit image(img, RENDER_FLAG_FLUSH | RENDER_FLAG_BLEND);

}

int Renderer::renderBA81(uint8_t renderFlags, uint16_t width, uint16_t height, uint32_t frameLen, uint8_t *frame)
{
    uint8_t greyTemp, grey;
    uint16_t x, y;
    uint32_t *line;
    uint32_t r, g, b;

    bool imageComplete,nOfChangesAboveTresh = false;
    uint16_t nOfChanges = 0;


    if (width*height>RAWFRAME_SIZE)
    {
        m_rawFrame.m_width = 0;
        m_rawFrame.m_height = 0;
    }
    else
    {
        memcpy(m_rawFrame.m_pixels, frame, width*height);
        m_rawFrame.m_width = width;
        m_rawFrame.m_height = height;
    }

    // skip first line
    frame += width;

    // don't render top and bottom rows, and left and rightmost columns because of color
    // interpolation
    QImage img(width-2, height-2, QImage::Format_RGB32);

    for (y=1; y<height-1; y++)
    {
        line = (unsigned int *)img.scanLine(y-1);
        frame++;
        for (x=1; x<width-1; x++, frame++)
        {
            interpolateBayer(width, x, y, frame, r, g, b);

            if(mdMode)
            {
                if(nOfChanges>=nOfP) nOfChangesAboveTresh = true;
                if(x==318 && y==198) imageComplete = true;
                greyTemp = grey = (r+g+b)/3; //converting to grayscale
                if(!firstImg)
                {
                    if((unsigned int)abs(m_lastImg[x-1][y-1] - grey) > treshPixelChange) //checks previous pixel in this point for difference
                    {
                        grey = 0xff;
                        nOfChanges++;
                        if(contObSepMode && !nOfChangesAboveTresh) emit sepObj((int)x-1,(int)y-1);
                    }
                    else
                    {
                        grey = 0x00;
                        //qDebug() << "No moving object in this pixel x=" <<x<<",y="<<y;
                    }
                }
                m_lastImg[x-1][y-1] = greyTemp;
                m_lastBwImg[x-1][y-1] = grey;
                if(bwMode) *line++ = (0xff<<24) | (grey<<16) | (grey<<8) | (grey<<0);
                else *line++ = (0xff<<24) | (r<<16) | (g<<8) | (b<<0);
            }
            else
                *line++ = (0xff<<24) | (r<<16) | (g<<8) | (b<<0);
        }
        frame++;
    }

    m_background = img;

    // send image to ourselves across threads
    // from chirp thread to gui thread
    if(!mdMode)emit image(img, renderFlags);
    if(mdMode && nOfChangesAboveTresh)emit image(img, renderFlags);

    if(firstImg && mdMode) firstImg = false;
    if(imageComplete && contObSepMode  && !nOfChangesAboveTresh) emit objCalc();
    else if(!contObSepMode  && !nOfChangesAboveTresh) emit sepObjs();

    return 0;
}

bool Renderer::render(uint32_t fourcc, const void *args[])
{
    // choose fourcc for representing formats fourcc.org
    if (fourcc==FOURCC('B','A','8','1'))
    {
        renderBA81(*(uint8_t *)args[0], *(uint16_t *)args[1], *(uint16_t *)args[2], *(uint32_t *)args[3], (uint8_t *)args[4]);
        return true;
    }
    else // format not recognized
    {
        qDebug() << "Format not recognized.";
        return false;
    }
}

/// \note Save image.
void Renderer::pixelsOut(int x0, int y0, int width, int height)
{
    uint pixel, *line;
    uint r, g, b;
    int x, y;

    DataExport dx(m_interpreter->m_pixymonParameters->value("Document folder").toString(), "pixels", ET_MATLAB);

    dx.startArray(3, "pixels");

    for (y=0; y<height; y++)
    {
        line = (unsigned int *)m_background.scanLine(y0+y);
        for (x=0; x<width; x++)
        {
            pixel = line[x0+x];
            b = pixel&0xff;
            pixel >>= 8;
            g = pixel&0xff;
            pixel >>= 8;
            r = pixel&0xff;

            dx.addElement(r);
            dx.addElement(g);
            dx.addElement(b);
        }
    }
}

int Renderer::renderBackground(uint8_t renderFlags)
{
    if (m_background.width()!=0)
        emit image(m_background, renderFlags);

    return 0;
}

int Renderer::saveImage(const QString &filename)
{
    return m_background.save(filename);
}


QImage *Renderer::backgroundImage()
{
    return &m_background;
}

Frame8 *Renderer::backgroundRaw()
{
    return &m_rawFrame;
}
