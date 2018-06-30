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

#include "importsolidmodel.h"
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


    
    
defineType(Import);
addToFactoryTable(Feature, Import);


size_t Import::calcHash() const
{
  ParameterListHash h;
  h+=this->type();
  h+=filepath_;
  return h.getHash();
}



Import::Import()
: Feature()
{}




Import::Import(const filesystem::path& filepath/*, ScalarPtr scale*/)
: filepath_(filepath)/*,
  scale_(scale)*/
{
}




FeaturePtr Import::create ( const boost::filesystem::path& filepath/*, ScalarPtr scale=ScalarPtr()*/ )
{
    return FeaturePtr(new Import(filepath));
}




void Import::build()
{
  ExecTimer t("Import::build() ["+featureSymbolName()+"]");

  if (!cache.contains(hash()))
  {
    boost::filesystem::path fp = filepath_;
    if (!boost::filesystem::exists(fp))
    {
      fp=sharedModelFilePath(filepath_.string());
      if (!boost::filesystem::exists(fp))
      {
        throw insight::Exception("File not found: "+filepath_.string());
      }
    }
    loadShapeFromFile(fp);

    cache.insert(shared_from_this());
  }
  else
  {
      this->operator=(*cache.markAsUsed<Import>(hash()));
  }

//   if (scale_)
//   {
//     gp_Trsf tr0;
//     tr0.SetScaleFactor(scale_->value());
//     s=BRepBuilderAPI_Transform(s, tr0).Shape();
//   }
//   setShape(s);
//   setShapeHash(); // not possible to use in build...
}




void Import::insertrule(parser::ISCADParser& ruleset) const
{
  ruleset.modelstepFunctionRules.add
  (
    "import",	
    typename parser::ISCADParser::ModelstepRulePtr(new typename parser::ISCADParser::ModelstepRule( 
      ( '(' >> 
	ruleset.r_path 
// 	>> (( ',' >> ruleset.r_scalarExpression ) | ( qi::attr(ScalarPtr(new ConstantScalar(1.0))) ))
	>> ')' ) [ qi::_val = phx::bind(&Import::create, qi::_1/*, qi::_2*/) ]
    ))
  );
}




FeatureCmdInfoList Import::ruleDocumentation() const
{
    return boost::assign::list_of
    (
        FeatureCmdInfo
        (
            "import",
         
            "( <path> )",
         
            "Imports a feature from a file. The format is recognized from the filename extension. Supported formats are IGS, STP, BREP."
        )
    );
}



}
}
