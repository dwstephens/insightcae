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

#include "hydrostatics.h"
#include "cadfeatures.h"

#include "AIS_Point.hxx"
// #include "AIS_Drawer.hxx"
#include "Prs3d_TextAspect.hxx"
#include "occtools.h"

using namespace boost;
using namespace std;

namespace insight
{
namespace cad
{

size_t Hydrostatics::calcHash() const
{
  ParameterListHash h;
  h+=*hullvolume_;
  h+=*shipmodel_;
  h+=psurf_->value();
  h+=nsurf_->value();
  h+=elong_->value();
  h+=evert_->value();
  return h.getHash();
}
  
Hydrostatics::Hydrostatics
(
  FeaturePtr hullvolume, 
  FeaturePtr shipmodel, 
  VectorPtr psurf, 
  VectorPtr nsurf, 
  VectorPtr elong, 
  VectorPtr evert
)
: hullvolume_(hullvolume), shipmodel_(shipmodel),
  psurf_(psurf), nsurf_(nsurf),
  elong_(elong), evert_(evert)
{}


void Hydrostatics::build()
{
  elat_=arma::cross(nsurf_->value(), elong_->value());
  
  std::shared_ptr<Cutaway> submerged_volume = std::dynamic_pointer_cast<Cutaway,Feature>( Cutaway::create(hullvolume_, psurf_, nsurf_) );
  submerged_volume->checkForBuildDuringAccess();
  V_=submerged_volume->modelVolume();
  cout<<"displacement V="<<V_<<endl;
  
  m_=shipmodel_->mass();
  cout<<"ship mass m="<<m_<<endl;

  FeaturePtr csf = submerged_volume->providedSubshapes().find("CutSurface")->second;
  if (!csf)
    throw insight::Exception("No cut surface present!");

  // write surface for debug
  TopoDS_Shape issh=static_cast<const TopoDS_Shape&>(*csf);
  
  std::cout<<issh<<std::endl;
  
  TopExp_Explorer ex(issh, TopAbs_FACE);
  TopoDS_Face f=TopoDS::Face(ex.Current());
  if (ex.More()) { std::cout<<"another"<<std::endl; ex.Next();
  if (ex.More()) std::cout<<"yet another"<<std::endl; }
//     throw insight::Exception("cut surface consists of more than a single face!");
  
  BRepTools::Write(f, "test.brep");
  
  GProp_GProps props;
  BRepGProp::SurfaceProperties(f, props);
  GProp_PrincipalProps pcp = props.PrincipalProperties();
  arma::mat I=arma::zeros(3);
  pcp.Moments(I(0), I(1), I(2));
  cout<<"I="<<I<<endl;
  double BM = I.min()/V_;
  cout<<"BM="<<BM<<endl;

  G_ = shipmodel_->modelCoG();
  B_ = submerged_volume->modelCoG();
  M_ = B_ + BM*(evert_->value());
  double GM = norm(M_ - G_, 2);
  cout<<"G="<<G_<<endl;
  cout<<"B="<<B_<<endl;
  cout<<"M="<<M_<<endl;
  cout<<"GM="<<GM<<endl;
}

Handle_AIS_InteractiveObject Hydrostatics::createAISRepr() const
{
  checkForBuildDuringAccess();
  
  TopoDS_Edge cG = BRepBuilderAPI_MakeEdge(gp_Circ(gp_Ax2(to_Pnt(G_),gp_Dir(to_Vec(elat_))), 1));
  Handle_AIS_Shape aisG = new AIS_Shape(cG);
  TopoDS_Edge cB = BRepBuilderAPI_MakeEdge(gp_Circ(gp_Ax2(to_Pnt(B_),gp_Dir(to_Vec(elat_))), 1));
  Handle_AIS_Shape aisB = new AIS_Shape(cB);
  TopoDS_Edge cM = BRepBuilderAPI_MakeEdge(gp_Circ(gp_Ax2(to_Pnt(M_),gp_Dir(to_Vec(elat_))), 1));
  Handle_AIS_Shape aisM = new AIS_Shape(cM);
  
  double d_gm=norm(G_-M_,2);

  Handle_AIS_InteractiveObject aisDim (createLengthDimension(
    BRepBuilderAPI_MakeVertex(to_Pnt(G_)), BRepBuilderAPI_MakeVertex(to_Pnt(M_)),
    Handle_Geom_Plane(new Geom_Plane(gp_Pln(to_Pnt(G_), to_Vec(vec3(0,1,0))))),
    d_gm, 
    str(format("GM = %g")%d_gm).c_str()
  ));
  
  Handle_AIS_InteractiveObject aisBLabel (createArrow(
    cB, str(format("B: V_disp = %g") % V_)
  ));
  Handle_AIS_InteractiveObject aisGLabel (createArrow(
    cG, str(format("G: m = %g") % m_)
  ));

  Handle_AIS_MultipleConnectedInteractive ais(new AIS_MultipleConnectedInteractive());

  ais->Connect(aisG);
  ais->Connect(aisB);
  ais->Connect(aisM);
  ais->Connect(aisDim);
  ais->Connect(aisGLabel);
  ais->Connect(aisBLabel);
  
  return ais;
}

void Hydrostatics::write(std::ostream& ) const
{
}

}
}
