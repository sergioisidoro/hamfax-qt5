#include "DisplayLevel.hpp"
#include <qpainter.h>

DisplayLevel::DisplayLevel(QWidget* parent)
	: QFrame(parent), w(0)
{
	setMinimumHeight(20);
	setFrameStyle(QFrame::Panel|QFrame::Sunken);
	setMargin(2);
	m=margin();
}

void DisplayLevel::paintEvent(QPaintEvent* e)
{
	QFrame::paintEvent(e);
	QPainter paint;
	paint.begin(this);
	paint.setPen(Qt::blue);
	paint.setBrush(Qt::blue);
	paint.drawRect(m,m,w,height()-2*margin());
	paint.end();
}

void DisplayLevel::setZero(void)
{
	w=0;
}

void DisplayLevel::samples(short* buffer, int n)
{
	short min=32767;
	short max=-32768;
	for(int i=0; i<n; i++) {
		short s=buffer[i];
		if(s>max) {
			max=s;
		}
		if(s<min) {
			min=s;
		}
	}
	double level=(max-min)/65536.0;
	w=static_cast<int>(level*(width()-2*m));
	update();
}
