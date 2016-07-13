/***************************************************************************
                          opsmeteorshowers.h  -  K Desktop Planetarium
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

#ifndef OPSMETEORSHOWERS_H_
#define OPSMETEORSHOWERS_H_

#include "ui_opsmeteorshowers.h"

#include <kconfigdialog.h>

class KStars;

/**
 * @class OpsMeteorShowers
 * The Meteor Showers Tab of the Options window.
 * @author Marcos Cardinot <mcardinot@gmail.com>
 */
class OpsMeteorShowers : public QFrame, public Ui::OpsMeteorShowers
{
    Q_OBJECT

public:
    /**
     * Constructor
     */
    explicit OpsMeteorShowers();

    /**
     * Destructor
     */
    ~OpsMeteorShowers();

private slots:
    void slotShowMeteorShowers(bool on);
};

#endif  //OPSMETEORSHOWERS_H_
