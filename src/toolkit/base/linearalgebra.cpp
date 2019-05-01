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

#include <stdio.h>
#include <math.h>

#include "linearalgebra.h"
#include "boost/lexical_cast.hpp"
#include "boost/tuple/tuple.hpp"
#include "boost/format.hpp"
#include "base/exception.h"

// #include "minpack.h"
// #include <dlib/optimization.h>

using namespace arma;
using namespace boost;

namespace insight
{
  
void insight_gsl_error_handler
(
 const char* reason,
 const char*,
 int,
 int
)
{
  throw insight::Exception("Error in GSL subroutine: "+std::string(reason));
}

GSLExceptionHandling::GSLExceptionHandling()
{
  oldHandler_ = gsl_set_error_handler(&insight_gsl_error_handler);
}

GSLExceptionHandling::~GSLExceptionHandling()
{
  gsl_set_error_handler(oldHandler_);
}


mat vec1(double x)
{
  mat v;
  v << x << endr;
  return v;
}

mat vec2(double x, double y)
{
  mat v;
  v << x <<endr << y << endr;
  return v;
}

mat vec3(double x, double y, double z)
{
  mat v;
  v << x <<endr << y << endr << z <<endr;
  return v;
}

arma::mat tensor3(
  double xx, double xy, double xz,
  double yx, double yy, double yz,
  double zx, double zy, double zz
)
{
  mat v;
  v 
    << xx << xy <<  xz <<endr
    << yx << yy <<  yz <<endr
    << zx << zy <<  zz <<endr;
    
  return v;
}



mat rotMatrix( double theta, mat u )
{
    double s=sin(theta);
    double c=cos(theta);
    double ux=u[0];
    double uy=u[1];
    double uz=u[2];
    mat m;
    m << ux*ux+(1-ux*ux)*c << ux*uy*(1-c)-uz*s << ux*uz*(1-c)+uy*s << endr
      << ux*uy*(1-c)+uz*s << uy*uy+(1-uy*uy)*c << uy*uz*(1-c)-ux*s << endr
      << ux*uz*(1-c)-uy*s << uy*uz*(1-c)+ux*s << uz*uz+(1-uz*uz)*c << endr;
    return m;
}

arma::mat rotated( const arma::mat&p, double theta, const arma::mat& axis, const arma::mat& p0 )
{
    return p0 + rotMatrix(theta, axis)*(p-p0);
}

std::string toStr(const arma::mat& v3)
{
  std::string s="";
  for (arma::uword i=0; i<3; i++)
    s+=" "+lexical_cast<std::string>(v3(i));
  return s+" ";
}

arma::mat linearRegression(const arma::mat& y, const arma::mat& x)
{
  return solve(x.t()*x, x.t()*y);
}

arma::mat polynomialRegression(const arma::mat& y, const arma::mat& x, int maxorder, int minorder)
{
  arma::mat xx(x.n_rows, arma::uword(maxorder-minorder) );
  for (arma::uword i=0; i<arma::uword(maxorder-minorder); i++)
    xx.col(i)=pow(x, arma::uword(minorder)+i);
  return linearRegression(y, xx);
}

double evalPolynomial(double x, const arma::mat& coeffs)
{
  double y=0;
  for (arma::uword k=0; k<coeffs.n_elem; k++)
  {
    arma::uword p=coeffs.n_elem-k-1;
    y+=coeffs(k)*pow(x,p);
  }
  return y;
}

typedef boost::tuple<RegressionModel&, const arma::mat&, const arma::mat&> RegressionData;

double f_nonlinearRegression(const gsl_vector * p, void * params)
{
  RegressionData* md = static_cast<RegressionData*>(params);
  
  RegressionModel& m = boost::get<0>(*md);
  const arma::mat& y = boost::get<1>(*md);
  const arma::mat& x = boost::get<2>(*md);
  
  m.setParameters(p->data);
  
  return m.computeQuality(y, x);
}

RegressionModel::~RegressionModel()
{
}

void RegressionModel::getParameters(double*) const
{
  throw insight::Exception("not implemented!");
}

arma::mat RegressionModel::weights(const arma::mat& x) const
{
  return ones(x.n_rows);
}

double RegressionModel::computeQuality(const arma::mat& y, const arma::mat& x) const
{
  double q=0.0;
  arma::mat w=weights(x);
  for (arma::uword r=0; r<y.n_rows; r++)
  {
    q +=  w(r) * pow(norm( y.row(r) - evaluateObjective(x.row(r)), 2 ), 2);
  }
  return q;
}


double nonlinearRegression(const arma::mat& y, const arma::mat& x,RegressionModel& model, double tol)
{
    try
    {
        const gsl_multimin_fminimizer_type *T =
            gsl_multimin_fminimizer_nmsimplex;
        gsl_multimin_fminimizer *s = NULL;
        gsl_vector *ss, *p;
        gsl_multimin_function minex_func;

        size_t iter = 0;
        int status;
        double size;

        /* Starting point */
        p = gsl_vector_alloc (model.numP());
        //gsl_vector_set_all (p, 1.0);
        model.setInitialValues(p->data);

        /* Set initial step sizes to 0.1 */
        ss = gsl_vector_alloc (model.numP());
        gsl_vector_set_all (ss, 0.1);

        /* Initialize method and iterate */
        RegressionData param(model, y, x);
        minex_func.n = model.numP();
        minex_func.f = f_nonlinearRegression;
        minex_func.params = (void*) (&param);

        s = gsl_multimin_fminimizer_alloc (T, model.numP());
        gsl_multimin_fminimizer_set (s, &minex_func, p, ss);

        do
        {
            iter++;
            status = gsl_multimin_fminimizer_iterate(s);

            if (status)
                break;

            size = gsl_multimin_fminimizer_size (s);
            status = gsl_multimin_test_size (size, tol);

        }
        while (status == GSL_CONTINUE && iter < 1000);

        model.setParameters(s->x->data);

        gsl_vector_free(p);
        gsl_vector_free(ss);
        gsl_multimin_fminimizer_free (s);

        return model.computeQuality(y, x);
    }
    catch (...)
    {
        std::ostringstream os;
        os<<"x=["<<x.t()<<"]\ty=["<<y.t()<<"]";
        throw insight::Exception("nonlinearRegression(): Failed to do regression.\nSupplied data: "+os.str());
    }

    return DBL_MAX;
}

#include <gsl/gsl_errno.h>
#include <gsl/gsl_roots.h>

Objective1D::~Objective1D()
{
}


double F_obj(double x, void *param)
{
  const Objective1D& model=*static_cast<Objective1D*>(param);
  return model(x);
}

double nonlinearSolve1D(const Objective1D& model, double x_min, double x_max)
{
  int i, times, status;
  gsl_function f;
  gsl_root_fsolver *workspace_f;
  double x, x_l, x_r;

 
  workspace_f = gsl_root_fsolver_alloc(gsl_root_fsolver_bisection);

  f.function = &F_obj;
  f.params = const_cast<void *>(static_cast<const void*>(&model));

  x_l = x_min;
  x_r = x_max;

  gsl_root_fsolver_set(workspace_f, &f, x_l, x_r);

  for(times = 0; times < 100; times++)
  {
      status = gsl_root_fsolver_iterate(workspace_f);

      x_l = gsl_root_fsolver_x_lower(workspace_f);
      x_r = gsl_root_fsolver_x_upper(workspace_f);

      status = gsl_root_test_interval(x_l, x_r, 1.0e-13, 1.0e-20);
      if(status != GSL_CONTINUE)
      {
	  break;
      }
  }

  gsl_root_fsolver_free(workspace_f);

  return x_l;
}

double F_min_obj(const gsl_vector* x, void *param)
{
  const Objective1D& model=*static_cast<Objective1D*>(param);
  cout<<"ITER: X="<<x->data[0]<<" F="<<model(x->data[0])<<endl;
  return model(x->data[0]);
}


double nonlinearMinimize1D(const Objective1D& model, double x_min, double x_max)
{
  try
  {
    const gsl_multimin_fminimizer_type *T = 
      gsl_multimin_fminimizer_nmsimplex;
    gsl_multimin_fminimizer *s = NULL;
    gsl_vector *ss, *p;
    gsl_multimin_function minex_func;

    size_t iter = 0;
    int status;
    double size;

    /* Starting point */
    p = gsl_vector_alloc (1);
    gsl_vector_set_all (p, 0.5*(x_min+x_max));

    /* Set initial step sizes to 0.1 */
    ss = gsl_vector_alloc (1);
    gsl_vector_set_all (ss, 0.1);

    /* Initialize method and iterate */
    minex_func.n = 1;
    minex_func.f = &F_min_obj;
    minex_func.params = const_cast<void *>(static_cast<const void*>(&model));

    s = gsl_multimin_fminimizer_alloc (T, 1);
    gsl_multimin_fminimizer_set (s, &minex_func, p, ss);

    do
      {
	iter++;
	status = gsl_multimin_fminimizer_iterate(s);
	
	if (status) 
	  break;

	size = gsl_multimin_fminimizer_size (s);
	status = gsl_multimin_test_size (size, 1e-3);

// 	if (status == GSL_SUCCESS)
// 	  {
// 	    printf ("converged to minimum at\n");
// 	  }

// 	printf ("%5d %10.3e %10.3e f() = %7.3f size = %.3f\n", 
// 		iter,
// 		gsl_vector_get (s->x, 0), 
// 		s->fval, size);
      }
    while (status == GSL_CONTINUE && iter < 100);
    
    double solution=s->x->data[0];
//     model.setParameters(s->x->data);
    
    gsl_vector_free(p);
    gsl_vector_free(ss);
    gsl_multimin_fminimizer_free (s);

    return solution; //model.computeQuality(y, x);
  }
  catch (...)
  {
    std::ostringstream os;
//     os<<"x=["<<x.t()<<"]\ty=["<<y.t()<<"]";
    throw insight::Exception("nonlinearMinimize1D(): Failed to do regression.\nSupplied data: "+os.str());
  }
  
  return DBL_MAX;
}


ObjectiveND::~ObjectiveND()
{}

typedef boost::tuple<const ObjectiveND&> nonlinearMinimizeNDData;

double f_nonlinearMinimizeND(const gsl_vector * p, void * params)
{
  nonlinearMinimizeNDData* md = static_cast<nonlinearMinimizeNDData*>(params);
  
  const ObjectiveND& m = boost::get<0>(*md);
  
  arma::mat x=arma::zeros(m.numP());
  for (int i=0; i<x.n_elem; i++)
  {
      x(i) = gsl_vector_get (p, i);
  }
  
  return m(x);
}

// const ObjectiveND* minpack_params;
// void f_nonlinearMinimizeND2(int *n, double *p, double *fvec, int *iflag)
// {
//   
//   const ObjectiveND& m = *minpack_params;
//   
//   if (*n != m.numP())
//     throw insight::Exception("incompatible!");
//   
//   arma::mat x=arma::zeros(m.numP());
//   for (int i=0; i<x.n_elem; i++)
//   {
//       x(i) = p[i];
//   }
//   
// 
//     std::cerr<<"F="<<m(x)<<std::endl;
//   if (*iflag == 0) {
//     /*      insert print statements here when nprint is positive. */
//     /* if the nprint parameter to lmder is positive, the function is
//        called every nprint iterations with iflag=0, so that the
//        function may perform special operations, such as printing
//        residuals. */
//     return;
//   }
// 
//   fvec[0]=m(x);
// }

// typedef dlib::matrix<double,0,1> column_vector;
// class f_nonlinearMinimizeND3
// {
// public:
//   const ObjectiveND* obj_;
//   f_nonlinearMinimizeND3(const ObjectiveND* obj): obj_(obj) {}
//   double operator()(const column_vector& arg) const
//   {
//     arma::mat p = arma::zeros(obj_->numP());
//     for (int i=0; i<obj_->numP(); i++) p(i)=arg(i);
//     double resi=(*obj_)(p);
//     std::cerr<<"resi="<<resi<<std::endl;
//     return resi;
//   }
// };
    
arma::mat nonlinearMinimizeND(const ObjectiveND& model, const arma::mat& x0, double tol, const arma::mat& steps)
{
//   int n=model.numP();
//   column_vector starting_point(n);
//   for (int i=0; i<n; i++) starting_point(i)=x0(i);
//   double r = dlib::find_min_bobyqa(
//     f_nonlinearMinimizeND3(&model), 
//     starting_point, 
//     10,    // number of interpolation points
//     dlib::uniform_matrix<double>(n,1, -1e100),  // lower bound constraint
//     dlib::uniform_matrix<double>(n,1, 1e100),   // upper bound constraint
//     10,    // initial trust region radius
//     1e-6,  // stopping trust region radius
//     100000    // max number of objective function evaluations
//   );
//   arma::mat res=arma::zeros(n);
//   for (int i=0; i<n; i++) res(i)=starting_point(i);
//   
//   return res;
  
//   minpack_params = &model;
// 
//     int j, n, info, lwa;
//   n = model.numP();
//   double tol2, fnorm;
//   double x[n], fvec[1], wa[180];
//   int one=1;
// 
// 
// /*      the following starting values provide a rough solution. */
// 
//   for (j=0; j<n; j++) {
//     x[j] = x0(j);
//   }
// 
//   lwa = 180;
// 
// /*      set tol to the square root of the machine precision. */
// /*      unless high solutions are required, */
// /*      this is the recommended setting. */
// 
//   tol2 = sqrt(dpmpar_(&one));
//   hybrd1_(&f_nonlinearMinimizeND2, &n, x, fvec, &tol2, &info, wa, &lwa);
//   fnorm = enorm_(&n, fvec);
// 
//   printf("     final L2 norm of the residuals %15.7g\n", (double)fnorm);
//   printf("     exit parameter                 %10i\n", info);
//   printf("     final approximates solution\n");
//   
//         arma::mat res=arma::zeros(n);
//         for (int i=0; i<n; i++)
//         {
//             res(i)=x[i];
//         };
// 	
// 	return res;
	

    try
    {
        const gsl_multimin_fminimizer_type *T =
            gsl_multimin_fminimizer_nmsimplex;
            
        gsl_multimin_fminimizer *s = NULL;
        gsl_vector *ss, *p/*, *olditer_p*/;
        gsl_multimin_function minex_func;

        size_t iter = 0;
        int status;
        double size;

        /* Starting point */
        p = gsl_vector_alloc (model.numP());
//         olditer_p = gsl_vector_alloc (model.numP());
        //gsl_vector_set_all (p, 1.0);
        for (int i=0; i<model.numP(); i++)
        {
            gsl_vector_set (p, i, x0(i));
        }

        /* Set initial step sizes to 0.1 */
        ss = gsl_vector_alloc (model.numP());
        gsl_vector_set_all (ss, 0.1);

        if (steps.n_elem!=0)
          {
            for (int i=0; i<model.numP(); i++)
              gsl_vector_set(ss, i, steps(i));
          }

        /* Initialize method and iterate */
        nonlinearMinimizeNDData param(model);
        minex_func.n = model.numP();
        minex_func.f = f_nonlinearMinimizeND;
        minex_func.params = (void*) (&param);

        s = gsl_multimin_fminimizer_alloc (T, model.numP());
        gsl_multimin_fminimizer_set (s, &minex_func, p, ss);

// 	double relax=0.01;
        do
        {
// 	    gsl_vector_memcpy(olditer_p, s->x);
	    
            iter++;
            status = gsl_multimin_fminimizer_iterate(s);
            if (status)
                break;

            size = gsl_multimin_fminimizer_size (s);
            status = gsl_multimin_test_size (size, tol);
// 	    std::cerr<<"i="<<iter<<": F="<<s->fval<<std::endl;
// 	    // relax
// 	    for (int i=0; i<model.numP(); i++)
// 	    {
// 	      gsl_vector_set(s->x, i, 
// 		      relax*gsl_vector_get(s->x, i) + (1.-relax)*gsl_vector_get(olditer_p, i) 
// 	      );
// 	    }
        }
        while (status == GSL_CONTINUE && iter < 10000);
        
        arma::mat res=arma::zeros(model.numP());
        for (int i=0; i<model.numP(); i++)
        {
            res(i)=gsl_vector_get (s->x, i);
        };
        
        gsl_vector_free(p);
        gsl_vector_free(ss);
//         gsl_vector_free(olditer_p);
        gsl_multimin_fminimizer_free (s);

        return res; //model.computeQuality(y, x);
    }
    catch (insight::Exception e)
    {
        std::ostringstream os;
        os<<"x0=["<<x0.t()<<"]";
        throw insight::Exception("nonlinearMinimizeND(): Exception occurred during regression.\nSupplied data: "+os.str()+"\n"+e.as_string());
    }
    catch (...)
    {
        std::ostringstream os;
        os<<"x0=["<<x0.t()<<"]";
        throw insight::Exception("nonlinearMinimizeND(): Failed to do regression.\nSupplied data: "+os.str());
    }

    return arma::zeros(x0.n_elem)+DBL_MAX;
}

arma::mat movingAverage(const arma::mat& timeProfs, double fraction, bool first_col_is_time, bool centerwindow)
{
  std::ostringstream msg;
  msg<<"Computing moving average for "
       "t="<<valueList_to_string(timeProfs.col(0))<<" and "
       "y="<<valueList_to_string(timeProfs.cols(1,timeProfs.n_cols-1))<<
       " with fraction="<<fraction<<", first_col_is_time="<<first_col_is_time<<" and centerwindow="<<centerwindow;
  CurrentExceptionContext ce(msg.str());

  if (!first_col_is_time)
    throw insight::Exception("Internal error: moving average without time column is currently unsupported!");

  if (timeProfs.n_cols<2)
    throw insight::Exception("movingAverage: only dataset with "
      +lexical_cast<std::string>(timeProfs.n_cols)+" columns given. There is no data to average.");

  if (timeProfs.n_rows>1)
  {
    arma::uword n_raw=timeProfs.n_rows;

    double x0=arma::min(timeProfs.col(0));
    double dx_raw=arma::max(timeProfs.col(0))-x0;

//    std::cout<<"mvg avg: range ["<<x0<<":"<<timeProfs.col(0).max()<<"]"<<std::endl;

    double window=fraction*dx_raw;
    double avgdx=dx_raw/double(n_raw);

    // number of averages to compute
    arma::uword n_avg=std::min(n_raw, std::max(arma::uword(2), arma::uword((dx_raw-window)/avgdx) ));
//    window=n_avg*avgdx;

    double window_ofs=window;
    if (centerwindow)
    {
        window_ofs=window/2.0;
    }

    arma::mat result=zeros(n_avg, timeProfs.n_cols);
    
    for (arma::uword i=0; i<n_avg; i++)
    {
        double x = x0 + window_ofs + double(i)*avgdx;

        double from = x - window_ofs, to = from + window;

//        std::cout<<"avg from "<<from<<" to "<<to<<std::endl;

        arma::uword j0=0;
        if (first_col_is_time)
        {
            j0=1;
            result(i,0)=x;
        }
        arma::uvec indices = arma::find( (timeProfs.col(0)>=from) && (timeProfs.col(0)<=to) );
        arma::mat selrows=timeProfs.rows( indices );
        if (selrows.n_rows==0) // nothing selected: take the closest row
        {
            indices = arma::sort_index(arma::mat(pow(timeProfs.col(0) - 0.5*(from+to), 2)));
            selrows=timeProfs.row( indices(0) );
        }
//        if (i==n_avg-1) std::cout<<"sel rows="<<selrows<<std::endl;

        if (selrows.n_rows==1)
        {
            for (arma::uword j=j0; j<timeProfs.n_cols; j++)
            {
               result(i, j) = arma::as_scalar(selrows.col(j));
            }
        }
        else
        {

            arma::mat xcol=selrows.col(0);
            for (arma::uword j=j0; j<timeProfs.n_cols; j++)
            {
              arma::mat ccol=selrows.col(j);
              double I=0;
              for (arma::uword k=1; k<xcol.n_rows; k++)
              {
                I+=0.5*(ccol(k)+ccol(k-1))*(xcol(k)-xcol(k-1));
              }
              result(i, j) = I/(xcol.max()-xcol.min());

//              if (i==n_avg-1)
//              {
//                  std::cout<<j<<" >> "<<result(i, j)<<" "<<ccol.min()<<" "<<ccol.max()<<std::endl;
//              }
            }
        }
    }
    
    return result;
    
  }
  else
  {
    return timeProfs;
  }
}

arma::mat sortedByCol(const arma::mat&m, int c)
{

  arma::uvec indices = arma::sort_index(m.col(c));
  arma::mat xy = arma::zeros(m.n_rows, m.n_cols);
  for (int r=0; r<m.n_rows; r++)
    xy.row(r)=m.row(indices(r));
  return xy;
}

arma::mat filterDuplicates(const arma::mat&m)
{

  arma::mat xy = arma::zeros(m.n_rows, m.n_cols);

  xy.row(0)=m.row(0);

  arma::uword j=1;
  for (arma::uword r=1; r<m.n_rows; r++)
  {
    if (arma::norm(m.row(r)-m.row(j-1),2) > 1e-8)
    {
      xy.row(j++)=m.row(r);
    }
  }
  xy.resize(j, m.n_cols);

  std::cout<<xy.row(0)<<std::endl<<xy.row(xy.n_rows-1)<<std::endl;

  return xy;
}


void Interpolator::initialize(const arma::mat& xy_us, bool force_linear)
{
    try
    {
        arma::mat xy = filterDuplicates(sortedByCol(xy_us, 0));
        xy_=xy;

        if (xy.n_cols<2)
            throw insight::Exception("Interpolate: interpolator requires at least 2 columns!");
        if (xy.n_rows<2)
            throw insight::Exception("Interpolate: interpolator requires at least 2 rows!");
        spline.clear();

        int nf=xy.n_cols-1;
        int nrows=xy.n_rows;

        acc.reset( gsl_interp_accel_alloc () );
        for (int i=0; i<nf; i++)
        {
//             cout<<"building interpolator for col "<<i<<endl;
            if ( (xy.n_rows==2) || force_linear )
                spline.push_back( gsl_spline_alloc (gsl_interp_linear, nrows) );
            else
                spline.push_back( gsl_spline_alloc (gsl_interp_cspline, nrows) );
            //cout<<"x="<<xy.col(0)<<endl<<"y="<<xy.col(i+1)<<endl;
            gsl_spline_init (&spline[i], xy.colptr(0), xy.colptr(i+1), nrows);
        }

        first_=xy.row(0);
        last_=xy.row(xy.n_rows-1);
    }
    catch (...)
    {
        std::ostringstream os;
        os<<xy_us;
        throw insight::Exception("Interpolator::Interpolator(): Failed to initialize interpolator.\nSupplied data: "+os.str());
    }
}

Interpolator::Interpolator(const arma::mat& xy_us, bool force_linear)
{
    initialize(xy_us, force_linear);
}


Interpolator::Interpolator(const arma::mat& x, const arma::mat& y, bool force_linear)
{
    arma::mat xy = arma::zeros(x.n_rows, 2);
    if (x.n_rows!=y.n_rows)
        throw insight::Exception(boost::str(boost::format("number of data points in x (%d) and y (%d) array differs!")%x.n_rows%y.n_rows));
    xy.col(0)=x;
    xy.col(1)=y;
    initialize(xy, force_linear);
}

Interpolator::~Interpolator()
{
//   for (int i=0; i<spline.size(); i++)
//     gsl_spline_free (spline[i]);
//   gsl_interp_accel_free (acc);
}

double Interpolator::integrate(double a, double b, int col) const
{
  if (col>=spline.size())
    throw insight::Exception(str(format("requested value interpolation in data column %d while there are only %d columns!")
			    % col % spline.size()));
    
//   double small=1e-20;
//   if (first(0)-a > small);
//     throw insight::Exception(str(format("Begin of integration interval (%g) before beginning of definition interval (%g)!")
// 			    % a % first(0)));
//   if (a-last(0) > small);
//     throw insight::Exception(str(format("Begin of integration interval (%g) after end of definition interval (%g)!")
// 			    % a % last(0)));
//   if (first(0)-b>small);
//     throw insight::Exception(str(format("End of integration interval (%g) before beginning of definition interval (%g)!")
// 			    % b % first(0)));
//   if (b-last(0)>small);
//     throw insight::Exception(str(format("End of integration interval (%g) after end of definition interval (%g)!")
// 			    % b % last(0)));
  
  return gsl_spline_eval_integ( &(spline[col]), a, b, &(*acc) );
}

double Interpolator::y(double x, int col, OutOfBounds* outOfBounds) const
{
  if (col>=spline.size())
    throw insight::Exception(str(format("requested value interpolation in data column %d while there are only %d columns!")
			    % col % spline.size()));
    
  if (x<first_(0)) { if (outOfBounds) *outOfBounds=IP_OUTBOUND_SMALL; return first_(col+1); }
  if (x>last_(0)) { if (outOfBounds) *outOfBounds=IP_OUTBOUND_LARGE; return last_(col+1); }
  if (outOfBounds) *outOfBounds=IP_INBOUND;

  double v=gsl_spline_eval (&(spline[col]), x, &(*acc));
  return v;
}

double Interpolator::dydx(double x, int col, OutOfBounds* outOfBounds) const
{
  if (col>=spline.size())
    throw insight::Exception(str(format("requested derivative interpolation in data column %d while there are only %d columns!")
			    % col % spline.size()));
    
  if (x<first_(0)) { if (outOfBounds) *outOfBounds=IP_OUTBOUND_SMALL; return dydx(first_(0), col); }
  if (x>last_(0)) { if (outOfBounds) *outOfBounds=IP_OUTBOUND_LARGE; return dydx(last_(0), col); }
  if (outOfBounds) *outOfBounds=IP_INBOUND;

  double v=gsl_spline_eval_deriv (&(spline[col]), x, &(*acc));
  return v;
}

arma::mat Interpolator::operator()(double x, OutOfBounds* outOfBounds) const
{
  arma::mat result=zeros(1, spline.size());
  for (int i=0; i<spline.size(); i++)
    result(0,i)=y(x, i, outOfBounds);
  return result;
}

arma::mat Interpolator::dydxs(double x, OutOfBounds* outOfBounds) const
{
  arma::mat result=zeros(1, spline.size());
  for (int i=0; i<spline.size(); i++)
    result(0,i)=dydx(x, i, outOfBounds);
  return result;
}

arma::mat Interpolator::operator()(const arma::mat& x, OutOfBounds* outOfBounds) const
{
  arma::mat result=zeros(x.n_rows, spline.size());
  for (int j=0; j<x.n_rows; j++)
  {
    result.row(j) = this->operator()(x(j), outOfBounds);
  }
  return result;
}

arma::mat Interpolator::dydxs(const arma::mat& x, OutOfBounds* outOfBounds) const
{
  arma::mat result=zeros(x.n_rows, spline.size());
  for (int j=0; j<x.n_rows; j++)
  {
    result.row(j) = this->dydxs(x(j), outOfBounds);
  }
  return result;
}

arma::mat Interpolator::xy(const arma::mat& x, OutOfBounds* outOfBounds) const
{
  return arma::mat(join_rows(x, operator()(x, outOfBounds)));
}

arma::mat integrate(const arma::mat& xy)
{
  arma::mat integ(zeros(xy.n_cols-1));
  
  arma::mat x = xy.col(0);
  arma::mat y = xy.cols(1, xy.n_cols-1);

  for (int i=0; i<xy.n_rows-1; i++)
  {
    integ += 0.5*( y(i) + y(i+1) ) * ( x(i+1) - x(i) );
  }
  
  return integ;
}

struct p_int
{
  const Interpolator* ipol_;
  int col_;
};

double f_int (double x, void * params) {
  p_int* p = static_cast<p_int *>(params);
  return p->ipol_->y(x, p->col_);
}

double integrate(const Interpolator& ipol, double a, double b, int col)
{
  gsl_integration_workspace * w 
    = gsl_integration_workspace_alloc (1000);

  double result, error;
  
  p_int p;
  p.ipol_=&ipol;
  p.col_=col;
  gsl_function F;
  F.function = &f_int;
  F.params = &p;

  gsl_integration_qags 
  (
    &F, 
    a, b, 
    0, 1e-5, 1000,
    w, &result, &error
  ); 
  cout<<"integration residual = "<<error<<" (result="<<result<<")"<<endl;

  gsl_integration_workspace_free (w);

  return result;
}

arma::mat integrate(const Interpolator& ipol, double a, double b)
{
  arma::mat res=zeros(ipol.ncol());
  for (int i=0; i<res.n_elem; i++)
  {
    res(i)=integrate(ipol, a, b, i);
  }
  return res;
}

#define SMALL 1e-10
bool compareArmaMat::operator()(const arma::mat& v1, const arma::mat& v2) const
{
  if ( fabs(v1(0) - v2(0))<SMALL )
    {
      if ( fabs(v1(1) - v2(1))<SMALL )
        {
          if (fabs(v1(2)-v2(2))<SMALL)
          {
              //return v1.instance_ < v2.instance_;
              return false;
          }
          else return v1(2)<v2(2);
        }
      else return v1(1)<v2(1);
    }
  else return v1(0)<v2(0);
}

}
