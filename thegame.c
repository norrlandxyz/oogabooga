inline float v2_length(Vector2 a) {
    return sqrt(a.x * a.x + a.y * a.y);
}
inline float v2_dist(LMATH_ALIGN Vector2 a, LMATH_ALIGN Vector2 b) {
	return v2_length(v2_sub(a, b));
}


//in case of engine update^^

float sin_breathe (float time, float rate) {
	return sin(time * rate+ 1.0) / 2.0;
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

//generic utils ^^

const int tile_width = 8;
const int entity_selection_radius = 16;
const float player_pickup_radius = 20.0;

int world_pos_to_tile_pos(float world_pos) {
	return	roundf(world_pos / (float)tile_width);
}

float tile_pos_to_world_pos(int tile_pos){
	return (float)tile_pos * (float)tile_width;
}

Vector2 round_v2_to_tile(Vector2 world_pos) {
	world_pos.x = tile_pos_to_world_pos(world_pos_to_tile_pos(world_pos.x));
	world_pos.y = tile_pos_to_world_pos(world_pos_to_tile_pos(world_pos.y));
	return world_pos;
}



typedef enum EntityArchetype {
	arch_nil = 0,
	arch_counter = 1,
	arch_holder = 2,
	arch_child = 3,
	arch_player = 4,

	arch_item_child = 5,	
	arch_item_holder = 6,
	arch_item_cactus = 7,
	arch_item_blueberry = 8,
	ARCH_MAX,
} EntityArchetype;



typedef struct Sprite {
	Gfx_Image* image;
} Sprite;
typedef enum SpriteID {
	SPRITE_nil,
	SPRITE_player,
	SPRITE_counter,
	SPRITE_holder,
	SPRITE_female,
	SPRITE_male,
	SPRITE_old_man,
	SPRITE_child,
	SPRITE_item_child,
	SPRITE_item_cactus,
	SPRITE_item_blueberry,
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

Vector2 get_sprite_size(Sprite* sprite) {
	return (Vector2){sprite->image->width, sprite->image->height};
}

SpriteID get_sprite_id_from_archetype(EntityArchetype arch) {
	switch(arch) {
		case arch_item_child: return SPRITE_item_child; break;
		case arch_item_cactus: return SPRITE_item_cactus; break;
		case arch_item_holder: return SPRITE_holder; break;
		default: return 0;
	}
}

typedef struct Entity {
	bool is_valid;
	EntityArchetype arch;
	Vector2 pos;
	bool render_sprite;
	SpriteID sprite;
	int health;
	bool destroyable_world_item;
	bool is_item;
} Entity;
// :entity
#define MAX_ENTITY_COUNT 1024

typedef struct ItemData {
	EntityArchetype type;
	int amount;
} ItemData;

typedef struct World {
	Entity entities[MAX_ENTITY_COUNT];
	ItemData inventory_items[ARCH_MAX];
	
} World;
World* world = 0;

typedef struct WorldFrame {
	Entity* selected_entity;
} WorldFrame;
WorldFrame world_frame;

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
void setup_holder(Entity* en) {
	en->arch = arch_holder;
	en->sprite = SPRITE_holder;
	en->destroyable_world_item = true;
}
void setup_item_holder(Entity* en) {
	en->arch = arch_item_holder;
	en->sprite = SPRITE_holder;
	en->is_item = true;
}
void setup_counter(Entity* en) {
	en->arch = arch_counter;
	en->sprite = SPRITE_counter;
	en->destroyable_world_item = true;
	en->health = 3;
}
void setup_child(Entity* en) {
	en->arch = arch_child;
	en->sprite = SPRITE_child;
	en->destroyable_world_item = true;
	en->health = 3;
}
void setup_item_child(Entity* en) {
	en->arch = arch_item_child;
	en->sprite = SPRITE_item_child;
	en->is_item = true;
}
void setup_player(Entity* en) {
	en->arch = arch_player;
	en->sprite = SPRITE_player;
}
void setup_item_cactus(Entity* en) {
	en->arch = arch_item_cactus;
	en->sprite = SPRITE_item_cactus;
	en->is_item = true;
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

	sprites[0] = (Sprite){ .image=load_image_from_disk(fixed_string("res/sprites/missing_tex.png"), get_heap_allocator())};
	sprites[SPRITE_player] = (Sprite){ .image=load_image_from_disk(fixed_string("res/sprites/player.png"), get_heap_allocator())};
	sprites[SPRITE_counter] = (Sprite){ .image=load_image_from_disk(fixed_string("res/sprites/counter.png"), get_heap_allocator())};
	sprites[SPRITE_holder] = (Sprite){ .image=load_image_from_disk(fixed_string("res/sprites/holder.png"), get_heap_allocator())};
	sprites[SPRITE_child] = (Sprite){ .image=load_image_from_disk(fixed_string("res/sprites/child.png"), get_heap_allocator())};
	sprites[SPRITE_item_child] = (Sprite){ .image=load_image_from_disk(fixed_string("res/sprites/item_child.png"), get_heap_allocator())};
	sprites[SPRITE_item_cactus] = (Sprite){ .image=load_image_from_disk(fixed_string("res/sprites/item_cactus.png"), get_heap_allocator())};



	Gfx_Font *font = load_font_from_disk(STR("C:/windows/fonts/arial.ttf"), get_heap_allocator());
	assert(font, "Failed loading arial.ttf");
	const u32 font_height = 48;

	// :init

	// test item adding
	{
		world->inventory_items[arch_item_child].amount = 5;
		world->inventory_items[arch_item_holder].amount = 5;
	}

	Entity* player_en = entity_create();
	setup_player(player_en);

	Entity *counter_en = entity_create();
	setup_counter(counter_en);
	counter_en->pos = round_v2_to_tile(counter_en->pos);

	Entity *holder_en = entity_create();
	setup_holder(holder_en);
	holder_en->pos = v2(30.0, 20.0);
	holder_en->pos = round_v2_to_tile(holder_en->pos);
	//counter_en->pos.y -= tile_width * 0.5;

	for (int i = 0; i < 10; i++) {
		Entity* en = entity_create();
		setup_child(en);
		en->pos = v2(get_random_float32_in_range(-120.0f, 120.0f), get_random_float32_in_range(-65.0f, 65.0f));
		en->pos = round_v2_to_tile(en->pos);
		//en->pos.y -= tile_width * 0.5;
	}


	float64 seconds_counter = 0.0;
	s32 frame_count = 0;

	float zoom = 5.3;
	Vector2 camera_pos = v2(0, 0);

	float64 last_time = os_get_current_time_in_seconds();
	while (!window.should_close) {
		reset_temporary_storage();
		world_frame = (WorldFrame){0};
		float64 now = os_get_current_time_in_seconds();
		float64 delta_t = now - last_time;
		last_time = now;

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

		Vector2 mouse_pos_world = screen_to_world();
		int mouse_tile_x = world_pos_to_tile_pos(mouse_pos_world.x);
		int mouse_tile_y = world_pos_to_tile_pos(mouse_pos_world.y);
		{
			
			float smallest_dist = INFINITY;		

			for (int i = 0; i < MAX_ENTITY_COUNT; i++) {
				Entity* en = &world->entities[i];
				if(en->is_valid && en->destroyable_world_item) {

					int entity_tile_x = world_pos_to_tile_pos(en->pos.x);
					int entity_tile_y = world_pos_to_tile_pos(en->pos.y);


					float dist = fabsf(v2_dist(en->pos, mouse_pos_world));
					if (dist < entity_selection_radius) {
						
						if (!world_frame.selected_entity || (dist < smallest_dist)) {
							world_frame.selected_entity = en;
							smallest_dist = dist;
						}

					}
				}
			}
		}

		// :tile rendering
		{
			int player_tile_x = world_pos_to_tile_pos(player_en->pos.x);
			int player_tile_y = world_pos_to_tile_pos(player_en->pos.y);
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

		// :update entities
		{
			for (int i = 0; i < MAX_ENTITY_COUNT; i++) {
				Entity* en = &world->entities[i];
				if (en->is_valid) {
					if(en->is_item) {
						//TODO epic physics
						if (fabsf(v2_dist(en->pos, player_en->pos)) < player_pickup_radius) {
							world->inventory_items[en->arch].amount += 1;
							entity_destroy(en);
						}
					}
				}
			}
		}


		// :click destroy
		{
			Entity* selected_en = world_frame.selected_entity;

			if (is_key_just_pressed(MOUSE_BUTTON_LEFT)) {
				consume_key_just_pressed(MOUSE_BUTTON_LEFT);

				if (selected_en) {
					selected_en->health -= 1;
					if(selected_en->health <= 0) {
						
						switch(selected_en->arch) {
							case arch_child: {
								Entity* en = entity_create();
								setup_item_child(en);
								en->pos = selected_en->pos;
							} break;

							case arch_holder: {
								Entity* en = entity_create();
								setup_item_holder(en);
								en->pos = selected_en->pos;
							} break;

							default: {} break;

						}

						entity_destroy(selected_en);
					}
				}
			}
		}

		//:render entites
		for (int i = 0; i < MAX_ENTITY_COUNT; i++) {
			Entity* en = &world->entities[i];
			if (en->is_valid) {
				
				switch (en->arch) {
					default:
					{
						Sprite* sprite = get_sprite(en->sprite);

						Matrix4 xform = m4_scalar(1.0);
						if(en->is_item) {
							xform = m4_translate(xform, v3(0, 2.0 * sin_breathe(os_get_current_time_in_seconds(), 5), 0));
						}
						xform = m4_translate(xform, v3(0, tile_width * -0.5, 0));
						xform = m4_translate(xform, v3(en->pos.x, en->pos.y, 0));
						xform = m4_translate(xform, v3(sprite->image->width * -0.5, 0.0, 0));

						Vector4 col = COLOR_WHITE;
						if (world_frame.selected_entity == en) {
							col = COLOR_RED;
						}

						draw_image_xform(sprite->image, xform, get_sprite_size(sprite), col);

						break;
					}
				}
			}
		}
				
		//:render UI
		{
			float width = 240.0;
			float height = 135.0;
			draw_frame.view = m4_scalar(1.0);
			draw_frame.projection = m4_make_orthographic_projection(0.0, width, 0.0, height, -1, 10);

			float y_pos = 70.0;

			int item_count = 0;
			for (int i = 0; i < ARCH_MAX; i++) {
				ItemData* item = &world->inventory_items[i];
				if(item->amount > 0) { 
					item_count += 1;
				}
			}

			const float icon = 8.0;
			const float padding = 2.0;
			float icon_width = icon + padding;
			float entire_thing = item_count * icon_width;
			float x_start_pos = (width/2.0)-(entire_thing/2.0) + (icon_width * 0.5);
			
			int slot_index = 0;
			for (int i = 0; i < ARCH_MAX; i++) {
				ItemData* item = &world->inventory_items[i];
				if(item->amount > 0) { 

					float slot_index_offset = slot_index * icon_width;

					Matrix4 xform = m4_scalar(1.0);
					xform = m4_translate(xform, v3(x_start_pos + slot_index_offset, y_pos, 0.0));
					xform = m4_translate(xform, v3(-4, -4, 0.0));
					draw_rect_xform(xform, v2(8, 8), COLOR_BLACK);

					Sprite* sprite = get_sprite(get_sprite_id_from_archetype(i));

					draw_image_xform(sprite->image, xform, get_sprite_size(sprite), COLOR_WHITE);

					slot_index += 1;
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