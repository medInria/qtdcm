#include "QtDcmFifoMover.h"
#include <QObject>

QtDcmFifoMover::QtDcmFifoMover():  QObject()
{
    m_MustRun = false;
}

QtDcmFifoMover::~QtDcmFifoMover()
{
    m_MustRun = false;
}


void QtDcmFifoMover::processing()
{
    m_MustRun = true;
    while(m_MustRun)
    {
        if(m_RequestIdMap.isEmpty())
        {
            sleep(1);
        }
        else
        {
            int key = m_RequestIdMap.firstKey();
            auto mover = m_RequestIdMap.take(key);
            mover->start();

            while(mover->isRunning())
            {
                emit sendPending(key, 1);
                sleep(1);
            }
            mover->deleteLater();
        }
    }
}

void QtDcmFifoMover::addRequest(int requestId, QtDcmMoveScu *mover)
{
    m_RequestIdMap[requestId] = mover;
}

void QtDcmFifoMover::stopProcessing()
{
    m_MustRun = false;
}
