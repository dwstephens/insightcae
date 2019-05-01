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

#include <QApplication>
#include <QMainWindow>
#include <QSplashScreen>

#include "base/boost_include.h"
#include "base/exception.h"
#include "iscadapplication.h"
#include "iscadmainwindow.h"

#ifndef Q_MOC_RUN
#include <boost/program_options/options_description.hpp>
#include <boost/program_options/parsers.hpp>
#include <boost/program_options/variables_map.hpp>
#endif

#include "parser.h"

#include <locale>
#include <QLocale>

#include <qthread.h>

Q_DECLARE_METATYPE(insight::cad::ScalarPtr)
Q_DECLARE_METATYPE(insight::cad::VectorPtr)
Q_DECLARE_METATYPE(insight::cad::FeaturePtr)
Q_DECLARE_METATYPE(insight::cad::DatumPtr)
Q_DECLARE_METATYPE(insight::cad::PostprocActionPtr)

int main ( int argc, char** argv )
{
  qRegisterMetaType<insight::cad::ScalarPtr>("insight::cad::ScalarPtr");
  qRegisterMetaType<insight::cad::VectorPtr>("insight::cad::VectorPtr");
  qRegisterMetaType<insight::cad::FeaturePtr>("insight::cad::FeaturePtr");
  qRegisterMetaType<insight::cad::DatumPtr>("insight::cad::DatumPtr");
  qRegisterMetaType<insight::cad::PostprocActionPtr>("insight::cad::PostprocActionPtr");

  namespace po = boost::program_options;

  typedef std::vector<std::string> StringList;

  // Declare the supported options.
  po::options_description desc ( "Allowed options" );
  desc.add_options()
  ( "help,h", "produce help message" )
  ( "batch,b", "evaluate model from specified input file without starting GUI" )
  ( "nolog,l", "put debug output to console instead of log window" )
  ( "nobgparse,g", "deactivate background parsing" )
  //       ("workdir,w", po::value<std::string>(), "execution directory")
  //       ("savecfg,c", po::value<std::string>(), "save final configuration (including command line overrides) to this file")
  //       ("bool,b", po::value<StringList>(), "boolean variable assignment")
  //       ("selection,l", po::value<StringList>(), "selection variable assignment")
  //       ("string,s", po::value<StringList>(), "string variable assignment")
  //       ("path,p", po::value<StringList>(), "path variable assignment")
  //       ("double,d", po::value<StringList>(), "double variable assignment")
  //       ("int,i", po::value<StringList>(), "int variable assignment")
  //       ("merge,m", po::value<StringList>(), "additional input file to merge into analysis parameters before variable assignments")
  ( "input-file,f", po::value< std::string >(),"Specifies input file." )
  ;

  po::positional_options_description p;
  p.add ( "input-file", -1 );

  po::variables_map vm;
  po::store
  (
    po::command_line_parser ( argc, argv ).options ( desc ).positional ( p ).run(),
    vm
  );
  po::notify ( vm );

  if ( vm.count ( "help" ) )
    {
      std::cout << desc << std::endl;
      exit ( -1 );
    }

  try {
  if ( vm.count ( "input-file" ) && vm.count ( "batch" ) )
    {
      boost::filesystem::path filename( vm["input-file"].as<std::string>() );

      insight::cad::ModelPtr model ( new insight::cad::Model );
      bool success;
      if (filename=="-")
      {
        success=insight::cad::parseISCADModelStream ( std::cin, model.get() );
      } 
      else if ( boost::filesystem::extension(filename) == ".iscad" )
      {
        success=insight::cad::parseISCADModelFile ( filename, model.get() );
      }
      else
      {
        std::string script = "model: import(\""+filename.string()+"\");\n";
        std::istringstream ms(script);
        success=insight::cad::parseISCADModelStream ( ms, model.get() );
      }
      
      if ( success )
        {
          auto postprocActions=model->postprocActions();
          for ( decltype ( postprocActions ) ::value_type const& v: postprocActions )
          {
            cout << "Executing " << v.first << endl;
            v.second->execute();
          }

          return 0;
        }
      else
      {
          std::cerr<<"Failed to parse ISCAD script!"<<std::endl;
          return -1;
      }
    }
  else
    {
      //     XInitThreads();

      ISCADApplication app ( argc, argv );
      std::locale::global ( std::locale::classic() );
      QLocale::setDefault ( QLocale::C );

      QPixmap pixmap ( ":/resources/insight_cad_splash.png" );
      QSplashScreen splash ( pixmap, Qt::WindowStaysOnTopHint|Qt::SplashScreen );
      splash.show();
      splash.showMessage ( "Wait..." );
      QElapsedTimer splashtime; splashtime.start();

      ISCADMainWindow window ( 0, 0, vm.count ( "nolog" ) );
      
      bool dobgparsing = (vm.count ( "nobgparse" ) == 0);
      
      if ( vm.count ( "input-file" ) )
        {
          boost::filesystem::path filename ( vm["input-file"].as<std::string>() );
          if ( boost::filesystem::extension(filename) == ".iscad" )
          {
            window.insertModel ( filename, dobgparsing );
          }
          else
          {
            std::string script = "model: import(\""+filename.string()+"\");\n";
            window.insertModelScript ( script, dobgparsing );
          }
        }
      else
      {
          window.insertEmptyModel( dobgparsing );
      }

      window.show();

//      while (splashtime.elapsed() < 3e3)
//        {
//            app.processEvents(); //This is used to accept a click on the screen so that user can cancel the screen
//        }
      splash.finish(&window);

      window.raise();

      return app.exec();
    }
  }
  catch (insight::Exception e)
  {
      std::cerr<<e<<std::endl;
      return -1;
  }
}
