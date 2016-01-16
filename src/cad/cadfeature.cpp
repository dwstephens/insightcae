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

#include "geotest.h"

#include <memory>

#include "cadfeature.h"
#include "datum.h"
#include "sketch.h"
#include "transform.h"

#include <base/exception.h>
#include "boost/foreach.hpp"
#include <boost/iterator/counting_iterator.hpp>
#include "boost/make_shared.hpp"

#include "dxfwriter.h"
#include "featurefilter.h"
#include "gp_Cylinder.hxx"

#include <BRepLib_FindSurface.hxx>
#include <BRepCheck_Shell.hxx>
#include "BRepOffsetAPI_NormalProjection.hxx"

#include "openfoam/openfoamdict.h"


namespace qi = boost::spirit::qi;
namespace repo = boost::spirit::repository;
namespace phx   = boost::phoenix;

using namespace std;
using namespace boost;

namespace boost
{
 
std::size_t hash<TopoDS_Shape>::operator()(const TopoDS_Shape& shape) const
{
  insight::cad::Feature m(shape);  
  return m.hash();
}

std::size_t hash<arma::mat>::operator()(const arma::mat& v) const
{
  std::hash<double> dh;
  size_t h=0;
  for (int i=0; i<v.n_elem; i++)
  {
    boost::hash_combine(h, dh(v(i)));
  }
  return h;
}

std::size_t hash<gp_Pnt>::operator()(const gp_Pnt& v) const
{
  std::hash<double> dh;
  size_t h=0;
  boost::hash_combine(h, dh(v.X()));
  boost::hash_combine(h, dh(v.Y()));
  boost::hash_combine(h, dh(v.Z()));
  return h;
}

std::size_t hash<insight::cad::Feature>::operator()(const insight::cad::Feature& m) const
{
  return m.hash();
}

}


namespace insight 
{
namespace cad 
{


ParameterListHash::ParameterListHash()
: model_(0), hash_(0)
{}

ParameterListHash::ParameterListHash(Feature *m)
: model_(m), hash_(m->hash())
{}


ParameterListHash::operator size_t ()
{
  return hash_;
}




std::ostream& operator<<(std::ostream& os, const Feature& m)
{
  os<<"ENTITIES\n================\n\n";
  BRepTools::Dump(m.shape(), os);
  os<<"\n================\n\n";
  return os;
}

defineType(Feature);
defineFactoryTable(Feature, NoParameters);
addToFactoryTable(Feature, Feature, NoParameters);

TopoDS_Shape Feature::loadShapeFromFile(const boost::filesystem::path& filename)
{
  cout<<"Reading "<<filename<<endl;
    
  std::string ext=filename.extension().string();
  std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
  
  if (ext==".brep")
  {
    BRep_Builder bb;
    TopoDS_Shape s;
    BRepTools::Read(s, filename.c_str(), bb);
    return s;
  } 
  else if ( (ext==".igs") || (ext==".iges") )
  {
    IGESControl_Reader igesReader;

    igesReader = IGESControl_Reader();
    igesReader.ReadFile(filename.c_str());
    igesReader.TransferRoots();

    return igesReader.OneShape();
  } 
  else if ( (ext==".stp") || (ext==".step") )
  {
    STEPControl_Reader stepReader;

    stepReader = STEPControl_Reader();
    stepReader.ReadFile(filename.c_str());
    stepReader.TransferRoots();

    return stepReader.OneShape();
  } 
  else
  {
    throw insight::Exception("Unknown import file format! (Extension "+ext+")");
    return TopoDS_Shape();
  }  
}

void Feature::setShapeHash()
{
  // create hash from
  // 1. total volume
  // 2. # vertices
  // 3. # faces
  // 4. vertex locations
  
  boost::hash_combine(hash_, boost::hash<double>()(modelVolume()));
  boost::hash_combine(hash_, boost::hash<int>()(vmap_.Extent()));
  boost::hash_combine(hash_, boost::hash<int>()(fmap_.Extent()));

  FeatureSetData vset=allVerticesSet();
  BOOST_FOREACH(const insight::cad::FeatureID& j, vset)
  {
    boost::hash_combine
    (
      hash_, 
      boost::hash<arma::mat>()(vertexLocation(j))
    );    
  }
}

void Feature::setShape(const TopoDS_Shape& shape)
{
  shape_=shape;
  nameFeatures();
  setValid();
}


Feature::Feature(const NoParameters&)
: isleaf_(true),
  density_(1.0),
  areaWeight_(0.0),
  hash_(0)
{
}

Feature::Feature(const Feature& o)
: isleaf_(true),
  providedSubshapes_(o.providedSubshapes_),
  providedDatums_(o.providedDatums_),
  density_(1.0),
  areaWeight_(0.0),
  explicitCoG_(o.explicitCoG_),
  explicitMass_(o.explicitMass_),
  hash_(o.hash_)
{
  setShape(o.shape_);
}

Feature::Feature(const TopoDS_Shape& shape)
: isleaf_(true),
  density_(1.0),
  areaWeight_(0.0),
  hash_(0)
{
  setShape(shape);
  setShapeHash();
  setValid();
}

Feature::Feature(const boost::filesystem::path& filepath)
: isleaf_(true),
  density_(1.0),
  areaWeight_(0.0),
  hash_(0)
{
  setShape(loadShapeFromFile(filepath));
  setShapeHash();
  setValid();
}

Feature::Feature(FeatureSetPtr creashapes)
: creashapes_(creashapes)
{}


Feature::~Feature()
{
}

double Feature::mass() const
{
  checkForBuildDuringAccess();
  if (explicitMass_)
  {
    cout<<"Explicit mass = "<<*explicitMass_<<endl;
    return *explicitMass_;
  }
  else
  {
    double mtot=density_*modelVolume()+areaWeight_*modelSurfaceArea();
    cout<<"Computed mass rho / V = "<<density_<<" / "<<modelVolume()
	<<", mf / A = "<<areaWeight_<<" / "<<modelSurfaceArea()
	<<", m = "<<mtot<<endl;
    return mtot;
  }
}

void Feature::build()
{
  TopoDS_Compound comp;
  BRep_Builder builder;
  builder.MakeCompound( comp );

  BOOST_FOREACH(const FeatureID& id, creashapes_->data())
  {
    TopoDS_Shape entity;
    if (creashapes_->shape()==Vertex)
    {
      entity=creashapes_->model()->vertex(id);
    }
    else if (creashapes_->shape()==Edge)
    {
      entity=creashapes_->model()->edge(id);
    }
    else if (creashapes_->shape()==Face)
    {
      entity=creashapes_->model()->face(id);
    }
    else if (creashapes_->shape()==Solid)
    {
      entity=creashapes_->model()->subsolid(id);
    }
    if (creashapes_->size()==1)
    {
      setShape(entity);
      return;
    }
    else
    {
      builder.Add(comp, entity);
    }
  }
  
  setShape(comp);
}


void Feature::setMassExplicitly(double m) 
{ 
  explicitMass_.reset(new double(m));
}

void Feature::setCoGExplicitly(const arma::mat& cog) 
{ 
  explicitCoG_.reset(new arma::mat(cog));
}

FeaturePtr Feature::subshape(const std::string& name)
{
  checkForBuildDuringAccess();
  SubfeatureMap::iterator i = providedSubshapes_.find(name);
  if (i==providedSubshapes_.end())
  {
    throw insight::Exception("Subfeature "+name+" is not present!");
    return FeaturePtr();
  }
  else
  {
    return i->second;
  }
}

Feature& Feature::operator=(const Feature& o)
{
  ASTBase::operator=(o);
  if (o.valid())
  {
    setShape(o.shape_);
    explicitCoG_=o.explicitCoG_;
    explicitMass_=o.explicitMass_;
  }
  return *this;
}

bool Feature::operator==(const Feature& o) const
{
  return shape()==o.shape();
}


GeomAbs_CurveType Feature::edgeType(FeatureID i) const
{
  const TopoDS_Edge& e = edge(i);
  double t0, t1;
  Handle_Geom_Curve crv=BRep_Tool::Curve(e, t0, t1);
  GeomAdaptor_Curve adapt(crv);
  return adapt.GetType();
}

GeomAbs_SurfaceType Feature::faceType(FeatureID i) const
{
  const TopoDS_Face& f = face(i);
  double t0, t1;
  Handle_Geom_Surface surf=BRep_Tool::Surface(f);
  GeomAdaptor_Surface adapt(surf);
  return adapt.GetType();
}

arma::mat Feature::vertexLocation(FeatureID i) const
{
  gp_Pnt cog=BRep_Tool::Pnt(vertex(i));
  return insight::vec3( cog.X(), cog.Y(), cog.Z() );
}

arma::mat Feature::edgeCoG(FeatureID i) const
{
  GProp_GProps props;
  BRepGProp::LinearProperties(edge(i), props);
  gp_Pnt cog = props.CentreOfMass();
  return insight::vec3( cog.X(), cog.Y(), cog.Z() );
}

arma::mat Feature::faceCoG(FeatureID i) const
{
  GProp_GProps props;
  BRepGProp::SurfaceProperties(face(i), props);
  gp_Pnt cog = props.CentreOfMass();
  return insight::vec3( cog.X(), cog.Y(), cog.Z() );
}

arma::mat Feature::subsolidCoG(FeatureID i) const
{
  GProp_GProps props;
  BRepGProp::VolumeProperties(subsolid(i), props);
  gp_Pnt cog = props.CentreOfMass();
  return insight::vec3( cog.X(), cog.Y(), cog.Z() );
}

arma::mat Feature::modelCoG() const
{
  checkForBuildDuringAccess();
  if (explicitCoG_)
  {
    return *explicitCoG_;
  }
  else
  {
    GProp_GProps props;
    TopoDS_Shape sh=shape();
    BRepGProp::VolumeProperties(sh, props);
    gp_Pnt cog = props.CentreOfMass();
    return vec3(cog);
  }
}

double Feature::modelVolume() const
{
  TopoDS_Shape sh=shape();
  TopExp_Explorer ex(sh, TopAbs_SOLID);
  if (ex.More())
  {
    GProp_GProps props;
    BRepGProp::VolumeProperties(sh, props);
    return props.Mass();
  }
  else
  {
    return 0.;
  }
}

double Feature::modelSurfaceArea() const
{
  checkForBuildDuringAccess();
  TopoDS_Shape sh=shape();
  TopExp_Explorer ex(sh, TopAbs_FACE);
  if (ex.More())
  {
    GProp_GProps props;
    BRepGProp::SurfaceProperties(sh, props);
    return props.Mass();
  }
  else
  {
    return 0.;
  }
}

double Feature::minDist(const arma::mat& p) const
{
  BRepExtrema_DistShapeShape dss
  (
    BRepBuilderAPI_MakeVertex(to_Pnt(p)).Vertex(), 
    shape()
  );
  
  if (!dss.Perform())
    throw insight::Exception("determination of minimum distance to point failed!");
  return dss.Value();
}

double Feature::maxVertexDist(const arma::mat& p) const
{
  double maxdist=0.;
  for (TopExp_Explorer ex(shape(), TopAbs_VERTEX); ex.More(); ex.Next())
  {
    TopoDS_Vertex v=TopoDS::Vertex(ex.Current());
    arma::mat vp=insight::Vector(BRep_Tool::Pnt(v)).t();
    maxdist=std::max(maxdist, norm(p-vp,2));
  }
  return maxdist;
}

double Feature::maxDist(const arma::mat& p) const
{
  if (!isSingleFace())
    throw insight::Exception("max distance determination from anything else than single face shapes is currently not supported!");
  
  BRepExtrema_ExtPF epf
  ( 
    BRepBuilderAPI_MakeVertex(to_Pnt(p)).Vertex(), 
    TopoDS::Face(asSingleFace()), 
    Extrema_ExtFlag_MAX,
    Extrema_ExtAlgo_Tree
  );
  std::cout<<"Nb="<<epf.NbExt()<<std::endl;
  if (!epf.IsDone())
    throw insight::Exception("determination of maximum distance to point failed!");
  double maxdistsq=0.;
  for (int i=1; i<epf.NbExt(); i++)
  {
    maxdistsq=std::max(maxdistsq, epf.SquareDistance(i));
  }
  return sqrt(maxdistsq);
}

arma::mat Feature::modelBndBox(double deflection) const
{
  checkForBuildDuringAccess();
  
  if (deflection>0)
  {
      BRepMesh_IncrementalMesh Inc(shape(), deflection);
  }

  Bnd_Box boundingBox;
  BRepBndLib::Add(shape(), boundingBox);

  arma::mat x=arma::zeros(3,2);
  double g=boundingBox.GetGap();
  cout<<"gap="<<g<<endl;
  boundingBox.Get
  (
    x(0,0), x(1,0), x(2,0), 
    x(0,1), x(1,1), x(2,1)
  );
  x.col(0)+=g;
  x.col(1)-=g;

  return x;
}


arma::mat Feature::faceNormal(FeatureID i) const
{
  BRepGProp_Face prop(face(i));
  double u1,u2,v1,v2;
  prop.Bounds(u1, u2, v1, v2);
  double u = (u1+u2)/2.;
  double v = (v1+v2)/2.;
  gp_Vec vec;
  gp_Pnt pnt;
  prop.Normal(u,v,pnt,vec);
  vec.Normalize();
  return insight::vec3( vec.X(), vec.Y(), vec.Z() );  
}

FeatureSetData Feature::allVerticesSet() const
{
  checkForBuildDuringAccess();
  FeatureSetData fsd;
  fsd.insert(
    boost::counting_iterator<int>( 1 ), 
    boost::counting_iterator<int>( vmap_.Extent()+1 ) 
  );
  return fsd;
}

FeatureSetData Feature::allEdgesSet() const
{
  checkForBuildDuringAccess();
  FeatureSetData fsd;
  fsd.insert(
    boost::counting_iterator<int>( 1 ), 
    boost::counting_iterator<int>( emap_.Extent()+1 ) 
  );
  return fsd;
}

FeatureSetData Feature::allFacesSet() const
{
  checkForBuildDuringAccess();
  FeatureSetData fsd;
  fsd.insert(
    boost::counting_iterator<int>( 1 ), 
    boost::counting_iterator<int>( fmap_.Extent()+1 ) 
  );
  return fsd;
}

FeatureSetData Feature::allSolidsSet() const
{
  checkForBuildDuringAccess();
  FeatureSetData fsd;  
  fsd.insert(
    boost::counting_iterator<int>( 1 ), 
    boost::counting_iterator<int>( somap_.Extent()+1 ) 
  );
  return fsd;
}

FeatureSet Feature::allVertices() const
{
  FeatureSet f(shared_from_this(), Vertex);
  f.setData(allVerticesSet());
  return f;
}

FeatureSet Feature::allEdges() const
{
  FeatureSet f(shared_from_this(), Edge);
  f.setData(allEdgesSet());
  return f;
}

FeatureSet Feature::allFaces() const
{
  FeatureSet f(shared_from_this(), Edge);
  f.setData(allFacesSet());
  return f;
}

FeatureSet Feature::allSolids() const
{
  FeatureSet f(shared_from_this(), Solid);
  f.setData(allSolidsSet());
  return f;
}

FeatureSetData Feature::query_vertices(FilterPtr f) const
{
  return query_vertices_subset(allVertices(), f);
}

FeatureSetData Feature::query_vertices(const string& queryexpr, const FeatureSetParserArgList& refs) const
{
  std::istringstream is(queryexpr);
  return query_vertices(parseVertexFilterExpr(is, refs));
}


FeatureSetData Feature::query_vertices_subset(const FeatureSetData& fs, FilterPtr f) const
{
//   Filter::Ptr f(filter.clone());
  checkForBuildDuringAccess();
  
  f->initialize(shared_from_this());
  BOOST_FOREACH(int i, fs)
  {
    f->firstPass(i);
  }
  FeatureSetData res;
  BOOST_FOREACH(int i, fs)
  {
    if (f->checkMatch(i)) res.insert(i);
  }
  cout<<"QUERY_VERTICES RESULT = "<<res<<endl;
  return res;
}

FeatureSetData Feature::query_vertices_subset(const FeatureSetData& fs, const std::string& queryexpr, const FeatureSetParserArgList& refs) const
{
  std::istringstream is(queryexpr);
  return query_vertices_subset(fs, parseVertexFilterExpr(is, refs));
}



FeatureSetData Feature::query_edges(FilterPtr f) const
{
//   Filter::Ptr f(filter.clone());
  return query_edges_subset(allEdges(), f);
}

FeatureSetData Feature::query_edges(const std::string& queryexpr, const FeatureSetParserArgList& refs) const
{
  std::istringstream is(queryexpr);
  return query_edges(parseEdgeFilterExpr(is, refs));
}

FeatureSetData Feature::query_edges_subset(const FeatureSetData& fs, FilterPtr f) const
{
  checkForBuildDuringAccess();
  
  f->initialize(shared_from_this());
  //for (int i=1; i<=emap_.Extent(); i++)
  BOOST_FOREACH(int i, fs)
  {
    f->firstPass(i);
  }
  FeatureSetData res;
  //for (int i=1; i<=emap_.Extent(); i++)
  BOOST_FOREACH(int i, fs)
  {
    if (f->checkMatch(i)) res.insert(i);
  }
  cout<<"QUERY_EDGES RESULT = "<<res<<endl;
  return res;
}

FeatureSetData Feature::query_edges_subset(const FeatureSetData& fs, const std::string& queryexpr, const FeatureSetParserArgList& refs) const
{
  std::istringstream is(queryexpr);
  return query_edges_subset(fs, parseEdgeFilterExpr(is, refs));
}

FeatureSetData Feature::query_faces(FilterPtr f) const
{
  return query_faces_subset(allFaces(), f);
}

FeatureSetData Feature::query_faces(const string& queryexpr, const FeatureSetParserArgList& refs) const
{
  std::istringstream is(queryexpr);
  return query_faces(parseFaceFilterExpr(is, refs));
}


FeatureSetData Feature::query_faces_subset(const FeatureSetData& fs, FilterPtr f) const
{
//   Filter::Ptr f(filter.clone());
  checkForBuildDuringAccess();
  
  f->initialize(shared_from_this());
  BOOST_FOREACH(int i, fs)
  {
    f->firstPass(i);
  }
  FeatureSetData res;
  BOOST_FOREACH(int i, fs)
  {
    bool ok=f->checkMatch(i);
    if (ok) std::cout<<"match! ("<<i<<")"<<std::endl;
    if (ok) res.insert(i);
  }
  cout<<"QUERY_FACES RESULT = "<<res<<endl;
  return res;
}

FeatureSetData Feature::query_faces_subset(const FeatureSetData& fs, const std::string& queryexpr, const FeatureSetParserArgList& refs) const
{
  std::istringstream is(queryexpr);
  return query_faces_subset(fs, parseFaceFilterExpr(is, refs));
}

FeatureSetData Feature::query_solids(FilterPtr f) const
{
  return query_solids_subset(allSolids(), f);
}

FeatureSetData Feature::query_solids(const string& queryexpr, const FeatureSetParserArgList& refs) const
{
  std::istringstream is(queryexpr);
  return query_solids(parseSolidFilterExpr(is, refs));
}


FeatureSetData Feature::query_solids_subset(const FeatureSetData& fs, FilterPtr f) const
{
//   Filter::Ptr f(filter.clone());
  checkForBuildDuringAccess();
  
  f->initialize(shared_from_this());
  BOOST_FOREACH(int i, fs)
  {
    f->firstPass(i);
  }
  FeatureSetData res;
  BOOST_FOREACH(int i, fs)
  {
    if (f->checkMatch(i)) res.insert(i);
  }
  cout<<"QUERY_SOLIDS RESULT = "<<res<<endl;
  return res;
}

FeatureSetData Feature::query_solids_subset(const FeatureSetData& fs, const std::string& queryexpr, const FeatureSetParserArgList& refs) const
{
  std::istringstream is(queryexpr);
  return query_solids_subset(fs, parseSolidFilterExpr(is, refs));
}

FeatureSet Feature::verticesOfEdge(const FeatureID& e) const
{
  FeatureSet vertices(shared_from_this(), Vertex);
  FeatureSetData fsd;
  fsd.insert(vmap_.FindIndex(TopExp::FirstVertex(edge(e))));
  fsd.insert(vmap_.FindIndex(TopExp::LastVertex(edge(e))));
  vertices.setData(fsd);
  return vertices;
}

FeatureSet Feature::verticesOfEdges(const FeatureSet& es) const
{
  FeatureSet vertices(shared_from_this(), Vertex);
  FeatureSetData fsd;
  BOOST_FOREACH(FeatureID i, es.data())
  {
    FeatureSet j=verticesOfEdge(i);
    fsd.insert(j.data().begin(), j.data().end());
  }
  vertices.setData(fsd);
  return vertices;
}

FeatureSet Feature::verticesOfFace(const FeatureID& f) const
{
  FeatureSet vertices(shared_from_this(), Vertex);
  FeatureSetData fsd;
  for (TopExp_Explorer ex(face(f), TopAbs_VERTEX); ex.More(); ex.Next())
  {
    fsd.insert(vmap_.FindIndex(TopoDS::Vertex(ex.Current())));
  }
  vertices.setData(fsd);
  return vertices;
}

FeatureSet Feature::verticesOfFaces(const FeatureSet& fs) const
{
  FeatureSet vertices(shared_from_this(), Vertex);
  FeatureSetData fsd;
  BOOST_FOREACH(FeatureID i, fs.data())
  {
    FeatureSet j=verticesOfFace(i);
    fsd.insert(j.data().begin(), j.data().end());
  }
  vertices.setData(fsd);
  return vertices;
}



void Feature::saveAs(const boost::filesystem::path& filename) const
{
  std::string ext=filename.extension().string();
  std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
  cout<<filename<<" >> "<<ext<<endl;
  if (ext==".brep")
  {
    BRepTools::Write(shape(), filename.c_str());
  } 
  else if ( (ext==".igs") || (ext==".iges") )
  {
    Interface_Static::SetIVal("write.iges.brep.mode", 1);
    IGESControl_Controller igesctrl;
    igesctrl.Init();
    IGESControl_Writer igeswriter;
    igeswriter.AddShape(shape());
    igeswriter.Write(filename.c_str());
  } 
  else if ( (ext==".stp") || (ext==".step") )
  {
    STEPControl_Writer stepwriter;
    stepwriter.Transfer(shape(), STEPControl_AsIs);
    stepwriter.Write(filename.c_str());
  } 
  else if ( (ext==".stl") || (ext==".stlb") )
  {
    StlAPI_Writer stlwriter;

    stlwriter.ASCIIMode() = (ext==".stl");
    //stlwriter.RelativeMode()=false;
    //stlwriter.SetDeflection(maxdefl);
    stlwriter.SetCoefficient(5e-5);
    stlwriter.Write(shape(), filename.c_str());
  }
  else
  {
    throw insight::Exception("Unknown export file format! (Extension "+ext+")");
  }
}

void Feature::exportSTL(const boost::filesystem::path& filename, double abstol) const
{
  TopoDS_Shape os=shape();
  
  ShapeFix_ShapeTolerance sf;
  sf.SetTolerance(os, abstol);
  
  BRepMesh_IncrementalMesh binc(os, abstol);
  
  StlAPI_Writer stlwriter;

  stlwriter.ASCIIMode() = false;
  stlwriter.RelativeMode()=false;
  stlwriter.SetDeflection(abstol);
  stlwriter.Write(shape(), filename.c_str());
}


void Feature::exportEMesh
(
  const boost::filesystem::path& filename, 
  const FeatureSet& fs, 
  double abstol,
  double maxlen
)
{
  if (fs.shape()!=Edge) 
    throw insight::Exception("Called with incompatible feature set!");
  
  std::vector<arma::mat> points;
  typedef std::pair<int, int> Edge;
  std::vector<Edge> edges;
  
  BOOST_FOREACH(const FeatureID& fi, fs.data())
  {
    
    TopoDS_Edge e=fs.model()->edge(fi);
    if (!BRep_Tool::Degenerated(e))
    {
//       std::cout<<"feature "<<fi<<std::endl;
//       BRepTools::Dump(e, std::cout);
      BRepAdaptor_Curve ac(e);
      GCPnts_QuasiUniformDeflection qud(ac, abstol);

      if (!qud.IsDone())
	throw insight::Exception("Discretization of curves into eMesh failed!");

      double clen=0.;
      arma::mat lp;
      std::set<int> splits;
      int iofs=points.size();
      for (int j=1; j<=qud.NbPoints(); j++)
      {
	gp_Pnt pp=ac.Value(qud.Parameter(j));

	arma::mat p=insight::Vector(pp);
	if (j>1) clen+=norm(p-lp, 2);

	lp=p;
	if ((clen>maxlen) && (j>1)) 
	{
	  points.push_back(lp+0.5*(p-lp)); // insert a second point at same loc
	  points.push_back(p); // insert a second point at same loc
	  splits.insert(points.size()-1); 
	  clen=0.0; 
	}
	else
	{
	  points.push_back(p);
	}
      }
      
      for (int i=1; i<points.size()-iofs; i++)
      {
	int from=iofs+i-1;
	int to=iofs+i;
	if (splits.find(to)==splits.end())
	  edges.push_back(Edge(from,to));
      }
    }
  }
  
  std::ofstream f(filename.c_str());
  f<<"FoamFile {"<<endl
   <<" version     2.0;"<<endl
   <<" format      ascii;"<<endl
   <<" class       featureEdgeMesh;"<<endl
   <<" location    \"\";"<<endl
   <<" object      "<<filename.filename().string()<<";"<<endl
   <<"}"<<endl;
   
  f<<points.size()<<endl
   <<"("<<endl;
  BOOST_FOREACH(const arma::mat& p, points)
  {
    f<<OFDictData::to_OF(p)<<endl;
  }
  f<<")"<<endl;

  f<<edges.size()<<endl
   <<"("<<endl;
  BOOST_FOREACH(const Edge& e, edges)
  {
    f<<"("<<e.first<<" "<<e.second<<")"<<endl;
  }
  f<<")"<<endl;

}


Feature::operator const TopoDS_Shape& () const 
{ 
  return shape(); 
}

const TopoDS_Shape& Feature::shape() const
{
  if (building())
    throw insight::Exception("Internal error: recursion during build!");
  checkForBuildDuringAccess();
  return shape_;
}


Feature::View Feature::createView
(
  const arma::mat p0,
  const arma::mat n,
  bool section
) const
{
  View result_view;
  
  TopoDS_Shape dispshape=shape();
  
  gp_Pnt p_base = gp_Pnt(p0(0), p0(1), p0(2));
  gp_Dir view_dir = gp_Dir(n(0), n(1), n(2));
  
  gp_Ax2 viewCS(p_base, view_dir); 
  HLRAlgo_Projector projector( viewCS );
  gp_Trsf transform=projector.FullTransformation();

  if (section)
  {
    gp_Dir normal = -view_dir;
    gp_Pln plane = gp_Pln(p_base, normal);
    gp_Pnt refPnt = gp_Pnt(p_base.X()-normal.X(), p_base.Y()-normal.Y(), p_base.Z()-normal.Z());
    
    TopoDS_Face Face = BRepBuilderAPI_MakeFace(plane);
    TopoDS_Shape HalfSpace = BRepPrimAPI_MakeHalfSpace(Face,refPnt).Solid();
    
    TopoDS_Compound dispshapes, xsecs;
    BRep_Builder builder1, builder2;
    builder1.MakeCompound( dispshapes );
    builder2.MakeCompound( xsecs );
    int i=-1, j=0;
    for (TopExp_Explorer ex(shape(), TopAbs_SOLID); ex.More(); ex.Next())
    {
      i++;
      try
      {
	builder1.Add(dispshapes, 	BRepAlgoAPI_Cut(ex.Current(), HalfSpace));
	builder2.Add(xsecs, 		BRepBuilderAPI_Transform(BRepAlgoAPI_Common(ex.Current(), Face), transform).Shape());
	j++;
      }
      catch (...)
      {
	cout<<"Warning: Failed to compute cross section of solid #"<<i<<endl;
      }
    }
    cout<<"Generated "<<j<<" cross-sections"<<endl;
    dispshape=dispshapes;
    result_view.crossSection = xsecs;
  }
  
  
  Handle_HLRBRep_Algo brep_hlr = new HLRBRep_Algo;
  brep_hlr->Add( dispshape );
  brep_hlr->Projector( projector );
  brep_hlr->Update();
  brep_hlr->Hide();

  // extracting the result sets:
  HLRBRep_HLRToShape shapes( brep_hlr );
  
  TopoDS_Compound allVisible;
  BRep_Builder builder;
  builder.MakeCompound( allVisible );
  TopoDS_Shape vs=shapes.VCompound();
  if (!vs.IsNull()) builder.Add(allVisible, vs);
  TopoDS_Shape r1vs=shapes.Rg1LineVCompound();
  if (!r1vs.IsNull()) builder.Add(allVisible, r1vs);
  TopoDS_Shape olvs = shapes.OutLineVCompound();
  if (!olvs.IsNull()) builder.Add(allVisible, olvs);
  
  TopoDS_Shape HiddenEdges = shapes.HCompound();
  
  result_view.visibleEdges=allVisible;
  result_view.hiddenEdges=HiddenEdges;
  
  return result_view;
  
//   BRepTools::Write(allVisible, "visible.brep");
//   BRepTools::Write(HiddenEdges, "hidden.brep");
//   
//   {
//     std::vector<LayerDefinition> addlayers;
//     if (section) addlayers.push_back
//     (
//       LayerDefinition("section", DL_Attributes(std::string(""), DL_Codes::black, 35, "CONTINUOUS"), false)
//     );
//     
//     DXFWriter dxf("view.dxf", addlayers);
//     
//     dxf.writeShapeEdges(allVisible, "0");
//     dxf.writeShapeEdges(HiddenEdges, "0_HL");
//     
//     if (!secshape.IsNull())
//     {
//       BRepTools::Write(secshape, "section.brep");
//       dxf.writeSection( BRepBuilderAPI_Transform(secshape, transform).Shape(), "section");
//     }
//   }
  
}

void Feature::insertrule(parser::ISCADParser& ruleset) const
{
  ruleset.modelstepFunctionRules.add
  (
    "asModel",	
    typename parser::ISCADParser::ModelstepRulePtr(new typename parser::ISCADParser::ModelstepRule( 

    ( '(' > ( ruleset.r_vertexFeaturesExpression | ruleset.r_edgeFeaturesExpression | ruleset.r_faceFeaturesExpression | ruleset.r_solidFeaturesExpression ) >> ')' ) 
	[ qi::_val = phx::construct<FeaturePtr>(phx::new_<Feature>(qi::_1)) ]
      
    ))
  );
}

bool Feature::isSingleEdge() const
{
  return false;
}

bool Feature::isSingleFace() const
{
  return false;
}

bool Feature::isSingleOpenWire() const
{
  return false;
}

bool Feature::isSingleClosedWire() const
{
  return false;
}

bool Feature::isSingleWire() const
{
  return (isSingleOpenWire() || isSingleClosedWire());
}


bool Feature::isSingleVolume() const
{
  return false;
}

TopoDS_Edge Feature::asSingleEdge() const
{
  if (!isSingleEdge())
    throw insight::Exception("Feature "+type()+" does not provide a single edge!");
  else
    return TopoDS::Edge(shape());
}

TopoDS_Face Feature::asSingleFace() const
{
  if (!isSingleFace())
    throw insight::Exception("Feature "+type()+" does not provide a single face!");
  else
    return TopoDS::Face(shape());
}

TopoDS_Wire Feature::asSingleOpenWire() const
{
  if (!isSingleOpenWire())
    throw insight::Exception("Feature "+type()+" does not provide a single open wire!");
  else
  {
    return asSingleWire();
  }
}

TopoDS_Wire Feature::asSingleClosedWire() const
{
  if (!isSingleClosedWire())
    throw insight::Exception("Feature "+type()+" does not provide a single closed wire!");
  else
  {
    return asSingleWire();
  }
}

TopoDS_Wire Feature::asSingleWire() const
{
  if (isSingleWire())
    return TopoDS::Wire(shape());
  else
    throw insight::Exception("Feature "+type()+" does not provide a single wire!");
}


TopoDS_Shape Feature::asSingleVolume() const
{
  if (!isSingleVolume())
    throw insight::Exception("Feature "+type()+" does not provide a single volume!");
  else
    return shape();
}

void Feature::copyDatums(const Feature& m1, const std::string& prefix)
{
  // Transform all ref points and ref vectors
  BOOST_FOREACH(const RefValuesList::value_type& v, m1.getDatumScalars())
  {
    if (refvalues_.find(prefix+v.first)!=refvalues_.end())
      throw insight::Exception("datum value "+prefix+v.first+" already present!");
    refvalues_[prefix+v.first]=v.second;
  }
  BOOST_FOREACH(const RefPointsList::value_type& p, m1.getDatumPoints())
  {
    if (refpoints_.find(prefix+p.first)!=refpoints_.end())
      throw insight::Exception("datum point "+prefix+p.first+" already present!");
    refpoints_[prefix+p.first]=p.second;
  }
  BOOST_FOREACH(const RefVectorsList::value_type& p, m1.getDatumVectors())
  {
    if (refvectors_.find(prefix+p.first)!=refvectors_.end())
      throw insight::Exception("datum vector "+prefix+p.first+" already present!");
    refvectors_[prefix+p.first]=p.second;
  }
  BOOST_FOREACH(const SubfeatureMap::value_type& sf, m1.providedSubshapes())
  {
#ifdef INSIGHT_CAD_DEBUG
    std::cout<<"adding subshape "<<(prefix+sf.first)<<std::endl;
#endif
    if (providedSubshapes_.find(prefix+sf.first)!=providedSubshapes_.end())
      throw insight::Exception("subshape "+prefix+sf.first+" already present!");
    providedSubshapes_[prefix+sf.first]=sf.second;
  }
}

void Feature::copyDatumsTransformed(const Feature& m1, const gp_Trsf& trsf, const std::string& prefix)
{
  // Transform all ref points and ref vectors
  BOOST_FOREACH(const RefValuesList::value_type& v, m1.getDatumScalars())
  {
    if (refvalues_.find(prefix+v.first)!=refvalues_.end())
      throw insight::Exception("datum value "+prefix+v.first+" already present!");
    refvalues_[prefix+v.first]=v.second;
  }
  BOOST_FOREACH(const RefPointsList::value_type& p, m1.getDatumPoints())
  {
    if (refpoints_.find(prefix+p.first)!=refpoints_.end())
      throw insight::Exception("datum point "+prefix+p.first+" already present!");
    refpoints_[prefix+p.first]=vec3(to_Pnt(p.second).Transformed(trsf));
  }
  BOOST_FOREACH(const RefVectorsList::value_type& p, m1.getDatumVectors())
  {
    if (refvectors_.find(prefix+p.first)!=refvectors_.end())
      throw insight::Exception("datum vector "+prefix+p.first+" already present!");
    refvectors_[prefix+p.first]=vec3(to_Vec(p.second).Transformed(trsf));
  }
  BOOST_FOREACH(const SubfeatureMap::value_type& sf, m1.providedSubshapes())
  {
#ifdef INSIGHT_CAD_DEBUG
    std::cout<<"adding subshape (transformed) "<<(prefix+sf.first)<<std::endl;
#endif
    if (providedSubshapes_.find(prefix+sf.first)!=providedSubshapes_.end())
      throw insight::Exception("subshape "+prefix+sf.first+" already present!");
    providedSubshapes_[prefix+sf.first]=FeaturePtr(new Transform(sf.second, trsf));
  }
}


const Feature::RefValuesList& Feature::getDatumScalars() const
{
  checkForBuildDuringAccess();
  return refvalues_;
}

const Feature::RefPointsList& Feature::getDatumPoints() const
{
  checkForBuildDuringAccess();
  return refpoints_;
}

const Feature::RefVectorsList& Feature::getDatumVectors() const
{
  checkForBuildDuringAccess();
  return refvectors_;
}

double Feature::getDatumScalar(const std::string& name) const
{
  checkForBuildDuringAccess();
  RefValuesList::const_iterator i = refvalues_.find(name);
  if (i!=refvalues_.end())
  {
    return i->second;
  }
  else
  {
    throw insight::Exception("the feature does not define a reference value named \""+name+"\"");
    return 0.0;
  }
}

arma::mat Feature::getDatumPoint(const std::string& name) const
{
  checkForBuildDuringAccess();
  RefPointsList::const_iterator i = refpoints_.find(name);
  if (i!=refpoints_.end())
  {
    return i->second;
  }
  else
  {
    throw insight::Exception("the feature does not define a reference point named \""+name+"\"");
    return arma::mat();
  }
}

arma::mat Feature::getDatumVector(const std::string& name) const
{
  checkForBuildDuringAccess();
  RefVectorsList::const_iterator i = refvectors_.find(name);
  if (i!=refvectors_.end())
  {
    return i->second;
  }
  else
  {
    throw insight::Exception("the feature does not define a reference vector named \""+name+"\"");
    return arma::mat();
  }
}

void Feature::nameFeatures()
{
  // Don't call "shape()" here!
  fmap_.Clear();
  emap_.Clear(); 
  vmap_.Clear(); 
  somap_.Clear(); 
  shmap_.Clear(); 
  wmap_.Clear();
  
  // Solids
  TopExp_Explorer exp0, exp1, exp2, exp3, exp4, exp5;
  for(exp0.Init(shape_, TopAbs_SOLID); exp0.More(); exp0.Next()) {
      TopoDS_Solid solid = TopoDS::Solid(exp0.Current());
      if(somap_.FindIndex(solid) < 1) {
	  somap_.Add(solid);

	  for(exp1.Init(solid, TopAbs_SHELL); exp1.More(); exp1.Next()) {
	      TopoDS_Shell shell = TopoDS::Shell(exp1.Current());
	      if(shmap_.FindIndex(shell) < 1) {
		  shmap_.Add(shell);

		  for(exp2.Init(shell, TopAbs_FACE); exp2.More(); exp2.Next()) {
		      TopoDS_Face face = TopoDS::Face(exp2.Current());
		      if(fmap_.FindIndex(face) < 1) {
			  fmap_.Add(face);

			  for(exp3.Init(exp2.Current(), TopAbs_WIRE); exp3.More(); exp3.Next()) {
			      TopoDS_Wire wire = TopoDS::Wire(exp3.Current());
			      if(wmap_.FindIndex(wire) < 1) {
				  wmap_.Add(wire);

				  for(exp4.Init(exp3.Current(), TopAbs_EDGE); exp4.More(); exp4.Next()) {
				      TopoDS_Edge edge = TopoDS::Edge(exp4.Current());
				      if(emap_.FindIndex(edge) < 1) {
					  emap_.Add(edge);

					  for(exp5.Init(exp4.Current(), TopAbs_VERTEX); exp5.More(); exp5.Next()) {
					      TopoDS_Vertex vertex = TopoDS::Vertex(exp5.Current());
					      if(vmap_.FindIndex(vertex) < 1)
						  vmap_.Add(vertex);
					  }
				      }
				  }
			      }
			  }
		      }
		  }
	      }
	  }
      }
  }

  // Free Shells
  for(exp1.Init(exp0.Current(), TopAbs_SHELL, TopAbs_SOLID); exp1.More(); exp1.Next()) {
      TopoDS_Shape shell = exp1.Current();
      if(shmap_.FindIndex(shell) < 1) {
	  shmap_.Add(shell);

	  for(exp2.Init(shell, TopAbs_FACE); exp2.More(); exp2.Next()) {
	      TopoDS_Face face = TopoDS::Face(exp2.Current());
	      if(fmap_.FindIndex(face) < 1) {
		  fmap_.Add(face);

		  for(exp3.Init(exp2.Current(), TopAbs_WIRE); exp3.More(); exp3.Next()) {
		      TopoDS_Wire wire = TopoDS::Wire(exp3.Current());
		      if(wmap_.FindIndex(wire) < 1) {
			  wmap_.Add(wire);

			  for(exp4.Init(exp3.Current(), TopAbs_EDGE); exp4.More(); exp4.Next()) {
			      TopoDS_Edge edge = TopoDS::Edge(exp4.Current());
			      if(emap_.FindIndex(edge) < 1) {
				  emap_.Add(edge);

				  for(exp5.Init(exp4.Current(), TopAbs_VERTEX); exp5.More(); exp5.Next()) {
				      TopoDS_Vertex vertex = TopoDS::Vertex(exp5.Current());
				      if(vmap_.FindIndex(vertex) < 1)
					  vmap_.Add(vertex);
				  }
			      }
			  }
		      }
		  }
	      }
	  }
      }
  }

  // Free Faces
  for(exp2.Init(shape_, TopAbs_FACE, TopAbs_SHELL); exp2.More(); exp2.Next()) {
      TopoDS_Face face = TopoDS::Face(exp2.Current());
      if(fmap_.FindIndex(face) < 1) {
	  fmap_.Add(face);

	  for(exp3.Init(exp2.Current(), TopAbs_WIRE); exp3.More(); exp3.Next()) {
	      TopoDS_Wire wire = TopoDS::Wire(exp3.Current());
	      if(wmap_.FindIndex(wire) < 1) {
		  wmap_.Add(wire);

		  for(exp4.Init(exp3.Current(), TopAbs_EDGE); exp4.More(); exp4.Next()) {
		      TopoDS_Edge edge = TopoDS::Edge(exp4.Current());
		      if(emap_.FindIndex(edge) < 1) {
			  emap_.Add(edge);

			  for(exp5.Init(exp4.Current(), TopAbs_VERTEX); exp5.More(); exp5.Next()) {
			      TopoDS_Vertex vertex = TopoDS::Vertex(exp5.Current());
			      if(vmap_.FindIndex(vertex) < 1)
				  vmap_.Add(vertex);
			  }
		      }
		  }
	      }
	  }
      }
  }

  // Free Wires
  for(exp3.Init(shape_, TopAbs_WIRE, TopAbs_FACE); exp3.More(); exp3.Next()) {
      TopoDS_Wire wire = TopoDS::Wire(exp3.Current());
      if(wmap_.FindIndex(wire) < 1) {
	  wmap_.Add(wire);

	  for(exp4.Init(exp3.Current(), TopAbs_EDGE); exp4.More(); exp4.Next()) {
	      TopoDS_Edge edge = TopoDS::Edge(exp4.Current());
	      if(emap_.FindIndex(edge) < 1) {
		  emap_.Add(edge);

		  for(exp5.Init(exp4.Current(), TopAbs_VERTEX); exp5.More(); exp5.Next()) {
		      TopoDS_Vertex vertex = TopoDS::Vertex(exp5.Current());
		      if(vmap_.FindIndex(vertex) < 1)
			  vmap_.Add(vertex);
		  }
	      }
	  }
      }
  }

  // Free Edges
  for(exp4.Init(shape_, TopAbs_EDGE, TopAbs_WIRE); exp4.More(); exp4.Next()) {
      TopoDS_Edge edge = TopoDS::Edge(exp4.Current());
      if(emap_.FindIndex(edge) < 1) {
	  emap_.Add(edge);

	  for(exp5.Init(exp4.Current(), TopAbs_VERTEX); exp5.More(); exp5.Next()) {
	      TopoDS_Vertex vertex = TopoDS::Vertex(exp5.Current());
	      if(vmap_.FindIndex(vertex) < 1)
		  vmap_.Add(vertex);
	  }
      }
  }

  // Free Vertices
  for(exp5.Init(shape_, TopAbs_VERTEX, TopAbs_EDGE); exp5.More(); exp5.Next()) {
      TopoDS_Vertex vertex = TopoDS::Vertex(exp5.Current());
      if(vmap_.FindIndex(vertex) < 1)
	  vmap_.Add(vertex);
  }
  
//   extractReferenceFeatures();
}

void Feature::extractReferenceFeatures()
{
  ///////////////////////////////////////////////////////////////////////////////
  /////////////// save reference points

  for (int i=1; i<=vmap_.Extent(); i++)
  {
    refpoints_[ str(format("v%d")%i) ] = vertexLocation(i);
  }
}

void Feature::write(const filesystem::path& file) const
{
  std::ofstream f(file.c_str());
  write(f);
}


void Feature::write(std::ostream& f) const
{
  f<<isleaf_<<endl;

  // the shape
  {
    std::ostringstream bufs;
    BRepTools::Write(shape(), bufs);
    std::string buf=bufs.str();
//     cout<<buf<<endl;
    f<<buf.size()<<endl;
    f<<buf<<endl;
  }

//   nameFeatures();

//   f<<providedSubshapes_.size()<<endl;
//   BOOST_FOREACH(const Feature::Map::value_type& i, providedSubshapes_)
//   {
//     f<<i.first<<endl;
//     i.second->write(f);
//     f<<endl;
//   }

//   typedef std::map<std::string, boost::shared_ptr<Datum> > DatumMap;
//   f<<providedDatums_.size()<<endl;
//   BOOST_FOREACH(const DatumMap::value_type& i, providedDatums_)
//   {
//     f<<i.first<<endl;
//     i.second->write(f);
//     f<<endl;
//   }


//   RefValuesList refvalues_;
  f<<refvalues_.size()<<endl;
  BOOST_FOREACH(const RefValuesList::value_type& i, refvalues_)
  {
    f<<i.first<<endl;
    f<<i.second<<endl;
  }
//   RefPointsList refpoints_;
  f<<refpoints_.size()<<endl;
  BOOST_FOREACH(const RefPointsList::value_type& i, refpoints_)
  {
    f<<i.first<<endl;
    f<<i.second(0)<<" "<<i.second(1)<<" "<<i.second(2)<<endl;
  }
//   RefVectorsList refvectors_;
  f<<refvectors_.size()<<endl;
  BOOST_FOREACH(const RefVectorsList::value_type& i, refvectors_)
  {
    f<<i.first<<endl;
    f<<i.second(0)<<" "<<i.second(1)<<" "<<i.second(2)<<endl;
  }

//   double density_, areaWeight_;
  f<<density_<<endl;
  f<<areaWeight_<<endl;
  
  f<<bool(explicitCoG_)<<endl;
  if (explicitCoG_) 
  {
    f<<(*explicitCoG_)(0)<<" "<<(*explicitCoG_)(1)<<" "<<(*explicitCoG_)(2)<<endl;
  }
  
  f<<bool(explicitMass_)<<endl;
  if (explicitMass_) f<<*explicitMass_<<endl;

}

void Feature::read(const filesystem::path& file)
{
  cout<<"Reading model of type "<<type()<<" from cache file "<<file<<endl;
  std::ifstream f(file.c_str());
  read(f);
}


void Feature::read(std::istream& f)
{
  int n;
  
  f>>isleaf_;

  {
    size_t s;
    f>>s;
    
    char buf[s+2];
    f.read(buf, s);
    buf[s]='\0';
//     cout<<buf<<endl;
    
    BRep_Builder b;
    std::istringstream bufs(buf);
    TopoDS_Shape sh;
    BRepTools::Read(sh, bufs, b);
    setShape(sh);
  }

//   f<<providedSubshapes_.size()<<endl;
//   BOOST_FOREACH(const Feature::Map::value_type& i, providedSubshapes_)
//   {
//     f<<i.first<<endl;
//     i.second->write(f);
//     f<<endl;
//   }

//   typedef std::map<std::string, boost::shared_ptr<Datum> > DatumMap;
//   int n;
// 
//   f>>n;
//   for (int i=0; i<n; i++)
//   {
//     std::string name;
//     getline(f, name);
//     providedDatums_[name].reset(new Datum(f));
//   }


//   RefValuesList refvalues_;
  f>>n;
  cout<<"reading "<<n<<" ref values"<<endl;
  for (int i=0; i<n; i++)
  {
    std::string name;
//     getline(f, name);
    f>>name;
    double v;
    f>>v;
    cout<<name<<": "<<v<<endl;
    refvalues_[name]=v;
  }
//   RefPointsList refpoints_;
  f>>n;
  cout<<"reading "<<n<<" ref points"<<endl;
  for (int i=0; i<n; i++)
  {
    std::string name;
//     getline(f, name);
    f>>name;
    double x, y, z;
    f>>x>>y>>z;
    cout<<name<<": "<<x<<" "<<y<<" "<<z<<endl;
    refpoints_[name]=vec3(x, y, z);
  }
//   RefVectorsList refvectors_;
  f>>n;
  cout<<"reading "<<n<<" ref vectors"<<endl;
  for (int i=0; i<n; i++)
  {
    std::string name;
//     getline(f, name);
    f>>name;
    double x, y, z;
    f>>x>>y>>z;
    refvectors_[name]=vec3(x, y, z);
  }

//   double density_, areaWeight_;
  f>>density_;
  f>>areaWeight_;
  
  bool has;
  
  f>>has;
  if (has)
  {
    double x,y,z;
    f>>x>>y>>z;
    explicitCoG_.reset(new arma::mat(vec3(x,y,z)));
  }
  
  f>>has;
  if (has)
  {
    double v;
    f>>v;
    explicitMass_.reset(new double(v));
  }

}




bool SingleFaceFeature::isSingleFace() const
{
  return true;
}

bool SingleFaceFeature::isSingleCloseWire() const
{
  return true;
}

TopoDS_Wire SingleFaceFeature::asSingleClosedWire() const
{
  return BRepTools::OuterWire(TopoDS::Face(shape()));;
}



bool SingleVolumeFeature::isSingleVolume() const
{
  return true;
}

FeatureCache::FeatureCache(const filesystem::path& cacheDir)
: cacheDir_(cacheDir),
  removeCacheDir_(false)
{
  if (cacheDir.empty())
  {
    removeCacheDir_=true;
    cacheDir_ = boost::filesystem::unique_path
    (
      boost::filesystem::temp_directory_path()/("iscad_cache_%%%%%%%")
    );
    boost::filesystem::create_directories(cacheDir_);
  }
}

FeatureCache::~FeatureCache()
{
  if (removeCacheDir_)
  {
    boost::filesystem::remove_all(cacheDir_);
  }
}

void FeatureCache::initRebuild()
{
  usedFilesDuringRebuild_.clear();
}

void FeatureCache::finishRebuild()
{
  // remove all cache files that have not been used
}


bool FeatureCache::contains(size_t hash) const
{
  return boost::filesystem::exists(fileName(hash));
}


filesystem::path FeatureCache::markAsUsed(size_t hash)
{
  usedFilesDuringRebuild_.insert(fileName(hash));
  return fileName(hash);
}

filesystem::path FeatureCache::fileName(size_t hash) const
{
  return boost::filesystem::absolute
  (
    cacheDir_ /
    boost::filesystem::path( str(format("%x")%hash) + ".iscad_cache" )
  );
}

FeatureCache cache;

}
}
