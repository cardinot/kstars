/***************************************************************************
                          fitshistogram.h  -  FITS Historgram
                          ---------------
    begin                : Thu Mar 4th 2004
    copyright            : (C) 2004 by Jasem Mutlaq
    email                : mutlaqja@ikarustech.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
 
#ifndef FITSHISTOGRAM
#define FITSHISTOGRAM
 
#include "ui_fitshistogramui.h"
#include <kcommand.h>

#include <QPixmap>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QDialog>
 
#define BARS 500
 
class FITSViewer;
class QPixmap;
 
 class histogramUI : public QDialog, public Ui::FITSHistogramUI
 {
   Q_OBJECT

    public:
     histogramUI(QDialog *parent=0);

 };

 class FITSHistogram : public QDialog
 {
   Q_OBJECT
   
   public:
    FITSHistogram(QWidget *parent);
    ~FITSHistogram();
    
    void constructHistogram(float *buffer);
    int  findMax();
    int type;
    int napply;
    int maxHeight;
    int histArray[BARS]; 

    
    private:
    
    double binSize;
    histogramUI *ui;
    

    FITSViewer * viewer;
    QPixmap *histogram;
       
    
    protected:
    void paintEvent( QPaintEvent *e);
    void mouseMoveEvent( QMouseEvent *e);
    
    
    public slots:
    void applyScale();
    void updateBoxes();
    void updateIntenFreq(int x);
    
    
 };
 
 class FITSHistogramCommand : public KCommand
{
  public:
        FITSHistogramCommand(QWidget * parent, FITSHistogram *inHisto, int newType, int lmin, int lmax);
	~FITSHistogramCommand();
            
        void execute();
        void unexecute();
        QString name() const;

    
    private:
        FITSHistogram *histo;
        int type;
	int min, max;
	float *buffer;
        FITSViewer *viewer;
	QImage *oldImage;
};
 
 
#endif
