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

#include <dcmtk/dcmnet/dimse.h>
#include <dcmtk/dcmnet/dfindscu.h>

#include <QtDcmServer.h>
#include <QtDcmMoveScu.h>
#include "QtDcmInterface.h"
#include "QtDcmFifoMover.h"

class QtDcmAPHP : public QtDcmInterface
{
public:
    QtDcmAPHP();//:m_port(-1){};
    ~QtDcmAPHP();

    int sendEcho() override;

    QList<QMap<DcmTagKey, QString>> cFind(const QMap<DcmTagKey, QString> &filters) override;
    
    bool moveRequest(int pi_requestId, const QString &queryLevel, const QString &key) override;

    bool isCachedDataPath(int requestId) override;

public slots:
    void stopMove(int pi_RequestId) override;

    void updateLocalParameters(QString const &aet, QString const &hostname, int port) override;
    void updateRemoteParameters(QString const &aet, QString const &hostname, int port) override;
    void sendPending(int requestId);

signals:
    void receivePending(const int requestId);
private:
    static bool isServerAvailable(const QString &hostName, int port);

    void dcmtkPerformQuery(OFList<OFString>  &keys, DcmFindSCUCallback &cb) const;

private:
    T_ASC_Network *m_net{}; // network struct, contains DICOM upper layer FSM etc.
    T_ASC_Parameters *m_params{}; // parameters of association request
    T_ASC_Association *m_assoc{};

    QMap<int, QtDcmMoveScu*> m_RequestIdMap;
    QTemporaryDir m_TemporaryDir;
    QThread m_Thread;
    QtDcmFifoMover *m_FifoMover;

    enum moveStatus {
        KO = -1,
        OK = 0,
        PENDING = 1,
        };

    QString m_aetitle;
    QString m_hostname;
    int m_port;
    QtDcmServer m_remoteServer;
    int m_remoteNumber;

    void updateQtdcmServerPrefs() const;
};



#endif //QTDCM_QTDCMAPHP_H
