#include "include.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


void Engine::init(void *param1, void *param2)
{
	float ident[16] = {	1.0f, 0.0f, 0.0f, 0.0f,
						0.0f, 1.0f, 0.0f, 0.0f,
						0.0f, 0.0f, 1.0f, 0.0f,
						0.0f, 0.0f, 0.0f, 1.0f};

	Engine::param1 = param1;
	Engine::param2 = param2;

	identity = ident;
	projection = ident;
	transformation = ident;


	//visual
	gfx.init(param1, param2);
	no_tex = load_texture(gfx, "media/notexture.tga");
	Model::CreateObjects(gfx);
	global.init(&gfx);

	//audio
	audio.init();
	menu.init(&gfx, &audio);
	menu.load("media/newmenu.txt", "media/newstate.txt");
	printf("altEngine2 Version %s\n", "1.0.0");
	GLenum err = glGetError();
	if ( err != GL_NO_ERROR)
	{
		printf("Fatal GL_ERROR %d after setting up opengl context\n", err);
		return;
	}

	menu.render(global);

	//net crap
	sequence = 0;
	server = false;
	client = false;
	memset(reliable.msg, 0, LINE_SIZE);
	reliable.sequence = -1;
	last_server_sequence = 0;
	spawn = 0;

	//md5 crap
	frame_step = 0;
	printf("Loading md5\n");
	md5.load_md5("zcc.md5mesh");
	md5.load_md5_animation("chaingun_idle.md5anim");
	// get bind pose vertex positions

	for(int i = 0; i < md5.model->num_mesh; i++)
	{
		md5.PrepareMesh(i, md5.model->joint, md5.num_index[i], md5.index_array[i], md5.vertex_array[i], md5.num_vertex[i]);
		md5.generate_tangent(md5.index_array[i], md5.num_index[i], md5.vertex_array[i], md5.num_vertex[i]);
	}

	md5.generate_animation(frame);
	generate_buffers();
	md5.destroy_animation(frame);

	printf("Done\n");
}

void Engine::generate_buffers()
{
	frame_index = new int *[md5.anim->num_frame];
	frame_vao = new int *[md5.anim->num_frame];
	count_index = new int *[md5.anim->num_frame];
	frame_vertex = new int *[md5.anim->num_frame];
	count_vertex = new int *[md5.anim->num_frame];
	for (int j = 0; j < md5.anim->num_frame; j++)
	{
		frame_vao[j] = new int [md5.model->num_mesh];
		frame_index[j] = new int [md5.model->num_mesh];
		count_index[j] = new int [md5.model->num_mesh];
		frame_vertex[j] = new int [md5.model->num_mesh];
		count_vertex[j] = new int [md5.model->num_mesh];

		for (int i = 0; i < md5.model->num_mesh; i++)
		{
			int num_index, num_vertex;

			md5.PrepareMesh(i, frame[j], num_index, index_array, vertex_array, num_vertex);
//			frame_vao[j][i] = gfx.CreateVertexArrayObject();
			frame_index[j][i] = gfx.CreateIndexBuffer(index_array, num_index);
			count_index[j][i] = num_index;
			frame_vertex[j][i] = gfx.CreateVertexBuffer(vertex_array, num_vertex);
			count_vertex[j][i] = num_vertex;
		}
	}

	tex_object = new int [md5.model->num_mesh];
	normal_object = new int [md5.model->num_mesh];
	for(int i = 0; i < md5.model->num_mesh; i++)
	{
		char buffer[256];
		int width, height, components, format;
		char *bytes;

		sprintf(buffer, "media/%s.tga", md5.model->mesh[i].shader);
		bytes = (char *)gltLoadTGA(buffer, &width, &height, &components, &format);
		if (bytes == NULL)
		{
			printf("Unable to load texture %s\n", buffer);
			continue;
		}
		tex_object[i] = gfx.LoadTexture(width, height, components, format, bytes);
		delete [] bytes;

		sprintf(buffer, "media/%s_normal.tga", md5.model->mesh[i].shader);
		bytes = (char *)gltLoadTGA(buffer, &width, &height, &components, &format);
		if (bytes == NULL)
		{
			printf("Unable to load texture %s\n", buffer);
			continue;
		}
		normal_object[i] = gfx.LoadTexture(width, height, components, format, bytes);
		delete [] bytes;
	}
}

void Engine::destroy_buffers()
{
	for (int j = 0; j < md5.anim->num_frame; j++)
	{
		for (int i = 0; i < md5.model->num_mesh; i++)
		{
			gfx.DeleteVertexArrayObject(frame_vao[j][i]);
			gfx.DeleteVertexBuffer(frame_vertex[j][i]);
			gfx.DeleteIndexBuffer(frame_index[j][i]);
		}
		delete [] frame_vao[j];
		delete [] frame_index[j];
		delete [] count_index[j];
		delete [] frame_vertex[j];
		delete [] count_vertex[j];
	}
	delete frame_vao;
	delete frame_index;
	delete count_index;
	delete frame_vertex;
	delete count_vertex;

	for(int i = 0; i < snd_wave.size(); i++)
	{
		delete snd_wave[i].data;
	}

	for(int i = 0; i < md5.model->num_mesh; i++)
	{
		gfx.DeleteTexture(tex_object[i]);
		gfx.DeleteTexture(normal_object[i]);
	}
	delete [] tex_object;
	delete [] normal_object;
}

void Engine::load(char *level)
{
	if (map.loaded)
		return;

	menu.delta("load", *this);
	gfx.clear();
	menu.render(global);
	gfx.swap();
	camera.reset();
	post.init(&gfx);
	mlight2.init(&gfx);

	try
	{
		map.load(level);
	}
	catch (const char *error)
	{
		printf("%s\n", error);
		menu.print(error);
		menu.delta("unload", *this);
		return;
	}

	map.generate_meshes(gfx);
	parse_entity(map.get_entities(), entity_list);
	menu.delta("entities", *this);
	gfx.clear();
	menu.render(global);
	gfx.swap();
	load_entities();

	// This renders map before loading textures
	// need to add vertex color to rendering
	vec3 color(1.0f, 1.0f, 1.0f);
	camera.set(transformation);
	matrix4 mvp = transformation * projection;
//	entity_list[spawn]->rigid->frame2ent(&camera, keyboard);
	spatial_testing();
	gfx.clear();
	global.Select();
	global.Params(mvp, 0);
	gfx.SelectTexture(0, no_tex);

	GLenum err;

	err = glGetError();
	if ( err != GL_NO_ERROR)
	{
		printf("DrawArray GL_ERROR %d: attempting to draw will likely blow things up...\n", err);
		return;
	}

	map.render(camera.pos, NULL, gfx);
	render_entities();

	menu.delta("textures", *this);
	menu.render(global);
	gfx.swap();
	map.load_textures(gfx);
	menu.delta("loaded", *this);
	menu.stop();
	menu.ingame = false;
	menu.console = false;

	for(int i = 0; i < entity_list.size(); i++)
	{
		camera.set(transformation);
		mvp = transformation.premultiply(entity_list[i]->rigid->get_matrix(mvp.m)) * projection;
		vec3 pos = mvp * vec4(entity_list[i]->position.x, entity_list[i]->position.y, entity_list[i]->position.z, 0.0f);

		menu.draw_text(entity_list[i]->type, pos.x,
			pos.y, pos.z, 1000.0f,
			vec3(1.0f, 1.0f, 1.0f));
	}

	/*
	for(int i = 0; i < entity_list.size(); i++)
	{
		if (entity_list[i]->light)
		{
			entity_list[i]->light->generate_volumes(map);
			entity_list[i]->rigid->angular_velocity = vec3();
		}
	}
	*/
}

void Engine::render()
{
	if (map.loaded == false)
		return;

	gfx.clear();
//	gfx.Blend(true);
	render_scene(true);
//	gfx.Blend(false);
#ifdef SHADOWVOL
	matrix4 mvp;
	gfx.Color(false);
	gfx.Stencil(true);
	gfx.Depth(false);
	gfx.StencilFunc("always", 0, 0);

	gfx.StencilOp("keep", "keep", "incr"); // increment shadows that pass depth
	render_shadows();

	gfx.StencilOp("keep", "keep", "decr"); // decrement shadows that backface pass depth
	gfx.CullFace("front");
	render_shadows();

	gfx.Depth(true);
	gfx.Color(true);
	gfx.CullFace("back");

	gfx.DepthFunc("<="); // is this necessary?
	gfx.StencilOp("keep", "keep", "keep");
	
	//all lit surfaces will correspond to a 0 in the stencil buffer
	//all shadowed surfaces will be one
	gfx.StencilFunc("equal", 0, 1);
	// render with lights
	gfx.cleardepth();
	render_scene(true);

	gfx.DepthFunc("<");
	gfx.Stencil(false);
#endif

//	if (keyboard.control)
//		post_process(5);
	gfx.cleardepth();
	debug_messages();

	if (menu.ingame)
		menu.render(global);

	if (menu.console)
		menu.render_console(global);
	gfx.swap();
}

void Engine::render_scene(bool lights)
{
	matrix4 mvp;

	entity_list[spawn]->rigid->frame2ent(&camera, keyboard);

	render_entities();
//	render_shadows();
	camera.set(transformation);
	mlight2.Select();
	mvp = transformation * projection;
	if (lights)
		mlight2.Params(mvp, 0, 1, 2, light_list, light_list.size());
	else
		mlight2.Params(mvp, 0, 1, 2, light_list, 0);

//	entity_list[spawn]->rigid->calc_frustum(mvp);
	//map.render(camera.pos, (Plane *)&(entity_list[spawn]->model->frustum[0]), gfx);
	map.render(camera.pos, NULL, gfx);
	gfx.SelectShader(0);
}

void Engine::render_entities()
{
	matrix4 mvp;

	mlight2.Select();
	for(int i = 0; i < entity_list.size(); i++)
	{
		if (entity_list[i]->visible == false)
			continue;

		camera.set(transformation);
		mvp = transformation.premultiply(entity_list[i]->rigid->get_matrix(mvp.m)) * projection;
		mlight2.Params(mvp, 0, 1, 2, light_list, light_list.size());
		entity_list[i]->rigid->render(gfx);
		entity_list[i]->rigid->render_box(gfx);
		if (spawn == i)
		{
			entity_list[i]->rigid->get_matrix(mvp.m);
			mvp.m[12] += mvp.m[0] * -5.0f + mvp.m[4] * 50.0f + mvp.m[8] * 5.0f;
			mvp.m[13] += mvp.m[1] * -5.0f + mvp.m[5] * 50.0f + mvp.m[9] * 5.0f;
			mvp.m[14] += mvp.m[2] * -5.0f + mvp.m[6] * 50.0f + mvp.m[10] * 5.0f;
			mvp = transformation.premultiply(mvp.m) * projection;
			mlight2.Params(mvp, 0, 1, 2, light_list, light_list.size());
			entity_list[spawn]->player->weapon_model.render(gfx);
		}
	}
	gfx.SelectShader(0);

	mlight2.Select();
	camera.set(transformation);
	mvp = transformation.premultiply(entity_list[entity_list.size() - 1]->rigid->get_matrix(mvp.m)) * projection;
	mlight2.Params(mvp, 0, 1, 2, light_list, light_list.size());

	for (int i = 0; i < md5.model->num_mesh; ++i)
	{
//		gfx.SelectVertexArrayObject(frame_vao[frame_step][i]);
		gfx.SelectTexture(0, tex_object[i]);
		gfx.SelectTexture(2, normal_object[i]);
		gfx.SelectIndexBuffer(frame_index[frame_step][i]);
		gfx.SelectVertexBuffer(frame_vertex[frame_step][i]);
		gfx.DrawArray("triangles", 0, 0, count_index[frame_step][i], count_vertex[frame_step][i]);
//		gfx.SelectVertexArrayObject(0);
		gfx.SelectVertexBuffer(0);
		gfx.SelectIndexBuffer(0);
	}

	gfx.SelectShader(0);

}

void Engine::render_shadows()
{
	matrix4 mvp;

	for(int i = 0; i < entity_list.size(); i++)
	{
		if (entity_list[i]->visible == false)
			continue;

		if (entity_list[i]->light && entity_list[i]->visible)
		{
			camera.set(transformation);
			mvp = transformation * projection;
			global.Select();
			global.Params(mvp, 0);
			entity_list[i]->light->render_shadows();
			gfx.SelectShader(0);
		}
	}
}

void Engine::post_process(int num_passes)
{
	int temp;

	gfx.SelectTexture(0, post.image);
	for(int pass = 0; pass < num_passes; pass++)
	{
#ifndef DIRECTX
		glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 0, 0, gfx.width, gfx.height, 0);
#endif
		gfx.SelectTexture(1, post.swap);
		post.Select();
		post.Params(0, 1);
		gfx.clear();
//		gfx.SelectVertexArrayObject(Model::quad_vao);
		gfx.SelectIndexBuffer(Model::quad_index);
		gfx.SelectVertexBuffer(Model::quad_vertex);
		gfx.DrawArray("triangles", 0, 0, 6, 4);
		gfx.SelectShader(0);
		gfx.DeselectTexture(1);
//		gfx.SelectVertexArrayObject(0);
	}
	temp = post.swap;
	post.swap = post.image;
	post.image = temp;
	gfx.DeselectTexture(0);
}

void Engine::debug_messages()
{
	char msg[LINE_SIZE];
	transformation = identity;
	projection = identity;
	vec3 color(1.0f, 1.0f, 1.0f);

	snprintf(msg, LINE_SIZE, "Debug Messages");
	menu.draw_text(msg, 0.01f, 0.025f, 0.025f, color);
	snprintf(msg, LINE_SIZE, "%d active lights.", light_list.size());
	menu.draw_text(msg, 0.01f, 0.05f, 0.025f, color);
	snprintf(msg, LINE_SIZE, "Bullets: %d", entity_list[spawn]->player->ammo_bullets);
	menu.draw_text(msg, 0.01f, 0.075f, 0.025f, color);
	snprintf(msg, LINE_SIZE, "Shells: %d", entity_list[spawn]->player->ammo_shells);
	menu.draw_text(msg, 0.01f, 0.1f, 0.025f, color);
	snprintf(msg, LINE_SIZE, "Rockets: %d", entity_list[spawn]->player->ammo_rockets);
	menu.draw_text(msg, 0.01f, 0.125f, 0.025f, color);
	snprintf(msg, LINE_SIZE, "Bolts: %d", entity_list[spawn]->player->ammo_lightning);
	menu.draw_text(msg, 0.01f, 0.15f, 0.025f, color);
	snprintf(msg, LINE_SIZE, "position: %3.3f %3.3f %3.3f", entity_list[spawn]->position.x, entity_list[spawn]->position.y, entity_list[spawn]->position.z);
	menu.draw_text(msg, 0.01f, 0.175f, 0.025f, color);

	snprintf(msg, LINE_SIZE, "%d/%d", entity_list[spawn]->player->health, entity_list[spawn]->player->armor);
	menu.draw_text(msg, 0.15f, 0.95f, 0.050f, color);

	projection.perspective(45.0, (float)gfx.width / gfx.height, 1.0f, 2001.0f, true);
}

void Engine::spatial_testing()
{
	for(int i = 0; i < entity_list.size(); i++)
	{
		if (entity_list[i]->model)
		{
			bool ent_visible = false;

			for(int j = 0; j < 8; j++)
			{
				vec3 position = entity_list[i]->position + entity_list[i]->model->aabb[j];
				bool vert_visible = map.vis_test(camera.pos, position);

				if (vert_visible)
				{
					ent_visible = true;
					break;
				}
			}

			if (entity_list[i]->trigger)
			{
				if (entity_list[i]->trigger->active == false)
				{
					entity_list[i]->visible = ent_visible;
				}
			}
			else
			{
				entity_list[i]->visible = ent_visible;
			}
		}
		else
		{
			entity_list[i]->visible = map.vis_test(camera.pos, entity_list[i]->position);
		}

		if (entity_list[i]->visible == false)
			continue;

		vec3 dist_vec = entity_list[i]->position - camera.pos;
		float distance = dist_vec.x * dist_vec.x + dist_vec.y * dist_vec.y + dist_vec.z * dist_vec.z;

		if (entity_list[i]->light)
			activate_light(distance, entity_list[i]->light);

	}
}

void Engine::activate_light(float distance, Light *light)
{
	if (distance < 800.0f * 800.0f && light->entity->visible)
	{
		if (light->active == false)
		{
			light_list.push_back(light);
			light->active = true;
			light->entity->rigid->angular_velocity.x = 10.0;
			light->entity->rigid->angular_velocity.y = 10.0;
		}
	}
	else
	{
		if (light->active == true )
		{
			for( int i = 0; i < light_list.size(); i++)
			{
				if (light_list[i] == light)
				{
					light_list.erase(light_list.begin() + i);
					break;
				}
			}
			light->active = false;
			light->entity->rigid->angular_velocity.x = 0.0;
			light->entity->rigid->angular_velocity.y = 0.0;
		}
	}
}

/*
	This function can be threaded by:
	1. Dividing particles by bsp leafs
	   call find leaf for each vertex in aabb
	   if an objects aabb is in multiple leafs
	   union objects in both leafs together
	2. create a thread for each leaf batch.
	3. compute naive O(n^2)
	   each core should be able to handle 1000+ objects in 16ms, so no worrys
	   write results
	4. join threads
*/
void Engine::dynamics()
{
	cfg_t	config;

	#pragma omp parallel for
	for(int i = 0; i < entity_list.size(); i++)
	{
		if (entity_list[i]->visible == false || entity_list[i]->rigid->sleep == true )
			continue;

		RigidBody *body = entity_list[i]->rigid;
		if (entity_list[i]->vehicle != NULL)
			body = entity_list[i]->vehicle;
		float delta_time = 0.016f;
		float target_time = delta_time;
		float current_time = 0.0f;
		int divisions = 0;

		if (body->gravity)
			body->net_force = vec3(0.0f, -9.8f * body->mass, 0.0f);

		while (current_time < delta_time)
		{
			body->save_config(config);
			body->integrate(target_time - current_time);
			if ( collision_detect(*body) )
			{
				body->load_config(config);
				target_time = (current_time + target_time) / 2.0f;
				divisions++;

				if (divisions > 10)
				{
//					printf("Entity %d is sleeping\n", i);
					body->sleep = true;
					break;
				}
				continue;
			}
			current_time = target_time;
			target_time = delta_time;
		}
		body->net_force = vec3(0.0f, 0.0f, 0.0f);
	}
}


/*
	Handles all collision detection
	return true if simulated too far.
	return false if collision handled.
*/
bool Engine::collision_detect(RigidBody &body)
{
	Plane plane(vec3(0.0f, 1.0f, 0.0f).normalize(), 500.0f);

	if (map_collision(body))
		return true;
	else if (body_collision(body))
		return true;
	else if (body.collision_detect(plane))
		return true;
	else
		return false;
}

bool Engine::map_collision(RigidBody &body)
{
	Plane plane;
	float depth;
	vec3 stairstep(0.0f, 12.0f, 0.0f);

	// Check bounding box against map
	for(int i = 0; i < 8; i++)
	{
		vec3 point = body.aabb[i] - body.center + body.entity->position;

		//bsps cant really give us depth of penetration, only hit/no hit
		if (map.collision_detect(point, (plane_t *)&plane, &depth))
		{
			if (depth > -0.25f && depth < 0.0f)
			{
				point = point * (1.0f / UNITS_TO_METERS);
				body.impulse(plane, point);
				body.entity->position = body.old_position;
				body.morientation = body.old_orientation;
			}
			else
			{
				// make sure we dont try to stairstep the ground plane
				if (body.velocity.normalize() * vec3(0.0f, 1.0f, 0.0f) >= 0.9f)
				{
					if (map.collision_detect(point + stairstep, (plane_t *)&plane, &depth) == false)
					{
						body.entity->rigid->velocity += (vec3(0.0f, 0.35f, 0.0f));
						return false;
					}
				}
				return true;
			}
		}
	}
	return false;
}

//O(N^2)
bool Engine::body_collision(RigidBody &body)
{
	/*
	for(int i = 0; i < entity_list.size(); i++)
	{
		if (entity_list[i] == body.entity)
			continue;

		if (map.leaf_test(body.entity->position, entity_list[i]->position));
			body.collision_detect(*entity_list[i]->rigid);
	}
	*/
	return false;
}

void Engine::step()
{
	frame_step++;
	if (frame_step == md5.anim->num_frame)
		frame_step = 0;

	if (map.loaded == false)
		return;

	if (reload > 0)
		reload--;

	if (keyboard.leftbutton && reload == 0)
	{
		reload = 120;
		Entity *entity = new Entity();
		entity->position = camera.pos;
		entity->rigid = new RigidBody(entity);
		entity->rigid->load(gfx, "media/models/box");
		entity->rigid->velocity = camera.forward * -10.0f;
		entity->rigid->angular_velocity = vec3();
		entity->rigid->gravity = true;
		entity->model = entity->rigid;
		entity_list.push_back(entity);
	}
	

	//entity test movement
	if (menu.ingame == false && menu.console == false)
	{
		if (keyboard.control == true)
			camera.update(keyboard);
		else
			entity_list[spawn]->rigid->move(camera, keyboard);
	}

	spatial_testing();
	update_audio();


	dynamics();

	check_triggers();



/*
	//network
	sequence++;
	if (server && sequence)
		server_step();
	else if (client && sequence)
		client_step();
*/
}

void Engine::check_triggers()
{
	for(int i = 0; i < entity_list.size(); i++)
	{
		bool inside = false;

		if (entity_list[i]->trigger == NULL)
			continue;

		for ( int j = 0; j < 8; j++)
		{
			vec3 point = entity_list[i]->model->aabb[j] + entity_list[i]->position - entity_list[i]->rigid->center;
			inside = entity_list[spawn]->rigid->collision_detect(point);

			if (inside)
				break;

			point = entity_list[spawn]->model->aabb[j] + entity_list[spawn]->position  - entity_list[spawn]->rigid->center;
			inside = entity_list[i]->rigid->collision_detect(point);

			if (inside)
				break;
		}

		if (inside == true && entity_list[i]->trigger->active == false)
		{
			Player *player = entity_list[spawn]->player;

			entity_list[i]->trigger->active = true;
			console(entity_list[i]->trigger->action);

			printf("%d health\n%d armor\n%d weapons\n%d rockets\n %d slugs\n %d plasma\n %d shells\n %d bullets\n %d lightning\n",
				player->health, player->armor, 	player->weapon, player->ammo_rockets,
				player->ammo_slugs, player->ammo_plasma, player->ammo_shells,
				player->ammo_bullets, player->ammo_lightning );

			entity_list[i]->visible = false;
			entity_list[i]->trigger->timeout = 30.0f;

			for(int j = 0; j < snd_wave.size(); j++)
			{
				if (strcmp(snd_wave[j].file, entity_list[i]->trigger->pickup_snd) == 0)
				{
					audio.select_buffer(entity_list[i]->trigger->source, snd_wave[j].buffer);
					break;
				}
			}
			audio.play(entity_list[i]->trigger->source);
		}


		if (entity_list[i]->trigger->timeout > 0)
		{
			entity_list[i]->trigger->timeout -= 0.016;
		}
		else
		{
			if (entity_list[i]->trigger->active)
			{
				for(int j = 0; j < snd_wave.size(); j++)
				{
					if (strcmp(snd_wave[j].file, entity_list[i]->trigger->respawn_snd) == 0)
					{
						audio.select_buffer(entity_list[i]->trigger->source, snd_wave[j].buffer);
						break;
					}
				}
				audio.play(entity_list[i]->trigger->source);
			}

			entity_list[i]->trigger->active = false;
			entity_list[i]->trigger->timeout = 0.0f;
		}
	}
}

void Engine::server_step()
{
	servermsg_t	servermsg;
	clientmsg_t clientmsg;
	char socketname[LINE_SIZE];
	bool connected = false;
	int index;

	// send entities to clients
	send_entities();

	// get client packet
	int size = net.recvfrom((char *)&clientmsg, 8192, socketname, LINE_SIZE);
	if ( size <= 0 )
		return;

	if (clientmsg.length != size)
	{
		printf("Packet size mismatch\n");
		return;
	}

	// see if this ip/port combo already connected to server
	for(int i = 0; i < client_list.size(); i++)
	{
		if (strcmp(client_list[i]->socketname, socketname) == 0)
		{
			index = i;
			connected = true;
			break;
		}
		else
		{
			printf("%s != %s\n", socketname, client_list[i]->socketname);
		}
	}

	// update client input
	if (connected)
	{
		int keystate;
		memcpy(&keystate, clientmsg.data, 4);
		button_t clientkeys = GetKeyState(keystate);

		if (clientmsg.sequence <= client_list[index]->client_sequence)
		{
			printf("Got old packet\n");
			return;
		}

		client_list[index]->server_sequence = clientmsg.server_sequence;
		if (clientmsg.server_sequence > reliable.sequence)
		{
			memset(reliable.msg, 0, LINE_SIZE);
			reliable.sequence = -1;
		}

		if (client_list[index]->entity > entity_list.size())
		{
			printf("Invalid Entity\n");
			return;
		}
		entity_list[ client_list[index]->entity ]->rigid->move(clientkeys);


		/*
		printf("-> server_sequence %d\n"
			"<- client_sequence %d\n"
			"<- clients_server_sequence %d\n"
			"client delay %d steps\n"
			"reliable msg: %s\n",
			sequence, clientmsg.sequence, clientmsg.server_sequence,
			sequence - clientmsg.server_sequence,
			reliable.msg);
		*/

		if (clientmsg.length > HEADER_SIZE + sizeof(int) + sizeof(int) + 1)
		{
			reliablemsg_t *reliablemsg = (reliablemsg_t *)&clientmsg.data[4];

			if (client_list[index]->client_sequence < reliablemsg->sequence)
			{
				char msg[LINE_SIZE];

				printf("client msg: %s\n", reliablemsg->msg);
				sprintf(msg, "%s: %s\n", client_list[index]->socketname, reliablemsg->msg);
				menu.print(msg);
				chat(msg);
			}
		}
		client_list[index]->client_sequence = clientmsg.sequence;
	}

	// client not in list, check if it is a connect msg
	reliablemsg_t *reliablemsg = (reliablemsg_t *)&clientmsg.data[clientmsg.num_cmds * sizeof(int)];
	if ( strcmp(reliablemsg->msg, "connect") == 0 )
	{
		printf("client %s connected\n", socketname);
		servermsg.sequence = sequence;
		servermsg.client_sequence = clientmsg.sequence;
		servermsg.num_ents = 0;
		strcpy(reliable.msg, "map media/maps/q3tourney2.bsp");
		reliable.sequence = sequence;
		memcpy(&servermsg.data[servermsg.num_ents * sizeof(entity_t)],
			&reliable,
			sizeof(int) + strlen(reliable.msg) + 1);
		servermsg.length = HEADER_SIZE + servermsg.num_ents * sizeof(entity_t) +
			sizeof(int) + strlen(reliable.msg) + 1;
		net.sendto((char *)&servermsg, servermsg.length, socketname);
		printf("sent client map data\n");
	}
	else if ( strcmp(reliablemsg->msg, "spawn") == 0 )
	{
		client_t *client = (client_t *)malloc(sizeof(client_t));
		client->socketname = (char *)malloc(strlen(socketname) + 1);

		strcpy(client->socketname, socketname);
		client_list.push_back(client);
		printf("client %s spawned\n", client->socketname);
		client->client_sequence = clientmsg.sequence;

		// assign entity to client
		//set to zero if we run out of info_player_deathmatches
		client->entity = 0;
		int count = 0;
		for(int i = 0; i < entity_list.size(); i++)
		{
			if (strcmp(entity_list[i]->type, "info_player_deathmatch") == 0)
			{
				if (count == client_list.size())
				{
					printf("client %s got entity %d\n", socketname, i);
					client->entity = i;
					break;
				}
				count++;
			}
		}

		servermsg.sequence = sequence;
		servermsg.client_sequence = clientmsg.sequence;
		servermsg.num_ents = 0;
		
		sprintf(reliable.msg, "spawn %d", client->entity);
		reliable.sequence = sequence;
		memcpy(&servermsg.data[servermsg.num_ents * sizeof(entity_t)],
			&reliable,
			sizeof(int) + strlen(reliable.msg) + 1);
		servermsg.length = HEADER_SIZE + servermsg.num_ents * sizeof(entity_t) +
			sizeof(int) + strlen(reliable.msg) + 1;
		net.sendto((char *)&servermsg, servermsg.length, client->socketname);
		printf("sent client spawn data\n");
	}
}

void Engine::send_entities()
{
	servermsg_t	servermsg;

	servermsg.sequence = sequence;
	servermsg.client_sequence = 0;
	servermsg.num_ents = 0;
	for(int i = 0; i < client_list.size(); i++)
	{
		for(int j = 0; j < entity_list.size(); j++)
		{
			entity_t ent;

			bool visible = map.vis_test(entity_list[j]->position,
				entity_list[client_list[i]->entity]->position);
			if ( visible == false )
				continue;

			ent.id = j;
			ent.morientation = entity_list[j]->rigid->morientation;
			ent.angular_velocity = entity_list[j]->rigid->angular_velocity;
			ent.velocity = entity_list[j]->rigid->velocity;
			ent.position = entity_list[j]->position;

			memcpy(&servermsg.data[servermsg.num_ents * sizeof(entity_t)],
				&ent, sizeof(entity_t));
			servermsg.num_ents++;
		}
		memcpy(&servermsg.data[servermsg.num_ents * sizeof(entity_t)],
			(void *)&reliable,
			sizeof(int) + strlen(reliable.msg) + 1);
		servermsg.length = HEADER_SIZE + servermsg.num_ents * sizeof(entity_t) +
			sizeof(int) + strlen(reliable.msg) + 1;
		servermsg.client_sequence = client_list[i]->client_sequence;
		net.sendto((char *)&servermsg, servermsg.length, client_list[i]->socketname);

/*
		// ~15 second timeout
		if (sequence - client_list[i]->server_sequence > 900)
		{
			printf("client %s timed out\n"
				"sequence %d packet sequence %d", client_list[i]->socketname,
				sequence, client_list[i]->server_sequence);
			client_list.erase(client_list.begin() + i);
			i--;
			continue;
		}
*/
	}
}

void Engine::client_step()
{
	servermsg_t	servermsg;
	clientmsg_t clientmsg;
	int socksize = sizeof(sockaddr_in);
	int keystate = GetKeyState(keyboard);

	printf("client keystate %d\n", keystate);

	// get entity information
	int size = ::recvfrom(net.sockfd, (char *)&servermsg, 8192, 0, (sockaddr *)&(net.servaddr), &socksize);
	if ( size > 0)
	{
		if (size != servermsg.length)
		{
			printf("Packet size mismatch: %d %d\n", size, servermsg.length);
			return;
		}

		if (servermsg.sequence <= last_server_sequence)
		{
			printf("Got old packet\n");
			return;
		}

//		printf("Recieved %d ents from server\n", servermsg.num_ents);
		if (servermsg.client_sequence > reliable.sequence)
		{
			memset(reliable.msg, 0, LINE_SIZE);
			reliable.sequence = -1;
		}

		for(int i = 0; i < servermsg.num_ents; i++)
		{
			entity_t	*ent = (entity_t *)servermsg.data;

			// dont let bad data cause an exception
			if (ent[i].id > entity_list.size())
			{
				printf("Invalid entity index, bad packet\n");
				break;
			}

			// need better way to identify entities
			entity_list[ent[i].id]->position = ent[i].position;
			entity_list[ent[i].id]->rigid->velocity = ent[i].velocity;
			entity_list[ent[i].id]->rigid->angular_velocity = ent[i].angular_velocity;
			entity_list[ent[i].id]->rigid->morientation = ent[i].morientation;
		}

		/*
		printf("-> client_sequence %d\n"
			"<- server_sequence %d\n"
			"<- servers_client_sequence %d\n"
			"server delay %d steps\n"
			"reliable msg: %s\n",
			sequence, servermsg.sequence, servermsg.client_sequence,
			sequence - servermsg.client_sequence,
			reliable.msg);
		*/

		if ( servermsg.length > HEADER_SIZE + servermsg.num_ents * sizeof(entity_t) + sizeof(int) + 1)
		{
			reliablemsg_t *reliablemsg = (reliablemsg_t *)&servermsg.data[servermsg.num_ents * sizeof(entity_t)];

			if (last_server_sequence < reliablemsg->sequence)
			{
				int entity;

				printf("server msg: %s\n", reliablemsg->msg);
				menu.print(reliablemsg->msg);

				int ret = sscanf(reliablemsg->msg, "spawn %d", &entity);
				if ( ret )
				{
					spawn = entity;
				}
			}
		}
		last_server_sequence = servermsg.sequence;
	}

	// send keyboard state
	memset(&clientmsg, 0, sizeof(clientmsg_t));
	clientmsg.sequence = sequence;
	clientmsg.server_sequence = last_server_sequence;
	clientmsg.num_cmds = 1;
	memcpy(clientmsg.data, &keystate, sizeof(int));
	memcpy(&clientmsg.data[clientmsg.num_cmds * sizeof(int)],
		(void *)&reliable,
		sizeof(int) + strlen(reliable.msg) + 1);
	clientmsg.length = HEADER_SIZE + clientmsg.num_cmds * sizeof(int)
		+ sizeof(int) + strlen(reliable.msg) + 1;
	::sendto(net.sockfd, (char *)&clientmsg, clientmsg.length, 0, (sockaddr *)&(net.servaddr), socksize);
}

// packs keyboard input into an integer
int Engine::GetKeyState(button_t &kb)
{
	int keystate = 0;

	if (kb.leftbutton)
		keystate |= 1;
	if (kb.middlebutton)
		keystate |= 2;
	if (kb.rightbutton)
		keystate |= 4;
	if (kb.enter)
		keystate |= 8;
	if (kb.up)
		keystate |= 16;
	if (kb.left)
		keystate |= 32;
	if (kb.down)
		keystate |= 64;
	if (kb.right)
		keystate |= 128;

	return keystate;
}

// unpacks keyboard integer into keyboard state
button_t Engine::GetKeyState(int keystate)
{
	button_t kb;

	if (keystate & 1)
		kb.leftbutton = true;
	else
		kb.leftbutton = false;

	if (keystate & 2)
		kb.middlebutton = true;
	else
		kb.middlebutton = false;

	if (keystate & 4)
		kb.rightbutton = true;
	else
		kb.rightbutton = false;

	if (keystate & 8)
		kb.enter = true;
	else
		kb.enter = false;

	if (keystate & 16)
		kb.up = true;
	else
		kb.up = false;

	if (keystate & 32)
		kb.left = true;
	else
		kb.left = false;

	if (keystate & 64)
		kb.down = true;
	else
		kb.down = false;

	if (keystate & 128)
		kb.right = true;
	else
		kb.right = false;

	return kb;
}

bool Engine::mousepos(int x, int y, int deltax, int deltay)
{
	if (map.loaded == false || menu.ingame == true || menu.console == true)
	{
		float devicex = (float)x / gfx.width;
		float devicey = (float)y / gfx.height;

		if (menu.console == true)
			return false;

		bool updated = menu.delta(devicex, devicey);
		if (menu.ingame == false && updated)
		{
			gfx.clear();
			menu.render(global);
			gfx.swap();
		}

		return false;
	}

	camera.update(vec2((float)deltax, (float)deltay));
	return true;
}

void Engine::keypress(char *key, bool pressed)
{
	char k = 0;

	if (strcmp("enter", key) == 0)
	{
		keyboard.enter = pressed;
	}
	else if (strcmp("leftbutton", key) == 0)
	{
		keyboard.leftbutton = pressed;
		k = 14;
	}
	else if (strcmp("middlebutton", key) == 0)
	{
		keyboard.middlebutton = pressed;
		k = 15;
	}
	else if (strcmp("rightbutton", key) == 0)
	{
		keyboard.rightbutton = pressed;
		k = 16;
	}
	else if (strcmp("shift", key) == 0)
	{
		keyboard.shift = pressed;
	}
	else if (strcmp("control", key) == 0)
	{
		keyboard.control = pressed;
	}
	else if (strcmp("escape", key) == 0)
	{
		keyboard.escape = pressed;
	}
	else if (strcmp("up", key) == 0)
	{
		keyboard.up = pressed;
		k = 3;
	}
	else if (strcmp("left", key) == 0)
	{
		keyboard.left = pressed;
		k = 4;
	}
	else if (strcmp("down", key) == 0)
	{
		keyboard.down = pressed;
		k = 5;
	}
	else if (strcmp("right", key) == 0)
	{
		keyboard.right = pressed;
		k = 6;
	}

	if (pressed)
		keystroke(k);
}

void Engine::keystroke(char key)
{
	if (map.loaded == false)
	{
		menu.ingame = false;

		if (menu.console)
		{
			gfx.clear();
			menu.render(global);
			menu.handle_console(key, this);
			if (menu.console)
				menu.render_console(global);
			gfx.swap();
		}
		else
		{
			gfx.clear();
			menu.handle(key, this);

			menu.render(global);
			if (menu.console)
				menu.render_console(global);
			gfx.swap();
		}
	}
	else
	{
		if (menu.console)
			menu.handle_console(key, this);
		else if (menu.ingame)
			menu.handle(key, this);
		else
			handle_game(key);
	}
}

void Engine::handle_game(char key)
{
	switch (key)
	{
	case '~':
	case '`':
		menu.console = !menu.console;
		break;
	case 27:
		menu.ingame = !menu.ingame;
		break;
	}
}

void Engine::resize(int width, int height)
{
	gfx.resize(width, height);
	post.resize(width, height);
	if (map.loaded == false)
	{
		gfx.clear();
		menu.render(global);
		if (menu.console)
			menu.render_console(global);
		gfx.swap();
	}
	projection.perspective( 45.0, (float) width / height, 1.0f, 2001.0f, true );
}

void Engine::load_sounds()
{
	wave_t wave[8];

	for(int i = 0; i < entity_list.size(); i++)
	{
		int		num_wave = 0;
		bool	add = true;

		if (entity_list[i]->speaker)
		{
			strcpy(wave[0].file, entity_list[i]->speaker->file);
			num_wave++;
		}
		else if (entity_list[i]->trigger)
		{
			strcpy(wave[0].file, entity_list[i]->trigger->pickup_snd);
			strcpy(wave[1].file, entity_list[i]->trigger->respawn_snd);
			num_wave += 2;
		}

		for(int k = 0; k < num_wave; k++)
		{
			for(int j = 0; j < snd_wave.size(); j++)
			{
				char *file = snd_wave[j].file;
				if (strcmp(wave[k].file, file) == 0)
				{
					add = false;
					break;
				}
			}

			if (add == false)
				continue;

			printf("Loading wave file %s\n", wave[k].file);
			audio.load(wave[k]);
			if (wave[k].data == NULL)
				continue;

			snd_wave.push_back(wave[k]);
		}


	}
}

// To prevent making a class that looks exactly like model...
// I will search previous entities for models that are already loaded
void Engine::load_models()
{
	entity_list[0]->model->load(gfx, "media/models/box");
	for(int i = 1; i < entity_list.size(); i++)
	{
		bool loaded = false;

		if (entity_list[i]->model == NULL)
			continue;

		for(int j = 0; j < i; j++)
		{
			if (entity_list[j]->model == NULL)
				continue;

			entity_list[i]->model->clone(*entity_list[0]->model);
			if (strcmp(entity_list[i]->type, entity_list[j]->type) == 0)
			{
				entity_list[i]->model->clone(*entity_list[j]->model);
				loaded = true;
				break;
			}
		}

		if (loaded)
			continue;

		load_model(*entity_list[i]);
	}
}

void Engine::load_model(Entity &ent)
{
	if (strcmp(ent.type, "item_armor_shard") == 0)
	{
		printf("Loading item_armor_shard\n");
		ent.model->load(gfx, "media/models/powerups/armor/shard");
		ent.rigid->angular_velocity = vec3(0.0f, 2.0f, 0.0);
		ent.rigid->gravity = false;
	}
	else if (strcmp(ent.type, "weapon_rocketlauncher") == 0)
	{
		printf("Loading weapon_rocketlauncher\n");
		ent.model->load(gfx, "media/models/weapons2/rocketl/rocketl");
		ent.rigid->angular_velocity = vec3(0.0f, 2.0f, 0.0);
		ent.rigid->gravity = false;
	}
	else if (strcmp(ent.type, "ammo_shells") == 0)
	{
		printf("Loading ammo_shells\n");
		ent.model->load(gfx, "media/models/powerups/ammo/ammo");
		ent.rigid->angular_velocity = vec3(0.0f, 2.0f, 0.0);
		ent.rigid->gravity = false;
	}
	else if (strcmp(ent.type, "ammo_rockets") == 0)
	{
		printf("Loading ammo_rockets\n");
		ent.model->load(gfx, "media/models/powerups/ammo/ammo");
		ent.rigid->angular_velocity = vec3(0.0f, 2.0f, 0.0);
		ent.rigid->gravity = false;
	}
	else if (strcmp(ent.type, "ammo_lightning") == 0)
	{
		printf("Loading ammo_rockets\n");
		ent.model->load(gfx, "media/models/powerups/ammo/ammo");
		ent.rigid->angular_velocity = vec3(0.0f, 2.0f, 0.0);
		ent.rigid->gravity = false;
	}
	else if (strcmp(ent.type, "item_armor_combat") == 0)
	{
		printf("Loading item_armor_combat\n");
		ent.model->load(gfx, "media/models/powerups/armor/item_armor_combat");
		ent.rigid->angular_velocity = vec3(0.0f, 2.0f, 0.0);
		ent.rigid->gravity = false;
	}
	else if (strcmp(ent.type, "weapon_lightning") == 0)
	{
		printf("Loading weapon_lightning\n");
		ent.model->load(gfx, "media/models/weapons2/lightning/lightning");
		ent.rigid->angular_velocity = vec3(0.0f, 2.0f, 0.0);
		ent.rigid->gravity = false;
	}
	else if (strcmp(ent.type, "weapon_shotgun") == 0)
	{
		printf("Loading weapon_shotgun\n");
		ent.model->load(gfx, "media/models/weapons2/shotgun/shotgun");
		ent.rigid->angular_velocity = vec3(0.0f, 2.0f, 0.0);
		ent.rigid->gravity = false;
	}
	else if (strcmp(ent.type, "weapon_railgun") == 0)
	{
		printf("Loading weapon_railgun\n");
		ent.model->load(gfx, "media/models/weapons2/railgun/railgun");
		ent.rigid->angular_velocity = vec3(0.0f, 2.0f, 0.0);
		ent.rigid->gravity = false;
	}
	else if (strcmp(ent.type, "item_health") == 0)
	{
		printf("Loading item_health\n");
		ent.model->load(gfx, "media/models/powerups/health/health");
		ent.rigid->angular_velocity = vec3(0.0f, 2.0f, 0.0);
		ent.rigid->gravity = false;
	}
	else if (strcmp(ent.type, "item_health_large") == 0)
	{
		printf("Loading item_health_large\n");
		ent.model->load(gfx, "media/models/powerups/health/health");
		ent.rigid->angular_velocity = vec3(0.0f, 2.0f, 0.0);
		ent.rigid->gravity = false;
	}
	else if (strcmp(ent.type, "info_player_deathmatch") == 0)
	{
		printf("Loading info_player_deathmatch\n");
		ent.rigid->load(gfx, "media/models/ball");
		ent.rigid->gravity = false;
	}

}

void Engine::init_camera()
{
	for(int i = 0; i < entity_list.size(); i++)
	{
		char *type = entity_list[i]->type;

		if (type == NULL)
			continue;

		if ( strcmp(type, "info_player_deathmatch") == 0 )
		{
			camera.pos = entity_list[i]->position;
			spawn = i;
			entity_list[spawn]->player = new Player(entity_list[i], gfx);
			break;
		}
	}
	audio.listener_position((float *)&(camera.pos));
}

// Loads media that may be shared with multiple entities
void Engine::load_entities()
{
	init_camera();
	load_sounds();
	create_sources();
	load_models();

	for(int i = 0; i < entity_list.size(); i++)
	{
		if (entity_list[i]->light)
			entity_list[i]->rigid->gravity = false;
	}

	entity_list[spawn]->rigid->load(gfx, "media/models/thug22/thug22");
//	entity_list[spawn]->rigid->load(gfx, "media/models/box");
	entity_list[spawn]->position += entity_list[spawn]->rigid->center;
}

void Engine::create_sources()
{
	// create and associate sources
	for(int i = 0; i < entity_list.size(); i++)
	{
		if (entity_list[i]->speaker != NULL)
		{
			entity_list[i]->rigid->gravity = false;
			for(int j = 0; j < snd_wave.size(); j++)
			{
				if (strcmp(snd_wave[j].file, entity_list[i]->speaker->file) == 0)
				{
					entity_list[i]->speaker->source = audio.create_source(entity_list[i]->speaker->loop, false);
					//alSourcef(entity_list[i]->speaker->source, AL_GAIN, 30.0f);
					audio.select_buffer(entity_list[i]->speaker->source, snd_wave[j].buffer);
					audio.effects(entity_list[i]->speaker->source);

					if (entity_list[i]->speaker->loop)
						audio.play(entity_list[i]->speaker->source);
					break;
				}
			}
		}
		else if (entity_list[i]->trigger != NULL)
		{
			entity_list[i]->rigid->gravity = false;
			entity_list[i]->trigger->source = audio.create_source(false, false);
			alSourcef(entity_list[i]->trigger->source, AL_GAIN, 30.0f);
			audio.effects(entity_list[i]->trigger->source);
		}
	}

	// position sources
	update_audio();
}

void Engine::update_audio()
{
	audio.listener_position((float *)&(camera.pos));
	audio.listener_velocity((float *)&(entity_list[spawn]->rigid->velocity));
	audio.listener_orientation((float *)&(entity_list[spawn]->rigid->morientation.m));
	for(int i = 0; i < entity_list.size(); i++)
	{
		if (entity_list[i]->speaker)
		{
			audio.source_position(entity_list[i]->speaker->source, (float *)(&entity_list[i]->position));
			audio.source_velocity(entity_list[i]->speaker->source, (float *)(&entity_list[i]->rigid->velocity));
		}
	}
}

void Engine::unload()
{
	if (map.loaded == false)
		return;

	menu.ingame = false;
	for(int i = 0; i < entity_list.size(); i++)
	{
		if (entity_list[i]->speaker)
			entity_list[i]->speaker->destroy(audio);
		delete entity_list[i];
	}
	entity_list.clear();
	light_list.clear();

//	light_list.~List();
//	entity_list~List();

	post.destroy();
	mlight2.destroy();
	map.unload(gfx);
	menu.play();
	menu.delta("unload", *this);
	menu.render(global);
}

void Engine::destroy()
{
	printf("Shutting down.\n");
	destroy_buffers();
	unload();
	gfx.destroy();
	audio.destroy();
}

void Engine::quit()
{
#ifdef _WINDOWS_
	HWND hwnd = *((HWND *)param1);
	PostMessage(hwnd, WM_CLOSE, 0, 0);
#endif
}


// Need asset manager class so things arent doubly loaded
int load_texture(Graphics &gfx, char *file_name)
{
	int width, height, components, format;
	int tex_object;

	byte *bytes = gltLoadTGA(file_name, &width, &height, &components, &format);
	if (bytes == NULL)
	{
		printf("Unable to load texture %s\n", file_name);
		return 0;
	}
	tex_object = gfx.LoadTexture(width, height, components, format, bytes);
	delete [] bytes;
	if (format == GL_BGRA_EXT)
	{
		// negative means it has an alpha channel
		return -tex_object;
	}
	return tex_object;
}

void Engine::console(char *cmd)
{
	char msg[LINE_SIZE];
	char data[LINE_SIZE];
	int port;
	int ret;

	printf("Console: %s\n", cmd);
	menu.print(cmd);

	ret = sscanf(cmd, "map %s", data);
	if (ret == 1)
	{
		unload();
		snprintf(msg, LINE_SIZE, "Loading %s\n", data);
		menu.print(msg);
		load(data);
		return;
	}

	ret = sscanf(cmd, "health %s", data);
	if (ret == 1)
	{
		snprintf(msg, LINE_SIZE, "health %s\n", data);
		menu.print(msg);
		entity_list[spawn]->player->health += atoi(data);
		if (entity_list[spawn]->player->health > 100)
			entity_list[spawn]->player->health = 100;
		return;
	}

	ret = sscanf(cmd, "armor %s", data);
	if (ret == 1)
	{
		snprintf(msg, LINE_SIZE, "armor %s\n", data);
		menu.print(msg);
		entity_list[spawn]->player->armor += atoi(data);
		if (entity_list[spawn]->player->armor > 200)
			entity_list[spawn]->player->armor = 200;
		return;
	}

	if (strcmp(cmd, "weapon_rocketlauncher") == 0)
	{
		snprintf(msg, LINE_SIZE, "weapon_rocketlauncher\n");
		menu.print(msg);
		entity_list[spawn]->player->weapon |= WEAPON_ROCKET;
		entity_list[spawn]->player->ammo_rockets = MAX(10, entity_list[spawn]->player->ammo_rockets);
		return;
	}

	if (strcmp(cmd, "weapon_shotgun")  == 0)
	{
		snprintf(msg, LINE_SIZE, "weapon_shotgun\n");
		menu.print(msg);
		entity_list[spawn]->player->weapon |= WEAPON_SHOTGUN;
		entity_list[spawn]->player->ammo_shells = MAX(10, entity_list[spawn]->player->ammo_shells);
		return;
	}

	if (strcmp(cmd, "weapon_lightning")  == 0)
	{
		snprintf(msg, LINE_SIZE, "weapon_lightning\n");
		menu.print(msg);
		entity_list[spawn]->player->weapon |= WEAPON_LIGHTNING;
		entity_list[spawn]->player->ammo_lightning = MAX(100, entity_list[spawn]->player->ammo_lightning);
		return;
	}

	ret = sscanf(cmd, "ammo_rockets %s", data);
	if (ret == 1)
	{
		snprintf(msg, LINE_SIZE, "ammo_rockets %s\n", data);
		menu.print(msg);
		entity_list[spawn]->player->ammo_rockets += atoi(data);
		return;
	}

	ret = sscanf(cmd, "ammo_slugs %s", data);
	if (ret == 1)
	{
		snprintf(msg, LINE_SIZE, "ammo_slugs %s\n", data);
		menu.print(msg);
		entity_list[spawn]->player->ammo_slugs += atoi(data);
		return;
	}

	ret = sscanf(cmd, "ammo_shells %s", data);
	if (ret == 1)
	{
		snprintf(msg, LINE_SIZE, "ammo_shells %s\n", data);
		menu.print(msg);
		entity_list[spawn]->player->ammo_shells += atoi(data);
		return;
	}

	ret = sscanf(cmd, "ammo_bullets %s", data);
	if (ret == 1)
	{
		snprintf(msg, LINE_SIZE, "ammo_bullets %s\n", data);
		menu.print(msg);
		entity_list[spawn]->player->ammo_bullets += atoi(data);
		return;
	}

	ret = sscanf(cmd, "ammo_lightning %s", data);
	if (ret == 1)
	{
		snprintf(msg, LINE_SIZE, "ammo_lightning %s\n", data);
		menu.print(msg);
		entity_list[spawn]->player->ammo_lightning += atoi(data);
		return;
	}

	ret = sscanf(cmd, "ammo_plasma %s", data);
	if (ret == 1)
	{
		snprintf(msg, LINE_SIZE, "ammo_plasma %s\n", data);
		menu.print(msg);
		entity_list[spawn]->player->ammo_plasma += atoi(data);
		return;
	}

	ret = sscanf(cmd, "ammo_bfg %s", data);
	if (ret == 1)
	{
		snprintf(msg, LINE_SIZE, "ammo_bfg %s\n", data);
		menu.print(msg);
		entity_list[spawn]->player->ammo_bfg += atoi(data);
		return;
	}

	ret = sscanf(cmd, "connect %s", data);
	if (ret == 1)
	{
		snprintf(msg, LINE_SIZE, "Connecting to %s\n", data);
		menu.print(msg);
		connect(data);
		return;
	}

	ret = sscanf(cmd, "say \"%[^\"]s", data);
	if (ret == 1)
	{
		chat(cmd);
		return;
	}

	if (strcmp(cmd, "res") == 0)
	{
		snprintf(msg, LINE_SIZE, "%dx%d\n", gfx.width, gfx.height);
		menu.print(msg);
		return;
	}

	ret = sscanf(cmd, "bind %d", &port);
	if (ret == 1)
	{
		snprintf(msg, LINE_SIZE, "binding to port %d\n", port);
		menu.print(msg);
		load("media/maps/q3tourney2.bsp");
		bind(port);
		return;
	}

	if (strcmp(cmd, "disconnect") == 0)
	{
		snprintf(msg, LINE_SIZE, "disconnecting\n");
		menu.print(msg);
		unload();
		return;
	}

	if (strcmp(cmd, "quit") == 0)
	{
		snprintf(msg, LINE_SIZE, "Exiting\n");
		menu.print(msg);
		quit();
	}
	snprintf(msg, LINE_SIZE, "Unknown command: %s\n", cmd);
	menu.print(msg);
}

void Engine::bind(int port)
{
	if (server)
	{
		printf("Server already bound to port\n");
		return;
	}

	try
	{
		net.bind(NULL, port);
	}
	catch (const char *err)
	{
		printf("Net Error: %s\n", err);
	}
	server = true;
}

/*
	network will use control tcp connection and udp streaming of gamestate
*/
void Engine::connect(char *server)
{
	clientmsg_t	clientmsg;
	servermsg_t servermsg;

	client = false;

	memset(&clientmsg, 0, sizeof(clientmsg_t));
	try
	{
		clientmsg.sequence = sequence;
		clientmsg.server_sequence = 0;
		clientmsg.num_cmds = 0;
		strcpy(reliable.msg, "connect");
		reliable.sequence = sequence;
		memcpy(&clientmsg.data[clientmsg.num_cmds * sizeof(int)],
			&reliable,
			sizeof(int) + strlen(reliable.msg) + 1);
		clientmsg.length = HEADER_SIZE + clientmsg.num_cmds * sizeof(int)
			+ sizeof(int) + strlen(reliable.msg) + 1;

		net.connect(server, 65535);
		printf("Sending map request\n");
		net.send((char *)&clientmsg, clientmsg.length);
		printf("Waiting for server info\n");

		if ( net.recv((char *)&servermsg, 8192, 5) )
		{
			char level[LINE_SIZE];

			client = true;
			printf("Connected\n");
			reliablemsg_t *reliablemsg = (reliablemsg_t *)&servermsg.data[servermsg.num_ents * sizeof(entity_t)];
			if (sscanf(reliablemsg->msg, "map %s", level) == 1)
			{
				printf("Loading %s\n", level);
				load((char *)level);
				strcpy(reliable.msg, "spawn");
				reliable.sequence = sequence;
				last_server_sequence = servermsg.sequence;
			}
			else
			{
				printf("Invalid response\n");
			}
		}
		else
		{
			printf("Connection timed out\n");
		}
	}
	catch (const char *err)
	{
		printf("Net Error: %d %s\n", errno, err);
		perror("error");
	}
}

void Engine::chat(char *msg)
{
	strcat(reliable.msg, msg);
	reliable.sequence = sequence;
}