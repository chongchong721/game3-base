#include "PlayMode.hpp"

#include "LitColorTextureProgram.hpp"

#include "DrawLines.hpp"
#include "Mesh.hpp"
#include "Load.hpp"
#include "gl_errors.hpp"
#include "data_path.hpp"

#include <glm/gtc/type_ptr.hpp>

#include <random>

GLuint meshes_for_lit_color_texture_program = 0;
Load< MeshBuffer > meshes(LoadTagDefault, []() -> MeshBuffer const * {
	MeshBuffer const *ret = new MeshBuffer(data_path("./level.pnct"));
	meshes_for_lit_color_texture_program = ret->make_vao_for_program(lit_color_texture_program->program);
	return ret;
});

Load< Scene > level(LoadTagDefault, []() -> Scene const * {
	return new Scene(data_path("./level.scene"), [&](Scene &scene, Scene::Transform *transform, std::string const &mesh_name){
		Mesh const &mesh = meshes->lookup(mesh_name);

		if(mesh_name!="marksphere"){
			scene.drawables.emplace_back(transform);
			Scene::Drawable &drawable = scene.drawables.back();

			drawable.pipeline = lit_color_texture_program_pipeline;

			drawable.pipeline.vao = meshes_for_lit_color_texture_program;
			drawable.pipeline.type = mesh.type;
			drawable.pipeline.start = mesh.start;
			drawable.pipeline.count = mesh.count;
		}
	});
});

Load< Sound::Sample > c2_sample(LoadTagDefault, []() -> Sound::Sample const * {
	return new Sound::Sample(data_path("c2.wav"));
});

Load< Sound::Sample > e2_sample(LoadTagDefault, []() -> Sound::Sample const * {
	return new Sound::Sample(data_path("e2.wav"));
});

Load< Sound::Sample > g2_sample(LoadTagDefault, []() -> Sound::Sample const * {
	return new Sound::Sample(data_path("g2.wav"));
});

Load< Sound::Sample > c3_sample(LoadTagDefault, []() -> Sound::Sample const * {
	return new Sound::Sample(data_path("c3.wav"));
});

Load< Sound::Sample > bounce_sample(LoadTagDefault, []() -> Sound::Sample const * {
	return new Sound::Sample(data_path("bounce.wav"));
});

Load< Sound::Sample > target_sample(LoadTagDefault, []() -> Sound::Sample const * {
	return new Sound::Sample(data_path("music.wav"));
});



PlayMode::PlayMode() : scene(*level) {

	scene.bboxes.clear();
	for (auto &mesh_object: meshes->meshes){
		std::string name = mesh_object.first;

		// Bouding boxes for objects
		if(name.find("Cube")!=std::string::npos){
			scene.bboxes.emplace_back();
			scene.bboxes.back().name = name;
			scene.bboxes.back().min = mesh_object.second.min;
			scene.bboxes.back().max = mesh_object.second.max;
		}
		// boundary
		else if(name.find("wall")!=std::string::npos){
			scene.bboxes.emplace_back();
			scene.bboxes.back().name = name;
			scene.bboxes.back().min = mesh_object.second.min;
			scene.bboxes.back().max = mesh_object.second.max;

			boundary.xmin = std::min(boundary.xmin,mesh_object.second.min.x);
			boundary.xmax = std::max(boundary.xmax,mesh_object.second.max.x);
			boundary.ymin = std::min(boundary.ymin,mesh_object.second.min.y);
			boundary.ymax = std::max(boundary.ymax,mesh_object.second.max.y);
		}


	}









	//get pointers to ball for convenience:
	for (auto &transform : scene.transforms) {
		if (transform.name == "Sphere") ball_transform = &transform;
		else if (transform.name == "Cube_target") target_transform = &transform;
	}

	ball_translation = ball_transform->position;

	{
		auto ball = meshes->lookup("Sphere");
		auto d = ball.max - ball.min;
		ball_metadata.radius = d.x * ball_transform->scale.x / 2;
		ball_metadata.original_position = ball_translation;
		ball_metadata.current_position = ball_translation;
		ball_metadata.world_speed = glm::vec3(0.0f);
	}

	{
		auto target = meshes->lookup("Cube_target");
		auto position =  (target.max + target.min);
		target_position = glm::vec3(position.x / 2, position.y / 2, position.z / 2);
	}

	{
		marksphere = meshes->lookup("Marksphere");
		blacksphere = meshes->lookup("Sphereblack");

		for(auto &it : scene.drawables){
			if (it.transform->name == "Sphere"){
				ball = &it;
				break;
			}
		}

	}


	//get pointer to camera for convenience:
	if (scene.cameras.size() != 1) throw std::runtime_error("Expecting scene to have exactly one camera, but it has " + std::to_string(scene.cameras.size()));
	camera = &scene.cameras.front();

	//start music loop playing:
	// (note: position will be over-ridden in update())
	//leg_tip_loop = Sound::loop_3D(*dusty_floor_sample, 1.0f, get_leg_tip_position(), 10.0f);

	target_loop = Sound::loop_3D(*target_sample,0.0f,get_target_position(),10.0f);
	c2_onetime = Sound::loop_3D(*c2_sample,0.0f,get_ball_position(),3.0f);
	e2_onetime = Sound::loop_3D(*e2_sample,0.0f,get_ball_position(),3.0f);
	g2_onetime = Sound::loop_3D(*g2_sample,0.0f,get_ball_position(),3.0f);
	c3_onetime = Sound::loop_3D(*c3_sample,0.0f,get_ball_position(),3.0f);


	listenmode = MODEXY;
}

PlayMode::~PlayMode() {
}

bool PlayMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) {

	if (evt.type == SDL_KEYDOWN) {
		if (evt.key.keysym.sym == SDLK_ESCAPE) {
			SDL_SetRelativeMouseMode(SDL_FALSE);
			return true;
		} else if (evt.key.keysym.sym == SDLK_a) {
			left.downs += 1;
			left.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_d) {
			right.downs += 1;
			right.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_w) {
			back.downs += 1;
			back.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_s) {
			forward.downs += 1;
			forward.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_2){
			modex.downs += 1;
			modex.pressed = true;
			return true;
		}else if (evt.key.keysym.sym == SDLK_3){
			modey.downs += 1;
			modey.pressed = true;
			return true;
		}
		else if (evt.key.keysym.sym == SDLK_4){
			modetarget.downs += 1;
			modetarget.pressed = true;
			return true;
		}else if (evt.key.keysym.sym == SDLK_1){
			modexy.downs += 1;
			modexy.pressed = true;
			return true;
		}
	} else if (evt.type == SDL_KEYUP) {
		if (evt.key.keysym.sym == SDLK_a) {
			left.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_d) {
			right.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_w) {
			back.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_s) {
			forward.pressed = false;
			return true;
		}else if (evt.key.keysym.sym == SDLK_2){
			modex.pressed = false;
			return true;
		}else if (evt.key.keysym.sym == SDLK_3){
			modey.pressed = false;
			return true;
		}else if (evt.key.keysym.sym == SDLK_4){
			modetarget.pressed = false;
			return true;
		}else if (evt.key.keysym.sym == SDLK_1){
			modexy.pressed = false;
			return true;
		}
	}
	

	return false;
}

void PlayMode::update(float elapsed) {


	{
		if(modex.pressed && !modey.pressed && !modetarget.pressed && !modexy.pressed) listenmode = MODEX;
		if(!modex.pressed && modey.pressed && !modetarget.pressed && !modexy.pressed) listenmode = MODEY;
		if(!modex.pressed && !modey.pressed && modetarget.pressed && !modexy.pressed) listenmode = MODETARGET;
		if(!modex.pressed && !modey.pressed && !modetarget.pressed && modexy.pressed) listenmode = MODEXY;
	}

	// 	camera->transform->position += move.x * frame_right + move.y * frame_forward;
	// }
	//move ball:
	{



		if (left.pressed && !right.pressed) ball_metadata.world_speed.y -= 0.02;
		if (!left.pressed && right.pressed) ball_metadata.world_speed.y += 0.02;
		if (forward.pressed && !back.pressed) ball_metadata.world_speed.x += 0.02;
		if (!forward.pressed && back.pressed) ball_metadata.world_speed.x -= 0.02;

		// absolute speed
		auto speed_total = std::sqrt(ball_metadata.world_speed.x * ball_metadata.world_speed.x + ball_metadata.world_speed.y * ball_metadata.world_speed.y);

		const float speed_max = 1.0;
		// max speed is 2
		if (speed_total > speed_max){
			ball_metadata.world_speed = ball_metadata.world_speed / speed_total * speed_max;
		}

		
	}

	handle_ball(elapsed);

	if(ball_metadata.current_position.x < boundary.xmax &&
	   ball_metadata.current_position.x > boundary.xmin &&
	   ball_metadata.current_position.y < boundary.ymax &&
	   ball_metadata.current_position.y > boundary.ymin)
	{
		ball->pipeline.start = blacksphere.start;
		ball->pipeline.count = blacksphere.count;
	}

	if(success){
		end = std::chrono::system_clock::now();
		elapsed_time = end - *start;
		if(elapsed_time.count() > exit_timer){
			Mode::set_current(nullptr);
		}
	}

	// auto stop_sound = [](std::shared_ptr<Sound::PlayingSample> sound){
	// 	if(sound!=nullptr){
	// 		if (!sound->stopped)
	// 			sound->stop();
	// 	}
	// };

	// auto is_zero = [](glm::vec3 speed)->bool{
	// 	if (speed.x == 0.0 && speed.y == 0.0 && speed.z == 0.0)
	// 		return true;
	// 	else
	// 		return false;
	// };

	{
		switch (listenmode)
		{
		case MODETARGET:
		{
			const float max_dist = 10.0f;
			auto ball_pos = ball_metadata.current_position;
			auto target_pos = target_transform->position;

			auto dist = glm::distance(ball_pos,target_pos);
			c2_onetime->set_volume(0.0f);
			g2_onetime->set_volume(0.0f);
			e2_onetime->set_volume(0.0f);
			c3_onetime->set_volume(0.0f);
			if (dist > max_dist)
				target_loop->set_volume(0.0);
			else
				target_loop->set_volume(2.0f * (max_dist-dist) / max_dist);
			{
				auto ball_pos = ball_metadata.current_position;
				auto speed = ball_metadata.world_speed;
				//Rotation Matrix of 90 degree is [[0,1],[-1,0]]
				glm::vec3 right = glm::vec3(speed.y,-speed.x,0);
				Sound::listener.set_position_right(ball_pos,right);
			}
			break;
		}
			
		case MODEX:
		{
			auto speed = ball_metadata.world_speed;
			c2_onetime->set_position(get_ball_position());
			g2_onetime->set_position(get_ball_position());
			e2_onetime->set_position(get_ball_position());
			c3_onetime->set_position(get_ball_position());
			target_loop->set_volume(0.0f);

			if (speed.x < 0.0){
				c2_onetime->set_volume(std::abs(speed.x));
				g2_onetime->set_volume(0);
			}else{
				g2_onetime->set_volume(std::abs(speed.x));
				c2_onetime->set_volume(0);
			}
			e2_onetime->set_volume(0.0f);
			c3_onetime->set_volume(0.0f);
			{ //update listener to camera position:
				glm::mat4x3 frame = camera->transform->make_local_to_parent();
				glm::vec3 frame_right = frame[0];
				glm::vec3 frame_at = frame[3];
				Sound::listener.set_position_right(frame_at, frame_right, 1.0f / 60.0f);
			}
			break;
		}
			


		case MODEY:
		{
			auto speed = ball_metadata.world_speed;
			c2_onetime->set_position(get_ball_position());
			g2_onetime->set_position(get_ball_position());
			e2_onetime->set_position(get_ball_position());
			c3_onetime->set_position(get_ball_position());
			target_loop->set_volume(0.0f);

			if (speed.y < 0.0){
				e2_onetime->set_volume(std::abs(speed.y));
				c3_onetime->set_volume(0);
			}else{
				c3_onetime->set_volume(std::abs(speed.y));
				e2_onetime->set_volume(0);
			}
			c2_onetime->set_volume(0.0f);
			g2_onetime->set_volume(0.0f);
			{ //update listener to camera position:
				glm::mat4x3 frame = camera->transform->make_local_to_parent();
				glm::vec3 frame_right = frame[0];
				glm::vec3 frame_at = frame[3];
				Sound::listener.set_position_right(frame_at, frame_right, 1.0f / 60.0f);
			}
			break;
		}

		case MODEXY:
		{
			auto speed = ball_metadata.world_speed;
			c2_onetime->set_position(get_ball_position());
			g2_onetime->set_position(get_ball_position());
			e2_onetime->set_position(get_ball_position());
			c3_onetime->set_position(get_ball_position());
			target_loop->set_volume(0.0f);

			if (speed.y < 0.0){
				e2_onetime->set_volume(std::abs(speed.y));
				c3_onetime->set_volume(0);
			}else{
				c3_onetime->set_volume(std::abs(speed.y));
				e2_onetime->set_volume(0);
			}
			if (speed.x < 0.0){
				c2_onetime->set_volume(std::abs(speed.x));
				g2_onetime->set_volume(0);
			}else{
				g2_onetime->set_volume(std::abs(speed.x));
				c2_onetime->set_volume(0);
			}
			{ //update listener to camera position:
				glm::mat4x3 frame = camera->transform->make_local_to_parent();
				glm::vec3 frame_right = frame[0];
				glm::vec3 frame_at = frame[3];
				Sound::listener.set_position_right(frame_at, frame_right, 1.0f / 60.0f);
			}
			break;
		}

			
		default:
			break;
		}

	}

	//reset button press counters:
	left.downs = 0;
	right.downs = 0;
	back.downs = 0;
	forward.downs = 0;
	modex.downs = 0;
	modey.downs = 0;
	modetarget.downs = 0;

}

void PlayMode::draw(glm::uvec2 const &drawable_size) {
	//update camera aspect ratio for drawable:
	camera->aspect = float(drawable_size.x) / float(drawable_size.y);

	//set up light type and position for lit_color_texture_program:
	// TODO: consider using the Light(s) in the scene to do this
	glUseProgram(lit_color_texture_program->program);
	glUniform1i(lit_color_texture_program->LIGHT_TYPE_int, 1);
	glUniform3fv(lit_color_texture_program->LIGHT_DIRECTION_vec3, 1, glm::value_ptr(glm::vec3(0.0f, 0.0f,-1.0f)));
	glUniform3fv(lit_color_texture_program->LIGHT_ENERGY_vec3, 1, glm::value_ptr(glm::vec3(1.0f, 1.0f, 0.95f)));
	glUseProgram(0);

	glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
	glClearDepth(1.0f); //1.0 is actually the default value to clear the depth buffer to, but FYI you can change it.
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS); //this is the default depth comparison function, but FYI you can change it.
	GL_ERRORS();
	scene.draw(*camera);

	{ //use DrawLines to overlay some text:
		glDisable(GL_DEPTH_TEST);
		float aspect = float(drawable_size.x) / float(drawable_size.y);
		DrawLines lines(glm::mat4(
			1.0f / aspect, 0.0f, 0.0f, 0.0f,
			0.0f, 1.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f
		));

		std::string text;

		if(!success){
			text = "WASD gives the ball speed; 1234 changes listening mode";
		}else{
			char out[100];
			auto length = sprintf(out,"Congrats! You Win! Game will end in %.1f seconds",exit_timer - elapsed_time.count());
			text = std::string(out,length);
		}

		constexpr float H = 0.09f;
		lines.draw_text(text,
			glm::vec3(-aspect + 0.1f * H, -1.0 + 0.1f * H, 0.0),
			glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
			glm::u8vec4(0x00, 0x00, 0x00, 0x00));
		float ofs = 2.0f / drawable_size.y;
		lines.draw_text(text,
			glm::vec3(-aspect + 0.1f * H + ofs, -1.0 + + 0.1f * H + ofs, 0.0),
			glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
			glm::u8vec4(0xff, 0xff, 0xff, 0x00));
	}

}

// glm::vec3 PlayMode::get_leg_tip_position() {
// 	//the vertex position here was read from the model in blender:
// 	return lower_leg->make_local_to_world() * glm::vec4(-1.26137f, -11.861f, 0.0f, 1.0f);
// }


glm::vec3 PlayMode::get_ball_position(){
	return ball_transform->position;
}

glm::vec3 PlayMode::get_target_position(){
	return target_position;
}




// All colllision check happens in "PLANE SPACE"
// block_location is indicating what value the sphere center's location should be set to
std::array<std::pair<bool,bool>,3> PlayMode::ball_collision_status(std::array<std::pair<float,float>,3> & block_location){
	auto ball_pos = ball_metadata.current_position;
	auto radius = ball_metadata.radius;

	// Use sphere-AABB collision test?
	// ret_val : [0] true,collision
	// https://developer.mozilla.org/en-US/docs/Games/Techniques/3D_collision_detection
	auto check_collision = [ball_pos,radius](Scene::Bbox bbox)-> std::array<std::pair<bool,bool>,3>{
		float x = std::max(bbox.min.x,std::min(bbox.max.x,ball_pos.x));
		float y = std::max(bbox.min.y,std::min(bbox.max.y,ball_pos.y));
		float z = std::max(bbox.min.z,std::min(bbox.max.z,ball_pos.z));

		auto close_to = [](float x,float x0)->bool{
			if((x - x0) > -0.01 && (x - x0) < 0.01 )
				return true;
			else
				return false;
		};

		auto distance = std::sqrt(
			(x-ball_pos.x) * (x-ball_pos.x) +
			(y-ball_pos.y) * (y-ball_pos.y) +
			(z-ball_pos.z) * (z-ball_pos.z)
		);

		std::array<std::pair<bool,bool>,3>ret{std::make_pair(false,false)};

		if(distance > radius + 0.05){
			return ret;
		}else{
			// Check the collision direction
			// Six possibility
			if(close_to(x,bbox.max.x)){
				ret[0].first = true;
				ret[0].second = true;
			}else if(close_to(x,bbox.min.x)){
				ret[0].first = true;
				ret[0].second = false;				
			}else if(close_to(y,bbox.max.y)){
				ret[1].first = true;
				ret[1].second = true;
			}else if(close_to(y,bbox.min.y)){
				ret[1].first = true;
				ret[1].second = false;
			}else if(close_to(z,bbox.max.z)){
				ret[2].first = true;
				ret[2].second = true;
			}else if(close_to(z,bbox.min.z)){
				ret[2].first = true;
				ret[2].second = false;
			}else{
				std::runtime_error("Collision bug");
			}

			return ret;
		}
	};

	std::array<std::pair<bool,bool>,3> collision_flag{std::make_pair(false,false)};

	for(auto bbox : scene.bboxes){
		auto ret = check_collision(bbox);
		if(ret[0].first){
			if(!collision_flag[0].first){
				if(bbox.name == "Cube_target"){
					success = true;
					if(start == nullptr){
						start = new std::chrono::time_point<std::chrono::system_clock>;
						*start = std::chrono::system_clock::now();
					}
				}
				collision_flag[0].first = true;
				collision_flag[0].second = ret[0].second;
				//set blocked location
				if(collision_flag[0].second){
					block_location[0].first = bbox.max.x + radius + 0.02;
				}else{
					block_location[0].second = bbox.min.x - radius - 0.02;
				}
			}
		}
		if(ret[1].first){
			if(!collision_flag[1].first){
				if(bbox.name == "Cube_target"){
					success = true;
					if(start == nullptr){
						start = new std::chrono::time_point<std::chrono::system_clock>;
						*start = std::chrono::system_clock::now();
					}
				}
				collision_flag[1].first = true;
				collision_flag[1].second = ret[1].second;
				if(collision_flag[1].second){
					block_location[1].first = bbox.max.y + radius + 0.02;
				}else{
					block_location[1].second = bbox.min.y - radius - 0.02;
				}
			}
		}
		if(ret[2].first){
			if(!collision_flag[2].first){
				collision_flag[2].first = true;
				collision_flag[2].second = ret[2].second;
				if(collision_flag[2].second){
					block_location[2].first = bbox.max.z + radius + 0.02;
				}else{
					block_location[2].second = bbox.min.z - radius - 0.02;
				}
			}
		}
	}

	return collision_flag;
}




void PlayMode::handle_ball(float elapsed){


	std::array<std::pair<float,float>,3> blocked_location{std::make_pair(-100.f,-100.0f)};

	auto collision_status = ball_collision_status(blocked_location);

	// Update location and speed using backward Euler?
	// Assuming there is no block
	// https://gamedev.stackexchange.com/questions/112892/is-this-a-correct-backward-euler-implementation
	// previous plane-space location
	auto x0 = ball_metadata.current_position.x;
	auto y0 = ball_metadata.current_position.y;
	auto z0 = ball_metadata.current_position.z;
	// previous plane-space speed
	auto vx0 = ball_metadata.world_speed.x;
	auto vy0 = ball_metadata.world_speed.y;
	auto vz0 = ball_metadata.world_speed.z;
	// updated plane-space location
	auto x = x0 + vx0 * elapsed;
	auto y = y0 + vy0 * elapsed;
	auto z = z0 + vz0 * elapsed;
	// updated plane-space speed
	auto vx = vx0;
	auto vy = vy0;
	auto vz = vz0;


	// // Bounce the speed when touching the wall
	// auto bounce = [collision_status](glm::vec3 speed)->glm::vec3{

	// };
	float bounce_factor = 0.75f;



	
	// If the direction is blocked, the location should be set to close the wall's location

	if(collision_status[0].first){ 
		if(collision_status[0].second){ // x- is blocked
			if(x < x0){
				//x = x0;
				x = blocked_location[0].first;
				add_mark_drawable(marksphere);
				if(vx0 < 0){
					bounce_onetime = Sound::play_3D(*bounce_sample,1.0f,get_ball_position(),3.0f);
					vx = -vx0 * bounce_factor;
				}

			}
		}else{ // x+ is blocked
			if(x > x0){
				x = blocked_location[0].second;
				add_mark_drawable(marksphere);
				if(vx0 > 0){
					bounce_onetime = Sound::play_3D(*bounce_sample,1.0f,get_ball_position(),3.0f);
					vx = -vx0 * bounce_factor;
				}
			}
		}
	}

	if(collision_status[1].first){ 
		if(collision_status[1].second){ // y- is blocked
			if(y < y0){
				y = blocked_location[1].first;
				add_mark_drawable(marksphere);
				if(vy0 < 0){
					bounce_onetime = Sound::play_3D(*bounce_sample,1.0f,get_ball_position(),3.0f);
					vy = -vy0 * bounce_factor;
				}

			}
		}else{ // y+ is blocked
			if(y > y0){
				y = blocked_location[1].second;
				add_mark_drawable(marksphere);
				if(vy0 > 0){
					bounce_onetime = Sound::play_3D(*bounce_sample,1.0f,get_ball_position(),3.0f);
					vy = -vy0 * bounce_factor;
				}
			}
		}
	}	
			

	// Should never get here?
	if(collision_status[2].first){ 
		if(collision_status[2].second){ // z- is blocked
			if(z < z0){
				z = blocked_location[2].first;
				add_mark_drawable(marksphere);
				if(vz0 > 0){
					bounce_onetime = Sound::play_3D(*bounce_sample,1.0f,get_ball_position(),3.0f);
					vz = -vz0 * bounce_factor;
				}

			}
		}else{ // z+ is blocked
			if(z > z0){
				z = blocked_location[2].second;
				add_mark_drawable(marksphere);
				if(vz0 > 0){
					bounce_onetime = Sound::play_3D(*bounce_sample,1.0f,get_ball_position(),3.0f);
					vz = -vz0 * bounce_factor;
				}
			}
		}
	}

	

	ball_metadata.current_position = glm::vec3(x,y,z);
	ball_metadata.world_speed = glm::vec3(vx,vy,vz);




	// If ball is too far, return it to original position
	if(
		ball_metadata.current_position.x < -12 || 
		ball_metadata.current_position.x > 12  ||
		ball_metadata.current_position.y < -12 ||
		ball_metadata.current_position.y > 12  ||
		ball_metadata.current_position.z > 10  ||
		ball_metadata.current_position.z < -10
	){
		//Reset
		ball_metadata.current_position = ball_metadata.original_position;
		ball_metadata.world_speed = glm::vec3{0.0f};
	}

	ball_transform->position = ball_metadata.current_position;

	// Is this needed here?
	//update_ball_to_plane();

	return;

}


void PlayMode::add_mark_drawable(Mesh mesh){

	Scene::Transform * new_point_transform = new Scene::Transform();
	new_point_transform->name = std::string("mark") + std::to_string(nmark++);
	new_point_transform->position = get_ball_position();
	//new_point_transform->position.z = 2.5;
	new_point_transform->scale = glm::vec3(0.15f);

	scene.drawables.emplace_back(new_point_transform);
	Scene::Drawable &drawable = scene.drawables.back();
	drawable.pipeline = lit_color_texture_program_pipeline;
	drawable.pipeline.vao = meshes_for_lit_color_texture_program;
	drawable.pipeline.type = mesh.type;
	drawable.pipeline.start = mesh.start;
	drawable.pipeline.count = mesh.count;
}