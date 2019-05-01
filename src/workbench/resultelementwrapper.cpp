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


#include "resultelementwrapper.h"
#include "base/latextools.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QIntValidator>
#include <QDoubleValidator>
#include <QGroupBox>
#include <QFileDialog>
#include <QInputDialog>

#ifndef Q_MOC_RUN
#include "boost/foreach.hpp"
#endif

using namespace boost;

defineType(ResultElementWrapper);
defineFactoryTable(ResultElementWrapper, LIST(QTreeWidgetItem* tree, const QString& name, insight::ResultElement& res), LIST(tree, name, res));

ResultElementWrapper::ResultElementWrapper(QTreeWidgetItem* tree, const QString& name, insight::ResultElement& res)
: QTreeWidgetItem(tree), // QWidget(get<0>(p)),
  name_(name),
  p_(res)
{}

ResultElementWrapper::~ResultElementWrapper()
{}


defineType(CommentWrapper);
addToFactoryTable(ResultElementWrapper, CommentWrapper);

CommentWrapper::CommentWrapper(QTreeWidgetItem* tree, const QString& name, insight::ResultElement& re)
: ResultElementWrapper(tree, name, re)
{
//   QHBoxLayout *layout=new QHBoxLayout(this);
//   QLabel *nameLabel = new QLabel(name_, this);
//   QFont f=nameLabel->font(); f.setBold(true); nameLabel->setFont(f);
//   layout->addWidget(nameLabel);
//   le_=new QLabel(this);
//   le_->setText( res().value().c_str() );
//   le_->setToolTip(QString(res().shortDescription().c_str()));
//   layout->addWidget(le_);
//   this->setLayout(layout);
  
  setText(0, name_);
  setText(1, res().shortDescription().toHTML().c_str() );
  setText(2, insight::SimpleLatex(res().value()).toHTML().c_str() );
}


defineType(ScalarResultWrapper);
addToFactoryTable(ResultElementWrapper, ScalarResultWrapper);

ScalarResultWrapper::ScalarResultWrapper(QTreeWidgetItem* tree, const QString& name, insight::ResultElement& re)
: ResultElementWrapper(tree, name, re)
{
//   QHBoxLayout *layout=new QHBoxLayout(this);
//   QLabel *nameLabel = new QLabel(name_, this);
//   QFont f=nameLabel->font(); f.setBold(true); nameLabel->setFont(f);
//   layout->addWidget(nameLabel);
//   le_=new QLabel(this);
//   le_->setText(QString::number(res().value()) + " " + res().unit().c_str() );
//   le_->setToolTip(QString(res().shortDescription().c_str()));
//   layout->addWidget(le_);
//   this->setLayout(layout);
    setText(0, name_);
    setText(1, res().shortDescription().toHTML().c_str());
    setText(2, QString::number(res().value()) + " " + res().unit().toHTML().c_str() );
}


defineType(ResultSectionWrapper);
addToFactoryTable(ResultElementWrapper, ResultSectionWrapper);

ResultSectionWrapper::ResultSectionWrapper(QTreeWidgetItem* tree, const QString& name, insight::ResultElement& re)
: ResultElementWrapper(tree, name, re)
{
//   QHBoxLayout *layout=new QHBoxLayout(this);
//   frame_ = new QGroupBox(name_, this);
//   layout->addWidget(frame_);
    setText(0, name_);
    addWrapperToWidget(res(), this/*, this*/);

//   this->setLayout(layout);
}

defineType(ResultSetWrapper);
addToFactoryTable(ResultElementWrapper, ResultSetWrapper);

ResultSetWrapper::ResultSetWrapper(QTreeWidgetItem* tree, const QString& name, insight::ResultElement& re)
: ResultElementWrapper(tree, name, re)
{
//   QHBoxLayout *layout=new QHBoxLayout(this);
//   frame_ = new QGroupBox(name_, this);
//   layout->addWidget(frame_);


  setText(0, name_);
  addWrapperToWidget(res(), this/*, this*/);

//   this->setLayout(layout);
}

defineType(ImageWrapper);
addToFactoryTable(ResultElementWrapper, ImageWrapper);

ImageWrapper::ImageWrapper(QTreeWidgetItem* tree, const QString& name, insight::ResultElement& re)
: ResultElementWrapper(tree, name, re)
{
//   QHBoxLayout *layout=new QHBoxLayout(this);
//   QLabel *nameLabel = new QLabel(name_, this);
//   QFont f=nameLabel->font(); f.setBold(true); nameLabel->setFont(f);
//   layout->addWidget(nameLabel);
//   
    QPixmap image(res().imagePath().c_str());
    
    // scale 300dpi (print) => 70dpi (screen)
    double w0=image.size().width();
    image=image.scaledToWidth(w0/4, Qt::SmoothTransformation);
//   
//   le_=new QLabel(this);
//   le_->setPixmap(image);
//   le_->setScaledContents(true);
//   
//   le_->setToolTip(QString(res().shortDescription().c_str()));
//   layout->addWidget(le_);
//   this->setLayout(layout);
    
    setText(0, name_);
    setText(1, res().shortDescription().toHTML().c_str());
    setData(2, 1, QVariant(image));
}


defineType(ChartWrapper);
addToFactoryTable(ResultElementWrapper, ChartWrapper);

ChartWrapper::ChartWrapper(QTreeWidgetItem* tree, const QString& name, insight::ResultElement& re)
: ResultElementWrapper(tree, name, re)
{
  
    chart_file_=boost::filesystem::unique_path(boost::filesystem::temp_directory_path()/"%%%%-%%%%-%%%%-%%%%.png");
    res().generatePlotImage(chart_file_);
    QPixmap image(chart_file_.c_str());
    
    // scale 300dpi (print) => 70dpi (screen)
    double w0=image.size().width();
    image=image.scaledToWidth(w0/4, Qt::SmoothTransformation);

    setText(0, name_);
    setText(1, res().shortDescription().toHTML().c_str());
    setData(2, 1, QVariant(image));

}


ChartWrapper::~ChartWrapper()
{
  boost::filesystem::remove(chart_file_);
}


defineType(TabularResultWrapper);
addToFactoryTable(ResultElementWrapper, TabularResultWrapper);

TabularResultWrapper::TabularResultWrapper(QTreeWidgetItem* tree, const QString& name, insight::ResultElement& re)
: ResultElementWrapper(tree, name, re)
{
    setText(0, name_);
    setText(1, res().shortDescription().toHTML().c_str());

//   QHBoxLayout *layout=new QHBoxLayout(this);
//   QLabel *nameLabel = new QLabel(name_, this);
//   QFont f=nameLabel->font(); f.setBold(true); nameLabel->setFont(f);
//   layout->addWidget(nameLabel);
//   
  le_=new QTableWidget(res().rows().size(), res().headings().size()/*, this*/);
  
  QStringList headers;
  for (const std::string& h: res().headings() )
  {
    headers << QString(h.c_str());
  }
  le_->setHorizontalHeaderLabels( headers );
     le_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    le_->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
 
  for (size_t i=0; i<res().rows().size(); i++)
  {
    const std::vector<double>& row=res().rows()[i];
    for (size_t j=0; j<row.size(); j++)
    {
      le_->setItem(i, j, new QTableWidgetItem( QString::number(row[j]) ));
    }
  }
    le_->doItemsLayout();
    le_->resizeColumnsToContents();
  
  treeWidget()->setItemWidget(this, 2, le_);
//   
//   le_->setToolTip(QString(res().shortDescription().c_str()));
//   layout->addWidget(le_);
//   
//   this->setLayout(layout);
}


defineType(AttributeTableResultWrapper);
addToFactoryTable(ResultElementWrapper, AttributeTableResultWrapper);

AttributeTableResultWrapper::AttributeTableResultWrapper(QTreeWidgetItem* tree, const QString& name, insight::ResultElement& re)
: ResultElementWrapper(tree, name, re)
{
    setText(0, name_);
    setText(1, res().shortDescription().toHTML().c_str());

//   QHBoxLayout *layout=new QHBoxLayout(this);
//   QLabel *nameLabel = new QLabel(name_, this);
//   QFont f=nameLabel->font(); f.setBold(true); nameLabel->setFont(f);
//   layout->addWidget(nameLabel);
//   
    le_=new QTableWidget(res().names().size(), 2/*, this*/);
    le_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    le_->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    
    QStringList headers;
    headers << "Attribute" << "Value";
    le_->setHorizontalHeaderLabels( headers );
    
    for(int i=0; i<res().names().size(); i++)
    {
        QString attr_name(res().names()[i].c_str());
        QString attr_val(boost::lexical_cast<std::string>(res().values()[i]).c_str());
        le_->setItem(i, 0, new QTableWidgetItem( attr_name ));
        le_->setItem(i, 1, new QTableWidgetItem( attr_val ));
    }
    le_->doItemsLayout();
    le_->resizeColumnsToContents();
    
    treeWidget()->setItemWidget(this, 2, le_);
    
//   le_->setToolTip(QString(res().shortDescription().c_str()));
//   layout->addWidget(le_);
//   
//   this->setLayout(layout);
}


void addWrapperToWidget ( insight::ResultElementCollection& rset, QTreeWidgetItem *node, QWidget *superform )
{
//   QVBoxLayout *vlayout=new QVBoxLayout(widget);

//   for(insight::ResultSet::iterator i=rset.begin(); i!=rset.end(); i++)
    std::vector<std::pair<insight::ResultElementCollection::key_type,insight::ResultElementCollection::mapped_type> > items;

//   std::transform
//   (
//     begin(),
//     end(),
//     std::back_inserter(items),
//     boost::bind(&value_type, _1) // does not work...
//   );

    std::for_each
    (
        rset.begin(),
        rset.end(),
        [&items] ( const insight::ResultElementCollection::value_type& p ) {
          items.push_back ( p );
    }
    );

    std::sort
    (
        items.begin(),
        items.end(),
    [] ( const insight::ResultElementCollection::value_type &left, const insight::ResultElementCollection::value_type &right ) {
        return left.second->order() < right.second->order();
    }
    );

    for ( const insight::ResultElementCollection::value_type& i: items ) {
        try {
            ResultElementWrapper *wrapper =
                ResultElementWrapper::lookup
                (
                    i.second->type(),
                    node, i.first.c_str(), *i.second
                );
//       vlayout->addWidget(wrapper);
        } catch ( insight::Exception e ) {
//       QLabel *comment=new QLabel( (i.first+": "+e.message()).c_str());
//       vlayout->addWidget(comment);
            QString comment ( ( i.first+": "+e.message() ).c_str() );
            QTreeWidgetItem *it = new QTreeWidgetItem ( node, QStringList() << "(error)" << " "<< comment );
        }
        /*
        if (superform)
        {
          QObject::connect(superform, SIGNAL(apply()), wrapper, SLOT(onApply()));
          QObject::connect(superform, SIGNAL(update()), wrapper, SLOT(onUpdate()));
        }
        */
    }

}
