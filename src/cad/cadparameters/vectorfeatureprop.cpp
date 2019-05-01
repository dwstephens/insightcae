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

#include "cadfeature.h"
#include "datum.h"
#include "vectorfeatureprop.h"

#include "gce_MakeCirc.hxx"
#include "GeomAPI_ExtremaCurveCurve.hxx"


insight::cad::PointFeatureProp::PointFeatureProp
(
  insight::cad::FeaturePtr model, 
  const std::string& name
)
: model_(model),
  name_(name)
{}




arma::mat insight::cad::PointFeatureProp::value() const
{
  return model_->getDatumPoint(name_);
}




insight::cad::VectorFeatureProp::VectorFeatureProp
(
  insight::cad::FeaturePtr model, 
  const std::string& name
)
: model_(model),
  name_(name)
{}




arma::mat insight::cad::VectorFeatureProp::value() const
{
  return model_->getDatumVector(name_);
}

insight::cad::SinglePointCoords::SinglePointCoords(insight::cad::ConstFeatureSetPtr pfs)
: pfs_(pfs)
{}




arma::mat insight::cad::SinglePointCoords::value() const
{
  if (!pfs_->size()==1)
    throw insight::Exception("vertex feature set must not contain more than one point for coordinate extraction!");
  
  FeatureID i=*(pfs_->data().begin());
  return pfs_->model()->vertexLocation(i);
}




insight::cad::CircleEdgeCenterCoords::CircleEdgeCenterCoords(insight::cad::ConstFeatureSetPtr pfs)
: pfs_(pfs)
{}




arma::mat insight::cad::CircleEdgeCenterCoords::value() const
{
  if (!pfs_->size()==1)
    throw insight::Exception("edge feature set must not contain more than one edge for coordinate extraction!");
  
  FeatureID i=*(pfs_->data().begin());
  
  TopLoc_Location tl;
  double c0, c1;
  Handle_Geom_Curve curve(BRep_Tool::Curve(pfs_->model()->edge(i), tl, c0, c1));
  GeomAdaptor_Curve adapt(curve);

  gp_Pnt p0;
  if (adapt.GetType()==GeomAbs_Circle)
  {
    gp_Circ icyl=adapt.Circle();
    p0=icyl.Location();
  } else if (adapt.GetType()==GeomAbs_BSplineCurve)
  {
    double u0=adapt.FirstParameter();
    double u1=0.9*adapt.LastParameter();
    double um=0.5*(u0+u1);
    gp_Pnt p1=adapt.Value(u0), p2=adapt.Value(um), p3=adapt.Value(u1);
    gp_Circ ic = gce_MakeCirc(p1, p2, p3);
    double Lref=std::max( std::max(p1.Distance(p2), p2.Distance(p3)), p1.Distance(p3) );

    // check
    GeomAPI_ExtremaCurveCurve ec(
          curve,
          Handle_Geom_Curve(new Geom_Circle(ic))
          );

    double max_dist=0.0;
    for (int i=1; i<=ec.NbExtrema(); i++)
    {
      max_dist=std::max(ec.Distance(i), max_dist);
    }
    if (max_dist>0.1*Lref)
    {
      throw insight::Exception(boost::str(boost::format
         ("selected edge is a BSplineCurve and possibly not circular! (max. distance=%g)") % max_dist
       ));
    }

    p0=ic.Location();
  }
  else
    throw insight::Exception("selected edge is not a circle or BSplineCurve! (instead is of type "+boost::lexical_cast<std::string>(adapt.GetType())+")");

  return vec3(p0.X(), p0.Y(), p0.Z());
}




insight::cad::DatumPointCoord::DatumPointCoord(insight::cad::ConstDatumPtr pfs)
: pfs_(pfs)
{}




arma::mat insight::cad::DatumPointCoord::value() const
{
  if ( pfs_->providesPointReference() )
  {
    return vec3(pfs_->point());
  }
  else
  {
    throw insight::Exception("supplied datum does not provide a point reference!");
  }
  return vec3(0,0,0);
}




insight::cad::DatumDir::DatumDir(insight::cad::ConstDatumPtr pfs)
: pfs_(pfs)
{}




arma::mat insight::cad::DatumDir::value() const
{
  if ( pfs_->providesAxisReference() )
  {
    return vec3(pfs_->axis().Direction());
  }
  else
  {
    throw insight::Exception("supplied datum does not provide an axis reference!");
  }
  return vec3(0,0,0);
}




insight::cad::DatumPlaneNormal::DatumPlaneNormal(insight::cad::ConstDatumPtr pfs)
: pfs_(pfs)
{}




arma::mat insight::cad::DatumPlaneNormal::value() const
{
//   if ( const DatumPlane *pl = dynamic_cast<const DatumPlane*>(pfs_.get()) )
  if (pfs_->providesPlanarReference())
  {
    return vec3(pfs_->plane().Direction());
  }
  else
  {
    throw insight::Exception("supplied datum has to be a plane!");
  }
  return vec3(0,0,0);
}




insight::cad::BBMin::BBMin(FeaturePtr model)
: model_(model)
{}




arma::mat insight::cad::BBMin::value() const
{
  return model_->modelBndBox().col(0);
}




insight::cad::BBMax::BBMax(FeaturePtr model)
: model_(model)
{}




arma::mat insight::cad::BBMax::value() const
{
  return model_->modelBndBox().col(1);
}




insight::cad::COG::COG(FeaturePtr model)
: model_(model)
{}




arma::mat insight::cad::COG::value() const
{
  return model_->modelCoG();
}




insight::cad::SurfaceCOG::SurfaceCOG(FeaturePtr model)
: model_(model)
{}




arma::mat insight::cad::SurfaceCOG::value() const
{
  return model_->surfaceCoG();
}




insight::cad::SurfaceInertiaAxis::SurfaceInertiaAxis(FeaturePtr model, int axis)
: model_(model), axis_(axis)
{}




arma::mat insight::cad::SurfaceInertiaAxis::value() const
{
  return model_->surfaceInertia(axis_);
}
