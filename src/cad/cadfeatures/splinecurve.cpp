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

#include "splinecurve.h"
#include "base/boost_include.h"
#include <boost/spirit/include/qi.hpp>
#include <boost/phoenix/fusion.hpp>

#include "TColgp_HArray1OfPnt.hxx"
#include "GeomAPI_Interpolate.hxx"

namespace qi = boost::spirit::qi;
namespace repo = boost::spirit::repository;
namespace phx   = boost::phoenix;

using namespace std;
using namespace boost;


namespace insight {
namespace cad {

    
    
    
defineType(SplineCurve);
addToFactoryTable(Feature, SplineCurve);



size_t SplineCurve::calcHash() const
{
  ParameterListHash h;
  h+=this->type();
  for (const VectorPtr& p: pts_)
  {
      h+=p->value();
  }
  if (tan0_) h+=tan0_->value();
  if (tan1_) h+=tan1_->value();
  return h.getHash();
}



SplineCurve::SplineCurve(): Feature()
{}




SplineCurve::SplineCurve(const std::vector<VectorPtr>& pts, VectorPtr tan0, VectorPtr tan1)
: pts_(pts), tan0_(tan0), tan1_(tan1)
{}




FeaturePtr SplineCurve::create(const std::vector<VectorPtr>& pts, VectorPtr tan0, VectorPtr tan1)
{
    return FeaturePtr(new SplineCurve(pts, tan0, tan1));
}




void SplineCurve::build()
{
//     TColgp_Array1OfPnt pts_col ( 1, pts_.size() );
    Handle_TColgp_HArray1OfPnt pts_col = new TColgp_HArray1OfPnt( 1, pts_.size() );
    for ( int j=0; j<pts_.size(); j++ ) {
        arma::mat pi=pts_[j]->value();
        pts_col->SetValue ( j+1, to_Pnt ( pi ) );
        refpoints_[str(format("p%d")%j)]=pi;
    }
//     GeomAPI_PointsToBSpline splbuilder ( pts_col );
    GeomAPI_Interpolate splbuilder ( pts_col, false, 1e-6 );
    if (tan0_ && tan1_)
    {
        splbuilder.Load(to_Vec(tan0_->value()), to_Vec(tan1_->value()));
    }
    splbuilder.Perform();
    Handle_Geom_BSplineCurve crv=splbuilder.Curve();
    setShape ( BRepBuilderAPI_MakeEdge ( crv, crv->FirstParameter(), crv->LastParameter() ) );
}




void SplineCurve::insertrule(parser::ISCADParser& ruleset) const
{
  ruleset.modelstepFunctionRules.add
  (
    "SplineCurve",	
    typename parser::ISCADParser::ModelstepRulePtr(new typename parser::ISCADParser::ModelstepRule( 

    ( '(' 
        > ruleset.r_vectorExpression % ',' 
        >> ( (',' >> qi::lit("der") >> ruleset.r_vectorExpression >> ruleset.r_vectorExpression ) | ( qi::attr(VectorPtr()) >> qi::attr(VectorPtr()) ) ) 
        >> ')' ) 
	[ qi::_val = phx::bind(&SplineCurve::create, qi::_1, phx::at_c<0>(qi::_2), phx::at_c<1>(qi::_2) ) ]
      
    ))
  );
}




FeatureCmdInfoList SplineCurve::ruleDocumentation() const
{
    return boost::assign::list_of
    (
        FeatureCmdInfo
        (
            "SplineCurve",
            "( <vector:p0>, ..., <vector:pn> )",
            "Creates a spline curve through all given points p0 to pn."
        )
    );
}



}
}
