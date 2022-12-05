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

#include "QtDcmAPHP.h"

#include <QDebug>
#include <QTcpSocket>

#include <dcmtk/dcmnet/dimse.h>
#include <memory>
#include <PluginAPHP/callbacks/QtDcmCallbacks.h>
#include <QtDcmManager.h>

QtDcmAPHP::QtDcmAPHP():m_port(-1)
{
    m_FifoMover = new QtDcmFifoMover;
    m_FifoMover->moveToThread(&m_Thread);
    connect(&m_Thread, &QThread::finished, m_FifoMover, &QObject::deleteLater);
    connect(&m_Thread, &QThread::started, m_FifoMover, &QtDcmFifoMover::processing);
    connect(m_FifoMover, SIGNAL(sendPending(const int, const int)), this, SIGNAL(moveProgress(int, int, QString)));
    m_Thread.start();
}

QtDcmAPHP::~QtDcmAPHP()
{
    m_FifoMover->stopProcessing();
    m_Thread.quit();
    m_Thread.wait();
    m_RequestIdMap.clear();
}

int QtDcmAPHP::sendEcho()
{

    OFCondition cond = ASC_initializeNetwork ( NET_REQUESTOR, 0, 30 /* timeout */, &m_net );
    if ( cond != EC_Normal ) {
        m_response.code = cond.code();
        m_response.message = "Cannot initialize network : " + QString(cond.text());
        ASC_dropNetwork ( &m_net );
        return m_response.code;
    }

    QTcpSocket socket;
    socket.connectToHost(m_remoteServer.address(), m_remoteServer.port().toInt());
    if (!socket.waitForConnected(1000))
    {
        m_response.code = -1;
        m_response.message = "Cannot connect to server " + m_remoteServer.address() + " on port " + m_remoteServer.port() + " !";

        ASC_dropNetwork(&m_net);
        return m_response.code;
    }

    socket.disconnectFromHost();



    cond = ASC_createAssociationParameters(&m_params, ASC_DEFAULTMAXPDU);
    if (cond.bad())
    {
        m_response.code = cond.code();
        m_response.message = cond.text();
        return m_response.code;
    }
    // set calling and called AE titles
    cond = ASC_setAPTitles(m_params, m_aetitle.toUtf8().data(), m_remoteServer.aetitle().toUtf8().data(), NULL);
    if (cond.bad())
    {
        m_response.code = cond.code();
        m_response.message = cond.text();
        return m_response.code;
    }

    // the DICOM server accepts connections at server.nowhere.com port 104
    cond = ASC_setPresentationAddresses(m_params, m_hostname.toUtf8().data(),
                                                QString(m_remoteServer.address() + ":" + m_remoteServer.port()).toLatin1().data());
    if (cond.bad())
    {
        m_response.code = cond.code();
        m_response.message = cond.text();
        return m_response.code;
    }

    // list of transfer syntaxes, only a single entry here
    const char *ts[] = {UID_LittleEndianImplicitTransferSyntax};

    // add presentation context to association request
    cond = ASC_addPresentationContext(m_params, 1, UID_VerificationSOPClass, ts, 1);
    if (cond.bad())
    {
        m_response.code = cond.code();
        m_response.message = cond.text();
        return m_response.code;
    }

    // request DICOM association
    if (ASC_requestAssociation(m_net, m_params, &m_assoc).good())
    {
        if (ASC_countAcceptedPresentationContexts(m_params) == 1)
        {
            // the remote SCP has accepted the Verification Service Class
            DIC_US id = m_assoc->nextMsgID++; // generate next message ID
            DIC_US status; // DIMSE status of C-ECHO-RSP will be stored here
            DcmDataset *sd = NULL; // status detail will be stored here
            // send C-ECHO-RQ and handle response
            DIMSE_echoUser(m_assoc, id, DIMSE_BLOCKING, 0, &status, &sd);
            delete sd; // we don't care about status detail

            m_response.code = 0;
            m_response.message = "Echo request successful !";
        }
        else
        {
            m_response.code = -1;
            m_response.message = "Wrong presentation context, echo request failed";
        }
    }
    else
    {
        m_response.code = -1;
        m_response.message = "Wrong dicom association, echo request failed";
    }

    ASC_releaseAssociation(m_assoc); // release association
    ASC_destroyAssociation(&m_assoc); // delete assoc structure
    ASC_dropNetwork(&m_net); // delete net structure

    m_net = nullptr;
    m_assoc = nullptr;

    return m_response.code;
}

QList<QMap<DcmTagKey, QString>> QtDcmAPHP::cFind(const QMap<DcmTagKey, QString> &filters)
{
    OFList<OFString> keys;

    for (DcmTagKey key : filters.keys())
    {
        keys.emplace_back(QString(key.toString().c_str() + QString("=") + filters.value(key)).toUtf8().data());
    }

    FindCallback cb(filters);

    dcmtkPerformQuery(keys, cb);

    return cb.m_List;
}

void QtDcmAPHP::dcmtkPerformQuery(std::list<std::string> &keys, DcmFindSCUCallback &cb) const
{
    //Image level
    OFList<OFString> fileNameList;
    DcmFindSCU findscu;

    if (isServerAvailable(m_remoteServer.address(), m_remoteServer.port().toInt()))
    {
        if ( findscu.initializeNetwork ( 30 ).good() )
        {

            QString queryInfoModel = UID_FINDPatientRootQueryRetrieveInformationModel;

            OFCondition cond = findscu.performQuery (m_remoteServer.address().toUtf8().data(),
                                                     m_remoteServer.port().toInt(),
                                                     m_aetitle.toUtf8().data(),
                                                     m_remoteServer.aetitle().toUtf8().data(),
                                                     queryInfoModel.toStdString().c_str() ,
                                                     EXS_Unknown, DIMSE_BLOCKING,
                                                     0, ASC_DEFAULTMAXPDU,
                                                     false, false,
                                                     1, false, -1, &keys,
                                                     &cb, &fileNameList );

            if (cond.good())
            {
                OFCondition cond2 = findscu.dropNetwork();
                if ( cond2.bad())
                {
                    qDebug()<<"Cannot drop network";
                }
            }
            else
            {
                QString message = "Cannot perform query C-FIND : " + QString(cond.text());
                qDebug()<<message;
            }
        }
    }
}

bool QtDcmAPHP::isServerAvailable(const QString &hostName, const int port)
{
    bool bRes = false;
    QTcpSocket socket;
    socket.setSocketOption ( QAbstractSocket::LowDelayOption, 1 );
    socket.connectToHost ( hostName,  port );
    if ( socket.waitForConnected ( 10000 ) ) {
        socket.disconnectFromHost();
        bRes = true;
    }
    return bRes;
}

bool QtDcmAPHP::moveRequest(int pi_requestId, const QString &queryLevel, const QString &key)
{
    bool bRes = false;
    QStringList data(key);
    QString outputDir;

    // WARN : It is a  hack to be able to use QtDcmMoveSCu class without modification
    QtDcmManager::instance()->setCurrentPacs(m_remoteNumber);

    if (m_TemporaryDir.isValid())
    {
        auto mover = new QtDcmMoveScu();
        QDir dir(m_TemporaryDir.path());

        mover->setOutputDir ( dir.path() );
        mover->setData ( data );
        mover->setImportDir ( outputDir );
        mover->setQueryLevel( queryLevel );
//        QObject::connect ( mover, &QtDcmMoveScu::finished,
//                  mover, &QtDcmMoveScu::deleteLater);

        m_RequestIdMap[pi_requestId] = mover;

        QObject::connect( mover, &QtDcmMoveScu::serieMoved, this, [=](const QString &path, const QString &serieUid, int number){
            emit moveProgress(pi_requestId, static_cast<int>(moveStatus::OK), path);
        });

        QObject::connect( mover, &QtDcmMoveScu::moveInProgress, this, [=](const QString &message){
            emit moveProgress(pi_requestId, static_cast<int>(moveStatus::PENDING));
        });

        QObject::connect( mover, &QtDcmMoveScu::moveFailed, this,  [=](const QString &reason){
            emit moveProgress(pi_requestId, static_cast<int>(moveStatus::KO));
        });

        m_FifoMover->addRequest(pi_requestId, mover);
//        emit requestAddedToFifo(pi_requestId, mover);
//        mover->start();
        bRes = true;
    }
    return bRes;
}

void QtDcmAPHP::sendPending(int requestId)
{
    emit moveProgress(requestId, static_cast<int>(moveStatus::PENDING));
}

bool QtDcmAPHP::isCachedDataPath(int requestId)
{
    bool bRes = false;
    auto mover = m_RequestIdMap[requestId];
    QString path = mover->getOutputDir();
    if (mover && (!path.isEmpty()))
    {
        bRes = true;
        // TODO move signal emission ?
        emit moveProgress(requestId, static_cast<int>(moveStatus::OK), path);
    }
    return bRes;
}



void QtDcmAPHP::stopMove(int pi_RequestId)
{
    if (m_RequestIdMap.contains(pi_RequestId))
    {
        auto mover = m_RequestIdMap[pi_RequestId];
        mover->onStopMove();
        m_RequestIdMap.remove(pi_RequestId);
        delete mover;
    }
}

void QtDcmAPHP::updateLocalParameters(const QString &aet, const QString &hostname, int port)
{
    m_aetitle = aet;
    QtDcmPreferences::instance()->setAetitle ( m_aetitle );
    m_hostname = hostname;
    QtDcmPreferences::instance()->setHostname ( m_hostname );
    m_port = port;
    QtDcmPreferences::instance()->setPort ( QString::number(m_port) );
}

void QtDcmAPHP::updateRemoteParameters(const QString &aet, const QString &hostname, int port)
{
    QtDcmPreferences *prefs = QtDcmPreferences::instance();
    bool alreadyRegistered = false;
    for (const auto& ser : prefs->servers())
    {
        if (ser.port()==QString::number(port) &&
        ser.aetitle()==aet &&
        ser.name()==aet &&
        ser.address()==hostname)
        {
            alreadyRegistered = true;
            break;
        }
    }
    if (!alreadyRegistered)
    {
        m_remoteServer.setName(aet);
        m_remoteServer.setAetitle(aet);
        m_remoteServer.setPort(QString::number(port));
        m_remoteServer.setAddress(hostname);
        m_remoteNumber = prefs->servers().size();
        prefs->addServer(m_remoteServer);

    }
    for (const auto &server : prefs->servers())
    {
        qDebug()<<"server : "<<server.aetitle()<<" , "<<server.address()<<" , "<<server.port();
    }
}


