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

#ifndef INSIGHT_LINEARALGEBRA_H
#define INSIGHT_LINEARALGEBRA_H

#include <armadillo>
#include <map>

#include <gsl/gsl_errno.h>
#include <gsl/gsl_spline.h>

#include "boost/shared_ptr.hpp"
#include "boost/ptr_container/ptr_vector.hpp"

// #define SIGN(x) ((x)<0.0?-1.0:1.0)

namespace insight 
{

class GSLExceptionHandling
{
  gsl_error_handler_t *oldHandler_;
public:
  GSLExceptionHandling();
  ~GSLExceptionHandling();
};

arma::mat vec3(double x, double y, double z);
arma::mat tensor3(
  double xx, double xy, double xz,
  double yx, double yy, double yz,
  double zx, double zy, double zz
);
arma::mat vec2(double x, double y);

template<class T>
arma::mat Tensor(const T& t)
{
  arma::mat rt;
  rt << t.XX() << t.XY() << t.XZ() << arma::endr
     << t.YX() << t.YY() << t.YZ() << arma::endr
     << t.ZX() << t.ZY() << t.ZZ() << arma::endr;
  return rt;
}

template<class T>
arma::mat tensor(const T& t)
{
  arma::mat rt;
  rt << t.xx() << t.xy() << t.xz() << arma::endr
     << t.yx() << t.yy() << t.yz() << arma::endr
     << t.zx() << t.zx() << t.zz() << arma::endr;
  return rt;
}

template<class T>
arma::mat vector(const T& t)
{
  arma::mat rt;
  rt << t.x() << t.y() << t.z() << arma::endr;
  return rt;
}


template<class T>
arma::mat Vector(const T& t)
{
  arma::mat rt;
  rt << t.X() << t.Y() << t.Z() << arma::endr;
  return rt;
}

template<class T>
arma::mat vec3(const T& t)
{
  arma::mat rt;
  rt << t.X() <<arma::endr<< t.Y() <<arma::endr<< t.Z() << arma::endr;
  return rt;
}

template<class T>
T toVec(const arma::mat& v)
{
  return T(v(0), v(1), v(2));
}

arma::mat rotMatrix( double theta, arma::mat u=vec3(0,0,1) );
arma::mat rotated( const arma::mat&p, double theta, const arma::mat& axis=vec3(0,0,1), const arma::mat& p0 = vec3(0,0,0) );

std::string toStr(const arma::mat& v3);

/**
 * Fits c_j in
 *  c_j*x_ij approx y_i
 * add a column with all values "1" to x_ij to include constant offset!
 * 
 * y: column vector with values to fit
 * x: matrix with x-values
 */
arma::mat linearRegression(const arma::mat& y, const arma::mat& x);

arma::mat polynomialRegression(const arma::mat& y, const arma::mat& x, int maxorder, int minorder=0);

/**
 * evaluate polynomial
 * coeffs: coefficients, highest order coefficient first
 */
double evalPolynomial(double x, const arma::mat& coeffs);

class RegressionModel
{
public:
  virtual ~RegressionModel();
  
  virtual int numP() const =0;
  virtual void setParameters(const double* params) =0;
  virtual void getParameters(double* params) const;
  virtual arma::mat evaluateObjective(const arma::mat& x) const =0;
  virtual void setInitialValues(double* x) const =0;
  virtual arma::mat weights(const arma::mat& x) const;
  double computeQuality(const arma::mat& y, const arma::mat& x) const;
};

/**
 * fits parameters of a nonlinear model F
 * return fit quality
 */
double nonlinearRegression(const arma::mat& y, const arma::mat& x, RegressionModel& model, double tol=1e-3);

class Objective1D
{
public:
  virtual ~Objective1D();
  
  virtual double operator()(double x) const =0;
};

class ObjectiveND
{
public:
  virtual ~ObjectiveND();
  
  virtual double operator()(const arma::mat& x) const =0;
  virtual int numP() const =0;
};

double nonlinearSolve1D(const Objective1D& model, double x_min, double x_max);
double nonlinearMinimize1D(const Objective1D& model, double x_min, double x_max);
arma::mat nonlinearMinimizeND(const ObjectiveND& model, const arma::mat& x0, double tol=1e-3, const arma::mat& steps = arma::mat());

arma::mat movingAverage(const arma::mat& timeProfs, double fraction=0.5, bool first_col_is_time=true, bool centerwindow=false);

arma::mat sortedByCol(const arma::mat&m, int c);

/**
 * interpolates in a 2D-matrix using GSL spline routines.
 * The first column is assumed to contain the x-values.
 * All remaining columns are dependents and to be interpolated.
 */
class Interpolator
{
public:
  typedef enum {
    IP_INBOUND,
    IP_OUTBOUND_LARGE,
    IP_OUTBOUND_SMALL
  } OutOfBounds;

private:
  arma::mat xy_, first_, last_;
  boost::shared_ptr<gsl_interp_accel> acc;
  boost::ptr_vector<gsl_spline> spline ;
  
//   Interpolator(const Interpolator&);
  void initialize(const arma::mat& xy, bool force_linear=false);
  
public:
  Interpolator(const arma::mat& xy, bool force_linear=false);
  Interpolator(const arma::mat& x, const arma::mat& y, bool force_linear=false);
  ~Interpolator();
  
  /**
   * integrate column col fromx=a to x=b
   */
  double integrate(double a, double b, int col=0) const;
  /**
   * returns a single y-value from column col
   */
  double y(double x, int col=0, OutOfBounds* outOfBounds=NULL) const;
  /**
   * returns a single dy/dx-value from column col
   */
  double dydx(double x, int col=0, OutOfBounds* outOfBounds=NULL) const;
  /**
   * interpolates all y values (row vector) at x
   */
  arma::mat operator()(double x, OutOfBounds* outOfBounds=NULL) const;
  /**
   * interpolates all y values (row vector) at x
   */
  arma::mat dydxs(double x, OutOfBounds* outOfBounds=NULL) const;
  /**
   * interpolates all y values (row vector) 
   * at multiple locations given in x
   * returns only the y values, no x-values in the first column
   */
  arma::mat operator()(const arma::mat& x, OutOfBounds* outOfBounds=NULL) const;
  /**
   * computes all derivative values (row vector) 
   * at multiple locations given in x
   */
  arma::mat dydxs(const arma::mat& x, OutOfBounds* outOfBounds=NULL) const;

  /**
   * interpolates all y values (row vector) 
   * at multiple locations given in x
   * and return matrix with x as first column
   */
  arma::mat xy(const arma::mat& x, OutOfBounds* outOfBounds=NULL) const;
  
  inline const arma::mat& rawdata() const { return xy_; }
  inline const arma::mat& first() const { return first_; }
  inline const arma::mat& last() const { return last_; }
  inline double firstX() const { return first_(0); }
  inline double lastX() const { return last_(0); }

  inline int ncol() const { return spline.size(); }
};

arma::mat integrate(const arma::mat& xy);

double integrate(const Interpolator& ipol, double a, double b, int comp);
arma::mat integrate(const Interpolator& ipol, double a, double b);

struct compareArmaMat 
{
  bool operator()(const arma::mat& v1, const arma::mat& v2) const;
};


//typedef std::map<arma::mat, int, CompMat> SortedMatMap;

}

#endif // INSIGHT_LINEARALGEBRA_H
