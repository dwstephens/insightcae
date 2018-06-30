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

#include "fillet.h"
#include "base/boost_include.h"
#include <boost/spirit/include/qi.hpp>
namespace qi = boost::spirit::qi;
namespace repo = boost::spirit::repository;
namespace phx   = boost::phoenix;

using namespace std;
using namespace boost;

namespace insight {
namespace cad {


    
    
defineType(Fillet);
addToFactoryTable(Feature, Fillet);


size_t Fillet::calcHash() const
{
  ParameterListHash h;
  h+=*edges_;
  h+=r_->value();
  return h.getHash();
}


Fillet::Fillet(): DerivedFeature()
{}

  
  
  
  
Fillet::Fillet(FeatureSetPtr edges, ScalarPtr r)
: DerivedFeature(edges->model()), edges_(edges), r_(r)
{}




FeaturePtr Fillet::create(FeatureSetPtr edges, ScalarPtr r)
{
    return FeaturePtr(new Fillet(edges, r));
}



void Fillet::build()
{
    const Feature& m1=* ( edges_->model() );
    m1.unsetLeaf();
    BRepFilletAPI_MakeFillet fb ( m1 );
    BOOST_FOREACH ( FeatureID f, edges_->data() ) {
        fb.Add ( r_->value(), m1.edge ( f ) );
    }
    fb.Build();
    setShape ( fb.Shape() );
}



/*! \page Fillet Fillet
  * Create a fillet on an edge.
  * 
  * Syntax:
  * ~~~~
  * Fillet(<edge feature set: edges>, <scalar: radius>) : feature
  * ~~~~
  */
void Fillet::insertrule(parser::ISCADParser& ruleset) const
{
  ruleset.modelstepFunctionRules.add
  (
    "Fillet",	
    typename parser::ISCADParser::ModelstepRulePtr(new typename parser::ISCADParser::ModelstepRule( 

    ( '(' >> ruleset.r_edgeFeaturesExpression >> ',' >> ruleset.r_scalarExpression >> ')' ) 
      [ qi::_val = phx::bind(&Fillet::create, qi::_1, qi::_2) ]
      
    ))
  );
}




FeatureCmdInfoList Fillet::ruleDocumentation() const
{
    return boost::assign::list_of
    (
        FeatureCmdInfo
        (
            "Fillet",
         
            "( <edgesSelection:edges>, <scalar:r> )",
         
            "Creates fillets at selected edges of a solid. All edges in the selection set edges are rounded with width r."
        )
    );
}



}
}
