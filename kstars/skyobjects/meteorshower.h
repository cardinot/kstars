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
 * @author Marcos Cardinot <mcardinot@gmail.com>
 */
class MeteorShower : public SkyObject
{

public:
    /**
     * @enum Meteor Shower status
     */
    enum Status {
        INVALID,          // not initialized properly
        UNDEFINED,        // it's loaded but 'activity' is undefined
        INACTIVE,         // inactive radiant
        ACTIVE_CONFIRMED, // active radiant - confirmed data
        ACTIVE_GENERIC    // active radiant - generic data
    };

    /**
     * @struct Activity
     */
    typedef struct
    {
        int year;                  // catalog year (0 for generic)
        int zhr;                   // ZHR on peak
        QList<int> variable;       // ZHR range when it's variable
        QDate start;               // initial date of activity
        QDate finish;              // last date of activity
        QDate peak;                // peak day
    } Activity;

    /**
     * Constructor
     * @param map A QVariantMap containing all the data about a Meteor Shower
     */
    explicit MeteorShower(const QVariantMap& map);

    /**
     * Destructor
     */
    virtual ~MeteorShower() {}

    /**
     * @return a clone of this object
     * @note See SkyObject::clone()
     */
    virtual MeteorShower* clone() const;

    /**
     * @return current meteor shower status
     */
    Status getStatus() { return m_status; }

private:
    Status m_status;                // meteor shower status

    // data from catalog
    QString m_showerID;             // meteor shower ID
    QString m_designation;          // meteor shower designation
    QList<Activity> m_activities;   // Activity list
    int m_speed;                    // speed of meteors
    dms m_radiantAlpha;             // radiant's R.A. (peak day)
    dms m_radiantDelta;             // radiant's Dec. (peak day)
    dms m_driftAlpha;               // drift of R.A. for each day from peak
    dms m_driftDelta;               // drift of Dec. for each day from peak
    QString m_parentObj;            // parent object for meteor shower
    float m_pidx;                   // the population index
};

#endif
