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


#include "resultset.h"
#include "base/latextools.h"
#include "base/tools.h"

#include <fstream>

#include "base/boost_include.h"
#include <algorithm>
#include "boost/bind.hpp"


#include "case.h"

#include "rapidxml/rapidxml_print.hpp"

#include "gnuplot-iostream.h"

using namespace std;
using namespace boost;
using namespace boost::filesystem;
using namespace boost::assign;
using namespace rapidxml;

namespace insight
{
  
  
    
string latex_subsection ( int level )
{
    string cmd="\\";
    if ( level==2 ) {
        cmd+="paragraph";
    } else if ( level==3 ) {
        cmd+="subparagraph";
    } else if ( level>3 ) {
        cmd="";
    } else {
        for ( int i=0; i<min ( 2,level ); i++ ) {
            cmd+="sub";
        }
        cmd+="section";
    }
    return cmd;
}




defineType(ResultElement);
defineFactoryTable
(
  ResultElement, 
  LIST(const std::string& shortdesc, const std::string& longdesc, const std::string& unit), 
  LIST(shortdesc, longdesc, unit) 
);


ResultElement::ResultElement ( const std::string& shortdesc, const std::string& longdesc, const std::string& unit )
    : shortDescription_ ( shortdesc ),
      longDescription_ ( longdesc ),
      unit_ ( unit ),
      order_ ( 0 )
{}


ResultElement::~ResultElement()
{}


const SimpleLatex& ResultElement::shortDescription() const
{
  return shortDescription_;
}


const SimpleLatex& ResultElement::longDescription() const
{
  return longDescription_;
}


const SimpleLatex& ResultElement::unit() const
{
  return unit_;
}


void ResultElement::writeLatexHeaderCode ( std::ostream& f ) const
{
}

void ResultElement::writeLatexCode ( ostream& f, const std::string& name, int level, const boost::filesystem::path& outputfilepath ) const
{
}

void ResultElement::exportDataToFile ( const std::string& name, const boost::filesystem::path& outputdirectory ) const
{
}

rapidxml::xml_node< char >* ResultElement::appendToNode
(
    const string& name,
    rapidxml::xml_document< char >& doc,
    rapidxml::xml_node< char >& node
) const
{
    using namespace rapidxml;
    xml_node<>* child = doc.allocate_node ( node_element, doc.allocate_string ( this->type().c_str() ) );
    node.append_node ( child );
    child->append_attribute ( doc.allocate_attribute
                              (
                                  "name",
                                  doc.allocate_string ( name.c_str() ) )
                            );
//   child->append_attribute(doc.allocate_attribute
//   (
//     "type",
//     doc.allocate_string( type().c_str() ))
//   );

    child->append_attribute ( doc.allocate_attribute
                              (
                                  "shortDescription",
                                  doc.allocate_string ( shortDescription_.simpleLatex().c_str() ) )
                            );
    child->append_attribute ( doc.allocate_attribute
                              (
                                  "longDescription",
                                  doc.allocate_string ( longDescription_.simpleLatex().c_str() ) )
                            );
    child->append_attribute ( doc.allocate_attribute
                              (
                                  "unit",
                                  doc.allocate_string ( unit_.simpleLatex().c_str() ) )
                            );
    child->append_attribute ( doc.allocate_attribute
                              (
                                  "order",
                                  doc.allocate_string ( str ( format ( "%g" ) % order_ ).c_str() ) )
                            );

    return child;
}


void ResultElement::readFromNode ( const string& name, rapidxml::xml_document< char >& doc, rapidxml::xml_node< char >& node )
{

}


ParameterPtr ResultElement::convertIntoParameter() const
{
    return ParameterPtr();
}





Ordering::Ordering ( double ordering_base, double ordering_step_fraction )
    : ordering_ ( ordering_base ),
      step_ ( ordering_base*ordering_step_fraction )
{}

double Ordering::next()
{
    ordering_+=step_;
    return ordering_;
}





defineType ( ResultSection );
addToFactoryTable ( ResultElement, ResultSection );

ResultSection::ResultSection ( const std::string& shortdesc, const std::string& longdesc, const std::string& unit )
    : ResultElement ( shortdesc, longdesc, unit )
{}

ResultSection::ResultSection ( const std::string& sectionName, const std::string& introduction )
    : ResultElement ( "", "", "" ),
      sectionName_ ( sectionName ),
      introduction_ ( introduction )
{}


// ResultElementCollection::ResultElementCollection(const boost::filesystem::path & file)
// {
//     readFromFile(file);
// }

void ResultElementCollection::writeLatexCodeOfElements
(
    std::ostream& f,
    const string& name,
    int level,
    const boost::filesystem::path& outputfilepath
) const
{
    std::vector<std::pair<key_type,mapped_type> > items;

//   std::transform
//   (
//     begin(),
//     end(),
//     std::back_inserter(items),
//     boost::bind(&value_type, _1) // does not work...
//   );

    std::for_each
    (
        begin(),
        end(),
        [&items] ( const value_type& p ) {
            items.push_back ( p );
        }
    );

    std::sort
    (
        items.begin(),
        items.end(),
        [] ( const value_type &left, const value_type &right ) {
              return left.second->order() < right.second->order();
          }
    );

    for ( const value_type& re: items ) {
        const ResultElement* r = & ( *re.second );

//         std::cout<<re.first<<" order="<<re.second->order() <<std::endl;

        std::string subelemname=re.first;
        if ( name!="" ) {
            subelemname=name+"__"+re.first;
        }


        if ( const ResultSection* se=dynamic_cast<const ResultSection*> ( r ) ) 
	{
            se->writeLatexCode ( f, subelemname, level+1, outputfilepath );
        } 
        else 
	{
            f << latex_subsection ( level+1 ) << "{" << SimpleLatex( re.first ).toLaTeX() << "}\n";

	    f << r->shortDescription().toLaTeX() << "\n\n";

            //     re.second->writeLatexCode(f, re.first, level+1, outputfilepath);
            r->writeLatexCode ( f, subelemname, level+2, outputfilepath );

            f << "\n\n" << r->longDescription().toLaTeX() << "\n\n";
            f << endl;
        }
    }
}

double ResultElementCollection::getScalar(const std::string& path) const 
{ 
    return this->get<NumericalResult<double> >(path).value(); 
}

void ResultElementCollection::appendToNode ( rapidxml::xml_document<>& doc, rapidxml::xml_node<>& node ) const
{
    for ( const_iterator i=begin(); i!= end(); i++ ) {
        i->second->appendToNode ( i->first, doc, node );
    }
}

void ResultElementCollection::readFromNode ( rapidxml::xml_document<>& doc, rapidxml::xml_node<>& node )
{
    for ( xml_node<> *e = node.first_node(); e; e = e->next_sibling() ) {
        std::string tname ( e->name() );
        std::string name ( e->first_attribute ( "name" )->value() );
//         std::cout<<"reading "<<name<<" of type "<<tname<<std::endl;

        ResultElementPtr re
        (
            ResultElement::lookup
            (
                tname,
                "", "", ""
            )
        );

        re->readFromNode ( name, doc, *e );
        insert ( name, re );
    }
//   for( iterator i=begin(); i!= end(); i++)
//   {
//     i->second->readFromNode(i->first, doc, node);
//   }
}





void ResultSection::writeLatexCode ( ostream& f, const string& name, int level, const path& outputfilepath ) const
{
    f << latex_subsection ( level ) << "{"<<SimpleLatex(sectionName_).toLaTeX()<<"}\n";
//   f << "\\label{" << cleanSymbols(name) << "}" << std::endl;  // problem with underscores: "\_" as returned by cleanSymbols is wrong here
    f << introduction_ << std::endl;

    writeLatexCodeOfElements ( f, name, level, outputfilepath );
}

void ResultSection::writeLatexHeaderCode ( ostream& f ) const
{
    for ( const value_type& i: *this ) {
        i.second->writeLatexHeaderCode ( f );
    }
}

void ResultSection::exportDataToFile ( const string& name, const path& outputdirectory ) const
{
    boost::filesystem::path subdir=outputdirectory/name;

    if ( !boost::filesystem::exists ( subdir ) ) {
        boost::filesystem::create_directories ( subdir );
    }

    for ( const value_type& re: *this ) {
        re.second->exportDataToFile ( re.first, subdir );
    }
}

xml_node< char >* ResultSection::appendToNode ( const string& name, xml_document< char >& doc, xml_node< char >& node ) const
{
    using namespace rapidxml;
    xml_node<>* child = ResultElement::appendToNode ( name, doc, node );

    child->append_attribute ( doc.allocate_attribute
                              (
                                  "sectionName",
                                  doc.allocate_string ( sectionName_.c_str() )
                              ) );

    child->append_attribute ( doc.allocate_attribute
                              (
                                  "introduction",
                                  doc.allocate_string ( introduction_.c_str() )
                              ) );

    ResultElementCollection::appendToNode ( doc, *child );

    return child;
}


std::shared_ptr< ResultElement > ResultSection::clone() const
{
    std::shared_ptr<ResultSection> res( new ResultSection ( sectionName_ ) );
    for ( const value_type& re: *this ) {
        ( *res ) [re.first] = re.second->clone();
    }
    res->setOrder ( order() );
    return std::dynamic_pointer_cast<ResultElement>( res );
}




defineType ( Image );
addToFactoryTable ( ResultElement, Image );


Image::Image ( const std::string& shortdesc, const std::string& longdesc, const std::string& unit )
    : ResultElement ( shortdesc, longdesc, unit )
{
}

Image::Image ( const boost::filesystem::path& location, const boost::filesystem::path& value, const std::string& shortDesc, const std::string& longDesc )
    : ResultElement ( shortDesc, longDesc, "" ),
      imagePath_ ( absolute ( value, location ) )
{
}

void Image::writeLatexHeaderCode ( std::ostream& f ) const
{
    f<<"\\usepackage{graphicx}\n";
    f<<"\\usepackage{placeins}\n";
}

void Image::writeLatexCode ( std::ostream& f, const std::string& name, int level, const boost::filesystem::path& outputfilepath ) const
{
    //f<< "\\includegraphics[keepaspectratio,width=\\textwidth]{" << cleanSymbols(imagePath_.c_str()) << "}\n";
    f<<
     "\n\nSee figure below.\n"
     "\\begin{figure}[!h]"
     "\\PlotFrame{keepaspectratio,width=\\textwidth}{" << make_relative ( outputfilepath, imagePath_ ).c_str() << "}\n"
     "\\caption{"+shortDescription_.toLaTeX()+"}\n"
     "\\end{figure}"
     "\\FloatBarrier";
}

xml_node< char >* Image::appendToNode ( const string& name, xml_document< char >& doc, xml_node< char >& node ) const
{
    using namespace rapidxml;
    xml_node<>* child = ResultElement::appendToNode ( name, doc, node );

    child->value
    (
        doc.allocate_string
        (
            base64_encode ( imagePath_ ).c_str()
        )
    );

    return child;
}


ResultElementPtr Image::clone() const
{
    ResultElementPtr res ( new Image ( imagePath_.parent_path(), imagePath_, shortDescription_.simpleLatex(), longDescription_.simpleLatex() ) );
    res->setOrder ( order() );
    return res;
}
  
  
  
  
defineType ( Comment );
addToFactoryTable ( ResultElement, Comment );


Comment::Comment ( const std::string& shortdesc, const std::string& longdesc, const std::string& unit )
    : ResultElement ( shortdesc, longdesc, unit )
{}


Comment::Comment ( const std::string& value, const std::string& shortDesc )
    : ResultElement ( shortDesc, "", "" ),
      value_ ( value )
{
}


void Comment::writeLatexCode ( std::ostream& f, const std::string& name, int level, const boost::filesystem::path& outputfilepath ) const
{
    f << value_ <<endl;
}


void Comment::exportDataToFile ( const string& name, const path& outputdirectory ) const
{
    boost::filesystem::path fname ( outputdirectory/ ( name+".txt" ) );
    std::ofstream f ( fname.c_str() );
    f<<value_;
}


xml_node< char >* Comment::appendToNode ( const string& name, xml_document< char >& doc, xml_node< char >& node ) const
{
    using namespace rapidxml;
    xml_node<>* child = ResultElement::appendToNode ( name, doc, node );

    child->append_attribute ( doc.allocate_attribute
                              (
                                  "value",
                                  doc.allocate_string ( value_.c_str() )
                              ) );

    return child;
}


ResultElementPtr Comment::clone() const
{
    ResultElementPtr res ( new Comment ( value_, shortDescription_.simpleLatex() ) );
    res->setOrder ( order() );
    return res;
}
  
  
  
  
defineType ( ScalarResult );
addToFactoryTable ( ResultElement, ScalarResult );


ScalarResult::ScalarResult ( const std::string& shortdesc, const std::string& longdesc, const std::string& unit )
    : NumericalResult< double > ( shortdesc, longdesc, unit )
{
}


ScalarResult::ScalarResult ( const double& value, const string& shortDesc, const string& longDesc, const string& unit )
    : NumericalResult< double > ( value, shortDesc, longDesc, unit )
{}


void ScalarResult::writeLatexCode ( ostream& f, const std::string& name, int level, const boost::filesystem::path& outputfilepath ) const
{
//   f.setf(ios::fixed,ios::floatfield);
//   f.precision(3);
    f << str ( format ( "%g" ) % value_ ) << unit_.toLaTeX();
}


ResultElementPtr ScalarResult::clone() const
{
    ResultElementPtr res ( new ScalarResult ( value_, shortDescription_.simpleLatex(), longDescription_.simpleLatex(), unit_.simpleLatex() ) );
    res->setOrder ( order() );
    return res;
}




defineType ( VectorResult );
addToFactoryTable ( ResultElement, VectorResult );


VectorResult::VectorResult ( const std::string& shortdesc, const std::string& longdesc, const std::string& unit )
    : NumericalResult< arma::mat > ( shortdesc, longdesc, unit )
{
}


VectorResult::VectorResult ( const arma::mat& value, const string& shortDesc, const string& longDesc, const string& unit )
    : NumericalResult< arma::mat > ( value, shortDesc, longDesc, unit )
{}


void VectorResult::writeLatexCode ( ostream& f, const std::string& name, int level, const boost::filesystem::path& outputfilepath ) const
{
//   f.setf(ios::fixed,ios::floatfield);
//   f.precision(3);
    f << str ( format ( "(%g %g %g)" ) % value_(0) % value_(1) % value_(2) ) << unit_.toLaTeX();
}


ResultElementPtr VectorResult::clone() const
{
    ResultElementPtr res ( new VectorResult ( value_, shortDescription_.simpleLatex(), longDescription_.simpleLatex(), unit_.simpleLatex() ) );
    res->setOrder ( order() );
    return res;
}



defineType ( TabularResult );
addToFactoryTable ( ResultElement, TabularResult );


TabularResult::TabularResult ( const std::string& shortdesc, const std::string& longdesc, const std::string& unit )
    : ResultElement ( shortdesc, longdesc, unit )
{
}


TabularResult::TabularResult
(
    const std::vector<std::string>& headings,
    const Table& rows,
    const std::string& shortDesc,
    const std::string& longDesc,
    const std::string& unit
)
    : ResultElement ( shortDesc, longDesc, unit )
{
    setTableData ( headings, rows );
}


TabularResult::TabularResult
(
    const std::vector<std::string>& headings,
    const arma::mat& rows,
    const std::string& shortDesc,
    const std::string& longDesc,
    const std::string& unit
)
    : ResultElement ( shortDesc, longDesc, unit )
{
    Table t;
    for ( int i=0; i<rows.n_rows; i++ ) {
        Row r;
        for ( int j=0; j<rows.n_cols; j++ ) {
            r.push_back ( rows ( i,j ) );
        }
        t.push_back ( r );
    }
    setTableData ( headings, t );
}


void TabularResult::setCellByName ( TabularResult::Row& r, const string& colname, double value )
{
    std::vector<std::string>::const_iterator ii=std::find ( headings_.begin(), headings_.end(), colname );
    if ( ii==headings_.end() ) {
        std::ostringstream msg;
        msg<<"Tried to write into column "+colname+" but this does not exist! Existing columns are:"<<endl;
        for ( const std::string& n: headings_ ) {
            msg<<n<<endl;
        }
        insight::Exception ( msg.str() );
    }
    int i= ii - headings_.begin();
    r[i]=value;
}


arma::mat TabularResult::getColByName ( const string& colname ) const
{
    std::vector<std::string>::const_iterator ii=std::find ( headings_.begin(), headings_.end(), colname );
    if ( ii==headings_.end() ) {
        std::ostringstream msg;
        msg<<"Tried to get column "+colname+" but this does not exist! Existing columns are:"<<endl;
        for ( const std::string& n: headings_ ) {
            msg<<n<<endl;
        }
        insight::Exception ( msg.str() );
    }
    int i= ii - headings_.begin();
    return toMat().col ( i );
}


arma::mat TabularResult::toMat() const
{
    arma::mat res;
    res.resize ( rows_.size(), rows_[0].size() );
    int i=0;
    for ( const std::vector<double>& row: rows_ ) {
        int j=0;
        for ( double v: row ) {
//             cout<<"res("<<i<<","<<j<<")="<<v<<endl;
            res ( i, j++ ) =v;
        }
        i++;
    }
    return res;
}


void TabularResult::writeGnuplotData ( std::ostream& f ) const
{
    f<<"#";
    for ( const std::string& head: headings_ ) {
        f<<" \""<<head<<"\"";
    }
    f<<std::endl;

    for ( const std::vector<double>& row: rows_ ) {
        for ( double v: row ) {
            f<<" "<<v;
        }
        f<<std::endl;
    }

}


ResultElementPtr TabularResult::clone() const
{
    ResultElementPtr res ( new TabularResult ( headings_, rows_, shortDescription_.simpleLatex(), longDescription_.simpleLatex(), unit_.simpleLatex() ) );
    res->setOrder ( order() );
    return res;
}


void TabularResult::writeLatexHeaderCode ( ostream& f ) const
{
    insight::ResultElement::writeLatexHeaderCode ( f );
    f<<"\\usepackage{longtable}\n";
    f<<"\\usepackage{placeins}\n";
}


void TabularResult::writeLatexCode ( std::ostream& f, const std::string& name, int level, const boost::filesystem::path& outputfilepath ) const
{
  std::vector<std::vector<int> > colsets;
  int i=0;
  std::vector<int> ccolset;
  for(int c=0; c<headings_.size(); c++)
  {
    ccolset.push_back(c);
    i++;
    if (i>5) { colsets.push_back(ccolset); ccolset.clear(); i=0; }
  }
  if (ccolset.size()>0) colsets.push_back(ccolset);

  for (const std::vector<int>& cols: colsets)
  {
    f<<
     "\\begin{longtable}{";
    for (int c: cols) {
        f<<"c";
    }
    f<<"}\n";

    for (int i=0; i<cols.size(); i++) {
        if ( i!=0 ) {
            f<<" & ";
        }
        f<<headings_[cols[i]];
    }
    f<<
     "\\\\\n"
     "\\hline\n"
     "\\endfirsthead\n"
     "\\endhead\n";
    for ( TabularResult::Table::const_iterator i=rows_.begin(); i!=rows_.end(); i++ ) {
        if ( i!=rows_.begin() ) {
            f<<"\\\\\n";
        }
//        for ( std::vector<double>::const_iterator j=i->begin(); j!=i->end(); j++ ) {
        for (int j=0; j< cols.size(); j++) {
            if ( j!=0 ) {
                f<<" & ";
            }
            if ( !std::isnan ( (*i)[cols[j]] ) ) {
                f<<(*i)[cols[j]];
            }
        }
    }
    f<<
     "\\end{longtable}\n\n"
//     "\\newpage\n"  // page break algorithm fails after too short "longtable"
        ;
  }
}


xml_node< char >* TabularResult::appendToNode ( const string& name, xml_document< char >& doc, xml_node< char >& node ) const
{
    using namespace rapidxml;
    xml_node<>* child = ResultElement::appendToNode ( name, doc, node );

    xml_node<>* heads = doc.allocate_node ( node_element, doc.allocate_string ( "headings" ) );
    child->append_node ( heads );
    for ( size_t i=0; i<headings_.size(); i++ ) {
        xml_node<>* chead = doc.allocate_node ( node_element, doc.allocate_string ( str ( format ( "header_%i" ) %i ).c_str() ) );
        heads->append_node ( chead );

        chead->append_attribute ( doc.allocate_attribute
                                  (
                                      "title",
                                      doc.allocate_string ( headings_[i].c_str() )
                                  ) );
    }

    xml_node<>* values = doc.allocate_node ( node_element, doc.allocate_string ( "values" ) );
    child->append_node ( values );
    writeMatToXMLNode ( toMat(), doc, *values );

    return child;
}


void TabularResult::exportDataToFile ( const string& name, const path& outputdirectory ) const
{
    boost::filesystem::path fname ( outputdirectory/ ( name+".csv" ) );
    std::ofstream f ( fname.c_str() );

    std::string sep="";
    for ( const std::string& h: headings_ ) {
        f<<sep<<"\""<<h<<"\"";
        sep=";";
    }
    f<<endl;

    for ( const Row& r: rows_ ) {
        sep="";
        for ( const double& v: r ) {
            f<<sep<<v;
            sep=";";
        }
        f<<endl;
    }
}




defineType ( AttributeTableResult );
addToFactoryTable ( ResultElement, AttributeTableResult );


AttributeTableResult::AttributeTableResult ( const std::string& shortdesc, const std::string& longdesc, const std::string& unit )
    : ResultElement ( shortdesc, longdesc, unit )
{
}


AttributeTableResult::AttributeTableResult
(
    AttributeNames names,
    AttributeValues values,
    const std::string& shortDesc,
    const std::string& longDesc,
    const std::string& unit
)
    : ResultElement ( shortDesc, longDesc, unit )
{
    setTableData ( names, values );
}


void AttributeTableResult::writeLatexCode ( std::ostream& f, const std::string& name, int level, const boost::filesystem::path& outputfilepath ) const
{
    f<<
     "\\begin{tabular}{lc}\n"
     "Attribute & Value \\\\\n"
     "\\hline\\\\";
    for ( int i=0; i<names_.size(); i++ ) {
        f<<names_[i]<<" & "<<values_[i]<<"\\\\"<<endl;
    }
    f<<"\\end{tabular}\n";
}


void AttributeTableResult::exportDataToFile ( const string& name, const path& outputdirectory ) const
{
    boost::filesystem::path fname ( outputdirectory/ ( name+".csv" ) );
    std::ofstream f ( fname.c_str() );

    for ( int i=0; i<names_.size(); i++ ) {
        f<<"\""<<names_[i]<<"\";"<<values_[i]<<endl;
    }
}


xml_node< char >* AttributeTableResult::appendToNode ( const string& name, xml_document< char >& doc, xml_node< char >& node ) const
{
    using namespace rapidxml;
    xml_node<>* child = ResultElement::appendToNode ( name, doc, node );

    for ( size_t i=0; i<names_.size(); i++ ) {
        xml_node<>* cattr = doc.allocate_node ( node_element, doc.allocate_string ( str ( format ( "attribute_%i" ) %i ).c_str() ) );
        child->append_node ( cattr );

        cattr->append_attribute ( doc.allocate_attribute
                                  (
                                      "name",
                                      doc.allocate_string ( names_[i].c_str() )
                                  ) );

        if ( const int* v = boost::get<int> ( &values_[i] ) ) {
            cattr->append_attribute ( doc.allocate_attribute (
                                          "type", doc.allocate_string ( "int" )
                                      ) );
            cattr->append_attribute ( doc.allocate_attribute (
                                          "value", doc.allocate_string ( boost::lexical_cast<std::string> ( *v ).c_str() )
                                      ) );
        } else if ( const double* v = boost::get<double> ( &values_[i] ) ) {
            cattr->append_attribute ( doc.allocate_attribute (
                                          "type", doc.allocate_string ( "double" )
                                      ) );
            cattr->append_attribute ( doc.allocate_attribute (
                                          "value", doc.allocate_string ( boost::lexical_cast<std::string> ( *v ).c_str() )
                                      ) );
        } else if ( const std::string* v = boost::get<std::string> ( &values_[i] ) ) {
            cattr->append_attribute ( doc.allocate_attribute (
                                          "type", doc.allocate_string ( "string" )
                                      ) );
            cattr->append_attribute ( doc.allocate_attribute (
                                          "value", doc.allocate_string ( v->c_str() )
                                      ) );
        }

    }

    return child;
}


ResultElementPtr AttributeTableResult::clone() const
{
    ResultElementPtr res ( new AttributeTableResult ( names_, values_,
                           shortDescription_.simpleLatex(), longDescription_.simpleLatex(), unit_.simpleLatex() ) );
    res->setOrder ( order() );
    return res;
}





ResultElementPtr polynomialFitResult
(
    const arma::mat& coeffs,
    const std::string& xvarName,
    const std::string& shortDesc,
    const std::string& longDesc,
    int minorder
)
{
    std::vector<std::string> header=boost::assign::list_of ( "Term" ) ( "Coefficient" );
    AttributeTableResult::AttributeNames names;
    AttributeTableResult::AttributeValues values;

    for ( int i=coeffs.n_rows-1; i>=0; i-- ) {
        int order=minorder+i;
        if ( order==0 ) {
            names.push_back ( "$1$" );
        } else if ( order==1 ) {
            names.push_back ( "$"+xvarName+"$" );
        } else {
            names.push_back ( "$"+xvarName+"^{"+lexical_cast<string> ( order )+"}$" );
        }
        values.push_back ( coeffs ( i ) );
    }

    return ResultElementPtr
           (
               new AttributeTableResult
               (
                   names, values,
                   shortDesc, longDesc, ""
               )
           );
}
  
  
  

defineType ( ResultSet );


ResultSet::ResultSet
(
    const ParameterSet& p,
    const std::string& title,
    const std::string& subtitle,
    const std::string* date,
    const std::string* author
)
    : ResultElement ( "", "", "" ),
      p_ ( p ),
      title_ ( title ),
      subtitle_ ( subtitle ),
      introduction_()
{
    if ( date ) {
        date_=*date;
    } else {
        using namespace boost::gregorian;
        date_=to_iso_extended_string ( day_clock::local_day() );
    }

    if ( author ) {
        author_=*author;
    } else {
        char  *iu=getenv ( "INSIGHT_REPORT_AUTHOR" );
        if ( iu ) {
            author_=iu;
        } else {
            char* iua=getenv ( "USER" );
            if ( iua ) {
                author_=iua;
            } else {
                author_="";
            }
        }
    }
}


ResultSet::~ResultSet()
{}


ResultSet::ResultSet ( const ResultSet& other )
    : //ptr_map< std::string, ResultElement>(other),
    ResultElementCollection ( other ),
    ResultElement ( "", "", "" ),
    p_ ( other.p_ ),
    title_ ( other.title_ ),
    subtitle_ ( other.subtitle_ ),
    author_ ( other.author_ ),
    date_ ( other.date_ ),
    introduction_ ( other.introduction_ )
{
}


void ResultSet::transfer ( const ResultSet& other )
{
//   ptr_map< std::string, ResultElement>::operator=(other);
    std::map< std::string, ResultElementPtr>::operator= ( other );
    p_=other.p_;
    title_=other.title_;
    subtitle_=other.subtitle_;
    author_=other.author_;
    date_=other.date_;
    introduction_=other.introduction_;
}


void ResultSet::writeLatexHeaderCode ( std::ostream& f ) const
{
    for ( ResultSet::const_iterator i=begin(); i!=end(); i++ ) {
        i->second->writeLatexHeaderCode ( f );
    }
}


void ResultSet::writeLatexCode ( std::ostream& f, const std::string& name, int level, const boost::filesystem::path& outputfilepath ) const
{
    if ( level>0 ) {
        f << title_ << "\n\n";

        f << subtitle_ << "\n\n";
    }

    if ( !introduction_.empty() ) {
        f << latex_subsection ( level ) << "{Introduction}\n";

        f<<introduction_;
    }

    if ( p_.size() >0 ) {
        f << latex_subsection ( level ) << "{Input Parameters}\n";

        f<<p_.latexRepresentation();
    }

    f << latex_subsection ( level ) << "{Numerical Result Summary}\n";

    writeLatexCodeOfElements ( f, name, level, outputfilepath );

//   for (ResultSet::const_iterator i=begin(); i!=end(); i++)
//   {
//     f << latex_subsection(level+1) << "{" << cleanSymbols(i->first) << "}\n";
//     f << cleanSymbols(i->second->shortDescription()) << "\n\n";
//
//     std::string subelemname=i->first;
//     if (name!="")
//       subelemname=name+"__"+i->first;
//
//     i->second->writeLatexCode(f, subelemname, level+2, outputfilepath);
//
//     f << "\n\n" << cleanSymbols(i->second->longDescription()) << "\n\n";
//     f << endl;
//   }
}


xml_node< char >* ResultSet::appendToNode ( const string& name, xml_document< char >& doc, xml_node< char >& node ) const
{
    using namespace rapidxml;
    xml_node<>* child = ResultElement::appendToNode ( name, doc, node );

    ResultElementCollection::appendToNode ( doc, *child );

    return child;
}


void ResultSet::exportDataToFile ( const std::string& name, const boost::filesystem::path& outputdirectory ) const
{
    path outsubdir ( outputdirectory/name );
    create_directory ( outsubdir );
    for ( ResultSet::const_iterator i=begin(); i!=end(); i++ ) {
        i->second->exportDataToFile ( i->first, outsubdir );
    }
}


std::string builtin_template=
    "\\documentclass[a4paper,10pt]{scrartcl}\n"
    "\\usepackage{hyperref}\n"
    "\\usepackage{fancyhdr}\n"
    "\\pagestyle{fancy}\n"
    "###HEADER###\n"
    "\\begin{document}\n"
    "\\title{###TITLE###\\\\\n"
    "\\vspace{0.5cm}\\normalsize{###SUBTITLE###}}\n"
    "\\date{###DATE###}\n"
    "\\author{###AUTHOR###}\n"
    "\\maketitle\n"
    "\\tableofcontents\n"
    "###CONTENT###\n"
    "\\end{document}\n";


void ResultSet::writeLatexFile ( const boost::filesystem::path& file ) const
{
    path filepath ( absolute ( file ) );

    std::ostringstream header, content;

    header<<"\\newcommand{\\PlotFrameB}[2]{%\n"
          <<"\\includegraphics[#1]{#2}\\endgroup}\n"
          <<"\\def\\PlotFrame{\\begingroup\n"
          <<"\\catcode`\\_=12\n"
          <<"\\PlotFrameB}\n"

          <<"\\usepackage{enumitem}\n"
          "\\setlist[enumerate]{label*=\\arabic*.}\n"
          "\\renewlist{enumerate}{enumerate}{10}\n"

          ;
    writeLatexHeaderCode ( header );

    writeLatexCode ( content, "", 0, filepath.parent_path() );

    // insert into template
    std::string file_content=builtin_template;

    std::string envvarname="INSIGHT_REPORT_TEMPLATE";
    if ( char *TEMPL=getenv ( envvarname.c_str() ) ) {
        std::ifstream in ( TEMPL );
        in.seekg ( 0, std::ios::end );
        file_content.resize ( in.tellg() );
        in.seekg ( 0, std::ios::beg );
        in.read ( &file_content[0], file_content.size() );
    }

    boost::replace_all ( file_content, "###AUTHOR###", author_ );
    boost::replace_all ( file_content, "###DATE###", date_ );
    boost::replace_all ( file_content, "###TITLE###", title_ );
    boost::replace_all ( file_content, "###SUBTITLE###", subtitle_ );

    boost::replace_all ( file_content, "###HEADER###", header.str() );
    boost::replace_all ( file_content, "###CONTENT###", content.str() );

    {
        std::ofstream f ( filepath.c_str() );
        f<<file_content;
    }

    {
        path outdir ( filepath.parent_path() / ( "report_data_"+filepath.stem().string() ) );
        create_directory ( outdir );
        for ( ResultSet::const_iterator i=begin(); i!=end(); i++ ) {
            i->second->exportDataToFile ( i->first, outdir );
        }
    }
}

void ResultSet::generatePDF ( const boost::filesystem::path& file ) const
{
  std::string stem = file.filename().stem().string();

  {
      path outdir ( file.parent_path() / ( "report_data_"+stem ) );
      create_directory ( outdir );
      for ( ResultSet::const_iterator i=begin(); i!=end(); i++ ) {
          i->second->exportDataToFile ( i->first, outdir );
      }
  }

  TemporaryCaseDir gendir;
  boost::filesystem::path outpath = gendir.dir / (stem+".tex");
  writeLatexFile( outpath );

  for (int i=0; i<2; i++)
  {
      if ( ::system( str( format("cd \"%s\" && pdflatex -interaction=batchmode \"%s\"") % gendir.dir.string() % outpath.filename().string() ).c_str() ))
      {
          throw insight::Exception("TeX input file was written but could not execute pdflatex successfully.");
      }
  }

  boost::filesystem::copy_file( gendir.dir/ (stem+".pdf"), file, copy_option::overwrite_if_exists );

}


void ResultElementCollection::saveToFile ( const boost::filesystem::path& file ) const
{
//   std::cout<<"Writing result set to file "<<file<<std::endl;

    xml_document<> doc;

    // xml declaration
    xml_node<>* decl = doc.allocate_node ( node_declaration );
    decl->append_attribute ( doc.allocate_attribute ( "version", "1.0" ) );
    decl->append_attribute ( doc.allocate_attribute ( "encoding", "utf-8" ) );
    doc.append_node ( decl );

    xml_node<> *rootnode = doc.allocate_node ( node_element, "root" );
    doc.append_node ( rootnode );

//   if (analysisName != "")
//   {
//     xml_node<> *analysisnamenode = doc.allocate_node(node_element, "analysis");
//     rootnode->append_node(analysisnamenode);
//     analysisnamenode->append_attribute(doc.allocate_attribute
//     (
//       "name",
//       doc.allocate_string(analysisName.c_str())
//     ));
//   }

    ResultElementCollection::appendToNode ( doc, *rootnode );

    {
        std::ofstream f ( file.c_str() );
        f << doc << std::endl;
        f << std::flush;
        f.close();
    }
}


void ResultElementCollection::readFromFile ( const boost::filesystem::path& file )
{
    std::ifstream in ( file.c_str() );
    std::string contents;
    in.seekg ( 0, std::ios::end );
    contents.resize ( in.tellg() );
    in.seekg ( 0, std::ios::beg );
    in.read ( &contents[0], contents.size() );
    in.close();

    xml_document<> doc;
    doc.parse<0> ( &contents[0] );

    xml_node<> *rootnode = doc.first_node ( "root" );

//   std::string analysisName;
//   xml_node<> *analysisnamenode = rootnode->first_node("analysis");
//   if (analysisnamenode)
//   {
//     analysisName = analysisnamenode->first_attribute("name")->value();
//   }

    ResultElementCollection::readFromNode ( doc, *rootnode );

//   return analysisName;
}


ParameterSetPtr ResultSet::convertIntoParameterSet() const
{
    ParameterSetPtr ps ( new ParameterSet() );
    for ( const_iterator::value_type rp: *this ) {
        ParameterPtr p=rp.second->convertIntoParameter();
        if ( p ) {
            std::string key=rp.first;
            ps->insert ( key, p->clone() );
        }
    }
    return ps;
}


ParameterPtr ResultSet::convertIntoParameter() const
{
    ParameterPtr ps ( new SubsetParameter() );
    static_cast<SubsetParameter*> ( ps.get() )->setParameterSet ( *convertIntoParameterSet() );
    return ps;
}





ResultElementCollection::~ResultElementCollection()
{}


ResultElement& ResultElementCollection::insert ( const string& key, ResultElement* elem )
{
    std::pair< iterator, bool > res=
        std::map<std::string, ResultElementPtr>::insert ( ResultSet::value_type ( key, ResultElementPtr ( elem ) ) );
    return * ( *res.first ).second;
}

// void ResultSet::insert(const string& key, auto_ptr< ResultElement > elem)
// {
//   this->insert(ResultSet::value_type(key, ResultElementPtr(elem.release())));
// }


ResultElement& ResultElementCollection::insert ( const string& key, ResultElementPtr elem )
{
    std::pair< iterator, bool > res=
        std::map<std::string, ResultElementPtr>::insert ( ResultSet::value_type ( key, elem ) );
    return * ( *res.first ).second;
}


ResultElement& ResultElementCollection::insert ( const string& key, const ResultElement& elem )
{
    std::pair< iterator, bool > res=
        std::map<std::string, ResultElementPtr>::insert ( ResultSet::value_type ( key, elem.clone() ) );
    return * ( *res.first ).second;
}



ResultElementPtr ResultSet::clone() const
{
    std::auto_ptr<ResultSet> nr ( new ResultSet ( p_, title_, subtitle_, &author_, &date_ ) );
    for ( ResultSet::const_iterator i=begin(); i!=end(); i++ ) {
//         cout<<i->first<<endl;
        std::string key ( i->first );
        nr->insert ( key, i->second->clone() );
    }
    nr->setOrder ( order() );
    nr->introduction() =introduction_;
    return ResultElementPtr ( nr.release() );
}




PlotCurve::PlotCurve()
{
}

PlotCurve::PlotCurve(const PlotCurve& o)
: xy_(o.xy_), plotcmd_(o.plotcmd_), plaintextlabel_(o.plaintextlabel_)
{}


PlotCurve::PlotCurve(const std::string& plaintextlabel, const char* plotcmd)
: plotcmd_(plotcmd), plaintextlabel_(plaintextlabel)
{
}

PlotCurve::PlotCurve(const std::vector<double>& x, const std::vector<double>& y, const std::string& plaintextlabel, const std::string& plotcmd)
: plotcmd_(plotcmd), plaintextlabel_(plaintextlabel)
{
  if (x.size()!=y.size())
  {
      throw insight::Exception
      ( 
        boost::str(boost::format("plot curve %s: number of point x (%d) != number of points y (%d)!")
          % plaintextlabel_ % x.size() % y.size() )
      );
  }

  xy_ = join_rows( arma::mat(x.data(), x.size(), 1), arma::mat(y.data(), y.size(), 1) );
}

PlotCurve::PlotCurve(const arma::mat& x, const arma::mat& y, const std::string& plaintextlabel, const std::string& plotcmd)
: plotcmd_(plotcmd), 
  plaintextlabel_(plaintextlabel)
{
  if (x.n_rows!=y.n_rows)
  {
      throw insight::Exception
      ( 
        boost::str(boost::format("plot curve %s: number of point x (%d) != number of points y (%d)!")
          % plaintextlabel_ % x.n_rows % y.n_rows )
      );
  }
  xy_ = join_rows(x, y);
}

PlotCurve::PlotCurve ( const arma::mat& xrange, double y, const std::string& plaintextlabel, const std::string& plotcmd )
: plotcmd_(plotcmd), plaintextlabel_(plaintextlabel)
{
    xy_ 
     << arma::as_scalar(arma::min(xrange)) << y << arma::endr
     << arma::as_scalar(arma::max(xrange)) << y << arma::endr
     ;
}


PlotCurve::PlotCurve(const arma::mat& xy, const std::string& plaintextlabel, const std::string& plotcmd)
: xy_(xy), plotcmd_(plotcmd), plaintextlabel_(plaintextlabel)
{}

void PlotCurve::sort()
{
  xy_=sortedByCol(xy_, 0);
}


std::string PlotCurve::title() const
{
  boost::regex re(".*t *'(.*)'.*");
  boost::smatch str_matches;
  if (boost::regex_match(plotcmd_, str_matches, re))
  {
//     std::cout<<" <> "<<str_matches[1]<<std::endl;
    return str_matches[1];
  }
  else return "";
}




insight::ResultElement& addPlot
(
    std::shared_ptr<ResultElementCollection> results,
    const boost::filesystem::path& workdir,
    const std::string& resultelementname,
    const std::string& xlabel,
    const std::string& ylabel,
    const PlotCurveList& plc,
    const std::string& shortDescription,
    const std::string& addinit,
    const std::string& watermarktext
)
{
    std::string precmd=addinit+";";
    if ( watermarktext!="" ) {
        precmd+=
	  "set label "
	  "'"+SimpleLatex( watermarktext ).toLaTeX()+"'"
	  " center at screen 0.5, 0.5 tc rgb\"#cccccc\" rotate by 30 font \",24\";"
	  ;
    }

    return results->insert ( resultelementname,
                             new Chart
                             (
                                 xlabel, ylabel, plc,
                                 shortDescription, "",
                                 precmd
                             ) );
}




insight::ResultElement& addPolarPlot
(
    std::shared_ptr<ResultElementCollection> results,
    const boost::filesystem::path& workdir,
    const std::string& resultelementname,
    const std::string& rlabel,
    const PlotCurveList& plc,
    const std::string& shortDescription,
    double phi_unit,
    const std::string& addinit,
    const std::string& watermarktext
)
{
    std::string precmd=addinit+";";
    if ( watermarktext!="" ) {
        precmd+=
          "set label "
          "'"+SimpleLatex( watermarktext ).toLaTeX()+"'"
          " center at screen 0.5, 0.5 tc rgb\"#cccccc\" rotate by 30 font \",24\";"
          ;
    }

    return results->insert ( resultelementname,
                             new PolarChart
                             (
                                 rlabel, plc,
                                 shortDescription, "",
                                 phi_unit,
                                 precmd
                             ) );
}




defineType(Chart);
addToFactoryTable(ResultElement, Chart);



Chart::Chart(const std::string& shortdesc, const std::string& longdesc, const std::string& unit)
: ResultElement(shortdesc, longdesc, unit)
{
}



Chart::Chart
(
  const std::string& xlabel,
  const std::string& ylabel,
  const PlotCurveList& plc,
  const std::string& shortDesc, const std::string& longDesc,
  const std::string& addinit
)
: ResultElement(shortDesc, longDesc, ""),
  xlabel_(xlabel),
  ylabel_(ylabel),
  plc_(plc),
  addinit_(addinit)
{
}


void Chart::gnuplotCommand(gnuplotio::Gnuplot& gp) const
{
 gp<<addinit_<<";";
 gp<<"set xlabel '"<<xlabel_<<"'; set ylabel '"<<ylabel_<<"'; set grid; ";
 if ( plc_.size() >0 )
 {
  gp<<"plot ";
  bool is_first=true;

  if (plc_.include_zero)
  {
   gp<<"0 not lt -1";
   is_first=false;
  }

  for ( const PlotCurve& pc: plc_ )
  {
   if ( !pc.plotcmd_.empty() )
   {
    if (!is_first) { gp << ","; is_first=false; }
    if ( pc.xy_.n_rows>0 )
    {
     gp<<"'-' "<<pc.plotcmd_;
    } else
    {
     gp<<pc.plotcmd_;
    }
   }
  }

  gp<<endl;

  for ( const PlotCurve& pc: plc_ )
  {
   if ( pc.xy_.n_rows>0 )
   {
    gp.send1d ( pc.xy_ );
   }
  }

 }
}



void Chart::generatePlotImage ( const path& imagepath ) const
{
//   std::string chart_file_name=(workdir/(resultelementname+".png")).string();
    std::string bn ( imagepath.filename().stem().string() );

    TemporaryCaseDir tmp ( false, bn+"-generate" );

    {
        Gnuplot gp;

        //gp<<"set terminal pngcairo; set termoption dash;";
        gp<<"set terminal epslatex standalone color dash linewidth 3 header \"\\\\usepackage{graphicx}\\n\\\\usepackage{epstopdf}\";";
        gp<<"set output '"+bn+".tex';";
//     gp<<"set output '"<<absolute(imagepath).string()<<"';";
        /*
            gp<<"set linetype  1 lc rgb '#0000FF' lw 1;"
        	"set linetype  2 lc rgb '#8A2BE2' lw 1;"
        	"set linetype  3 lc rgb '#A52A2A' lw 1;"
        	"set linetype  4 lc rgb '#E9967A' lw 1;"
        	"set linetype  5 lc rgb '#5F9EA0' lw 1;"
        	"set linetype  6 lc rgb '#006400' lw 1;"
        	"set linetype  7 lc rgb '#8B008B' lw 1;"
        	"set linetype  8 lc rgb '#696969' lw 1;"
        	"set linetype  9 lc rgb '#DAA520' lw 1;"
        	"set linetype cycle  9;";
        */

       gnuplotCommand(gp);
    }

    ::system (
        (
            "mv "+bn+".tex "+ ( tmp.dir/ ( bn+".tex" ) ).string()+"; "
            "mv "+bn+"-inc.eps "+ ( tmp.dir/ ( bn+"-inc.eps" ) ).string()+"; "
            "cd "+tmp.dir.string()+"; "
            "pdflatex -interaction=batchmode -shell-escape "+bn+".tex; "
            "convert -density 600 "+bn+".pdf "+absolute ( imagepath ).string()
        ).c_str() );
}

  
void Chart::writeLatexHeaderCode(std::ostream& f) const
{
  f<<"\\usepackage{graphicx}\n";
  f<<"\\usepackage{placeins}\n";
}


void Chart::writeLatexCode ( std::ostream& f, const std::string& name, int level, const boost::filesystem::path& outputfilepath ) const
{
    path chart_file=cleanLatexImageFileName ( outputfilepath/ ( name+".png" ) ).string();

    generatePlotImage ( chart_file );

    //f<< "\\includegraphics[keepaspectratio,width=\\textwidth]{" << cleanSymbols(imagePath_.c_str()) << "}\n";
    f<<
     "\n\nSee figure below.\n"
     "\\begin{figure}[!h]"
     "\\PlotFrame{keepaspectratio,width=\\textwidth}{" << make_relative ( outputfilepath, chart_file ).c_str() << "}\n"
     "\\caption{"+shortDescription_.toLaTeX()+"}\n"
     "\\end{figure}"
     "\\FloatBarrier";
}


void Chart::exportDataToFile ( const std::string& name, const boost::filesystem::path& outputdirectory ) const
{
    int curveID=0;
    for ( const PlotCurve& pc: plc_ ) {
        std::string suf=pc.plaintextlabel();
        replace_all ( suf, "/", "_" );
        if ( suf=="" ) {
            suf=str ( format ( "curve%d" ) %curveID );
        }

        boost::filesystem::path fname ( outputdirectory/ ( name+"__"+suf+".xy" ) );

        std::ofstream f ( fname.c_str() );
        pc.xy_.save ( fname.string(), arma::raw_ascii );
        curveID++;
    }
}


rapidxml::xml_node<>* Chart::appendToNode
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
                                  "xlabel",
                                  doc.allocate_string ( xlabel_.c_str() )
                              ) );
    child->append_attribute ( doc.allocate_attribute
                              (
                                  "ylabel",
                                  doc.allocate_string ( xlabel_.c_str() )
                              ) );
    child->append_attribute ( doc.allocate_attribute
                              (
                                  "addinit",
                                  doc.allocate_string ( xlabel_.c_str() )
                              ) );

    for ( const PlotCurve& pc: plc_ ) {
        xml_node<> *pcnode = doc.allocate_node
                             (
                                 node_element,
                                 "PlotCurve"
                             );
        child->append_node ( pcnode );

        pcnode->append_attribute
        (
            doc.allocate_attribute
            (
                "plaintextlabel",
                doc.allocate_string ( pc.plaintextlabel().c_str() )
            )
        );

        pcnode->append_attribute
        (
            doc.allocate_attribute
            (
                "plotcmd",
                doc.allocate_string ( pc.plotcmd_.c_str() )
            )
        );

        writeMatToXMLNode ( pc.xy_, doc, *pcnode );
    }

    return child;
}



  
ResultElementPtr Chart::clone() const
{
    ResultElementPtr res ( new Chart ( xlabel_, ylabel_, plc_, shortDescription().simpleLatex(), longDescription().simpleLatex(), addinit_ ) );
    res->setOrder ( order() );
    return res;
}





defineType(PolarChart);
addToFactoryTable(ResultElement, PolarChart);



PolarChart::PolarChart(const std::string& shortdesc, const std::string& longdesc, const std::string& unit)
: Chart(shortdesc, longdesc, unit)
{}



PolarChart::PolarChart
(
  const std::string& rlabel,
  const PlotCurveList& plc,
  const std::string& shortDesc, const std::string& longDesc,
  double phi_unit,
  const std::string& addinit
)
: Chart("", rlabel, plc, shortDesc, longDesc, addinit),
  phi_unit_(phi_unit)
{}


void PolarChart::gnuplotCommand(gnuplotio::Gnuplot& gp) const
{
 gp<<addinit_<<";";
 gp<<"unset border;"
     " set polar;"
     " set grid polar 60.*pi/180.;"
     " set trange [0:2.*pi];"
     " set key rmargin;"
     " set size square;"
     " unset xtics;"
     " unset ytics;"
     ;

 double rmax=0.;
 for ( const PlotCurve& pc: plc_ ) {
  rmax=std::max(rmax, pc.xy().col(1).max());
 }

 gp<<"set_label(x, text) = sprintf(\"set label '%s' at ("<<rmax<<"*1.05*cos(%f)), ("<<rmax<<"*1.05*sin(%f)) center\", text, x, x);"
  <<"eval set_label(0, \"$0^\\\\circ$\");"
 <<"eval set_label(60.*pi/180., \"$60^\\\\circ$\");"
 <<"eval set_label(120.*pi/180., \"$120^\\\\circ$\");"
 <<"eval set_label(180.*pi/180., \"$180^\\\\circ$\");"
 <<"eval set_label(240.*pi/180., \"$240^\\\\circ$\");"
 <<"eval set_label(300.*pi/180., \"$300^\\\\circ$\");";

 //gp<<"set xlabel '"<<xlabel_<<"'; set ylabel '"<<ylabel_<<"'; ";

 if ( plc_.size() >0 )
 {
  gp<<"plot ";
  bool is_first=true;

  for ( const PlotCurve& pc: plc_ )
  {
   if ( !pc.plotcmd_.empty() )
   {

    if (!is_first)
    {
     gp << ",";
    }
    else is_first=false;

    if ( pc.xy_.n_rows>0 )
    {
     gp<<"'-' "<<pc.plotcmd_;
    }
    else
    {
     gp<<pc.plotcmd_;
    }

   }
  }

  gp<<endl;

  for ( const PlotCurve& pc: plc_ )
  {
   if ( pc.xy_.n_rows>0 )
   {
    arma::mat xy = pc.xy_;
    xy.col(0) *= phi_unit_;
    gp.send1d ( xy );
   }
  }

 }
}




ResultElementPtr PolarChart::clone() const
{
    ResultElementPtr res ( new PolarChart ( ylabel_, plc_, shortDescription().simpleLatex(), longDescription().simpleLatex(), phi_unit_, addinit_ ) );
    res->setOrder ( order() );
    return res;
}




PlotField::PlotField()
{
}


PlotField::PlotField ( const arma::mat& xy, const std::string& plotcmd )
    : xy_ ( xy ), plotcmd_ ( plotcmd )
{}




void addContourPlot
(
    std::shared_ptr<ResultElementCollection> results,
    const boost::filesystem::path& workdir,
    const std::string& resultelementname,
    const std::string& xlabel,
    const std::string& ylabel,
    const PlotFieldList& plc,
    const std::string& shortDescription,
    const std::string& addinit
)
{
    std::string chart_file_name= ( workdir/ ( resultelementname+".png" ) ).string();
    //std::string chart_file_name_i=(workdir/(resultelementname+".ps")).string();

    {
        Gnuplot gp;

        //gp<<"set terminal postscript color;";
        //gp<<"set output '"<<chart_file_name_i<<"';";
        gp<<"set terminal pngcairo; set termoption dash;";
        gp<<"set output '"<<chart_file_name<<"';";

        gp<<addinit<<";";
        gp<<"set xlabel '"<<xlabel<<"'; set ylabel '"<<ylabel<<"'; set grid; ";
        gp<<"splot ";
        for ( const PlotCurve& pc: plc ) {
            gp<<"'-' "<<pc.plotcmd_;
        }
        gp<<endl;
        for ( const PlotCurve& pc: plc ) {
            gp.send ( pc.xy_ );
        }
    }
    /*
     std::string cmd="ps2pdf "+chart_file_name_i+" "+chart_file_name;
     int ret=::system(cmd.c_str());
     if (ret || !exists(chart_file_name))
      throw insight::Exception("Conversion from postscript chart to pdf failed! Command was:\n"+cmd);
     else
      remove(chart_file_name_i);
      */
    results->insert ( resultelementname,
                      std::auto_ptr<Image> ( new Image
                              (
                                  workdir, chart_file_name,
                                  shortDescription, ""
                              ) ) );
}


}



