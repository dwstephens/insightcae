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

#ifndef ANALYSISFORM_H
#define ANALYSISFORM_H

#ifndef Q_MOC_RUN
#include "base/analysis.h"
#include "base/resultset.h"
#include "parametereditorwidget.h"
#endif

#include <QMdiSubWindow>
#include <QThread>
#include <QMetaType>
#include <QTreeWidget>
#include <QPushButton>
#include <QPlainTextEdit>

#include "boost/shared_ptr.hpp"

#include "graphprogressdisplayer.h"

#include "qdebugstream.h"




namespace Ui
{
class AnalysisForm;
}




Q_DECLARE_METATYPE(insight::ParameterSet);




class AnalysisWorker
: public QObject
{
  Q_OBJECT
  QThread workerThread_;
  
  boost::shared_ptr<insight::Analysis> analysis_;
  
public:
  AnalysisWorker(const boost::shared_ptr<insight::Analysis>& analysis);
  
public slots:
  void doWork(insight::ProgressDisplayer* pd=NULL);
  
signals:
  void resultReady(insight::ResultSetPtr);
  void finished();
};




class AnalysisForm
: public QMdiSubWindow
{
  Q_OBJECT
  
protected:
  std::string analysisName_;
  insight::ParameterSet parameters_;
  
  boost::shared_ptr<insight::Analysis> analysis_;  
  insight::ResultSetPtr results_;
  
  GraphProgressDisplayer *progdisp_;
  QThread workerThread_;
  
  QTreeWidget *rt_;
  QTreeWidgetItem* rtroot_;

  insight::DirectoryParameter executionPathParameter_;
  ParameterEditorWidget* peditor_;
  
  Q_DebugStream *cout_log_, *cerr_log_;
  QPlainTextEdit *log_;
  
  QPushButton *save_log_btn_, *send_log_btn_, *clear_log_btn_, *auto_scroll_down_btn_;
  
public:
  AnalysisForm(QWidget* parent, const std::string& analysisName);
  ~AnalysisForm();
  
  inline insight::ParameterSet& parameters() { return parameters_; }
  inline insight::Analysis& analysis() { return *analysis_; }
  inline insight::DirectoryParameter& executionPathParameter() { return executionPathParameter_; }
  
  inline void forceUpdate() { emit update(); }
    
private slots:
  void onSaveParameters();
  void onLoadParameters();
  void onRunAnalysis();
  void onKillAnalysis();
  void onResultReady(insight::ResultSetPtr);
  void onCreateReport();
  
  void saveLog();
  void sendLog();
  void clearLog();
  void autoScrollLog();

signals:
  void apply();
  void update();
  void runAnalysis(insight::ProgressDisplayer*);
  
private:
  Ui::AnalysisForm* ui;

};

#endif // ANALYSISFORM_H
