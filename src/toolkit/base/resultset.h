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


#ifndef INSIGHT_RESULTSET_H
#define INSIGHT_RESULTSET_H

#include "base/units.h"
#include "base/parameterset.h"
#include "base/linearalgebra.h"

#include <string>
#include <vector>

#include "base/boost_include.h"
#include "boost/gil/gil_all.hpp"


namespace gnuplotio {
 class Gnuplot;
}

namespace insight 
{
  
    
    

class ResultElement
    : public boost::noncopyable
{
public:
    declareFactoryTable
    (
        ResultElement,
        LIST ( const std::string& shortdesc, const std::string& longdesc, const std::string& unit ),
        LIST ( shortdesc, longdesc, unit )
    );

protected:
  /**
   * short description of result quantity in LaTeX format
   */
    SimpleLatex shortDescription_;

  /**
   * detailed description of result quantity in LaTeX format
   */
    SimpleLatex longDescription_;
    
  /**
   * unit of result quantity in LaTeX format
   */
    SimpleLatex unit_;
    
    /**
     * numerical quantity which determines order relative to other result elements
     */
    double order_;

public:
    declareType ( "ResultElement" );

    ResultElement ( const std::string& shortdesc, const std::string& longdesc, const std::string& unit );
    virtual ~ResultElement();

  /**
   * short description of result quantity
   */
  const SimpleLatex& shortDescription() const;

  /**
   * detailed description of result quantity
   */
  const SimpleLatex& longDescription() const;
  
  /**
   * unit of result quantity
   */
  const SimpleLatex& unit() const;
  
    inline void setOrder ( double o ) { order_=o; }
    inline double order() const { return order_; }

    virtual void writeLatexHeaderCode ( std::ostream& f ) const;
    virtual void writeLatexCode ( std::ostream& f, const std::string& name, int level, const boost::filesystem::path& outputfilepath ) const;
    virtual void exportDataToFile ( const std::string& name, const boost::filesystem::path& outputdirectory ) const;

    /**
     * append the contents of this element to the given xml node
     */
    virtual rapidxml::xml_node<>* appendToNode
    (
        const std::string& name,
        rapidxml::xml_document<>& doc,
        rapidxml::xml_node<>& node
    ) const;

    /**
     * restore the contents of this element from the given node
     */
    virtual void readFromNode
    (
        const std::string& name,
        rapidxml::xml_document<>& doc,
        rapidxml::xml_node<>& node
    );

    /**
     * convert this result element into a parameter
     * returns an invalid pointer per default
     * Since not all Results can be converted into parameters, a check for validity is required before using the pointer.
     */
    virtual ParameterPtr convertIntoParameter() const;

    virtual std::shared_ptr<ResultElement> clone() const =0;
};




class Ordering
{
  double ordering_, step_;
public:
  Ordering(double ordering_base=1., double ordering_step_fraction=0.001);
  
  double next();
};




//typedef std::auto_ptr<ResultElement> ResultElementPtr;
typedef std::shared_ptr<ResultElement> ResultElementPtr;





class Image
    : public ResultElement
{
protected:
    boost::filesystem::path imagePath_;

public:
    declareType ( "Image" );
    Image ( const std::string& shortdesc, const std::string& longdesc, const std::string& unit );
    Image ( const boost::filesystem::path& location, const boost::filesystem::path& value, const std::string& shortDesc, const std::string& longDesc );

    inline const boost::filesystem::path& imagePath() const
    {
        return imagePath_;
    }
    inline void setPath ( const boost::filesystem::path& value )
    {
        imagePath_=value;
    }

    virtual void writeLatexHeaderCode ( std::ostream& f ) const;
    virtual void writeLatexCode ( std::ostream& f, const std::string& name, int level, const boost::filesystem::path& outputfilepath ) const;

    /**
     * append the contents of this element to the given xml node
     */
    virtual rapidxml::xml_node<>* appendToNode
    (
        const std::string& name,
        rapidxml::xml_document<>& doc,
        rapidxml::xml_node<>& node
    ) const;

    virtual ResultElementPtr clone() const;
};




template<class T>
class NumericalResult
    : public ResultElement
{
protected:
    T value_;

public:

    NumericalResult ( const std::string& shortdesc, const std::string& longdesc, const std::string& unit )
        : ResultElement ( shortdesc, longdesc, unit )
    {}

    NumericalResult ( const T& value, const std::string& shortDesc, const std::string& longDesc, const std::string& unit )
        : ResultElement ( shortDesc, longDesc, unit ),
          value_ ( value )
    {}

    inline void setValue ( const T& value )
    {
        value_=value;
    }

    inline const T& value() const
    {
        return value_;
    }

    virtual void exportDataToFile ( const std::string& name, const boost::filesystem::path& outputdirectory ) const
    {
        boost::filesystem::path fname ( outputdirectory/ ( name+".dat" ) );
        std::ofstream f ( fname.c_str() );
        f<<value_<<std::endl;
    }

    /**
     * append the contents of this element to the given xml node
     */
    virtual rapidxml::xml_node<>* appendToNode
    (
        const std::string& name,
        rapidxml::xml_document<>& doc,
        rapidxml::xml_node<>& node
    ) const
    {
        using namespace rapidxml;
        xml_node<>* child = ResultElement::appendToNode ( name, doc, node );

        child->append_attribute ( doc.allocate_attribute
                                  (
                                      "value",
                                      doc.allocate_string ( boost::lexical_cast<std::string> ( value_ ).c_str() )
                                  ) );

        return child;
    }

    inline operator const T& () const
    {
        return value();
    }
    inline const T& operator() () const
    {
        return value();
    }

};




class Comment
    : public ResultElement
{
protected:
    std::string value_;

public:
    declareType ( "Comment" );

    Comment ( const std::string& shortdesc, const std::string& longdesc, const std::string& unit );
    Comment ( const std::string& value, const std::string& shortDesc );
    virtual void writeLatexCode ( std::ostream& f, const std::string& name, int level, const boost::filesystem::path& outputfilepath ) const;
    virtual void exportDataToFile ( const std::string& name, const boost::filesystem::path& outputdirectory ) const;

    /**
     * append the contents of this element to the given xml node
     */
    virtual rapidxml::xml_node<>* appendToNode
    (
        const std::string& name,
        rapidxml::xml_document<>& doc,
        rapidxml::xml_node<>& node
    ) const;

    inline const std::string& value() const
    {
        return value_;
    }

    virtual ResultElementPtr clone() const;
};

#ifdef SWIG
%template(doubleNumericalResult) NumericalResult<double>;
#endif

class ScalarResult
    : public NumericalResult<double>
{
public:
    declareType ( "ScalarResult" );

    ScalarResult ( const std::string& shortdesc, const std::string& longdesc, const std::string& unit );
    ScalarResult ( const double& value, const std::string& shortDesc, const std::string& longDesc, const std::string& unit );
    virtual void writeLatexCode ( std::ostream& f, const std::string& name, int level, const boost::filesystem::path& outputfilepath ) const;
    virtual ResultElementPtr clone() const;
};

#ifdef SWIG
%template(vectorNumericalResult) NumericalResult<arma::mat>;
#endif

class VectorResult
    : public NumericalResult<arma::mat>
{
public:
    declareType ( "VectorResult" );

    VectorResult ( const std::string& shortdesc, const std::string& longdesc, const std::string& unit );
    VectorResult ( const arma::mat& value, const std::string& shortDesc, const std::string& longDesc, const std::string& unit );
    virtual void writeLatexCode ( std::ostream& f, const std::string& name, int level, const boost::filesystem::path& outputfilepath ) const;
    virtual ResultElementPtr clone() const;
};


class TabularResult
    : public ResultElement
{
public:
    typedef std::vector<double> Row;
    typedef std::vector<Row> Table;

protected:
    std::vector<std::string> headings_;
    Table rows_;

public:
    declareType ( "TabularResult" );

    TabularResult ( const std::string& shortdesc, const std::string& longdesc, const std::string& unit );

    TabularResult
    (
        const std::vector<std::string>& headings,
        const Table& rows,
        const std::string& shortDesc,
        const std::string& longDesc,
        const std::string& unit
    );

    TabularResult
    (
        const std::vector<std::string>& headings,
        const arma::mat& rows,
        const std::string& shortDesc,
        const std::string& longDesc,
        const std::string& unit
    );

    inline const std::vector<std::string>& headings() const
    {
        return headings_;
    }
    inline const Table& rows() const
    {
        return rows_;
    }
    inline Row& appendRow()
    {
        rows_.push_back ( Row ( headings_.size() ) );
        return rows_.back();
    }
    void setCellByName ( Row& r, const std::string& colname, double value );

    arma::mat getColByName ( const std::string& colname ) const;

    arma::mat toMat() const;

    inline void setTableData ( const std::vector<std::string>& headings, const Table& rows )
    {
        headings_=headings;
        rows_=rows;
    }

    virtual void writeGnuplotData ( std::ostream& f ) const;
    virtual void writeLatexHeaderCode ( std::ostream& f ) const;
    virtual void writeLatexCode ( std::ostream& f, const std::string& name, int level, const boost::filesystem::path& outputfilepath ) const;
    virtual void exportDataToFile ( const std::string& name, const boost::filesystem::path& outputdirectory ) const;

    /**
     * append the contents of this element to the given xml node
     */
    virtual rapidxml::xml_node<>* appendToNode
    (
        const std::string& name,
        rapidxml::xml_document<>& doc,
        rapidxml::xml_node<>& node
    ) const;

    virtual ResultElementPtr clone() const;
};




class AttributeTableResult
    : public ResultElement
{
public:
    typedef std::vector<std::string> AttributeNames;
    typedef boost::variant<int, double, std::string> AttributeValue;
    typedef std::vector<AttributeValue> AttributeValues;

protected:
    AttributeNames names_;
    AttributeValues values_;

public:
    declareType ( "AttributeTableResult" );

    AttributeTableResult ( const std::string& shortdesc, const std::string& longdesc, const std::string& unit );

    AttributeTableResult
    (
        AttributeNames names,
        AttributeValues values,
        const std::string& shortDesc,
        const std::string& longDesc,
        const std::string& unit
    );

    inline void setTableData ( AttributeNames names, AttributeValues values )
    {
        names_=names;
        values_=values;
    }

    inline const AttributeNames& names() const
    {
        return names_;
    }
    inline const AttributeValues& values() const
    {
        return values_;
    }

    virtual void writeLatexCode ( std::ostream& f, const std::string& name, int level, const boost::filesystem::path& outputfilepath ) const;
    virtual void exportDataToFile ( const std::string& name, const boost::filesystem::path& outputdirectory ) const;

    /**
     * append the contents of this element to the given xml node
     */
    virtual rapidxml::xml_node<>* appendToNode
    (
        const std::string& name,
        rapidxml::xml_document<>& doc,
        rapidxml::xml_node<>& node
    ) const;

    virtual ResultElementPtr clone() const;
};




ResultElementPtr polynomialFitResult
(
  const arma::mat& coeffs, 
  const std::string& xvarName,
  const std::string& shortDesc, 
  const std::string& longDesc,
  int minorder=0
);





class ResultElementCollection
    : public std::map<std::string, ResultElementPtr>
{
public:
//     ResultElementCollection(const boost::filesystem::path & file);
    virtual ~ResultElementCollection();

#ifndef SWIG
    /**
     * insert elem into the set.
     * elem is put into a shared_ptr but not clone. So don't delete it!
     */
    ResultElement& insert ( const std::string& key, ResultElement* elem );
#endif

//   void insert(const std::string& key, std::auto_ptr<ResultElement> elem);
    ResultElement& insert ( const std::string& key, ResultElementPtr elem );

    /**
     * insert elem into the set.
     * elem is cloned.
     */
    ResultElement& insert ( const std::string& key, const ResultElement& elem );

    void writeLatexCodeOfElements ( std::ostream& f, const std::string&, int level, const boost::filesystem::path& outputfilepath ) const;

    template<class T>
    T& get ( const std::string& name );
    
    template<class T>
    const T& get ( const std::string& name ) const
    {
        return const_cast<ResultElementCollection&>(*this).get<T>(name);
    }
    
    double getScalar(const std::string& path) const;
  
    /**
     * append the result elements to the given xml node
     */
    virtual void appendToNode ( rapidxml::xml_document<>& doc, rapidxml::xml_node<>& node ) const;

    /**
     * restore the result elements from the given node
     */
    virtual void readFromNode ( rapidxml::xml_document<>& doc, rapidxml::xml_node<>& node );

    /**
     * save result set to XML file
     */
    virtual void saveToFile ( const boost::filesystem::path& file ) const;

    /**
     * read result set from xml file
     */
    virtual void readFromFile ( const boost::filesystem::path& file );

};

typedef std::shared_ptr<ResultElementCollection> ResultElementCollectionPtr;



template<class T>
T& ResultElementCollection::get ( const std::string& name )
{
  using namespace boost;
  using namespace boost::algorithm;

  if ( boost::contains ( name, "/" ) )
    {
      std::string prefix = copy_range<std::string> ( *make_split_iterator ( name, first_finder ( "/" ) ) );
      std::string remain=name;
      erase_head ( remain, prefix.size()+1 );

      std::vector<std::string> path;
      boost::split ( path, name, boost::is_any_of ( "/" ) );
      
      return this->get<ResultElementCollection>( prefix ) .get<T> ( remain );
    }
  else
    {
      iterator i = find ( name );
      if ( i==end() )
        {
          throw insight::Exception ( "Result "+name+" not found in result set" );
        }
      else
        {
          std::shared_ptr<T> pt
          ( 
            std::dynamic_pointer_cast<T>( i->second )
          );
          if ( pt )
          {
            return ( *pt );
          }
          else
          {
            throw insight::Exception ( "Parameter "+name+" not of requested type!" );
          }
        }
    }
}




class ResultSection
    : public ResultElementCollection,
      public ResultElement
{
    std::string sectionName_, introduction_;

public:
    declareType ( "ResultSection" );

    ResultSection ( const std::string& shortdesc, const std::string& longdesc, const std::string& unit );
    ResultSection ( const std::string& sectionName, const std::string& introduction=std::string() );

    virtual void writeLatexHeaderCode ( std::ostream& f ) const;
    virtual void writeLatexCode ( std::ostream& f, const std::string& name, int level, const boost::filesystem::path& outputfilepath ) const;
    virtual void exportDataToFile ( const std::string& name, const boost::filesystem::path& outputdirectory ) const;

    /**
     * append the contents of this element to the given xml node
     */
    virtual rapidxml::xml_node<>* appendToNode
    (
        const std::string& name,
        rapidxml::xml_document<>& doc,
        rapidxml::xml_node<>& node
    ) const;

    virtual std::shared_ptr<ResultElement> clone() const;
};


typedef std::shared_ptr<ResultSection> ResultSectionPtr;


class ResultSet
    :
    public ResultElementCollection,
//   public boost::ptr_map<std::string, ResultElement>,
    public ResultElement
{
protected:
    ParameterSet p_;
    std::string title_, subtitle_, date_, author_;
    std::string introduction_;

public:
    declareType ( "ResultSet" );

    ResultSet
    (
        const ParameterSet& p,
        const std::string& title,
        const std::string& subtitle,
        const std::string *author = NULL,
        const std::string *date = NULL
    );

    ResultSet ( const ResultSet& other );
    virtual ~ResultSet();

    inline std::string& introduction()
    {
        return introduction_;
    }

    inline const std::string& title() const
    {
        return title_;
    }
    inline const std::string& subtitle() const
    {
        return subtitle_;
    }

    void transfer ( const ResultSet& other );
    inline const ParameterSet& parameters() const
    {
        return p_;
    }

    virtual void writeLatexHeaderCode ( std::ostream& f ) const;
    virtual void writeLatexCode ( std::ostream& f, const std::string& name, int level, const boost::filesystem::path& outputfilepath ) const;

    virtual void exportDataToFile ( const std::string& name, const boost::filesystem::path& outputdirectory ) const;
    virtual void writeLatexFile ( const boost::filesystem::path& file ) const;
    virtual void generatePDF ( const boost::filesystem::path& file ) const;

    /**
     * append the contents of this element to the given xml node
     */
    virtual rapidxml::xml_node<>* appendToNode
    (
        const std::string& name,
        rapidxml::xml_document<>& doc,
        rapidxml::xml_node<>& node
    ) const;



    virtual ParameterSetPtr convertIntoParameterSet() const;
    virtual ParameterPtr convertIntoParameter() const;

    virtual ResultElementPtr clone() const;
};




typedef std::shared_ptr<ResultSet> ResultSetPtr;




inline ResultElement* new_clone(const ResultElement& e)
{
  return e.clone().get();
}




struct PlotCurve {
    arma::mat xy_;
    std::string plotcmd_;

    /**
     * curve identifier on plain-text-level
     */
    std::string plaintextlabel_;

    PlotCurve();
    PlotCurve ( const PlotCurve& o );

    /**
     * construct a plot curve by a gnuplot command only, i.e. a formula
     */
    PlotCurve ( const std::string& plaintextlabel, const char* plotcmd );

    /**
     * construct from separate x and y arrays (sizes have to match)
     */
    PlotCurve ( const std::vector<double>& x, const std::vector<double>& y, const std::string& plaintextlabel, const std::string& plotcmd = "" );

    /**
     * construct from separate x and y column vectors (sizes have to match)
     */
    PlotCurve ( const arma::mat& x, const arma::mat& y, const std::string& plaintextlabel, const std::string& plotcmd );

    /**
     * construct a horizontal line spanning xrange with value y
     */
    PlotCurve ( const arma::mat& xrange, double y, const std::string& plaintextlabel, const std::string& plotcmd );

    /**
     * construct from matrix containing two columns with x and y values
     */
    PlotCurve ( const arma::mat& xy, const std::string& plaintextlabel, const std::string& plotcmd = "w l" );

    /**
     * sort the curve values by x.
     * This needs to be explicitly called, because it is not always wanted (e.g. parametric plots)
     */
    void sort();

    std::string title() const;

    const arma::mat& xy() const
    {
        return xy_;
    }
    const std::string& plaintextlabel() const
    {
        return plaintextlabel_;
    }
};




struct PlotCurveList : public std::vector<PlotCurve>
{
 PlotCurveList()
  : std::vector<PlotCurve>()
 {}

 PlotCurveList(size_t n)
  : std::vector<PlotCurve>(n)
 {}

 template<class It>
 PlotCurveList(It begin, It end)
  : std::vector<PlotCurve>(begin, end)
 {}

 bool include_zero=true;
};




insight::ResultElement& addPlot
(
    std::shared_ptr<ResultElementCollection> results,
    const boost::filesystem::path& workdir,
    const std::string& resultelementname,
    const std::string& xlabel,
    const std::string& ylabel,
    const PlotCurveList& plc,
    const std::string& shortDescription,
    const std::string& addinit = "",
    const std::string& watermarktext = ""
);




class Chart
    : public ResultElement
{
protected:
    std::string xlabel_;
    std::string ylabel_;
    PlotCurveList plc_;
    std::string addinit_;

public:
    declareType ( "Chart" );
    Chart ( const std::string& shortdesc, const std::string& longdesc, const std::string& unit );
    Chart
    (
        const std::string& xlabel,
        const std::string& ylabel,
        const PlotCurveList& plc,
        const std::string& shortDesc, const std::string& longDesc,
        const std::string& addinit = ""
    );

    virtual void gnuplotCommand(gnuplotio::Gnuplot&) const;
    virtual void generatePlotImage ( const boost::filesystem::path& imagepath ) const;

    virtual void writeLatexHeaderCode ( std::ostream& f ) const;
    virtual void writeLatexCode ( std::ostream& f, const std::string& name, int level, const boost::filesystem::path& outputfilepath ) const;
    virtual void exportDataToFile ( const std::string& name, const boost::filesystem::path& outputdirectory ) const;

    /**
     * append the contents of this element to the given xml node
     */
    virtual rapidxml::xml_node<>* appendToNode
    (
        const std::string& name,
        rapidxml::xml_document<>& doc,
        rapidxml::xml_node<>& node
    ) const;

    virtual ResultElementPtr clone() const;
};


insight::ResultElement& addPolarPlot
(
    std::shared_ptr<ResultElementCollection> results,
    const boost::filesystem::path& workdir,
    const std::string& resultelementname,
    const std::string& rlabel,
    const PlotCurveList& plc,
    const std::string& shortDescription,
    double phi_unit = SI::rad,
    const std::string& addinit = "",
    const std::string& watermarktext = ""
);


class PolarChart
  : public Chart
{
 double phi_unit_;
public:
 declareType ( "PolarChart" );
 PolarChart ( const std::string& shortdesc, const std::string& longdesc, const std::string& unit );
 PolarChart
 (
     const std::string& rlabel,
     const PlotCurveList& plc,
     const std::string& shortDesc,
     const std::string& longDesc,
     double phi_unit = SI::rad,
     const std::string& addinit = ""
 );

 virtual void gnuplotCommand(gnuplotio::Gnuplot&) const;

 virtual ResultElementPtr clone() const;
};




struct PlotField {
    arma::mat xy_;
    std::string plotcmd_;

    PlotField();
    PlotField ( const arma::mat& xy, const std::string& plotcmd = "" );
};




typedef std::vector<PlotCurve> PlotFieldList;




void addContourPlot
(
    std::shared_ptr<ResultElementCollection> results,
    const boost::filesystem::path& workdir,
    const std::string& resultelementname,
    const std::string& xlabel,
    const std::string& ylabel,
    const PlotFieldList& plc,
    const std::string& shortDescription,
    const std::string& addinit = ""
);

}


#ifdef SWIG
%template(vector_PlotCurve) std::vector<insight::PlotCurve>;
#endif

#endif // INSIGHT_RESULTSET_H
