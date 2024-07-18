const int tile_width = 8;

int world_to_tile_pos(float world_pos) {
	return	roundf(world_pos / (float)tile_width);
}

float tile_pos_to_world_pos(int tile_pos){
	return (float)tile_pos * (float)tile_width;
}

Vector2 round_v2_to_tile(Vector2 world_pos) {
	world_pos.x = tile_pos_to_world_pos(world_to_tile_pos(world_pos.x));
	world_pos.y = tile_pos_to_world_pos(world_to_tile_pos(world_pos.x));
	return world_pos;
}

bool almost_equals(float a, float b, float epsilon) {
	return fabs(a - b) <= epsilon;
}

bool animate_f32_to_target(float* value, float target, float delta_t, float rate) {
	*value += (target - *value) * (1.0 - pow(2.0f, -rate * delta_t));
	if(almost_equals(*value, target, 0.001f)) {
		*value = target;
		return true;
	}
	return false;
} 

void animate_v2_to_target(Vector2* value, Vector2 target, float delta_t, float rate) {
	animate_f32_to_target(&(value->x), target.x, delta_t, rate);
	animate_f32_to_target(&(value->y), target.y, delta_t, rate);
}

typedef enum EntityArchetype {
	arch_nil = 0,
	arch_counter = 1,
	arch_holder = 2,
	arch_child = 3,
	arch_player = 4,
} EntityArchetype;

typedef struct Sprite {
	Gfx_Image* image;
	Vector2 size;
} Sprite;
typedef enum SpriteID {
	SPRITE_nil,
	SPRITE_player,
	SPRITE_counter,
	SPRITE_child,
	SPRITE_MAX,
} SpriteID;
Sprite sprites[SPRITE_MAX];
Sprite* get_sprite(SpriteID id) {
	if (id >= 0 && id < SPRITE_MAX) {
		return &sprites[id];
	}
	return &sprites[0];
}
// randy: maybe we make this an X marco????, (but a bitch to debug)

typedef struct Entity {
	bool is_valid;
	EntityArchetype arch;
	Vector2 pos;
	bool render_sprite;
	SpriteID sprite;
} Entity;
#define MAX_ENTITY_COUNT 1024

typedef struct World {
	Entity entities[MAX_ENTITY_COUNT];
} World;
World* world = 0;

Entity* entity_create() {
	Entity* entity_found = 0;
	for (int i = 0; i < MAX_ENTITY_COUNT; i++) {
		Entity* existing_entity = &world->entities[i];
		if(!existing_entity->is_valid) {
			entity_found = existing_entity;
			break;
		}
	}
	assert(entity_found, "No more free entities!");
	entity_found->is_valid = true;
	return entity_found;
}

void entity_destroy(Entity* entity) {
	memset(entity, 0, sizeof(Entity));
}

void setup_counter(Entity* en) {
	en->arch = arch_counter;
	en->sprite = SPRITE_counter;
}
void setup_child(Entity* en) {
	en->arch = arch_child;
	en->sprite = SPRITE_child;
}
void setup_player(Entity* en) {
	en->arch = arch_player;
	en->sprite = SPRITE_player;
}

Vector2 screen_to_world() {

	float mouse_x = input_frame.mouse_x;
	float mouse_y = input_frame.mouse_y;
	Matrix4 proj = draw_frame.projection;
	Matrix4 view = draw_frame.view;
	float window_w = window.width;
	float window_h = window.height;

	// Normalize the mouse coordinates
	float ndc_x = (mouse_x / (window_w * 0.5f)) - 1.0f;
	float ndc_y = (mouse_y / (window_h * 0.5f)) - 1.0f;

	//transform to world coordinates
	Vector4 world_pos = v4(ndc_x, ndc_y, 0, 1);
	world_pos = m4_transform(m4_inverse(proj), world_pos);
	world_pos = m4_transform(view, world_pos);
	//log("%f, %f", world_pos.x, world_pos.y);

	// Return as 2D vector
	return (Vector2){ world_pos.x, world_pos.y };
}


int entry(int argc, char **argv) {

	window.title = STR("Minimal Game Example");
	window.scaled_width = 1280; // We need to set the scaled size if we want to handle system scaling (DPI)
	window.scaled_height = 720; 
	window.x = 200;
	window.y = 90;
	window.clear_color = hex_to_rgba(0xe2b570ff);

	seed_for_random = os_get_current_cycle_count();

	world = alloc(get_heap_allocator(), sizeof(World));
	memset(world, 0, sizeof(World));

	sprites[SPRITE_player] = (Sprite){ .image=load_image_from_disk(fixed_string("player.png"), get_heap_allocator()), .size=v2(6.0, 6.0) };
	sprites[SPRITE_counter] = (Sprite){ .image=load_image_from_disk(fixed_string("counter.png"), get_heap_allocator()), .size=v2(17, 10) };
	sprites[SPRITE_child] = (Sprite){ .image=load_image_from_disk(fixed_string("child.png"), get_heap_allocator()), .size=v2(3, 4) };

	Gfx_Font *font = load_font_from_disk(STR("C:/windows/fonts/arial.ttf"), get_heap_allocator());
	assert(font, "Failed loading arial.ttf");
	const u32 font_height = 48;

	Entity* player_en = entity_create();
	setup_player(player_en);

	Entity *counter_en = entity_create();
	setup_counter(counter_en);
	counter_en->pos = round_v2_to_tile(counter_en->pos);
	counter_en->pos.y -= tile_width * 0.5;

	for (int i = 0; i < 10; i++) {
		Entity* en = entity_create();
		setup_child(en);
		en->pos = v2(get_random_float32_in_range(-120.0, 120.0), get_random_float32_in_range(-67.0, 67.0));
		en->pos = round_v2_to_tile(en->pos);
		en->pos.y -= tile_width * 0.5;
	}


	float64 seconds_counter = 0.0;
	s32 frame_count = 0;

	float zoom = 5.3;
	Vector2 camera_pos = v2(0, 0);

	float64 last_time = os_get_current_time_in_seconds();
	while (!window.should_close) {
		float64 now = os_get_current_time_in_seconds();
		float64 delta_t = now - last_time;
		last_time = now;

		reset_temporary_storage();

		draw_frame.projection = m4_make_orthographic_projection(window.width * -0.5, window.width * 0.5, window.height * -0.5, window.height * 0.5, 
			-1, 10);

		//	:camera
		{
			Vector2 target_pos = player_en->pos;
			animate_v2_to_target(&camera_pos, target_pos, delta_t, 30.0f);

			draw_frame.view = m4_make_scale(v3(1.0, 1.0, 1.0));
			draw_frame.view = m4_mul(draw_frame.view, m4_make_translation(v3(camera_pos.x, camera_pos.y, 0)));
			draw_frame.view = m4_mul(draw_frame.view, m4_make_scale(v3(1.0/zoom, 1.0/zoom, 1.0)));
		}

		Vector2 mouse_pos = screen_to_world();
		int mouse_tile_x = world_to_tile_pos(mouse_pos.x);
		int mouse_tile_y = world_to_tile_pos(mouse_pos.y);
		{
			
			//draw_text(font, sprint(temp, STR("%f %f"), pos.x, pos.y), font_height, pos, v2(0.1,0.1), COLOR_RED);
			for (int i = 0; i < MAX_ENTITY_COUNT; i++) {
				Entity* en = &world->entities[i];
				if(en->is_valid) {
					Sprite* sprite = get_sprite(en->sprite);
					Range2f bounds = range2f_make_bottom_center(sprite->size);
					bounds = range2f_shift(bounds, en->pos);

					Vector4 col = COLOR_RED;
					col.a = 0.4;
					if (range2f_contains(bounds, mouse_pos)) {
						col.a = 1.0;
					}

					draw_rect(bounds.min, range2f_size(bounds), col);
				}
			}
		}

		// :tile rendering
		{
			int player_tile_x = world_to_tile_pos(player_en->pos.x);
			int player_tile_y = world_to_tile_pos(player_en->pos.y);
			const int tile_radius_x = 40;
			const int tile_radius_y = 30;
		
			for (int x = player_tile_x - tile_radius_x; x < player_tile_x + tile_radius_x; x++) {
				for (int y = player_tile_y - tile_radius_y; y < player_tile_y + tile_radius_y; y++) {
					if((x + (y % 2 == 0)) % 2 == 0) {
						float x_pos = x * tile_width;
						float y_pos = y * tile_width;
						Vector4 col = v4(0.1, 0.1, 0.1, 0.1);
						draw_rect(v2(x_pos + tile_width * -0.5, y_pos + tile_width*-0.5), v2(tile_width, tile_width), col);
					}
				}
			}
		}

		draw_rect(v2(tile_pos_to_world_pos(mouse_tile_x) + tile_width * -0.5, tile_pos_to_world_pos(mouse_tile_y) + tile_width * -0.5), v2(tile_width, tile_width), v4(0.5, 0.5, 0.5, 0.5));

		for (int i = 0; i < MAX_ENTITY_COUNT; i++) {
			Entity* en = &world->entities[i];
			if (en->is_valid) {
				
				switch (en->arch) {
					default:
					{
						Sprite* sprite = get_sprite(en->sprite);

						Matrix4 xform = m4_scalar(1.0);
						xform = m4_translate(xform, v3(en->pos.x, en->pos.y, 0));
						xform = m4_translate(xform, v3(sprite->size.x * -0.5, 0.0, 0));
						draw_image_xform(sprite->image, xform, sprite->size, COLOR_WHITE);

						draw_text(font, sprint(temp, STR("%f %f"), en->pos.x, en->pos.y), font_height, en->pos, v2(0.1,0.1), COLOR_WHITE);
						break;
					}
				}
			}
		}
				
		if(is_key_just_pressed(KEY_ESCAPE)) {
			window.should_close = true;
		}

		Vector2 input_axis = v2(0,0);
		if (is_key_down('A')) {
			input_axis.x -= 1.0;
		}
		if (is_key_down('D')) {
			input_axis.x += 1.0;
		}
		if (is_key_down('S')) {
			input_axis.y -= 1.0;
		}
		if (is_key_down('W')) {
			input_axis.y += 1.0;
		}
		input_axis = v2_normalize(input_axis);

		//player_pos = player_pos + (input_axis * 10.0);
		player_en->pos = v2_add(player_en->pos, v2_mulf(input_axis, 100.0 * delta_t));


		os_update(); 
		gfx_update();
		seconds_counter += delta_t;
		frame_count += 1;
		if (seconds_counter > 1.0) {
			log("fps: %i", frame_count);
			seconds_counter = 0.0;
			frame_count = 0;
		}
	}

	return 0;
}