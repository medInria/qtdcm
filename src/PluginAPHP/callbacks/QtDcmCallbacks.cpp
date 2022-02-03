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

#include <QDebug>
#include "QtDcmCallbacks.h"

#include <dcmtk/dcmdata/dcdeftag.h>

void FindPatientCallback::callback (T_DIMSE_C_FindRQ *request, int responseCount, T_DIMSE_C_FindRSP *rsp, DcmDataset *responseIdentifiers )
{
    Q_UNUSED(request)
    Q_UNUSED(responseCount)
    Q_UNUSED(rsp)

    QMap<QString, QString> infosMap;

    OFString info;

    responseIdentifiers->findAndGetOFString ( DCM_PatientName, info );
    infosMap.insert ( "Name", QString ( info.c_str() ) );
    responseIdentifiers->findAndGetOFString ( DCM_PatientID, info );
    infosMap.insert ( "ID", QString ( info.c_str() ) );

    emit patientFinded(infosMap);
}

void FindStudyCallback::callback (T_DIMSE_C_FindRQ *request, int responseCount, T_DIMSE_C_FindRSP *rsp, DcmDataset *responseIdentifiers )
{
    Q_UNUSED(request)
    Q_UNUSED(responseCount)
    Q_UNUSED(rsp)

    QMap<QString, QString> infosMap;

    OFString info;

    responseIdentifiers->findAndGetOFString ( DCM_StudyDescription, info );
    infosMap.insert ( "Description", QString ( info.c_str() ) );
    responseIdentifiers->findAndGetOFString ( DCM_StudyInstanceUID, info );
    infosMap.insert ( "StudyInstanceUID", QString ( info.c_str() ) );

    emit studyFinded(infosMap);
}

void FindSeriesCallback::callback(T_DIMSE_C_FindRQ *request, int responseCount, T_DIMSE_C_FindRSP *rsp,
                                  DcmDataset *responseIdentifiers)
{
    Q_UNUSED(request)
    Q_UNUSED(responseCount)
    Q_UNUSED(rsp)

    QMap<QString, QString> infosMap;

    OFString info;

    responseIdentifiers->findAndGetOFString ( DCM_SeriesDescription, info );
    infosMap.insert ( "Description", QString ( info.c_str() ) );
    responseIdentifiers->findAndGetOFString ( DCM_SeriesInstanceUID, info );
    infosMap.insert ( "SeriesInstanceUID", QString ( info.c_str() ) );

    emit seriesFinded(infosMap);

}
