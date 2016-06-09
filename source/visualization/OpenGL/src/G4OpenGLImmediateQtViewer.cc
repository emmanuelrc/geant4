//
// ********************************************************************
// * License and Disclaimer                                           *
// *                                                                  *
// * The  Geant4 software  is  copyright of the Copyright Holders  of *
// * the Geant4 Collaboration.  It is provided  under  the terms  and *
// * conditions of the Geant4 Software License,  included in the file *
// * LICENSE and available at  http://cern.ch/geant4/license .  These *
// * include a list of copyright holders.                             *
// *                                                                  *
// * Neither the authors of this software system, nor their employing *
// * institutes,nor the agencies providing financial support for this *
// * work  make  any representation or  warranty, express or implied, *
// * regarding  this  software system or assume any liability for its *
// * use.  Please see the license in the file  LICENSE  and URL above *
// * for the full disclaimer and the limitation of liability.         *
// *                                                                  *
// * This  code  implementation is the result of  the  scientific and *
// * technical work of the GEANT4 collaboration.                      *
// * By using,  copying,  modifying or  distributing the software (or *
// * any work based  on the software)  you  agree  to acknowledge its *
// * use  in  resulting  scientific  publications,  and indicate your *
// * acceptance of all terms of the Geant4 Software license.          *
// ********************************************************************
//
//
// $Id: G4OpenGLImmediateQtViewer.cc,v 1.17 2009/11/03 11:02:32 lgarnier Exp $
// GEANT4 tag $Name: geant4-09-03 $
//
//
// Class G4OpenGLImmediateQtViewer : a class derived from G4OpenGLQtViewer and
//                                G4OpenGLImmediateViewer.

#ifdef G4VIS_BUILD_OPENGLQT_DRIVER

#include "G4OpenGLImmediateQtViewer.hh"
#include "G4OpenGLImmediateSceneHandler.hh"

#include "G4ios.hh"

G4OpenGLImmediateQtViewer::G4OpenGLImmediateQtViewer
(G4OpenGLImmediateSceneHandler& sceneHandler,
 const G4String&  name):
  G4VViewer (sceneHandler, sceneHandler.IncrementViewCount (), name),
  G4OpenGLViewer (sceneHandler),
  G4OpenGLQtViewer (sceneHandler),
  G4OpenGLImmediateViewer (sceneHandler)
{

#if QT_VERSION < 0x040000
  setFocusPolicy(QWidget::StrongFocus); // enable keybord events
#else
  setFocusPolicy(Qt::StrongFocus); // enable keybord events
#endif
  fHasToRepaint =false;

  if (fViewId < 0) return;  // In case error in base class instantiation.
}

G4OpenGLImmediateQtViewer::~G4OpenGLImmediateQtViewer() {
  makeCurrent();
}

void G4OpenGLImmediateQtViewer::Initialise() {
#ifdef G4DEBUG_VIS_OGL
  printf("G4OpenGLImmediateQtViewer::Initialise \n");
#endif
  fReadyToPaint = false;
  CreateMainWindow (this,QString(fName));
  CreateFontLists ();

  fReadyToPaint = true;
}

void G4OpenGLImmediateQtViewer::initializeGL () {

  InitializeGLView ();


  // If a double buffer context has been forced upon us, ignore the
  // back buffer for this OpenGLImmediate view.
  //   glDrawBuffer (GL_FRONT); // FIXME : Ne marche pas avec cette ligne, mais affiche le run correctement...
  // clear the buffers and window.
  ClearView ();
  FinishView ();

  glDepthFunc (GL_LEQUAL);
  glDepthMask (GL_TRUE);

  if (fSceneHandler.GetScene() == 0) {
    fHasToRepaint =false;
  } else {
    fHasToRepaint =true;
  }

}


/** To ensure compatibility with DrawView method
 */
void  G4OpenGLImmediateQtViewer::DrawView() {
#ifdef G4DEBUG_VIS_OGL
  printf("G4OpenGLImmediateQtViewer::DrawView  VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV\n");
#endif
  // That's no the same logic as Stored Viewer, I don't know why...
  // see G4OpenGLStoredQtViewer::DrawView for more informations

  updateQWidget();
#ifdef G4DEBUG_VIS_OGL
  printf("G4OpenGLImmediateQtViewer::DrawView  ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^\n");
#endif
}


void G4OpenGLImmediateQtViewer::ComputeView () {

#ifdef G4DEBUG_VIS_OGL
  printf("G4OpenGLImmediateQtViewer::ComputeView %d %d   VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV\n",getWinWidth(), getWinHeight());
#endif
  makeCurrent();
  // If a double buffer context has been forced upon us, ignore the
  // back buffer for this OpenGLImmediate view.
  //  glDrawBuffer (GL_FRONT);

  G4ViewParameters::DrawingStyle style = GetViewParameters().GetDrawingStyle();

  if(style!=G4ViewParameters::hlr &&
     haloing_enabled) {

    HaloingFirstPass ();
    NeedKernelVisit ();
    ProcessView ();
    glFlush ();
#ifdef G4DEBUG_VIS_OGL
  printf("G4OpenGLImmediateQtViewer::ComputeView First ProcessView ok\n");
#endif
    HaloingSecondPass ();

  }

  NeedKernelVisit ();  // Always need to visit G4 kernel.
  ProcessView ();

  if (isRecording()) {
    savePPMToTemp();
  }
   
#ifdef G4DEBUG_VIS_OGL
  printf("G4OpenGLImmediateQtViewer::ComputeView %d %d ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^ \n",getWinWidth(), getWinHeight());
#endif
  fHasToRepaint = true;
}

void G4OpenGLImmediateQtViewer::FinishView()
{
#ifdef G4DEBUG_VIS_OGL
  printf("G4OpenGLImmediateQtViewer::FinishView() BEGIN\n");
#endif
  glFlush ();
#ifdef G4DEBUG_VIS_OGL
  printf("G4OpenGLImmediateQtViewer::FinishView() END\n");
#endif

}


/**
   - Lors du resize de la fenetre, on doit non pas redessiner le detecteur, mais aussi les evenements
*/
void G4OpenGLImmediateQtViewer::resizeGL(
 int aWidth
,int aHeight)
{  
  ResizeWindow(aWidth,aHeight);
  fHasToRepaint = sizeHasChanged();
}


void G4OpenGLImmediateQtViewer::paintGL()
{
#ifdef G4DEBUG_VIS_OGL
  printf("\n\nG4OpenGLImmediateQtViewer::paintGL ??\n");
#endif
  if (!fReadyToPaint) {
    fReadyToPaint= true;
    return;
  }
  // DO NOT RESIZE IF SIZE HAS NOT CHANGE
  if ( !fHasToRepaint) {
#if QT_VERSION < 0x040000
    if (((getWinWidth() == (unsigned int)width())) &&(getWinHeight() == (unsigned int) height())) { 
#else
    // L. Garnier : Trap to get the size with mac OSX 10.6 and Qt 4.6(devel)
    // Tested on Qt4.5 on mac, 4.4 on windows, 4.5 on unbuntu
    int sw = 0;
    int sh = 0;
    if (!isMaximized() && !isFullScreen()) {
      sw = normalGeometry().width();
      sh = normalGeometry().height();
    } else {
      sw = frameGeometry().width();
      sh = frameGeometry().height();
    }
    if ((getWinWidth() == (unsigned int)sw) &&(getWinHeight() == (unsigned int)sh)) {
#endif
      return;
    }
  }
#ifdef G4DEBUG_VIS_OGL
  printf("G4OpenGLImmediateQtViewer::paintGL VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV ready %d\n",fReadyToPaint);
#endif

  SetView();
   
  ClearView (); //ok, put the background correct
  ComputeView();

  fHasToRepaint = false; // could be set to false by ComputeView

#ifdef G4DEBUG_VIS_OGL
  printf("G4OpenGLImmediateQtViewer::paintGL ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^ ready %d\n\n\n",fReadyToPaint);
#endif
}

void G4OpenGLImmediateQtViewer::mousePressEvent(QMouseEvent *event)
{
  G4MousePressEvent(event);
}

void G4OpenGLImmediateQtViewer::keyPressEvent (QKeyEvent * event) 
{
  G4keyPressEvent(event);
}

void G4OpenGLImmediateQtViewer::wheelEvent (QWheelEvent * event) 
{
  G4wheelEvent(event);
}

/**
 * This function was build in order to make a zoom on double clic event.
 * It was think to build a rubberband on the zoom area, but never work fine
 */
void G4OpenGLImmediateQtViewer::mouseDoubleClickEvent(QMouseEvent *)
{
  G4MouseDoubleClickEvent();
}

void G4OpenGLImmediateQtViewer::mouseReleaseEvent(QMouseEvent *)
{
  G4MouseReleaseEvent();
}

void G4OpenGLImmediateQtViewer::mouseMoveEvent(QMouseEvent *event)
{
  G4MouseMoveEvent(event);
}


void G4OpenGLImmediateQtViewer::contextMenuEvent(QContextMenuEvent *e)
{
  G4manageContextMenuEvent(e);
}

void G4OpenGLImmediateQtViewer::updateQWidget() {
  fHasToRepaint= true;
  updateGL();
  fHasToRepaint= false;
}


void G4OpenGLImmediateQtViewer::ShowView (
) 
//////////////////////////////////////////////////////////////////////////////
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!//
{
#if QT_VERSION < 0x040000
  setActiveWindow();
#else
  activateWindow();
#endif
}
#endif