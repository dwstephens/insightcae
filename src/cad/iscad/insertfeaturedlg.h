/*
 * This file is part of Insight CAE, a workbench for Computer-Aided Engineering
 * Copyright (C) 2014  Hannes Kroeger <hannes@kroegeronline.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */

#ifndef INSERTFEATUREDLG_H
#define INSERTFEATUREDLG_H

#include "cadtypes.h"

#include <QDialog>
#include <QListWidgetItem>

#include "helpwidget.h"

namespace Ui
{
class InsertFeatureDlg;
}

class InsertFeatureDlg 
: public QDialog
{
    Q_OBJECT
    
public:
    QString insert_string_;
    HelpWidget* featureCmdHelp_;

    InsertFeatureDlg(QWidget* parent);
    
protected slots:
    void onItemSelectionChanged ();
    
   virtual void accept();
   
   virtual void onIsIntermediateStepActivated();
   virtual void onIsFinalComponentActivated();
   virtual void onOnlyFeatureCommandActivated();

private:
    Ui::InsertFeatureDlg* ui;
};

#endif // INSERTFEATUREDLG_H
