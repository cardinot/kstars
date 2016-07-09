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
     * @brief updates the radiant's position
     */
    void update();

    /**
     * @return current meteor shower status (enum)
     */
    Status getStatus() { return m_status; }

    /**
     * @return current meteor shower status (string)
     */
    QString getStatusStr();

    /**
     * @brief get meteor speed
     * @return meteor speed
     */
    int getSpeed() { return m_speed; }

    /**
     * @brief get parent object
     * @return partent object
     */
    QString getParentObject() { return m_parentObj; }

    /**
     * @brief get population index
     * @return population index
     */
    float getPopulationIdx() { return m_pidx; }

private:
    Status m_status;                // meteor shower status

    // data from catalog
    QString m_showerID;             // meteor shower ID
    QString m_designation;          // meteor shower designation
    QList<Activity> m_activities;   // activity list
    int m_speed;                    // speed of meteors
    dms m_peakAlpha;                // radiant's R.A. (peak day)
    dms m_peakDelta;                // radiant's Dec. (peak day)
    float m_driftAlpha;             // drift of R.A. for each day from peak
    float m_driftDelta;             // drift of Dec. for each day from peak
    QString m_parentObj;            // parent object for meteor shower
    float m_pidx;                   // the population index

    // current information
    Activity m_activity;            // current activity

    /**
     * @brief initialize the popup menu
     * @param pmenu
     */
    void initPopupMenu(KSPopupMenu* pmenu);

    /**
     * @brief find generic data for a given date
     * @param date QDate
     * @param found output
     * @return Activity struct
     */
    Activity findGenericData(QDate date, bool& found) const;

    /**
     * @brief find confirmed data for a given date
     * @param date QDate
     * @param found output
     * @return Activity struct
     */
    Activity findConfirmedData(QDate date, bool& found) const;
};

#endif
