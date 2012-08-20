/***************************************************************************
                    catalogcomponent.cpp  -  K Desktop Planetarium
                             -------------------
    begin                : 2005/17/08
    copyright            : (C) 2005 by Thomas Kabelmann
    email                : thomas.kabelmann@gmx.de
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "catalogcomponent.h"

#include <QDir>
#include <QFile>
#include <QPixmap>
#include <QTextStream>
#include <kdebug.h>
#include <klocale.h>
#include <kmessagebox.h>

#include "Options.h"
#include "kstarsdata.h"
#include "skymap.h"
#include "skyobjects/starobject.h"
#include "skyobjects/deepskyobject.h"
#include "skypainter.h"

QStringList CatalogComponent::m_Columns = QString( "ID RA Dc Tp Nm Mg Flux Mj Mn PA Ig" ).split( ' ', QString::SkipEmptyParts );

CatalogComponent::CatalogComponent(SkyComposite *parent, const QString &catname, bool showerrs, int index) :
    ListComponent(parent),
    m_catName( catname ),
    m_Showerrs( showerrs ),
    m_ccIndex(index)
{
    loadData();
}

CatalogComponent::~CatalogComponent()
{
}

//TODO(spacetime): Save to DB using KSParser
//TODO(spacetime): Load from DB
//TODO(spacetime): Remove previous code




void CatalogComponent::loadData()
{
    emitProgressText( i18n("Loading custom catalog: %1", m_catName ) );

    /*
     * ******************************************
     *   READ FROM DB HERE
     * ******************************************
    */
    QMap <int, QString> names;
    KStars::Instance()->data()->catalogdb()->GetAllObjects(m_catName,
                                                           m_ObjectList,
                                                           names,
                                                           this);
    const int number_of_types = 10;
    for (int i=0; i < number_of_types; i++) {
      QList<QString> retrieved = names.values(i);
      if (retrieved.length()>0) {
          objectNames(i).append(retrieved);
      }
    }
//     objectNames(iType).append( name );
/*    QFile ccFile( m_Filename );

    if ( ccFile.open( QIODevice::ReadOnly ) ) {
        int iStart(0); //the line number of the first non-header line
        QStringList errs; //list of error messages
        QStringList Columns; //list of data column descriptors in the header

        QTextStream stream( &ccFile );
        QStringList lines = stream.readAll().split( '\n', QString::SkipEmptyParts );

        if ( parseCustomDataHeader( lines, Columns, iStart, m_Showerrs, errs ) ) {

            for ( int i=iStart; i < lines.size(); ++i ) {
                QStringList d = lines.at(i).split( ' ', QString::SkipEmptyParts );

                //Now, if one of the columns is the "Name" field, the name may contain spaces.
                //In this case, the name field will need to be surrounded by quotes.
                //Check for this, and adjust the d list accordingly
                int iname = Columns.indexOf( "Nm" );
                if ( iname >= 0 && d[iname].left(1) == "\"" ) { //multi-word name in quotes
                    d[iname] = d[iname].mid(1); //remove leading quote
                    //It's possible that the name is one word, but still in quotes
                    if ( d[iname].right(1) == "\"" ) {
                        d[iname] = d[iname].left( d[iname].length() - 1 );
                    } else {
                        int iend = iname + 1;
                        while ( d[iend].right(1) != "\"" ) {
                            d[iname] += ' ' + d[iend];
                            ++iend;
                        }
                        d[iname] += ' ' + d[iend].left( d[iend].length() - 1 );

                        //remove the entries from d list that were the multiple words in the name
                        for ( int j=iname+1; j<=iend; j++ ) {
                            d.removeAll( d.at(iname + 1) ); //index is *not* j, because next item becomes "iname+1" after remove
                        }
                    }
                }

                if ( d.size() == Columns.size() ) {
                    processCustomDataLine( i, d, Columns, m_Showerrs, errs );
                } else {
                    if ( m_Showerrs ) errs.append( i18n( "Line %1 does not contain %2 fields.  Skipping it.", i, Columns.size() ) );
                }
            }
        }

        if ( m_ObjectList.size() ) {
            if ( errs.size() > 0 ) { //some data parsed, but there are errs to report
                QString message( i18n( "Some lines in the custom catalog could not be parsed; see error messages below." ) + '\n' +
                                 i18n( "To reject the file, press Cancel. " ) +
                                 i18n( "To accept the file (ignoring unparsed lines), press Accept." ) );
                if ( KMessageBox::warningContinueCancelList( 0, message, errs,
                        i18n( "Some Lines in File Were Invalid" ), KGuiItem( i18n( "Accept" ) ) ) != KMessageBox::Continue ) {
                    m_ObjectList.clear();
                    return ;
                }
            }
        } else {
            if ( m_Showerrs ) {
                QString message( i18n( "No lines could be parsed from the specified file, see error messages below." ) );
                KMessageBox::informationList( 0, message, errs,
                                              i18n( "No Valid Data Found in File" ) );
            }
            m_ObjectList.clear();
            return;
        }
        */

//     } else { //Error opening catalog file
//         if ( m_Showerrs )
//             KMessageBox::sorry( 0, i18n( "Could not open custom data file: %1", m_Filename ),
//                                 i18n( "Error opening file" ) );
//         else
//             kDebug() << i18n( "Could not open custom data file: %1", m_Filename );
//     }
}

void CatalogComponent::update( KSNumbers * )
{
    if ( selected() ) {
        KStarsData *data = KStarsData::Instance();
        foreach ( SkyObject *obj, m_ObjectList ) {
            DeepSkyObject *dso  = dynamic_cast< DeepSkyObject * >( obj );
            StarObject *so = dynamic_cast< StarObject *>( so );
            Q_ASSERT( dso || so ); // We either have stars, or deep sky objects
            if( dso ) {
                // Update the deep sky object if need be
                if ( dso->updateID != data->updateID() ) {
                    dso->updateID = data->updateID();
                    if ( dso->updateNumID != data->updateNumID() ) {
                        dso->updateCoords( data->updateNum() );

                    }
                    dso->EquatorialToHorizontal( data->lst(), data->geo()->lat() );
                }
            }
            else {
                // Do exactly the same thing for stars
                if ( so->updateID != data->updateID() ) {
                    so->updateID = data->updateID();
                    if ( so->updateNumID != data->updateNumID() ) {
                        so->updateCoords( data->updateNum() );
                    }
                    so->EquatorialToHorizontal( data->lst(), data->geo()->lat() );
                }
            }
        }
        this->updateID = data->updateID();
    }
}

void CatalogComponent::draw( SkyPainter *skyp )
{
    if ( ! selected() ) return;

    skyp->setBrush( Qt::NoBrush );
    skyp->setPen( QColor( m_catColor ) );

    // Check if the coordinates have been updated
    if( updateID != KStarsData::Instance()->updateID() )
        update( 0 );

    //Draw Custom Catalog objects
    foreach ( SkyObject *obj, m_ObjectList ) {
        if ( obj->type()==0 ) {
            StarObject *starobj = (StarObject*)obj;
            //FIXME_SKYPAINTER
            skyp->drawPointSource(starobj, starobj->mag(), starobj->spchar() );
        } else {
            //FIXME: this PA calc is totally different from the one that was in
            //DeepSkyComponent which is now in SkyPainter .... O_o
            //      --hdevalence
            //PA for Deep-Sky objects is 90 + PA because major axis is horizontal at PA=0
            //double pa = 90. + map->findPA( dso, o.x(), o.y() );
            DeepSkyObject *dso = (DeepSkyObject*)obj;
            skyp->drawDeepSkyObject(dso,true);
        }
    }
}


bool CatalogComponent::processCustomDataLine(int lnum, const QStringList &d, const QStringList &Columns, bool showerrs, QStringList &errs )
{

    //object data
    unsigned char iType(0);
    dms RA, Dec;
    float mag(0.0), a(0.0), b(0.0), PA(0.0), flux(0.0);
    QString name, lname;

    for ( int i=0; i<Columns.size(); i++ ) {
        if ( Columns.at(i) == "ID" )
            name = m_catPrefix + ' ' + d.at(i);

        if ( Columns.at(i) == "Nm" )
            lname = d.at(i);

        if ( Columns[i] == "RA" ) {
            if ( ! RA.setFromString( d.at(i), false ) ) {
                if ( showerrs )
                    errs.append( i18n( "Line %1, field %2: Unable to parse RA value: %3" ,
                                       lnum, i, d.at(i) ) );
                return false;
            }
        }

        if ( Columns.at(i) == "Dc" ) {
            if ( ! Dec.setFromString( d.at(i), true ) ) {
                if ( showerrs )
                    errs.append( i18n( "Line %1, field %2: Unable to parse Dec value: %3" ,
                                       lnum, i, d.at(i) ) );
                return false;
            }
        }

        if ( Columns.at(i) == "Tp" ) {
            bool ok(false);
            iType = d.at(i).toUInt( &ok );
            if ( ok ) {
                if ( iType == 2 || (iType > 8 && iType < 13) || iType == 19 || iType == 20 ) {
                    if ( showerrs )
                        errs.append( i18n( "Line %1, field %2: Invalid object type: %3" ,
                                           lnum, i, d.at(i) ) +
                                     i18n( "Must be one of 0, 1, 3, 4, 5, 6, 7, 8, 18" ) );
                    return false;
                }
            } else {
                if ( showerrs )
                    errs.append( i18n( "Line %1, field %2: Unable to parse Object type: %3" ,
                                       lnum, i, d.at(i) ) );
                return false;
            }
        }

        if ( Columns.at(i) == "Mg" ) {
            bool ok(false);
            mag = d.at(i).toFloat( &ok );
            if ( ! ok ) {
                if ( showerrs )
                    errs.append( i18n( "Line %1, field %2: Unable to parse Magnitude: %3" ,
                                       lnum, i, d.at(i) ) );
                return false;
            }
        }

        if ( Columns.at(i) == "Flux" ) {
            bool ok(false);
            flux = d.at(i).toFloat( &ok );
            if ( ! ok ) {
                if ( showerrs )
                    errs.append( i18n( "Line %1, field %2: Unable to parse Flux: %3" ,
                                       lnum, i, d.at(i) ) );
                return false;
            }
        }

        if ( Columns.at(i) == "Mj" ) {
            bool ok(false);
            a = d[i].toFloat( &ok );
            if ( ! ok ) {
                if ( showerrs )
                    errs.append( i18n( "Line %1, field %2: Unable to parse Major Axis: %3" ,
                                       lnum, i, d.at(i) ) );
                return false;
            }
        }

        if ( Columns.at(i) == "Mn" ) {
            bool ok(false);
            b = d[i].toFloat( &ok );
            if ( ! ok ) {
                if ( showerrs )
                    errs.append( i18n( "Line %1, field %2: Unable to parse Minor Axis: %3" ,
                                       lnum, i, d.at(i) ) );
                return false;
            }
        }

        if ( Columns.at(i) == "PA" ) {
            bool ok(false);
            PA = d[i].toFloat( &ok );
            if ( ! ok ) {
                if ( showerrs )
                    errs.append( i18n( "Line %1, field %2: Unable to parse Position Angle: %3" ,
                                       lnum, i, d.at(i) ) );
                return false;
            }
        }
    }

    // Precess the catalog coordinates to J2000.0
    SkyPoint t;
    t.set( RA, Dec );
    if( m_catEpoch == 1950 ) {
        // Assume B1950 epoch
        t.B1950ToJ2000(); // t.ra() and t.dec() are now J2000.0 coordinates
    }
    else if( m_catEpoch == 2000 ) {
        // Do nothing
        ;
    }
    else {
        // FIXME: What should we do?
        // FIXME: This warning will be printed for each line in the catalog rather than once for the entire catalog
        kWarning() << "Unknown epoch while dealing with custom catalog."
                      "Will ignore the epoch and assume J2000.0";
    }
    RA = t.ra();
    Dec = t.dec();

    if ( iType == 0 ) { //Add a star
        StarObject *o = new StarObject( RA, Dec, mag, lname );
        m_ObjectList.append( o );
    } else { //Add a deep-sky object
        DeepSkyObject *o = new DeepSkyObject( iType, RA, Dec, mag,
                                              name, QString(), lname, m_catPrefix, a, b, PA );
        o->setFlux(flux);
        o->setCustomCatalog(this);

        m_ObjectList.append( o );

        //Add name to the list of object names
        if ( ! name.isEmpty() )
            objectNames(iType).append( name );
    }
    if ( ! lname.isEmpty() && lname != name )
        objectNames(iType).append( lname );

    return true;
}
