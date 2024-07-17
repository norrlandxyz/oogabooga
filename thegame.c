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



int entry(int argc, char **argv) {
	
	window.title = STR("Minimal Game Example");
	window.scaled_width = 1280; // We need to set the scaled size if we want to handle system scaling (DPI)
	window.scaled_height = 720; 
	window.x = 200;
	window.y = 90;
	window.clear_color = hex_to_rgba(0xe2b570ff);

	world = alloc(get_heap_allocator(), sizeof(World));
	memset(world, 0, sizeof(World));

	sprites[SPRITE_player] = (Sprite){ .image=load_image_from_disk(fixed_string("player.png"), get_heap_allocator()), .size=v2(6.0, 6.0) };
	sprites[SPRITE_counter] = (Sprite){ .image=load_image_from_disk(fixed_string("counter.png"), get_heap_allocator()), .size=v2(17, 10) };
	sprites[SPRITE_child] = (Sprite){ .image=load_image_from_disk(fixed_string("child.png"), get_heap_allocator()), .size=v2(3, 4) };

	Entity* player_en = entity_create();
	setup_player(player_en);

	Entity *counter_en = entity_create();
	setup_counter(counter_en);

	for (int i = 0; i < 10; i++) {
		Entity* en = entity_create();
		setup_child(en);
		en->pos = v2(get_random_float32_in_range(-200.0, 200.0), get_random_float32_in_range(-200.0, 200.0));
	}


	float64 seconds_counter = 0.0;
	s32 frame_count = 0;

	float64 last_time = os_get_current_time_in_seconds();
	while (!window.should_close) {
		reset_temporary_storage();

		//sometimes a little off since the screen is integer...
		draw_frame.projection = m4_make_orthographic_projection(window.width * -0.5, window.width * 0.5, window.height * -0.5, window.height * 0.5, 
			-1, 10);

		float zoom = 5.3;
		draw_frame.view = m4_make_scale(v3(1.0/zoom, 1.0/zoom, 1.0));

		float64 now = os_get_current_time_in_seconds();
		float64 delta_t = now - last_time;
		last_time = now;

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