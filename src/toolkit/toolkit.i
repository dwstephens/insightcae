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

%include "common.i"
%include "exception.i"

%module(directors="1") toolkit

%{
#include "base/factory.h"
#include "base/parameter.h"
#include "base/parameterset.h"
#include "base/resultset.h"
#include "base/analysis.h"
#include "base/pythonanalysis.h"
#include "base/parameterstudy.h"
#include "base/case.h"
#include "base/caseelement.h"
#include "base/linearalgebra.h"

#include "openfoam/openfoamcase.h"
#include "openfoam/openfoamdict.h"
#include "openfoam/openfoamtools.h"
#include "openfoam/openfoamcaseelements.h"
#include "openfoam/openfoamanalysis.h"
#include "openfoam/blockmesh.h"
#include "openfoam/blockmesh_templates.h"
#include "openfoam/remoteexecution.h"

#include "code_aster/caexportfile.h"
#include "code_aster/codeasterrun.h"
#include "base/exception.h"

using namespace insight;
using namespace insight::bmd;
%}

%exception {
	try {
	$action
	}
	catch (insight::Exception& e) {
	 	SWIG_exception(SWIG_RuntimeError, ("Insight exception: "+e.as_string()).c_str());
	}
}


%typemap(out) int& getInt %{
  $result = PyInt_FromLong(*$1);
%}
%typemap(out) bool& getBool %{
  $result = PyBool_FromLong(*$1);
%}
%typemap(out) double& getDouble %{
  $result = PyFloat_FromDouble(*$1);
%}


// %feature("director") Analysis;
%include "base/factory.h"
%include "base/parameter.h"
%include "base/parameterset.h"
%include "base/resultset.h"
%include "base/parameter.h"
%include "base/analysis.h"
%include "base/parameterstudy.h"
%include "base/case.h"
%include "base/caseelement.h"
%include "base/linearalgebra.h"

%include "openfoam/openfoamcase.h"
%include "openfoam/openfoamdict.h"
%include "openfoam/openfoamtools.h"
//%include "openfoam/caseelements/numericscaseelements.h"
%include "openfoam/caseelements/analysiscaseelements.h"
%include "openfoam/openfoamanalysis.h"
//%include "openfoam/blockmesh.h"
%include "openfoam/remoteexecution.h"

%rename(Cylinder_Parameters) Cylinder::Parameters;

%include "openfoam/blockmesh_templates.h"

%include "code_aster/caexportfile.h"
%include "code_aster/codeasterrun.h"
