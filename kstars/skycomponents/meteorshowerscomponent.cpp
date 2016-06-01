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

MeteorShowersComponent::MeteorShowersComponent(SkyComposite* parent)
    : ListComponent(parent)
{
}

MeteorShowersComponent::~MeteorShowersComponent()
{
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
