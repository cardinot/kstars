/***************************************************************************
                          meteorshower.h  -  K Desktop Planetarium
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

#ifndef METEOR_SHOWER_H
#define METEOR_SHOWER_H

#include "skyobject.h"

#include <qvarlengtharray.h>

/**
 * @class MeteorShower
 * @author Marcos Cardinot
 */
class MeteorShower : public SkyObject
{

public:
    explicit MeteorShower();

    virtual ~MeteorShower() {}

    virtual MeteorShower* clone() const;

};

#endif
