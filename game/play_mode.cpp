#include "play_mode.h"
#include "world/world.h"
#include "systems/system_factory.h"
#include "../engine/gui/gui_frame.h"
#include "../engine/gui/gui_static_text.h"
#include "gui/gui_main_game_view.h"
#include "game.h"
#include "raws/raws.h"
#include <sstream>

using namespace engine;
using std::make_unique;

class game_render : public engine::base_node {
public:
    int mouse_x = 0;
    int mouse_y = 0;
    int mouse_vx = 0;
    int mouse_vy = 0;
    int mouse_hover_time = 0;

    void process_mouse_events() {
    vector<mouse_motion_message> * mouse_movement = game_engine->messaging->get_messages_by_type<mouse_motion_message>();
    for (mouse_motion_message &m : *mouse_movement) {
	mouse_x = m.x;
	mouse_y = m.y;
	const int new_mouse_vx = mouse_x / 16;
	const int new_mouse_vy = (mouse_y-48) / 16;
	
	if (mouse_vx != new_mouse_vx or mouse_vy != new_mouse_vy) {	
	    mouse_vx = new_mouse_vx;
	    mouse_vy = new_mouse_vy;
	    mouse_hover_time = 0;
	} else {
	    ++mouse_hover_time;
	}
	m.deleted = true;
    }
    
    vector<command_message> * actions = game_engine->messaging->get_messages_by_type<command_message>();
    for (command_message &m : *actions) {
      if (m.command == RIGHT_CLICK) {
	  m.deleted = true;
	  if (mouse_vy > 3) {
	      world::paused = true;
	      // Change to options mode
	  }
      }
      if (m.command == TOGGLE_RENDER_MODE) {
	  world::render_graphics = !world::render_graphics;
      }
    }
}
  
    virtual void render(sdl2_backend * SDL) {
      process_mouse_events();
      if ( world::render_graphics) {
	  render_map( SDL );
      } else {
	  render_map_ascii( SDL );
      }
      // Render particles
      render_power_bar( SDL );
      render_date_time ( SDL );
      render_paused ( SDL );
      render_emotes( SDL );
      render_tool_tips( SDL );
    }
    
    inline void render_tool_tips ( sdl2_backend * SDL ) {
	if (mouse_hover_time < 10) return;
	//if (mouse_vx < 0 or mouse_vx > 1024/16) return;
	//if (mouse_vy < 0 or mouse_vy > 768-48/16) return;
	
	const SDL_Color sdl_green {64,255,64,0};
	const SDL_Color sdl_cyan {0,255,255,0};
	const SDL_Color sdl_white {255,255,255,0};
	const position_component * camera_pos = game_engine->ecs->find_entity_component<position_component>(world::camera_handle);
      
	int region_x = camera_pos->x - 32 + mouse_vx;
	int region_y = camera_pos->y - 23 + mouse_vy;
	//std::cout << "(" << region_x << "," << region_y << ")\n";
	
	const int idx = world::current_region->idx( region_x, region_y );
	if ( world::current_region->visible[ idx ] == false ) return;
	
	const int screen_x = mouse_vx*16 + 16;
	const int screen_y = mouse_vy*16 + 48;
	
	std::vector<std::pair<std::string,SDL_Color>> lines;
	
	lines.push_back( make_pair( world::current_region->tiles[idx].get_description(), sdl_green ) );
	lines.push_back( make_pair( world::current_region->tiles[idx].get_climate(), sdl_cyan ) );
		
	vector<position_component> * positions = game_engine->ecs->find_components_by_type<position_component>();
	for (const position_component &pos : *positions) {
	    if (pos.x == region_x and pos.y == region_y) {
		const int entity_id = pos.entity_id;
		std::stringstream desc;
		settler_ai_component * settler = game_engine->ecs->find_entity_component<settler_ai_component>(entity_id);
		if (settler != nullptr) {
		    game_species_component * species = game_engine->ecs->find_entity_component<game_species_component>(entity_id);
		    desc << settler->first_name << " " << settler->last_name << " (" << settler->profession_tag << ")";
		    lines.push_back( make_pair( desc.str(), sdl_white ));
		} else {	    
		    debug_name_component * debug_name = game_engine->ecs->find_entity_component<debug_name_component>(entity_id);
		    if (debug_name != nullptr) {
			desc << debug_name->debug_name;
			lines.push_back( make_pair( desc.str(), sdl_white ));
		    }
		    
		    // Stored items
		    vector<item_storage_component *> stored_items = game_engine->ecs->find_components_by_func<item_storage_component>(
		      [entity_id] ( const item_storage_component &e) {
			  if (e.container_id == entity_id) return true;
			  return false;
		      }
		    );
		    for (item_storage_component * item : stored_items) {
			debug_name_component * nc = game_engine->ecs->find_entity_component<debug_name_component>( item->entity_id );
			lines.push_back( make_pair( string("   ") + nc->debug_name, sdl_green ) );
		    }
		}
	    }
	}
	
	int width = 0;
	for ( const std::pair<std::string,SDL_Color> &tooltip_line : lines ) {
	    if (width < tooltip_line.first.size() * 7) width = tooltip_line.first.size() * 7;	    
	}
	
	int y = screen_y - ((lines.size()/2) * 16);
	
	SDL->set_alpha_mod( "spritesheet", 128 );
	SDL_Rect source = raws::get_tile_source_by_name("BLACKMASK");
	const unsigned char height = lines.size()*16;
	SDL_Rect dest = { screen_x, y-4, width, height+8 };
	SDL->render_bitmap( "spritesheet", source, dest );
	SDL->set_alpha_mod( "spritesheet", 255 );
	
	for ( const std::pair<std::string,SDL_Color> &tooltip_line : lines ) {
	    std::string line_s = SDL->render_text_to_image( "disco14", tooltip_line.first, "tmp", tooltip_line.second );
	    SDL->render_bitmap_simple( line_s, screen_x + 16, y );
	    y += 16;
	}
    }
    
    inline void render_power_bar ( sdl2_backend * SDL ) {
      SDL_Rect src { 0, 0, 1024, 16 };
      SDL_Rect dest { 0 , 16 , 1024, 16 };
      SDL->render_bitmap( "power_bar_red", src, dest );
      
      const float power_percent = float(world::stored_power) / float(world::max_power);
      const int ticks = 1024 * power_percent;
      dest = { 0 , 16 , ticks, 16 };
      SDL->render_bitmap( "power_bar_green", src, dest );
      
      std::stringstream ss;
      ss << "<Power: " << world::stored_power << " / " << world::max_power << ">";
      const SDL_Color sdl_yellow {255,255,0,0};
      string emote_text = SDL->render_text_to_image( "disco14", ss.str(), "tmp", sdl_yellow );
      SDL->render_bitmap_centered( emote_text, 506, 16 );
    }
    
    inline void render_emotes( sdl2_backend * SDL ) {
      vector<chat_emote_message> * emote_ptr = game_engine->messaging->get_messages_by_type<chat_emote_message>();
      if ( !emote_ptr->empty() ) {
	
	// Calculate the "home" position - top left.
	const position_component * camera_pos = game_engine->ecs->find_entity_component<position_component>(world::camera_handle);      
	const int region_x = camera_pos->x - 32;
	const int region_y = camera_pos->y - 23;
	
	
	for (chat_emote_message &emote : *emote_ptr) {
	    const unsigned char fade = 8 * emote.ttl;
	    const SDL_Color sdl_black {0,0,0,0};
	    string emote_text = SDL->render_text_to_image( "disco12", emote.message, "tmp", sdl_black );
	    SDL->set_alpha_mod( emote_text, fade );
	    SDL->set_alpha_mod( "emote_bubble", fade );
	    std::pair<int,int> emote_size = SDL->query_bitmap_size( emote_text );
	    
	    // Left part of bubble
	    const int x = ( emote.x - region_x ) * 16;
	    const int y = ( emote.y - region_y )*16 + 48;
	    const int height = emote_size.second;

	    SDL_Rect src { 0, 0, 4, 8 };
	    SDL_Rect dest { x , y , 4, height };
	    SDL->render_bitmap( "emote_bubble", src, dest );
	    
	    // Center of bubble
	    src = { 5, 0, 4, 8 };
	    dest = { x + 4, y, emote_size.first, height };
	    SDL->render_bitmap( "emote_bubble", src, dest );
	    
	    // Right part of bubble
	    src = { 27, 0, 4, 8 };
	    dest = { x + 4+emote_size.first, y, 4, height };
	    SDL->render_bitmap( "emote_bubble", src, dest );
	    
	    // The text itself
	    SDL->render_bitmap_simple( emote_text, x+4, y );
	    SDL->set_alpha_mod( "emote_bubble", 0 );
	}
      }
    }
    
    inline void render_date_time( sdl2_backend * SDL ) {
	SDL_Color sdl_white = {255,255,255,255};
	
	if (world::display_day_month.empty()) world::display_day_month = " ";
	if (world::display_time.empty()) world::display_time = " ";
	const std::string the_date = game_engine->render_text_to_image( "disco14", world::display_day_month, "btn_playgame", sdl_white );
	const std::string the_time = game_engine->render_text_to_image( "disco14", world::display_time, "btn_playgame", sdl_white );
	SDL->render_bitmap_simple(the_date, 0, 0);
	SDL->render_bitmap_simple(the_time, 100, 0);
    }
    
    inline void render_paused ( sdl2_backend * SDL ) {
      if (world::paused) {
	  SDL->render_bitmap_simple( "paused", 900, 0 );
      }
    }
    
    inline void set_base_source(SDL_Rect &source, const int &idx) {
	if (world::current_region->tiles[ idx ].base_tile_type == tile_type::WATER ) {
	    source = raws::get_tile_source_by_name("WATER"); // Water
	} else if ( world::current_region->tiles[ idx ].base_tile_type == tile_type::RAMP or
	  world::current_region->tiles[ idx ].base_tile_type > 4
	) {
	    switch (world::current_region->tiles[ idx ].base_tile_type) {
	      case tile_type::RAMP_NU_SD : source = raws::get_tile_source_by_name("RAMP_NU_SD"); break;
	      case tile_type::RAMP_ED_WU : source = raws::get_tile_source_by_name("RAMP_ED_WU"); break;
	      case tile_type::RAMP_EU_WD : source = raws::get_tile_source_by_name("RAMP_EU_WD"); break;
	      case tile_type::RAMP_ND_SU : source = raws::get_tile_source_by_name("RAMP_ND_SU"); break;
	      case tile_type::RAMP_EU_SU : source = raws::get_tile_source_by_name("RAMP_EU_SU"); break;
	      case tile_type::RAMP_WU_SU : source = raws::get_tile_source_by_name("RAMP_WU_SU"); break;
	      case tile_type::RAMP_EU_NU : source = raws::get_tile_source_by_name("RAMP_EU_NU"); break;
	      case tile_type::RAMP_WU_NU : source = raws::get_tile_source_by_name("RAMP_WU_NU"); break;
	      case tile_type::RAMP_WD_ND : source = raws::get_tile_source_by_name("RAMP_WD_ND"); break;
	      case tile_type::RAMP_ED_ND : source = raws::get_tile_source_by_name("RAMP_ED_ND"); break;
	      case tile_type::RAMP_WD_SD : source = raws::get_tile_source_by_name("RAMP_WD_SD"); break;
	      case tile_type::RAMP_ED_SD : source = raws::get_tile_source_by_name("RAMP_ED_SD"); break;
	      default: source = raws::get_tile_source_by_name("DOOD");
	    }
	} else {
	    switch (world::current_region->tiles [ idx ].ground ) {
	      case tile_ground::IGNEOUS : source = raws::get_tile_source_by_name("IGNEOUS"); break;
	      case tile_ground::SEDIMENTARY : raws::get_tile_source_by_name("SEDIMENTARY"); break;
	      case tile_ground::GRAVEL : source = raws::get_tile_source_by_name("GRAVEL"); break;
	      case tile_ground::WHITE_SAND : source = raws::get_tile_source_by_name("WHITE_SAND"); break;
	      case tile_ground::YELLOW_SAND : source = raws::get_tile_source_by_name("YELLOW_SAND"); break;
	      case tile_ground::RED_SAND : source = raws::get_tile_source_by_name("RED_SAND"); break;
	      default : std::cout << "Oops: unknown ground type : " << world::current_region->tiles [ idx ].ground << "\n";
	    }
	    source = raws::get_tile_source_by_name("SEDIMENTARY");
	}	
    }
    
    inline bool set_covering_source(SDL_Rect &source, const int &idx) {
	  bool render_cover = false;
	  if ( world::current_region->tiles[ idx ].base_tile_type == tile_type::RAMP or
	    world::current_region->tiles[ idx ].base_tile_type > 4
	  ) return false;
	  
	  if (world::current_region->tiles [ idx ].covering == tile_covering::CACTUS) { source = raws::get_tile_source_by_name("CACTUS"); render_cover = true; }
	  if (world::current_region->tiles [ idx ].covering == tile_covering::GORSE) { source = raws::get_tile_source_by_name("GORSE"); render_cover = true; }
	  if (world::current_region->tiles [ idx ].covering == tile_covering::GRASS) { source = raws::get_tile_source_by_name("GRASS"); render_cover = true; }
	  if (world::current_region->tiles [ idx ].covering == tile_covering::HEATHER) { source = raws::get_tile_source_by_name("HEATHER"); render_cover = true; }
	  if (world::current_region->tiles [ idx ].covering == tile_covering::LYCHEN) { source = raws::get_tile_source_by_name("LYCHEN"); render_cover = true; }
	  if (world::current_region->tiles [ idx ].covering == tile_covering::MOSS) { source = raws::get_tile_source_by_name("MOSS"); render_cover = true; }
	  if (world::current_region->tiles [ idx ].covering == tile_covering::REEDS) { source = raws::get_tile_source_by_name("REEDS"); render_cover = true; }
	  if (world::current_region->tiles [ idx ].covering == tile_covering::SHRUB) { source = raws::get_tile_source_by_name("SHRUB"); render_cover = true; }
	  if (world::current_region->tiles [ idx ].covering == tile_covering::THISTLE) { source = raws::get_tile_source_by_name("THISTLE"); render_cover = true; }
	  if (world::current_region->tiles [ idx ].covering == tile_covering::WILDFLOWER) { source = raws::get_tile_source_by_name("WILDFLOWER"); render_cover = true; }	  
	  return render_cover;
    }
    
    inline void render_map_ascii(sdl2_backend * SDL) {
      const position_component * camera_pos = game_engine->ecs->find_entity_component<position_component>(world::camera_handle);
      
      int region_y = camera_pos->y - 23;
      int left_x = camera_pos->x - 32;
      const SDL_Rect background_source{176, 208, 16, 16};
      
      // map goes from 0,48 to 1024,768
      SDL_Rect source = {16,32,16,16};
      for (int y=0; y<45; ++y) {
	int region_x = left_x;
	for (int x=0; x<64; ++x) {
	    const int idx = world::current_region->idx(region_x, region_y);
	    SDL_Rect dest = {x*16, (y*16) + 48, 16, 16};
 	    if (region_x >= 0 and region_x < landblock_width and region_y > 0 and region_y <= landblock_height-1 and world::current_region->revealed[idx] ) {
		// Tile type to render
		unsigned char target_char = world::current_region->tiles[idx].display;
		int texture_x = (target_char % 16) * 16;
		int texture_y = (target_char / 16) * 16;
		source = {texture_x, texture_y, 16,16 };
		color_t foreground = world::current_region->tiles[idx].foreground;
		SDL->set_color_mod("font", std::get<0>(foreground), std::get<1>(foreground), std::get<2>(foreground));
		SDL->render_bitmap("font", source, dest);

		// Render any renderable items for this tile
		auto finder = world::entity_render_list.find( idx );
		if ( finder != world::entity_render_list.end() ) {
		    target_char = std::get<1>(finder->second);
		    texture_x = (target_char % 16) * 16;
		    texture_y = (target_char / 16) * 16;
		    source = {texture_x, texture_y, 16,16 };
		    color_t foreground = std::get<2>(finder->second);
		    color_t background = std::get<3>(finder->second);
		    SDL->set_color_mod("font", std::get<0>(background), std::get<1>(background), std::get<2>(background));
		    SDL->render_bitmap("font", background_source, dest);
		    SDL->set_color_mod("font", std::get<0>(foreground), std::get<1>(foreground), std::get<2>(foreground));
		    SDL->render_bitmap("font", source, dest);
		}
		
		// Altitude-based mask; lighter for higher elevations
		// Add in time-of-day
		const float angle_difference = std::abs( 90.0F - world::sun_angle );
		float intensity_pct = angle_difference/90.0F;
		if (world::current_region->visible[idx]) intensity_pct /= 2.0;
		intensity_pct = 0.0;
		unsigned char alpha_mask = (64.0F * intensity_pct) + ( ( 10 - world::current_region->tiles[idx].level_band) * 16 );
		if (alpha_mask < 0) alpha_mask = 0;
		SDL->set_alpha_mod( "spritesheet", alpha_mask );
		source = raws::get_tile_source_by_name("BLACKMASK");
		SDL->render_bitmap("spritesheet", source, dest);
		SDL->set_alpha_mod( "spritesheet", 255 );
		
		// Render not visible
		if (world::current_region->visible[idx] == false) {
		    source = raws::get_tile_source_by_name("HIDEMASK");
		    SDL->render_bitmap("spritesheet", source, dest);
		}

	    }
	    ++region_x;
	}
	++region_y;
      }
    }
    
    inline void render_map(sdl2_backend * SDL) {
      const position_component * camera_pos = game_engine->ecs->find_entity_component<position_component>(world::camera_handle);
      
      int region_y = camera_pos->y - 23;
      int left_x = camera_pos->x - 32;
      
      // map goes from 0,48 to 1024,768
      SDL_Rect source = {16,32,16,16};
      for (int y=0; y<45; ++y) {
	int region_x = left_x;
	for (int x=0; x<64; ++x) {
	    const int idx = world::current_region->idx(region_x, region_y);
 	    if (region_x >= 0 and region_x < landblock_width and region_y > 0 and region_y <= landblock_height-1 and world::current_region->revealed[idx] ) {
//	    if (region_x >= 0 and region_x < landblock_width and region_y > 0 and region_y <= landblock_height-1 ) {
		// Render the base ground
		set_base_source(source, idx);
		SDL_Rect dest = {x*16, (y*16) + 48, 16, 16};
		SDL->render_bitmap("spritesheet", source, dest);
		
		// Render any covering
		bool render_cover = set_covering_source(source, idx);
		if (render_cover) {
		  SDL->render_bitmap("spritesheet", source, dest);
		}
		
		// Render any renderable items for this tile
		auto finder = world::entity_render_list.find( idx );
		if ( finder != world::entity_render_list.end() ) {
		    const int sprite_idx = std::get<0>(finder->second);
		    source = raws::get_tile_source( sprite_idx );
		    SDL->render_bitmap("spritesheet", source, dest);
		}
		
		// Altitude-based mask; lighter for higher elevations
		// Add in time-of-day
		const float angle_difference = std::abs( 90.0F - world::sun_angle );
		float intensity_pct = angle_difference/90.0F;
		if (world::current_region->visible[idx]) intensity_pct /= 2.0;
		intensity_pct = 0.0;
		unsigned char alpha_mask = (64.0F * intensity_pct) + ( ( 10 - world::current_region->tiles[idx].level_band) * 16 );
		if (alpha_mask < 0) alpha_mask = 0;
		SDL->set_alpha_mod( "spritesheet", alpha_mask );
		source = raws::get_tile_source_by_name("BLACKMASK");
		SDL->render_bitmap("spritesheet", source, dest);
		SDL->set_alpha_mod( "spritesheet", 255 );
		
		// Render not visible
		if (world::current_region->visible[idx] == false) {
		    source = raws::get_tile_source_by_name("HIDEMASK");
		    SDL->render_bitmap("spritesheet", source, dest);
		}
	    }
	    ++region_x;
	}
	++region_y;
      }
    }
};

void play_mode::init_systems()
{
     game_engine->ecs->add_system ( make_input_system() );
     game_engine->ecs->add_system ( make_camera_system() );
     game_engine->ecs->add_system ( make_calendar_system() );
     game_engine->ecs->add_system ( make_obstruction_system() );
     game_engine->ecs->add_system ( make_flowmap_system() );
     game_engine->ecs->add_system ( make_power_system() );
     game_engine->ecs->add_system ( make_settler_ai_system() );
     game_engine->ecs->add_system ( make_viewshed_system() );
     game_engine->ecs->add_system ( make_renderable_system() );
     
     SDL_Rect all {0, 0, 1024, 48};
     sg.children.push_back( make_unique<scene_blit>( "header", all, all ) );
     sg.children.push_back( make_unique<game_render>() );
}

void play_mode::init()
{
     game_engine->ecs->init();
     game_engine->ecs->load_game("world/saved_game.dat");
     init_systems();
     
     quitting = false;
     world::log.write ( "Welcome to @B_YELLOW@Black Future" );
     world::log.write ( "Use the @B_WHITE@arrow keys@WHITE@ to move, or press @B_RED@Q@WHITE@ to quit." );
     int widx = world::world_idx(2,2);
     world::current_region = new land_block(widx);
}

void play_mode::done()
{
    world::current_region->save();
    game_engine->ecs->save_game("world/saved_game.dat");
    delete world::current_region;
    game_engine->ecs->done();
}

pair< return_mode, unique_ptr< base_mode > > play_mode::tick ( const double time_elapsed )
{
     if ( command::is_key_down ( command::Q ) ) quitting = true;

     if ( quitting ) {
	  return make_pair ( POP, NO_PUSHED_MODE );
     }
     return make_pair ( CONTINUE, NO_PUSHED_MODE );
}
