//
// Created by Julien Castelneau on 11/10/2021.
//

#ifndef QTDCM_QTDCMINTERFACE_H
#define QTDCM_QTDCMINTERFACE_H


#include <QString>
#include <QObject>
#include <dcmtk/dcmnet/dimse.h>
#include <dcmtk/dcmnet/dfindscu.h>
#include <QtDcmPreferences.h>

class QtDcmInterface : public QObject
{
    Q_OBJECT
public:

    struct response
    {
        QString message; // OFConsition.text() in most of the cases
        int code; // OFCondition.code() in most of the cases
    } m_response;

    virtual void setConnectionParameters(const QString &aetitle, const QString &hostname, const int port, const QString &serverAet,
                                 const QString &serverHostName, const int serverPort) {
        m_aetitle = "MEDINRIA";//aetitle;
        m_hostname = "localhost";//hostname;
        m_port = 2010;//port;
        m_serverAet = "SERVER";//serverAet;
        m_serverHostname = "localhost";//serverHostName;
        m_serverPort = 11112;//serverPort;

        QtDcmPreferences::instance()->setAetitle ( "MEDINRIA" );
        QtDcmPreferences::instance()->setPort ( QString::number(2010) );
        QtDcmPreferences::instance()->setHostname ( "localhost" );

        QtDcmPreferences* prefs = QtDcmPreferences::instance();
        QList<QtDcmServer> servers;
        QtDcmServer server;
        server.setName("SPHERE");
        server.setAetitle("SERVER");
        server.setPort(QString::number(11112));
        server.setAddress("localhost");
        servers << server;
        prefs->setServers(servers);
    }
    /* ***********************************************************************/
    /* **************************** Echo Request *****************************/
    /* ***********************************************************************/
    virtual int sendEcho() = 0;

    /* ***********************************************************************/
    /* ***************************** Find Request ****************************/
    /* ***********************************************************************/
    virtual QList<QMap<QString, QString>> findPatientMinimalEntries() = 0;

    virtual QList<QMap<QString, QString>> findStudyMinimalEntries(const QString &key) = 0;

    virtual QList<QMap<QString, QString>> findSeriesMinimalEntries(const QString &key) = 0;

    /* ***********************************************************************/
    /* ***************************** Move Request ****************************/
    /* ***********************************************************************/
    virtual bool moveRequest(int pi_requestId, const QString &queryLevel, const QString &key) = 0;

signals:
    void moveProgress(int requestId, int status);
    void pathToData(int requestId, const QString &path);

public slots:
    virtual void stopMove(int requestId) = 0;

protected:
    T_ASC_Network *m_net{}; // network struct, contains DICOM upper layer FSM etc.
    T_ASC_Parameters *m_params{}; // parameters of association request
    T_ASC_Association *m_assoc{};

    QString m_aetitle;
    QString m_hostname;
    int m_port;
    QString m_serverAet;
    QString m_serverHostname;
    int m_serverPort;
};

#endif //QTDCM_QTDCMINTERFACE_H
