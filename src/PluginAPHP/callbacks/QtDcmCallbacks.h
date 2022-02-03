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

#pragma once

#include <dcmtk/dcmnet/dfindscu.h>
#include <QObject>

class FindPatientCallback : public QObject, public DcmFindSCUCallback
{
    Q_OBJECT
public:
    void callback ( T_DIMSE_C_FindRQ *request, int responseCount, T_DIMSE_C_FindRSP *rsp, DcmDataset *responseIdentifiers ) override;

signals:
    void patientFinded(const QMap<QString, QString>&);
};

class FindStudyCallback : public QObject, public DcmFindSCUCallback
{
    Q_OBJECT
public:
    void callback ( T_DIMSE_C_FindRQ *request, int responseCount, T_DIMSE_C_FindRSP *rsp, DcmDataset *responseIdentifiers ) override;

signals:
    void studyFinded(const QMap<QString, QString>&);
};

class FindSeriesCallback : public QObject, public DcmFindSCUCallback
{
    Q_OBJECT
public:
    void callback ( T_DIMSE_C_FindRQ *request, int responseCount, T_DIMSE_C_FindRSP *rsp, DcmDataset *responseIdentifiers ) override;

signals:
    void seriesFinded(const QMap<QString, QString>&);
};
