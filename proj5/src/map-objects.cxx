/*! \file map-objects.cxx
 *
 * Implementation of the Objects class.
 *
 * \author John Reppy
 */

/*
 * COPYRIGHT (c) 2017 John Reppy (http://cs.uchicago.edu/~jhr)
 * All rights reserved.
 */

#include "map-objects.hxx"
#include "map.hxx"
#include "map-cell.hxx"
#include "mesh.hxx"
#include "json.hxx"
#include <unistd.h>

/* helper functions to make extracting values from the JSON easier */

//! load a vec3f from a JSON object.
//! \return false if okay, true if there is an error.
bool LoadVec3 (JSON::Object const *jv, cs237::vec3f &vec)
{
    if (jv == nullptr) {
        return true;
    }

    const JSON::Number *x = jv->fieldAsNumber("x");
    const JSON::Number *y = jv->fieldAsNumber("y");
    const JSON::Number *z = jv->fieldAsNumber("z");
    if ((x == nullptr) || (y == nullptr) || (z == nullptr)) {
        return true;
    }

    vec = cs237::vec3f (
        static_cast<float>(x->realVal()),
        static_cast<float>(y->realVal()),
        static_cast<float>(z->realVal()));

    return false;
}

//! load a color3f from a JSON object.
//! \return false if okay, true if there is an error.
bool LoadColor (JSON::Object const *jv, cs237::color3f &color)
{
    if (jv == nullptr) {
        return true;
    }

    const JSON::Number *r = jv->fieldAsNumber("r");
    const JSON::Number *g = jv->fieldAsNumber("g");
    const JSON::Number *b = jv->fieldAsNumber("b");
    if ((r == nullptr) || (g == nullptr) || (b == nullptr)) {
        return true;
    }

    color = cs237::color3f (
        static_cast<float>(r->realVal()),
        static_cast<float>(g->realVal()),
        static_cast<float>(b->realVal()));

    return false;
}

/***** class Objects member functions *****/

Objects::Objects (const Map *map)
  : _map(map), _objsDir(map->_path + "objects/"), _objs(), _texs()
{ }

// load the objects instances for a map cell
//
bool Objects::LoadObjects (std::string const &cell, std::vector<Instance *> &objs)
{
    std::string objsFile = cell + "/objects.json";

  // make sure that objs is empty
    objs.clear();

  // if the objects.json file does not exist, then we return the empty
  // vector
    if (access(objsFile.c_str(), F_OK) != 0) {
        return false;
    }

  // load the objects list
    JSON::Value *root = JSON::ParseFile(objsFile);

  // check for errors
    if (root == nullptr) {
        std::cerr << "Unable to load objects list \"" << objsFile << "\"\n";
        return true;
    } else if (! root->isArray()) {
        std::cerr << "Invalid object list in \"" << objsFile
            << "\"; root is not an array\n";
        return true;
    }
    const JSON::Array *rootObj = root->asArray();

    if (rootObj->length() != 0) {
      // allocate space for the objects in the scene
        objs.reserve(rootObj->length());

      // load the object instances in the cell
        for (int i = 0;  i < rootObj->length();  i++) {
            JSON::Object const *object = (*rootObj)[i]->asObject();
            if (object == nullptr) {
                std::cerr << "Expected array of JSON objects in \""
                    << objsFile << "\"\n";
                return true;
            }
            JSON::String const *file = object->fieldAsString("file");
            JSON::Object const *frame = object->fieldAsObject("frame");
            cs237::vec3f pos, xAxis, yAxis, zAxis;
            cs237::color3f color;
            if ((file == nullptr) || (frame == nullptr)
            ||  LoadVec3 (object->fieldAsObject("pos"), pos)
            ||  LoadVec3 (frame->fieldAsObject("x-axis"), xAxis)
            ||  LoadVec3 (frame->fieldAsObject("y-axis"), yAxis)
            ||  LoadVec3 (frame->fieldAsObject("z-axis"), zAxis)
            ||  LoadColor (object->fieldAsObject("color"), color)) {
                std::cerr << "Invalid object description in \"" << objsFile << "\"\n";
                return true;
            }
            Instance *inst = this->_MakeInstance(
                file->value(),
                cs237::mat4f (
                    cs237::vec4f (xAxis, 0.0f),
                    cs237::vec4f (yAxis, 0.0f),
                    cs237::vec4f (zAxis, 0.0f),
                    cs237::vec4f (pos, 1.0f)),
                color);
          // add to objs vector
            objs.push_back (inst);
        }

    }

    return false;
}

// load an OBJ model from a file
//
GObject *Objects::LoadModel (
    std::string const &dir,
    std::string const &file,
    cs237::AABBf &bbox)
{
    GObject *gObj;
    auto it = this->_objs.find (file);
    if (it == this->_objs.end()) {
      // load the model from the OBJ file
        OBJ::Model *model = new OBJ::Model (dir + file);
        bbox = model->BBox();
      // preload any textures in the materials of the model
        for (auto grpIt = model->beginGroups();  grpIt != model->endGroups();  grpIt++) {
            const OBJ::Material *mat = &model->Material((*grpIt).material);
            /* we ignore the ambient map */
            this->_LoadTexture (dir, mat->emissiveMap, true);
            this->_LoadTexture (dir, mat->diffuseMap, true);
            this->_LoadTexture (dir, mat->specularMap, true);
            this->_LoadTexture (dir, mat->normalMap, false);
        }
      // create the meshes
        gObj = new std::vector<TriMesh *>();
        gObj->reserve(model->NumGroups());
        for (auto git = model->beginGroups();  git != model->endGroups();  git++) {
            TriMesh *mesh = new TriMesh (this, model, *git);
            gObj->push_back(mesh);
        }
      // cache the meshes
        this->_objs[file] = std::pair<cs237::AABBf, GObject *>(bbox, gObj);
      // we can release the storage for the model
        delete model;
    }
    else {
        bbox = it->second.first;
        gObj = it->second.second;
    }

    return gObj;
}

// helper function for making instances of objects
Instance *Objects::_MakeInstance (
    std::string const &file,
    cs237::mat4f const &toCell,
    cs237::color3f const &color)
{
  // first we need to get the GObject and bounding box for the file
    cs237::AABBf bbox;
    GObject *gObj = this->LoadModel (this->_objsDir, file, bbox);

  // create the instance
    Instance *inst = new Instance;
    inst->meshes = gObj;
    inst->toCell = toCell;
    inst->normToWorld = toCell.normalMatrix();
    inst->normFromWorld = inst->normToWorld.transpose();
    inst->color = color;

  // compute the bounding box after transformation to cell coordinates
    inst->bbox.clear();
    for (int j = 0;   j < 8;  j++) {
        cs237::vec3f pt =
            cs237::vec3f(inst->toCell * cs237::vec4f(bbox.corner(j), 1));
        inst->bbox.addPt(pt);
    }

    return inst;

}

// return the pointer to a pre-loaded 2D texture
//
cs237::texture2D *Objects::LoadTexture2D (std::string const &name) const
{
    if (! name.empty()) {
        auto it = this->_texs.find(name);
        if (it != this->_texs.end()) {
            return it->second;
        }
    }
    return nullptr;

}

// helper function for loading textures
//
void Objects::_LoadTexture (std::string path, std::string name, bool genMipmaps)
{
    if (name.empty()) {
        return;
    }
  // have we already loaded this texture?
    if (this->_texs.find(name) != this->_texs.end()) {
        return;
    }
  // load the image data;
    cs237::image2d *img = new cs237::image2d(path + name);
    if (img == nullptr) {
        std::cerr << "Unable to find texture-image file \"" << path+name << "\"\n";
        exit (1);
    }
    cs237::texture2D *txt = new cs237::texture2D(GL_TEXTURE_2D, img);
    txt->Parameter (GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    if (genMipmaps) {
        CS237_CHECK( glGenerateMipmap(GL_TEXTURE_2D) );
        txt->Parameter (GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    }
    else {
        txt->Parameter (GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    }
  // add to _texs map
    this->_texs.insert (std::pair<std::string, cs237::texture2D *>(name, txt));
  // free image
    delete img;

}
