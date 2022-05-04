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
#include <dcmtk/dcmnet/dfindscu.h>

#include <memory>
#include <PluginAPHP/callbacks/QtDcmCallbacks.h>
#include <QtDcmManager.h>
#include <QtDcmMoveScu.h>

class QtDcmAPHPPrivate
{
public:
	T_ASC_Network *net; // network struct, contains DICOM upper layer FSM etc.
	T_ASC_Parameters *params; // parameters of association request
	T_ASC_Association *assoc;
	QMap<int, QtDcmMoveScu*> requestIdMap;

};

QtDcmAPHP::QtDcmAPHP() : m_port(-1)
{
	d = new QtDcmAPHPPrivate;
	d->net = nullptr; // network struct, contains DICOM upper layer FSM etc.
	d->params = nullptr; // parameters of association request
	d->assoc = nullptr;
}

QtDcmAPHP::~QtDcmAPHP()
{
	delete d;
}

int QtDcmAPHP::sendEcho()
{

    OFCondition cond = ASC_initializeNetwork ( NET_REQUESTOR, 0, 30 /* timeout */, &d->net );
    if ( cond != EC_Normal ) {
        m_response.code = cond.code();
        m_response.message = "Cannot initialize network : " + QString(cond.text());
        ASC_dropNetwork ( &d->net );
        return m_response.code;
    }

    QTcpSocket socket;
    socket.connectToHost(m_remoteServer.address(), m_remoteServer.port().toInt());
    if (!socket.waitForConnected(1000))
    {
        m_response.code = -1;
        m_response.message = "Cannot connect to server " + m_remoteServer.address() + " on port " + m_remoteServer.port() + " !";

        ASC_dropNetwork(&d->net);
        return m_response.code;
    }

    socket.disconnectFromHost();



    cond = ASC_createAssociationParameters(&d->params, ASC_DEFAULTMAXPDU);
    if (cond.bad())
    {
        m_response.code = cond.code();
        m_response.message = cond.text();
        return m_response.code;
    }
    // set calling and called AE titles
    cond = ASC_setAPTitles(d->params, m_aetitle.toUtf8().data(), m_remoteServer.aetitle().toUtf8().data(), NULL);
    if (cond.bad())
    {
        m_response.code = cond.code();
        m_response.message = cond.text();
        return m_response.code;
    }

    // the DICOM server accepts connections at server.nowhere.com port 104
    cond = ASC_setPresentationAddresses(d->params, m_hostname.toUtf8().data(),
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
    cond = ASC_addPresentationContext(d->params, 1, UID_VerificationSOPClass, ts, 1);
    if (cond.bad())
    {
        m_response.code = cond.code();
        m_response.message = cond.text();
        return m_response.code;
    }

    // request DICOM association
    if (ASC_requestAssociation(d->net, d->params, &d->assoc).good())
    {
        if (ASC_countAcceptedPresentationContexts(d->params) == 1)
        {
            // the remote SCP has accepted the Verification Service Class
            DIC_US id = d->assoc->nextMsgID++; // generate next message ID
            DIC_US status; // DIMSE status of C-ECHO-RSP will be stored here
            DcmDataset *sd = NULL; // status detail will be stored here
            // send C-ECHO-RQ and handle response
            DIMSE_echoUser(d->assoc, id, DIMSE_BLOCKING, 0, &status, &sd);
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

    ASC_releaseAssociation(d->assoc); // release association
    ASC_destroyAssociation(&d->assoc); // delete assoc structure
    ASC_dropNetwork(&d->net); // delete net structure

    d->net = nullptr;
    d->assoc = nullptr;

    return m_response.code;
}

QList<QMap<QString, QString>> QtDcmAPHP::findPatientMinimalEntries(const QMap<QString, QString> &filters)
{
    OFList<OFString> keys;
    keys.emplace_back(( QString ( "QueryRetrieveLevel=" ) + QString ( "" "PATIENT" "" ) ).toUtf8().data() );
    //Patient level
    if (filters.contains("PatientID"))
    {
        keys.emplace_back((QString ( "PatientID=" ) + filters["PatientID"]).toUtf8().data() );
    }
    else
    {
        keys.emplace_back(QString ( "PatientID=").toUtf8().data());
    }
    if (filters.contains("PatientName"))
    {
        keys.emplace_back((QString ( "PatientName=" ) + filters["PatientName"]).toUtf8().data() );
    }
    else
    {
        keys.emplace_back(QString ( "PatientName=").toUtf8().data());
    }
    if (filters.contains("PatientSex"))
    {
        keys.emplace_back((QString ( "PatientSex=" ) + filters["PatientSex"]).toUtf8().data() );
    }
    else
    {
        keys.emplace_back(QString ( "PatientSex=").toUtf8().data());
    }

    return dcmtkPerformQuery(keys, 0);
}

QList<QMap<QString, QString>>
QtDcmAPHP::findStudyMinimalEntries(const QString &patientID, const QMap<QString, QString> &filters)
{
    OFList<OFString> keys;
    keys.emplace_back(( QString ( "QueryRetrieveLevel=" ) + QString ( "" "STUDY" "" ) ).toUtf8().data() );

    keys.emplace_back((QString ( "PatientID=" ) + patientID).toUtf8().data() );

    if (filters.contains("StudyDescription"))
    {
        keys.emplace_back((QString ( "StudyDescription=" ) + filters["StudyDescription"]).toUtf8().data() );
    }
    else
    {
        keys.emplace_back(QString ( "StudyDescription=").toUtf8().data());
    }
    if (filters.contains("StudyDate"))
    {
        keys.emplace_back((QString ( "StudyDate=" ) + filters["StudyDate"]).toUtf8().data() );
    }
    else
    {
        keys.emplace_back(QString ( "StudyDate=").toUtf8().data());
    }
    keys.emplace_back(QString ( "StudyInstanceUID" ).toUtf8().data() );

    return dcmtkPerformQuery(keys, 1);
}

QList<QMap<QString, QString>> QtDcmAPHP::findSeriesMinimalEntries(const QString &studyInstanceUID, const QMap<QString, QString> &filters)
{
    OFList<OFString> keys;
    keys.emplace_back(( QString ( "QueryRetrieveLevel=" ) + QString ( "" "SERIES" "" ) ).toUtf8().data() );

    keys.emplace_back((QString ( "StudyInstanceUID=" ) + studyInstanceUID).toUtf8().data() );

    if (filters.contains("SeriesDescription"))
    {
        keys.emplace_back((QString ( "SeriesDescription=" ) + filters["SeriesDescription"]).toUtf8().data() );
    }
    else
    {
        keys.emplace_back(QString ( "SeriesDescription=").toUtf8().data());
    }
    if (filters.contains("Modality"))
    {
        keys.emplace_back((QString ( "Modality=" ) + filters["Modality"]).toUtf8().data() );
    }
    else
    {
        keys.emplace_back(QString ( "Modality=").toUtf8().data());
    }
    keys.emplace_back(QString ( "SeriesInstanceUID=").toUtf8().data() );

    return dcmtkPerformQuery(keys, 2);
}

QList<QMap<QString, QString>> QtDcmAPHP::dcmtkPerformQuery(std::list<std::string> &keys, int level ) const
{
	QList<QMap<QString, QString>> res;

    //Image level
    OFList<OFString> fileNameList;
    DcmFindSCU findscu;
	DcmFindSCUCallback *cb = nullptr;
	switch (level)
	{
	case 0: cb = new FindPatientCallback; break;
	case 1: cb = new FindStudyCallback; break;
	case 2: cb = new FindSeriesCallback; break;
	default: break;
	}

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
                                                     cb, &fileNameList );

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
	switch (level)
	{
	case 0: res = static_cast<FindPatientCallback*>(cb)->m_patientsList; break;
	case 1: res = static_cast<FindStudyCallback  *>(cb)->m_studiesList; break;
	case 2: res = static_cast<FindSeriesCallback *>(cb)->m_seriesList; break;
	default: break;
	}

	return res;
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
        auto mover = new QtDcmMoveScu ( );
        QDir dir(m_TemporaryDir.path());

        mover->setOutputDir ( dir.path() );
        mover->setData ( data );
        mover->setImportDir ( outputDir );
        mover->setQueryLevel( queryLevel );
        QObject::connect ( mover, &QtDcmMoveScu::finished,
                  mover, &QtDcmMoveScu::deleteLater);

        d->requestIdMap[pi_requestId] = mover;

        QObject::connect( mover, &QtDcmMoveScu::serieMoved, [=](const QString &path, const QString &serieUid, int number){
            emit moveProgress(pi_requestId, static_cast<int>(moveStatus::OK));
            emit pathToData(pi_requestId, path);
        });

        QObject::connect( mover, &QtDcmMoveScu::moveInProgress, [=](const QString &message){
            emit moveProgress(pi_requestId, static_cast<int>(moveStatus::PENDING));
        });

        QObject::connect( mover, &QtDcmMoveScu::moveFailed, [=](const QString &reason){
            emit moveProgress(pi_requestId, static_cast<int>(moveStatus::KO));
        });

        mover->start();
        bRes = true;
    }
    return bRes;
}

bool QtDcmAPHP::isCachedDataPath(int requestId)
{
    bool bRes = false;
    auto mover = d->requestIdMap[requestId];
    QString path = mover->getOutputDir();
    if (mover && (!path.isEmpty()))
    {
        bRes = true;
        // TODO move signal emission ?
        emit pathToData(requestId, path);
    }
    return bRes;
}



void QtDcmAPHP::stopMove(int pi_RequestId)
{
    if (d->requestIdMap.contains(pi_RequestId))
    {
        auto mover = d->requestIdMap[pi_RequestId];
        mover->onStopMove();
		d->requestIdMap.remove(pi_RequestId);
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

