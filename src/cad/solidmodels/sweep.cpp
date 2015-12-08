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

#include "sweep.h"
#include "base/boost_include.h"
#include <boost/spirit/include/qi.hpp>
namespace qi = boost::spirit::qi;
namespace repo = boost::spirit::repository;
namespace phx   = boost::phoenix;

using namespace std;
using namespace boost;

namespace insight {
namespace cad {

defineType(Sweep);
addToFactoryTable(SolidModel, Sweep, NoParameters);

Sweep::Sweep(const NoParameters& nop): SolidModel(nop)
{}


Sweep::Sweep(const std::vector<SolidModelPtr>& secs)
{
  if (secs.size()<2)
    throw insight::Exception("Insufficient number of sections given!");
  
  bool create_solid=false;
  {
    TopoDS_Shape cs0=*secs[0];
    if (cs0.ShapeType()==TopAbs_FACE)
      create_solid=true;
    else if (cs0.ShapeType()==TopAbs_WIRE)
    {
      create_solid=TopoDS::Wire(cs0).Closed();
    }
  }
  
  BRepOffsetAPI_ThruSections sb(create_solid);
 
  BOOST_FOREACH(const SolidModelPtr& skp, secs)
  {
    TopoDS_Wire cursec;
    TopoDS_Shape cs=*skp;
    if (cs.ShapeType()==TopAbs_FACE)
     cursec=BRepTools::OuterWire(TopoDS::Face(cs));
    else if (cs.ShapeType()==TopAbs_WIRE)
    {
     cursec=TopoDS::Wire(cs);
    }
    else if (cs.ShapeType()==TopAbs_EDGE)
    {
     BRepBuilderAPI_MakeWire w;
     w.Add(TopoDS::Edge(cs));
     cursec=w.Wire();
    }
    else
    {
      throw insight::Exception("Incompatible section shape for Sweep!");
    }
    sb.AddWire(cursec);
  }
  
  setShape(sb.Shape());
}

void Sweep::insertrule(parser::ISCADParser& ruleset) const
{
  ruleset.modelstepFunctionRules.add
  (
    "Sweep",	
    typename parser::ISCADParser::ModelstepRulePtr(new typename parser::ISCADParser::ModelstepRule( 

    ( '(' >> (ruleset.r_solidmodel_expression % ',' ) >> ')' ) 
      [ qi::_val = phx::construct<SolidModelPtr>(phx::new_<Sweep>(qi::_1)) ]
      
    ))
  );
}

}
}