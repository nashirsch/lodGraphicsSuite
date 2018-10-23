/*! \file view.hxx
 *
 * \author John Reppy
 */

/* CMSC23700 Final Project sample code (Autumn 2017)
 *
 * COPYRIGHT (c) 2017 John Reppy (http://cs.uchicago.edu/~jhr)
 * All rights reserved.
 */

#ifndef _VIEW_HXX_
#define _VIEW_HXX_

#include "cs237.hxx"
#include "map.hxx"
#include "camera.hxx"
#include <vector>

// animation time step (100Hz)
#define TIME_STEP       0.001
#define MORPH_TIME      2.5f


class View {
  public:

  //! construct a viewer for the map
    View (Map *map);

  //! return the view's window
    GLFWwindow *Window() const { return this->_window; }

  //! initialize the view (i.e., allocate its window)
    void Init (int wid, int ht);

  //! method to handle display of the view
  //! \param dt time since last frame
    void Render (float dt);

  //! animation method
  //! \param now time of current frame
    void Animate (double now);

  //! handle kwyboard input
    void HandleKey (int key, int mods);

  //! handle mouse entering/leaving the view's window
    void HandleMouseEnter (bool entered);

  //! handle mouse movement
    void HandleMouseMove (double x, double y);

  //! handle mouse buttons
  //! \param button the mouse button that was pressed
  //! \param action was the button pressed or released?
  //! \param mods   the state of the modifier keys
    void HandleMouseButton (int button, int action, int mods);

  //! handle resizing the view
    void Resize (int wid, int ht);

  //! handle visibility change
    void SetVisible (bool isVis) { this->_isVis = isVis; }

  //! should this view close?
    bool shouldClose () const { return glfwWindowShouldClose(this->_window); }

  //! is the view in wireframe mode?
    bool wireframeMode () const { return this->_wireframe; }

  //! the view's current camera state
    class Camera const &Camera () const { return this->_cam; }

  //! the view's current error limit
    float ErrorLimit () const { return this->_errorLimit; }

  //! the cache of VAO objects for representing chunks
    class BufferCache *VAOCache () const { return this->_bCache; }

  //! the cache of OpenGL textures for the map tiles
    class TextureCache *TxtCache () const { return this->_tCache; }

  //! does the map have an normal map?
    bool  hasNorMap() { return this->_map->hasNormalMap(); }

  //! does the map have a color map?
    bool  hasColMap() { return this->_map->hasColorMap(); }

  //! renders the skybox
    void drawSky();

  //! returns a pointer to the map
    class Map* Map() { return this->_map; }

  //! returns a pointer to the view frustum
    Frustum* Frustum() { return this->_frustum; }

  //! wireframe shader and uniform locations
    cs237::ShaderProgram *wfshader;
    int wfViewMatLoc; // view matrix
    int wfProjMatLoc; // projection matrix
    int wfOLoc; // camera-relative origin of the cell
    int wfScalarLoc; // scalars to bring coordinates into camera-relative world space
    int wfColorLoc; // wireframe color (LOD specific)
    int wfskyboxLoc; // boolean signifying that the skybox is being drawn

  //! texture shader and uniform locations
    cs237::ShaderProgram *textureshader;
    int tViewMatLoc; // view matrix
    int tProjMatLoc; // projection matrix
    int tOLoc; // camera-relative origin of the cell
    int tScalarLoc; // scalars to bring coordinates into camera-relative world space
    int tTileWidthLoc; // width of the given tile
    int tMapLoc; // texture/color map
    int normMapLoc; // normal map
    int detailMapLoc; // detail map
    int lDirLoc; // light direction
    int lAmbLoc; // ambient light intensity
    int lIntLoc; // directional light intensity
    int noLightLoc; // boolean signifying if the scene is being rendered with light
    int noFogLoc; // boolean signifying if the scene is being rendered with fog
    int fogDensityLoc; // fog density
    int fogColorLoc; // fog color
    int tColLoc; // column number
    int tRowLoc; // row number
    int tskyboxLoc; // boolean signifying that the skybox is being drawn
    int tskyboxTopLoc; // top of the skybox, used to blend with fog
    int rainLoc; // boolean signifying if rain is being drawn

  //! rain shader and uniform locations
    cs237::ShaderProgram *rainshader;
    int rViewMatLoc; // view matrix
    int rProjMatLoc; // projection matrix

  //! skybox shader and unifomr locations
    cs237::ShaderProgram *skyboxshader;
    int skyViewMatLoc; // view matrix
    int skyProjMatLoc; // projection matrix
    int cubeMapLoc; // cube map texture sampler
    GLuint sunnyTexture; // sunny skybox texture
    GLuint cloudyTexture; // cloudy skybox texture (for rain mode)


  private:
    class Map   *_map;          //!< the map being rendered
    class Camera _cam;          //!< tracks viewer position, etc.
    float       _errorLimit;    //!< screen-space error limit
    bool        _isVis;         //!< true when this window is visible
    GLFWwindow  *_window;       //!< the main window
    int         _fbWid;         //!< current framebuffer width
    int         _fbHt;          //!< current framebuffer height
    bool        _wireframe;     //!< true if we are rendering the wireframe
    bool        _rainMode;      //!< true if rain is turned on
    bool        _noLight;       //!< true if lighting is turned off
    bool        _noFog;         //!< true if fog is turned off
    double      _lastStep;      //!< time of last animation step
    cs237::AABBd _mapBBox;      //!< a bounding box around the entire map

  // resource management
    class BufferCache   *_bCache;       //!< cache of OpenGL VAO objects used for chunks
    class TextureCache  *_tCache;       //!< cache of OpenGL textures

  // view frustum
    class Frustum       *_frustum;      //!< contains 6 normals and signed distances
};

//! \brief Load, compile, and link a shader program.
//! \param vertShader the path to the vertex shader
//! \param fragShader the path to the fragment shader
//! \param geomShader the path to the geometry shader ("" for no geometry pass)
//! \return the shader program
//
cs237::ShaderProgram *LoadShader (
    std::string const &vertShader,
    std::string const &fragShader,
    std::string const &geomShader);

//! Load a simple (vertex+fragment) shader from the file system, compile it,
//! and link it.
//! \param shaderPrefix the prefix path to the vertex shader.
//! \return the shader program
//
cs237::ShaderProgram *LoadShader (std::string const &shaderPrefix);

#endif // !_VIEW_HXX_
