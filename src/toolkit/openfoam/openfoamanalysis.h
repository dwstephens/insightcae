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


#ifndef INSIGHT_OPENFOAMANALYSIS_H
#define INSIGHT_OPENFOAMANALYSIS_H

#include "base/analysis.h"
#include "openfoam/openfoamcase.h"
#include "base/parameterstudy.h"


namespace insight {
  
class OpenFOAMAnalysis
: public Analysis
{
public:
  
#include "openfoamanalysis__OpenFOAMAnalysis__Parameters.h"
/*
PARAMETERSET>>> OpenFOAMAnalysis Parameters

run = set 
{	
 machine 	= 	string 	"" 	"Machine or queue, where the external commands are executed on. Defaults to 'localhost', if left empty."
 OFEname 	= 	string 	"OFplus" "Identifier of the OpenFOAM installation, that shall be used"
 np 		= 	int 	1 	"Number of processors for parallel run (less or equal 1 means serial execution)"
 mapFrom 	= 	path 	"" 	"Map solution from specified case, if not empty. potentialinit is skipped if specified."
 potentialinit 	= 	bool 	false 	"Whether to initialize the flow field by potentialFoam when no mapping is done"
 evaluateonly	= 	bool 	false 	"Whether to skip solver run and do only the evaluation"
} "Execution parameters"

mesh = set
{
 linkmesh 	= 	path 	"" 	"If not empty, the mesh will not be generated, but a symbolic link to the polyMesh folder of the specified OpenFOAM case will be created."
} "Properties of the computational mesh"

fluid = set
{
 turbulenceModel = dynamicclassparameters "insight::turbulenceModel" default "kOmegaSST" "Turbulence model"
} "Parameters of the fluid"

eval = set
{
 reportdicts 	= 	bool 	true 	"Include dictionaries into report"
 skipmeshquality 	= 	bool 	false 	"Check to exclude mesh check during evaluation"
} "Parameters for evaluation after solver run"

<<<PARAMETERSET
*/

protected:
    bool stopFlag_;
    ResultSetPtr derivedInputData_;
    
    std::vector<std::shared_ptr<ConvergenceAnalysisDisplayer> > convergenceAnalysis_;

public:
    OpenFOAMAnalysis
    (
        const std::string& name,
        const std::string& description,
        const ParameterSet& ps,
        const boost::filesystem::path& exepath
    );
    
    virtual void cancel();
    
    static ParameterSet defaultParameters();
    virtual boost::filesystem::path setupExecutionEnvironment();
    
    virtual void reportIntermediateParameter(const std::string& name, double value, const std::string& description="", const std::string& unit="");
    
    virtual void calcDerivedInputData();
    virtual void createMesh(OpenFOAMCase& cm) =0;
    virtual void createCase(OpenFOAMCase& cm) =0;
    
    virtual void createDictsInMemory(OpenFOAMCase& cm, std::shared_ptr<OFdicts>& dicts);
    
    /**
     * Customize dictionaries before they get written to disk
     */
    virtual void applyCustomOptions(OpenFOAMCase& cm, std::shared_ptr<OFdicts>& dicts);
    
    virtual void writeDictsToDisk(OpenFOAMCase& cm, std::shared_ptr<OFdicts>& dicts);
    
    /**
     * Do modifications to the case when it has been created on disk
     */
    virtual void applyCustomPreprocessing(OpenFOAMCase& cm);
    
    virtual void mapFromOther(OpenFOAMCase& cm, const boost::filesystem::path& mapFromPath, bool is_parallel);
    virtual void initializeSolverRun(OpenFOAMCase& cm);
    
    virtual void installConvergenceAnalysis(std::shared_ptr<ConvergenceAnalysisDisplayer> cc);
    virtual void runSolver(ProgressDisplayer* displayer, OpenFOAMCase& cm);
    virtual void finalizeSolverRun(OpenFOAMCase& cm);
    
    virtual ResultSetPtr evaluateResults(OpenFOAMCase& cm);
    
    /**
     * integrate all steps before the actual run
     */
    virtual void createCaseOnDisk(OpenFOAMCase& cm);
    
    virtual ResultSetPtr operator()(ProgressDisplayer* displayer=NULL);
};


turbulenceModel* insertTurbulenceModel(OpenFOAMCase& cm, const OpenFOAMAnalysis::Parameters& params );
turbulenceModel* insertTurbulenceModel(OpenFOAMCase& cm, const SelectableSubsetParameter& ps );

}

#endif // INSIGHT_OPENFOAMANALYSIS_H
