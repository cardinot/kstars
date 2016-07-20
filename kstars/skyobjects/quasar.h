/***************************************************************************
                          quasar.h  -  K Desktop Planetarium
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

#ifndef QUASAR_H
#define QUASAR_H

#include "skyobject.h"

#include <qvarlengtharray.h>

/**
 * @class Quasar
 * @author Marcos Cardinot <mcardinot@gmail.com>
 */
class Quasar : public SkyObject
{

public:
    /**
     * Constructor
     * @param map A QVariantMap containing all the data about a Meteor Shower
     */
    explicit Quasar(const QVariantMap& map);

    /**
     * Destructor
     */
    virtual ~Quasar() {}

    /**
     * @return a clone of this object
     * @note See SkyObject::clone()
     */
    virtual Quasar* clone() const;

private:
    QString m_quasarID;        // the Quasar id
    double m_ra;               // R.A.
    double m_dec;              // Dec.
    float m_absMag;            // absolute magnitude
    float m_visMag;            // visual magnitude
    float m_bV;                // B-V color index
    float m_redshift;          // distance

    bool m_initialized;

    /**
     * @brief initialize the popup menu
     * @param pmenu
     */
    void initPopupMenu(KSPopupMenu* pmenu);
};

#endif
