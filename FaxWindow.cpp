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

#include "FaxWindow.hpp"
#include <qpopupmenu.h>
#include <qfiledialog.h>
#include <qstring.h>
#include <qlayout.h>
#include <qdatetime.h>
#include <qimage.h>
#include <qinputdialog.h>
#include <qmenubar.h>
#include <qmessagebox.h>
#include <qspinbox.h>
#include <qstatusbar.h>
#include <qtooltip.h>
#include <math.h>
#include "Config.hpp"
#include "Error.hpp"
#include "HelpDialog.hpp"
#include "OptionsDialog.hpp"
#include "ReceiveDialog.hpp"
#include "PTT.hpp"

FaxWindow::FaxWindow(const QString& version)
	: QMainWindow(0,0,WDestructiveClose), version(version)
{
	setCaption("HamFax");

	// create child objects
	Config* config=new Config(this);
	setCentralWidget(faxImage=new FaxImage(this));
	faxReceiver=new FaxReceiver(this);
	faxTransmitter=new FaxTransmitter(this,faxImage);
	file=new File(this);
	ptc=new PTC(this);
	sound=new Sound(this);
	PTT* ptt=new PTT(this);
	faxModulator=new FaxModulator(this);
	faxDemodulator=new FaxDemodulator(this);
	transmitDialog=new TransmitDialog(this);
	ReceiveDialog* receiveDialog=new ReceiveDialog(this);
	correctDialog=new CorrectDialog(this);
	OptionsDialog* optionsDialog=new OptionsDialog(this);

	// create items in status bar
	statusBar()->setSizeGripEnabled(false);
	statusBar()->addWidget(sizeText=new QLabel(statusBar()),0,true);
	statusBar()->addWidget(iocText=new QLabel(statusBar()),0,true);
	QToolTip::add(iocText,tr("Index Of Cooperation:\n"
				 "image width in pixels divided by PI"));
	
	// build tool bars
	modTool=new QToolBar(tr("modulation settings"),this);
	new QLabel(tr("carrier"),modTool);
	QSpinBox* carrier=new QSpinBox(1500,2400,100,modTool);
	carrier->setSuffix(tr("Hz"));
	modTool->addSeparator();
	QToolTip::add(carrier,tr("signal carrier for FM and AM"));
	new QLabel(tr("deviation"),modTool);
	QSpinBox* deviation=new QSpinBox(400,500,10,modTool);
	deviation->setSuffix(tr("Hz"));
	QToolTip::add(deviation, tr("deviation for FM"));
	modTool->addSeparator();
	new QLabel(tr("modulation"),modTool);
	modulation=new QComboBox(false,modTool);
	modulation->insertItem(tr("AM"));
	modulation->insertItem(tr("FM"));
	QToolTip::add(modulation,tr("AM is only used for getting images\n"
				    "from weather satellites together with\n"
				    "a FM receiver. FM together with a\n"
				    "USB (upper side band) transceiver is\n"
				    "the right setting for HF"));

	aptTool=new QToolBar(tr("apt settings"),this);
	new QLabel(tr("apt start"),aptTool);
	QSpinBox* aptStartLength=new QSpinBox(0,20,1,aptTool);
	aptStartLength->setSuffix(tr("s"));
	QToolTip::add(aptStartLength,tr("length of the black/white pattern\n"
					"at the beginning of a facsimile"));
	QSpinBox* aptStartFreq=new QSpinBox(300,675,10,aptTool);
	aptStartFreq->setSuffix(tr("Hz"));
	QToolTip::add(aptStartFreq,tr("frequency of the black/white pattern\n"
				      "at the beginning of a facsimile"));
	aptTool->addSeparator();
	new QLabel(tr("apt stop"),aptTool);
	QSpinBox* aptStopLength=new QSpinBox(0,20,1,aptTool);
	QToolTip::add(aptStopLength,tr("length of the black/white pattern\n"
				       "at the end of a facsimile"));
	aptStopLength->setSuffix(tr("s"));
	QSpinBox* aptStopFreq=new QSpinBox(300,675,10,aptTool);
	aptStopFreq->setSuffix(tr("Hz"));
	QToolTip::add(aptStopFreq,tr("frequency of the black/white pattern\n"
				     "at the end of a facsimile"));
	faxTool=new QToolBar(tr("facsimile settings"),this);
	new QLabel(tr("lpm"),faxTool);
	QSpinBox* lpm=new QSpinBox(60,360,10,faxTool);
	QToolTip::add(lpm,tr("lines per minute"));
	faxTool->addSeparator();
	new QLabel(tr("phasing lines"),faxTool);
	QSpinBox* phaseLines=new QSpinBox(0,50,1,faxTool);
	QToolTip::add(phaseLines,tr("phasing lines mark the beginning\n"
				    "of a line and the speed (lpm)"));
	invertPhase=new QComboBox(false,faxTool);
	invertPhase->insertItem(tr("normal"));
	invertPhase->insertItem(tr("inverted"));
	QToolTip::add(invertPhase,tr("normal means 2.5% white, 95% black\n"
				     "and again 2.5% white"));
	faxTool->addSeparator();
	colorBox=new QComboBox(false,faxTool);
	colorBox->insertItem(tr("mono"));
	colorBox->insertItem(tr("color"));
	QToolTip::add(colorBox,
		      tr("In color mode each line\n"
			 "is split in three lines:\n"
			 "red, green and blue."));

	// build menu bar
	QPopupMenu* fileMenu=new QPopupMenu(this);
	fileMenu->insertItem(tr("&Open"),this,SLOT(load()));
	fileMenu->insertItem(tr("&Save"),this,SLOT(save()));
	fileMenu->insertItem(tr("&Quick save as PNG"),this,SLOT(quickSave()));
	fileMenu->insertSeparator();
	fileMenu->insertItem(tr("&Exit"),this,SLOT(close()));
	QPopupMenu* transmitMenu=new QPopupMenu(this);
	transmitMenu->insertItem(tr("Transmit using &dsp"),DSP);
	transmitMenu->insertItem(tr("Transmit to &file"),FILE);
	transmitMenu->insertItem(tr("Transmit using &PTC"),SCSPTC);
	QPopupMenu* receiveMenu=new QPopupMenu(this);
	receiveMenu->insertItem(tr("Receive from d&sp"),DSP);
	receiveMenu->insertItem(tr("Receive from f&ile"),FILE);
	receiveMenu->insertItem(tr("Receive from P&TC"),SCSPTC);
	imageMenu=new QPopupMenu(this);
	imageMenu->insertItem(tr("&Adjust IOC (change width)"),
			      this,SLOT(adjustIOC()));
	imageMenu->insertItem(tr("&Scale to IOC (scale whole image)"),
			      this,SLOT(scaleToIOC()));
	imageMenu->insertSeparator();
	slantID=imageMenu->insertItem(tr("slant correction"),
				      this,SLOT(slantWaitFirst()));
	colDrawID=imageMenu->insertItem(tr("redraw as color facsimile")
					,this,SLOT(redrawColor()));
	monoDrawID=imageMenu->insertItem(tr("redraw as mono facsimile"),
					 this,SLOT(redrawMono()));
	imageMenu->insertSeparator();
	imageMenu->insertItem(tr("shift colors (R->G,G->B,B->R)"),
			      faxImage,SLOT(shiftCol1()));
	imageMenu->insertItem(tr("shift colors (R->B,G->R,B->G)"),
			      faxImage,SLOT(shiftCol2()));
	imageMenu->insertItem(tr("set beginning of line"),
			      this,SLOT(setBegin()));
	optionsMenu=new QPopupMenu(this);
	optionsMenu->insertItem(tr("device settings"),
				optionsDialog,SLOT(doDialog()));
	optionsMenu->insertSeparator();
	pttID=optionsMenu->
		insertItem(tr("key PTT while transmitting with DSP"),
			   this,SLOT(changePTT()));
	scrollID=optionsMenu->
		insertItem(tr("automatic scroll to last received line"),
			   this,SLOT(changeScroll()));
	toolTipID=optionsMenu->
		insertItem(tr("show tool tips"),
			   this,SLOT(changeToolTip()));
	QPopupMenu* helpMenu=new QPopupMenu(this);
	helpMenu->insertItem(tr("&Help"),this,SLOT(help()));
	helpMenu->insertSeparator();
	helpMenu->insertItem(tr("&About HamFax"),this,SLOT(about()));
	helpMenu->insertItem(tr("About &QT"),this,SLOT(aboutQT()));

	menuBar()->insertItem(tr("&File"),fileMenu);
	menuBar()->insertItem(tr("&Transmit"),transmitMenu);
	menuBar()->insertItem(tr("&Receive"),receiveMenu);
	menuBar()->insertItem(tr("&Image"),imageMenu);
	menuBar()->insertItem(tr("&Options"),optionsMenu);
	menuBar()->insertSeparator();
	menuBar()->insertItem(tr("&Help"),helpMenu);

	// how configuration values get to the correct places
	connect(config,SIGNAL(carrier(int)),carrier,SLOT(setValue(int)));
	connect(carrier,SIGNAL(valueChanged(int)),
		config,SLOT(setCarrier(int)));
	connect(config,SIGNAL(carrier(int)),
		faxModulator,SLOT(setCarrier(int)));
	connect(config,SIGNAL(carrier(int)),
		faxDemodulator,SLOT(setCarrier(int)));

	connect(config,SIGNAL(deviation(int)),
		deviation,SLOT(setValue(int)));
	connect(deviation,SIGNAL(valueChanged(int)),
		config,SLOT(setDeviation(int)));
	connect(config,SIGNAL(deviation(int)),
		faxModulator,SLOT(setDeviation(int)));
	connect(config,SIGNAL(deviation(int)),
		faxDemodulator,SLOT(setDeviation(int)));
	connect(config,SIGNAL(deviation(int)),ptc,SLOT(setDeviation(int)));

	connect(config,SIGNAL(useFM(bool)),SLOT(setModulation(bool)));
	connect(modulation,SIGNAL(activated(int)),config,SLOT(setUseFM(int)));
	connect(config,SIGNAL(useFM(bool)),faxModulator,SLOT(setFM(bool)));
	connect(config,SIGNAL(useFM(bool)),faxDemodulator,SLOT(setFM(bool)));
	connect(config,SIGNAL(useFM(bool)),ptc,SLOT(setFM(bool)));

	connect(config,SIGNAL(aptStartLength(int)),
		aptStartLength,SLOT(setValue(int)));
	connect(aptStartLength,SIGNAL(valueChanged(int)),
		config,SLOT(setAptStartLength(int)));
	connect(config,SIGNAL(aptStartLength(int)),
		faxTransmitter,SLOT(setAptStartLength(int)));

	connect(config,SIGNAL(aptStartFreq(int)),
		aptStartFreq,SLOT(setValue(int)));
	connect(aptStartFreq,SIGNAL(valueChanged(int)),
		config,SLOT(setAptStartFreq(int)));
	connect(config,SIGNAL(aptStartFreq(int)),
		faxTransmitter,SLOT(setAptStartFreq(int)));
	connect(config,SIGNAL(aptStartFreq(int)),
		faxReceiver,SLOT(setAptStartFreq(int)));

	connect(config,SIGNAL(aptStopLength(int)),
		aptStopLength,SLOT(setValue(int)));
	connect(aptStopLength,SIGNAL(valueChanged(int)),
		config,SLOT(setAptStopLength(int)));
	connect(config,SIGNAL(aptStopLength(int)),
		faxTransmitter,SLOT(setAptStopLength(int)));

	connect(config,SIGNAL(aptStopFreq(int)),
		aptStopFreq,SLOT(setValue(int)));
	connect(aptStopFreq,SIGNAL(valueChanged(int)),
		config,SLOT(setAptStopFreq(int)));
	connect(config,SIGNAL(aptStopFreq(int)),
		faxTransmitter,SLOT(setAptStopFreq(int)));
	connect(config,SIGNAL(aptStopFreq(int)),
		faxReceiver,SLOT(setAptStopFreq(int)));

	connect(config,SIGNAL(lpm(int)),lpm,SLOT(setValue(int)));
	connect(lpm,SIGNAL(valueChanged(int)),config,SLOT(setLpm(int)));
	connect(config,SIGNAL(lpm(int)),faxTransmitter,SLOT(setLPM(int)));
	connect(config,SIGNAL(lpm(int)),faxReceiver,SLOT(setTxLPM(int)));
	
	connect(config,SIGNAL(phaseLines(int)),
		phaseLines,SLOT(setValue(int)));
	connect(phaseLines,SIGNAL(valueChanged(int)),
		config,SLOT(setPhaseLines(int)));
	connect(config,SIGNAL(phaseLines(int)),
		faxTransmitter,SLOT(setPhasingLines(int)));

	connect(config,SIGNAL(phaseInvert(bool)),SLOT(setPhasingPol(bool)));
	connect(invertPhase,SIGNAL(activated(int)),
		config,SLOT(setPhaseInvert(int)));
	connect(config,SIGNAL(phaseInvert(bool)),
		faxTransmitter,SLOT(setPhasePol(bool)));
	connect(config,SIGNAL(phaseInvert(bool)),
		faxReceiver,SLOT(setPhasePol(bool)));

	connect(config,SIGNAL(color(bool)),SLOT(setColor(bool)));
	connect(colorBox,SIGNAL(activated(int)),config,SLOT(setColor(int)));
	connect(config,SIGNAL(color(bool)),
		faxTransmitter,SLOT(setColor(bool)));
	connect(config,SIGNAL(color(bool)),faxReceiver,SLOT(setColor(bool)));
	connect(this,SIGNAL(color(bool)),config,SLOT(setColor(bool)));

	connect(config,SIGNAL(autoScroll(bool)),
		faxImage,SLOT(setAutoScroll(bool)));
	connect(config,SIGNAL(autoScroll(bool)), SLOT(setAutoScroll(bool)));
	connect(this,SIGNAL(autoScroll(bool)), 
		config, SLOT(setAutoScroll(bool)));

	connect(config,SIGNAL(toolTip(bool)), SLOT(setToolTip(bool)));
	connect(this,SIGNAL(toolTip(bool)), config,SLOT(setToolTip(bool)));

	connect(config,SIGNAL(DSPDevice(const QString&)),
		sound,SLOT(setDSPDevice(const QString&)));
	connect(config,SIGNAL(DSPDevice(const QString&)),
		optionsDialog,SLOT(setDSP(const QString&)));
	connect(optionsDialog,SIGNAL(dsp(const QString&)),
		config,SLOT(setDSP(const QString&)));

	connect(config,SIGNAL(PTCDevice(const QString&)),
		ptc,SLOT(setDeviceName(const QString&)));
	connect(config,SIGNAL(PTCDevice(const QString&)),
		optionsDialog,SLOT(setPTC(const QString&)));
	connect(optionsDialog,SIGNAL(ptc(const QString&)),
		config,SLOT(setPTC(const QString&)));

	connect(config,SIGNAL(PTTDevice(const QString&)),
		ptt,SLOT(setDeviceName(const QString&)));
	connect(config,SIGNAL(PTTDevice(const QString&)),
		optionsDialog,SLOT(setPTT(const QString&)));
	connect(optionsDialog,SIGNAL(ptt(const QString&)),
		config,SLOT(setPTT(const QString&)));

	connect(config,SIGNAL(ptcSpeed(int)),
		optionsDialog,SLOT(setPtcSpeed(int)));
	connect(config,SIGNAL(ptcSpeed(int)),
		ptc,SLOT(setSpeed(int)));
	connect(optionsDialog,SIGNAL(ptcSpeed(int)),
		config,SLOT(setPtcSpeed(int)));
	
	connect(config,SIGNAL(keyPTT(bool)),ptt,SLOT(setUse(bool)));
	connect(config,SIGNAL(keyPTT(bool)),SLOT(setUsePTT(bool)));
	connect(this,SIGNAL(usePTT(bool)),config,SLOT(setKeyPTT(bool)));

	connect(this,SIGNAL(loadFile(QString)),faxImage,SLOT(load(QString)));
	connect(this,SIGNAL(saveFile(QString)),faxImage,SLOT(save(QString)));

	connect(faxImage,SIGNAL(sizeUpdated(int, int)),
		SLOT(newImageSize(int, int)));
	connect(faxImage,SIGNAL(sizeUpdated(int,int)),
		faxReceiver,SLOT(setWidth(int)));
	connect(faxReceiver,SIGNAL(newPixel(int, int, int, int)),
		faxImage,SLOT(setPixel(int, int, int,int)));
	connect(this,SIGNAL(scaleToWidth(int)),
		faxImage,SLOT(scale(int)));
	connect(faxReceiver, SIGNAL(newSize(int, int, int, int)),
		faxImage, SLOT(resize(int, int, int, int)));

	connect(sound,SIGNAL(newSampleRate(int)),
		faxModulator,SLOT(setSampleRate(int)));
	connect(sound,SIGNAL(newSampleRate(int)),
		faxDemodulator,SLOT(setSampleRate(int)));
	connect(sound,SIGNAL(newSampleRate(int)),
		faxReceiver,SLOT(setSampleRate(int)));
	connect(sound,SIGNAL(newSampleRate(int)),
		faxTransmitter,SLOT(setSampleRate(int)));
	
	connect(file,SIGNAL(newSampleRate(int)),
		faxModulator,SLOT(setSampleRate(int)));
	connect(file,SIGNAL(newSampleRate(int)),
		faxDemodulator,SLOT(setSampleRate(int)));
	connect(file,SIGNAL(newSampleRate(int)),
		faxReceiver,SLOT(setSampleRate(int)));
	connect(file,SIGNAL(newSampleRate(int)),
		faxTransmitter,SLOT(setSampleRate(int)));
	
	connect(ptc,SIGNAL(newSampleRate(int)),
		faxReceiver,SLOT(setSampleRate(int)));
	connect(ptc,SIGNAL(newSampleRate(int)),
		faxTransmitter,SLOT(setSampleRate(int)));

	// transmission
	connect(faxTransmitter,SIGNAL(start()),transmitDialog,SLOT(start()));
	connect(sound,SIGNAL(openForWriting()),ptt,SLOT(set()));
	connect(faxTransmitter,SIGNAL(start()),SLOT(disableControls()));
	connect(faxTransmitter,SIGNAL(phasing()),
		transmitDialog,SLOT(phasing()));
	connect(faxTransmitter,SIGNAL(imageLine(int)),
		transmitDialog,SLOT(imageLine(int)));
	connect(faxTransmitter,SIGNAL(aptStop()),
		transmitDialog,SLOT(aptStop()));
	connect(faxTransmitter,SIGNAL(end()),SLOT(endTransmission()));
	connect(sound,SIGNAL(deviceClosed()),transmitDialog,SLOT(hide()));
	connect(ptc,SIGNAL(deviceClosed()),transmitDialog,SLOT(hide()));
	connect(file,SIGNAL(deviceClosed()),transmitDialog,SLOT(hide()));
	connect(file,SIGNAL(deviceClosed()),SLOT(enableControls()));
	connect(sound,SIGNAL(deviceClosed()),SLOT(enableControls()));
	connect(ptc,SIGNAL(deviceClosed()),SLOT(enableControls()));
	connect(sound,SIGNAL(deviceClosed()),ptt,SLOT(release()));
	connect(transmitDialog,SIGNAL(cancelClicked()),
		faxTransmitter,SLOT(doAptStop()));

	// reception
	connect(faxReceiver,SIGNAL(startReception()),
		receiveDialog,SLOT(aptStart()));
	connect(faxReceiver,SIGNAL(startReception()),SLOT(disableControls()));
	connect(faxReceiver,SIGNAL(startReception()),
		receiveDialog,SLOT(show()));
	connect(sound, SIGNAL(data(short*,int)),
		receiveDialog, SLOT(samples(short*,int)));
	connect(faxDemodulator, SIGNAL(data(int*,int)),
		receiveDialog, SLOT(imageData(int*,int)));
	connect(ptc,SIGNAL(data(int*,int)),
		receiveDialog, SLOT(imageData(int*,int)));
	connect(faxReceiver,SIGNAL(aptFound(int)),
		receiveDialog,SLOT(apt(int)));
	connect(faxReceiver,SIGNAL(startingPhasing()),
		receiveDialog,SLOT(phasing()));
	connect(faxReceiver,SIGNAL(phasingLine(double)),
		receiveDialog,SLOT(phasingLine(double)));
	connect(faxReceiver,SIGNAL(imageStarts()),
		receiveDialog,SLOT(disableSkip()));
	connect(faxReceiver,SIGNAL(imageRow(int)),
		receiveDialog,SLOT(imageRow(int)));
	connect(faxReceiver,SIGNAL(end()),SLOT(endReception()));
	connect(faxReceiver,SIGNAL(end()),SLOT(enableControls()));
	connect(faxReceiver,SIGNAL(end()),receiveDialog,SLOT(hide()));
	connect(receiveDialog,SIGNAL(skipClicked()),faxReceiver,SLOT(skip()));
	connect(receiveDialog,SIGNAL(cancelClicked()),
		faxReceiver,SLOT(endReception()));

	// correction
	connect(faxReceiver,SIGNAL(bufferNotEmpty(bool)),
		SLOT(setImageAdjust(bool)));
	connect(faxImage,SIGNAL(newImage()),faxReceiver,SLOT(releaseBuffer()));
	connect(faxReceiver,SIGNAL(redrawStarts()),SLOT(disableControls()));

	connect(this,SIGNAL(correctSlant()),faxImage,SLOT(correctSlant()));
	connect(faxImage,SIGNAL(widthAdjust(double)),
		faxReceiver,SLOT(correctLPM(double)));
	connect(this,SIGNAL(correctBegin()),faxImage,SLOT(correctBegin()));
	connect(correctDialog,SIGNAL(cancelClicked()),SLOT(enableControls()));
	connect(this,SIGNAL(newWidth(int)),
		faxReceiver,SLOT(correctWidth(int)));
	connect(faxReceiver,SIGNAL(imageWidth(int)),
		faxImage,SLOT(setWidth(int)));

	connect(transmitMenu,SIGNAL(activated(int)),SLOT(initTransmit(int)));
	connect(receiveMenu,SIGNAL(activated(int)),SLOT(initReception(int)));
	
	faxImage->create(904,904);
	resize(600,440);
	config->readFile();
}

void FaxWindow::help(void)
{
	HelpDialog* helpDialog=new HelpDialog(this);
	helpDialog->exec();
	delete helpDialog;
}

void FaxWindow::about(void)
{
	QMessageBox::information(this,caption(),
				 tr("HamFax is a QT application for "
				    "transmitting and receiving "
				    "ham radio facsimiles.\n"
				    "Author: Christof Schmitt, DH1CS "
				    "<cschmit@suse.de>\n"
				    "License: GNU General Public License\n"
				    "Version: %1").arg(version));
}

void FaxWindow::aboutQT(void)
{
	QMessageBox::aboutQt(this,caption());
}

void FaxWindow::load(void)
{
	QString fileName=getFileName(tr("load image"),
				     "*."+QImage::inputFormatList().
				     join(" *.").lower());
	if(!fileName.isEmpty()) {
		emit loadFile(fileName);
	}
}

void FaxWindow::save(void)
{
	QString fileName=getFileName(tr("save image"),
				     "*."+QImage::outputFormatList().
				     join(" *.").lower());
	if(!fileName.isEmpty()) {
		emit saveFile(fileName);
	}
}

void FaxWindow::changePTT(void)
{
	bool b = optionsMenu->isItemChecked(pttID) ? false : true;
	optionsMenu->setItemChecked(pttID,b);
	emit usePTT(b);
}

void FaxWindow::setUsePTT(bool b)
{
	optionsMenu->setItemChecked(pttID,b);
}

void FaxWindow::changeScroll(void)
{
	bool b = optionsMenu->isItemChecked(scrollID) ? false : true;
	optionsMenu->setItemChecked(scrollID,b);
	emit autoScroll(b);
}

void FaxWindow::setAutoScroll(bool b)
{
	optionsMenu->setItemChecked(scrollID,b);
}

QString FaxWindow::getFileName(QString caption, QString filter)
{
	QFileDialog* fd=new QFileDialog(this,0,true);
	fd->setSizeGripEnabled(false);
	fd->setCaption(caption);
	fd->setFilter(filter);
	QString s;
	if(fd->exec()) {
		s=fd->selectedFile();
	}
	delete fd;
	return s;
}

void FaxWindow::initTransmit(int item)
{
	try {
		QString fileName;
		switch(interface=item) {
		case FILE:
			fileName=getFileName(tr("output file name"),"*.au");
			if(fileName.isEmpty()) {
				return;
			}
			file->startOutput(fileName);
			connect(file,SIGNAL(next(int)),
				faxTransmitter,SLOT(doNext(int)));
			connect(faxTransmitter,	SIGNAL(data(double*, int)),
				faxModulator, SLOT(modulate(double*, int)));
			connect(faxModulator, SIGNAL(data(short*, int)),
				file, SLOT(write(short*, int)));
			break;
		case DSP:
			sound->startOutput();
			connect(sound,SIGNAL(spaceLeft(int)),
				faxTransmitter,SLOT(doNext(int)));
			connect(faxTransmitter,	SIGNAL(data(double*, int)),
				faxModulator, SLOT(modulate(double*, int)));
			connect(faxModulator, SIGNAL(data(short*, int)),
				sound, SLOT(write(short*, int)));
			break;
		case SCSPTC:
			ptc->startOutput();
			connect(ptc,SIGNAL(spaceLeft(int)),
				faxTransmitter,SLOT(doNext(int)));
			connect(faxTransmitter,SIGNAL(data(double*, int)),
				ptc,SLOT(transmit(double*, int)));
			break;
		}
		faxTransmitter->startTransmission();
	} catch (Error e) {
		QMessageBox::warning(this,tr("error"),e.getText());
	}
}

void FaxWindow::endTransmission(void)
{
	switch(interface) {
	case FILE:
		file->end();
		disconnect(sound,SIGNAL(spaceLeft(int)),
			   faxTransmitter,SLOT(doNext(int)));
		disconnect(faxTransmitter, SIGNAL(data(double*, int)),
			   faxModulator, SLOT(modulate(double*, int)));
		disconnect(faxModulator, SIGNAL(data(short*, int)),
			   sound, SLOT(write(short*, int)));
		break;
	case DSP:
		sound->end();
		disconnect(sound,SIGNAL(spaceLeft(int)),
			   faxTransmitter,SLOT(doNext(int)));
		disconnect(faxTransmitter, SIGNAL(data(double*,int)),
			faxModulator, SLOT(modulate(double*,int)));
		disconnect(faxModulator, SIGNAL(data(short*,int)),
			sound, SLOT(write(short*,int)));
		break;
	case SCSPTC:
		ptc->end();
		disconnect(ptc,SIGNAL(spaceLeft(int)),
			   faxTransmitter,SLOT(doNext(int)));
		disconnect(faxTransmitter, SIGNAL(data(double*, int)),
			   ptc,SLOT(transmit(double*, int)));
	}
}

void FaxWindow::initReception(int item)
{
	try {
		QString fileName;
		switch(interface=item) {
		case FILE:
			fileName=getFileName(tr("input file name"),"*.au");
			if(fileName.isEmpty()) {
				return;
			}
			file->startInput(fileName);
			connect(file, SIGNAL(data(short*,int)),
				faxDemodulator, SLOT(newSamples(short*,int)));
			connect(faxDemodulator, SIGNAL(data(int*,int)),
				faxReceiver, SLOT(decode(int*,int)));
			break;
		case DSP:
			sound->startInput();
			connect(sound,SIGNAL(data(short*,int)),
				faxDemodulator,	SLOT(newSamples(short*,int)));
			connect(faxDemodulator,	SIGNAL(data(int*,int)),
				faxReceiver, SLOT(decode(int*,int)));
			break;
		case SCSPTC:
			ptc->startInput();
			connect(ptc,SIGNAL(data(int*,int)),
				faxReceiver, SLOT(decode(int*, int)));
			break;
		}
		faxReceiver->init();
	} catch(Error e) {
		QMessageBox::warning(this,tr("error"),e.getText());
	}
}

void FaxWindow::endReception(void)
{
	switch(interface) {
	case FILE:
		file->end();
		disconnect(file, SIGNAL(data(short*, int)),
			faxDemodulator, SLOT(newSamples(short*, int)));
		disconnect(faxDemodulator, SIGNAL(data(int*, int)),
			faxReceiver, SLOT(decode(int*, int)));
		break;
	case DSP:
		sound->end();
		disconnect(sound, SIGNAL(data(short*, int)),
			faxDemodulator,	SLOT(newSamples(short*, int)));
		disconnect(faxDemodulator, SIGNAL(data(int*, int)),
			faxReceiver, SLOT(decode(int*, int)));
		break;
	case SCSPTC:
		ptc->end();
		disconnect(ptc,SIGNAL(data(int*, int)),
			   faxReceiver, SLOT(decode(int*, int)));
		break;
	}
}

void FaxWindow::closeEvent(QCloseEvent* close)
{
	switch(QMessageBox::information(this,caption(),
					tr("Really exit?"),
					tr("&Exit"),tr("&Don't Exit"))) {
	case 0:
		close->accept();
		break;
	case 1:
		break;
	}
}

void FaxWindow::quickSave(void)
{
	QDateTime dt=QDateTime::currentDateTime();
	QDate date=dt.date();
	QTime time=dt.time();
	emit saveFile(QString().
		      sprintf("%04d-%02d-%02d-%02d-%02d-%02d.png",
			      date.year(),date.month(),date.day(),
			      time.hour(),time.minute(),time.second()));
}

void FaxWindow::newImageSize(int w, int h)
{
	sizeText->setText(QString(tr("image size: %1x%2")).arg(w).arg(h));
	ioc=static_cast<int>(0.5+w/M_PI);
	iocText->setText(QString(tr("IOC: %1").arg(ioc)));
}

void FaxWindow::setModulation(bool b)
{
	modulation->setCurrentItem(b ? 1 : 0);
}

void FaxWindow::setPhasingPol(bool b)
{
	invertPhase->setCurrentItem(b ? 1 : 0);
}

void FaxWindow::setColor(bool b)
{
	colorBox->setCurrentItem(b ? 1 : 0);
}

void FaxWindow::slantWaitFirst(void)
{
	correctDialog->setText(tr("select first point of vertical line"));
	disableControls();
	correctDialog->show();
	connect(faxImage,SIGNAL(clicked()),this,SLOT(slantWaitSecond()));
}

void FaxWindow::slantWaitSecond(void)
{
	correctDialog->setText(tr("select second point of vertical line"));
	disconnect(faxImage,SIGNAL(clicked()),this,SLOT(slantWaitSecond()));
	connect(faxImage,SIGNAL(clicked()),this,SLOT(slantEnd()));
}

void FaxWindow::slantEnd(void)
{
	correctDialog->hide();
	disconnect(faxImage,SIGNAL(clicked()),this,SLOT(slantEnd()));
	emit correctSlant();
}

void FaxWindow::redrawColor(void)
{
	emit color(true);
	faxReceiver->correctLPM(1);
}

void FaxWindow::redrawMono(void)
{
	emit color(false);
	faxReceiver->correctLPM(1);
}

void FaxWindow::disableControls(void)
{
	menuBar()->setDisabled(true);
	modTool->setDisabled(true);
	aptTool->setDisabled(true);
	faxTool->setDisabled(true);
}

void FaxWindow::enableControls(void)
{
	menuBar()->setDisabled(false);
	modTool->setDisabled(false);
	aptTool->setDisabled(false);
	faxTool->setDisabled(false);
}

void FaxWindow::setImageAdjust(bool b)
{
	imageMenu->setItemEnabled(slantID,b);
	imageMenu->setItemEnabled(colDrawID,b);
	imageMenu->setItemEnabled(monoDrawID,b);
}

void FaxWindow::setBegin(void)
{
	correctDialog->setText(tr("select beginning of line"));
	disableControls();
	correctDialog->show();
	connect(faxImage,SIGNAL(clicked()),this,SLOT(setBeginEnd()));
}

void FaxWindow::setBeginEnd(void)
{
	correctDialog->hide();
	emit correctBegin();
	disconnect(faxImage,SIGNAL(clicked()),this,SLOT(setBeginEnd()));
	enableControls();
}

void FaxWindow::changeToolTip(void)
{
	setToolTip(!optionsMenu->isItemChecked(toolTipID));
}

void FaxWindow::setToolTip(bool b)
{
	optionsMenu->setItemChecked(toolTipID,b);
	QToolTip::setEnabled(b);
	emit toolTip(b);
}

void FaxWindow::adjustIOC(void)
{
	bool ok;
	int iocNew=QInputDialog::getInteger(caption(),tr("Please enter IOC"),
					    ioc, 204, 576, 1, &ok, this );
	if(ok) {
		emit newWidth(M_PI*iocNew);
	}
}

void FaxWindow::scaleToIOC(void)
{
	bool ok;
	int newIOC=QInputDialog::getInteger(caption(),tr("Please enter IOC"),
					    ioc, 204, 576, 1, &ok, this );
	if(ok) {
		emit scaleToWidth(M_PI*newIOC);
	}
}
