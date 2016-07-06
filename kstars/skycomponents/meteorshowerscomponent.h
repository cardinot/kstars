/***************************************************************************
                          meteorshowerscomponent.h  -  K Desktop Planetarium
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

#ifndef METEOR_SHOWERS_COMPONENT_H
#define METEOR_SHOWERS_COMPONENT_H

#include "listcomponent.h"
#include "skyobjects/meteorshower.h"

#include <QList>

#define MS_CATALOG_VERSION 1

class MeteorShowers;

/**
 * @class MeteorShowersComponent
 * @author Marcos Cardinot <mcardinot@gmail.com>
 */
class MeteorShowersComponent : public QObject, public ListComponent
{
    Q_OBJECT

public:
    explicit MeteorShowersComponent(SkyComposite* parent);

    virtual ~MeteorShowersComponent();

    virtual bool selected();

    virtual void update(KSNumbers*);

    virtual void draw(SkyPainter* skyp);

    void loadData(const QString& jsonPath);
};

#endif

