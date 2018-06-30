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

#include "cutaway.h"
#include "quad.h"
#include "datum.h"

#include "booleanintersection.h"
#include "base/boost_include.h"
#include <boost/spirit/include/qi.hpp>
#include "base/tools.h"

namespace qi = boost::spirit::qi;
namespace repo = boost::spirit::repository;
namespace phx   = boost::phoenix;

using namespace std;
using namespace boost;

namespace insight {
namespace cad {


    
    
defineType(Cutaway);
addToFactoryTable(Feature, Cutaway);


size_t Cutaway::calcHash() const
{
  ParameterListHash h;
  h+=this->type();
  h+=*model_;
  if (p0_ && n_)
    {
      h+=p0_->value();
      h+=n_->value();
    }
  else
    {
      h+=*pl_;
      h+=inverted_;
    }
  return h.getHash();
}


Cutaway::Cutaway(): DerivedFeature()
{}





Cutaway::Cutaway(FeaturePtr model, VectorPtr p0, VectorPtr n)
: DerivedFeature(model), model_(model), p0_(p0), n_(n)
{
}




Cutaway::Cutaway(FeaturePtr model, ConstDatumPtr pl, bool inverted)
: DerivedFeature(model), model_(model), pl_(pl), inverted_(inverted)
{
}




FeaturePtr Cutaway::create ( FeaturePtr model, VectorPtr p0, VectorPtr n )
{
    return FeaturePtr(new Cutaway(model, p0, n));
}




FeaturePtr Cutaway::create_plane ( FeaturePtr model, ConstDatumPtr pl, bool inverted )
{
    return FeaturePtr(new Cutaway(model, pl, inverted));
}




void Cutaway::build()
{
  ExecTimer t("Cutaway::build() ["+featureSymbolName()+"]");

  if (!cache.contains(hash()))
    {

      arma::mat p0, n;

      if ( pl_ ) {
          if ( !pl_->providesPlanarReference() ) {
              throw insight::Exception ( "Cutaway: Given datum does not provide a planar reference!" );
            }
          gp_Ax3 pl=pl_->plane();

          p0=vec3 ( pl.Location() );
          n=vec3 ( pl.Direction() );
          if ( inverted_ ) {
              n*=-1.;
            }
        } else {
          if ( ( !p0_ ) || ( !n_ ) ) {
              throw insight::Exception ( "Cutaway: origin and normal direction undefined!" );
            }
          p0=p0_->value();
          n=n_->value();
        }

      arma::mat bb=model_->modelBndBox ( 0.1 );
      double L=10.*norm ( bb.col ( 1 )-bb.col ( 0 ), 2 );

      arma::mat ex=cross ( n, vec3 ( 1,0,0 ) );
      if ( norm ( ex,2 ) <1e-8 ) {
          ex=cross ( n, vec3 ( 0,1,0 ) );
        }
      ex/=norm ( ex,2 );

      arma::mat ey=cross ( n,ex );
      ey/=norm ( ey,2 );

#warning Relocate p0 in plane to somewhere nearer to model center!
      FeaturePtr q=Quad::create
          (
            matconst ( p0-0.5*L* ( ex+ey ) ),
            matconst ( L*ex ),
            matconst ( L*ey )
            );
      this->setShape ( q->shape() );
      //   std::cout<<"Airspace"<<std::endl;
      TopoDS_Shape airspace=BRepPrimAPI_MakePrism ( TopoDS::Face ( q->shape() ), to_Vec ( L*n ) );

      //   SolidModel(airspace).saveAs("airspace.stp");
      refpoints_["p0"]=p0;
      refvectors_["n"]=n;
      providedSubshapes_["AirSpace"]=FeaturePtr ( new Feature ( airspace ) );

      try {
        providedSubshapes_["CutSurface"]=
            BooleanIntersection::create
            (
              model_,
              FeaturePtr ( new Feature ( TopoDS::Face ( q->shape() ) ) )
              );
      } catch ( ... ) {
        insight::Warning ( "Could not create cutting surface!" );
      }

      try {
        this->setShape ( BRepAlgoAPI_Cut ( model_->shape(), airspace ) );
      } catch ( ... ) {
        throw insight::Exception ( "Could not create cut!" );
      }

    }
  else
    {
      this->operator=(*cache.markAsUsed<Cutaway>(hash()));
    }
}




/** @addtogroup cad_parser
  * @{
  * 
  * @section cutaway Cut away a halfspace
  * Remove everything beyond a plane from a feature
  * 
  * Syntax: 
  * ~~~~
  * Cutaway(<feature expression: model>, <vector: p0>, <vector: n>) : feature
  * ~~~~
  * @}
  */
void Cutaway::insertrule(parser::ISCADParser& ruleset) const
{
  ruleset.modelstepFunctionRules.add
  (
    "Cutaway",	
    typename parser::ISCADParser::ModelstepRulePtr(new typename parser::ISCADParser::ModelstepRule( 

    ( '(' >> (
     ( ruleset.r_solidmodel_expression >> ',' >> ruleset.r_vectorExpression >> ',' >> ruleset.r_vectorExpression )
      [ qi::_val = phx::bind(&Cutaway::create, qi::_1, qi::_2, qi::_3) ]
      |
     ( ruleset.r_solidmodel_expression >> ',' >> ruleset.r_datumExpression 
        >> ( (',' >> qi::lit("inverted") >> qi::attr(true) ) | (qi::attr(false)) ) )
      [ qi::_val = phx::bind(&Cutaway::create_plane, qi::_1, qi::_2, qi::_3) ]
     ) >> ')' )
    ))
  );
}




FeatureCmdInfoList Cutaway::ruleDocumentation() const
{
    return boost::assign::list_of
    (
        FeatureCmdInfo
        (
            "Cutaway",
         
            "( <feature:base>, ( <vector:p0>, <vector:n> ) | ( <datum:plane> [, inverted] ) )",
         
            "Removes a halfspace from a feature. The halfspace is either specified by a point p0 and the normal vector n or by a datum plane."
            " In the latter case, the removal side of plane can be toggled by giving the option inverted."
        )
    );
}




}
}
