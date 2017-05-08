#include "GUIRenderer.hpp"

using namespace Tribalia::Graphics;

/* Game window width and height */
int win_w, win_h;

/* Great square, used to show the DebugWrite() messages */
static const float debug_msg_panel_pos[][3] =
{
    {-1, -1, 1}, { 1, -1, 1}, { 1, 1, 1},
    {-1, -1, 1}, {-1,  1, 1}, { 1, 1, 1}
};

/* Coordinates for every panel texture.
   Since they have the same vertex order, we don't need to declare multiple
   texture coordinates
*/
static const float panel_texture_coord[][2] =
{ {-1, -1}, {1, -1}, {1, 1}, {-1, -1}, {-1, 1}, {1, 1}};


GUIRenderer::GUIRenderer(Window* w)
{
    /* Get window size */
    w->GetSize(win_w, win_h);

    /* Compile the GUI shader */
    auto sFrag = new Shader{"shaders/GUI.frag", SHADER_PIXEL};
    auto sVert = new Shader{"shaders/GUI.vert", SHADER_VERTEX};
    if (!sFrag->Compile()) {
        throw shader_exception("GUI shader failed to compile", glGetError(),
            sFrag->GetPath(), sFrag->GetType());
    }

    if (!sVert->Compile()) {
        throw shader_exception("GUI shader failed to compile", glGetError(),
            sVert->GetPath(), sVert->GetType());
    }

    sGUI = new ShaderProgram{sVert, sFrag};
    if (!sGUI->Link()) {
        char shnum[6];
        sprintf(shnum, "%d", sGUI->GetID());
        throw shader_exception("GUI shader failed to link", glGetError(),
            shnum, SHADER_PROGRAM);
    }



    /* Create a panel for debug messages */
	
    _w = w;
}

bool surfaceChanged = false;

void GUIRenderer::DebugWrite(int x, int y, const char* fmt, ...)
{
    return;
    
    char* ch = new char[512];
    va_list vl;
    va_start(vl, fmt);
    vsprintf(ch, fmt, vl);
    va_end(vl);
   
   
    printf("dw (%d %d): %s\n", x, y, ch);
    delete[] ch;
/*
	cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 1.0);
	cairo_select_font_face(cr, "monospace", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
	cairo_set_font_size(cr, 14.0);
	cairo_move_to(cr, x*1.0, y*1.0);
	cairo_show_text(cr, ch);
	surfaceChanged = true;
	delete[] ch; */
}

void GUIRenderer::SetFramebuffer(Framebuffer* f) {
	_f = f;
	Log::GetLog()->Write("[GUIRenderer] Framebuffer now it's the texture #%d", f->GetTextureHandle());

}

bool fb_setup = false;
bool GUIRenderer::Render()
{
    glClearColor(0.0, 0.0, 0.0, 0.0);
    sGUI->Use();
    //glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    this->Redraw(nullptr);
    glClearColor(0.0, 0.0, 0.0, 1.0);
    
    return true;
}

/* Redraw the child controls */
void GUIRenderer::Redraw(cairo_t* ctxt)
{
    glActiveTexture(GL_TEXTURE0);
    sGUI->SetUniform("texPanel", 0);

    for (auto& p : _panels) {
	//p.panel->Redraw(p.ctxt);
	cairo_surface_flush(p.csurf);
	
	glBindVertexArray(p.vao);

	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, p.vbo_vert);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

	glEnableVertexAttribArray(1);
	glBindBuffer(GL_ARRAY_BUFFER, p.vbo_tex);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 0);

	glBindTexture(GL_TEXTURE_2D, p.tex_id);
/*	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, p.pw, p.ph, 0, GL_BGRA,
		     GL_UNSIGNED_INT_8_8_8_8_REV,
		     (void*)cairo_image_surface_get_data(p.csurf)); */
	
	glDrawArrays(GL_TRIANGLES, 0, 6);

	GLenum err = glGetError();
	if (err != GL_NO_ERROR) {
	    Log::GetLog()->Fatal("OpenGL error %#x", err);
	}

    }

    glBindVertexArray(0);
    glDisableVertexAttribArray(0);
}

/* Add a panel using the panel position or a new position */
 int GUIRenderer::AddPanel(GUI::IPanel* p)
 {
	 Log::GetLog()->Write("GUIRenderer: Added panel (%#p)", p);

	 /* Create panel vertices */
	 int px, py, pw, ph;
	 p->GetBounds(px, py, pw, ph);

	 float relx = float(px)/win_w, rely = float(py)/win_h;
	 float relw = float(pw)/win_w, relh = float(ph)/win_h;
	 
	 printf(" panel relpos %.2f %.2f %.2f %.2f\n", relx, rely, relw, relh);

	 /* Create the panel vertices */
	 float win_vectors[][3] =
	 {
	     {relx, 1.0f-(rely+relh), 0.9f}, {relx+relw, 1.0f-(rely+relh), 0.9f},
	     {relx+relw, 1.0f-(rely), 0.9f},
	     {relx, 1.0f-(rely+relh), 0.9f}, {relx, 1.0f-rely, 0.9f},
	     {relx+relw, 1.0f-rely, 0.9f}
	 };
	 
	 PanelRenderObject pro;

	 glGenVertexArrays(1, &(pro.vao));
	 glBindVertexArray(pro.vao);

	 /* Create vertex information */
	 glGenBuffers(1, &(pro.vbo_vert));
	 glBindBuffer(GL_ARRAY_BUFFER, pro.vbo_vert);
	 glBufferData(GL_ARRAY_BUFFER, sizeof(win_vectors), win_vectors, GL_STATIC_DRAW);
	 glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
	 glEnableVertexAttribArray(0);

	 glGenBuffers(1, &(pro.vbo_tex));
	 glBindBuffer(GL_ARRAY_BUFFER, pro.vbo_tex);
	 glBufferData(GL_ARRAY_BUFFER, sizeof(panel_texture_coord),
		      panel_texture_coord, GL_STATIC_DRAW);
	 glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);
	 glEnableVertexAttribArray(1);

	 glBindVertexArray(0);

	 /* Create cairo context and surface for each panel */
	 pro.csurf = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, pw, ph);
	 pro.ctxt = cairo_create(pro.csurf);

	 /* Clean the surface with transparent color */
	 cairo_set_source_rgba(pro.ctxt, 0, 0, 0, 0);
	 cairo_rectangle(pro.ctxt, 0, 0, 1.0, 1.0);
	 cairo_fill(pro.ctxt);
	 cairo_surface_flush(pro.csurf);   

	 
	 /* Generate textures */
	 glGenTextures(1, &(pro.tex_id));
	 glBindTexture(GL_TEXTURE_2D, pro.tex_id);

	 unsigned int* c = new unsigned int[pw*ph];
	 for (int i = 0; i < pw*ph; i++)
	     c[i] = 0xffffffff;
	 glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, pw, ph, 0, GL_BGRA,
		      GL_UNSIGNED_INT_8_8_8_8_REV, c);

       	
	 glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	 glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	 glBindTexture(GL_TEXTURE_2D, 0);

	 pro.pw = pw;
	 pro.ph = ph;
	 
	 /* Upload panel */
	 pro.panel = p;
	 _panels.push_back(pro);
	 
	 return 1;
 }

 int GUIRenderer::AddPanel(GUI::IPanel* p, int x, int y)
 {
	 p->SetPosition(x, y);
	 return AddPanel(p);
 }

/* Remove the panel */
 void GUIRenderer::RemovePanel(GUI::IPanel* p)
 {

 }

 void GUIRenderer::SetBounds(int x, int y, int w, int h) {}
 void GUIRenderer::SetPosition(int x, int y) {}
