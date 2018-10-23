/*! \file view.cxx
 *
 * \brief This file defines the viewer operations.
 *
 * \author John Reppy
 */

/* CMSC23700 Final Project sample code (Autumn 2017)
 *
 * COPYRIGHT (c) 2017 John Reppy (http://cs.uchicago.edu/~jhr)
 * All rights reserved.
 */

#include "cs237.hxx"
#include "view.hxx"
#include "map-cell.hxx"
#include "buffer-cache.hxx"
#include "texture-cache.hxx"
#include <map>

static void Error (int err, const char *msg);
static GLFWwindow *InitGLFW (int wid, int ht, const char *title);


/***** class View member functions *****/

View::View (class Map *map)
    : _map(map), _errorLimit(2.0), _isVis(true), _window(nullptr), _wireframe(true),
      _bCache(new BufferCache()), _tCache(new TextureCache())
{
}

void View::Init (int wid, int ht)
{
    this->_window = InitGLFW(wid, ht, this->_map->Name().c_str());

  // attach the view to the window so we can get it from callbacks
    glfwSetWindowUserPointer (this->_window, this);

  // Compute the bounding box for the entire map
    this->_mapBBox = cs237::AABBd(
        cs237::vec3d(0.0, double(this->_map->MinElevation()), 0.0),
        cs237::vec3d(
            double(this->_map->hScale()) * double(this->_map->Width()),
            double(this->_map->MaxElevation()),
            double(this->_map->hScale()) * double(this->_map->Height())));

  // Place the viewer in the center of cell(0,0), just above the
  // cell's bounding box.
    cs237::AABBd bb = this->_map->Cell(0,0)->Tile(0).BBox();
    cs237::vec3d pos = bb.center();
    pos.y = bb.maxY() + 0.01 * (bb.maxX() - bb.minX());

  // The camera's direction is toward the bulk of the terrain
    cs237::vec3d  at;
    if ((this->_map->nRows() == 1) && (this->_map->nCols() == 1)) {
        at = pos + cs237::vec3d(1.0, -0.25, 1.0);
    }
    else {
        at = pos + cs237::vec3d(double(this->_map->nCols()-1), 0.0, double(this->_map->nRows()-1));
    }
    this->_cam.move(pos, at, cs237::vec3d(0.0, 1.0, 0.0));

  // set the FOV and near/far planes
    this->_cam.setFOV (60.0);
    double diagonal = 1.02 * std::sqrt(
        double(this->_map->nRows() * this->_map->nRows())
        + double(this->_map->nCols() * this->_map->nCols()));
    this->_cam.setNearFar (
        10.0,
        diagonal * double(this->_map->CellWidth()) * double(this->_map->hScale()));
    this->Resize (wid, ht);


  // initialize fog & lighting to be on
    this->_noLight = false;

    if(this->_map->hasFog()){
      this->_noFog = false;
    }
    else
      this->_noFog = true;

    glClearColor(0.2f, 0.2f, 0.4f, 1.0f);

  // initialize detail texture
    if(/*this->_map->Name() == "Grand Canyon"*/ true){
      cs237::image2d detail = cs237::image2d::image2d("../data/detail.png");
      this->_tCache->_SetDetailTex(&detail);
    }
    else{
      cs237::image2d detail = cs237::image2d::image2d("../data/noise.png");
      this->_tCache->_SetDetailTex(&detail);
    }

  /** initialize shaders **/

  // Wireframe Mode
    this->wfshader = LoadShader("../shaders/wfshader");
    this->wfViewMatLoc = this->wfshader->UniformLocation("viewMat");
    this->wfProjMatLoc = this->wfshader->UniformLocation("projMat");
    this->wfOLoc = this->wfshader->UniformLocation("Opos");
    this->wfScalarLoc = this->wfshader->UniformLocation("scaling");
    this->wfColorLoc = this->wfshader->UniformLocation("color");

    this->wfshader->Use();
    cs237::setUniform(this->wfProjMatLoc, this->_cam.projTransform());
    cs237::setUniform(this->wfScalarLoc, cs237::vec4f(this->_map->hScale(),
                                                      this->_map->vScale(),
                                                      this->_map->hScale(),
                                                      this->_map->vScale()));
    CS237_CHECK( glPolygonMode(GL_FRONT_AND_BACK, GL_LINE) );
    CS237_CHECK( glDisable(GL_CULL_FACE) );
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_PRIMITIVE_RESTART);
    glPrimitiveRestartIndex(0xffff);


  // Texture Mode
    this->textureshader = LoadShader("../shaders/texture");
    this->tViewMatLoc = this->textureshader->UniformLocation("viewMat");
    this->tProjMatLoc = this->textureshader->UniformLocation("projMat");
    this->tOLoc = this->textureshader->UniformLocation("Opos");
    this->tScalarLoc = this->textureshader->UniformLocation("scaling");
    this->tTileWidthLoc = this->textureshader->UniformLocation("tileWidth");
    this->tMapLoc = this->textureshader->UniformLocation("tMap");  // will always set to 1
    this->normMapLoc = this->textureshader->UniformLocation("normMap"); //will always set to 1
    this->lDirLoc = this->textureshader->UniformLocation("direction");
    this->lAmbLoc = this->textureshader->UniformLocation("ambient");
    this->lIntLoc = this->textureshader->UniformLocation("intensity");
    this->noLightLoc = this->textureshader->UniformLocation("noLight");
    this->noFogLoc = this->textureshader->UniformLocation("noFog");
    this->fogDensityLoc = this->textureshader->UniformLocation("fogDensity");
    this->fogColorLoc = this->textureshader->UniformLocation("fogColor");
    this->tColLoc = this->textureshader->UniformLocation("tCol");
    this->tRowLoc = this->textureshader->UniformLocation("tRow");
    this->rainLoc = this->textureshader->UniformLocation("rain");
    this->detailMapLoc = this->textureshader->UniformLocation("detailMap");


  // Rain Shader
    this->rainshader = LoadShader("../shaders/rainshader");
    this->rViewMatLoc = this->rainshader->UniformLocation("viewMat");
    this->rProjMatLoc = this->rainshader->UniformLocation("projMat");

  // Skybox Shader
  this->skyboxshader = LoadShader("../shaders/skybox");
  this->skyViewMatLoc = this->skyboxshader->UniformLocation("viewMat");
  this->skyProjMatLoc = this->skyboxshader->UniformLocation("projMat");
  this->cubeMapLoc = this->skyboxshader->UniformLocation("cubeMap");

  // Initialize Cell textures
    for(int i = 0; i < this->_map->nRows(); i++){
        for(int j = 0; j < this->_map->nCols(); j++){
            this->_map->Cell(i, j)->InitTextures(this);
        }
    }

  // Initialize view frustum
    _frustum = new class Frustum();
    _frustum->updateFrustum(&this->_cam);

  // Initialize particle system
    this->_map->initParticles();

  // Initialize animation state
    this->_lastStep = glfwGetTime();

    //initializing sunny skybox textures
    glGenTextures(1, &this->sunnyTexture);
    glBindTexture(GL_TEXTURE_CUBE_MAP, this->sunnyTexture);

    cs237::image2d *NSunnyImage = new cs237::image2d("../../maps/Sunny/NSunny.png", false);
    cs237::image2d *WSunnyImage = new cs237::image2d("../../maps/Sunny/WSunny.png", false);
    cs237::image2d *ESunnyImage = new cs237::image2d("../../maps/Sunny/ESunny.png", false);
    cs237::image2d *SSunnyImage = new cs237::image2d("../../maps/Sunny/SSunny.png", false);
    cs237::image2d *USunnyImage = new cs237::image2d("../../maps/Sunny/USunny.png", false);
    cs237::image2d *DSunnyImage = new cs237::image2d("../../maps/Sunny/DSunny.png", false);

    glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, 0, GL_RGBA,
                 NSunnyImage->width(), NSunnyImage->height(), 0,
                 NSunnyImage->format(), NSunnyImage->type(),
                 NSunnyImage->data());

    glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, GL_RGBA,
                 WSunnyImage->width(), WSunnyImage->height(), 0,
                 WSunnyImage->format(), WSunnyImage->type(),
                 WSunnyImage->data());

    glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, 0, GL_RGBA,
                 ESunnyImage->width(), ESunnyImage->height(), 0,
                 ESunnyImage->format(), ESunnyImage->type(),
                 ESunnyImage->data());

    glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, 0, GL_RGBA,
                 SSunnyImage->width(), SSunnyImage->height(), 0,
                 SSunnyImage->format(), SSunnyImage->type(),
                 SSunnyImage->data());

    glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, 0, GL_RGBA,
                 USunnyImage->width(), USunnyImage->height(), 0,
                 USunnyImage->format(), USunnyImage->type(),
                 USunnyImage->data());

    glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, 0, GL_RGBA,
                 DSunnyImage->width(), DSunnyImage->height(), 0,
                 DSunnyImage->format(), DSunnyImage->type(),
                 DSunnyImage->data());

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    //initialize cloudy skybox textures
    glGenTextures(1, &this->cloudyTexture);
    glBindTexture(GL_TEXTURE_CUBE_MAP, this->cloudyTexture);

    cs237::image2d *NCloudyImage = new cs237::image2d("../../maps/cloudy/NCloudy.png", false);
    cs237::image2d *WCloudyImage = new cs237::image2d("../../maps/cloudy/WCloudy.png", false);
    cs237::image2d *ECloudyImage = new cs237::image2d("../../maps/cloudy/ECloudy.png", false);
    cs237::image2d *SCloudyImage = new cs237::image2d("../../maps/cloudy/SCloudy.png", false);
    cs237::image2d *UCloudyImage = new cs237::image2d("../../maps/cloudy/UCloudy.png", false);
    cs237::image2d *DCloudyImage = new cs237::image2d("../../maps/cloudy/DCloudy.png", false);

    glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, 0, GL_RGBA,
                 NCloudyImage->width(), NCloudyImage->height(), 0,
                 NCloudyImage->format(), NCloudyImage->type(),
                 NCloudyImage->data());

    glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, GL_RGBA,
                 WCloudyImage->width(), WCloudyImage->height(), 0,
                 WCloudyImage->format(), WCloudyImage->type(),
                 WCloudyImage->data());

    glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, 0, GL_RGBA,
                 ECloudyImage->width(), ECloudyImage->height(), 0,
                 ECloudyImage->format(), ECloudyImage->type(),
                 ECloudyImage->data());

    glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, 0, GL_RGBA,
                 SCloudyImage->width(), SCloudyImage->height(), 0,
                 SCloudyImage->format(), SCloudyImage->type(),
                 SCloudyImage->data());

    glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, 0, GL_RGBA,
                 UCloudyImage->width(), UCloudyImage->height(), 0,
                 UCloudyImage->format(), UCloudyImage->type(),
                 UCloudyImage->data());

    glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, 0, GL_RGBA,
                 DCloudyImage->width(), DCloudyImage->height(), 0,
                 DCloudyImage->format(), DCloudyImage->type(),
                 DCloudyImage->data());

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
}

#define SQRT_2                  1.41421356237f
#define ONE_SQRT_2              0.707106781188f

void View::HandleKey (int key, int mods)
{
    switch (key) {
      case GLFW_KEY_ESCAPE:
      case GLFW_KEY_Q:
        if (mods == 0)
            glfwSetWindowShouldClose (this->_window, true);
        break;
      case GLFW_KEY_W: // toggle wireframe mode
        this->_wireframe = !this->_wireframe;
        if(this->_wireframe){
          glClearColor(0.2f, 0.2f, 0.4f, 1.0f);

          wfshader->Use();
          cs237::setUniform(this->wfProjMatLoc, this->_cam.projTransform());
          CS237_CHECK( glPolygonMode(GL_FRONT_AND_BACK, GL_LINE) );
          CS237_CHECK( glDisable(GL_CULL_FACE) );
          glEnable(GL_DEPTH_TEST);
          glEnable(GL_PRIMITIVE_RESTART);
          glPrimitiveRestartIndex(0xffff);
        }
        else{ // texture renderer
          if(this->_map->hasFog() && !this->_noFog){
            cs237::color3f fog = this->_map->FogColor();
            glClearColor(fog[0], fog[1], fog[2], 1.0f);
          }

          textureshader->Use();
          cs237::setUniform(this->tProjMatLoc, this->_cam.projTransform());
          cs237::setUniform(this->lDirLoc, this->_map->SunDirection());
          cs237::setUniform(this->lAmbLoc, this->_map->AmbientIntensity());
          cs237::setUniform(this->lIntLoc, this->_map->SunIntensity());
          cs237::setUniform(this->tMapLoc, 0);
          cs237::setUniform(this->normMapLoc, 1);
          cs237::setUniform(this->detailMapLoc, 2);
          cs237::setUniform(this->noLightLoc, GL_FALSE);
          cs237::setUniform(this->fogColorLoc, this->_map->FogColor());
          cs237::setUniform(this->fogDensityLoc, this->_map->FogDensity());

          CS237_CHECK( glPolygonMode(GL_FRONT_AND_BACK, GL_FILL) );
          CS237_CHECK( glEnable(GL_CULL_FACE) );
          glCullFace(GL_BACK);
          glEnable(GL_DEPTH_TEST);
          glEnable(GL_PRIMITIVE_RESTART);
          glPrimitiveRestartIndex(0xffff);
        }
        break;
      case GLFW_KEY_L: // toggle lighting
        if(!this->_wireframe){
          this->_noLight = !this->_noLight;
          if(this->_noLight){
            cs237::setUniform(this->noLightLoc, GL_TRUE);
          }
          else{
            cs237::setUniform(this->noLightLoc, GL_FALSE);
          }
        }
        break;
      case GLFW_KEY_F: // toggle fog
      if(!this->_wireframe && this->_map->hasFog()){
        this->_noFog = !this->_noFog;
        if(this->_noFog){
          glClearColor(0.2f, 0.2f, 0.4f, 1.0f);
          cs237::setUniform(this->noFogLoc, GL_TRUE);
        }
        else{
          cs237::color3f fog = this->_map->FogColor();
          glClearColor(fog[0], fog[1], fog[2], 1.0f);
          cs237::setUniform(this->noFogLoc, GL_FALSE);
        }
      }
      break;
      case GLFW_KEY_R: // toggle rain
        this->_rainMode = !this->_rainMode;
      break;
      case GLFW_KEY_EQUAL:
        if (mods == GLFW_MOD_SHIFT) { // shift+'=' is '+'
          // decrease error tolerance
            if (this->_errorLimit > 0.5)
                this->_errorLimit *= ONE_SQRT_2;
        }
        break;
      case GLFW_KEY_KP_ADD:  // keypad '+'
        if (mods == 0) {
          // decrease error tolerance
            if (this->_errorLimit > 0.5)
                this->_errorLimit *= ONE_SQRT_2;
        }
        break;
      case GLFW_KEY_MINUS:
        if (mods == 0) {
          // increase error tolerance
            this->_errorLimit *= SQRT_2;
        }
        break;
      case GLFW_KEY_KP_SUBTRACT:  // keypad '-'
        if (mods == 0) {
          // increase error tolerance
            this->_errorLimit *= SQRT_2;
        }
        break;
      default:
        if(mods == 0)
          return; //return if calling camera control from glfwCallback, should
                  //only be called with glfwGetKey
    }

    switch (key) {
      case GLFW_KEY_UP: // rotate around the camera's right-axis
        this->_cam.pitch(1.0f);
        _frustum->updateFrustum(&this->_cam);
        break;
      case GLFW_KEY_DOWN: // rotate around the camera's right-axis
        this->_cam.pitch(-1.0f);
        _frustum->updateFrustum(&this->_cam);
        break;
      case GLFW_KEY_LEFT: // rotate around the camera's up-axis
        this->_cam.look(cs237::vec3f(cs237::rotateY(-1.0f) *
                                     cs237::vec4f(this->_cam.direction(), 0.0f)),
                        cs237::vec3f(cs237::rotateY(-1.0f) *
                                     cs237::vec4f(this->_cam.up(), 0.0f)));
        _frustum->updateFrustum(&this->_cam);
        break;
      case GLFW_KEY_RIGHT: // rotate around the camera's up-axis
      this->_cam.look(cs237::vec3f(cs237::rotateY(1.0f) *
                                   cs237::vec4f(this->_cam.direction(), 0.0f)),
                      cs237::vec3f(cs237::rotateY(1.0f) *
                                   cs237::vec4f(this->_cam.up(), 0.0f)));
        _frustum->updateFrustum(&this->_cam);
        break;
      case GLFW_KEY_B: // rotate around the camera's direction-axis
        this->_cam.roll(1.0f);
        _frustum->updateFrustum(&this->_cam);
        break;
      case GLFW_KEY_N: // rotate around the camera's direction-axis
        this->_cam.roll(-1.0f);
        _frustum->updateFrustum(&this->_cam);
        break;
      case GLFW_KEY_Y: // translate along the camera's direction-axis
        this->_cam.longitudinal(5.0f * ((this->_map->hScale() + this->_map->vScale()) / 2));
        _frustum->updateFrustum(&this->_cam);
        break;
      case GLFW_KEY_H: // translate along the camera's direction-axis
        this->_cam.longitudinal(-5.0f * ((this->_map->hScale() + this->_map->vScale()) / 2));
        _frustum->updateFrustum(&this->_cam);
        break;
      case GLFW_KEY_G: // translate along the camera's right-axis
        this->_cam.lateral(-5.0f * ((this->_map->hScale() + this->_map->vScale()) / 2));
        _frustum->updateFrustum(&this->_cam);
        break;
      case GLFW_KEY_J: // translate along the camera's right-axis
        this->_cam.lateral(5.0f * ((this->_map->hScale() + this->_map->vScale()) / 2));
        _frustum->updateFrustum(&this->_cam);
        break;
      default: // ignore all other keys
        return;
    }

}

void View::HandleMouseEnter (bool entered)
{
}

void View::HandleMouseMove (double x, double y)
{
}

void View::HandleMouseButton (int button, int action, int mods)
{
}

void View::Animate (double now)
{
    double dt = now - this->_lastStep;
    if (dt >= TIME_STEP) {
        this->_lastStep = now;

        /* ANIMATION CODE */

    }

}

void View::Resize (int wid, int ht)
{
    glfwGetFramebufferSize (this->_window, &this->_fbWid, &this->_fbHt);
    glViewport(0, 0 , this->_fbWid, this->_fbHt);

  // default error limit is 1%
    this->_errorLimit = float(this->_fbHt) / 100.0f;

    this->_cam.setViewport (this->_fbWid, this->_fbHt);

}

/**** Utility functions *****/

// Load, compile, and link a shader program.
//
cs237::ShaderProgram *LoadShader (
    std::string const &vertShader,
    std::string const &fragShader,
    std::string const &geomShader)
{
  /// cache of previously loaded shaders
    static std::map<std::string, cs237::VertexShader *> VShaders;
    static std::map<std::string, cs237::FragmentShader *> FShaders;
    static std::map<std::string, cs237::GeometryShader *> GShaders;

    cs237::VertexShader *vsh;
    cs237::FragmentShader *fsh;
    cs237::GeometryShader *gsh;
    cs237::ShaderProgram *shader;

    auto vit = VShaders.find(vertShader);
    if (vit == VShaders.end()) {
        vsh = new cs237::VertexShader (vertShader.c_str());
        VShaders.insert (
            std::pair<std::string, cs237::VertexShader *>(vertShader, vsh));
    }
    else {
        vsh = vit->second;
    }

    auto fit = FShaders.find(fragShader);
    if (fit == FShaders.end()) {
        fsh = new cs237::FragmentShader (fragShader.c_str());
        FShaders.insert (
            std::pair<std::string, cs237::FragmentShader *>(fragShader, fsh));
    }
    else {
        fsh = fit->second;
    }

    if (geomShader.empty()) {
      // no geometry shader
        shader = new cs237::ShaderProgram (*vsh, *fsh);
    }
    else {
        auto git = GShaders.find(geomShader);
        if (git == GShaders.end()) {
            gsh = new cs237::GeometryShader (geomShader.c_str());
            GShaders.insert (
                std::pair<std::string, cs237::GeometryShader *>(geomShader, gsh));
        }
        else {
            gsh = git->second;
        }
        shader = new cs237::ShaderProgram (*vsh, *gsh, *fsh);
    }

    if (shader == nullptr) {
        std::cerr << "Cannot build shader from \"" << vertShader << "\"/\""
            << fragShader << "\"";
        if (! geomShader.empty()) {
            std::cerr << "/\"" << geomShader << "\"";
        }
        std::cerr << "\n";
        exit (1);
    }

    return shader;

}

// Load a shader from the file system and compile and link it.  We keep
// a cache of shaders that have already been loaded to allow two renderers
// to share the same shader program without compiling it twice.
//
cs237::ShaderProgram *LoadShader (std::string const & shaderPrefix)
{
    static std::map<std::string, cs237::ShaderProgram *> Shaders;

    auto it = Shaders.find(shaderPrefix);
    if (it == Shaders.end()) {
      // load, compile, and link the shader program
        auto shader = LoadShader(shaderPrefix + ".vsh", shaderPrefix + ".fsh", "");
        Shaders.insert (std::pair<std::string, cs237::ShaderProgram *>(shaderPrefix, shader));
        return shader;
    }
    else {
        return it->second;
    }

}

/***** Local utility functions *****/

static void Error (int err, const char *msg)
{
    std::cerr << "[GLFW ERROR " << err << "] " << msg << "\n" << std::endl;
}

static GLFWwindow *InitGLFW (int wid, int ht, const char *title)
{
    glfwSetErrorCallback (Error);

    glfwInit ();

  // Check the GLFW version
    {
        int major;
        glfwGetVersion (&major, NULL, NULL);
        if (major < 3) {
            std::cerr << "CS237 requires GLFW 3.0 or later\n" << std::endl;
            exit (EXIT_FAILURE);
        }
    }


    glfwWindowHint (GLFW_RESIZABLE, GL_TRUE);
    glfwWindowHint (GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint (GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint (GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint (GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    GLFWwindow *window = glfwCreateWindow(wid, ht, title, NULL, NULL);
    if (window == nullptr) {
        exit (EXIT_FAILURE);
    }

    glfwMakeContextCurrent (window);

  // Check the OpenGL version
    {
        GLint major, minor;
        glGetIntegerv (GL_MAJOR_VERSION, &major);
        glGetIntegerv (GL_MINOR_VERSION, &minor);
        if ((major < 4) || ((major == 4) && (minor < 1))) {
            std::cerr << "CS237 requires OpenGL 4.1 or later; found " << major << "." << minor << std::endl;
            exit (EXIT_FAILURE);
        }
    }

    return window;
}
