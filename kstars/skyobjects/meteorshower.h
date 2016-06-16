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

    /**
     * @struct Activity
     */
    typedef struct
    {
        int year;                  // The catalog year (0 for generic)
        int zhr;                   // The ZHR on peak
        QList<int> variable;       // The ZHR range when it's variable
        QDate start;               // Initial date of activity
        QDate finish;              // Last date of activity
        QDate peak;                // Peak activity
    } Activity;

    /**
     * Constructor
     */
    explicit MeteorShower();

    /**
     * Destructor
     */
    virtual ~MeteorShower() {}

    /**
     * @return a clone of this object
     * @note See SkyObject::clone()
     */
    virtual MeteorShower* clone() const;

private:
    // data from catalog
    QString m_showerID;                // The ID of the meteor shower
    QString m_designation;             // The designation of the meteor shower
    QList<Activity> m_activities;      // Activity list
    int m_speed;                       // Speed of meteors
    float m_rAlphaPeak;                // radiant's R.A. (peak day)
    float m_rDeltaPeak;                // radiant's Dec. (peak day)
    float m_driftAlpha;                // Drift of R.A. for each day from peak
    float m_driftDelta;                // Drift of Dec. for each day from peak
    QString m_parentObj;               // Parent object for meteor shower
    float m_pidx;                      // The population index
};

#endif
