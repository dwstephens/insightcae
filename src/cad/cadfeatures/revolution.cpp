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

#include "revolution.h"
#include "base/boost_include.h"
#include <boost/spirit/include/qi.hpp>
namespace qi = boost::spirit::qi;
namespace repo = boost::spirit::repository;
namespace phx   = boost::phoenix;

using namespace std;
using namespace boost;

namespace insight {
namespace cad {

    
    
  
defineType(Revolution);
addToFactoryTable(Feature, Revolution);


size_t Revolution::calcHash() const
{
  ParameterListHash h;
  h+=this->type();
  h+=*sk_;
  h+=p0_->value();
  h+=axis_->value();
  h+=angle_->value();
  h+=centered_;
  return h.getHash();
}



Revolution::Revolution(): Feature()
{}




Revolution::Revolution(FeaturePtr sk, VectorPtr p0, VectorPtr axis, ScalarPtr angle, bool centered)
: sk_(sk), p0_(p0), axis_(axis), angle_(angle), centered_(centered)
{}




FeaturePtr Revolution::create(FeaturePtr sk, VectorPtr p0, VectorPtr axis, ScalarPtr angle, bool centered)
{
    return FeaturePtr(new Revolution(sk, p0, axis, angle, centered));
}




void Revolution::build()
{
    if ( !centered_ ) {
        BRepPrimAPI_MakeRevol mkr ( *sk_, gp_Ax1 ( to_Pnt ( p0_->value() ), gp_Dir ( to_Vec ( axis_->value() ) ) ), angle_->value(), centered_ );
        providedSubshapes_["frontFace"]=FeaturePtr ( new Feature ( mkr.FirstShape() ) );
        providedSubshapes_["backFace"]=FeaturePtr ( new Feature ( mkr.LastShape() ) );
        setShape ( mkr.Shape() );
    } else {
        gp_Trsf trsf;
        gp_Vec ax=to_Vec ( axis_->value() );
        ax.Normalize();
        trsf.SetRotation ( gp_Ax1 ( to_Pnt ( p0_->value() ), ax ), -0.5*angle_->value() );
        BRepPrimAPI_MakeRevol mkr
        (
            BRepBuilderAPI_Transform ( *sk_, trsf ).Shape(),
            gp_Ax1 ( to_Pnt ( p0_->value() ), gp_Dir ( ax ) ), angle_->value()
        );
        providedSubshapes_["frontFace"]=FeaturePtr ( new Feature ( mkr.FirstShape() ) );
        providedSubshapes_["backFace"]=FeaturePtr ( new Feature ( mkr.LastShape() ) );
        setShape ( mkr.Shape() );
    }

    copyDatums ( *sk_ );
}




void Revolution::insertrule ( parser::ISCADParser& ruleset ) const
{
    ruleset.modelstepFunctionRules.add
    (
        "Revolution",
        typename parser::ISCADParser::ModelstepRulePtr ( new typename parser::ISCADParser::ModelstepRule (

                    ( '(' 
                        >> ruleset.r_solidmodel_expression >> ',' 
                        >> ruleset.r_vectorExpression >> ','
                        >> ruleset.r_vectorExpression >> ',' 
                        >> ruleset.r_scalarExpression
                        >> ( ( ',' >> qi::lit ( "centered" ) >> qi::attr ( true ) ) | qi::attr ( false ) )
                        >> ')' )
                    [ qi::_val = phx::bind ( &Revolution::create, qi::_1, qi::_2, qi::_3, qi::_4, qi::_5 ) ]

                ) )
    );
}




FeatureCmdInfoList Revolution::ruleDocumentation() const
{
    return boost::assign::list_of
    (
        FeatureCmdInfo
        (
            "Revolution",
            "( <feature:xsec>, <vector:p0>, <vector:axis>, <scalar:angle> [, centered] )",
            "Creates a revolution of the planar feature xsec."
            " The rotation axis is specified by origin point p0 and the direction vector axis."
            " Revolution angle is specified as angle. By giving the keyword centered, the revolution is created symmetrically around the base feature."
        )
    );
}



}
}
