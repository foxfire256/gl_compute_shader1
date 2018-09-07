#include "gfx.hpp"

#include <iostream>

#include <GL/glu.h>

#include "fox/counter.hpp"
#include "fox/gfx/eigen_opengl.hpp"

#ifdef _WIN32
std::string data_root = "C:/dev/gl_compute_shader1";
#else // Linux
std::string data_root = "/home/foxfire/dev/gl_compute_shader1";
#endif

#define print_opengl_error() print_opengl_error2((char *)__FILE__, __LINE__)
int print_opengl_error2(char *file, int line);

gfx::gfx()
{
	this->generator = std::mt19937_64(std::random_device{}());
}

void gfx::init()
{
	done = 0;
	int ret;
	std::string window_title = "OpenGL Compute Shader 1";
	win_w = 768;
	win_h = 768;
	
	ret = SDL_Init(SDL_INIT_VIDEO);
	if(ret < 0)
	{
		printf("Unable to init SDL: %s\n", SDL_GetError());
		exit(1);
	}
	
	window = SDL_CreateWindow(
		window_title.c_str(),
		SDL_WINDOWPOS_UNDEFINED,
		SDL_WINDOWPOS_UNDEFINED,
		win_w,
		win_h,
		SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE
		);
	
	if(!window)
	{
		printf("Couldn't create window: %s\n", SDL_GetError());
		SDL_Quit();
		exit(-1);
	}
	
	SDL_ShowWindow(window);
	
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);
	SDL_GL_SetAttribute(SDL_GL_RETAINED_BACKING, 1);
	
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);
	
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,
						SDL_GL_CONTEXT_PROFILE_CORE);
	//SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,
	//	SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);
	
	context = SDL_GL_CreateContext(window);
	
	ret = SDL_GL_MakeCurrent(window, context);
	if(ret)
	{
		printf("ERROR could not make GL context current after init!\n");
		if(window)
			SDL_DestroyWindow(window);
		
		SDL_Quit();
		exit(1);
	}
	
	// 0 = no vsync
	SDL_GL_SetSwapInterval(0);
	
	std::cout << "Running on platform: " << SDL_GetPlatform() << std::endl;
	std::cout << "Number of logical CPU cores: " << SDL_GetCPUCount() << std::endl;
	int ram_mb = SDL_GetSystemRAM();
	char buffer[8];
	snprintf(buffer, 8, "%.1f", ram_mb / 1024.0f);
	std::cout << "System RAM " << ram_mb << "MB (" << buffer << " GB)\n";
	
	// OpenGL init
	// init glew first
	glewExperimental = GL_TRUE; // Needed in core profile
	if(glewInit() != GLEW_OK)
	{
		printf("Failed to initialize GLEW\n");
		exit(-1);
	}
	
	// HACK: to get around initial glew error with core profiles
	GLenum gl_err = glGetError();
	while(gl_err != GL_NO_ERROR)
	{
		//printf("glError in file %s @ line %d: %s (after glew init)\n",
		//	(char *)__FILE__, __LINE__, gluErrorString(gl_err));
		gl_err = glGetError();
	}
	
	print_opengl_error();
	
	print_info();

	GLint val;
	glGetIntegerv(GL_MAX_VERTEX_UNIFORM_BLOCKS, &val);
	printf("GL_MAX_VERTEX_UNIFORM_BLOCKS: %i\n", val);
	glGetIntegerv(GL_MAX_FRAGMENT_UNIFORM_BLOCKS, &val);
	printf("GL_MAX_FRAGMENT_UNIFORM_BLOCKS: %i\n", val);
	glGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE, &val);
	printf("GL_MAX_UNIFORM_BLOCK_SIZE: %i\n", val);
	printf("This means a max of %i objects\n", val / 4 / 16);

	print_opengl_error();

	// init basic OpenGL stuff
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

	// TODO: may want depth test off and blending off
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_BLEND); // alpha channel
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glEnable(GL_POLYGON_SMOOTH);
	glHint(GL_POLYGON_SMOOTH_HINT, GL_NICEST);

	glPointSize(1.5f);

	// OpenGL 3.2 core requires a VAO to be bound to use a VBO
	// WARNING: GLES 2.0 does not support VAOs
	glGenVertexArrays(1, &default_vao);
	glBindVertexArray(default_vao);

	// initialize some defaults
	eye = Eigen::Vector3f(0.0f, 0.0f, 10.0f);
	target = Eigen::Vector3f(0.0f, 0.0f, 0.0f);
	up = Eigen::Vector3f(0.0f, 1.0f, 0.0f);

	fox::gfx::look_at(eye, target, up, V);
	fox::gfx::perspective(65.0f, (float)win_w / (float)win_h, 0.01f, 40.0f, P);

	print_opengl_error();

	obj_count = 64;
	x[0].resize(obj_count);
	x[1].resize(obj_count);
	v[0].resize(obj_count);
	v[1].resize(obj_count);
	a[0].resize(obj_count);
	a[1].resize(obj_count);
	m.resize(obj_count);

	float mass_range[2];
	float distance_range[2];

	// random init stuff
	std::uniform_real_distribution<float> dist_m(mass_range[0],
		mass_range[1]);
	std::uniform_real_distribution<float> dist_d(distance_range[0],
		distance_range[1]);

	for(uint16_t i = 0; i < obj_count; i++)
	{
		m[i] = dist_m(generator);

		x[0][i] = Eigen::Vector3f(dist_d(generator),
			dist_d(generator),
			dist_d(generator));

		v[0][i] = Eigen::Vector3f(0.0, 0.0, 0.0);
		a[0][i] = Eigen::Vector3f(0.0, 0.0, 0.0);
	}

	glGenBuffers(1, &x_vbo_0);
	glBindBuffer(GL_ARRAY_BUFFER, x_vbo_0);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * obj_count * 3,
		x[0].data(), GL_STATIC_DRAW);
	glGenBuffers(1, &x_vbo_1);
	glBindBuffer(GL_ARRAY_BUFFER, x_vbo_1);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * obj_count * 3,
		x[1].data(), GL_STATIC_DRAW);
	glGenBuffers(1, &v_vbo_0);
	glBindBuffer(GL_ARRAY_BUFFER, v_vbo_0);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * obj_count * 3,
		v[0].data(), GL_STATIC_DRAW);
	glGenBuffers(1, &v_vbo_1);
	glBindBuffer(GL_ARRAY_BUFFER, v_vbo_1);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * obj_count * 3,
		v[1].data(), GL_STATIC_DRAW);
	glGenBuffers(1, &a_vbo_0);
	glBindBuffer(GL_ARRAY_BUFFER, a_vbo_0);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * obj_count * 3,
		a[0].data(), GL_STATIC_DRAW);
	glGenBuffers(1, &a_vbo_1);
	glBindBuffer(GL_ARRAY_BUFFER, a_vbo_1);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * obj_count * 3,
		a[1].data(), GL_STATIC_DRAW);

	glGenBuffers(1, &m_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * obj_count,
		m.data(), GL_STATIC_DRAW);

	print_opengl_error();
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	print_opengl_error();
	fflush(stdout);
}

void gfx::deinit()
{
	// TODO: should we really use swap to force a deallocation and is this the
	// right way to do it?

	std::vector<Eigen::Vector3f> n0, n1;
	x[0].clear();
	x[1].clear();
	x[0].swap(n0);
	x[1].swap(n1);

	std::vector<Eigen::Vector3f> n2, n3;
	v[0].clear();
	v[1].clear();
	v[0].swap(n2);
	v[1].swap(n3);

	std::vector<Eigen::Vector3f> n4, n5;
	a[0].clear();
	a[1].clear();
	a[0].swap(n4);
	a[1].swap(n5);

	std::vector<float> n7;
	m.clear();
	m.swap(n7);

	SDL_GL_DeleteContext(context);
	SDL_DestroyWindow(window);
	
	SDL_Quit();
}

void gfx::render()
{
	int status = SDL_GL_MakeCurrent(window, context);
	if(status)
	{
		printf("SDL_GL_MakeCurrent() failed in render(): %s\n",
			   SDL_GetError());
		SDL_GL_DeleteContext(context);
		SDL_DestroyWindow(window);
		
		SDL_Quit();
		exit(1);
	}
	
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	SDL_GL_SwapWindow(window);
}

void gfx::resize(int w, int h)
{
	win_w = w;
	win_h = h;
	
	glViewport(0, 0, win_w, win_h);
}

int gfx::main_loop()
{
	SDL_Event event;
	while(SDL_PollEvent(&event))
	{
		switch(event.type)
		{
			case SDL_KEYDOWN:
				if(event.key.keysym.sym == SDLK_ESCAPE)
				{
					done = 1;
				}
				break;
			case SDL_KEYUP:
				break;
			case SDL_QUIT:
				done = 1;
				break;
			case SDL_WINDOWEVENT:
				switch (event.window.event)
				{
					case SDL_WINDOWEVENT_CLOSE:
						done = 1;
						break;
					case SDL_WINDOWEVENT_RESIZED:
						resize(event.window.data1, event.window.data2);
						break;
					default:
						break;
				}
			default:
				break;
		}
	}
	return done;
}

void gfx::print_info()
{
	printf("GLEW library version %s\n", glewGetString(GLEW_VERSION));
	
	if(glewIsSupported("GL_VERSION_5_1"))
	{
		printf("GLEW supported GL_VERSION_5_1\n");
	}
	else if(glewIsSupported("GL_VERSION_5_0"))
	{
		printf("GLEW supported GL_VERSION_5_0\n");
	}
	else if(glewIsSupported("GL_VERSION_4_8"))
	{
		printf("GLEW supported GL_VERSION_4_8\n");
	}
	else if(glewIsSupported("GL_VERSION_4_7"))
	{
		printf("GLEW supported GL_VERSION_4_7\n");
	}
	else if(glewIsSupported("GL_VERSION_4_6"))
	{
		printf("GLEW supported GL_VERSION_4_6\n");
	}
	else if(glewIsSupported("GL_VERSION_4_5"))
	{
		printf("GLEW supported GL_VERSION_4_5\n");
	}
	else if(glewIsSupported("GL_VERSION_4_4"))
	{
		printf("GLEW supported GL_VERSION_4_4\n");
	}
	else if(glewIsSupported("GL_VERSION_4_3"))
	{
		printf("GLEW supported GL_VERSION_4_3\n");
	}
	else if(glewIsSupported("GL_VERSION_4_2"))
	{
		printf("GLEW supported GL_VERSION_4_2\n");
	}
	else if(glewIsSupported("GL_VERSION_4_1"))
	{
		printf("GLEW supported GL_VERSION_4_1\n");
	}
	else if(glewIsSupported("GL_VERSION_4_0"))
	{
		printf("GLEW supported GL_VERSION_4_0\n");
	}
	else if(glewIsSupported("GL_VERSION_3_2"))
	{
		printf("GLEW supported GL_VERSION_3_2\n");
	}
	else if(glewIsSupported("GL_VERSION_3_1"))
	{
		printf("GLEW supported GL_VERSION_3_1\n");
	}
	else if(glewIsSupported("GL_VERSION_3_0"))
	{
		printf("GLEW supported GL_VERSION_3_0\n");
	}
	else if(glewIsSupported("GL_VERSION_2_1"))
	{
		printf("GLEW supported GL_VERSION_2_1\n");
	}
	else if(glewIsSupported("GL_VERSION_2_0"))
	{
		printf("GLEW supported GL_VERSION_2_0\n");
	}
	else
	{
		printf("NO GLEW GL_VERSION seems to be supported!\n");
	}
	
	printf("GL_VENDOR: %s\n", glGetString(GL_VENDOR));
	printf("GL_RENDERER: %s\n", glGetString(GL_RENDERER));
	printf("GL_VERSION: %s\n", glGetString(GL_VERSION));
	printf("GL_SHADING_LANGUAGE_VERSION: %s\n",
		   glGetString(GL_SHADING_LANGUAGE_VERSION));
	
	printf("Eigen version: %d.%d.%d\n", EIGEN_WORLD_VERSION,
		   EIGEN_MAJOR_VERSION,  EIGEN_MINOR_VERSION);
	
	
}

//------------------------------------------------------------------------------
// Returns 1 if an OpenGL error occurred, 0 otherwise.
int print_opengl_error2(char *file, int line)
{
	GLenum gl_err;
	int	ret_code = 0;
	
	gl_err = glGetError();
	while(gl_err != GL_NO_ERROR)
	{
		printf("glError in file %s @ line %d: %s\n", file, line,
			gluErrorString(gl_err));
		
		ret_code = 1;
		gl_err = glGetError();
	}
	return ret_code;
}
