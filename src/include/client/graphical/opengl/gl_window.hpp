#pragma once

#include <client/graphical/opengl/gl_headers.hpp>

#ifdef RENDERER_OPENGL

#include <client/graphical/window.hpp>

#include <client/graphical/opengl/gl_device.hpp>
#include <client/graphical/framebuffer.hpp>
#include <client/graphical/shader.hpp>
#include <client/graphical/opengl/gl_gui_manager.hpp>

namespace familyline::graphics
{

    class GLWindow : public Window
    {
    private:
        GLDevice* _dev = nullptr;

        SDL_Window* _win      = nullptr;
        SDL_GLContext _glctxt = nullptr;
        int _width, _height;
        int _fwidth, _fheight;

        ShaderProgram* winShader = nullptr;
        GLuint base_vao, base_vbo, base_index_vbo;

        Framebuffer* _f3D;
        Framebuffer* _fGUI;

        void createWindowSquare();

        std::unique_ptr<Renderer> renderer_;
        std::unique_ptr<gui::GLGUIManager> guim_;
    
    public:
        GLWindow(GLDevice* dev, int width = 800, int height = 600);

        /* Get the window size, in pixels */
        virtual void getSize(int& width, int& height) const;

        /* Get the window framebuffer size
         *
         * This value can differ from the window size if the
         * OS do some sort of dpi scaling
         *
         * macOS is one example
         */
        virtual void getFramebufferSize(int& width, int& height);

        virtual void setFramebuffers(Framebuffer* f3D, Framebuffer* fGUI);
        virtual void show();
        virtual void update();

        virtual Renderer* createRenderer();
        virtual Renderer* getRenderer();

        virtual gui::GUIManager* createGUIManager();

        /**
         * Shows a message box (can be a system or an in-game message box,
         * depends on the renderer, but this function cannot cause an error)
         *
         * A message box will usually be used to report errors
         */
        virtual void showMessageBox(
            std::string title, SysMessageBoxFlags flags, std::string content);

        
        virtual ~GLWindow() { delete winShader; }
    };


}

#endif
