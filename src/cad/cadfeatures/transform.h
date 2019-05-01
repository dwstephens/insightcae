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

#ifndef INSIGHT_CAD_TRANSFORM_H
#define INSIGHT_CAD_TRANSFORM_H

#include "cadparameters.h"
#include "derivedfeature.h"

namespace insight 
{
namespace cad 
{


    
    
class Transform
    : public DerivedFeature
{

    FeaturePtr m1_;
    VectorPtr trans_;
    VectorPtr rotorg_;
    VectorPtr rot_;
    ScalarPtr sf_;
    FeaturePtr other_;

    std::shared_ptr<gp_Trsf> trsf_;

    Transform ( FeaturePtr m1, VectorPtr trans, VectorPtr rot, ScalarPtr sf );
    Transform ( FeaturePtr m1, VectorPtr rot, VectorPtr rotorg );
    Transform ( FeaturePtr m1, VectorPtr trans );
    Transform ( FeaturePtr m1, ScalarPtr scale );
    Transform ( FeaturePtr m1, FeaturePtr other );

    virtual size_t calcHash() const;
    virtual void build();

public:
    declareType ( "Transform" );

    Transform ();
    Transform ( FeaturePtr m1, const gp_Trsf& trsf );

    static FeaturePtr create ( FeaturePtr m1, VectorPtr trans, VectorPtr rot, ScalarPtr sf );
    static FeaturePtr create_rotate ( FeaturePtr m1, VectorPtr rot, VectorPtr rotorg );
    static FeaturePtr create_translate ( FeaturePtr m1, VectorPtr trans );
    static FeaturePtr create_scale ( FeaturePtr m1, ScalarPtr scale );
    static FeaturePtr create_copy ( FeaturePtr m1, FeaturePtr other );


    virtual void insertrule ( parser::ISCADParser& ruleset ) const;
    virtual FeatureCmdInfoList ruleDocumentation() const;

    virtual bool isTransformationFeature() const;
    virtual gp_Trsf transformation() const;

    static gp_Trsf calcTrsfFromOtherTransformFeature(FeaturePtr other);
};




}
}

#endif // INSIGHT_CAD_TRANSFORM_H
