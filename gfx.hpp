#ifndef GFX_HPP
#define GFX_HPP

#define _USE_MATH_DEFINES
#include <vector>
#include <random>
#include <SDL2/SDL.h>
#include <GL/glew.h>
#include <Eigen/Core>
#include <Eigen/Geometry>

namespace fox
{
	class counter;
}

class gfx
{
public:

	gfx();
	
	void init();
	void deinit();
	void render();
	void resize(int w, int h);
	int main_loop();
	
private:
	fox::counter *fps_counter;
	SDL_Window *window;
	SDL_GLContext context;
	int done;
	int win_w;
	int win_h;
	
	void print_info();

	/**
	 * @brief Position (x1, x2, x3), two vectors for new and old
	 */
	std::vector<Eigen::Vector3f> x[2];
	/**
	 * @brief Velocity, two vectors for new and old
	 */
	std::vector<Eigen::Vector3f> v[2];
	/**
	 * @brief Acceleration, two vectors for new and old
	 */
	std::vector<Eigen::Vector3f> a[2];
	/**
	 * @brief Object mass
	 */
	std::vector<float> m;

	/**
	 * @brief A random generator that is initialized in the constructor
	 */
	std::mt19937_64 generator;

	double G = 6.67408e-11;
	uint16_t obj_count;

	// an empty vertex array object to bind to
	uint32_t default_vao;
	Eigen::Vector3f eye, target, up;
	Eigen::Affine3f V;
	Eigen::Projective3f P;

	GLuint gfx_shader_id, shader_vert_id, shader_frag_id, comp_shader_id;
	GLuint x_vbo_0, x_vbo_1, v_vbo_0, v_vbo_1, a_vbo_0, a_vbo_1, m_vbo;
};

#endif
