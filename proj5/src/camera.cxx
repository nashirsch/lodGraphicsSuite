/*! \file camera.cxx
 *
 * \author John Reppy
 *
 * The camera class encapsulates the current view and projection matrices.
 */

/* CMSC23700 Final Project sample code (Autumn 2017)
 *
 * COPYRIGHT (c) 2017 John Reppy (http://cs.uchicago.edu/~jhr)
 * All rights reserved.
 */

#include "cs237.hxx"
#include "camera.hxx"

/***** class Camera member functions *****/

Camera::Camera ()
    : _errorFactor(-1)
{ }

// translate a point to the coordinate system that has the camera as the origin, but is
// oriented and scaled the same as the world coordinates.
cs237::vec3d Camera::translate (cs237::vec3d const &p) const
{
    return p - this->_pos;
}

// a transformation matrix that assumes that the camera is at the origin.
cs237::mat4x4f Camera::viewTransform () const
{
    return cs237::lookAt (
        cs237::vec3f(0.0f, 0.0f, 0.0f),
        this->_dir,
        this->_up);
}

// the projection transform for the camera
cs237::mat4x4f Camera::projTransform () const
{
    float n_e = this->_nearZ * tanf (this->_halfFOV);  // n/e
    return cs237::frustum(
        -n_e, n_e,
        -this->_aspect * n_e, this->_aspect * n_e,
        this->_nearZ, this->_farZ);
}

// update the camera for the aspect ratio of the given viewport.  This operation
// will change the aspect ratio, but not the field of view.
void Camera::setViewport (int wid, int ht)
{
    this->_errorFactor = -1.0f;  // mark the error factor as invalid
    this->_aspect = float(ht) / float(wid);
    this->_wid = wid;
}

// set the horizontal field of view in degrees
void Camera::setFOV (float angle)
{
    this->_errorFactor = -1.0f;
    this->_halfFOV = cs237::radians(0.5 * angle);
}

// set the near and far planes
void Camera::setNearFar (double nearZ, double farZ)
{
    assert ((0.0 < nearZ) && (nearZ < farZ));
    this->_nearZ = nearZ;
    this->_farZ = farZ;
}

// move the camera to a new position while maintaining its heading
void Camera::move (cs237::vec3d const &pos)
{
    this->_pos = pos;
}

// move the camera to a new position and heading, while maintaining
// its up vector
void Camera::move (cs237::vec3d const &pos, cs237::vec3d const &at)
{
    this->_pos = pos;
    this->_dir = normalize(cs237::toFloat(at - pos));
}

// move the camera to a new position, heading, and up vector
void Camera::move (cs237::vec3d const &pos, cs237::vec3d const &at, cs237::vec3d const &up)
{
    this->_pos = pos;
    this->_dir = cs237::toFloat(normalize(at - pos));
    this->_up = cs237::toFloat(normalize(up));
}

// change the direction of the camera
void Camera::look (cs237::vec3f const &dir)
{
    this->_dir = normalize(dir);
}

// change the direction of the camera
void Camera::look (cs237::vec3f const &dir, cs237::vec3f const &up)
{
    this->_dir = normalize(dir);
    this->_up = up;
}

// compute the screen-space error for a geometric error of size err at distance dist.
float Camera::screenError (float dist, float err) const
{
    if (this->_errorFactor < 0.0f) {
        this->_errorFactor = float(this->_wid) / (2.0 * tanf(this->_halfFOV));
    }
    return this->_errorFactor * (err / dist);

}

// rotate the camera around the right vector
void Camera::pitch(float degrees){
    cs237::vec3f right = cross(this->direction(), this->_up);
    cs237::mat4x4f r = cs237::rotate(degrees, right);

    this->_up = cs237::vec3f(normalize(r * cs237::vec4f(this->_up, 0)));
    this->_dir = cs237::vec3f(normalize(r * cs237::vec4f(this->_dir, 0)));
}

// rotate the camera around the up vector
void Camera::yaw(float degrees){
    cs237::mat4x4f r = cs237::rotate(degrees, this->_up);

    this->_dir = cs237::vec3f(normalize(r * cs237::vec4f(this->_dir, 0)));
}

// rotate the camera around the direction vector
void Camera::roll(float degrees){
    cs237::mat4x4f r = cs237::rotate(degrees, this->_dir);

    this->_up = cs237::vec3f(normalize(r * ( cs237::vec4f(this->_up, 0))));
}

// move the camera left and right
void Camera::lateral(double step){
    cs237::vec3f rightf = normalize(cross(this->direction(), this->_up));
    cs237::vec3d rightd = normalize(cs237::toDouble(rightf));

    this->_pos = this->_pos + step*rightd;
}

// move the camera forwards and backwards
void Camera::longitudinal(double step){
    cs237::vec3d dird = normalize(cs237::toDouble(this->_dir));

    this->_pos = this->_pos + step*dird;
}

/***** Output *****/


Frustum::Frustum(){
}

// compute the new plane data given a camera state
void Frustum::updateFrustum(Camera* cam){

  double hclose = 2.0 * tan((cam->aspect() * cam->HalfFOV())) * (double)cam->near();
  double wclose = hclose * (1.0/cam->aspect());

  double hfar = 2.0 * tan((cam->aspect() * cam->HalfFOV())) * cam->far();
  double wfar = hfar * (1.0/cam->aspect());

  cs237::vec3d nearcenter = cam->position() + normalize(cs237::toDouble(cam->direction())) * (double)cam->near();
  cs237::vec3d farcenter = cam->position() + normalize(cs237::toDouble(cam->direction())) * (double)cam->far();

  cs237::vec3d right = normalize(cs237::__detail::cross(cs237::toDouble(cam->direction()), cs237::toDouble(cam->up())));
  cs237::vec3d up = normalize(cs237::toDouble(cam->up()));

  //near plane bounding points
  //start at top left corner, going clockwise
  cs237::vec3d near[4] = {nearcenter + (up * hclose/2.0) - (right * wclose/2.0),
                          nearcenter + (up * hclose/2.0) + (right * wclose/2.0),
                          nearcenter - (up * hclose/2.0) + (right * wclose/2.0),
                          nearcenter - (up * hclose/2.0) - (right * wclose/2.0)};

  //far plane bounding points
  //start at top left corner, going clockwise
  cs237::vec3d far[4] = {farcenter + (up * hfar/2.0) - (right * wfar/2.0),
                         farcenter + (up * hfar/2.0) + (right * wfar/2.0),
                         farcenter - (up * hfar/2.0) + (right * wfar/2.0),
                         farcenter - (up * hfar/2.0) - (right * wfar/2.0)};

  //planes of the view frustum
  // indexes:
  //    0: top plane
  //    1: right plane
  //    2: bottom plane
  //    3: left plane
  //    4: near plane
  //    5: far plane

  //top plane
  normals[0] = normalize(cs237::__detail::cross((far[0] - near[0]), (near[1] - near[0])) +
                         0.18 * cs237::toDouble(cam->direction()));
  distances[0] = -cs237::__detail::dot(normals[0], near[0]);

  //right plane
  normals[1] = normalize(cs237::__detail::cross((far[1] - near[1]), (near[2] - near[1])) +
                         0.18 * cs237::toDouble(cam->direction()));
  distances[1] = -cs237::__detail::dot(normals[1], near[1]);

  //bottom plane
  normals[2] = normalize(cs237::__detail::cross((far[2] - near[2]), (near[3] - near[2])) +
                         0.18 * cs237::toDouble(cam->direction()));
  distances[2] = -cs237::__detail::dot(normals[2], near[2]);

  //left plane
  normals[3] = normalize(cs237::__detail::cross((far[3] - near[3]), (near[0] - near[3])) +
                         0.18 * cs237::toDouble(cam->direction()));
  distances[3] = -cs237::__detail::dot(normals[3], near[3]);

  //near plane
  normals[4] = normalize(cs237::__detail::cross((near[3] - near[2]), (near[1] - near[2])));
  distances[4] = -cs237::__detail::dot(normals[4], near[0]);

  //far plane
  normals[5] = normalize(cs237::__detail::cross((far[3] - far[0]), (far[1] - far[0])));
  distances[5] = -cs237::__detail::dot(normals[5], far[0]);

}


std::ostream& operator<< (std::ostream& s, Camera const &cam)
{
    s << "Camera {"
        << "\n  position =  " << cam.position()
        << "\n  direction = " << cam.direction()
        << "\n  up =        " << cam.up()
        << "\n  near Z =    " << cam.near()
        << "\n  far Z =     " << cam.far()
        << "\n  aspect =    " << cam.aspect()
        << "\n  fov =       " << cam.fov()
        << "\n}";

    return s;
}
