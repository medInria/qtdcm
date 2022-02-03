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

static int requestId = -1;

QtDcmAPHP::~QtDcmAPHP()
{
    delete m_net;
    delete m_assoc;
    delete m_params;
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
    socket.connectToHost(m_serverHostname, m_serverPort);
    if (!socket.waitForConnected(1000))
    {
        m_response.code = -1;
        m_response.message = "Cannot connect to server " + m_serverHostname + " on port " + QString::number(m_serverPort) + " !";

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
    cond = ASC_setAPTitles(m_params, m_aetitle.toUtf8().data(), m_serverAet.toUtf8().data(), NULL);
    if (cond.bad())
    {
        m_response.code = cond.code();
        m_response.message = cond.text();
        return m_response.code;
    }

    // the DICOM server accepts connections at server.nowhere.com port 104
    cond = ASC_setPresentationAddresses(m_params, m_hostname.toUtf8().data(),
                                                QString(m_serverHostname + ":" + QString::number(m_serverPort)).toLatin1().data());
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

QList<QMap<QString, QString>> QtDcmAPHP::findPatientMinimalEntries()
{
    QList<QMap<QString, QString>> entries;

    OFList<OFString> keys;
    keys.emplace_back(( QString ( "QueryRetrieveLevel=" ) + QString ( "" "PATIENT" "" ) ).toUtf8().data() );
    //Patient level
    keys.emplace_back(QString ( "PatientID=").toUtf8().data());
    keys.emplace_back(QString ( "PatientName=").toUtf8().data());

    FindPatientCallback cb;
    QObject::connect(&cb, &FindPatientCallback::patientFinded, [&entries](const QMap<QString, QString> &infos){
        entries.append(infos);
    });

    dcmtkPerformQuery(keys, cb);

    return entries;
}

QList<QMap<QString, QString>>
QtDcmAPHP::findStudyMinimalEntries(const QString &patientID)
{
    QList<QMap<QString, QString>> entries;

    OFList<OFString> keys;
    keys.emplace_back(( QString ( "QueryRetrieveLevel=" ) + QString ( "" "STUDY" "" ) ).toUtf8().data() );

    keys.emplace_back((QString ( "PatientID=" ) + patientID).toUtf8().data() );

    keys.emplace_back(QString ( "StudyDescription").toUtf8().data() );
    keys.emplace_back(QString ( "StudyInstanceUID" ).toUtf8().data() );

    FindStudyCallback cb;
    QObject::connect(&cb, &FindStudyCallback::studyFinded, [&entries](const QMap<QString, QString> &infos){
        entries.append(infos);
    });

    dcmtkPerformQuery(keys, cb);

    return entries;
}

QList<QMap<QString, QString>> QtDcmAPHP::findSeriesMinimalEntries(const QString &studyInstanceUID)
{
    QList<QMap<QString, QString>> entries;

    OFList<OFString> keys;
    keys.emplace_back(( QString ( "QueryRetrieveLevel=" ) + QString ( "" "SERIES" "" ) ).toUtf8().data() );

    keys.emplace_back((QString ( "StudyInstanceUID=" ) + studyInstanceUID).toUtf8().data() );

    keys.emplace_back(QString ( "SeriesDescription").toUtf8().data() );
    keys.emplace_back(QString ( "SeriesInstanceUID").toUtf8().data() );

    FindSeriesCallback cb;
    QObject::connect(&cb, &FindSeriesCallback::seriesFinded, [&entries](const QMap<QString, QString> &infos){
        entries.append(infos);
    });

    dcmtkPerformQuery(keys, cb);

    return entries;
}

void QtDcmAPHP::dcmtkPerformQuery(std::list<std::string> &keys, DcmFindSCUCallback &cb) const
{
    //Image level
    OFList<OFString> fileNameList;
    DcmFindSCU findscu;

    if (isServerAvailable(m_hostname, m_serverPort))
    {
        if ( findscu.initializeNetwork ( 30 ).good() )
        {

            QString queryInfoModel = UID_FINDPatientRootQueryRetrieveInformationModel;

            OFCondition cond = findscu.performQuery (m_hostname.toUtf8().data(),
                                                     m_serverPort,
                                                     m_aetitle.toUtf8().data(),
                                                     m_serverAet.toUtf8().data(),
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

int QtDcmAPHP::moveRequest(const QString &queryLevel, const QString &key)
{
    int intRes = -1;
    QTemporaryDir dir;
    QStringList data(key);
    QString outputDir;
    QtDcmManager::instance()->setCurrentPacs(0);

    if (dir.isValid())
    {
        dir.setAutoRemove(false);
        auto mover = new QtDcmMoveScu ( );
        mover->setOutputDir ( dir.path() );
        mover->setData ( data );
        mover->setImportDir ( outputDir );
        mover->setQueryLevel( queryLevel );
        QObject::connect ( mover, &QtDcmMoveScu::finished,
                  mover, &QtDcmMoveScu::deleteLater);

        requestIdMap[requestId++] = mover;

        QObject::connect( mover, &QtDcmMoveScu::serieMoved, [&](const QString &path, const QString &serie, int number){
            emit moveProgress(requestId, static_cast<int>(moveStatus::OK));
            emit pathToData(requestId, path);
        });

        QObject::connect( mover, &QtDcmMoveScu::moveInProgress, [=](const QString &message){
            emit moveProgress(requestId, static_cast<int>(moveStatus::PENDING));
        });

        QObject::connect( mover, &QtDcmMoveScu::moveFailed, [=](const QString &reason){
            emit moveProgress(requestId, static_cast<int>(moveStatus::KO));
        });

        intRes = requestId;
        mover->start();
    }
    return intRes;
}

void QtDcmAPHP::stopMove(int request)
{
    if (requestIdMap.contains(request))
    {
        auto mover = requestIdMap[request];
        mover->onStopMove();
    }
}


