/***************************************************************************
                          plotwidget.cpp - A widget for plotting in KStars
                             -------------------
    begin                : Sun 18 May 2003
    copyright            : (C) 2003 by Jason Harris
    email                : kstars@30doradus.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <math.h> //for log10(), pow(), modf()
#include <kdebug.h>
#include <qcolor.h>
#include <qpainter.h>
#include <qpixmap.h>
#include <qstring.h>

#include "plotwidget.h"

PlotWidget::PlotWidget( double x1, double x2, double y1, double y2, QWidget *parent, const char* name )
 : QWidget( parent, name ), XS1( XPADDING ), YS1( YPADDING ),
   dXtick1(0.0), dYtick1(0.0), dXtick2(0.0), dYtick2(0.0),
   nmajX1(0), nmajX2(0), nminX1(0), nminX2(0), nmajY1(0), nmajY2(0), nminY1(0), nminY2(0) {

	setLimits( x1, x2, y1, y2 );
	setSecondaryLimits( 0.0, 0.0, 0.0, 0.0 );

	//Set dXS, XS2, dYS, YS2
	XS2 = width() - XS1;
	YS2 = height() - YS2;
	dXS = XS2 - XS1;
	dYS = YS2 - YS1;

	buffer = new QPixmap();

	//default colors:
	setBGColor( QColor( "black" ) );
	setFGColor( QColor( "white" ) );
	setGridColor( QColor( "grey" ) );

	ObjectList.setAutoDelete( TRUE );
}

void PlotWidget::setLimits( double x1, double x2, double y1, double y2 ) {
	if (x2<x1) { XA1=x2; XA2=x1; }
	else { XA1=x1; XA2=x2; }
	if ( y2<y1) { YA1=y2; YA2=y1; }
	else { YA1=y1; YA2=y2; }

	dXA=XA2-XA1; dYA=YA2-YA1;
	DataRect = DRect( XA1, YA1, dXA, dYA );

	updateTickmarks();
}

void PlotWidget::setSecondaryLimits( double x1, double x2, double y1, double y2 ) {
	if (x2<x1) { XB1=x2; XB2=x1; }
	else { XB1=x1; XB2=x2; }
	if ( y2<y1) { YB1=y2; YB2=y1; }
	else { YB1=y1; YB2=y2; }

	dXB=XB2-XB1; dYB=YB2-YB1;

	updateTickmarks();
}

void PlotWidget::updateTickmarks() {
	// Determine the number and spacing of tickmarks for the current plot limits.
	// dX and dY are the interval covered by the limits in X and Y.
	// We then factor this number by a power of ten: dX = tX * 10^[powX], such
	// that tX is between 3 and 30.  The goal is to have int(tX) equal to the
	// number of major tickmarks, each spaced by some distance dtX.
	// So we set dtX=10^[powX] at first.  Then, if tX is too big (greater
	// than 5), we divide tX and multiply dtX by the same integer factor
	// (2,4,5) until we get 3 <= tX <= 5 (and the same for tY, of course).

	if ( x1()==x2() ) {
		kdWarning() << "X range invalid! " << x1() << " to " << x2() << endl;
		XA1=0.0; XA2=1.0;
		return;
	}
	if ( y1()==y2() ) {
		kdWarning() << "Y range invalid! " << y1() << " to " << y2() << endl;
		YA1=0.0; YA2=1.0;
		return;
	}

	int ntry(1);
	if ( dXB > 0.0 ) ntry=2; //secondary limits are defined

	for ( int itry=1; itry<=ntry; itry++ ) {
		int nmajX(0), nmajY(0), nminX(0), nminY(0);
		double mX(0.0), mY(0.0);
		double dX(0.0), dY(0.0), tX(0.0), tY(0.0), dtX(0.0), dtY(0.0);
		//determine size of region to be drawn, in draw units
		if ( itry==1 ) {
			dX = x2() - x1();
			dY = y2() - y1();
		} else {
			dX = xb2() - xb1();
			dY = yb2() - yb1();
		}

		//dX = tX * 10^(powX).  e.g., dX=350 then powX = 2.0, tX=3.5, dtX = 100.0
		double powX(0.0), powY(0.0);
		modf( log10(dX), &powX );
		modf( log10(dY), &powY );
		dtX = pow( 10.0, powX );
		dtY = pow( 10.0, powY );
		tX = dX/dtX;
		tY = dY/dtY;
		if ( tX < 3.0 ) { tX *= 10.0; dtX /= 10.0; } //don't use powX/powY after here
		if ( tY < 3.0 ) { tY *= 10.0; dtY /= 10.0; }
		//tX and tY are now between 3 and 30.

		//make sure tX and tY are between 3 and 5;
		//set nX (number of big ticks) and mX (distance btwn ticks)
		if ( tX < 6.0 ) { //accept current values
			mX = dtX;
			nmajX = int(tX);
			nminX = 5;
		} else if ( tX < 10.0 ) { //factor of 2
			mX = dtX*2.0;
			nmajX = int(tX/2.0);
			nminX = 4;
		}	 else if ( tX < 20.0 ) { //factor of 4
			mX = dtX*4.0;
			nmajX = int(tX/4.0);
			nminX = 4;
		} else { //factor of 5
			mX = dtX*5.0;
			nmajX = int(tX/5.0);
			nminX = 5;
		}

		if ( tY < 6.0 ) { //accept current values
			mY = dtY;
			nmajY = int(tY);
			nminY = 5;
		} else if ( tX < 10.0 ) { //factor of 2
			mY = dtY*2.0;
			nmajY = int(tY/2.0);
			nminY = 4;
		} else if ( tX < 20.0 ) { //factor of 4
			mY = dtY*4.0;
			nmajY = int(tY/4.0);
			nminY = 4;
		} else { //factor of 5
			mY = dtY*5.0;
			nmajY = int(tY/5.0);
			nminY = 5;
		}

		if ( itry==1 ) {
			nmajX1 = nmajX;
			nmajY1 = nmajY;
			nminX1 = nminX;
			nminY1 = nminY;
			dXtick1 = mX;
			dYtick1 = mY;
		} else {
			nmajX2 = nmajX;
			nmajY2 = nmajY;
			nminX2 = nminX;
			nminY2 = nminY;
			dXtick2 = mX;
			dYtick2 = mY;
		}
	} //end for(itry)
}

void PlotWidget::resizeEvent( QResizeEvent *e ) {
	XS2 = width()  - XPADDING;
	YS2 = height() - YPADDING;
	dXS = XS2 - XS1;
	dYS = YS2 - YS1;
	PixRect = QRect( 0, 0, dXS, dYS );

	buffer->resize( width(), height() );
}

void PlotWidget::paintEvent( QPaintEvent *e ) {
	QPainter p;

	p.begin( buffer );
	p.fillRect( 0, 0, width(), height(), bgColor() );

	p.translate( XS1, YS1 );

	drawObjects( &p );
	drawBox( &p );
	p.end();

	bitBlt( this, 0, 0, buffer );
}

void PlotWidget::drawObjects( QPainter *p ) {
	for ( PlotObject *po = ObjectList.first(); po; po = ObjectList.next() ) {
		//draw the plot object
		p->setPen( QColor( po->color() ) );

		switch ( po->type() ) {
			case PlotObject::POINTS :
			{
				p->setBrush( QColor( po->color() ) );

				for ( DPoint *dp = po->points()->first(); dp; dp = po->points()->next() ) {
					QPoint q = dp->qpoint( PixRect, DataRect );
					int x1 = q.x() - po->size()/2;
					int y1 = q.y() - po->size()/2;

					switch( po->param() ) {
						case PlotObject::CIRCLE : p->drawEllipse( x1, y1, po->size(), po->size() ); break;
						case PlotObject::SQUARE : p->drawRect( x1, y1, po->size(), po->size() ); break;
						case PlotObject::LETTER : p->drawText( q, po->name().left(1) ); break;
						default: p->drawPoint( q );
					}
				}

				p->setBrush( Qt::NoBrush );
				break;
			}

			case PlotObject::CURVE :
			{
				p->setPen( QPen( QColor( po->color() ), po->size(), (QPen::PenStyle)po->param() ) );
				DPoint *dp = po->points()->first();
				p->moveTo( dp->qpoint( PixRect, DataRect ) );
				for ( dp = po->points()->next(); dp; dp = po->points()->next() )
					p->lineTo( dp->qpoint( PixRect, DataRect ) );
				break;
			}

			case PlotObject::LABEL :
			{
				QPoint q = po->points()->first()->qpoint( PixRect, DataRect );
				p->drawText( q, po->name() );
				break;
			}
		}
	}
}

double PlotWidget::dmod( double a, double b ) { return ( b * ( ( a / b ) - int( a / b ) ) ); }

void PlotWidget::drawBox( QPainter *p, bool showAxes, bool showTickMarks, bool showTickLabels, bool showGrid ) {
	//First, fill in padding region with bgColor() to mask out-of-bounds plot data
	p->setPen( bgColor() );
	p->setBrush( bgColor() );
	//left padding
	p->drawRect( -XS1, -YS1, XS1, height() );
	//right padding
	p->drawRect( dXS, -YS1, XS1, height() );
	//top padding
	p->drawRect( 0, -YS1, dXS, YS1 );
	//bottom padding
	p->drawRect( 0, dYS, dXS, YS1 );

	if ( showGrid ) {
		//Grid lines are placed at locations of primary axes' major tickmarks

		p->setPen( gridColor() );

		//vertical grid lines
		double x0 = XA1 - dmod( XA1, dXtick1 ); //zeropoint; x(i) is this plus i*dXtick1
		for ( int ix = 1; ix <= nmajX1; ix++ ) {
			int px = int( dXS * ( (x0 + ix*dXtick1)/dXA ) );
			p->drawLine( px, 0, px, dYS );
		}

		//horizontal grid lines
		double y0 = YA1 - dmod( YA1, dYtick1 ); //zeropoint; y(i) is this plus i*mX
		for ( int iy = 1; iy <= nmajY1; iy++ ) {
			int py = int( dYS * ( (y0 + iy*dYtick1)/dYA ) );
			p->drawLine( 0, py, dXS, py );
		}
	}

	p->setPen( fgColor() );
	p->setBrush( Qt::NoBrush );

	if ( showAxes ) p->drawRect( PixRect ); //box outline

	if ( showTickMarks ) {
		//spacing between minor tickmarks (in data units)
		double dminX = dXtick1/nminX1;
		double dminY = dYtick1/nminY1;

		bool secondaryLimits( false );
		if ( dXB > 0.0 ) secondaryLimits = true;

		//--- Draw primary X tickmarks on bottom axis---//
		double x0 = XA1 - dmod( XA1, dXtick1 ); //zeropoint; tickmark i is this plus i*dXtick1 (in data units)
		if ( XA1 < 0 ) x0 -= dXtick1;

		for ( int ix = 0; ix <= nmajX1+1; ix++ ) {
			int px = int( dXS * ( (x0 + ix*dXtick1 - XA1)/dXA ) ); //position of tickmark i (in screen units)
			if ( px > 0 && px < dXS ) {
				p->drawLine( px, dYS - 2, px, dYS - BIGTICKSIZE - 2 ); //move tickmarks 2 pixels (avoids sticking out other side)
				if ( !secondaryLimits ) p->drawLine( px, 0, px, BIGTICKSIZE );
			}

			//tick label
			if ( showTickLabels ) {
				double lab = x0 + ix*dXtick1;
				if ( fabs(lab)/dXtick1 < 0.00001 ) lab = 0.0; //fix occassional roundoff error with "0.0" label
				QString str = QString( "%1" ).arg( lab, 0, 'g', 2 );
				if ( px > 0 && px < dXS ) p->drawText( px - BIGTICKSIZE, dYS + 2*BIGTICKSIZE, str );
			}

			//draw minor ticks
			for ( int j=0; j < nminX1; j++ ) {
				int pmin = int( px + dXS*j*dminX/dXA ); //position of minor tickmark j (in screen units)

				if ( pmin > 0 && pmin < dXS ) {
					p->drawLine( pmin, dYS - 2, pmin, dYS - SMALLTICKSIZE - 2 );
					if ( !secondaryLimits ) p->drawLine( pmin, 0, pmin, SMALLTICKSIZE );
				}
			}
		}

		//--- Draw primary Y tickmarks on left axis ---//
		double y0 = YA1 - dmod( YA1, dYtick1 ); //zeropoint; tickmark i is this plus i*dYtick1 (in data units)

		for ( int iy = 0; iy <= nmajY1; iy++ ) {
			int py = dYS - int( dYS * ( (y0 + iy*dYtick1 - YA1)/dYA ) ); //position of tickmark i (in screen units)
			if ( py > 0 && py < dYS ) {
				p->drawLine( 0, py, BIGTICKSIZE, py );
				if ( !secondaryLimits ) p->drawLine( dXS, py, dXS-BIGTICKSIZE, py );
			}

			//tick label
			if ( showTickLabels ) {
				QString str = QString( "%1" ).arg( (y0 + iy*dYtick1), 0, 'g', 2 );
				if ( py > 0 && py < dYS ) p->drawText( -3*BIGTICKSIZE, py + BIGTICKSIZE, str );
			}

			//minor ticks
			for ( int j=0; j < nminY1; j++ ) {
				int pmin = int( py - dYS*j*dminY/dYA ); //position of minor tickmark j (in screen units)
				if ( pmin > 0 && pmin < dYS ) {
					p->drawLine( 0, pmin, SMALLTICKSIZE, pmin );
					if ( !secondaryLimits ) p->drawLine( dXS, pmin, dXS-SMALLTICKSIZE, pmin );
				}
			}
		}

		if ( secondaryLimits ) {
			double dminX2 = dXtick2/nminX2;
			double dminY2 = dYtick2/nminY2;

			//--- Draw secondary X tickmarks on top axis---//
			double x0 = XB1 - dmod( XB1, dXtick2 ); //zeropoint; tickmark i is this plus i*dXtick2 (in data units)

			for ( int ix = 0; ix <= nmajX2; ix++ ) {
				int px = int( dXS * ( (x0 + ix*dXtick2 - XB1)/dXB ) ); //position of tickmark i (in screen units)
				if ( px > 0 && px < dXS ) p->drawLine( px, 0, px, BIGTICKSIZE );

				//tick label
				if ( showTickLabels ) {
					QString str = QString( "%1" ).arg( (x0 + ix*dXtick2), 0, 'g', 2 );
					if ( px > 0 && px < dXS ) p->drawText( px - BIGTICKSIZE, -2*BIGTICKSIZE, str );
				}

				//draw minor ticks
				for ( int j=0; j < nminX2; j++ ) {
					int pmin = int( px + dXS*j*dminX2/dXB ); //position of minor tickmark j (in screen units)
					if ( pmin > 0 && pmin < dXS ) p->drawLine( pmin, 0, pmin, SMALLTICKSIZE );
				}
			}

			//--- Draw secondary Y tickmarks on right axis ---//
			double y0 = YB1 - dmod( YB1, dYtick2 ); //zeropoint; tickmark i is this plus i*mX (in data units)

			for ( int iy = 0; iy <= nmajY2; iy++ ) {
				int py = dYS - int( dYS * ( (y0 + iy*dYtick2 - YB1)/dYB ) ); //position of tickmark i (in screen units)
				if ( py > 0 && py < dYS ) p->drawLine( dYS, py, dXS-BIGTICKSIZE, py );

				//tick labels
				if ( showTickLabels ) {
					QString str = QString( "%1" ).arg( (y0 + iy*dYtick2), 0, 'g', 2 );
					if ( py > 0 && py < dYS ) p->drawText( dXS + 2*BIGTICKSIZE, py + BIGTICKSIZE, str );
				}

				//minor ticks
				for ( int j=0; j < nminY2; j++ ) {
					int pmin = py - int( dYS*j*dminY2/dYB ); //position of minor tickmark j (in screen units)
					if ( pmin > 0 && pmin < dYS ) p->drawLine( dXS, pmin, dXS-SMALLTICKSIZE, pmin );
				}
			}

		} //end if ( secondaryLimits )

	} //end if ( showTickmarks )
}

#include "plotwidget.moc"
