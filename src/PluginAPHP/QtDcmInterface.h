//
// Created by Julien Castelneau on 11/10/2021.
//

#ifndef QTDCM_QTDCMINTERFACE_H
#define QTDCM_QTDCMINTERFACE_H

#include <QObject>
#include <QString>
#include <dcmtk/dcmdata/dctagkey.h>

#include <qtdcmExports.h>

class QTDCM_EXPORT QtDcmInterface : public QObject
{
    Q_OBJECT
public:
    static QtDcmInterface* createNewInstance();
    virtual ~QtDcmInterface() = default;

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
    void moveProgress(int requestId, int status, QString path = "");
//    void requestAddedToFifo(int requestId, QtDcmMoveScu *mover);

public slots:
    virtual void stopMove(int requestId) = 0;

    virtual void updateLocalParameters(QString const &aet, QString const &hostname, int port) = 0;
    virtual void updateRemoteParameters(QString const &aet, QString const &hostname, int port) = 0;

protected:

};

#endif //QTDCM_QTDCMINTERFACE_H
