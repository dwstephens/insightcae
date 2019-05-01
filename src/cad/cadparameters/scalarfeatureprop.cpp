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

#include "scalarfeatureprop.h"
#include "cadfeature.h"
#include "geotest.h"


insight::cad::ScalarFeatureProp::ScalarFeatureProp
(
  insight::cad::FeaturePtr model, 
  const std::string& name
)
: model_(model),
  name_(name)
{}

double insight::cad::ScalarFeatureProp::value() const
{
  return model_->getDatumScalar(name_);
}


insight::cad::FeatureVolume::FeatureVolume(insight::cad::FeaturePtr model)
: model_(model)
{}


double insight::cad::FeatureVolume::value() const
{
  return model_->modelVolume();
}

insight::cad::CumulativeEdgeLength::CumulativeEdgeLength(insight::cad::FeaturePtr model)
: model_(model)
{}

double insight::cad::CumulativeEdgeLength::value() const
{
    double L=0;
    FeatureSetData ae = model_->allEdgesSet();
    for (const FeatureID& i: ae)
    {
        L+=edgeLength(model_->edge(i));
    }
    return L;
}
