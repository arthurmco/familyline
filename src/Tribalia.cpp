
/***
    Tribalia main file

    Copyright 2016 Arthur M.

***/

#define GLM_FORCE_RADIANS


#include "EnviroDefs.h"
#include "config.h"

#include <cstdio>
#include <cstdlib>

#include <cstring>

#include "logic/ObjectRenderer.hpp"
#include "logic/PathFinder.hpp"
#include "logic/ObjectFactory.hpp"
#include "logic/ObjectPathManager.hpp"
#include "logic/TerrainFile.hpp"
#include "logic/Team.hpp"

#include "graphical/Renderer.hpp"
#include "graphical/GUIRenderer.hpp"
#include "graphical/ShaderProgram.hpp"
#include "graphical/Camera.hpp"
#include "graphical/AssetFile.hpp"
#include "graphical/Light.hpp"
#include "graphical/meshopener/OBJOpener.hpp"
#include "graphical/materialopener/MTLOpener.hpp"
#include "graphical/TerrainRenderer.hpp"
#include "graphical/TextureOpener.hpp"
#include "graphical/TextureManager.hpp"
#include "graphical/MaterialManager.hpp"
#include "graphical/AssetManager.hpp"
#include "graphical/Window.hpp"
#include "graphical/Framebuffer.hpp"
#include "graphical/AnimationManager.hpp"

#include "graphical/gui/Panel.hpp"
#include "graphical/gui/Label.hpp"
#include "graphical/gui/Button.hpp"

#include "input/InputPicker.hpp"

#include "Log.hpp"
#include "Timer.hpp"
#include "HumanPlayer.hpp"
#include "objects/WatchTower.hpp"
#include "objects/Tent.hpp"

#include <glm/gtc/matrix_transform.hpp> //glm::lookAt()

using namespace Tribalia;
using namespace Tribalia::Logic;
using namespace Tribalia::Graphics;
using namespace Tribalia::Graphics::GUI;
using namespace Tribalia::Input;

#ifdef _MSC_VER
    #undef main  //somehow vs does not find main()

#endif

static int get_arg_index(const char* name, int argc, char const* argv[])
{
	for (int i = 0; i < argc; i++) {
		if (!strcmp(argv[i], name)) {
			return i;
		}
	}

	return -1;
}

static void show_version()
{
	printf("Tribalia " VERSION "\n");
	printf("Compiled in " __DATE__ "\n");
	printf("Commit hash " COMMIT "\n");
	printf("\n");
}

static void show_help()
{
	printf("Tribalia help:\n");
	printf("--version:\tPrint version and, if compiled inside a Git repo, commit hash\n");
	printf("--help:\t\tPrint this help information\n");
	printf("--size <W>x<H>: Changes the game resolution to <W>x<H> pixels\n");
}

static int winW = 640, winH = 480;

static int check_size(int i, int argc, char const* argv[])
{
    if (i >= argc) {
	printf("size not defined. Expected <W>x<H> for size! Aborting");
	return -1;
    }
    
    if (sscanf(argv[i+1], "%dx%d", &winW, &winH) <= 1) {
	printf("size format is wrong. Expected <W>x<H> for size! Aborting...");
	return -1;
    }
    
    printf("pre-load: chosen %d x %d for screen size\n", winW, winH);
    return 0;
    
}

int main(int argc, char const *argv[])
{
    //int winW = 640, winH = 480;
	if (get_arg_index("--version",argc,argv) >= 0) {
	    show_version();
	    return EXIT_SUCCESS;
	}

	int i =  get_arg_index("--size", argc, argv);
	if (i >= 0) {
	    if( check_size(i, argc, argv) < 0) {
		return EXIT_FAILURE;
	    }
	}

	if (get_arg_index("--help", argc, argv) >= 0) {
		show_help();
		return EXIT_SUCCESS;
	}

	
    FILE* fLog = stderr;// fopen("log.txt", "w");
    Log::GetLog()->SetFile(fLog);
    Log::GetLog()->Write("", "Tribalia " VERSION);
    Log::GetLog()->Write("", "built on " __DATE__ " by " USERNAME);
#if defined(COMMIT)
    Log::GetLog()->Write("", "git commit is " COMMIT);
#endif

    Log::GetLog()->Write("", "Default model directory is " MODELS_DIR);
    Log::GetLog()->Write("", "Default texture directory is " TEXTURES_DIR);
    Log::GetLog()->Write("", "Default material directory is " MATERIALS_DIR);

    Framebuffer* fbGUI = nullptr;
    Framebuffer* fb3D = nullptr;
    Window* w = nullptr;
    GUIRenderer* guir = nullptr;
    try {
	w = new Window(winW, winH, WindowOptions::WIN_DEBUG_CONTEXT);
	Framebuffer::SetDefaultSize(winW, winH);
	w->Show();

	if (GLEW_ARB_debug_output) {
	    /* Create the callback */
	    __glewDebugMessageCallbackARB(
		[](GLuint source, GLuint type, unsigned int id, GLuint severity,
		   int length, const char* msg, const void* userparam){

		    (void) userparam;
		    
#define	DEBUG_SOURCE_API_ARB                              0x8246
#define	DEBUG_SOURCE_WINDOW_SYSTEM_ARB                    0x8247
#define	DEBUG_SOURCE_SHADER_COMPILER_ARB                  0x8248
#define	DEBUG_SOURCE_THIRD_PARTY_ARB                      0x8249
#define	DEBUG_SOURCE_APPLICATION_ARB                      0x824A
#define	DEBUG_SOURCE_OTHER_ARB                            0x824B
		    
		    const char *ssource, *stype, *sseverity;
		    switch (source) {
		    case DEBUG_SOURCE_API_ARB: ssource = "OpenGL API"; break;
		    case DEBUG_SOURCE_WINDOW_SYSTEM_ARB: ssource = "window system"; break;
		    case DEBUG_SOURCE_SHADER_COMPILER_ARB: ssource = "shader compiler"; break;
		    case DEBUG_SOURCE_THIRD_PARTY_ARB: ssource = "third party"; break;
		    case DEBUG_SOURCE_APPLICATION_ARB: ssource = "application"; break;
		    case DEBUG_SOURCE_OTHER_ARB: ssource = "other"; break;
		    default: ssource = "unknown"; break;
		    }

#define DEBUG_TYPE_ERROR_ARB                              0x824C
#define DEBUG_TYPE_DEPRECATED_BEHAVIOR_ARB                0x824D
#define DEBUG_TYPE_UNDEFINED_BEHAVIOR_ARB                 0x824E
#define DEBUG_TYPE_PORTABILITY_ARB                        0x824F
#define DEBUG_TYPE_PERFORMANCE_ARB                        0x8250
#define DEBUG_TYPE_OTHER_ARB                              0x8251

		    switch (type) {
		    case DEBUG_TYPE_ERROR_ARB: stype = "error"; break;
		    case DEBUG_TYPE_DEPRECATED_BEHAVIOR_ARB: stype = "deprecated behavior"; break;
		    case DEBUG_TYPE_UNDEFINED_BEHAVIOR_ARB: stype = "undefined behavior"; break;
		    case DEBUG_TYPE_PORTABILITY_ARB: stype = "portability issue"; break;
		    case DEBUG_TYPE_PERFORMANCE_ARB: stype = "performance"; break;
		    case DEBUG_TYPE_OTHER_ARB: stype = "other"; break;
		    default: stype = "unknown"; break;
		    }
		    
#define DEBUG_SEVERITY_HIGH_ARB                           0x9146
#define DEBUG_SEVERITY_MEDIUM_ARB                         0x9147
#define DEBUG_SEVERITY_LOW_ARB                            0x9148

		    switch (severity) {
		    case DEBUG_SEVERITY_HIGH_ARB: sseverity = "HIGH"; break;
		    case DEBUG_SEVERITY_MEDIUM_ARB: sseverity = "MEDIUM"; break;
		    case DEBUG_SEVERITY_LOW_ARB: sseverity = "LOW"; break;
		    default: sseverity = "????"; break;
		    }

		    char* m = new char[std::min(length*2, length+70)];

		    sprintf(m, "(%d) PRIO: %s (source: %s | type: %s) %s",
			    id, sseverity, ssource, stype, msg);

		    if (severity == DEBUG_SEVERITY_MEDIUM_ARB ||
			severity == DEBUG_SEVERITY_HIGH_ARB) {
			Log::GetLog()->Warning("gl-debug-output", m);
		    } else {
			Log::GetLog()->Write("gl-debug-output", m);
		    }

		    delete[] m;
		    
		}, nullptr);
	} else {
	    Log::GetLog()->Warning("init", "GLEW_ARB_debug_output not supported");
	}
	
	InputManager::GetInstance()->Initialize();

	fbGUI = new Framebuffer{ winW, winH, GL_UNSIGNED_BYTE };
	fb3D = new Framebuffer{ winW, winH, GL_UNSIGNED_BYTE };
	w->SetGUIFramebuffer(fbGUI);
	w->Set3DFramebuffer(fb3D);

	guir = new GUIRenderer{w};
	guir->SetFramebuffer(fbGUI);

    } catch (window_exception& we) {
	Log::GetLog()->Fatal("init", "Window creation error: %s (%d)", we.what(), we.code);
	exit(EXIT_FAILURE);
    } catch (shader_exception &se) {
	Log::GetLog()->Fatal("init", "Shader error: %s [%d]", se.what(), se.code);
	Log::GetLog()->Fatal("init", "Shader file: %s, type %d", se.file.c_str(), se.type);
	exit(EXIT_FAILURE);
    }

    /* Render the menu */
    bool r = true;
    auto deflistener = InputManager::GetInstance()->GetDefaultListener();
    int frames = 0;

    /* Lock frames to 60fps */
    double b = SDL_GetTicks();

    Label l = Label(0.37, 0.03, 0.25, 0.1, "TRIBALIA");
    l.SetForeColor(230, 240, 235, 255);
    l.SetFontData("Garamond", 32);

    Label lv = Label(0.35, 0.8, 0.4, 0.1, "Version " VERSION ", commit " COMMIT);
    lv.SetForeColor(255, 255, 255, 255);
    
    Button bnew = Button(0.1, 0.2, 0.8, 0.1, "New Game");
    Button bquit = Button(0.1, 0.31, 0.8, 0.1, "Exit Game");
    bquit.SetOnClickListener([&r](GUI::IControl* cc) {
	    r = false;
	});
    bnew.SetOnClickListener([&](GUI::IControl* cc) {
	    printf("New Game\n");
	});    
    
    guir->AddPanel(&l);
    guir->AddPanel(&lv);
    guir->AddPanel(&bnew);
    guir->AddPanel(&bquit);
    guir->InitInput();
    
    while (r) {

	// Input
	InputManager::GetInstance()->Run();
	InputEvent ev;
	guir->ProcessInput(ev);
	
	if (deflistener->PopEvent(ev)) {
	    /* Only listen for FINISH events.
	       The others will be handled by the GUI listener */
	    if (ev.eventType == EVENT_FINISH)
		r = false;
	}

	// Render
	fbGUI->SetAsBoth();
	guir->Render();
	fbGUI->Unset();
	
	w->Update();
	double e = SDL_GetTicks();
	
	if ((e-b) < 1000/60.0)
	    SDL_Delay((unsigned int)(1000/60.0 - (e-b)));
	
	b = SDL_GetTicks();
	
	frames++;
    }

    
    
    // ObjectManager* om = nullptr;
    // HumanPlayer* hp = nullptr;
    // Terrain* terr = nullptr;
    // TeamCoordinator* tc = nullptr;
    
    // Window* win = nullptr;
    // Renderer* rndr = nullptr;
    // SceneManager* scenemng = nullptr;

    // bool player = false;

    // Camera* cam;

    // AssetManager* am = AssetManager::GetInstance();
    // TerrainFile* terrFile;

    // /* GameContext gctx;
    // try {
    // 	/* Initialise things */
    //     om = new ObjectManager{};
    //     win = new Window{ winW, winH };

    //     Framebuffer::SetDefaultSize(winW, winH);
    //     win->Show();

    //     rndr = new Renderer{};

    //     gctx.om = om;

    //     hp = new HumanPlayer{"Arthur"};

    // 	terrFile = new TerrainFile(ASSET_FILE_DIR "terrain_test.trtb");
    // 	terr = terrFile->GetTerrain();

    // 	tc = new TeamCoordinator();
    // 	auto tteam = tc->CreateTeam("test");
    // 	printf("%s -- %#x\n", tteam->name.c_str(), tteam->id);
        

    // 	scenemng = new SceneManager(terr->GetWidth() * SEC_SIZE, terr->GetHeight() * SEC_SIZE);

    // 	cam = new Camera{glm::vec3(6.0f, 36.0f, 6.0f), (float)winW/(float)winH, glm::vec3(0,0,0)};
    //     scenemng->SetCamera(cam);
    //     hp->SetCamera(cam);

    //     rndr->SetSceneManager(scenemng);

    //     if (!am->ReadFromFile(ASSET_FILE_DIR "assets.taif")) {
    //         throw asset_exception(nullptr, "Invalid asset file!");
    //     }

    // } catch (window_exception& we) {
    // 	Log::GetLog()->Fatal("Window creation error: %s (%d)", we.what(), we.code);
    // 	exit(EXIT_FAILURE);
    // } catch (renderer_exception& re) {
    //     Log::GetLog()->Fatal("Rendering error: %s [%d]",
    //         re.what(), re.code);
    //     exit(EXIT_FAILURE);
    // }  catch (mesh_exception& se) {
    //     Log::GetLog()->Fatal("Mesh error: %s", se.what());
    //     Log::GetLog()->Fatal("Mesh file: %s", se.file.c_str());
    //     exit(EXIT_FAILURE);
    // }  catch (material_exception& se) {
    //     Log::GetLog()->Fatal("Material error: %s ", se.what());
    //     Log::GetLog()->Fatal("Material file: %s", se.file.c_str());
    //     exit(EXIT_FAILURE);
    // } catch (shader_exception& se) {
    //     Log::GetLog()->Fatal("Shader error: %s [%d]", se.what(), se.code);
    //     Log::GetLog()->Fatal("Shader file: %s, type %d", se.file.c_str(), se.type);
    //     exit(EXIT_FAILURE);
    // } catch (asset_exception& ae) {
    //     Log::GetLog()->Fatal("Asset file error: %s", ae.what());
    //     if (ae.assetptr) {
    //         AssetFileItem* a = (AssetFileItem*)ae.assetptr;
    //         Log::GetLog()->Fatal("Asset %s, file: %s", a->name.c_str(), a->path.c_str());
    //     }
    //     exit(EXIT_FAILURE);
    // } catch (terrain_file_exception& te) {
    // 	Log::GetLog()->Fatal("Terrain file error: %s on file %s", te.what(), te.file.c_str());
    // 	if (te.code != 0) {
    // 	    Log::GetLog()->Fatal("Error code: %d (%s)", te.code, strerror(te.code));
    // 	}
    // 	exit(EXIT_FAILURE);
    // }
   
    // Light* l = new Light{ "mainLight", glm::vec3(16, 30, 16), 0xff, 0xff, 0xff, 10 };
    // scenemng->AddObject(l);

    // TerrainRenderer* terr_rend = new TerrainRenderer{rndr};
    // terr_rend->SetTerrain(terr);
    // terr_rend->SetCamera(cam);

    // ObjectRenderer* objrend = new ObjectRenderer(om, scenemng);
    // hp->objr = objrend;

    // InputManager::GetInstance()->Initialize();

    // InputPicker* ip = new InputPicker{ terr_rend, win, scenemng, cam, om};
    // hp->SetPicker(ip);

    // PathFinder* pathf = new PathFinder(om);
    // pathf->InitPathmap(terr->GetWidth(), terr->GetHeight());
    // pathf->UpdatePathmap(terr->GetWidth(), terr->GetHeight());
    // hp->SetPathfinder(pathf);

    // unsigned int ticks = SDL_GetTicks();
    // unsigned int frame = 0;

    // int delta = 1;

    // Framebuffer fbRender{ winW, winH, GL_UNSIGNED_BYTE };
    // Framebuffer fbGUI{ winW, winH, GL_UNSIGNED_BYTE };
    // win->Set3DFramebuffer(&fbRender);
    // win->SetGUIFramebuffer(&fbGUI);
    
    // GUIRenderer gr{ win };
    // gr.SetFramebuffer(&fbGUI);

    // Panel p = Panel{0, 0, 320, 240};
    // p.SetBackColor(255, 0, 0, 25);
    // // gr.AddPanel(&p);

    // Label lbl = Label(120, 460, "This is a true label");
    // lbl.SetForeColor(255, 128, 0, 255);

    // Label lblVersion = Label(10, 10, "Tribalia " VERSION " commit " COMMIT);
    // lblVersion.SetForeColor(255, 255, 255, 255);

    // Panel pnl = Panel(0, 0, 630, 100);
    // pnl.AddPanel(&lblVersion, 10, 10);
    // pnl.SetBackColor(0, 0, 0, 128);

    // gr.AddPanel(&pnl);
    // gr.AddPanel(&lbl);

    // Button btn = Button(300, 200, 150, 40, "Test");
    // btn.SetOnClickListener([&](GUI::IControl* c) {
    // 	    static int times = 0;
    // 	    times++;

    // 	    char ss[8];
    // 	    snprintf(ss, 7, "%d", times);
    // 	    lbl.SetText(ss);
    // 	});
    // gr.AddPanel(&btn);

    // double pms = 0.0;
    
    // /* Adds the objects to the factory */
    // ObjectFactory::GetInstance()->AddObject(new WatchTower);
    // ObjectFactory::GetInstance()->AddObject(new Tent);

    // ObjectPathManager::getInstance()->SetTerrain(terr);
    // gr.InitInput();

    // CombatManager::GetInstance()->SetOnDeath([&gctx](AttackableObject* at) {
    // 	    gctx.om->UnregisterObject(at); 
    // 	});
    // do {

    //     ip->UpdateIntersectedObject();
    //     ip->UpdateTerrainProjectedPosition();

    // 	gr.DebugWrite(10, 40, "Press C to create an object at mouse cursor, and R to remove it.");
    //     player = true;
    //     gctx.elapsed_seconds = delta / 1000.0;

    // 	InputEvent ev;
    // 	gr.ProcessInput(ev);
    // 	hp->ProcessInput();
    //     if (!hp->Play(&gctx))
    //         player = false;
	
    // 	terr_rend->Update();

    //     bool objupdate = objrend->Check();
    //     if (objupdate || hp->HasUpdatedObject()) {
    //         objrend->Update();
    //         pathf->UpdatePathmap(terr->GetWidth(), terr->GetHeight());
    //     }
    // 	objrend->Update();

    //     LocatableObject* selected = hp->GetSelectedObject();
    //     auto locc = ip->GetIntersectedObject();
    //     if (locc) {
    //         gr.DebugWrite(10, 100, "Hovering object '%s'", locc->GetName());

    // 	    if (selected && locc->HasProperty("maxHP")) {
    // 		AttackableObject* a = (AttackableObject*)locc;
    // 		gr.DebugWrite(350, 100, a->CheckAttackRange((AttackableObject*)selected) ? "In range" : "Not in range");
    // 	    }
    // 	}

    //     {
    //         int qx, qy;
    //         scenemng->GetCameraQuadrant(qx, qy);
    //         gr.DebugWrite(10, 160, "Camera quadrant: %d x %d", qx, qy);
    //     }

    //     if (selected) {
    // 	    if (selected->HasProperty("maxHP")) {
    // 		AttackableObject* a = (AttackableObject*)selected;
    // 		gr.DebugWrite(10, 120, "Selected object: '%s' (%4d/%4d)",
    // 			      a->GetName(), (int)a->GetHP(), a->GetMaxHP());
	
    // 	    } else {
    // 		gr.DebugWrite(10, 120, "Selected object: '%s'",
    // 			      selected->GetName());
	      
    // 	    }	    
    //     }

    // 	CombatManager::GetInstance()->DoAttacks(gctx.elapsed_seconds);
    //     glm::vec3 p = ip->GetTerrainProjectedPosition();
    //     glm::vec2 q = ip->GetGameProjectedPosition();

    //     AnimationManager::GetInstance()->Iterate();
    // 	ObjectPathManager::getInstance()->UpdatePaths();

    // 	fbRender.SetAsBoth();
    // 	rndr->SetBoundingBox(hp->renderBBs);
    // 	if (objupdate) rndr->UpdateObjects();
    //     rndr->UpdateFrames();
    //     rndr->Render();

    // 	gr.DebugWrite(10, 140, "Terrain pos: (OpenGL: %.3f,%.3f,%.3f | Game: %.2f, %.2f)",
    // 		      p.x, p.y, p.z, q.x, q.y);
    // 	gr.DebugWrite(10, 180, "Camera rotation: %.1fº",
    // 		      cam->GetRotation() * 180 / M_PI);
    //     gr.DebugWrite(10, 65, "Bounding box: %s", hp->renderBBs ?
    //       "Enabled" : "Disabled");

    // 	fbGUI.SetAsBoth();
    // 	gr.Render();
    // 	fbGUI.Unset();
	
    // 	win->Update();

    //     frame++;

    //     unsigned int elapsed = SDL_GetTicks();
    //     delta = elapsed - ticks;

    //     ticks = SDL_GetTicks();
    //     Timer::getInstance()->RunTimers(delta);

    // 	if (frame % 15 == 0) {
    // 	    pms = delta * 1.0;
    // 	}

	
	
    // 	gr.DebugWrite(0, 420, "%.2f ms, %.2f fps", pms, 1000 / pms);

    // 	//Locked in ~60 fps
    //     if (delta < 1000/60.0) {
    // 	    SDL_Delay(1000/60.0 - delta);
    //     }

    // } while (player);

    printf("\nExited. (%d frames)\n", frames);

    return 0;
}
