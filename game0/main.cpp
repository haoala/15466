#include "Draw.hpp"
#include "GL.hpp"

#include <SDL.h>
#include <glm/glm.hpp>

#include <chrono>
#include <iostream>

int main(int argc, char **argv) {
	//Configuration:
	struct {
		std::string title = "Stacko";
		glm::uvec2 size = glm::uvec2(500, 500);
	} config;

	//------------  initialization ------------

	//Initialize SDL library:
	SDL_Init(SDL_INIT_VIDEO);

	//Ask for an OpenGL context version 3.3, core profile, enable debug:
	SDL_GL_ResetAttributes();
	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);

	//create window:
	SDL_Window *window = SDL_CreateWindow(
		config.title.c_str(),
		SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
		config.size.x, config.size.y,
		SDL_WINDOW_OPENGL /*| SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI*/
	);

	if (!window) {
		std::cerr << "Error creating SDL window: " << SDL_GetError() << std::endl;
		return 1;
	}

	//Create OpenGL context:
	SDL_GLContext context = SDL_GL_CreateContext(window);

	if (!context) {
		SDL_DestroyWindow(window);
		std::cerr << "Error creating OpenGL context: " << SDL_GetError() << std::endl;
		return 1;
	}

	#ifdef _WIN32
	//On windows, load OpenGL extensions:
	if (!init_gl_shims()) {
		std::cerr << "ERROR: failed to initialize shims." << std::endl;
		return 1;
	}
	#endif

	//Set VSYNC + Late Swap (prevents crazy FPS):
	if (SDL_GL_SetSwapInterval(-1) != 0) {
		std::cerr << "NOTE: couldn't set vsync + late swap tearing (" << SDL_GetError() << ")." << std::endl;
		if (SDL_GL_SetSwapInterval(1) != 0) {
			std::cerr << "NOTE: couldn't set vsync (" << SDL_GetError() << ")." << std::endl;
		}
	}

	//Hide mouse cursor (note: showing can be useful for debugging):
	//SDL_ShowCursor(SDL_DISABLE);

	//------------  game state ------------

	const float block_height = 0.2;
	const glm::u8vec4 block_color(0xff, 0xff, 0xff, 0xff);
	
	glm::vec2 block_velocity = glm::vec2(2.0, 0.0);

	struct Block {
		glm::vec2 pos;
		float half_width = 0.2;

		Block():
			pos(0.0, -0.9)
		{}

		Block(float height):
			pos(0.0, height)
		{}

		float left() const {
			return pos.x - half_width;
		}

		float right() const {
			return pos.x + half_width;
		}
	};

	std::vector<Block> placed_blocks(1);
	Block moving_block(-1 + 3 * block_height / 2.0);

	//------------  game loop ------------

	std::cout << "Get to the top of the screen to win!" << std::endl;

	auto previous_time = std::chrono::high_resolution_clock::now();
	bool should_quit = false;
	while (true) {
		static SDL_Event evt;
		while (SDL_PollEvent(&evt) == 1) {
			//handle input:
			if (evt.type == SDL_MOUSEBUTTONDOWN) {
				// Calculate overlap between moving block and last placed block
				const Block& last_placed_block = placed_blocks.back();

				const float new_left = std::max(last_placed_block.left(), moving_block.left());
				const float new_right = std::min(last_placed_block.right(), moving_block.right());

				const float overlap = new_right - new_left;

				if (overlap > 0) { // Add block
					// Do stuff
					moving_block.pos.x = (new_left + new_right) / 2.0;
					moving_block.half_width = overlap / 2.0;
					placed_blocks.push_back(moving_block);
					if (placed_blocks.size() == 10) {
						std::cout << "You win!" << std::endl;
						should_quit = true;
						break;
					}
					moving_block.pos.x += block_height;
					moving_block.pos.y += block_height;

				} else { // Game over!
					std::cout << "You lose!" << std::endl;
					should_quit = true;
					break;
				}

			} else if (evt.type == SDL_KEYDOWN && evt.key.keysym.sym == SDLK_ESCAPE) {
				should_quit = true;
			} else if (evt.type == SDL_QUIT) {
				should_quit = true;
				break;
			}
		}
		if (should_quit) break;

		auto current_time = std::chrono::high_resolution_clock::now();
		float elapsed = std::chrono::duration< float >(current_time - previous_time).count();
		previous_time = current_time;

		{ //update game state:
			moving_block.pos += elapsed * block_velocity;
			if (moving_block.pos.x - moving_block.half_width < -1) block_velocity.x = fabs(block_velocity.x);
			if (moving_block.pos.x + moving_block.half_width >  1) block_velocity.x =-fabs(block_velocity.x);
		}

		//draw output:
		glClearColor(0.0, 0.0, 0.0, 0.0);
		glClear(GL_COLOR_BUFFER_BIT);


		{ //draw game state:
			Draw draw;
			// Draw placed blocks
			for (const Block& block: placed_blocks) {
				draw.add_rectangle(block.pos - glm::vec2(block.half_width, block_height / 2.0), block.pos + glm::vec2(block.half_width, block_height / 2.0), block_color);
			}

			// Draw moving block
			draw.add_rectangle(moving_block.pos - glm::vec2(moving_block.half_width, block_height / 2.0), moving_block.pos + glm::vec2(moving_block.half_width, block_height / 2.0), block_color);

			draw.draw();
		}


		SDL_GL_SwapWindow(window);
	}


	//------------  teardown ------------

	SDL_GL_DeleteContext(context);
	context = 0;

	SDL_DestroyWindow(window);
	window = NULL;

	return 0;
}
