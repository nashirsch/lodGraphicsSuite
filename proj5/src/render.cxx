/*! \file render.cxx
 *
 * \author John Reppy
 *
 * These are additional functions for the View, Cell, Tile, and Chunk classes for
 * rendering the mesh.
 */

/* CMSC23700 Final Project sample code (Autumn 2017)
 *
 * COPYRIGHT (c) 2017 John Reppy (http://cs.uchicago.edu/~jhr)
 * All rights reserved.
 */

#include "cs237.hxx"
#include "view.hxx"
#include "camera.hxx"
#include "map-cell.hxx"
#include "buffer-cache.hxx"
#include "texture-cache.hxx"

//! Colors to use for rendering wireframes at different levels of detail
static cs237::color4ub MeshColor[Cell::MAX_NUM_LODS] = {
        cs237::color4ub{ 255, 255,   0, 255 },
        cs237::color4ub{ 255,   0, 255, 255 },
        cs237::color4ub{   0, 255, 255, 255 },
        cs237::color4ub{ 255, 128, 128, 255 },
        cs237::color4ub{ 128, 255, 128, 255 },
        cs237::color4ub{ 128, 128, 255, 255 },
        cs237::color4ub{ 255,   0,   0, 255 },
        cs237::color4ub{   0, 255,   0, 255 },
        cs237::color4ub{   0,   0, 255, 255 }
    };

bool Tile::isBehind(cs237::vec3d pt, cs237::vec3d norm, double distance){
  if(cs237::__detail::dot(pt, norm) < -distance){
    return true;
  }
  else
    return false;
}

// true if inside/instersecting, false otherwise
bool Tile::FrustumCheck(View* view, cs237::AABBd bbox){
  cs237::vec3d corners[8];

  int i;
  for(i = 0; i < 8; i++){
    corners[i] = bbox.corner(i);
  }

  int p, j;
  for(p = 0; p < 6; p++){
    int iInCount = 8;

    for(j = 0; j < 8; j++){
      if(isBehind(corners[j], view->Frustum()->normals[p], view->Frustum()->distances[p])) {
        --iInCount;
      }
    }

    if(iInCount == 0)
      return false;
  }

    return true;
}

// return true if the error margin is satisfactory
bool Tile::ErrorCheck(View* view){
  float D = this->BBox().distanceToPt(view->Camera().position());
  float scError = view->Camera().screenError(D, this->Chunk()._maxError);

  if(view->ErrorLimit() >= scError)
      return true;
  else
      return false;
}

// reset status variables & release resources
void Tile::Release(View* view, int Status){
  this->_morphFrom = 0;
  this->_drawStatus = Status;
  this->_currentT = 0.0f;

  view->VAOCache()->Release(this->_vao);
  this->_texture->Release();
  this->_nmap->Release();

}

//abort any morphing to hower level of detail in the tile's subtree
void Tile::AbortMorphUp(View* view){


  if(this->NumChildren() == 0)
    return;

  if(this->Child(0)->GetMorph() == -1){
    this->Child(0)->SetMorph(0);
    this->Child(0)->SetCurrentT(0.0f);
    this->Child(0)->Release(view, NotDrawn);
  }


  if(this->Child(1)->GetMorph() == -1){
    this->Child(1)->SetMorph(0);
    this->Child(1)->SetCurrentT(0.0f);
    this->Child(1)->Release(view, NotDrawn);
  }

  if(this->Child(2)->GetMorph() == -1){
    this->Child(2)->SetMorph(0);
    this->Child(2)->SetCurrentT(0.0f);
    this->Child(2)->Release(view, NotDrawn);
  }

  if(this->Child(3)->GetMorph() == -1){
    this->Child(3)->SetMorph(0);
    this->Child(3)->SetCurrentT(0.0f);
    this->Child(3)->Release(view, NotDrawn);
  }

  this->Child(0)->AbortMorphUp(view);
  this->Child(1)->AbortMorphUp(view);
  this->Child(2)->AbortMorphUp(view);
  this->Child(3)->AbortMorphUp(view);


}

// loop through the texture trees, releasing and acquiring resources as needed
void Tile::TileSet(int releaseMode, View* view, float dt,
                                                TQT::TextureQTree *ttree,
                                                TQT::TextureQTree *ntree){

    //if rM == 2: still looking for tile
    //  if not in the frustrum, remove from cache, call w rM = 1
    //  if not enough detail, make sure released from caches
    //  if enough detail, add to caches

    //if rM == 1: already found tile
    //  make sure everything removed

    //if rM == 0: not in frustum

    //visibility information
    //drawStatus: 0, 1, 2 (0 = not in frustum, 1 = not drawn, 2 = drawn)

    VAO* bufferReturn;

    //if were still looking for a tile to draw, or are looking for the higher LOD to morph to
    if((releaseMode == TileSearch) || (releaseMode == MorphDown) ){
      if(FrustumCheck(view, this->BBox())){ //tile is in the view frustum
        if(ErrorCheck(view)){ //error margin is satisfactory

          if(this->_drawStatus != Drawn){ //wasnt shown last frame

            if(this->_morphFrom != -1){ //isnt morphing to higher LOD

              //acquire resources
              bufferReturn = view->VAOCache()->Acquire();
              bufferReturn->Load(this->Chunk());
              this->_vao = bufferReturn;
              this->_texture = view->TxtCache()->Make(ttree, this->LOD(),
                                                        this->NWRow()/this->Width(),
                                                        this->NWCol()/this->Width());
              this->_texture->Activate();
              this->_nmap = view->TxtCache()->Make(ntree, this->LOD(),
                                                     this->NWRow()/this->Width(),
                                                     this->NWCol()/this->Width());
              this->_nmap->Activate();
              this->_drawStatus = Drawn;

              //if this node has children, and we arent morphing to higher LOD, we must morph to lower LOD
              if((this->NumChildren() != 0) && (this->GetMorph() == 0) && (releaseMode == TileSearch)){

              	//if child isn't morphing, initiate morph
                if((this->Child(0)->GetMorph() == 0)){
                  if(this->Child(0)->GetStatus() != Drawn){
                    this->Child(0)->TileSet(MorphUp, view, dt, ttree, ntree);
                  }
                  this->Child(0)->SetMorph(-1);
                  this->SetMorph(-1);
                  this->SetStatus(NotDrawn);
                }
                else if(this->Child(0)->GetMorph() == -1){
                  this->Child(0)->AbortMorphUp(view);
                  this->Child(0)->SetCurrentT(0.0f);
                  this->Child(0)->SetStatus(Drawn);
                }

                //if child isn't morphing, initiate morph
                if((this->Child(1)->GetMorph() == 0)){
                  if(this->Child(1)->GetStatus() != Drawn){
                    this->Child(1)->TileSet(MorphUp, view, dt, ttree, ntree);
                  }
                  this->Child(1)->SetMorph(-1);
                  this->SetMorph(-1);
                  this->SetStatus(NotDrawn);
                }
                else if(this->Child(1)->GetMorph() == -1){
                  this->Child(1)->AbortMorphUp(view);
                  this->Child(1)->SetCurrentT(0.0f);
                  this->Child(1)->SetStatus(Drawn);
                }

                //if child isn't morphing, initiate morph
                if((this->Child(2)->GetMorph() == 0)){
                  if(this->Child(2)->GetStatus() != Drawn){
                    this->Child(2)->TileSet(MorphUp, view, dt, ttree, ntree);
                  }
                  this->Child(2)->SetMorph(-1);
                  this->SetMorph(-1);
                  this->SetStatus(NotDrawn);
                }
                else if(this->Child(2)->GetMorph() == -1){
                  this->Child(2)->AbortMorphUp(view);
                  this->Child(2)->SetCurrentT(0.0f);
                  this->Child(2)->SetStatus(Drawn);
                }

                //if child isn't morphing, initiate morph
                if((this->Child(3)->GetMorph() == 0)){
                  if(this->Child(3)->GetStatus() != Drawn){
                    this->Child(3)->TileSet(MorphUp, view, dt, ttree, ntree);
                  }
                  this->Child(3)->SetMorph(-1);
                  this->SetMorph(-1);
                  this->SetStatus(NotDrawn);
                }
                else if(this->Child(3)->GetMorph() == -1){
                  this->Child(3)->AbortMorphUp(view);
                  this->Child(3)->SetCurrentT(0.0f);
                  this->Child(3)->SetStatus(Drawn);
                }
                return;
              }
            }
          }

          //if this is the tile we're morphing down to, initiate
          if(releaseMode == MorphDown){

              this->_drawStatus = Drawn;
              this->_currentT = 1.0f;
              this->_morphFrom = 1;

          }

          //found tile to draw, recurse on children freeing them
          if(this->NumChildren() != 0) { //child is a leaf
            this->Child(0)->TileSet(FoundTile, view, dt, ttree, ntree);
            this->Child(1)->TileSet(FoundTile, view, dt, ttree, ntree);
            this->Child(2)->TileSet(FoundTile, view, dt, ttree, ntree);
            this->Child(3)->TileSet(FoundTile, view, dt, ttree, ntree);
          }
          return;

        }
        else{ //error margin in not satisfactory
          if(this->NumChildren() == 0){ //no more children, draw anyway

            if(this->_drawStatus != Drawn){
                bufferReturn = view->VAOCache()->Acquire();
                bufferReturn->Load(this->Chunk());
                this->_vao = bufferReturn;

                this->_texture = view->TxtCache()->Make(ttree, this->LOD(),
                                                          this->NWRow()/this->Width(),
                                                          this->NWCol()/this->Width());
                this->_texture->Activate();

                this->_nmap = view->TxtCache()->Make(ntree, this->LOD(),
                                                       this->NWRow()/this->Width(),
                                                       this->NWCol()/this->Width());
                this->_nmap->Activate();

                this->_drawStatus = Drawn;

            }
            return;

          }
          else{

          	//drawn tile no longer satisfactory, morph down a level
            if(this->_drawStatus == Drawn){

                this->Release(view, NotDrawn);

                this->Child(0)->TileSet(MorphDown, view, dt, ttree, ntree);
                this->Child(1)->TileSet(MorphDown, view, dt, ttree, ntree);
                this->Child(2)->TileSet(MorphDown, view, dt, ttree, ntree);
                this->Child(3)->TileSet(MorphDown, view, dt, ttree, ntree);

            }
            else{

              //if morph in progress to lower LOD, abort it as were going to higher LOD now
              if(this->GetMorph() == -1){
                this->AbortMorphUp(view);
                this->Release(view, NotDrawn);

                this->Child(0)->TileSet(MorphUp, view, dt, ttree, ntree);
                this->Child(1)->TileSet(MorphUp, view, dt, ttree, ntree);
                this->Child(2)->TileSet(MorphUp, view, dt, ttree, ntree);
                this->Child(3)->TileSet(MorphUp, view, dt, ttree, ntree);
              }
              else{ //no morph, just go to higher LOD

                this->_drawStatus = NotDrawn;
                this->Child(0)->TileSet(TileSearch, view, dt, ttree, ntree);
                this->Child(1)->TileSet(TileSearch, view, dt, ttree, ntree);
                this->Child(2)->TileSet(TileSearch, view, dt, ttree, ntree);
                this->Child(3)->TileSet(TileSearch, view, dt, ttree, ntree);

              }
            }

            return;
          }
        }
      }
      else{ //tile is not in contained in the view frustum

      	//clear tile outside frustum if not morphing
        if((this->_drawStatus == Drawn) && (this->GetMorph() != -1)){
            this->Release(view, OutsideFrustum);
        }

        //recurse on children to clear them
        if(this->_drawStatus == NotDrawn){
          if(this->NumChildren() != 0){
              this->Child(0)->TileSet(OutsideFrustum, view, dt, ttree, ntree);
              this->Child(1)->TileSet(OutsideFrustum, view, dt, ttree, ntree);
              this->Child(2)->TileSet(OutsideFrustum, view, dt, ttree, ntree);
              this->Child(3)->TileSet(OutsideFrustum, view, dt, ttree, ntree);
          }

          //again, if this tile morphing dont change
          if(this->GetMorph() != -1)
            this->_drawStatus = OutsideFrustum;
        }

        return;
      }
    }
    else if(releaseMode == FoundTile){ //adequate tile already found, releasing resources

      if(this->_morphFrom == -1)
        return;

      if(this->_drawStatus == Drawn){
          this->Release(view, NotDrawn);
      }

      this->_drawStatus = NotDrawn;
      if(this->NumChildren() != 0){
          this->Child(0)->TileSet(FoundTile, view, dt, ttree, ntree);
          this->Child(1)->TileSet(FoundTile, view, dt, ttree, ntree);
          this->Child(2)->TileSet(FoundTile, view, dt, ttree, ntree);
          this->Child(3)->TileSet(FoundTile, view, dt, ttree, ntree);
      }

      return;
    }
    else if(releaseMode == OutsideFrustum){ //tile determined to be outside frustum

      if(this->GetMorph() != -1){ //if not morphing to lower LOD, release tile

        if(this->_drawStatus == Drawn){
            this->Release(view, OutsideFrustum);
        }
        this->_drawStatus = OutsideFrustum;

      }

      if(this->NumChildren() != 0){ //recurse on children
          this->Child(0)->TileSet(OutsideFrustum, view, dt, ttree, ntree);
          this->Child(1)->TileSet(OutsideFrustum, view, dt, ttree, ntree);
          this->Child(2)->TileSet(OutsideFrustum, view, dt, ttree, ntree);
          this->Child(3)->TileSet(OutsideFrustum, view, dt, ttree, ntree);
      }

      return;

    }
    else if(releaseMode == MorphUp){ //used to explicitly acquire resources for LOD below for morphing
    								 //regardless of frustm or error checks

      bufferReturn = view->VAOCache()->Acquire();
      bufferReturn->Load(this->Chunk());
      this->_vao = bufferReturn;
      this->_texture = view->TxtCache()->Make(ttree, this->LOD(),
                                                this->NWRow()/this->Width(),
                                                this->NWCol()/this->Width());
      this->_texture->Activate();
      this->_nmap = view->TxtCache()->Make(ntree, this->LOD(),
                                             this->NWRow()/this->Width(),
                                             this->NWCol()/this->Width());
      this->_nmap->Activate();
      this->_drawStatus = Drawn;

    }

}

// loop through the tiles, prepping them for drawing
void Tile::DrawChunks(View* view, float dt){
    int DS = this->_drawStatus;

    if(DS == Drawn)
      this->Draw(view, dt);

    else if(DS == NotDrawn){

      if(this->NumChildren() == 0){
        return;
      }

      this->Child(0)->DrawChunks(view, dt);
      this->Child(1)->DrawChunks(view, dt);
      this->Child(2)->DrawChunks(view, dt);
      this->Child(3)->DrawChunks(view, dt);

    }

    if((DS == NotDrawn) && (this->_morphFrom == -1)){
      if((this->Child(0)->_morphFrom +
          this->Child(1)->_morphFrom +
          this->Child(2)->_morphFrom +
          this->Child(3)->_morphFrom) == 0){

        this->_morphFrom = 0;
        this->_drawStatus = Drawn;
        this->Draw(view, dt);
      }
    }

}

// render a tile
void Tile::Draw(View* view, float dt){
  if(this->_morphFrom == 1){

    this->_currentT -= dt/MORPH_TIME;
    if(this->_currentT <= 0){
      this->_morphFrom = 0;
      this->_currentT = 0.0f;
    }

  }

  if(this->_morphFrom == -1){

    this->_currentT += dt/MORPH_TIME;
    if(this->_currentT >= 1.0f){
      this->_morphFrom = 0;
      this->_currentT = 0.0f;

      this->Release(view, NotDrawn);

      return;
    }

  }

  if(view->wireframeMode()){

    cs237::setUniform(view->wfScalarLoc, cs237::vec4f(view->Map()->hScale(),
                                                      view->Map()->vScale(),
                                                      view->Map()->hScale(),
                                                      view->Map()->vScale() * this->_currentT));

    cs237::setUniform(view->wfColorLoc, MeshColor[this->LOD()]);
    cs237::setUniform(view->wfViewMatLoc, view->Camera().viewTransform());

  }
  else{

    cs237::setUniform(view->tScalarLoc, cs237::vec4f(view->Map()->hScale(),
                                                     view->Map()->vScale(),
                                                     view->Map()->hScale(),
                                                     view->Map()->vScale() * this->_currentT));

    cs237::setUniform(view->tViewMatLoc, view->Camera().viewTransform());
    cs237::setUniform(view->tTileWidthLoc, (int)this->Width());
    cs237::setUniform(view->tColLoc, (int) this->NWCol());
    cs237::setUniform(view->tRowLoc, (int) this->NWRow());

    this->_texture->Use(0);
    this->_nmap->Use(1);

  }

  this->_vao->Render();

}

//NOTE: The code used to handle the skybox was based off this tutorial:
//      http://antongerdelan.net/opengl/cubemaps.html
void View::drawSky(){

  float verts[] = {
     -1000.0f,  1000.0f, -1000.0f,
     -1000.0f, -1000.0f, -1000.0f,
      1000.0f, -1000.0f, -1000.0f,
      1000.0f, -1000.0f, -1000.0f,
      1000.0f,  1000.0f, -1000.0f,
     -1000.0f,  1000.0f, -1000.0f,

     -1000.0f, -1000.0f,  1000.0f,
     -1000.0f, -1000.0f, -1000.0f,
     -1000.0f,  1000.0f, -1000.0f,
     -1000.0f,  1000.0f, -1000.0f,
     -1000.0f,  1000.0f,  1000.0f,
     -1000.0f, -1000.0f,  1000.0f,

      1000.0f, -1000.0f, -1000.0f,
      1000.0f, -1000.0f,  1000.0f,
      1000.0f,  1000.0f,  1000.0f,
      1000.0f,  1000.0f,  1000.0f,
      1000.0f,  1000.0f, -1000.0f,
      1000.0f, -1000.0f, -1000.0f,

     -1000.0f, -1000.0f,  1000.0f,
     -1000.0f,  1000.0f,  1000.0f,
      1000.0f,  1000.0f,  1000.0f,
      1000.0f,  1000.0f,  1000.0f,
      1000.0f, -1000.0f,  1000.0f,
     -1000.0f, -1000.0f,  1000.0f,

     -1000.0f,  1000.0f, -1000.0f,
      1000.0f,  1000.0f, -1000.0f,
      1000.0f,  1000.0f,  1000.0f,
      1000.0f,  1000.0f,  1000.0f,
     -1000.0f,  1000.0f,  1000.0f,
     -1000.0f,  1000.0f, -1000.0f,

     -1000.0f, -1000.0f, -1000.0f,
     -1000.0f, -1000.0f,  1000.0f,
      1000.0f, -1000.0f, -1000.0f,
      1000.0f, -1000.0f, -1000.0f,
     -1000.0f, -1000.0f,  1000.0f,
      1000.0f, -1000.0f,  1000.0f
  };


    unsigned int vao;
    unsigned int v_vbo;

  // load in textures for sunny skybox
  this->skyboxshader->Use();
  cs237::setUniform(this->skyProjMatLoc, this->_cam.projTransform());

  cs237::mat3x3f view = (cs237::mat3x3f)this->_cam.viewTransform();

  //NOTE: I talked to Alex Friedman about constructing this matrix
  // essentially, it removes the camera translation from the view matrix
  cs237::mat4x4f view4f = cs237::mat4x4f( cs237::vec4f(view[0], 0.0f),
                                          cs237::vec4f(view[1], 0.0f),
                                          cs237::vec4f(view[2], 0.0f),
                                          cs237::vec4f(0.0f, 0.0f, 0.0f, 1.0f));

  cs237::setUniform(this->skyViewMatLoc, view4f);

  cs237::setUniform(this->cubeMapLoc, 3);

  glActiveTexture(GL_TEXTURE3);

  if(this->_rainMode)
    glBindTexture(GL_TEXTURE_CUBE_MAP, this->cloudyTexture);
  else
    glBindTexture(GL_TEXTURE_CUBE_MAP, this->sunnyTexture);


    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &v_vbo);

    CS237_CHECK( glBindVertexArray (vao) );

  // vertex buffer set-up
    CS237_CHECK( glBindBuffer (GL_ARRAY_BUFFER, v_vbo) );
    CS237_CHECK( glBufferData (
            GL_ARRAY_BUFFER,
            sizeof(verts),
            verts,
            GL_STATIC_DRAW)
        );
    CS237_CHECK( glVertexAttribPointer (0, 3, GL_FLOAT, GL_FALSE, 0, 0) );
    CS237_CHECK( glEnableVertexAttribArray (0) );

  //Draw
    glDrawArrays(GL_TRIANGLES, 0, 36);

  // clean up
    CS237_CHECK( glBindBuffer (GL_ARRAY_BUFFER, 0) );
    CS237_CHECK( glBindVertexArray (0) );
}

void View::Render (float dt)
{
    if (! this->_isVis)
        return;

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    //resource pass
    for(int i = 0; i < this->_map->nRows(); i++){
        for(int j = 0; j < this->_map->nCols(); j++){
            this->_map->Cell(i, j)->Tile(0).TileSet(2, this, dt,
                                                             this->_map->Cell(i, j)->ColorTQT(),
                                                             this->_map->Cell(i, j)->NormTQT());
        }
    }

    // set up detail textures
    if(!this->wireframeMode()){
      // use GL_TEXTURE2 because 0 and 1 are taken by color and normal textures
      CS237_CHECK(glActiveTexture(GL_TEXTURE2));
      //add detail textures
      this->_tCache->_GetDetailTex()->Bind();
    }

    //drawing pass
    for(int i = 0; i < this->_map->nRows(); i++){
        for(int j = 0; j < this->_map->nCols(); j++){

            cs237::vec3d nwCorner = this->_map->NWCellCorner(i,j);
            nwCorner = this->Camera().translate(nwCorner);
            cs237::vec3f floatCorner = cs237::vec3f((float)nwCorner[0],
                                                    (float)nwCorner[1],
                                                    (float)nwCorner[2]);
            if(this->wireframeMode())
              cs237::setUniform(this->wfOLoc, floatCorner);
            else{
              cs237::setUniform(this->tOLoc, floatCorner);

              if(this->_rainMode)
                cs237::setUniform(this->rainLoc, GL_TRUE);
              else
                cs237::setUniform(this->rainLoc, GL_FALSE);
            }

            this->_map->Cell(i, j)->Tile(0).DrawChunks(this, dt);
        }
    }

    //draw skybox
    if(!this->wireframeMode()){
      glDepthMask(GL_FALSE);
      glDepthFunc(GL_LEQUAL);
      this->drawSky();
      glDepthMask(GL_TRUE);
      this->textureshader->Use();
    }

    //rain loop
    if(this->_rainMode){
      this->rainshader->Use();
      cs237::setUniform(this->rViewMatLoc, this->_cam.viewTransform());
      cs237::setUniform(this->rProjMatLoc, this->_cam.projTransform());
      this->_map->drawRain(&this->_cam, dt);

      if(this->wireframeMode())
        this->wfshader->Use();
      else
        this->textureshader->Use();
    }

    glfwSwapBuffers (this->_window);

}

void Cell::InitTextures (View *view)
{
  // load textures
    if (this->_map->hasColorMap()) {
        this->_colorTQT = new TQT::TextureQTree (this->Datafile("/color.tqt").c_str());
    }
    if (this->_map->hasNormalMap()) {
        this->_normTQT = new TQT::TextureQTree (this->Datafile("/norm.tqt").c_str());
    }
#ifndef NDEBUG
    if ((this->_colorTQT != nullptr) && (this->_normTQT != nullptr)) {
        assert (this->_colorTQT->Depth() == this->_normTQT->Depth());
    }
#endif

}
