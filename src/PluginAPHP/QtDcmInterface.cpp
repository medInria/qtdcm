
#include  "QtDcmInterface.h"
#include  "QtDcmAPHP.h"

QtDcmInterface* QtDcmInterface::createNewInstance()
{
    return new QtDcmAPHP();
}
