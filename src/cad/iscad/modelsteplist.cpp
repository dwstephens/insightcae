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
 */

#include "modelsteplist.h"
#include "qmodelstepitem.h"

ModelStepList::ModelStepList(QWidget* parent)
: QListWidget(parent)
{
  setContextMenuPolicy(Qt::CustomContextMenu);
  connect
  (
    this,
    SIGNAL(customContextMenuRequested(const QPoint &)),
    this,
    SLOT(showContextMenuForWidget(const QPoint &))
  );
}

void ModelStepList::showContextMenuForWidget(const QPoint &p)
{
  QModelStepItem * mi=dynamic_cast<QModelStepItem*>(itemAt(p));
  if (mi)
  {
    mi->showContextMenu(this->mapToGlobal(p));
  }
}