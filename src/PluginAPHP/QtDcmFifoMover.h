/*
    QtDcm is a C++ Qt based library for communication and conversion of Dicom images.
    Copyright (C) 2011  Alexandre Abadie <Alexandre.Abadie@univ-rennes1.fr>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef MEDINRIA_QTDCMFIFOMOVER_H
#define MEDINRIA_QTDCMFIFOMOVER_H

#include <QtDcmMoveScu.h>
#include <QMap>
#include <QObject>

class QtDcmAPHP;

class QtDcmFifoMover : public QObject
{
    Q_OBJECT
public:

    QtDcmFifoMover();
    ~QtDcmFifoMover() override;

public slots:
    void processing();
    void addRequest(int requestId, QtDcmMoveScu *mover);
    void stopProcessing();

signals:
    void sendPending(const int requestId, const int status);

private:
    QMap<int, QtDcmMoveScu*> m_RequestIdMap;
    bool m_MustRun;

};


#endif //MEDINRIA_QTDCMFIFOMOVER_H
