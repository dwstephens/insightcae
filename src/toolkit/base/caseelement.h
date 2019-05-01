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


#ifndef CASEELEMENT_H
#define CASEELEMENT_H

#include <string>

#include "base/softwareenvironment.h"
#include "base/parameterset.h"
#include <boost/typeof/typeof.hpp>


namespace insight
{
  
class Case;


#define addToCaseElementFactoryTable(DerivedClass) \
 addToStaticFunctionTable(CaseElement, DerivedClass, isInConflict ); \


class CaseElement
{
public:
  declareStaticFunctionTableWithArgs( isInConflict, bool, LIST(const insight::CaseElement&), LIST(const insight::CaseElement& e) );
  declareType ( "CaseElement" );
  
protected:
  std::string name_;
  Case& case_;

public:
    CaseElement(Case& c, const std::string& name);
    CaseElement(const CaseElement& other);
    virtual ~CaseElement();

    inline const std::string& name() const { return name_; };
    const ParameterSet& params() const;

    static bool isInConflict(const CaseElement& other);

};

}

#endif // CASEELEMENT_H
