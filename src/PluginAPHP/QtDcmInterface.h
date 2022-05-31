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

    /* ***********************************************************************/
    /* **************************** Echo Request *****************************/
    /* ***********************************************************************/
    virtual int sendEcho() = 0;

    /* ***********************************************************************/
    /* ***************************** Find Request ****************************/
    /* ***********************************************************************/
    virtual QList<QMap<DcmTagKey, QString>> cFind(const QMap<DcmTagKey, QString> &filters) = 0;

    /* ***********************************************************************/
    /* ***************************** Move Request ****************************/
    /* ***********************************************************************/
    virtual bool moveRequest(int pi_requestId, const QString &queryLevel, const QString &key) = 0;

    virtual bool isCachedDataPath(int requestId) = 0;
signals:
    void moveProgress(int requestId, int status);
    void pathToData(int requestId, const QString &path);

public slots:
    virtual void stopMove(int requestId) = 0;

    virtual void updateLocalParameters(QString const &aet, QString const &hostname, int port) = 0;
    virtual void updateRemoteParameters(QString const &aet, QString const &hostname, int port) = 0;

protected:
    T_ASC_Network *m_net{}; // network struct, contains DICOM upper layer FSM etc.
    T_ASC_Parameters *m_params{}; // parameters of association request
    T_ASC_Association *m_assoc{};

};

#endif //QTDCM_QTDCMINTERFACE_H
