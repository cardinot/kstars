/***************************************************************************
                          supernova.cpp  -  K Desktop Planetarium
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

#include "meteorshower.h"

#include <typeinfo>
#include "kspopupmenu.h"

MeteorShower::MeteorShower(const QVariantMap& map)
    : m_speed(0)
    , m_radiantAlpha(0)
    , m_radiantDelta(0)
    , m_driftAlpha(0)
    , m_driftDelta(0)
    , m_pidx(0)
{
    // return initialized if the mandatory fields are not present
    if(!map.contains("showerID") || !map.contains("activity")
            || !map.contains("radiantAlpha") || !map.contains("radiantDelta"))
    {
        qWarning() << "MeteorShower: INVALID meteor shower!" << map.value("showerID").toString();
        qWarning() << "MeteorShower: Please, check your 'showers.json' catalog!";
        return;
    }

    m_showerID = map.value("showerID").toString();
    m_designation  = map.value("designation").toString();
    m_speed = map.value("speed").toInt();
    m_radiantAlpha = dms(map.value("radiantAlpha").toString());
    m_radiantDelta = dms(map.value("radiantDelta").toString());
    m_parentObj = map.value("parentObj").toString();
    m_pidx = map.value("pidx").toFloat();

    // the catalog (IMO) will give us the drift for a five-day interval from peak
    m_driftAlpha = dms(map.value("driftAlpha").toFloat() / 5.f);
    m_driftDelta = dms(map.value("driftDelta").toFloat() / 5.f);

    const int genericYear = 1000;

    // build the activity list
    QList<QVariant> activities = map.value("activity").toList();
    foreach(const QVariant& ms, activities)
    {
        QVariantMap activityMap = ms.toMap();
        Activity d;
        d.zhr = activityMap.value("zhr").toInt();

        QStringList variable = activityMap.value("variable").toString().split("-");
        if (d.zhr == -1) // is variable
        {
            bool ok = variable.size() == 2;
            for (int i=0; i < 2 && ok; i++)
            {
                d.variable.append(variable.at(i).toInt(&ok));
            }

            if (!ok)
            {
                qWarning() << "MeteorShower: INVALID data for " << m_showerID;
                qWarning() << "MeteorShower: Please, check your 'showers.json' catalog!";
                return;
            }
        }

        d.year = activityMap.value("year").toInt();
        QString year = QString::number(d.year == 0 ? genericYear : d.year);

        QString start = activityMap.value("start").toString();
        start = start.isEmpty() ? "" : QString(start + " " + year);
        d.start = QDate::fromString(start, "MM.dd yyyy");

        QString finish = activityMap.value("finish").toString();
        finish = finish.isEmpty() ? "" : QString(finish + " " + year);
        d.finish = QDate::fromString(finish, "MM.dd yyyy");

        QString peak = activityMap.value("peak").toString();
        peak = peak.isEmpty() ? "" : QString(peak + " " + year);
        d.peak = QDate::fromString(peak, "MM.dd yyyy");

        m_activities.append(d);
    }

    // filling null values of the activity list with generic data
    // and fixing the edge cases (showers between december and january)
    const Activity& g = m_activities.at(0);
    const int activitiesSize = m_activities.size();
    for (int i = 0; i < activitiesSize; ++i)
    {
        Activity a = m_activities.at(i);
        if (i != 0)
        {
            if (a.zhr == 0)
            {
                a.zhr = g.zhr;
                a.variable = g.variable;
            }

            int aux = a.year - genericYear;
            a.start = a.start.isValid() ? a.start : g.start.addYears(aux);
            a.finish = a.finish.isValid() ? a.finish : g.finish.addYears(aux);
            a.peak = a.peak.isValid() ? a.peak : g.peak.addYears(aux);
        }

        if (a.start.isValid() && a.finish.isValid() && a.peak.isValid())
        {
            // Fix the 'finish' year! Handling cases when the shower starts on
            // the current year and ends on the next year!
            if(a.start.operator >(a.finish))
            {
                a.finish = a.finish.addYears(1);
            }
            // Fix the 'peak' year
            if(a.start.operator >(a.peak))
            {
                a.peak = a.peak.addYears(1);
            }
        }
        else
        {
            qWarning() << "MeteorShower: INVALID data for "
                   << m_showerID << "Unable to read some dates!";
            qWarning() << "MeteorShower: Please, check your 'showers.json' catalog!";
            return;
        }

        m_activities.replace(i, a);
    }
}

MeteorShower::~MeteorShower()
{
}

MeteorShower* MeteorShower::clone() const
{
    return NULL;
}
