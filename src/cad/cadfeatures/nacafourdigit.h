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

#ifndef INSIGHT_CAD_NACAFOURDIGIT_H
#define INSIGHT_CAD_NACAFOURDIGIT_H

#include "cadfeature.h"

namespace insight {
namespace cad {

    
    

class NacaFourDigit
    : public SingleFaceFeature
{
    std::string code_;
    // alt:
    ScalarPtr tc_, m_, p_;

    VectorPtr p0_;
    VectorPtr ez_;
    VectorPtr ex_;
    ScalarPtr tofs_, clipte_;

    NacaFourDigit 
    ( 
        const std::string& code, VectorPtr p0, VectorPtr ex, VectorPtr ez, 
        ScalarPtr tofs=scalarconst(0), 
        ScalarPtr clipte=scalarconst(0) 
    );
    NacaFourDigit
    (
        ScalarPtr tc, ScalarPtr m, ScalarPtr p,
        VectorPtr p0, VectorPtr ex, VectorPtr ez,
        ScalarPtr tofs=scalarconst(0),
        ScalarPtr clipte=scalarconst(0)
    );

    virtual size_t calcHash() const;
    virtual void build();

public:
    declareType ( "Naca4" );
    NacaFourDigit ();

    static FeaturePtr create 
    ( 
        const std::string& code, VectorPtr p0, VectorPtr ex, VectorPtr ez, 
        ScalarPtr tofs=scalarconst(0),
        ScalarPtr clipte=scalarconst(0) 
    );
    static FeaturePtr create_values
    (
        ScalarPtr tc, ScalarPtr m, ScalarPtr p,
        VectorPtr p0, VectorPtr ex, VectorPtr ez,
        ScalarPtr tofs=scalarconst(0),
        ScalarPtr clipte=scalarconst(0)
    );

    void calcProfile(double xc, double tc, double m, double p, double& t, double& yc, double& dycdx) const;

    operator const TopoDS_Face& () const;


    virtual void insertrule ( parser::ISCADParser& ruleset ) const;
    virtual FeatureCmdInfoList ruleDocumentation() const;
};




}
}

#endif // INSIGHT_CAD_NACAFOURDIGIT_H
