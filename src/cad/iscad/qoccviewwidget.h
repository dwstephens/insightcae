#ifndef QOCCVIEWWIDGET_H
#define QOCCVIEWWIDGET_H

#include "clear_occ_macros.h"
#include <QtCore>
#include <QtGui>
#include "qocc.h"
#include <QtOpenGL/QGLWidget>

#include "qmodelstepitem.h"


class QDisplayableModelTreeItem;

#if ((OCC_VERSION_MAJOR<7)&&(OCC_VERSION_MINOR>=7))
#include "Graphic3d_ClipPlane.hxx"
#endif

/** the key for multi selection */
#define MULTISELECTIONKEY  Qt::ShiftModifier   

/** The key for shortcut ( use to activate dynamic rotation, panning ) */
//#define CASCADESHORTCUTKEY Qt::ControlModifier 
#define ZOOMSHORTCUTKEY Qt::ControlModifier 
#define PANSHORTCUTKEY Qt::ShiftModifier 
#define ROTATESHORTCUTKEY Qt::AltModifier

#define ValZWMin 1 /** For elastic bean selection */

// class Handle_AIS_InteractiveContext;
// class Handle_V3d_View;

class QOCC_DECLSPEC QoccViewWidget
: public QWidget
{
	Q_OBJECT

public:

  enum CurrentAction3d 
  {	
    CurAction3d_Undefined,
    CurAction3d_Nothing, 
    CurAction3d_Picking,
    CurAction3d_DynamicZooming,
    CurAction3d_WindowZooming, 
    CurAction3d_DynamicPanning,
    CurAction3d_GlobalPanning, 
    CurAction3d_DynamicRotation,
  };

  enum CurrentInteractionMode
  {
    CIM_Normal,
    CIM_MeasurePoints,
    CIM_InsertPointIDs,
    CIM_InsertEdgeIDs,
    CIM_InsertFaceIDs
  };

protected:
  std::vector<Handle_AIS_InteractiveObject> additionalDisplayObjectsForSelection_;

public:

  QoccViewWidget
    (
     const Handle_AIS_InteractiveContext& aContext = NULL, 
     QWidget *parent = NULL, 
     Qt::WindowFlags wflags = 0 
    );
  
  ~QoccViewWidget();
  
  void initializeOCC(const Handle_AIS_InteractiveContext& aContext = NULL);

  inline Handle_AIS_InteractiveContext&	getContext( void ) { return myContext; }
  inline const Handle_V3d_View& getView( void )    { return myView; }
  inline const Handle_V3d_View& getOccView( void )    { return myView; }

  //Overrides
  QPaintEngine* paintEngine() const;
  QToolBar*	  myToolBar;

  void redraw( bool isPainting = false );

  QDisplayableModelTreeItem* getOwnerItem(Handle_AIS_InteractiveObject selected);
  QDisplayableModelTreeItem* getSelectedItem();

signals:

  void initialized();
  void graphicalSelectionChanged(QDisplayableModelTreeItem* selection, QoccViewWidget* viewer);
  void mouseMoved   ( V3d_Coordinate X, V3d_Coordinate Y, V3d_Coordinate Z );
  void pointClicked ( V3d_Coordinate X, V3d_Coordinate Y, V3d_Coordinate Z );
  void sendStatus   ( const QString aMessage );
  //! Just a placeholder for now
  void error ( int errorCode, QString& errorDescription );

  void addEvaluationToModel (const QString& name, insight::cad::PostprocActionPtr smp, bool visible);

  void insertNotebookText(const QString& text);

protected slots:
  
  void onGraphicalSelectionChanged(QDisplayableModelTreeItem* selection, QoccViewWidget* viewer);

public slots:
  
  void idle();
  void fitExtents();
  void fitAll();
  void fitArea();
  void zoom();
  void pan();
  void globalPan();
  void rotation();
  void hiddenLineOn();
  void hiddenLineOff();
  void background();
  void viewFront();
  void viewBack();
  void viewTop();
  void viewBottom();
  void viewLeft();
  void viewRight();
  void viewAxo();
  void viewTopFront();
  void viewGrid();
  void viewReset();
  void setReset();
  
  void toggleGrid ();
  void toggleClipXY ( void );
  void toggleClipYZ ( void );
  void toggleClipXZ ( void );
  void toggleClip ( double px, double py, double pz, double nx, double ny, double nz );
  
  void displayMessage(const QString& msg);

  void onShow(QDisplayableModelTreeItem* di);
  void onHide(QDisplayableModelTreeItem* di);
  void onSetDisplayMode(QDisplayableModelTreeItem* di, AIS_DisplayMode sm);
  void onSetColor(QDisplayableModelTreeItem* di, Quantity_Color c);
  void onSetResolution(QDisplayableModelTreeItem* di, double res);

  void onSetClipPlane(QObject* datumplane);

  void onMeasureDistance();
  void onSelectPoints();
  void onSelectEdges();
  void onSelectFaces();

protected: // methods

  virtual void paintEvent        ( QPaintEvent* e );
  virtual void resizeEvent       ( QResizeEvent* e );
  virtual void mousePressEvent   ( QMouseEvent* e );
  virtual void mouseReleaseEvent ( QMouseEvent* e );
  virtual void mouseMoveEvent    ( QMouseEvent* e );
  virtual void wheelEvent        ( QWheelEvent* e );
  virtual void keyPressEvent     ( QKeyEvent* e );
  virtual void keyReleaseEvent   ( QKeyEvent* e );
  
  virtual void leaveEvent	 ( QEvent * );

  void displayContextMenu( const QPoint& p);
  
private: // members
  
#ifdef WNT
  Handle_WNT_Window		myWindow;
#else
  Handle_Xw_Window		myWindow;
#endif // WNT
  
  Handle_AIS_InteractiveContext   myContext;
  Handle_V3d_View                 myView;
  Handle_V3d_Viewer               myViewer;

#if ((OCC_VERSION_MAJOR<7)&&(OCC_VERSION_MINOR<7))
  Handle_V3d_Plane		clipPlane_;
#else
  Handle_Graphic3d_ClipPlane	clipPlane_;  
#endif

  bool		myViewResized;
  bool		myViewInitialized;
  CurrentAction3d               myMode;
  double               myCurZoom;
  bool		myGridSnap;
  AIS_StatusOfDetection		myDetection;
  
  V3d_Coordinate					
    myV3dX,   
    myV3dY,   
    myV3dZ;
  
  QRubberBand*			myRubberBand;
  QPoint			myStartPoint;
  QPoint			myCurrentPoint;
  
  double			myPrecision;
  double			myViewPrecision;
  bool		myMapIsValid;
  Qt::KeyboardModifiers		myKeyboardFlags;
  Qt::MouseButton		myButtonFlags;
  QCursor			myCrossCursor;
  
  bool showGrid;

  CurrentInteractionMode cimode_;

  // data for measure points
  insight::cad::VectorPtr measpts_p1_, measpts_p2_;

  // for pointIDs
  insight::cad::FeatureSetPtr selpts_;

private: // methods
  
  void onLeftButtonDown  ( Qt::KeyboardModifiers nFlags, const QPoint point );
  void onMiddleButtonDown( Qt::KeyboardModifiers nFlags, const QPoint point );
  void onRightButtonDown ( Qt::KeyboardModifiers nFlags, const QPoint point );
  void onLeftButtonUp    ( Qt::KeyboardModifiers nFlags, const QPoint point );
  void onMiddleButtonUp  ( Qt::KeyboardModifiers nFlags, const QPoint point );
  void onRightButtonUp   ( Qt::KeyboardModifiers nFlags, const QPoint point );
  
  void onMouseMove  
    (
     Qt::MouseButtons buttons, 
     Qt::KeyboardModifiers nFlags, 
     const QPoint point,
     Qt::KeyboardModifiers curFlags
     );
  
  AIS_StatusOfPick dragEvent 
    (
     const QPoint startPoint, 
     const QPoint endPoint, 
     const bool multi = false 
     );

  AIS_StatusOfPick inputEvent( const bool multi = false );
  AIS_StatusOfDetection	moveEvent ( const QPoint point );
  
  void setMode( const CurrentAction3d mode );
    
  Standard_Real precision( Standard_Real aReal );
  Standard_Real viewPrecision( bool resized = false );
  
  void drawRubberBand( const QPoint origin, const QPoint position );
  void showRubberBand( void );
  void hideRubberBand( void );
  
  Standard_Boolean convertToPlane
    (
     const Standard_Integer Xs, 
     const Standard_Integer Ys, 
     Standard_Real& X,
     Standard_Real& Y,
     Standard_Real& Z
     );
  
#if (OCC_VERSION_MAJOR<7)
  void paintOCC();
  static int paintCallBack 
    (
     Aspect_Drawable, 
     void*, 
     Aspect_GraphicCallbackStruct*
     );
#endif
    
public:

  bool dump(Standard_CString theFile);

};

//----------------------------------------------------------------------------------------------------------

class OCCViewScreenshots : public QDialog
{
  Q_OBJECT
protected:
  QoccViewWidget* occWidget_;
  QString format_, initialPath_;

public:
  QStringList files;

  OCCViewScreenshots(Handle_AIS_InteractiveContext& context, QString initPath );

public slots:
  void screenShot(void);

};

#endif // QoccViewWidget_H
