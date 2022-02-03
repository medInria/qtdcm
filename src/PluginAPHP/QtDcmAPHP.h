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


#ifndef QTDCM_QTDCMAPHP_H
#define QTDCM_QTDCMAPHP_H

#include <QtDcmMoveScu.h>
#include "QtDcmInterface.h"

class QtDcmAPHP : public QtDcmInterface
{
public:
//    explicit QtDcmAPHP(){};
    ~QtDcmAPHP() override;

    int sendEcho() override;

    QList<QMap<QString, QString>> findPatientMinimalEntries() override;

    QList<QMap<QString, QString>> findStudyMinimalEntries(const QString &patientID) override;

    QList<QMap<QString, QString>> findSeriesMinimalEntries(const QString &key) override;

    int moveRequest(const QString &queryLevel, const QString &key) override;

public slots:
    void stopMove(int request) override;

private:
    static bool isServerAvailable(const QString &hostName, int port);

    void dcmtkPerformQuery(std::list<std::string> &keys, DcmFindSCUCallback &cb) const;

    QMap<int, QtDcmMoveScu*> requestIdMap;

    enum moveStatus {
        KO = -1,
        OK = 0,
        PENDING = 1,
        };

};



#endif //QTDCM_QTDCMAPHP_H
