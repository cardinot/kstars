/***************************************************************************
                          kstarsdata.cpp  -  K Desktop Planetarium
                             -------------------
    begin                : Sun Jul 29 2001
    copyright            : (C) 2001 by Heiko Evermann
    email                : heiko@evermann.de
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

#include <QApplication>
#include <QDir>
#include <QFileInfo>
#include <QSet>
#include <QTextStream>

#include <kcomponentdata.h>
#include <kmessagebox.h>
#include <kdebug.h>
#include <klocale.h>
#include <kstandarddirs.h>

#include "Options.h"
#include "dms.h"
#include "skymap.h"
#include "ksutils.h"
#include "ksfilereader.h"
#include "ksnumbers.h"
#include "skyobjects/skypoint.h"
#include "skyobjects/skyobject.h"

#include "simclock.h"
#include "timezonerule.h"

#include <config-kstars.h>

#ifdef HAVE_INDI_H
#include "indi/indidriver.h"
#include "lilxml.h"
#include "indi/indistd.h"
#endif

#include "dialogs/detaildialog.h"


KStarsData* KStarsData::pinstance = 0;

KStarsData* KStarsData::Create( KStars* kstars )
{
    if ( pinstance )
        delete pinstance;
    pinstance = new KStarsData( kstars );
    return pinstance;
}

KStarsData* KStarsData::Instance( )
{
    return pinstance;
}


KStarsData::KStarsData(KStars* kstars) :
    locale(0),
    m_kstars(kstars),
    m_preUpdateID(0),        m_updateID(0),
    m_preUpdateNumID(0),     m_updateNumID(0),
    m_preUpdateNum( J2000 ), m_updateNum( J2000 )
{
    startupComplete = false;

    TypeName[0] = i18n( "star" );
    TypeName[1] = i18n( "star" );
    TypeName[2] = i18n( "planet" );
    TypeName[3] = i18n( "open cluster" );
    TypeName[4] = i18n( "globular cluster" );
    TypeName[5] = i18n( "gaseous nebula" );
    TypeName[6] = i18n( "planetary nebula" );
    TypeName[7] = i18n( "supernova remnant" );
    TypeName[8] = i18n( "galaxy" );
    TypeName[9] = i18n( "comet" );
    TypeName[10] = i18n( "asteroid" );
    TypeName[11] = i18n( "constellation" );
    TypeName[12] = i18n( "Moon" );
    TypeName[13] = i18n( "asterism" );
    TypeName[14] = i18n( "galaxy cluster" );
    TypeName[15] = i18n( "dark nebula" );
    TypeName[16] = i18n( "quasar" );
    TypeName[17] = i18n( "multiple star" );

    //standard directories and locale objects
    locale = new KLocale( "kstars" );

    m_SkyComposite = new SkyMapComposite( 0, this );
    m_logObject = new Comast::Log;

    // at startup times run forward
    setTimeDirection( 0.0 );

    //The StoredDate is used when saving user settings in a script; initialize to invalid date
    StoredDate = KDateTime();

    temporaryTrail = false;
}

KStarsData::~KStarsData() {
    delete locale;
    delete m_logObject;

    qDeleteAll( geoList );
    qDeleteAll( VariableStarsList );
    qDeleteAll( INDIHostsList );
    qDeleteAll( ADVtreeList );
}

QString KStarsData::typeName( int i ) {
    if ( i >= 0 && i < 12 )
        return TypeName[i];
    return i18n( "no type" );
}

void KStarsData::initialize() {
    if (startupComplete) return;

    QTimer::singleShot(0, this, SLOT( slotInitialize() ) );
    initCounter = 0;
}

void KStarsData::initError(const QString &s, bool required = false) {
    QString message, caption;

    if (required) {
        message = i18n( "The file %1 could not be found. "
                        "KStars cannot run properly without this file. "
                        "To continue loading, place the file in one of the "
                        "following locations, then press Retry:\n\n", s )
                  + QString( "\t$(KDEDIR)/share/apps/kstars/%1\n" ).arg( s )
                  + QString( "\t~/.kde/share/apps/kstars/%1\n\n" ).arg( s )
                  + i18n( "Otherwise, press Cancel to shutdown." );
        caption = i18n( "Critical File Not Found: %1", s );
    } else {
        message = i18n( "The file %1 could not be found. "
                        "KStars can still run without this file. "
                        "However, to avoid seeing this message in the future, you can "
                        "place the file in one of the following locations, then press Retry:\n\n", s )
                  + QString( "\t$(KDEDIR)/share/apps/kstars/%1\n" ).arg( s )
                  + QString( "\t~/.kde/share/apps/kstars/%1\n\n" ).arg( s )
                  + i18n( "Otherwise, press Cancel to continue loading without this file.");
        caption = i18n( "Non-Critical File Not Found: %1", s );
    }

    if ( KMessageBox::warningContinueCancel( 0, message, caption, KGuiItem( i18n( "Retry" ) ) ) == KMessageBox::Continue ) {
        initCounter--;
        QTimer::singleShot(0, this, SLOT( slotInitialize() ) );
    } else {
        if (required) {
            emit initFinished(false);
        } else {
            QTimer::singleShot(0, this, SLOT( slotInitialize() ) );
        }
    }
}

void KStarsData::slotInitialize() {

    qApp->flush(); // flush all paint events before loading data

    switch ( initCounter )
    {
    case 0: //Load Time Zone Rules//
        emit progressText( i18n("Reading time zone rules") );

        // timezone rules
        if ( !readTimeZoneRulebook( ) )
            initError( "TZrules.dat", true );

        break;

    case 1: //Load Cities//
        emit progressText( i18n("Loading city data") );
        if ( !readCityData( ) )
            initError( "Cities.dat", true );
        break;

    case 2: //Initialize SkyMapComposite//

        emit progressText(i18n("Loading sky objects" ) );
        skyComposite()->init( this );
        break;

    case 3: //Load Image URLs//

        emit progressText( i18n("Loading Image URLs" ) );
        if ( !readURLData( "image_url.dat", 0 ) ) {
            initError( "image_url.dat", false );
        }

        break;

    case 4: //Load Information URLs//

        emit progressText( i18n("Loading Information URLs" ) );
        if ( !readURLData( "info_url.dat", 1 ) ) {
            initError( "info_url.dat", false );
        }

        break;

    case 5:
        emit progressText( i18n("Loading Variable Stars" ) );
        readINDIHosts();
        readUserLog();
        readVARData();
        readADVTreeData();
        break;

    default:
        startupComplete = true;
        emit initFinished(true);
        break;
    } // switch ( initCounter )

    initCounter++;  // before processEvents!
    if(!startupComplete)
        QTimer::singleShot(0, this, SLOT( slotInitialize() ) );
    qApp->processEvents();
}

void KStarsData::updateTime( GeoLocation *geo, SkyMap *skymap, const bool automaticDSTchange ) {
    // sync LTime with the simulation clock
    LTime = geo->UTtoLT( ut() );
    syncLST();

    //Only check DST if (1) TZrule is not the empty rule, and (2) if we have crossed
    //the DST change date/time.
    if ( !geo->tzrule()->isEmptyRule() ) {
        if ( TimeRunsForward ) {
            // timedirection is forward
            // DST change happens if current date is bigger than next calculated dst change
            if ( ut() > NextDSTChange ) resetToNewDST(geo, automaticDSTchange);
        } else {
            // timedirection is backward
            // DST change happens if current date is smaller than next calculated dst change
            if ( ut() < NextDSTChange ) resetToNewDST(geo, automaticDSTchange);
        }
    }

    KSNumbers num( ut().djd() );

    //TIMING
    QTime t;

    if ( fabs( ut().djd() - LastNumUpdate.djd() ) > 1.0 ) {
        LastNumUpdate = ut().djd();
        //TIMING
        //		t.start();

        m_preUpdateNumID++;
        m_preUpdateNum = KSNumbers( num );
        skyComposite()->update( this, &num );

        //TIMING
        //		kDebug() << QString("SkyMapComposite::update() took %1 ms").arg(t.elapsed());
    }

    if ( fabs( ut().djd() - LastPlanetUpdate.djd() ) > 0.01 ) {
        LastPlanetUpdate = ut().djd();
        //TIMING
        //		t.start();

        skyComposite()->updatePlanets( this, &num );

        //TIMING
        //		kDebug() << QString("SkyMapComposite::updatePlanets() took %1 ms").arg(t.elapsed());
    }

    // Moon moves ~30 arcmin/hr, so update its position every minute.
    if ( fabs( ut().djd() - LastMoonUpdate.djd() ) > 0.00069444 ) {
        LastMoonUpdate = ut();
        //TIMING
        //		t.start();

        skyComposite()->updateMoons( this, &num );

        //TIMING
        //		kDebug() << QString("SkyMapComposite::updateMoons() took %1 ms").arg(t.elapsed());
    }

    //Update Alt/Az coordinates.  Timescale varies with zoom level
    //If Clock is in Manual Mode, always update. (?)
    if ( fabs( ut().djd() - LastSkyUpdate.djd() ) > 0.1/Options::zoomFactor() || clock()->isManualMode() ) {
        LastSkyUpdate = ut();
        //TIMING
        //		t.start();

        m_preUpdateID++;
        skyComposite()->update( this ); //omit KSNumbers arg == just update Alt/Az coords

        //Update focus
        skymap->updateFocus();

        //TIMING
        //		kDebug() << QString("SkyMapComposite::update() for Alt/Az took %1 ms").arg(t.elapsed());

        if ( clock()->isManualMode() )
            QTimer::singleShot( 0, skymap, SLOT( forceUpdateNow() ) );
        else skymap->forceUpdate();
    }
}

void KStarsData::syncUpdateIDs()
{
    m_updateID = m_preUpdateID;
    if ( m_updateNumID == m_preUpdateNumID ) return;
    m_updateNumID = m_preUpdateNumID;
    m_updateNum = KSNumbers( m_preUpdateNum );
}

unsigned int KStarsData::incUpdateID() {
    m_preUpdateID++;
    m_preUpdateNumID++;
    syncUpdateIDs();
    return m_updateID;
}

void KStarsData::setFullTimeUpdate() {
    //Set the update markers to invalid dates to trigger updates in each category
    LastSkyUpdate = KDateTime();
    LastPlanetUpdate = KDateTime();
    LastMoonUpdate = KDateTime();
    LastNumUpdate = KDateTime();
}

void KStarsData::syncLST() {
    LST.set( geo()->GSTtoLST( ut().gst() ) );
}

void KStarsData::changeDateTime( const KStarsDateTime &newDate ) {
    //Turn off animated slews for the next time step.
    setSnapNextFocus();

    clock()->setUTC( newDate );

    LTime = geo()->UTtoLT( ut() );
    //set local sideral time
    syncLST();

    //Make sure Numbers, Moon, planets, and sky objects are updated immediately
    setFullTimeUpdate();

    // reset tzrules data with new local time and time direction (forward or backward)
    geo()->tzrule()->reset_with_ltime(LTime, geo()->TZ0(), isTimeRunningForward() );

    // reset next dst change time
    setNextDSTChange( geo()->tzrule()->nextDSTChange() );
}

void KStarsData::resetToNewDST(const GeoLocation *geo, const bool automaticDSTchange) {
    // reset tzrules data with local time, timezone offset and time direction (forward or backward)
    // force a DST change with option true for 3. parameter
    geo->tzrule()->reset_with_ltime( LTime, geo->TZ0(), TimeRunsForward, automaticDSTchange );
    // reset next DST change time
    setNextDSTChange( geo->tzrule()->nextDSTChange() );
    //reset LTime, because TZoffset has changed
    LTime = geo->UTtoLT( ut() );
}

void KStarsData::setTimeDirection( float scale ) {
    TimeRunsForward = ( scale < 0 ? false : true );
}

GeoLocation* KStarsData::locationNamed( const QString &city, const QString &province, const QString &country ) {
    foreach ( GeoLocation *loc, geoList ) {
        if ( loc->translatedName() == city &&
                ( province.isEmpty() || loc->translatedProvince() == province ) &&
                ( country.isEmpty() || loc->translatedCountry() == country ) ) {
            return loc;
        }
    }

    return 0;
}

void KStarsData::setLocationFromOptions() {
    setLocation( GeoLocation ( Options::longitude(), Options::latitude(),
                               Options::cityName(), Options::provinceName(), Options::countryName(),
                               Options::timeZone(), &(Rulebook[ Options::dST() ]), 4, Options::elevation() ) );
}

void KStarsData::setLocation( const GeoLocation &l ) {
    Geo = GeoLocation(l);
    if ( Geo.lat()->Degrees() >= 90.0 ) Geo.setLat( 89.99 );
    if ( Geo.lat()->Degrees() <= -90.0 ) Geo.setLat( -89.99 );

    //store data in the Options objects
    Options::setCityName( Geo.name() );
    Options::setProvinceName( Geo.province() );
    Options::setCountryName( Geo.country() );
    Options::setTimeZone( Geo.TZ0() );
    Options::setElevation( Geo.height() );
    Options::setLongitude( Geo.lng()->Degrees() );
    Options::setLatitude( Geo.lat()->Degrees() );
}

SkyObject* KStarsData::objectNamed( const QString &name ) {
    if ( (name== "star") || (name== "nothing") || name.isEmpty() ) return NULL;

    return skyComposite()->findByName( name );
}

bool KStarsData::readCityData( void ) {
    QFile file;
    bool citiesFound = false;

    // begin new code
    if ( KSUtils::openDataFile( file, "Cities.dat" ) ) {
        KSFileReader fileReader( file ); // close file is included
        while ( fileReader.hasMoreLines() ) {
            citiesFound |= processCity( fileReader.readLine() );
        }
        file.close();  // -jbb because I changed KSFileReader.
    }
    // end new code

    //check for local cities database, but don't require it.
    file.setFileName( KStandardDirs::locate( "appdata", "mycities.dat" ) ); //determine filename in local user KDE directory tree.
    if ( file.exists() && file.open( QIODevice::ReadOnly ) ) {
        QTextStream stream( &file );

        while ( !stream.atEnd() ) {
            QString line = stream.readLine();
            citiesFound |= processCity( line );
        }	// while ( !stream.atEnd() )
        file.close();
    }	// if ( fileopen() )

    return citiesFound;
}

bool KStarsData::processCity( const QString& line ) {
    QString totalLine;
    QString name, province, country;
    QStringList fields;
    TimeZoneRule *TZrule;
    bool intCheck = true;
    QChar latsgn, lngsgn;
    int lngD, lngM, lngS;
    int latD, latM, latS;
    double TZ;
    float lng, lat;

    totalLine = line;

    // separate fields
    fields = line.split( ':' );

    for ( int i=0; i< fields.size(); ++i )
        fields[i] = fields[i].trimmed();

    if ( fields.size() < 11 ) {
        kDebug()<< i18n( "Cities.dat: Ran out of fields.  Line was:" );
        kDebug()<< totalLine.toLocal8Bit();
        return false;
    } else if ( fields.size() < 12 ) {
        // allow old format (without TZ) for mycities.dat
        fields.append(QString());
        fields.append("--");
    } else if ( fields.size() < 13 ) {
        // Set TZrule to "--"
        fields.append("--");
    }

    name = fields[0];
    province = fields[1];
    country = fields[2];

    latD = fields[3].toInt( &intCheck );
    if ( !intCheck ) {
        kDebug() << fields[3] << i18n( "\nCities.dat: Bad integer.  Line was:\n" ) << totalLine;
        return false;
    }

    latM = fields[4].toInt( &intCheck );
    if ( !intCheck ) {
        kDebug() << fields[4] << i18n( "\nCities.dat: Bad integer.  Line was:\n" ) << totalLine;
        return false;
    }

    latS = fields[5].toInt( &intCheck );
    if ( !intCheck ) {
        kDebug() << fields[5] << i18n( "\nCities.dat: Bad integer.  Line was:\n" ) << totalLine;
        return false;
    }

    QChar ctemp = fields[6].at(0);
    latsgn = ctemp;
    if (latsgn != 'N' && latsgn != 'S') {
        kDebug() << latsgn << i18n( "\nCities.dat: Invalid latitude sign.  Line was:\n" ) << totalLine;
        return false;
    }

    lngD = fields[7].toInt( &intCheck );
    if ( !intCheck ) {
        kDebug() << fields[7] << i18n( "\nCities.dat: Bad integer.  Line was:\n" ) << totalLine;
        return false;
    }

    lngM = fields[8].toInt( &intCheck );
    if ( !intCheck ) {
        kDebug() << fields[8] << i18n( "\nCities.dat: Bad integer.  Line was:\n" ) << totalLine;
        return false;
    }

    lngS = fields[9].toInt( &intCheck );
    if ( !intCheck ) {
        kDebug() << fields[9] << i18n( "\nCities.dat: Bad integer.  Line was:\n" ) << totalLine;
        return false;
    }

    ctemp = fields[10].at(0);
    lngsgn = ctemp;
    if (lngsgn != 'E' && lngsgn != 'W') {
        kDebug() << latsgn << i18n( "\nCities.dat: Invalid longitude sign.  Line was:\n" ) << totalLine;
        return false;
    }

    lat = (float)latD + ((float)latM + (float)latS/60.0)/60.0;
    lng = (float)lngD + ((float)lngM + (float)lngS/60.0)/60.0;

    if ( latsgn == 'S' ) lat *= -1.0;
    if ( lngsgn == 'W' ) lng *= -1.0;

    // find time zone. Use value from Cities.dat if available.
    // otherwise use the old approximation: int(lng/15.0);
    if ( fields[11].isEmpty() || ('x' == fields[11].at(0)) ) {
        TZ = int(lng/15.0);
    } else {
        bool doubleCheck = true;
        TZ = fields[11].toDouble( &doubleCheck);
        if ( !doubleCheck ) {
            kDebug() << fields[11] << i18n( "\nCities.dat: Bad time zone.  Line was:\n" ) << totalLine;
            return false;
        }
    }

    //last field is the TimeZone Rule ID.
    TZrule = &( Rulebook[ fields[12] ] );

    //	if ( fields[12]=="--" )
    //		kDebug() << "Empty rule start month: " << TZrule->StartMonth;
    geoList.append ( new GeoLocation( lng, lat, name, province, country, TZ, TZrule ));  // appends city names to list
    return true;
}

bool KStarsData::readTimeZoneRulebook( void ) {
    QFile file;
    QString id;

    if ( KSUtils::openDataFile( file, "TZrules.dat" ) ) {
        QTextStream stream( &file );

        while ( !stream.atEnd() ) {
            QString line = stream.readLine().trimmed();
            if ( line.length() && !line.startsWith('#') ) { //ignore commented and blank lines
                QStringList fields = line.split( ' ', QString::SkipEmptyParts );
                id = fields[0];
                QTime stime = QTime( fields[3].left( fields[3].indexOf(':')).toInt() ,
                                     fields[3].mid( fields[3].indexOf(':')+1, fields[3].length()).toInt() );
                QTime rtime = QTime( fields[6].left( fields[6].indexOf(':')).toInt(),
                                     fields[6].mid( fields[6].indexOf(':')+1, fields[6].length()).toInt() );

                Rulebook[ id ] = TimeZoneRule( fields[1], fields[2], stime, fields[4], fields[5], rtime );
            }
        }
        return true;
    } else {
        return false;
    }
}

bool KStarsData::openUrlFile(const QString &urlfile, QFile & file) {
    //QFile file;
    QString localFile;
    bool fileFound = false;
    QFile localeFile;

    if ( locale->language() != "en_US" )
        localFile = locale->language() + '/' + urlfile;

    if ( ! localFile.isEmpty() && KSUtils::openDataFile( file, localFile ) ) {
        fileFound = true;
    } else {
        // Try to load locale file, if not successful, load regular urlfile and then copy it to locale.
        file.setFileName( KStandardDirs::locateLocal( "appdata", urlfile ) );
        if ( file.open( QIODevice::ReadOnly ) ) {
            //local file found.  Now, if global file has newer timestamp, then merge the two files.
            //First load local file into QStringList
            bool newDataFound( false );
            QStringList urlData;
            QTextStream lStream( &file );
            while ( ! lStream.atEnd() ) urlData.append( lStream.readLine() );

            //Find global file(s) in findAllResources() list.
            QFileInfo fi_local( file.fileName() );
            QStringList flist = KGlobal::mainComponent().dirs()->findAllResources( "appdata", urlfile );
            for ( int i=0; i< flist.size(); i++ ) {
                if ( flist[i] != file.fileName() ) {
                    QFileInfo fi_global( flist[i] );

                    //Is this global file newer than the local file?
                    if ( fi_global.lastModified() > fi_local.lastModified() ) {
                        //Global file has newer timestamp than local.  Add lines in global file that don't already exist in local file.
                        //be smart about this; in some cases the URL is updated but the image is probably the same if its
                        //label string is the same.  So only check strings up to last ":"
                        QFile globalFile( flist[i] );
                        if ( globalFile.open( QIODevice::ReadOnly ) ) {
                            QTextStream gStream( &globalFile );
                            while ( ! gStream.atEnd() ) {
                                QString line = gStream.readLine();

                                //If global-file line begins with "XXX:" then this line should be removed from the local file.
                                if ( line.startsWith(QLatin1String("XXX:"))  && urlData.contains( line.mid( 4 ) ) ) {
                                    urlData.removeAt( urlData.indexOf( line.mid( 4 ) ) );
                                } else {
                                    //does local file contain the current global file line, up to second ':' ?

                                    bool linefound( false );
                                    for ( int j=0; j< urlData.size(); ++j ) {
                                        if ( urlData[j].contains( line.left( line.indexOf( ':', line.indexOf( ':' ) + 1 ) ) ) ) {
                                            //replace line in urlData with its equivalent in the newer global file.
                                            urlData.replace( j, line );
                                            if ( !newDataFound ) newDataFound = true;
                                            linefound = true;
                                            break;
                                        }
                                    }
                                    if ( ! linefound ) {
                                        urlData.append( line );
                                        if ( !newDataFound ) newDataFound = true;
                                    }
                                }
                            }
                        }
                    }
                }
            }

            file.close();

            //(possibly) write appended local file
            if ( newDataFound ) {
                if ( file.open( QIODevice::WriteOnly ) ) {
                    QTextStream outStream( &file );
                    for ( int i=0; i<urlData.size(); i++ ) {
                        outStream << urlData[i] << endl;
                    }
                    file.close();
                }
            }

            if ( file.open( QIODevice::ReadOnly ) ) fileFound = true;

        } else {
            if ( KSUtils::openDataFile( file, urlfile ) ) {
                if ( locale->language() != "en_US" ) kDebug() << i18n( "No localized URL file; using default English file." );
                // we found urlfile, we need to copy it to locale
                localeFile.setFileName( KStandardDirs::locateLocal( "appdata", urlfile ) );
                if (localeFile.open(QIODevice::WriteOnly)) {
                    QTextStream readStream(&file);
                    QTextStream writeStream(&localeFile);
                    while ( ! readStream.atEnd() ) {
                        QString line = readStream.readLine();
                        if ( !line.startsWith( QLatin1String( "XXX:" ) ) ) //do not write "deleted" lines
                            writeStream << line << endl;
                    }

                    localeFile.close();
                    file.reset();
                } else {
                    kDebug() << i18n( "Failed to copy default URL file to locale folder, modifying default object links is not possible" );
                }
                fileFound = true;
            }
        }
    }
    return fileFound;
}

bool KStarsData::readURLData( const QString &urlfile, int type, bool deepOnly ) {
    QFile file;
    if (!openUrlFile(urlfile, file)) return false;

    QTextStream stream(&file);

    while ( !stream.atEnd() ) {
        QString line = stream.readLine();

        //ignore comment lines
        if ( !line.startsWith('#') ) {
            int idx = line.indexOf(':');
            QString name = line.left( idx );
            QString sub = line.mid( idx + 1 );
            idx = sub.indexOf(':');
            QString title = sub.left( idx );
            QString url = sub.mid( idx + 1 );
            // Dirty hack to fix things up for planets
            SkyObject *o;
            if( name == "Mercury" || name == "Venus" || name == "Mars" || name == "Jupiter"
                || name == "Saturn" || name == "Uranus" || name == "Neptune" || name == "Pluto" )
                o = skyComposite()->findByName( i18n( name.toLocal8Bit().data() ) );
            else
                o = skyComposite()->findByName( name );

            if ( !o ) {
                kWarning() << i18n( "Object named %1 not found", name ) ;
            } else {
                if ( ! deepOnly || ( o->type() > 2 && o->type() < 9 ) ) {
                    if ( type==0 ) { //image URL
                        o->ImageList().append( url );
                        o->ImageTitle().append( title );
                    } else if ( type==1 ) { //info URL
                        o->InfoList().append( url );
                        o->InfoTitle().append( title );
                    }
                }
            }
        }
    }
    file.close();
    return true;
}

bool KStarsData::readUserLog(void)
{
    QFile file;
    QString buffer;
    QString sub, name, data;

    if (!KSUtils::openDataFile( file, "userlog.dat" )) return false;

    QTextStream stream(&file);

    if (!stream.atEnd()) buffer = stream.readAll();

    while (!buffer.isEmpty()) {
        int startIndex, endIndex;

        startIndex = buffer.indexOf(QLatin1String("[KSLABEL:"));
        sub = buffer.mid(startIndex);
        endIndex = sub.indexOf(QLatin1String("[KSLogEnd]"));

        // Read name after KSLABEL identifer
        name = sub.mid(startIndex + 9, sub.indexOf(']') - (startIndex + 9));
        // Read data and skip new line
        data   = sub.mid(sub.indexOf(']') + 2, endIndex - (sub.indexOf(']') + 2));
        buffer = buffer.mid(endIndex + 11);

        //Find the sky object named 'name'.
        //Note that ObjectNameList::find() looks for the ascii representation
        //of star genetive names, so stars are identified that way in the user log.
        SkyObject *o = skyComposite()->findByName(name);
        if ( !o ) {
            kWarning() << name << " not found" ;
        } else {
            o->userLog() = data;
        }

    } // end while
    file.close();
    return true;
}

bool KStarsData::readADVTreeData(void)
{
    QFile file;
    QString Interface;
    QString Name, Link, subName;

    if (!KSUtils::openDataFile(file, "advinterface.dat"))
        return false;

    QTextStream stream(&file);
    QString Line;

    while  (!stream.atEnd())
    {
        int Type, interfaceIndex;

        Line = stream.readLine();

        if (Line.startsWith(QLatin1String("[KSLABEL]")))
        {
            Name = Line.mid(9);
            Type = 0;
        }
        else if (Line.startsWith(QLatin1String("[END]")))
            Type = 1;
        else if (Line.startsWith(QLatin1String("[KSINTERFACE]")))
        {
            Interface = Line.mid(13);
            continue;
        }

        else
        {
            int idx = Line.indexOf(':');
            Name = Line.left(idx);
            Link = Line.mid(idx + 1);

            // Link is empty, using Interface instead
            if (Link.isEmpty())
            {
                Link = Interface;
                subName = Name;
                interfaceIndex = Link.indexOf(QLatin1String("KSINTERFACE"));
                Link.remove(interfaceIndex, 11);
                Link = Link.insert(interfaceIndex, subName.replace(' ', '+'));

            }

            Type = 2;
        }

        ADVTreeData *ADVData = new ADVTreeData;

        ADVData->Name = Name;
        ADVData->Link = Link;
        ADVData->Type = Type;

        ADVtreeList.append(ADVData);
    }

    return true;
}

bool KStarsData::readVARData(void)
{
    QString varFile("valaav.txt");
    QFile localeFile;
    QFile file;

    file.setFileName( KStandardDirs::locateLocal( "appdata", varFile ) );
    if ( !file.open( QIODevice::ReadOnly ) )
    {
        // Open default variable stars file
        if ( KSUtils::openDataFile( file, varFile ) )
        {
            // we found urlfile, we need to copy it to locale
            localeFile.setFileName( KStandardDirs::locateLocal( "appdata", varFile ) );

            if (localeFile.open(QIODevice::WriteOnly))
            {
                QTextStream readStream(&file);
                QTextStream writeStream(&localeFile);
                writeStream <<  readStream.readAll();
                localeFile.close();
                file.reset();
            }
        }
        else
            return false;
    }


    QTextStream stream(&file);
    stream.readLine();

    QString Name, Designation, Line;
    while  (!stream.atEnd())
    {
        Line = stream.readLine();

        if (Line.startsWith('*'))
            break;

        Designation = Line.left(8).trimmed();
        Name        = Line.mid(10,20).simplified();

        VariableStarInfo *VInfo = new VariableStarInfo;

        VInfo->Designation = Designation;
        VInfo->Name        = Name;
        VariableStarsList.append(VInfo);
    }

    return true;
}


bool KStarsData::readINDIHosts(void)
{
#ifdef HAVE_INDI_H
    QString indiFile("indihosts.xml");
    QFile localeFile;
    QFile file;
    char errmsg[1024];
    char c;
    LilXML *xmlParser = newLilXML();
    XMLEle *root = NULL;
    XMLAtt *ap;

    file.setFileName( KStandardDirs::locate( "appdata", indiFile ) );
    if ( file.fileName().isEmpty() || !file.open( QIODevice::ReadOnly ) )
        return false;

    while ( file.getChar( &c ) )
    {
        root = readXMLEle(xmlParser, c, errmsg);

        if (root)
        {
            // Get host name
            ap = findXMLAtt(root, "name");
            if (!ap) {
                delLilXML(xmlParser);
                return false;
            }

            INDIHostsInfo *VInfo = new INDIHostsInfo;
            VInfo->name = QString(valuXMLAtt(ap));

            // Get host name
            ap = findXMLAtt(root, "hostname");

            if (!ap) {
                delete VInfo;
                delLilXML(xmlParser);
                return false;
            }

            VInfo->hostname = QString(valuXMLAtt(ap));
            ap = findXMLAtt(root, "port");

            if (!ap) {
                delete VInfo;
                delLilXML(xmlParser);
                return false;
            }

            VInfo->portnumber = QString(valuXMLAtt(ap));
            VInfo->isConnected = false;
            VInfo->deviceManager = NULL;
            INDIHostsList.append(VInfo);

            delXMLEle(root);
        }
        else if (errmsg[0])
            kDebug() << errmsg;
    }

    delLilXML(xmlParser);

    return true;
    #else
    return false;
    #endif
}

//There's a lot of code duplication here, but it's not avoidable because 
//this function is only called from main.cpp when the user is using 
//"dump" mode to produce an image from the command line.  In this mode, 
//there is no KStars object, so none of the DBus functions can be called 
//directly.
bool KStarsData::executeScript( const QString &scriptname, SkyMap *map ) {
    int cmdCount(0);

    QFile f( scriptname );
    if ( !f.open( QIODevice::ReadOnly) ) {
        kDebug() << i18n( "Could not open file %1", f.fileName() );
        return false;
    }

    QTextStream istream(&f);
    while ( ! istream.atEnd() ) {
        QString line = istream.readLine();
        line.remove( "string:" );
        line.remove( "int:" );
        line.remove( "double:" );
        line.remove( "bool:" );

        //find a dbus line and extract the function name and its arguments
        //The function name starts after the last occurrence of "org.kde.kstars."
        //or perhaps "org.kde.kstars.SimClock.".
        if ( line.startsWith(QString("dbus-send")) ) {
            QString funcprefix = "org.kde.kstars.SimClock.";
            int i = line.lastIndexOf( funcprefix );
            if ( i >= 0 ) {
                i += funcprefix.length();
            } else {
                funcprefix = "org.kde.kstars.";
                i = line.lastIndexOf( funcprefix );
                if ( i >= 0 ) {
                    i += funcprefix.length();
                }
            }
            if ( i < 0 ) {
                kWarning() << "Could not parse line: " << line;
                return false;
            }

            QStringList fn = line.mid(i).split( ' ' );

            //DEBUG
            kDebug() << fn << endl;

            if ( fn[0] == "lookTowards" && fn.size() >= 2 ) {
                double az(-1.0);
                QString arg = fn[1].toLower();
                if ( arg == "n"  || arg == "north" )     az =   0.0;
                if ( arg == "ne" || arg == "northeast" ) az =  45.0;
                if ( arg == "e"  || arg == "east" )      az =  90.0;
                if ( arg == "se" || arg == "southeast" ) az = 135.0;
                if ( arg == "s"  || arg == "south" )     az = 180.0;
                if ( arg == "sw" || arg == "southwest" ) az = 225.0;
                if ( arg == "w"  || arg == "west" )      az = 270.0;
                if ( arg == "nw" || arg == "northwest" ) az = 335.0;
                if ( az >= 0.0 ) {
                    map->setFocusAltAz( 90.0, map->focus()->az()->Degrees() );
                    map->focus()->HorizontalToEquatorial( &LST, geo()->lat() );
                    map->setDestination( map->focus() );
                    cmdCount++;
                }

                if ( arg == "z" || arg == "zenith" ) {
                    map->setFocusAltAz( 90.0, map->focus()->az()->Degrees() );
                    map->focus()->HorizontalToEquatorial( &LST, geo()->lat() );
                    map->setDestination( map->focus() );
                    cmdCount++;
                }

                //try a named object.  The name is everything after fn[0],
                //concatenated with spaces.
                fn.removeAll( fn.first() );
                QString objname = fn.join( " " );
                SkyObject *target = objectNamed( objname );
                if ( target ) { 
                    map->setFocus( target );
                    map->focus()->EquatorialToHorizontal( &LST, geo()->lat() );
                    map->setDestination( map->focus() );
                    cmdCount++;
                }

            } else if ( fn[0] == "setRaDec" && fn.size() == 3 ) {
                bool ok( false );
                dms r(0.0), d(0.0);

                ok = r.setFromString( fn[1], false ); //assume angle in hours
                if ( ok ) ok = d.setFromString( fn[2], true );  //assume angle in degrees
                if ( ok ) {
                    map->setFocus( r, d );
                    map->focus()->EquatorialToHorizontal( &LST, geo()->lat() );
                    cmdCount++;
                }

            } else if ( fn[0] == "setAltAz" && fn.size() == 3 ) {
                bool ok( false );
                dms az(0.0), alt(0.0);

                ok = alt.setFromString( fn[1] );
                if ( ok ) ok = az.setFromString( fn[2] );
                if ( ok ) {
                    map->setFocusAltAz( alt, az );
                    map->focus()->HorizontalToEquatorial( &LST, geo()->lat() );
                    cmdCount++;
                }

            } else if ( fn[0] == "loadColorScheme" ) {
                fn.removeAll( fn.first() );
                QString csName = fn.join(" ").remove( '\"' );
                kDebug() << "Color scheme: " << csName << endl;

                QString filename = csName.toLower().trimmed();
                bool ok( false );

                //Parse default names which don't follow the regular file-naming scheme
                if ( csName == i18nc("use default color scheme", "Default Colors") ) filename = "classic.colors";
                if ( csName == i18nc("use 'star chart' color scheme", "Star Chart") ) filename = "chart.colors";
                if ( csName == i18nc("use 'night vision' color scheme", "Night Vision") ) filename = "night.colors";

                //Try the filename if it ends with ".colors"
                if ( filename.endsWith( QLatin1String( ".colors" ) ) )
                    ok = colorScheme()->load( filename );

                //If that didn't work, try assuming that 'name' is the color scheme name
                //convert it to a filename exactly as ColorScheme::save() does
                if ( ! ok ) {
                    if ( !filename.isEmpty() ) {
                        for( int i=0; i<filename.length(); ++i)
                            if ( filename.at(i)==' ' ) filename.replace( i, 1, "-" );
            
                        filename = filename.append( ".colors" );
                        ok = colorScheme()->load( filename );
                    }
            
                    if ( ! ok ) kDebug() << i18n( "Unable to load color scheme named %1. Also tried %2.", csName, filename ) << endl;
                }

            } else if ( fn[0] == "zoom" && fn.size() == 2 ) {
                bool ok(false);
                double z = fn[1].toDouble(&ok);
                if ( ok ) {
                    if ( z > MAXZOOM ) z = MAXZOOM;
                    if ( z < MINZOOM ) z = MINZOOM;
                    Options::setZoomFactor( z );
                    cmdCount++;
                }

            } else if ( fn[0] == "zoomIn" ) {
                if ( Options::zoomFactor() < MAXZOOM ) {
                    Options::setZoomFactor( Options::zoomFactor() * DZOOM );
                    cmdCount++;
                }
            } else if ( fn[0] == "zoomOut" ) {
                if ( Options::zoomFactor() > MINZOOM ) {
                    Options::setZoomFactor( Options::zoomFactor() / DZOOM );
                    cmdCount++;
                }
            } else if ( fn[0] == "defaultZoom" ) {
                Options::setZoomFactor( DEFAULTZOOM );
                cmdCount++;
            } else if ( fn[0] == "setLocalTime" && fn.size() == 7 ) {
                bool ok(false);
                // min is a macro - use mnt
                int yr(0), mth(0), day(0) ,hr(0), mnt(0), sec(0);
                yr = fn[1].toInt(&ok);
                if ( ok ) mth = fn[2].toInt(&ok);
                if ( ok ) day = fn[3].toInt(&ok);
                if ( ok ) hr  = fn[4].toInt(&ok);
                if ( ok ) mnt = fn[5].toInt(&ok);
                if ( ok ) sec = fn[6].toInt(&ok);
                if ( ok ) {
                    changeDateTime( geo()->LTtoUT( KStarsDateTime( QDate(yr, mth, day), QTime(hr,mnt,sec) ) ) );
                    cmdCount++;
                } else {
                    kWarning() << ki18n( "Could not set time: %1 / %2 / %3 ; %4:%5:%6" )
                    .subs( day ).subs( mth ).subs( yr )
                    .subs( hr ).subs( mnt ).subs( sec ).toString() << endl;
                }
            } else if ( fn[0] == "changeViewOption" && fn.size() == 3 ) {
                bool bOk(false), nOk(false), dOk(false);

                //parse bool value
                bool bVal(false);
                if ( fn[2].toLower() == "true" ) { bOk = true; bVal = true; }
                if ( fn[2].toLower() == "false" ) { bOk = true; bVal = false; }
                if ( fn[2] == "1" ) { bOk = true; bVal = true; }
                if ( fn[2] == "0" ) { bOk = true; bVal = false; }

                //parse int value
                int nVal = fn[2].toInt( &nOk );

                //parse double value
                double dVal = fn[2].toDouble( &dOk );

                // FIXME: REGRESSION
//                if ( fn[1] == "FOVName"                ) { Options::setFOVName(       fn[2] ); cmdCount++; }
//                if ( fn[1] == "FOVSizeX"         && dOk ) { Options::setFOVSizeX( (float)dVal ); cmdCount++; }
//                if ( fn[1] == "FOVSizeY"         && dOk ) { Options::setFOVSizeY( (float)dVal ); cmdCount++; }
//                if ( fn[1] == "FOVShape"        && nOk ) { Options::setFOVShape(       nVal ); cmdCount++; }
//                if ( fn[1] == "FOVColor"               ) { Options::setFOVColor(      fn[2] ); cmdCount++; }
                if ( fn[1] == "ShowStars"         && bOk ) { Options::setShowStars(    bVal ); cmdCount++; }
                if ( fn[1] == "ShowMessier"        && bOk ) { Options::setShowMessier( bVal ); cmdCount++; }
                if ( fn[1] == "ShowMessierImages"  && bOk ) { Options::setShowMessierImages( bVal ); cmdCount++; }
                if ( fn[1] == "ShowCLines"      && bOk ) { Options::setShowCLines(   bVal ); cmdCount++; }
                if ( fn[1] == "ShowCNames"      && bOk ) { Options::setShowCNames(   bVal ); cmdCount++; }
                if ( fn[1] == "ShowNGC"         && bOk ) { Options::setShowNGC(      bVal ); cmdCount++; }
                if ( fn[1] == "ShowIC"          && bOk ) { Options::setShowIC(       bVal ); cmdCount++; }
                if ( fn[1] == "ShowMilkyWay"    && bOk ) { Options::setShowMilkyWay( bVal ); cmdCount++; }
                if ( fn[1] == "ShowGrid"        && bOk ) { Options::setShowGrid(     bVal ); cmdCount++; }
                if ( fn[1] == "ShowEquator"     && bOk ) { Options::setShowEquator(  bVal ); cmdCount++; }
                if ( fn[1] == "ShowEcliptic"    && bOk ) { Options::setShowEcliptic( bVal ); cmdCount++; }
                if ( fn[1] == "ShowHorizon"     && bOk ) { Options::setShowHorizon(  bVal ); cmdCount++; }
                if ( fn[1] == "ShowGround"      && bOk ) { Options::setShowGround(   bVal ); cmdCount++; }
                if ( fn[1] == "ShowSun"         && bOk ) { Options::setShowSun(      bVal ); cmdCount++; }
                if ( fn[1] == "ShowMoon"        && bOk ) { Options::setShowMoon(     bVal ); cmdCount++; }
                if ( fn[1] == "ShowMercury"     && bOk ) { Options::setShowMercury(  bVal ); cmdCount++; }
                if ( fn[1] == "ShowVenus"       && bOk ) { Options::setShowVenus(    bVal ); cmdCount++; }
                if ( fn[1] == "ShowMars"        && bOk ) { Options::setShowMars(     bVal ); cmdCount++; }
                if ( fn[1] == "ShowJupiter"     && bOk ) { Options::setShowJupiter(  bVal ); cmdCount++; }
                if ( fn[1] == "ShowSaturn"      && bOk ) { Options::setShowSaturn(   bVal ); cmdCount++; }
                if ( fn[1] == "ShowUranus"      && bOk ) { Options::setShowUranus(   bVal ); cmdCount++; }
                if ( fn[1] == "ShowNeptune"     && bOk ) { Options::setShowNeptune(  bVal ); cmdCount++; }
                if ( fn[1] == "ShowPluto"       && bOk ) { Options::setShowPluto(    bVal ); cmdCount++; }
                if ( fn[1] == "ShowAsteroids"   && bOk ) { Options::setShowAsteroids( bVal ); cmdCount++; }
                if ( fn[1] == "ShowComets"      && bOk ) { Options::setShowComets(   bVal ); cmdCount++; }
                if ( fn[1] == "ShowSolarSystem" && bOk ) { Options::setShowSolarSystem( bVal ); cmdCount++; }
                if ( fn[1] == "ShowDeepSky"     && bOk ) { Options::setShowDeepSky(  bVal ); cmdCount++; }
                if ( fn[1] == "ShowStarNames"      && bOk ) { Options::setShowStarNames(      bVal ); cmdCount++; }
                if ( fn[1] == "ShowStarMagnitudes" && bOk ) { Options::setShowStarMagnitudes( bVal ); cmdCount++; }
                if ( fn[1] == "ShowAsteroidNames"  && bOk ) { Options::setShowAsteroidNames(  bVal ); cmdCount++; }
                if ( fn[1] == "ShowCometNames"     && bOk ) { Options::setShowCometNames(     bVal ); cmdCount++; }
                if ( fn[1] == "ShowPlanetNames"    && bOk ) { Options::setShowPlanetNames(    bVal ); cmdCount++; }
                if ( fn[1] == "ShowPlanetImages"   && bOk ) { Options::setShowPlanetImages(   bVal ); cmdCount++; }

                if ( fn[1] == "UseAltAz"         && bOk ) { Options::setUseAltAz(      bVal ); cmdCount++; }
                if ( fn[1] == "UseRefraction"    && bOk ) { Options::setUseRefraction( bVal ); cmdCount++; }
                if ( fn[1] == "UseAutoLabel"     && bOk ) { Options::setUseAutoLabel(  bVal ); cmdCount++; }
                if ( fn[1] == "UseAutoTrail"     && bOk ) { Options::setUseAutoTrail(  bVal ); cmdCount++; }
                if ( fn[1] == "UseAnimatedSlewing"   && bOk ) { Options::setUseAnimatedSlewing( bVal ); cmdCount++; }
                if ( fn[1] == "FadePlanetTrails" && bOk ) { Options::setFadePlanetTrails( bVal ); cmdCount++; }
                if ( fn[1] == "SlewTimeScale"    && dOk ) { Options::setSlewTimeScale(    dVal ); cmdCount++; }
                if ( fn[1] == "ZoomFactor"       && dOk ) { Options::setZoomFactor(       dVal ); cmdCount++; }
                //                if ( fn[1] == "MagLimitDrawStar"     && dOk ) { Options::setMagLimitDrawStar( dVal ); cmdCount++; }
                if ( fn[1] == "StarDensity"         && dOk ) { Options::setStarDensity( dVal ); cmdCount++; }
                //                if ( fn[1] == "MagLimitDrawStarZoomOut" && dOk ) { Options::setMagLimitDrawStarZoomOut( dVal ); cmdCount++; }
                if ( fn[1] == "MagLimitDrawDeepSky"     && dOk ) { Options::setMagLimitDrawDeepSky( dVal ); cmdCount++; }
                if ( fn[1] == "MagLimitDrawDeepSkyZoomOut" && dOk ) { Options::setMagLimitDrawDeepSkyZoomOut( dVal ); cmdCount++; }
                if ( fn[1] == "StarLabelDensity" && dOk ) { Options::setStarLabelDensity( dVal ); cmdCount++; }
                if ( fn[1] == "MagLimitHideStar"     && dOk ) { Options::setMagLimitHideStar(     dVal ); cmdCount++; }
                if ( fn[1] == "MagLimitAsteroid"     && dOk ) { Options::setMagLimitAsteroid(     dVal ); cmdCount++; }
                if ( fn[1] == "AsteroidLabelDensity" && dOk ) { Options::setAsteroidLabelDensity( dVal ); cmdCount++; }
                if ( fn[1] == "MaxRadCometName"      && dOk ) { Options::setMaxRadCometName(      dVal ); cmdCount++; }

                //these three are a "radio group"
                if ( fn[1] == "UseLatinConstellationNames" && bOk ) {
                    Options::setUseLatinConstellNames( true );
                    Options::setUseLocalConstellNames( false );
                    Options::setUseAbbrevConstellNames( false );
                    cmdCount++;
                }
                if ( fn[1] == "UseLocalConstellationNames" && bOk ) {
                    Options::setUseLatinConstellNames( false );
                    Options::setUseLocalConstellNames( true );
                    Options::setUseAbbrevConstellNames( false );
                    cmdCount++;
                }
                if ( fn[1] == "UseAbbrevConstellationNames" && bOk ) {
                    Options::setUseLatinConstellNames( false );
                    Options::setUseLocalConstellNames( false );
                    Options::setUseAbbrevConstellNames( true );
                    cmdCount++;
                }
            } else if ( fn[0] == "setGeoLocation" && ( fn.size() == 3 || fn.size() == 4 ) ) {
                QString city( fn[1] ), province, country( fn[2] );
                province.clear();
                if ( fn.size() == 4 ) {
                    province = fn[2];
                    country = fn[3];
                }

                bool cityFound( false );
                foreach ( GeoLocation *loc, geoList ) {
                    if ( loc->translatedName() == city &&
                            ( province.isEmpty() || loc->translatedProvince() == province ) &&
                            loc->translatedCountry() == country ) {

                        cityFound = true;
                        setLocation( *loc );
                        cmdCount++;
                        break;
                    }
                }

                if ( !cityFound )
                    kWarning() << i18n( "Could not set location named %1, %2, %3" , city, province, country) ;
            }
        }
    }  //end while

    if ( cmdCount ) return true;
    return false;
}

void KStarsData::syncFOV()
{
    visibleFOVs.clear();
    // Add visible FOVs 
    foreach(FOV* fov, availFOVs) {
        if( Options::fOVNames().contains( fov->name() ) ) 
            visibleFOVs.append( fov );
    }
    // Remove unavailable FOVs
    QSet<QString> names = QSet<QString>::fromList( Options::fOVNames() );
    QSet<QString> all;
    foreach(FOV* fov, visibleFOVs) {
        all.insert(fov->name());
    }
    Options::setFOVNames( all.intersect(names).toList() );
}

/*void KStarsData::appendTelescopeObject(SkyObject * object)
{
  INDITelescopeList.append(object);
}*/

void KStarsData::saveTimeBoxShaded( bool b ) { Options::setShadeTimeBox( b ); }
void KStarsData::saveGeoBoxShaded( bool b ) { Options::setShadeGeoBox( b ); }
void KStarsData::saveFocusBoxShaded( bool b ) { Options::setShadeFocusBox( b ); }
void KStarsData::saveTimeBoxPos( QPoint p ) { Options::setPositionTimeBox( p ); }
void KStarsData::saveGeoBoxPos( QPoint p ) { Options::setPositionGeoBox( p ); }
void KStarsData::saveFocusBoxPos( QPoint p ) { Options::setPositionFocusBox( p ); }

#include "kstarsdata.moc"
