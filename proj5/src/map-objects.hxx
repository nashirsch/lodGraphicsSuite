/*! \file map-objects.hxx
 *
 * Support for loading OBJ objects with supporting textures.  The Objects class
 * is a singleton class that is embedded in the Map object.  It is meant to support
 * both loading renderer-specific objects from the 'data' directory as well as
 * map-specific objects from the map's 'objects' directory.
 *
 * Note that names used for objects and textures need to be globally unique.
 *
 * \author John Reppy
 */

/*
 * COPYRIGHT (c) 2017 John Reppy (http://cs.uchicago.edu/~jhr)
 * All rights reserved.
 */

#ifndef _MAP_OBJECTS_HXX_
#define _MAP_OBJECTS_HXX_

#include "cs237.hxx"
#include "obj.hxx"
#include <map>
#include <functional>

class TriMesh;  // defined in "mesh.hxx"
class Map; // defined in map.hxx

//! one or more meshes that define an object.  These correspond
//! to the groups in the OBJ file
typedef std::vector<TriMesh *> GObject;

//! An instance of a graphical object in a map cell
struct Instance {
    const GObject       *meshes;        //!< the mesh data
    cs237::mat4f        toCell;         //!< affine transform from object space to cell space
    cs237::mat3f        normToWorld;    //!< linear transform that maps object-space normals
                                        //!  to world-space normals
    cs237::mat3f        normFromWorld;  //!< linear transform that maps world-space normals
                                        //!  to object-space normals (e.g., light)
    cs237::AABBf        bbox;           //!< the bounding box around the transformed mesh
                                        //!< in map-cell coordinates
    cs237::color3f      color;          //!< the color of the object for wireframe
                                        //!< rendering
};

class Objects {
  public:

    //! constructor
    //! \param map the map that contains the objects
    Objects (const Map *map);

    ~Objects ();

    //! load the objects instances for a map cell
    //! \param cell the path to the cell's subdirectory
    //! \param objs the vector of object instances that is filled from the objects list
    //! \return true if there is an error, false otherwise
    bool LoadObjects (std::string const &cell, std::vector<Instance *> &objs);

    //! load an OBJ model from a file
    //! \param[in] dir the name of the directory holding the model and supporting files
    //! \param[in] file the name of the OBJ file
    //! \param[out] bbox is set to the model's AABB on exit
    //! \return the meshes representing the object
    GObject *LoadModel (
        std::string const &dir,
        std::string const &file,
        cs237::AABBf &bbox);

    //! return the pointer to a pre-loaded 2D texture
    //! \param name the name of the texture source image file relative to the
    //!             map's objects directory
    //! \returns a pointer to the texture object or nullptr if the file is not found
    cs237::texture2D *LoadTexture2D (std::string const &name) const;

  private:
    const Map *_map;                    //!< the map
    std::string _objsDir;               //!< the 'objects' directory that holds OBJ, MTL,
                                        //!  and texture files.
    std::map<std::string, std::pair<cs237::AABBf, GObject *> > _objs;
                                        //!< object-mesh cache
    std::map<std::string, cs237::texture2D *> _texs;
                                        //!< texture cache

    //! helper function for creating instances
    Instance *_MakeInstance (
        std::string const &file,
        cs237::mat4f const &toCell,
        cs237::color3f const &color);

    //! helper function for pre-loading textures for materials
    //! \param path the name of the directory holding the texture
    //! \param name the name of the texture source image file relative to path
    //! \param genMipmaps if true, then glGenerateMipmap is called to generated
    //!                   mipmaps and the sampling mode is set to LINEAR_MIPMAP_LINEAR
    void _LoadTexture (std::string path, std::string name, bool genMipmaps = false);
};

#endif //! _MAP_OBJECTS_HXX_
