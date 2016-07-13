/***************************************************************************
                          opsmeteorshowers.cpp  -  K Desktop Planetarium
                             -------------------
    begin                : 01 July 2016
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

#include "opsmeteorshowers.h"
#include "kstars.h"

OpsMeteorShowers::OpsMeteorShowers() : QFrame(KStars::Instance())
{
    setupUi(this);

    // Signals and slots connections
    connect(kcfg_ShowMeteorShowers, SIGNAL(toggled(bool)),
            this, SLOT(slotShowMeteorShowers(bool)));
}

OpsMeteorShowers::~OpsMeteorShowers()
{
}

void OpsMeteorShowers::slotShowMeteorShowers(bool on)
{
    kcfg_ShowMeteorShowers->setChecked(on);
}
