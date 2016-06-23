/***************************************************************************
                          meteorshowerscomponent.cpp  -  K Desktop Planetarium
                             -------------------
    begin                : 01 June 2016
    copyright            : (C) 2016 by Marcos Cardinot
    email                : mcardinot@gmail.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "meteorshowerscomponent.h"
#include "skypainter.h"

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>

MeteorShowersComponent::MeteorShowersComponent(SkyComposite* parent)
    : ListComponent(parent)
{
    // Loads the JSON catalog
    QString path = QStandardPaths::locate(QStandardPaths::DataLocation, QString("showers.json"));
    loadData(path);
}

MeteorShowersComponent::~MeteorShowersComponent()
{
}

void MeteorShowersComponent::loadData(const QString& jsonPath)
{
    qDebug() << "Meteor Showers: Loading data!" << QDir::toNativeSeparators(jsonPath);

    m_meteorShowers.clear();
    objectNames(SkyObject::METEOR_SHOWER).clear();

    QFile jsonFile(jsonPath);
    if (!jsonFile.exists())
    {
        qWarning() << "Meteor Showers: Catalog file does not exist!";
        return;
    }

    if (!jsonFile.open(QIODevice::ReadOnly))
    {
        qWarning() << "Meteor Showers: Cannot to open the catalog file!";
        return;
    }

    QJsonObject json(QJsonDocument::fromJson(jsonFile.readAll()).object());
    jsonFile.close();

    if (json["shortName"].toString() != "meteor showers data"
            || json["version"].toInt() != MS_CATALOG_VERSION)
    {
        qWarning()  << "Meteor Showers: The current catalog is not compatible!";
        return;
    }

    QVariantMap map = json["showers"].toObject().toVariantMap();
    foreach (QString msKey, map.keys())
    {
        QVariantMap msData = map.value(msKey).toMap();
        msData["showerID"] = msKey;

        MeteorShower* ms = new MeteorShower(msData);
        if (ms->getStatus() != MeteorShower::INVALID)
        {
            m_meteorShowers.append(ms);
            objectNames(SkyObject::METEOR_SHOWER).append(ms->name());
        }
    }
}

void MeteorShowersComponent::update(KSNumbers* num)
{
}

bool MeteorShowersComponent::selected()
{
    return false;
}

SkyObject* MeteorShowersComponent::findByName(const QString& name)
{
    return NULL;
}

SkyObject* MeteorShowersComponent::objectNearest(SkyPoint* p, double& maxrad)
{
    return NULL;
}

void MeteorShowersComponent::draw(SkyPainter *skyp)
{
}
