/***************************************************************************
                          quasar.cpp  -  K Desktop Planetarium
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

#include "kstarsdata.h"
#include "quasar.h"

#include <typeinfo>
#include "kspopupmenu.h"

Quasar::Quasar(const QVariantMap& map)
    : SkyObject(SkyObject::QUASAR)
    , m_ra(0)
    , m_dec(0)
    , m_absMag(21)
    , m_visMag(21)
    , m_bV(0)
    , m_redshift(0)
    , m_initialized(false)
{
    if (!map.contains("quasarID") || !map.contains("RA") || !map.contains("DE")) {
        qWarning() << "Quasar: INVALID quasar!" << map.value("quasarID").toString();
        qWarning() << "Quasar: Please, check your 'quasars.json' catalog!";
        return;
    }

    m_quasarID  = map.value("quasarID").toString();
    m_ra = dms(map.value("RA").toString());
    m_dec = dms(map.value("DE").toString());
    m_absMag = map.value("Amag").toFloat();
    m_visMag = map.value("Vmag").toFloat();
    m_bV = map.value("bV").toFloat();
    m_redshift = map.value("z").toFloat();

    m_initialized = true;
}

Quasar* Quasar::clone() const
{
    Q_ASSERT(typeid(this) == typeid(static_cast<const Quasar*> (this)));
    return new Quasar(*this);
}

void Quasar::initPopupMenu(KSPopupMenu* pmenu)
{
    pmenu->createQuasarMenu(this);
}
