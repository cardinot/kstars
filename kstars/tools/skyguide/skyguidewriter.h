/***************************************************************************
                          skyguidewriter.h  -  K Desktop Planetarium
                             -------------------
    begin                : 2015/07/28
    copyright            : (C) 2015 by Marcos Cardinot
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

#ifndef SKYGUIDEWRITER_H
#define SKYGUIDEWRITER_H

#include <QDialog>

#include "skyguideobject.h"

class SkyGuideMgr;
class Ui_SkyGuideAuthor;
class Ui_SkyGuideSlide;
class Ui_SkyGuideWriter;

/**
 * @class SkyGuideWriter
 * @short Implements a dialog that enable users to create and edit SkyGuides.
 * @author Marcos Cardinot <mcardinot@gmail.com>
 */
class SkyGuideWriter : public QDialog
{
    Q_OBJECT

public:
    SkyGuideWriter(SkyGuideMgr *mgr, QWidget *parent=0);
    ~SkyGuideWriter();

private slots:
    void slotNew();
    void slotOpen();
    void slotSave();
    void slotSaveAs();
    void slotInstall();

    void slotShowAuthorDlg();
    void slotAddAuthor();
    void slotEditAuthor(QModelIndex idx);
    void slotRemoveAuthor();
    void slotAuthorsMoved(const QModelIndex&, int src, int, const QModelIndex&, int dst);

    void slotShowSlideDlg();
    void slotAddSlide();
    void slotEditSlide(QModelIndex idx);
    void slotRemoveSlide();
    void slotGetImagePath();
    void slotSlidesMoved(const QModelIndex&, int src, int, const QModelIndex&, int dst);

    void slotUpdateButtons();

    void slotFieldsChanged();

private:
    QDialog* m_authorDlg;
    QDialog* m_slideDlg;

    Ui_SkyGuideWriter* m_ui;
    Ui_SkyGuideAuthor* m_uiAuthor;
    Ui_SkyGuideSlide* m_uiSlide;

    SkyGuideMgr* m_skyGuideMgr;
    SkyGuideObject* m_skyGuideObject;
    bool m_unsavedChanges;
    QString m_currentFilePath;
    int m_isEditingAuthorIdx;
    int m_isEditingSlideIdx;

    void populateFields();
    void setUnsavedChanges(bool b);
    void blockSignals(bool b);

    void saveWarning();
    bool checkRequiredFieldsWarning();
};

#endif // SKYGUIDEWRITER_H
