/***************************************************************************
                          meteorshower.cpp  -  K Desktop Planetarium
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
#include "meteorshower.h"

#include <typeinfo>
#include "kspopupmenu.h"

MeteorShower::MeteorShower(const QVariantMap& map)
    : SkyObject(SkyObject::METEOR_SHOWER)
    , m_status(INVALID)
    , m_speed(0)
    , m_peakAlpha(0)
    , m_peakDelta(0)
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
    m_peakAlpha = dms(map.value("radiantAlpha").toString());
    m_peakDelta = dms(map.value("radiantDelta").toString());
    m_parentObj = map.value("parentObj").toString();
    m_pidx = map.value("pidx").toFloat();

    // the catalog (IMO) will give us the drift for a five-day interval from peak
    m_driftAlpha = map.value("driftAlpha").toFloat() / 5.f;
    m_driftDelta = map.value("driftDelta").toFloat() / 5.f;

    const int genericYear = 1000;

    // build the activity list
    QList<QVariant> activities = map.value("activity").toList();
    foreach(const QVariant& ms, activities)
    {
        QVariantMap activityMap = ms.toMap();
        Activity d;
        d.zhr = activityMap.value("zhr").toInt();

        if (d.zhr == -1) // is variable
        {
            QStringList variable = activityMap.value("variable").toString().split("-");
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

    // SkyObject set-up
    set(m_peakAlpha, m_peakDelta);
    setName(m_designation);
    setName2(m_showerID);
    setLongName(m_designation);
    setMag(NaN::f);

    m_status = UNDEFINED;
}

MeteorShower* MeteorShower::clone() const
{
    Q_ASSERT(typeid(this) == typeid(static_cast<const MeteorShower*> (this)));
    return new MeteorShower(*this);
}

void MeteorShower::initPopupMenu(KSPopupMenu* pmenu)
{
    pmenu->createMeteorShowerMenu(this);
}

MeteorShower::Activity MeteorShower::findGenericData(QDate date, bool& found) const
{
    int year = date.year();
    Activity g = m_activities.at(0);

    // Fix the 'generic year'!
    // Handling cases in which the shower starts in the current year and
    // ends in the next year; or when it started in the last year...
    if (g.start.year() != g.finish.year()) // edge case?
    {
        // trying the current year with the next year
        g.start.setDate(year, g.start.month(), g.start.day());
        g.finish.setDate(year + 1, g.finish.month(), g.finish.day());
        found = date >= g.start && date <= g.finish;

        if (!found)
        {
            // trying the last year with the current year
            g.start.setDate(year - 1, g.start.month(), g.start.day());
            g.finish.setDate(year, g.finish.month(), g.finish.day());
            found = date >= g.start && date <= g.finish;
        }
    }
    else
    {
        g.start.setDate(year, g.start.month(), g.start.day());
        g.finish.setDate(year, g.finish.month(), g.finish.day());
        found = date >= g.start && date <= g.finish;
    }

    if (!found)
    {
        return Activity();
    }

    g.year = g.start.year();
    int peakYear = g.peak.year() == g.start.year() ? g.start.year() : g.finish.year();
    g.peak.setDate(peakYear, g.peak.month(), g.peak.day());

    return g;
}

MeteorShower::Activity MeteorShower::findConfirmedData(QDate date, bool &found) const
{
    const int activitiesSize = m_activities.size();
    for (int i = 1; i < activitiesSize; ++i)
    {
        const Activity& a = m_activities.at(i);
        if (date.operator >=(a.start) && date.operator <=(a.finish))
        {
            found = true;
            return a;
        }
    }
    return Activity();
}

void MeteorShower::update()
{
    if (m_status == INVALID) {
        return;
    }

    bool found = false;
    QDate currentDate = KStarsData::Instance()->ut().date();

    m_activity = findConfirmedData(currentDate, found);
    if (found) {
        m_status = ACTIVE_CONFIRMED;
    } else {
        m_activity = findGenericData(currentDate, found);
        m_status = found ? ACTIVE_GENERIC : INACTIVE;
    }

    // fix the radiant position (considering drift)
    dms alpha = m_peakAlpha;
    dms delta = m_peakDelta;
    if (found) {
        double daysToPeak = (KStarsData::Instance()->ut().toUTC().toMSecsSinceEpoch()
                             - QDateTime(m_activity.peak).toMSecsSinceEpoch()) / 86400000.;
        alpha.setD(m_peakAlpha.degree() + m_driftAlpha * daysToPeak);
        delta.setD(m_peakDelta.degree() + m_driftDelta * daysToPeak);
    }
    setRA(alpha);
    setDec(delta);
}

QString MeteorShower::getStatusStr()
{
    switch (m_status) {
        case INACTIVE:
            return i18n("Inactive");
        case ACTIVE_CONFIRMED:
            return i18n("Active (confirmed)");
        case ACTIVE_GENERIC:
            return i18n("Active (generic)");
        default:
            return i18n("Invalid");
    }
}
