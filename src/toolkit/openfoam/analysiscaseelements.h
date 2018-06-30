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

#ifndef INSIGHT_ANALYSISCASEELEMENTS_H
#define INSIGHT_ANALYSISCASEELEMENTS_H

#include "basiccaseelements.h"
#include "base/resultset.h"
#include "base/analysis.h"

namespace insight {

  
  
  
/** ===========================================================================
 * Base class for function objects, that understand the output* options
 */
class outputFilterFunctionObject
: public OpenFOAMCaseElement
{
public:

#include "analysiscaseelements__outputFilterFunctionObject__Parameters.h"

/*
PARAMETERSET>>> outputFilterFunctionObject Parameters

name = string "unnamed" "Name of the function object"
timeStart = double 0 "Time value, when the function object evaluation should start"
outputControl = string "outputTime" "Output time control"
outputInterval = double 1.0 "Time interval between outputs"

<<<PARAMETERSET
*/

protected:
  Parameters p_;
  
public:
  declareFactoryTable 
  ( 
      outputFilterFunctionObject, 
      LIST 
      (  
	  OpenFOAMCase& c, 
	  const ParameterSet& ps
      ), 
      LIST ( c, ps ) 
  );
  declareStaticFunctionTable ( defaultParameters, ParameterSet );
  declareType("outputFilterFunctionObject");

  outputFilterFunctionObject(OpenFOAMCase& c, const ParameterSet & ps = Parameters::makeDefault() );
  virtual OFDictData::dict functionObjectDict() const =0;
  virtual void addIntoDictionaries(OFdicts& dictionaries) const;
  static ParameterSet defaultParameters() { return Parameters::makeDefault(); }
  virtual void evaluate
  (
    OpenFOAMCase& cm, const boost::filesystem::path& location, ResultSetPtr& results, 
    const std::string& shortDescription
  ) const;
  
  static std::string category() { return "Postprocessing"; }
};




/** ===========================================================================
 * function object front end for field fieldAveraging
 */
class fieldAveraging
: public outputFilterFunctionObject
{
    
public:
#include "analysiscaseelements__fieldAveraging__Parameters.h"

/*
PARAMETERSET>>> fieldAveraging Parameters
inherits outputFilterFunctionObject::Parameters

fields = array [ string "U" "Name of a field" ]*1 "Names of fields to average in time"

<<<PARAMETERSET
*/

protected:
  Parameters p_;
  
public:
    declareType("fieldAveraging");
  fieldAveraging(OpenFOAMCase& c, const ParameterSet& ps = Parameters::makeDefault() );
    static ParameterSet defaultParameters()
    {
        return Parameters::makeDefault();
    }
  virtual OFDictData::dict functionObjectDict() const;
};




/** ===========================================================================
 * function object front end for probes
 */
class probes
: public outputFilterFunctionObject
{
    
public:
#include "analysiscaseelements__probes__Parameters.h"

/*
PARAMETERSET>>> probes Parameters
inherits outputFilterFunctionObject::Parameters

fields = array [ string "U" "Name of a field" ]*1 "Names of fields to average in time"
probeLocations = array [ vector (0 0 0) "Probe point location" ]*1 "Locations of probe points"

<<<PARAMETERSET
*/

protected:
  Parameters p_;
  
public:
    declareType("probes");
    probes(OpenFOAMCase& c, const ParameterSet& ps = Parameters::makeDefault() );

    /**
     * reads and returns probe sample data.
     * Matrix dimensions: [ninstants, npts, ncmpt]
     */
    static arma::cube readProbes
    (
        const OpenFOAMCase& c, 
        const boost::filesystem::path& location, 
        const std::string& foName, 
        const std::string& fieldName
    );
    
    /**
     * read sample locations from controlDict
     */
    static arma::mat readProbesLocations
    (
        const OpenFOAMCase& c, 
        const boost::filesystem::path& location, 
        const std::string& foName
    );
    static std::string category() {
        return "Postprocessing";
    }

    static ParameterSet defaultParameters() {
        return Parameters::makeDefault();
    }
    virtual OFDictData::dict functionObjectDict() const;
};




/** ===========================================================================
 * function object front end for volume integrate
 */
class volumeIntegrate
: public outputFilterFunctionObject
{

public:
#include "analysiscaseelements__volumeIntegrate__Parameters.h"

/*
PARAMETERSET>>> volumeIntegrate Parameters
inherits outputFilterFunctionObject::Parameters

fields = array [ string "alpha.phase1" "Name of a field" ]*1 "Names of fields to average over volume"
domain = selectablesubset {{

 wholedomain set { }

 cellZone set {
  cellZoneName = string "zone" "name of cellZone to integrate"
 }

}} wholedomain "select domain of integration"

<<<PARAMETERSET
*/

protected:
  Parameters p_;

public:
    declareType("volumeIntegrate");
    volumeIntegrate(OpenFOAMCase& c, const ParameterSet& ps = Parameters::makeDefault() );

    static std::string category() {
        return "Postprocessing";
    }

    static ParameterSet defaultParameters() {
        return Parameters::makeDefault();
    }
    virtual OFDictData::dict functionObjectDict() const;
};


/** ===========================================================================
 * function object front end for surface integrate
 */
class surfaceIntegrate
: public outputFilterFunctionObject
{

public:
#include "analysiscaseelements__surfaceIntegrate__Parameters.h"

/*
PARAMETERSET>>> surfaceIntegrate Parameters
inherits outputFilterFunctionObject::Parameters

fields = array [ string "alpha.phase1" "Name of a field" ]*1 "Names of fields to average over volume"
domain = selectablesubset {{

 patch set {
  patchName = string "inlet" "name of patch to integrate"
 }

 faceZone set {
  faceZoneName = string "inlet" "name of faceZone to integrate"
 }

}} patch "select domain of integration"

operation = selection ( sum areaIntegrate ) areaIntegrate "operation to execute on data"

<<<PARAMETERSET
*/

protected:
  Parameters p_;

public:
    declareType("surfaceIntegrate");
    surfaceIntegrate(OpenFOAMCase& c, const ParameterSet& ps = Parameters::makeDefault() );

    static std::string category() {
        return "Postprocessing";
    }

    static ParameterSet defaultParameters() {
        return Parameters::makeDefault();
    }
    virtual OFDictData::dict functionObjectDict() const;
};


/** ===========================================================================
 * function object front end for live cutting plane sampling during run
 */
class cuttingPlane
: public outputFilterFunctionObject
{
    
public:
#include "analysiscaseelements__cuttingPlane__Parameters.h"

/*
PARAMETERSET>>> cuttingPlane Parameters
inherits outputFilterFunctionObject::Parameters

fields = array [ string "U" "Name of a field" ]*1 "Names of fields to average in time"
basePoint = vector (0 0 0) "Cutting plane base point"
normal = vector (0 0 1) "Cutting plane normal direction"
interpolate = bool true "Whether to output VTKs with interpolated fields"

<<<PARAMETERSET
*/

protected:
  Parameters p_;
  
public:
    declareType("cuttingPlane");
  cuttingPlane(OpenFOAMCase& c, const ParameterSet& ps = Parameters::makeDefault() );
    static ParameterSet defaultParameters()
    {
        return Parameters::makeDefault();
    }
  virtual OFDictData::dict functionObjectDict() const;
};




/** ===========================================================================
 * function object front end for two-point correlation sampling
 */
class twoPointCorrelation
: public outputFilterFunctionObject
{
public:
#include "analysiscaseelements__twoPointCorrelation__Parameters.h"

/*
PARAMETERSET>>> twoPointCorrelation Parameters
inherits outputFilterFunctionObject::Parameters

p0 = vector (0 0 0) "Base point"
directionSpan = vector (1 0 0) "Direction and distance of correlation"
homogeneousTranslationUnit = vector (0 1 0) "Translational distance between two subsequent correlation lines for homogeneous averaging"
np = int 50 "Number of correlation points"
nph = int 1 "Number of homogeneous averaging locations"

<<<PARAMETERSET
*/

protected:
  Parameters p_;
  
public:
  declareType("twoPointCorrelation");
  twoPointCorrelation(OpenFOAMCase& c, const ParameterSet& ps = Parameters::makeDefault() );
    
  static ParameterSet defaultParameters() { return Parameters::makeDefault(); }
  
  virtual OFDictData::dict functionObjectDict() const;
  virtual OFDictData::dict csysConfiguration() const;

  inline const std::string& name() const { return p_.name; }
  static boost::ptr_vector<arma::mat> readCorrelations(const OpenFOAMCase& c, const boost::filesystem::path& location, const std::string& tpcName);
};




/** ===========================================================================
 * front end for sampling of two-point correlations in cylindrical CS
 */
class cylindricalTwoPointCorrelation
: public twoPointCorrelation
{
    
public:
#include "analysiscaseelements__cylindricalTwoPointCorrelation__Parameters.h"

/*
PARAMETERSET>>> cylindricalTwoPointCorrelation Parameters
inherits twoPointCorrelation::Parameters

ez = vector (0 0 1) "Axial direction"
er = vector (1 0 0) "Radial direction"
degrees = bool false ""

<<<PARAMETERSET
*/

protected:
  Parameters p_;
  
public:
  declareType("cylindricalTwoPointCorrelation");
  cylindricalTwoPointCorrelation(OpenFOAMCase& c, const ParameterSet& ps = Parameters::makeDefault() );
  static ParameterSet defaultParameters()
    {
        return Parameters::makeDefault();
    }
  virtual OFDictData::dict csysConfiguration() const;
};



#ifdef SWIG
%template(getforcesCaseElement) insight::OpenFOAMCase::get<const forces>;
#endif

class forces
: public OpenFOAMCaseElement
{
    
public:
#include "analysiscaseelements__forces__Parameters.h"

/*
PARAMETERSET>>> forces Parameters

name = string "forces" "Name of this forces function object"
patches = array [ string "patch" "Name of patch" ] *1 "Name of patches for force integration"
pName = string "p" "Name of pressure field"
UName = string "U" "Name of velocity field"
rhoName = string "rhoInf" "Name of density field. rhoInf for constant rho"
rhoInf = double 1.0 "Value of constant density"
outputControl = string "timeStep" "Output control"
outputInterval = double 10.0 "Output interval"
CofR = vector (0 0 0) "Center point for torque calculation"

<<<PARAMETERSET
*/

protected:
  Parameters p_;
  
public:
  declareType("forces");
  forces(OpenFOAMCase& c, const ParameterSet& ps = Parameters::makeDefault() );
  static ParameterSet defaultParameters()
    {
        return Parameters::makeDefault();
    }
  virtual void addIntoDictionaries(OFdicts& dictionaries) const;
  
  static arma::mat readForces(const OpenFOAMCase& c, const boost::filesystem::path& location, const std::string& foName);
  
  static std::string category() { return "Postprocessing"; }
};




class extendedForces
: public forces
{
public:
#include "analysiscaseelements__extendedForces__Parameters.h"

/*
PARAMETERSET>>> extendedForces Parameters
inherits insight::forces::Parameters

maskField = string "" "Optional: name of field which masks the force evaluation. The local force density is multiplied by this field."

<<<PARAMETERSET
*/

protected:
  Parameters p_;
  
public:
  declareType("extendedForces");
  extendedForces(OpenFOAMCase& c, const ParameterSet& ps = Parameters::makeDefault() );
  static ParameterSet defaultParameters() { return Parameters::makeDefault(); }
  virtual void addIntoDictionaries(OFdicts& dictionaries) const;
};



class CorrelationFunctionModel
: public RegressionModel
{
public:
  double A_, B_, omega_, phi_;
  
  CorrelationFunctionModel();
  virtual int numP() const;
  virtual void setParameters(const double* params);
  virtual void setInitialValues(double* params) const;
  virtual arma::mat weights(const arma::mat& x) const;
  virtual arma::mat evaluateObjective(const arma::mat& x) const;
  
  double lengthScale() const;
};



class ComputeLengthScale
: public Analysis
{
public:
#include "analysiscaseelements__ComputeLengthScale__Parameters.h"

/*
PARAMETERSET>>> ComputeLengthScale Parameters

R_vs_x = matrix 10x2 "autocorrelation function: first column contains coordinate and second column the normalized autocorrelation values."

<<<PARAMETERSET
*/

protected:
  Parameters p_;

public:
  declareType("ComputeLengthScale");
  ComputeLengthScale(const ParameterSet& ps, const boost::filesystem::path& exepath );

  virtual ResultSetPtr operator()(ProgressDisplayer* displayer=NULL);

  static std::string category() { return "General Postprocessing"; }
  static ParameterSet defaultParameters() { return Parameters::makeDefault(); }
};



template<class TPC, const char* TypeName>
class TPCArray
: public outputFilterFunctionObject
{
  
  static const char * cmptNames[];
  
public:
#include "analysiscaseelements__TPCArrayBase__Parameters.h"

// x = double 0.0 "X-coordinate of base point"
// z = double 0.0 "Z-coordinate of base point"
// name_prefix = string "tpc" "name prefix of individual autocorrelation sampling FOs"
// timeStart = double 0.0 "start time of sampling. A decent UMean field has to be available in database at that time."
// outputControl = string "outputTime" "output control selection"
// outputInterval = double 10.0 "output interval"
  
/*
PARAMETERSET>>> TPCArrayBase Parameters
inherits outputFilterFunctionObject::Parameters

p0 = vector (0 0 0) "base point of TPC array"
tanSpan = double 3.14159265359 "length evaluation section in transverse direction"
axSpan = double 1.0 "length evaluation section in longitudinal direction"
np = int 50 "number of autocorrelation points"
nph = int 8 "number of instances for averaging over homogeneous direction"
R = double 1.0 "Maximum radius coordinate (or Y-coordinate) array. Minimum of range is always 0. Sampling locations are more concentrated at end point according to cosinus rule."
e_ax = vector (1 0 0) "longitudinal direction"
e_rad = vector (0 1 0) "radial direction"
e_tan = vector (0 0 1) "transverse direction"
grading = selection (towardsEnd towardsStart none) towardsEnd "Grading in placement of TPC sample FOs"

<<<PARAMETERSET
*/

protected:
  Parameters p_;
  std::vector<double> r_;
  boost::ptr_vector<TPC> tpc_ax_;
  boost::ptr_vector<TPC> tpc_tan_;
  
  typename TPC::Parameters getTanParameters(int i) const;
  typename TPC::Parameters getAxParameters(int i) const;
  
  std::string axisTitleTan() const;
  std::string axisTitleAx() const;
  
public:
  declareType(TypeName);
  
  TPCArray(OpenFOAMCase& c, ParameterSet const &ps = Parameters::makeDefault() );
  virtual OFDictData::dict functionObjectDict() const;
  virtual void addIntoDictionaries(OFdicts& dictionaries) const;
  virtual void evaluate(OpenFOAMCase& cm, const boost::filesystem::path& location, ResultSetPtr& results, 
			const std::string& shortDescription) const;
  virtual void evaluateSingle
  (
    OpenFOAMCase& cm, const boost::filesystem::path& location, 
    ResultSetPtr& results, 
    const std::string& name_prefix,
    double span,
    const std::string& axisLabel,
    const boost::ptr_vector<TPC>& tpcarray,
    const std::string& shortDescription,
    Ordering& so = Ordering()
  ) const;

  static ParameterSet defaultParameters() { return Parameters::makeDefault(); }

};

extern const char LinearTPCArrayTypeName[];
typedef TPCArray<twoPointCorrelation, LinearTPCArrayTypeName> LinearTPCArray;
extern const char RadialTPCArrayTypeName[];
typedef TPCArray<cylindricalTwoPointCorrelation, RadialTPCArrayTypeName> RadialTPCArray;

}

#include "analysiscaseelementstemplates.h"

#endif // INSIGHT_ANALYSISCASEELEMENTS_H
