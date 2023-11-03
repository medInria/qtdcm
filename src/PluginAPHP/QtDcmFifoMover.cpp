#include "QtDcmFifoMover.h"
#include <QObject>
#include <thread>

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
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        else
        {
            int key = m_RequestIdMap.firstKey();
            auto mover = m_RequestIdMap.take(key);
            mover->start();

            while(mover->isRunning() && m_MustRun)
            {
                for (auto other : m_RequestIdMap.keys())
                {
                    emit sendPending(other, 1);
                }
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
            if (mover->isRunning())
            {
                mover->quit();
                mover->wait();
            }
            mover->deleteLater();
        }
    }
    for (auto mover : m_RequestIdMap)
    {
        mover->quit();
        mover->wait();
        mover->deleteLater();
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
