/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2014  hannes <email>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "datum.h"

namespace insight {
namespace cad {
  
DatumPlane::DatumPlane(const arma::mat& p0, const arma::mat& ni)
{
  arma::mat n=ni/norm(ni,2);

  arma::mat vx=cross(vec3(0,1,0), n); 
  double m=norm(vx, 2);
  if (m<1e-6)
  {
    vx=cross(vec3(1,0,0), n);
    m=norm(vx, 2);
  }
  vx/=m;
  
  cs_ = gp_Ax3( to_Pnt(p0), gp_Dir(to_Vec(n)), gp_Dir(to_Vec(vx)) );
}

DatumPlane::DatumPlane
(
  const SolidModel& m, 
  FeatureID f
)
{
  arma::mat p0=m.faceCoG(f);
  arma::mat n=m.faceNormal(f);
  
  n/=norm(n,2);
  
  arma::mat vx=cross(vec3(0,1,0), n); 
  double mo=norm(vx, 2);
  if (mo<1e-6)
  {
    vx=cross(vec3(1,0,0), n);
    mo=norm(vx, 2);
  }
  vx/=mo;
  
  cs_ = gp_Ax3( to_Pnt(p0), gp_Dir(to_Vec(n)), gp_Dir(to_Vec(vx)) );
}

DatumPlane::operator const gp_Ax3& () const
{
  return cs_;
}

}
}