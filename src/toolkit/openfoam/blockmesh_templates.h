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
 *
 */


#ifndef INSIGHT_BLOCKMESH_TEMPLATES_H
#define INSIGHT_BLOCKMESH_TEMPLATES_H

#include "openfoam/blockmesh.h"

namespace insight {
  
namespace bmd
{
  
    
    
    
class BlockMeshTemplate
    : public insight::bmd::blockMesh
{
public:
    BlockMeshTemplate(OpenFOAMCase& c);
    virtual void addIntoDictionaries ( OFdicts& dictionaries ) const;
    virtual void create_bmd () =0;
    
    static arma::mat correct_trihedron(arma::mat& ex, arma::mat &ez);
};

  



/**
 * A cylinder meshed with an O-grid
 */

class blockMeshDict_Cylinder
    : public BlockMeshTemplate
{
public:
#include "blockmesh_templates__blockMeshDict_Cylinder__Parameters.h"
/*
PARAMETERSET>>> blockMeshDict_Cylinder Parameters

geometry = set
{
    D = double 1.0 "[m] Diameter"
    L = double 1.0 "[m] Length"
    p0 = vector(0 0 0) "[m] Center point"
    ex = vector(0 0 1) "[m] Axial direction"
    er = vector(1 0 0) "[m] Radial direction"
}

mesh = set
{
    nx = int 50 "# cells in axial direction"
    nr = int 10 "# cells in radial direction (from edge of core block to outer radius)"
    nu = int 10 "# cells in circumferential direction (in one of four segments)"
    gradr = double 1 "grading towards outer boundary"
    core_fraction = double 0.33 "radial extent of core block given as fraction of radius"

    defaultPatchName = string "walls" "name of patch where all patches with empty names are assigned to."
    circumPatchName = string "" "name of patch on circumferential surface"
    basePatchName = string "" "name of patch on base end"
    topPatchName = string "" "name of patch on top end"
}

<<<PARAMETERSET
*/

protected:
    Parameters p_;

public:
    declareType ( "blockMeshDict_Cylinder" );

    blockMeshDict_Cylinder ( OpenFOAMCase& c, const ParameterSet& ps = Parameters::makeDefault() );

    virtual void create_bmd();

    inline static ParameterSet defaultParameters()
    {
        return Parameters::makeDefault();
    }

    double rCore() const;
};





/**
 * A simple rectangular mesh
 */

class blockMeshDict_Box
    : public BlockMeshTemplate
{
public:
#include "blockmesh_templates__blockMeshDict_Box__Parameters.h"
/*
PARAMETERSET>>> blockMeshDict_Box Parameters

geometry = set
{
    L = double 1.0 "[m] Length (X)"
    W = double 1.0 "[m] Width (Y)"
    H = double 1.0 "[m] Height (Z)"
    p0 = vector(0 0 0) "[m] Lower left corner"
    ex = vector(1 0 0) "[m] X direction"
    ez = vector(0 0 1) "[m] Upward (Z) direction"
}

mesh = set
{
    resolution = selectablesubset {{

     cubical set {
        n_max = int 10 "Number of cells along longest side. The other sides are discretized with the same cell size but with adjusted number of cells."
     }

     cubical_size set {
        delta = double 0.1 "Uniform cell length."
     }

     individual set {
        nx = int 10 "# cells in X direction"
        ny = int 10 "# cells in Y direction"
        nz = int 10 "# cells in Z direction"
     }

    }} cubical "Mesh resolution"

    defaultPatchName = string "walls" "name of patch where all patches with empty names are assigned to."
    XpPatchName = string "" "name of patch on forward (+X) side"
    XmPatchName = string "" "name of patch on rearward (-X) side"
    YpPatchName = string "" "name of patch on right (+Y) side"
    YmPatchName = string "" "name of patch on left (-Y) side"
    ZpPatchName = string "" "name of patch on top (+Z) side"
    ZmPatchName = string "" "name of patch on bottom (-Z) side"
}

<<<PARAMETERSET
*/

protected:
    Parameters p_;

public:
    declareType ( "blockMeshDict_Box" );

    blockMeshDict_Box ( OpenFOAMCase& c, const ParameterSet& ps = Parameters::makeDefault() );

    virtual void create_bmd();

    inline static ParameterSet defaultParameters()
    {
        return Parameters::makeDefault();
    }
};






/**
 * A simple spherical mesh
 */

class blockMeshDict_Sphere
    : public BlockMeshTemplate
{
public:
#include "blockmesh_templates__blockMeshDict_Sphere__Parameters.h"
/*
PARAMETERSET>>> blockMeshDict_Sphere Parameters

geometry = set
{
    R = double 1.0 "[m] Radius"
    center = vector(0 0 0) "[m] Sphere center"
}

mesh = set
{
    n_rad = int 10 "Number of cells in radial direction"

    outerPatchName = string "outer" "name of boundary patch."
}

<<<PARAMETERSET
*/

protected:
    Parameters p_;

public:
    declareType ( "blockMeshDict_Sphere" );

    blockMeshDict_Sphere ( OpenFOAMCase& c, const ParameterSet& ps = Parameters::makeDefault() );

    virtual void create_bmd();

    inline static ParameterSet defaultParameters()
    {
        return Parameters::makeDefault();
    }
};

}

}

#endif
