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


#ifndef INSIGHT_PARAMETER_H
#define INSIGHT_PARAMETER_H

#include "factory.h"
#include "base/latextools.h"
#include "base/linearalgebra.h"

#include <string>
#include <vector>
#include <string>
#include <typeinfo>
#include <set>

#include "base/boost_include.h"
#include <boost/noncopyable.hpp>
#include <boost/concept_check.hpp>
#include "boost/version.hpp"

#include "rapidxml/rapidxml.hpp"




namespace boost 
{ 
namespace filesystem
{
  
template < >
path& path::append< typename path::iterator >( typename path::iterator begin, typename path::iterator end, const codecvt_type& cvt);

//boost::filesystem::path make_relative( boost::filesystem::path a_From, boost::filesystem::path a_To );
  
} 
}




namespace insight {
  
std::string base64_encode(const std::string& s);
std::string base64_encode(const boost::filesystem::path& f);
std::string base64_decode(const std::string& s);




void writeMatToXMLNode(const arma::mat& matrix, rapidxml::xml_document< char >& doc, rapidxml::xml_node< char >& node);




class Parameter
    : public boost::noncopyable
{

public:
    declareFactoryTable ( Parameter, LIST ( const std::string& descr ), LIST ( descr ) );

protected:
    SimpleLatex description_;

    bool isHidden_, isExpert_, isNecessary_;
    int order_;

public:
    declareType ( "Parameter" );

    Parameter();
    Parameter ( const std::string& description,  bool isHidden, bool isExpert, bool isNecessary, int order);
    virtual ~Parameter();

    bool isHidden() const;
    bool isExpert() const;
    bool isNecessary() const;
    int order() const;

    inline const SimpleLatex& description() const
    {
        return description_;
    }
    
    inline SimpleLatex& description()
    {
        return description_;
    }

    /**
     * LaTeX representation of the parameter value
     */
    virtual std::string latexRepresentation() const =0;
    virtual std::string plainTextRepresentation(int indent=0) const =0;

    virtual rapidxml::xml_node<>* appendToNode
    (
        const std::string& name,
        rapidxml::xml_document<>& doc,
        rapidxml::xml_node<>& node,
        boost::filesystem::path inputfilepath
    ) const;

    virtual void readFromNode
    (
        const std::string& name,
        rapidxml::xml_document<>& doc,
        rapidxml::xml_node<>& node,
        boost::filesystem::path inputfilepath
    ) =0;

    rapidxml::xml_node<> *findNode ( rapidxml::xml_node<>& father, const std::string& name );
    virtual Parameter* clone() const =0;

    /**
     * @brief isPacked
     * check, if contains file contents
     * @return
     */
    virtual bool isPacked() const;

    /**
     * @brief pack
     * pack the external file. Replace stored content, if present.
     */
    virtual void pack();

    /**
     * @brief unpack
     * restore file contents on disk, if file is not there
     */
    virtual void unpack();
};




typedef std::shared_ptr<Parameter> ParameterPtr;




template<class V>
std::string valueToString(const V& value)
{
  return boost::lexical_cast<std::string>(value);
}




std::string valueToString(const arma::mat& value);




template<class V>
void stringToValue(const std::string& s, V& v)
{
  v=boost::lexical_cast<V>(s);
}




void stringToValue(const std::string& s, arma::mat& v);
  



template<class T, char const* N>
class SimpleParameter
    : public Parameter
{

public:
    typedef T value_type;

protected:
    T value_;

public:
    declareType ( N );

    SimpleParameter ( const std::string& description,  bool isHidden=false, bool isExpert=false, bool isNecessary=false, int order=0 )
        : Parameter ( description, isHidden, isExpert, isNecessary, order )
    {}

    SimpleParameter ( const T& value, const std::string& description,  bool isHidden=false, bool isExpert=false, bool isNecessary=false, int order=0 )
        : Parameter ( description, isHidden, isExpert, isNecessary, order ),
          value_ ( value )
    {}

    virtual ~SimpleParameter()
    {}

    virtual T& operator() ()
    {
        return value_;
    }
    virtual const T& operator() () const
    {
        return value_;
    }

    virtual std::string latexRepresentation() const
    {
        return SimpleLatex( valueToString ( value_ ) ).toLaTeX();
    }

    virtual std::string plainTextRepresentation(int indent=0) const
    {
        return SimpleLatex( valueToString ( value_ ) ).toPlainText();
    }


    virtual Parameter* clone() const
    {
        return new SimpleParameter<T, N> ( value_, description_.simpleLatex(), isHidden_, isExpert_, isNecessary_, order_ );
    }

    virtual rapidxml::xml_node<>* appendToNode ( const std::string& name, rapidxml::xml_document<>& doc, rapidxml::xml_node<>& node,
            boost::filesystem::path inputfilepath ) const
    {
        using namespace rapidxml;
        xml_node<>* child = Parameter::appendToNode ( name, doc, node, inputfilepath );
        child->append_attribute
        (
            doc.allocate_attribute
            (
                "value",
                doc.allocate_string ( valueToString ( value_ ).c_str() )
            )
        );
        return child;
    }

    virtual void readFromNode ( const std::string& name, rapidxml::xml_document<>& doc, rapidxml::xml_node<>& node,
                                boost::filesystem::path inputfilepath )
    {
//        std::cout<<"Reading simple "<<name<< std::endl;
        using namespace rapidxml;
        xml_node<>* child = findNode ( node, name );
        if ( child ) {
            stringToValue ( child->first_attribute ( "value" )->value(), value_ );
        }
    }

};



  
extern char DoubleName[];
extern char IntName[];
extern char BoolName[];
extern char VectorName[];
extern char StringName[];
extern char PathName[];

typedef SimpleParameter<double, DoubleName> DoubleParameter;
typedef SimpleParameter<int, IntName> IntParameter;
typedef SimpleParameter<bool, BoolName> BoolParameter;
typedef SimpleParameter<arma::mat, VectorName> VectorParameter;
typedef SimpleParameter<std::string, StringName> StringParameter;
// typedef SimpleParameter<boost::filesystem::path, PathName> PathParameter;




class PathParameter
    : public SimpleParameter<boost::filesystem::path, PathName>
{
    // store content of file, if packed
    std::string file_content_;

public:
    declareType ( "path" );

    PathParameter ( const std::string& description,  bool isHidden=false, bool isExpert=false, bool isNecessary=false, int order=0 );
    PathParameter ( const boost::filesystem::path& value, const std::string& description,  bool isHidden=false, bool isExpert=false, bool isNecessary=false, int order=0, const char* base64_content = "" );

    virtual boost::filesystem::path& operator() ();
    virtual const boost::filesystem::path& operator() () const;

    /**
     * @brief isPacked
     * check, if contains file contents
     * @return
     */

    bool isPacked() const;

    /**
     * @brief pack
     * pack the external file. Replace stored content, if present.
     */
    void pack();

    /**
     * @brief unpack
     * restore file contents on disk, if file is not there
     */
    void unpack();

    virtual Parameter* clone() const;

    virtual rapidxml::xml_node<>* appendToNode(const std::string& name, rapidxml::xml_document<>& doc, rapidxml::xml_node<>& node,
     boost::filesystem::path inputfilepath) const;

    virtual void readFromNode(const std::string& name, rapidxml::xml_document<>& doc, rapidxml::xml_node<>& node,
     boost::filesystem::path inputfilepath);
};




#ifdef SWIG
%template(DoubleParameter) SimpleParameter<double, DoubleName>;
%template(IntParameter) SimpleParameter<int, IntName>;
%template(BoolParameter) SimpleParameter<bool, BoolName>;
%template(VectorParameter) SimpleParameter<arma::mat, VectorName>;
%template(StringParameter) SimpleParameter<std::string, StringName>;
// %template(PathParameter) SimpleParameter<boost::filesystem::path, PathName>;
#endif




//template<> rapidxml::xml_node<>* SimpleParameter<boost::filesystem::path, PathName>::appendToNode(const std::string& name, rapidxml::xml_document<>& doc, rapidxml::xml_node<>& node,
//    boost::filesystem::path inputfilepath) const;

//template<> void SimpleParameter<boost::filesystem::path, PathName>::readFromNode(const std::string& name, rapidxml::xml_document<>& doc, rapidxml::xml_node<>& node,
//  boost::filesystem::path inputfilepath);



  
class DirectoryParameter
    : public PathParameter
{
public:
    declareType ( "directory" );

    DirectoryParameter ( const std::string& description,  bool isHidden=false, bool isExpert=false, bool isNecessary=false, int order=0 );
    DirectoryParameter ( const boost::filesystem::path& value, const std::string& description,  bool isHidden=false, bool isExpert=false, bool isNecessary=false, int order=0 );
    virtual std::string latexRepresentation() const;
    virtual Parameter* clone() const;
    virtual rapidxml::xml_node<>* appendToNode ( const std::string& name, rapidxml::xml_document<>& doc, rapidxml::xml_node<>& node,
            boost::filesystem::path inputfilepath ) const;
    virtual void readFromNode ( const std::string& name, rapidxml::xml_document<>& doc, rapidxml::xml_node<>& node,
                                boost::filesystem::path inputfilepath );
};




class SelectionParameter
    : public IntParameter
{
public:
    typedef std::vector<std::string> ItemList;

protected:
    ItemList items_;

public:
    declareType ( "selection" );

    SelectionParameter ( const std::string& description,  bool isHidden=false, bool isExpert=false, bool isNecessary=false, int order=0 );
    SelectionParameter ( const int& value, const ItemList& items, const std::string& description,  bool isHidden=false, bool isExpert=false, bool isNecessary=false, int order=0 );
    SelectionParameter ( const std::string& key, const ItemList& items, const std::string& description,  bool isHidden=false, bool isExpert=false, bool isNecessary=false, int order=0 );
    virtual ~SelectionParameter();

    inline ItemList& items()
    {
        return items_;
    };
    virtual const ItemList& items() const;
    inline void setSelection ( const std::string& sel )
    {
        value_=selection_id ( sel );
    }
    inline const std::string& selection() const
    {
        return items_[value_];
    }
    inline int selection_id ( const std::string& key ) const
    {
        return  std::find ( items_.begin(), items_.end(), key ) - items_.begin();
    }

    virtual std::string latexRepresentation() const;
    virtual std::string plainTextRepresentation(int indent=0) const;

    virtual Parameter* clone() const;

    virtual rapidxml::xml_node<>* appendToNode ( const std::string& name, rapidxml::xml_document<>& doc, rapidxml::xml_node<>& node,
            boost::filesystem::path inputfilepath ) const;
    virtual void readFromNode ( const std::string& name, rapidxml::xml_document<>& doc, rapidxml::xml_node<>& node,
                                boost::filesystem::path inputfilepath );
};




class DoubleRangeParameter
    : public Parameter
{
public:
    typedef std::set<double> RangeList;

protected:
    RangeList values_;

public:
    typedef RangeList value_type;

    declareType ( "doubleRange" );

    DoubleRangeParameter ( const std::string& description,  bool isHidden=false, bool isExpert=false, bool isNecessary=false, int order=0 );
    DoubleRangeParameter ( const RangeList& value, const std::string& description,  bool isHidden=false, bool isExpert=false, bool isNecessary=false, int order=0 );
    DoubleRangeParameter ( double defaultFrom, double defaultTo, int defaultNum, const std::string& description,  bool isHidden=false, bool isExpert=false, bool isNecessary=false, int order=0 );
    virtual ~DoubleRangeParameter();

    inline void insertValue ( double v )
    {
        values_.insert ( v );
    }
    inline RangeList::iterator operator() ()
    {
        return values_.begin();
    }
    inline RangeList::const_iterator operator() () const
    {
        return values_.begin();
    }

    inline RangeList& values()
    {
        return values_;
    }
    inline const RangeList& values() const
    {
        return values_;
    }

    virtual std::string latexRepresentation() const;
    virtual std::string plainTextRepresentation(int indent=0) const;

    DoubleParameter* toDoubleParameter ( RangeList::const_iterator i ) const;

    virtual Parameter* clone() const;

    virtual rapidxml::xml_node<>* appendToNode ( const std::string& name, rapidxml::xml_document<>& doc, rapidxml::xml_node<>& node,
            boost::filesystem::path inputfilepath ) const;
    virtual void readFromNode ( const std::string& name, rapidxml::xml_document<>& doc, rapidxml::xml_node<>& node,
                                boost::filesystem::path inputfilepath );
};




class ArrayParameter
    : public Parameter
{
public:
    typedef std::vector<ParameterPtr> value_type;

protected:
    ParameterPtr defaultValue_;
    std::vector<ParameterPtr> value_;

public:
    declareType ( "array" );

    ArrayParameter ( const std::string& description,  bool isHidden=false, bool isExpert=false, bool isNecessary=false, int order=0 );
    ArrayParameter ( const Parameter& defaultValue, int size, const std::string& description,  bool isHidden=false, bool isExpert=false, bool isNecessary=false, int order=0 );

    //inline void setParameterSet(const ParameterSet& paramset) { value_.reset(paramset.clone()); }
    inline void setDefaultValue ( const Parameter& defP )
    {
        defaultValue_.reset ( defP.clone() );
    }
    inline void eraseValue ( int i )
    {
        value_.erase ( value_.begin()+i );
    }
    inline void appendValue ( const Parameter& np )
    {
        value_.push_back ( ParameterPtr( np.clone() ) );
    }
    inline void appendEmpty()
    {
        value_.push_back ( ParameterPtr( defaultValue_->clone() ) );
    }
    inline Parameter& operator[] ( int i )
    {
        return *(value_[i]);
    }
    inline const Parameter& operator[] ( int i ) const
    {
        return *(value_[i]);
    }
    inline int size() const
    {
        return value_.size();
    }
    inline void clear()
    {
        value_.clear();
    }

    virtual std::string latexRepresentation() const;
    virtual std::string plainTextRepresentation(int indent=0) const;

    virtual bool isPacked() const;
    virtual void pack();
    virtual void unpack();

    virtual Parameter* clone () const;

    virtual rapidxml::xml_node<>* appendToNode ( const std::string& name, rapidxml::xml_document<>& doc, rapidxml::xml_node<>& node,
            boost::filesystem::path inputfilepath ) const;
    virtual void readFromNode ( const std::string& name, rapidxml::xml_document<>& doc, rapidxml::xml_node<>& node,
                                boost::filesystem::path inputfilepath );
};




class MatrixParameter
    : public Parameter
{
public:
    typedef arma::mat value_type;

protected:
    arma::mat value_;

public:
    declareType ( "matrix" );

    MatrixParameter ( const std::string& description,  bool isHidden=false, bool isExpert=false, bool isNecessary=false, int order=0 );
    MatrixParameter ( const arma::mat& defaultValue, const std::string& description,  bool isHidden=false, bool isExpert=false, bool isNecessary=false, int order=0 );

    arma::mat& operator() ();
    const arma::mat& operator() () const;

    virtual std::string latexRepresentation() const;
    virtual std::string plainTextRepresentation(int indent=0) const;

    virtual Parameter* clone () const;

    virtual rapidxml::xml_node<>* appendToNode ( const std::string& name, rapidxml::xml_document<>& doc, rapidxml::xml_node<>& node,
            boost::filesystem::path inputfilepath ) const;
    virtual void readFromNode ( const std::string& name, rapidxml::xml_document<>& doc, rapidxml::xml_node<>& node,
                                boost::filesystem::path inputfilepath );
};




inline Parameter* new_clone(const Parameter& p)
{
  return p.clone();
}




}

#endif // INSIGHT_PARAMETER_H
