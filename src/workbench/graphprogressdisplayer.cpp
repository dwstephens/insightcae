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


#include "graphprogressdisplayer.h"

#ifndef Q_MOC_RUN
#include "boost/foreach.hpp"
#endif

#include <QCoreApplication>
#include <QTimer>

#include "qwt_scale_engine.h"
#include "qwt_plot_grid.h"
#include "qwt_legend.h"

using namespace insight;

void GraphProgressDisplayer::reset()
{
  typedef std::map<std::string, QwtPlotCurve*> CurveList;
  for ( CurveList::value_type& i: curve_)
  {
    delete i.second;
  }
  curve_.clear();
  progressX_.clear();
  progressY_.clear();
  needsRedraw_=true;
  this->replot();
}


void GraphProgressDisplayer::update(const insight::ProgressState& pi)
{
  mutex_.lock();
  
  double iter=pi.first;
  const ProgressVariableList& pvl=pi.second;
  
  for ( const ProgressVariableList::value_type& i: pvl)
  {
    const std::string& name = i.first;
    
    std::vector<double>& x = progressX_[name];
    std::vector<double>& y = progressY_[name];

    if (i.second > 0.0) // only add, if y>0. Plot gets unreadable otherwise
    {
        x.push_back(iter);
        y.push_back(i.second);
    }
    if (x.size() > maxCnt_) 
    {
        x.erase(x.begin());
        y.erase(y.begin());    
    }
  }
  
  setAxisAutoScale(QwtPlot::yLeft);
  needsRedraw_=true;
  
  mutex_.unlock();
}

GraphProgressDisplayer::GraphProgressDisplayer(QWidget* parent)
: QwtPlot(parent),
  maxCnt_(500),
  needsRedraw_(true)
{
  setTitle("Progress Plot");
  insertLegend( new QwtLegend() );
  setCanvasBackground( Qt::white );
  
#if (QWT_VERSION < 0x060100)
  setAxisScaleEngine(QwtPlot::yLeft, new QwtLog10ScaleEngine() );
#else
  setAxisScaleEngine(QwtPlot::yLeft, new QwtLogScaleEngine() );
#endif
  
  QwtPlotGrid *grid = new QwtPlotGrid();
  grid->attach(this);
  
  QTimer *timer=new QTimer;
  connect(timer, &QTimer::timeout, this, &GraphProgressDisplayer::checkForUpdate);
  timer->setInterval(500);
  timer->start();
}

GraphProgressDisplayer::~GraphProgressDisplayer()
{
}

void GraphProgressDisplayer::checkForUpdate()
{
    mutex_.lock();

    if (needsRedraw_)
    {
        needsRedraw_=false;
        for ( const ArrayList::value_type& i: progressX_ )
        {
            const std::string& name=i.first;

            if (curve_.find(name) == curve_.end())
            {
                QwtPlotCurve *crv=new QwtPlotCurve();
                crv->setTitle(name.c_str());
                crv->setPen(QPen(QColor(
                                     double(qrand())*255.0/double(RAND_MAX),
                                     double(qrand())*255.0/double(RAND_MAX),
                                     double(qrand())*255.0/double(RAND_MAX)
                                 ), 2.0));
                crv->attach(this);
                curve_[name]=crv;
            }

            curve_[name]->setSamples(&progressX_[name][0], &progressY_[name][0], progressY_[name].size()); // leave out first sample, since it is sometimes =0 and this make logscaled plot unreadable
        }

        this->replot();
    }

    mutex_.unlock();
}


