/*! \file mesh.hxx
 *
 * A class that gathers together all of the information about a triangle mesh.
 *
 * \author John Reppy
 */

/* CMSC23700 Final Project sample code (Autumn 2017)
 *
 * COPYRIGHT (c) 2017 John Reppy (http://cs.uchicago.edu/~jhr)
 * All rights reserved.
 */

#ifndef _MESH_HXX_
#define _MESH_HXX_

#include "cs237.hxx"
#include "obj.hxx"

class Objects;  // defined in map-objects.hxx

//! the different types of textures
enum {
    COLOR_TEX = 0,      //!< index of color texture
    SPEC_TEX = 1,       //!< index of specular-map texture
    NORM_TEX = 2,       //!< index of normal-map texture
    MAX_NUM_TEXS = 3    //!< max number of textures (not all textures are present for all skins)
};

//! the information needed to render a mesh
class TriMesh {
  public:

    //! The locations of the standard mesh attributes.  The layout directives
    //! in the shaders should match these values.
    //
    const GLint     CoordAttrLoc = 0;       //!< location of vertex coordinates attribute
    const GLint     NormAttrLoc = 1;        //!< location of normal-vector attribute
    const GLint     TexCoordAttrLoc = 2;    //!< location of texture coordinates attribute
    const GLint     TanAttrLoc = 3;         //!< location of extended tangent vector

  //! create a TriMesh object for an group in an OBJ model
  //! \param mapObjs the map-objects manager
  //! \param model the model that the group is part of
  //! \param grp the OBJ Group used to initialize the vertex buffers
  //!
  //! The vertex buffers for the mesh are initialized from the \arg grp, while
  //! the material properties are taken from the group's material in the model.
  //
    TriMesh (const Objects *mapObjs, OBJ::Model const *model, OBJ::Group const &grp);

  //! destructor
    virtual ~TriMesh ();

  /***** Diffuse color operations *****/

  //! return the diffuse color for this mesh to use in lieu of the diffuse-color map
    cs237::color3f DiffuseColor () const { return this->_diffuseC; }

  //! does this mesh have a diffuse-color map?
    bool hasDiffuseMap () const { return (this->_dMap != nullptr); }

  //! return this mesh's diffuse-color map (or nullptr)
    const cs237::texture2D *DiffuseMap() const  { return this->_dMap; }

  //! bind the mesh's diffuse-color map to the specified texture unit
    void BindDiffuseMap (GLuint txtUnit)
    {
        assert (this->_dMap != nullptr);
        CS237_CHECK( glActiveTexture(txtUnit) );
        this->_dMap->Bind();
    }

  /***** Specular color operations *****/

  //! does this mesh have a specular color?
    bool hasSpecular () const { return this->_hasSpecular; }

  //! return the uniform specular color for this mesh
    cs237::color3f SpecularColor () const { return this->_specularC; }

  //! return the sharpness exponent of the surface (aka the Phong exponent)
    float Sharpness () const { return this->_sharpness; }

  //! does this mesh have a specular-color map?
    bool hasSpecularMap () const { return (this->_sMap != nullptr); }

  //! return this mesh's specular-color map (or nullptr)
    const cs237::texture2D *SpecularMap() const  { return this->_sMap; }

  //! bind the mesh's specular-color map to the specified texture unit
    void BindSpecularMap (GLuint txtUnit)
    {
        assert (this->_sMap != nullptr);
        CS237_CHECK( glActiveTexture(txtUnit) );
        this->_sMap->Bind();
    }

  /***** Emissive color operations *****/

  //! does this mesh have a emissive color?
    bool hasEmissive () const { return this->_hasEmissive; }

  //! return the uniform emissive color for this mesh
    cs237::color3f EmissiveColor () const { return this->_emissiveC; }

  //! does this mesh have a emissive-color map?
    bool hasEmissiveMap () const { return (this->_eMap != nullptr); }

  //! return this mesh's emissive-color map (or nullptr)
    const cs237::texture2D *EmissiveMap() const  { return this->_eMap; }

  //! bind the mesh's emissive-color map to the specified texture unit
    void BindEmissiveMap (GLuint txtUnit)
    {
        assert (this->_eMap != nullptr);
        CS237_CHECK( glActiveTexture(txtUnit) );
        this->_eMap->Bind();
    }

  /***** Normal-map operations *****/

  //! set a mesh's normal map
    void SetNormalMap (cs237::texture2D *map) { this->_nMap = map; }

  //! does this mesh have a normal map?
    bool hasNormalMap () const { return (this->_nMap != nullptr); }

  //! return this mesh's normal map (or nullptr)
    const cs237::texture2D *NormalMap() const { return this->_nMap; }

  //! bind the mesh's normal map to the specified texture unit
    void BindNormalMap (GLuint txtUnit)
    {
        assert (this->_nMap != nullptr);
        CS237_CHECK( glActiveTexture(txtUnit) );
        this->_nMap->Bind();
    }

  //! draw the mesh using a glDrawElements call
  //! \param enableNorms   when true, we enable the normal-vector attribute buffer
  //! \param enableTxts    when true, we enable the texture-coordinate attribute buffer
  //! \param enableTans    when true, we wnable the tangent-vector attribute buffer
  //
  //! We assume that the shader uniforms have been set
  //
    void Draw (bool enableNorms, bool enableTxts, bool enableTans);

  protected:

  //! a constructor for subclasses of meshes.
  //! \param p the primitive use to render the mesh
    TriMesh (GLenum p);

    GLuint              _vaoId;         //!< vertex-array-object ID for this mesh
    GLuint              _vBufId;        //!< buffer ID for vertex coordinates
    GLuint              _nBufId;        //!< buffer ID for normal vectors
    GLuint              _tcBufId;       //!< buffer ID for texture coordinates
    GLuint              _tanBufId;      //!< buffer ID for tangent 4-vectors; this buffer
                                        //! is present when there are normals and a normal
                                        //! map.
    GLuint              _eBufId;        //!< buffer ID for the index array
    GLenum              _prim;          //!< the primitive type for rendering the mesh
                                        //!  (e.g., GL_TRIANGLES, GL_TRIANGLE_FAN, etc.)
    int                 _nIndices;      //!< the number of vertex indices
    bool                _hasNorms;      //!< does this mesh have vertex normals?
    bool                _hasTxtCoords;  //!< does this mesh have texture coordinates?
    bool                _hasTans;       //!< does this mesh have tangent-vectors?
  // material properties
  // We assume all meshes have a diffuse default color.  They may also have a
  // a diffuse color map, a specular color/map, and an emissive color/map.
    bool                _hasEmissive;   //!< true if this mesh has a uniform emissive color
    bool                _hasSpecular;   //!< true if this mesh has a uniform specular color
    cs237::color3f      _emissiveC;     //!< the uniform emissive color
    cs237::color3f      _diffuseC;      //!< the uniform diffuse color
    cs237::color3f      _specularC;     //!< the uniform specular color
    float               _sharpness;     //!< the sharpness exponent
    cs237::texture2D    *_eMap;         //!< the emissive-color map for the mesh (or nullptr)
    cs237::texture2D    *_dMap;         //!< the diffuse-color map for the mesh (or nullptr)
    cs237::texture2D    *_sMap;         //!< the specular-color map for the mesh (or nullptr)
    cs237::texture2D    *_nMap;         //!< the normal-map for the mesh (or nullptr)

};

#endif // !_MESH_HXX_
