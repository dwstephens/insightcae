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

#ifndef INSIGHT_CAD_MAXIMAL_H
#define INSIGHT_CAD_MAXIMAL_H

#include "feature.h"
#include <map>

namespace insight {
namespace cad {

class maximal
    : public Filter
{
public:
  typedef std::pair<double, FeatureID>  RankEntry;

protected:
    int rank_, lrank_;
    std::shared_ptr<scalarQuantityComputer> qtc_;
    std::vector<RankEntry> ranking_;

public:
    maximal(const scalarQuantityComputer& qtc, int rank=0, int lrank=-1);
    virtual void firstPass(FeatureID feature);
    virtual void initialize(ConstFeaturePtr m);
    virtual bool checkMatch(FeatureID feature) const;

    virtual FilterPtr clone() const;
};

}
}

#endif // INSIGHT_CAD_MAXIMAL_H
