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

#include "nacafourdigit.h"

#include "base/boost_include.h"
#include <boost/spirit/include/qi.hpp>
namespace qi = boost::spirit::qi;
namespace repo = boost::spirit::repository;
namespace phx   = boost::phoenix;

using namespace std;
using namespace boost;

namespace insight {
namespace cad {


    

defineType(NacaFourDigit);
addToFactoryTable(Feature, NacaFourDigit);


size_t NacaFourDigit::calcHash() const
{
  ParameterListHash h;
  h+=this->type();
  if (tc_)
  {
      h+=tc_->value();
      h+=m_->value();
      h+=p_->value();
  }
  else
  {
    h+=code_;
  }
  h+=p0_->value();
  h+=ez_->value();
  h+=ex_->value();
  h+=tofs_->value();
  h+=clipte_->value();
  return h.getHash();
}



NacaFourDigit::NacaFourDigit()
{}




NacaFourDigit::NacaFourDigit
(
    const std::string& code, VectorPtr p0, VectorPtr ex, VectorPtr ez, 
    ScalarPtr tofs, ScalarPtr clipte 
)
: code_(code), p0_(p0), ez_(ez), ex_(ex), tofs_(tofs), clipte_(clipte)
{}

NacaFourDigit::NacaFourDigit
(
    ScalarPtr tc, ScalarPtr m, ScalarPtr p,
    VectorPtr p0, VectorPtr ex, VectorPtr ez,
    ScalarPtr tofs,
    ScalarPtr clipte
)
: tc_(tc), m_(m), p_(p),
  p0_(p0), ez_(ez), ex_(ex), tofs_(tofs), clipte_(clipte)
{}

FeaturePtr NacaFourDigit::create
(
    const std::string& code, VectorPtr p0, VectorPtr ex, VectorPtr ez, 
    ScalarPtr tofs, ScalarPtr clipte
)
{
    return FeaturePtr(new NacaFourDigit(code, p0, ex, ez, tofs, clipte));
}

FeaturePtr NacaFourDigit::create_values
(
    ScalarPtr tc, ScalarPtr m, ScalarPtr p,
    VectorPtr p0, VectorPtr ex, VectorPtr ez,
    ScalarPtr tofs,
    ScalarPtr clipte
)
{
    return FeaturePtr(new NacaFourDigit(tc, m, p, p0, ex, ez, tofs, clipte));
}



void NacaFourDigit::calcProfile(double xc, double tc, double m, double p, double& t, double& yc, double& dycdx) const
{
    t=5.*tc*(0.2969*::sqrt(xc)-0.1260*xc-0.3516*::pow(xc,2)+0.2843*::pow(xc,3)-0.1015*::pow(xc,4));

    yc=0.;
    dycdx=0.;
    if (fabs(m)>1e-10)
    {
      if (xc<p)
      {
        yc=m*xc/p/p * (2.*p - xc);
        dycdx=2*m/p/p * (p - xc);
      }
      else
      {
        yc=m*(1.-xc)/::pow(1-p,2) * (1.+xc-2*p);
        dycdx=2*m/::pow(1-p,2) * (p - xc);
      }
    }
}

void NacaFourDigit::build()
{
  double 
    tc=0.0, // thickness
    m=0.0, // max camber
    p=0.0; // max chamber location
    
  double tofs=tofs_->value();
  double clipte=clipte_->value();
    
  if (tc_)
  {
      tc=tc_->value();
      m=m_->value();
      p=p_->value();
  }
  else
  {
      if (code_.size()!=4)
      {
        throw insight::Exception("Invalid NACA code! (was "+code_+")");
      }
      else
      {
        int t100=atoi(code_.substr(2, 2).c_str());
        tc=double(t100)/100.;
        int m100=atoi(code_.substr(0, 1).c_str());
        m=double(m100)/100.;
        int p10=atoi(code_.substr(1, 1).c_str());
        p=double(p10)/10.;
      }
  }

  arma::mat ex=ex_->value();
  arma::mat ez=ez_->value();
  
  double L=arma::norm(ex,2);
  ex/=L;
  ez/=arma::norm(ez,2);
  arma::mat ey=arma::cross(ez, ex);
  
//   tc *= 1. - 2.*tofs/(tc*L);
  L -= 2.0*tofs;
  
  
//   cout<<"L="<<L<<", tc="<<tc<<endl;
  
  // create support points
  // ===================================================
  int np=25;
  TColgp_Array1OfPnt pts_up(1, np), pts_lo(1, np);

  int ee=np-1;
  if (clipte>0.) ee=np;
  for (int j=0; j<ee; j++) 
  {
    double xc=0.5*(1.-::cos(M_PI*double(j) / double(np-1))) * (L-clipte)/L;
    
    double t=0.0, yc=0.0, dycdx=0.0;
    calcProfile(xc, tc, m, p, t, yc, dycdx);
    t *= std::max(0.1, 1. - 2.*tofs/(t*L));    
    double theta=::atan(dycdx);
    pts_up.SetValue
    (
        j+1, 
        to_Pnt( p0_->value() +tofs*ex +(xc -t*::sin(theta))*L*ex + (yc +t*::cos(theta))*L*ey )
    );
    pts_lo.SetValue
    (
        j+1, 
        to_Pnt( p0_->value() +tofs*ex +(xc +t*::sin(theta))*L*ex + (yc -t*::cos(theta))*L*ey )
    );
  }
  if (clipte==0.)
  {
    pts_up.SetValue(np, to_Pnt( p0_->value() +tofs*ex +L*ex ) );
    pts_lo.SetValue(np, to_Pnt( p0_->value() +tofs*ex +L*ex ) );
  }  
  
  // build splines from points
  // ===================================================
  GeomAPI_PointsToBSpline splbuilderup(pts_up);
  Handle_Geom_BSplineCurve crvup=splbuilderup.Curve();
  TopoDS_Edge eup=BRepBuilderAPI_MakeEdge(crvup, crvup->FirstParameter(), crvup->LastParameter());

  GeomAPI_PointsToBSpline splbuilderlo(pts_lo);
  Handle_Geom_BSplineCurve crvlo=splbuilderlo.Curve();
  TopoDS_Edge elo=BRepBuilderAPI_MakeEdge(crvlo, crvlo->FirstParameter(), crvlo->LastParameter());
  
  
  BRepBuilderAPI_MakeWire w;
  w.Add(eup);
  w.Add(elo);
  
  if (clipte>0.)
  {
      double xc=(L-clipte)/L;
      double t=0.0, yc=0.0, dycdx=0.0;
      calcProfile( xc, tc, m, p, t, yc, dycdx );
      w.Add
      (
          BRepBuilderAPI_MakeEdge
          (
              pts_up.Value(np), 
              to_Pnt( p0_->value() +tofs*ex +xc*L*ex + yc*L*ey ) 
          )
      );
      w.Add
      (
          BRepBuilderAPI_MakeEdge
          (
              pts_lo.Value(np), 
              to_Pnt( p0_->value() +tofs*ex +xc*L*ex + yc*L*ey ) 
          )
      );
  }
    
  BRepBuilderAPI_MakeFace fb(w.Wire(), true);
  if (!fb.IsDone())
    throw insight::Exception("Failed to generate planar face!");
  
//   providedSubshapes_["OuterWire"].reset(new SolidModel(w.Wire()));
  providedSubshapes_["OuterWire"]=FeaturePtr(new Feature(w.Wire()));
  
  refvalues_["L"]=L;
  
  refpoints_["p_le"]=p0_->value() +tofs*ex ;
  refpoints_["p_te"]=p0_->value() +tofs*ex +L*ex;
  
  setShape(fb.Face());
//   setShape(w.Wire());
}




NacaFourDigit::operator const TopoDS_Face& () const
{
  return TopoDS::Face(shape());
}




void NacaFourDigit::insertrule(parser::ISCADParser& ruleset) const
{
  ruleset.modelstepFunctionRules.add
  (
    "Naca4",	
    typename parser::ISCADParser::ModelstepRulePtr(new typename parser::ISCADParser::ModelstepRule( 

    ( '('  >> ruleset.r_string >> ',' >> ruleset.r_vectorExpression >> ',' >> ruleset.r_vectorExpression >> ',' >> ruleset.r_vectorExpression
           >> ( (',' >> ruleset.r_scalarExpression) | qi::attr(scalarconst(0.0)) ) 
           >> ( (',' >> qi::lit("clipte") >> ruleset.r_scalarExpression) | qi::attr(scalarconst(0.0)) ) 
           >> ')' ) 
	[ qi::_val = phx::bind(&NacaFourDigit::create, qi::_1, qi::_2, qi::_3, qi::_4, qi::_5, qi::_6) ]
      
    |
      ( '('  >> ruleset.r_scalarExpression >> ',' >> ruleset.r_scalarExpression >> ',' >> ruleset.r_scalarExpression >> ','
             >> ruleset.r_vectorExpression >> ',' >> ruleset.r_vectorExpression >> ',' >> ruleset.r_vectorExpression
             >> ( (',' >> ruleset.r_scalarExpression) | qi::attr(scalarconst(0.0)) )
             >> ( (',' >> qi::lit("clipte") >> ruleset.r_scalarExpression) | qi::attr(scalarconst(0.0)) )
             >> ')' )
      [ qi::_val = phx::bind(&NacaFourDigit::create_values, qi::_1, qi::_2, qi::_3, qi::_4, qi::_5, qi::_6, qi::_7, qi::_7) ]
    ))
  );
}




FeatureCmdInfoList NacaFourDigit::ruleDocumentation() const
{
    return boost::assign::list_of
    (
        FeatureCmdInfo
        (
            "Naca4",
            "( <string:code>, <vector:p0>, <vector:L>, <vector:ez> )",
            "Creates an airfoil section from the NACA four digit series. The four digit code is passed as a string."
            " The leading edge is positioned at point p0. Length and direction of the chord line are specified by vector L."
            " The normal direction of the foil section, i.e. spanwise direction of the wing, is given by vector ez."
        )
    );
}




}
}
