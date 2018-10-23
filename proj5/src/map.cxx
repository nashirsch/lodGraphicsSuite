/*! \file map.cxx
 *
 * \author John Reppy
 *
 * Information about heightfield maps.
 */

/* CMSC23700 Final Project sample code (Autumn 2017)
 *
 * COPYRIGHT (c) 2017 John Reppy (http://cs.uchicago.edu/~jhr)
 * All rights reserved.
 */

#include "json.hxx"
#include "cs237.hxx"
#include "map.hxx"
#include "map-cell.hxx"
#include "map-objects.hxx"
#include <unistd.h>

/***** class Map member functions *****/

Map::Map () : _grid(nullptr) { }

Map::~Map ()
{
    if (this->_grid != nullptr) {
        for (int i = 0;  i < this->_nCells();  i++) {
            if (this->_grid[i] != nullptr)
                delete this->_grid[i];
        }
        delete this->_grid;
    }

}

static void error (std::string file, std::string msg)
{
    std::cerr << "error reading map file \"" << file << "\": " << msg << "\n";
}

// helper function to get an optional array of three numbers from the map object; returns
// true if there is an error (but false if the array is null)
//
static bool GetThreeFloats (const JSON::Array *arr, float out[3])
{
    if (arr == nullptr) {
      // array is not present
        return true;
    }

    if (arr->length() != 3) {
        return true;
    }

    for (int i = 0;  i < 3;  i++) {
        const JSON::Number *num = (*arr)[i]->asNumber();
        if (num == nullptr) {
            return false;
        }
        out[i] = static_cast<float>(num->realVal());
    }

    return false;
}

bool Map::LoadMap (std::string const &mapName, bool verbose)
{
    if (this->_grid != nullptr) {
      // map file has already been loaded, so return false
        return false;
    }

    this->_path = mapName + "/";

    JSON::Value *map = JSON::ParseFile(this->_path + "map.json");
    const JSON::Object *root = (map != nullptr) ? map->asObject() : nullptr;

    if (root == nullptr) {
        std::cerr << "error reading map file\n";
        return false;
    }

  // get map name
    const JSON::String *name = root->fieldAsString("name");
    if (name == nullptr) {
        error (mapName, "missing/bogus name field");
        return false;
    }
    this->_name = name->value();

    const JSON::Number *num;

  // get hScale
    num = root->fieldAsNumber("h-scale");
    if (num == nullptr) {
        error (mapName, "missing/bogus h-scale field");
        return false;
    }
    this->_hScale = static_cast<float>(num->realVal());

  // get vScale
    num = root->fieldAsNumber("v-scale");
    if (num == nullptr) {
        error (mapName, "missing/bogus v-scale field");
        return false;
    }
    this->_vScale = static_cast<float>(num->realVal());

  // get base elevation (optional)
    JSON::Value *v = (*root)["base-elev"];
    if (v != nullptr) {
        num = v->asNumber();
        if (num == nullptr) {
            error (mapName, "bogus base-elev field");
            return false;
        }
        this->_baseElev = static_cast<float>(num->realVal());
    }
    else
        this->_baseElev = 0.0;

  // get minimum elevation
    num = root->fieldAsNumber("min-elev");
    if (num == nullptr) {
        error (mapName, "missing/bogus min-elev field");
        return false;
    }
    this->_minElev = static_cast<float>(num->realVal());

  // get maximum elevation
    num = root->fieldAsNumber("max-elev");
    if (num == nullptr) {
        error (mapName, "missing/bogus max-elev field");
        return false;
    }
    this->_maxElev = static_cast<float>(num->realVal());

  // get bottom of skybox
    num = root->fieldAsNumber("min-sky");
    if (num == nullptr) {
        error (mapName, "missing/bogus min-sky field");
        return false;
    }
    this->_minSky = static_cast<float>(num->realVal());

  // get top of skybox
    num = root->fieldAsNumber("max-sky");
    if (num == nullptr) {
        error (mapName, "missing/bogus max-sky field");
        return false;
    }
    this->_maxSky = static_cast<float>(num->realVal());

  // get map width
    const JSON::Integer *intnum = root->fieldAsInteger("width");
    if ((intnum == nullptr) || (intnum->intVal() < 1)) {
        error (mapName, "missing/bogus width field");
        return false;
    }
    this->_width = static_cast<uint32_t>(intnum->intVal());

  // get map height
    intnum = root->fieldAsInteger("height");
    if ((intnum == nullptr) || (intnum->intVal() < 1)) {
        error (mapName, "missing/bogus height field");
        return false;
    }
    this->_height = static_cast<uint32_t>(intnum->intVal());

  // get cell size
    intnum = root->fieldAsInteger("cell-size");
    if ((intnum == nullptr) || (intnum->intVal() < MIN_CELL_SIZE)
    || (MAX_CELL_SIZE < intnum->intVal()))
    {
        error (mapName, "missing/bogus cell-size field");
        return false;
    }
    this->_cellSize = static_cast<uint32_t>(intnum->intVal());

  // check properties
    v = (*root)["color-map"];
    if (v != nullptr) {
        const JSON::Bool *b = v->asBool();
        if (b == nullptr) {
            error (mapName, "bogus color-map field");
            return false;
        }
        this->_hasColor = b->value();
    }
    else
        this->_hasColor = false;

    v = (*root)["normal-map"];
    if (v != nullptr) {
        const JSON::Bool *b = v->asBool();
        if (b == nullptr) {
            error (mapName, "bogus normal-map field");
            return false;
        }
        this->_hasNormals = b->value();
    }
    else
        this->_hasNormals = false;

    v = (*root)["water-map"];
    if (v != nullptr) {
        const JSON::Bool *b = v->asBool();
        if (b == nullptr) {
            error (mapName, "bogus water-map field");
            return false;
        }
        this->_hasWater = b->value();
    }
    else
        this->_hasWater = false;

  // lighting info (optional)
    {
        float sunDir[3] = { 0.0, 1.0, 0.0 };  // default is high noon
        float sunI[3] = { 0.9, 0.9, 0.9 };  // bright sun
        float ambI[3] = { 0.1, 0.1, 0.1 };

        if (GetThreeFloats (root->fieldAsArray("sun-dir"), sunDir)) {
            error (mapName, "bogus sun-dir field");
            return false;
        }

        if (GetThreeFloats (root->fieldAsArray("sun-intensity"), sunI)) {
            error (mapName, "bogus sun-intensity field");
            return false;
        }

        if (GetThreeFloats (root->fieldAsArray("ambient"), ambI)) {
            error (mapName, "bogus ambient field");
            return false;
        }

        this->_sunDir = normalize (cs237::vec3f(sunDir[0], sunDir[1], sunDir[2]));
        this->_sunI = cs237::color3f(sunI[0], sunI[1], sunI[2]);
        this->_ambI = cs237::color3f(ambI[0], ambI[1], ambI[2]);
    }

  // fog (optional)
    if ((*root)["fog-color"] != nullptr) {
        float fogColor[3];
        this->_hasFog = true;
        if (GetThreeFloats (root->fieldAsArray("fog-color"), fogColor)) {
            error (mapName, "bogus fog-color field");
            return false;
        }
        this->_fogColor = cs237::color3f(fogColor[0], fogColor[1], fogColor[2]);

        num = root->fieldAsNumber("fog-density");
        if (num == nullptr) {
            error (mapName, "missing/bogus fog-density field");
            return false;
        }
        this->_fogDensity = num->realVal();
    }
    else {
        this->_hasFog = false;
        this->_fogColor = cs237::color3f(0, 0, 0);
        this->_fogDensity = 0;
    }

  // are there any objects?
    {
        std::string objectsDir = this->_path + "objects";
        if (access(objectsDir.c_str(), F_OK) == 0) {
            this->_objects = new Objects (this);
        }
    }

  // compute and check other map info
    int cellShft = ilog2(this->_cellSize);
    if ((cellShft < 0)
    || (this->_cellSize < Map::MIN_CELL_SIZE)
    || (Map::MAX_CELL_SIZE < this->_cellSize)) {
        error (mapName, "cellSize must be power of 2 in range");
        return false;
    }

    this->_nRows = this->_height >> cellShft;
    this->_nCols = this->_width >> cellShft;
    if ((this->_nRows << cellShft) != this->_height) {
        error (mapName, "map height must be multiple of cell size");
        return false;
    }
    if ((this->_nCols << cellShft) != this->_width) {
        error (mapName, "map width must be multiple of cell size");
        return false;
    }

    if (verbose) {
        std::clog << "name = " << this->_name << "\n";
        std::clog << "h-scale = " << this->_hScale << "\n";
        std::clog << "v-scale = " << this->_vScale << "\n";
        std::clog << "base-elev = " << this->_baseElev << "\n";
        std::clog << "min-elev = " << this->_minElev << "\n";
        std::clog << "max-elev = " << this->_maxElev << "\n";
        std::clog << "min-sky = " << this->_minSky << "\n";
        std::clog << "max-sky = " << this->_maxSky << "\n";
        std::clog << "width = " << this->_width
            << " (" << this->_nCols << " cols)\n";
        std::clog << "height = " << this->_height
            << " (" << this->_nRows << " rows)\n";
        std::clog << "cell-size = " << this->_cellSize << "\n";
        std::clog << "sun-dir = " << this->_sunDir << "\n";
        std::clog << "sun-intensity = " << this->_sunI << "\n";
        std::clog << "ambient = " << this->_ambI << "\n";
        std::clog << "fog-color = " << this->_fogColor << "\n";
        std::clog << "fog-density = " << this->_fogDensity << "\n";
    }

  // get array of grid filenames
    const JSON::Array *grid = root->fieldAsArray("grid");
    if (num == nullptr) {
        error (mapName, "missing/bogus h-scale field");
        return false;
    }
    else if (grid->length() != this->_nCells()) {
        error (mapName, "incorrect number of cells in grid field");
        return false;
    }
    this->_grid = new class Cell*[this->_nCells()];
    for (int r = 0;  r < this->_nRows;  r++) {
        for (int c = 0;  c < this->_nCols;  c++) {
            int i = this->_cellIdx(r, c);
            const JSON::String *s = (*grid)[i]->asString();
            if (s == nullptr) {
                error (mapName, "bogus grid item");
            }
            this->_grid[i] = new class Cell(this, r, c, this->_path + s->value());
        }
    }

    return true;

}


/** Rain Functions **/
//NOTE: The particle system I used is based off of the following tutorial:
// http://www.opengl-tutorial.org/intermediate-tutorials/billboards-particles/particles-instancing/

// sort droplets based on distance from camera
void Map::sortDroplets(){
  std::sort(&particles_container[0], &particles_container[MAX_DROPLETS]);
}

// find the index of a dead droplet
int Map::FindDeadDroplet(){

  // loop through the latter fraction of particles
  int i;
	for(i = lastUsed; i < MAX_DROPLETS; i++){
		if (particles_container[i].lifeSpan < 0){
			lastUsed = i;
			return i;
		}
	}

  // loop through the first fraction of particles
  int j;
	for(j = 0; j < lastUsed; j++){
		if (particles_container[j].lifeSpan < 0){
			lastUsed = j;
			return j;
		}
	}

  // default to first particle if all are alive
	return 0;
}

// iniitializes all particle characteristics to be "dead"
void Map::initParticles(){
  int i;
  for(i = 0; i < MAX_DROPLETS; i++){
    this->particles_container[i].lifeSpan = -1.0f;
    this->particles_container[i].cameraDist = -1.0f;
  }
}

// render the rain
void Map::drawRain(Camera *cam, float dt){

    // array holding all the position data
    GLfloat* position_data = new GLfloat[MAX_DROPLETS * 3];


    //vao
    GLuint vaoId;
  	CS237_CHECK( glGenVertexArrays(1, &vaoId));
  	CS237_CHECK( glBindVertexArray(vaoId));

    //vertices
    GLuint vBufId;
    CS237_CHECK( glGenBuffers(1, &vBufId));
	  CS237_CHECK( glBindBuffer(GL_ARRAY_BUFFER, vBufId));
	  CS237_CHECK( glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW));

    //positions
    GLuint posBufId;
    CS237_CHECK( glGenBuffers(1, &posBufId));
	  CS237_CHECK( glBindBuffer(GL_ARRAY_BUFFER, posBufId));
	  CS237_CHECK(  glBufferData(GL_ARRAY_BUFFER, MAX_DROPLETS * 3 * sizeof(GLfloat), NULL, GL_STREAM_DRAW));

  // Generate new droplets (with an upper limit)
    int lim = (int)(0.016f*10000.0);
    int new_droplets = (int)(dt*10000.0f);

    if(new_droplets > lim)
      new_droplets = lim;

    int i;
    for(i = 0; i < new_droplets; i++){
      int index = FindDeadDroplet();
      // raindrops will live for 15 seconds
      particles_container[index].lifeSpan = 15.0f;
      // spawn the raindrop in a random position (within a range around the camera)
      particles_container[index].pos = cs237::vec3f((float)((rand() % 100 + 1) - 50),
                                                    (float)(rand() % 10 + 100),
                                                    (float)((rand() % 100 + 1) - 50));

      // randomize a wind effect
      float windx = (float)((rand() % 10)/ 70) + 2.5f;
      float windz = (float)((rand() % 10)/ 70) + 2.5f;
      particles_container[index].velocity = cs237::vec3f(windx, -0.0f, windz);
    }


  // main simulation block
    int count = 0;

    int j;
    for(j = 0; j < MAX_DROPLETS; j++){
      particles& droplet = particles_container[i];

      // droplet is alive
      if(droplet.lifeSpan > 0.0f){

        // decay
        droplet.lifeSpan -= dt;

        // raindrop is still alive
        if(droplet.lifeSpan > 0.0f){
          // update droplet characteristics
          droplet.velocity += cs237::vec3f(0.0f, -9.81f, 0.0f) * dt * 0.5f;
          droplet.pos += droplet.velocity * dt;
          droplet.cameraDist = droplet.pos.length();

          // fill buffer
          position_data[3 * count] = droplet.pos[0];
          position_data[3 * count + 1] = droplet.pos[1];
          position_data[3 * count + 2] = droplet.pos[2];

        }
        else // droplet just died
          droplet.cameraDist = -1.0f;

        count++; // only incrementing count for live droplets
      }
    }

    // need to sort droplets to handle opaqueness and blending
    sortDroplets();



  /** setup and draw particles **/

  CS237_CHECK( glBindBuffer(GL_ARRAY_BUFFER, posBufId));
  CS237_CHECK( glBufferData(GL_ARRAY_BUFFER, MAX_DROPLETS * 3 * sizeof(GLfloat), NULL, GL_STREAM_DRAW));
  CS237_CHECK( glBufferSubData(GL_ARRAY_BUFFER, 0, count * sizeof(GLfloat) * 3, position_data));

  CS237_CHECK( glEnable(GL_BLEND));
	CS237_CHECK( glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));

  CS237_CHECK( glEnable(GL_DEPTH_TEST));
  CS237_CHECK( glDepthFunc(GL_LESS));

  // vertex buffer
  CS237_CHECK( glEnableVertexAttribArray(0));
  CS237_CHECK( glBindBuffer(GL_ARRAY_BUFFER, vBufId));
  CS237_CHECK( glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void *)0));

  // position buffer
  CS237_CHECK( glEnableVertexAttribArray(1));
  CS237_CHECK( glBindBuffer(GL_ARRAY_BUFFER, posBufId));
  CS237_CHECK( glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (void *)0));


  //settings for instanced drawing
  CS237_CHECK( glVertexAttribDivisor(0, 0)); // model vertices are reused
  CS237_CHECK( glVertexAttribDivisor(1, 1)); // one "position" per streak

  //draw!
  CS237_CHECK( glDrawArraysInstanced(GL_LINES, 0, 3, count));


  //clean up
  CS237_CHECK( glDisable(GL_BLEND));

  CS237_CHECK( glBindBuffer (GL_ARRAY_BUFFER, 0));
  CS237_CHECK( glBindVertexArray (0));
}


/***** Utility functions *****/

// return the integer log2 of n; if n is not a power of 2, then return -1.
int ilog2 (uint32_t n)
{
    int k = 0, two_k = 1;
    while (two_k < n) {
        k++;
        two_k *= 2;
        if (two_k == n)
            return k;
    }
    return -1;

}
