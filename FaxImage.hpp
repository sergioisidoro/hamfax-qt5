// HamFax -- an application for sending and receiving amateur radio facsimiles
// Copyright (C) 2001 Christof Schmitt, DH1CS <cschmit@suse.de>
//  
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//  
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

#ifndef FAXIMAGE_HPP
#define FAXIMAGE_HPP

#include <qobject.h>
#include <qimage.h>
#include <qstring.h>
#include <qpoint.h>

class FaxImage : public QObject {
	Q_OBJECT
	friend class FaxView;
public:
	FaxImage(QObject* parent);
	unsigned int getRows(void);
	unsigned int getCols(void);
	unsigned int getPixelGray(unsigned int col, unsigned int row);
	unsigned int getPixelRed(unsigned int col, unsigned int row);
	unsigned int getPixelGreen(unsigned int col, unsigned int row);
	unsigned int getPixelBlue(unsigned int col, unsigned int row);
private:
	QImage image;
	bool autoScroll;
	QPoint slant1;
	QPoint slant2;
signals:
	void sizeUpdated(unsigned int width, unsigned int height);
	void contentUpdated(int x, int y, int w, int h);
	void scrollTo(int x, int y);
public slots:
        bool setPixel(unsigned int col, unsigned int row,
		      unsigned int value, unsigned int rgbg);
        void create(unsigned int cols, unsigned int rows);
	void load(QString fileName);
	void save(QString fileName);
	void scale(unsigned int width, unsigned int height);
	void resize(unsigned int x, unsigned int y,
		    unsigned int w, unsigned int h);
	void resizeHeight(unsigned int y, unsigned int h);
	void scaleToIOC(unsigned int ioc);
	void scaleToIOC288(void);
	void scaleToIOC576(void);
	void halfWidth(void);
	void doubleWidth(void);
	void rotateLeft(void);
	void rotateRight(void);
	void setAutoScroll(bool b);
	void setSlantPoint(const QPoint& p);
	void correctSlant(void);
};

#endif
