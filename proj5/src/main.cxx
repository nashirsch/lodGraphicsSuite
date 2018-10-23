/*! \file main.cxx
 *
 * \author John Reppy
 */

/* CMSC23700 Final Project sample code (Autumn 2017)
 *
 * COPYRIGHT (c) 2017 John Reppy (http://cs.uchicago.edu/~jhr)
 * All rights reserved.
 */

#include "cs237.hxx"
#include "map.hxx"
#include "map-cell.hxx"
#include "view.hxx"
#include <unistd.h>

/***** callbacks *****
 *
 * These callback functions are wrappers for the methods of the View class
 */

/*! \brief Window resize callback.
 *  \param wid the new width of the window.
 *  \param ht the new height of the window.
 */
void Reshape (GLFWwindow *win, int wid, int ht)
{
    reinterpret_cast<View *>(glfwGetWindowUserPointer(win))->Resize (wid, ht);

} /* end of Reshape */

/*! \brief Keyboard-event callback.
 *  \param win the window receiving the event
 *  \param key The keyboard code of the key
 *  \param scancode The system-specific scancode of the key.
 *  \param action GLFW_PRESS, GLFW_RELEASE or GLFW_REPEAT.
 *  \param mods the state of the keyboard modifier keys.
 */
void Key (GLFWwindow *win, int key, int scancode, int action, int mods)
{
    if (action == GLFW_RELEASE)
        return;

    reinterpret_cast<View *>(glfwGetWindowUserPointer(win))->HandleKey (key, mods);

} /* Key */

/*! \brief The mouse enter/leave callback function
 */
void MouseEnter (GLFWwindow *win, int entered)
{
    reinterpret_cast<View *>(glfwGetWindowUserPointer(win))->HandleMouseEnter(entered != 0);

}

/*! \brief The mouse motion callback function
 */
void MouseMotion (GLFWwindow *win, double xPos, double yPos)
{
    reinterpret_cast<View *>(glfwGetWindowUserPointer(win))->HandleMouseMove(xPos, yPos);

}

/*! \brief The mouse button callback function
 */
void MouseButton (GLFWwindow *win, int button, int action, int mods)
{
    reinterpret_cast<View *>(glfwGetWindowUserPointer(win))->HandleMouseButton(button, action, mods);

}

/*! \brief Visibility-change callback: sets the visibility state of the view.
 *  \param state the current state of the window; GL_TRUE for iconified, GL_FALSE otherwise.
 */
void Visible (GLFWwindow *win, int state)
{
    reinterpret_cast<View *>(glfwGetWindowUserPointer(win))->SetVisible (state == GL_TRUE);

} /* end of Visible */

//! main entrypoint
//
int main (int argc, const char **argv)
{
    Map map;


  // get the mapfile
    if (argc < 2) {
        std::cerr << "usage: proj5 <map-dir>\n";
        return 1;
    }
    std::string mapDir(argv[1]);
    std::clog << "loading " << mapDir << std::endl;
    if (! map.LoadMap (mapDir)) {
        return 1;
    }

    std::clog << "loading cells\n";
    for (int r = 0;  r < map.nRows(); r++) {
        for (int c = 0;  c < map.nCols();  c++) {
            map.Cell(r, c)->Load();
        }
    }

    std::clog << "initializing view\n";
    View *view = new View (&map);
    view->Init (1024, 768);

  // initialize the callback functions
    glfwSetWindowSizeCallback (view->Window(), Reshape);
    glfwSetWindowIconifyCallback (view->Window(), Visible);
    glfwSetKeyCallback (view->Window(), Key);
    glfwSetCursorEnterCallback (view->Window(), MouseEnter);
    glfwSetCursorPosCallback (view->Window(), MouseMotion);
    glfwSetMouseButtonCallback (view->Window(), MouseButton);

  // we keep track of the time between frames for morphing and for
  // any time-based animation
    double lastFrameTime = glfwGetTime();

  // user interface
    printf("\n\n~~~~~~~~~~~~~~~~~~~~~~~");
    printf("\nUser Interface\n\n");
    printf("Up/Down: look up/down\n");
    printf("Left/Right: look left/right\n");
    printf("B/N: 'roll' the camera\n");
    printf("Y/H: move forwards/backwards\n");
    printf("G/J: move left/right\n");
    printf("W: toggle wireframe mode\n");
    printf("L: toggle lighting\n");
    printf("F: toggle fog\n");
    printf("R: toggle rain\n");
    printf("+/-: increase/decrease error tolerance\n");
    printf("~~~~~~~~~~~~~~~~~~~~~~~\n\n");

    while (! view->shouldClose()) {
      // how long since last frame?
        double now = glfwGetTime();
        float dt = float(now - lastFrameTime);
        lastFrameTime = now;

        glfwPollEvents (); // Poll Events to get smoother movement

        if(glfwGetKey(view->Window(), GLFW_KEY_Y) == GLFW_PRESS)
          Key(view->Window(), GLFW_KEY_Y, 0, GLFW_PRESS, 1);
        else if(glfwGetKey(view->Window(), GLFW_KEY_H) == GLFW_PRESS)
          Key(view->Window(), GLFW_KEY_H, 0, GLFW_PRESS, 1);

        if(glfwGetKey(view->Window(), GLFW_KEY_G) == GLFW_PRESS)
          Key(view->Window(), GLFW_KEY_G, 0, GLFW_PRESS, 1);
        else if(glfwGetKey(view->Window(), GLFW_KEY_J) == GLFW_PRESS)
          Key(view->Window(), GLFW_KEY_J, 0, GLFW_PRESS, 1);

        if(glfwGetKey(view->Window(), GLFW_KEY_UP) == GLFW_PRESS)
          Key(view->Window(), GLFW_KEY_UP, 0, GLFW_PRESS, 1);
        else if(glfwGetKey(view->Window(), GLFW_KEY_DOWN) == GLFW_PRESS)
          Key(view->Window(), GLFW_KEY_DOWN, 0, GLFW_PRESS, 1);

        if(glfwGetKey(view->Window(), GLFW_KEY_LEFT) == GLFW_PRESS)
          Key(view->Window(), GLFW_KEY_LEFT, 0, GLFW_PRESS, 1);
        else if(glfwGetKey(view->Window(), GLFW_KEY_RIGHT) == GLFW_PRESS)
          Key(view->Window(), GLFW_KEY_RIGHT, 0, GLFW_PRESS, 1);

        if(glfwGetKey(view->Window(), GLFW_KEY_B) == GLFW_PRESS)
          Key(view->Window(), GLFW_KEY_B, 0, GLFW_PRESS, 1);
        else if(glfwGetKey(view->Window(), GLFW_KEY_N) == GLFW_PRESS)
          Key(view->Window(), GLFW_KEY_N, 0, GLFW_PRESS, 1);

        view->Render (dt);

        view->Animate (now);

    }

    glfwTerminate ();

    return EXIT_SUCCESS;

}
