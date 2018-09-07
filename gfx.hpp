#ifndef GFX_HPP
#define GFX_HPP

#define _USE_MATH_DEFINES
#include <SDL2/SDL.h>
#include <GL/glew.h>
#include <Eigen/Core>
#include <Eigen/Geometry>

class gfx
{
public:
	
	void init();
	void deinit();
	void render();
	void resize(int w, int h);
	int main_loop();
	
private:
	SDL_Window *window;
	SDL_GLContext context;
	int done;
	int win_w;
	int win_h;
	
	void print_info();
};

#endif
