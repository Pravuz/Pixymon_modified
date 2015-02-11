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

#ifndef RENDERER_H
#define RENDERER_H

#include <QObject>
#include <QImage>
#include "pixytypes.h"
#include "monmodule.h"
#include <simplevector.h>
typedef SimpleVector<Point16> Points;
typedef QVector<Point16> Pixels;

#define RAWFRAME_SIZE    0x10000
#define CROSSHAIR_X 159
#define CROSSHAIR_Y 99

class Interpreter;

class VideoWidget;

struct m_detectedObject
{
    uint16_t objPixels;
    Point16 topLeft, bottomRight, midPoint;

    bool sameObject(const Point16 &p)
    {
        if(topLeft.m_x - 2 <= p.m_x && p.m_x <= bottomRight.m_x + 2 && bottomRight.m_y - 2 <= p.m_y && p.m_y <= topLeft.m_y + 2)
        {
            if(topLeft.m_x > p.m_x) topLeft.m_x = p.m_x;
            else if(bottomRight.m_x < p.m_x) bottomRight.m_x = p.m_x;
            if(topLeft.m_y < p.m_y) topLeft.m_y = p.m_y;
            else if(bottomRight.m_y > p.m_y) bottomRight.m_y = p.m_y;
            objPixels++;
            return true;
        }
        return false;
    }
};

class Renderer : public QObject, public MonModule
{
    Q_OBJECT

public:
    Renderer(VideoWidget *video, Interpreter *interpreter);
    ~Renderer();

    virtual bool render(uint32_t fourcc, const void *args[]);

    int renderBackground(uint8_t renderFlags);
    QImage *backgroundImage(); // get background from BA81 formatted image data
    Frame8 *backgroundRaw();

    int renderBA81(uint8_t renderFlags, uint16_t width, uint16_t height, uint32_t frameLen, uint8_t *frame);

    int saveImage(const QString &filename);
    void pixelsOut(int x0, int y0, int width, int height);
    void renderRL(QImage *image, uint color, uint row, uint startCol, uint len);

    Frame8 m_rawFrame;

    //uRobotics specific schtuff
    void setParameters(unsigned int treshPixelChange, unsigned int vectorStdDev, unsigned int nOfP, bool mdEnabled, bool medianEnabled, bool bwEnabled, bool contObSepEnabled);

signals:
    void image(QImage image, uchar renderFlags);
    void drawRect(const Point16 tL, const Point16 bR, const Point16 objMid, bool drawVector);
    void objCalc();
    void sepObj(const int x, const int y);
    void sepObjs();

private:
    inline void interpolateBayer(unsigned int width, unsigned int x, unsigned int y, unsigned char *pixel, unsigned int &r, unsigned int &g, unsigned int &b);

    VideoWidget *m_video;
    Interpreter *m_interpreter;
    QImage m_background;

    //uRobotics specific schtuff
    void treshFilter();

    uint8_t m_lastImg[318][198], m_lastBwImg[318][198];
    QVector<m_detectedObject> m_detectedObjectList;
    QList<Point16> vectorsToCrosshair;
    Point16 p_crossHair;
    QPoint q_crossHair;
    unsigned int treshPixelChange, vectorStdDev, nOfP;
    bool mdMode, bwMode, contObSepMode, medianMode, firstImg;

private slots:
    void drawObjRect(const Point16 tL, const Point16 bR, const Point16 objMidOrVect, bool drawVector);
    void objCalcs();
    void separateObjects(const int x, const int y);
    void separateAllObjects();
};

#endif // RENDERER_H

