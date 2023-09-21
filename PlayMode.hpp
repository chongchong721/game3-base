#include "Mode.hpp"

#include "Scene.hpp"
#include "Sound.hpp"
#include "Mesh.hpp"

#include <glm/glm.hpp>

#include <vector>
#include <deque>

#include <chrono>

struct PlayMode : Mode {
	PlayMode();
	virtual ~PlayMode();

	//functions called by main loop:
	virtual bool handle_event(SDL_Event const &, glm::uvec2 const &window_size) override;
	virtual void update(float elapsed) override;
	virtual void draw(glm::uvec2 const &drawable_size) override;


	// Ball update
	std::array<std::pair<bool,bool>,3> ball_collision_status(std::array<std::pair<float,float>,3> & block_location);
	void handle_ball(float elapsed);
	glm::vec3 get_ball_position();
	glm::vec3 get_target_position();

	// Add some mark
	void add_mark_drawable(Mesh mesh);
	uint32_t nmark = 0;

	//----- game state -----

	//input tracking:
	struct Button {
		uint8_t downs = 0;
		uint8_t pressed = 0;
	} left, right, forward, back, modey,modex,modetarget,modexy;

	enum{
		MODEX,
		MODEY,
		MODETARGET,
		MODEXY
	}listenmode;

	struct{
		float xmin = 10000; // back
		float xmax = -10000; // front
		float ymin = 10000; // left
		float ymax = -10000; // right
	}boundary;



	struct{
		glm::vec3 original_position; // original position in world space
		//glm::vec3 original_position_local; // original position in plane space // No need to use local space in this game
		glm::vec3 current_position;	// current position in world space
		//glm::vec3 current_position_local; // current position in plane space
		float radius;
		//Axie aligned bbox in world space
		struct{
			glm::vec3 min;
			glm::vec3 max;
		} bbox_world;
		//Axis aligned bbox in plane space
		// struct{
		// 	glm::vec3 min;
		// 	glm::vec3 max;
		// }bbox_plane;
		glm::vec3 world_speed{0.0f};
		//glm::vec3 plane_speed{0.0f};
	}ball_metadata;
	glm::vec3 ball_translation;


	//local copy of the game scene (so code can change it during gameplay):
	Scene scene;


	Scene::Transform *ball_transform = nullptr;
	Scene::Transform *target_transform = nullptr;
	glm::vec3 target_position;

	Mesh marksphere;
	Mesh blacksphere;
	Scene::Drawable *ball = nullptr;



	std::shared_ptr<Sound::PlayingSample> c2_onetime;
	std::shared_ptr<Sound::PlayingSample> e2_onetime;
	std::shared_ptr<Sound::PlayingSample> g2_onetime;
	std::shared_ptr<Sound::PlayingSample> c3_onetime;
	std::shared_ptr<Sound::PlayingSample> bounce_onetime;
	std::shared_ptr<Sound::PlayingSample> target_loop;

	std::list<std::shared_ptr<Sound::PlayingSample>> speed_tones;
	
	//camera:
	Scene::Camera *camera = nullptr;




	// Success flag
	bool success = false;
	float exit_timer = 5.0f;
	std::chrono::time_point<std::chrono::system_clock> * start = nullptr;
	std::chrono::time_point<std::chrono::system_clock> end;
	std::chrono::duration<double> elapsed_time;



};
