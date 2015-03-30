#include <string>
#include <iostream>

#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>

#include "options.hpp"
#include "render_context.hpp"
#include "imgui_sdlbackend.hpp"
#include "pokedex_screen.hpp"
#include "text.hpp"

using options::WINDOW_WIDTH;
using options::WINDOW_HEIGHT;

bool PokedexScreen::initialize(RenderContext *context, ScreenDispatcher *dispatcher)
{
	if (!Screen::initialize(context, dispatcher)) {
		return false;
	}

	m_font = TTF_OpenFont("assets/unifont-7.0.06.ttf", 16);
	if (m_font == nullptr) {
		std::cerr << "TTF_OpenFont Error: " << TTF_GetError() << std::endl;
		return false;
	}

	m_pokeData.setPokemon(1);
	m_currentPokemonID = m_pokeData.getID();
	setPokedexData(m_currentPokemonID);

	// initialize the user interface
	m_userInterface.setRenderBackend(std::make_unique<imgui::SDLRenderBackend>(m_context->renderer));
	return true;
}

void PokedexScreen::setPokedexData(int id)
{
	if (m_textTexture) {
		SDL_DestroyTexture(m_textTexture);
	}

	m_pokeData.setPokemon(id);
	m_pokeData.setType(m_pokeData.getTypeID1(), m_pokeData.getTypeID2());

	// Pokemon data
	{
		std::string txt = "Name: " + m_pokeData.getName()
			+ "\n" + "Base HP: " + std::to_string(m_pokeData.getBaseHP())
			+ "\n" + "Base Att: " + std::to_string(m_pokeData.getBaseAtt())
			+ "\n" + "Base Def: " + std::to_string(m_pokeData.getBaseDef())
			+ "\n" + "Base Sp Att: " + std::to_string(m_pokeData.getBaseSpAtt())
			+ "\n" + "Base Sp Def: " + std::to_string(m_pokeData.getBaseSpDef());

		txt.append("\nTypes: " + m_pokeData.getTypeName(m_pokeData.getTypeID1()));
		if(m_pokeData.getTypeID2() != 0){
			txt.append(", " + m_pokeData.getTypeName(m_pokeData.getTypeID2()));
		}
		txt.append("\n\n"  + m_pokeData.getFlavorText());
		txt.append("\n\nWeak To: ");
		{
			std::vector<int> weakTo = m_pokeData.getTypesWeakTo();
			std::vector<int>::const_iterator it = weakTo.begin();
			if(it != weakTo.end()){
				while(it != weakTo.end()-1){
					txt.append(m_pokeData.getTypeName(*it) + ", ");
					++it;
				}
				txt.append(m_pokeData.getTypeName(*it));
			}
		}
		txt.append("\nDouble Weak To: ");
		{
			std::vector<int> doubleWeakTo = m_pokeData.getTypesDoubleWeakTo();
			std::vector<int>::const_iterator it = doubleWeakTo.begin();
			if(it != doubleWeakTo.end()){
				while(it != doubleWeakTo.end()-1){
					txt.append(m_pokeData.getTypeName(*it) + ", ");
					++it;
				}
				txt.append(m_pokeData.getTypeName(*it));
			}
		}
		txt.append("\nResistant To: ");
		{
			std::vector<int> resistantTo = m_pokeData.getTypesResistantTo();
			std::vector<int>::const_iterator it = resistantTo.begin();
			if(it != resistantTo.end()){
				while(it != resistantTo.end()-1){
					txt.append(m_pokeData.getTypeName(*it) + ", ");
					++it;
				}
				txt.append(m_pokeData.getTypeName(*it));
			}
		}
		txt.append("\nDouble Resistant To: ");
		{
			std::vector<int> doubleResistantTo = m_pokeData.getTypesDoubleResistantTo();
			std::vector<int>::const_iterator it = doubleResistantTo.begin();
			if(it != doubleResistantTo.end()){
				while(it != doubleResistantTo.end()-1){
					txt.append(m_pokeData.getTypeName(*it) + ", ");
					++it;
				}
				txt.append(m_pokeData.getTypeName(*it));
			}
		}
		txt.append("\nImmune To: ");
		{
			std::vector<int> immuneTo = m_pokeData.getTypesImmuneTo();
			std::vector<int>::const_iterator it = immuneTo.begin();
			if(it != immuneTo.end()){
				while(it != immuneTo.end()-1){
					txt.append(m_pokeData.getTypeName(*it) + ", ");
					++it;
				}
				txt.append(m_pokeData.getTypeName(*it));
			}
		}
		txt.append("\nDamaged Normally by: ");
		{
			std::vector<int> normalTo = m_pokeData.getTypesDamagedNormallyBy();
			std::vector<int>::const_iterator it = normalTo.begin();
			if(it != normalTo.end()){
				while(it != normalTo.end()-1){
					txt.append(m_pokeData.getTypeName(*it) + ", ");
					++it;
				}
				txt.append(m_pokeData.getTypeName(*it));
			}
		}

		const SDL_Color white = {255, 255, 255, 255};
		int textStartX = WINDOW_WIDTH/3;
		m_textTexture = getTextRenderTexture(m_context->renderer, m_font, txt, white, WINDOW_WIDTH - textStartX);
		m_pokemonName.setImage(m_textTexture);
		m_pokemonName.setPosition(WINDOW_WIDTH/3, 0);
	}

	// Pokemon image
	{
		m_pokemonImage = m_context->loadTexture(m_pokeData.getSpriteLocation());
		m_pokemonImage.setPosition(0, 0);
		m_pokemonImage.setScale(2.0f);
	}

	// Create window
	{
		m_windowTarget = SDL_CreateTexture(m_context->renderer,
			SDL_PIXELFORMAT_RGBA8888,
			SDL_TEXTUREACCESS_TARGET,
			200, WINDOW_HEIGHT-(155+20)-4);

		uint8_t r, g, b, a;
		SDL_GetRenderDrawColor(m_context->renderer, &r, &g, &b, &a);
		SDL_SetRenderDrawColor(m_context->renderer, 128, 0, 128, 255);
		SDL_SetRenderTarget(m_context->renderer, m_windowTarget);
		SDL_RenderClear(m_context->renderer);
		SDL_SetRenderTarget(m_context->renderer, nullptr);
		SDL_SetRenderDrawColor(m_context->renderer, r, g, b, a);

		m_window.setImage(m_windowTarget);
		m_window.setPosition(4, 154+20);
	}

	// Create text for window
	{
		const SDL_Color white = {255, 255, 255, 255};

		PokemonData::Characteristics characteristics;
		auto pokeList = m_pokeData.getPokemonWithCharacteristics(characteristics);
		m_totalPokemon = static_cast<int>(pokeList.size());

		std::string names;
		for (auto i : pokeList) {
			m_pokeData.setPokemon(i);
			names += m_pokeData.getName() + "\n";
		}
		m_textTestTexture = getTextRenderTexture(
			m_context->renderer,
			m_font,
			names, white, 200+4+2);
		m_textTest.setImage(m_textTestTexture);
		m_textTest.setPosition(2, 2);
	}
}

void PokedexScreen::handleEvent(const SDL_Event &sdlEvent)
{
	switch(sdlEvent.type) {
		case SDL_MOUSEMOTION:
			m_userInterface.mouseX = sdlEvent.motion.x;
			m_userInterface.mouseY = sdlEvent.motion.y;
			break;
		case SDL_MOUSEBUTTONDOWN:
			if (sdlEvent.button.button == SDL_BUTTON_LEFT) {
				m_userInterface.mouseDown = true;
			}
			break;
		case SDL_MOUSEBUTTONUP:
			if (sdlEvent.button.button == SDL_BUTTON_LEFT)
				m_userInterface.mouseDown = false;
			break;
		case SDL_KEYDOWN:
			switch (sdlEvent.key.keysym.sym) {
				case SDLK_EQUALS:
				case SDLK_KP_PLUS:
					m_currentPokemonID = (m_currentPokemonID % m_pokeData.numPokemon()) + 1;
					setPokedexData(m_currentPokemonID);
					break;
				case SDLK_MINUS:
				case SDLK_KP_MINUS:
					m_currentPokemonID--;
					if (m_currentPokemonID < 1) {
						m_currentPokemonID = m_pokeData.numPokemon();
					}
					setPokedexData(m_currentPokemonID);
					break;
			}
			break;
	}

	m_userInterface.handleEvent(sdlEvent);
}

PokedexScreen::~PokedexScreen()
{
	SDL_DestroyTexture(m_windowTarget);
}

void PokedexScreen::frameStep(unsigned long)
{
	SDL_RenderClear(m_context->renderer);
	m_context->render(m_pokemonName);
	m_context->render(m_pokemonImage);

	m_userInterface.begin();
	if (m_userInterface.textField(1, 4, 150, 200, 20, m_textField)) {
	}

	// window
	{
		SDL_SetRenderTarget(m_context->renderer, m_window.texture());
		uint8_t r, g, b, a;
		SDL_GetRenderDrawColor(m_context->renderer, &r, &g, &b, &a);
		SDL_SetRenderDrawColor(m_context->renderer, 128, 0, 128, 255);

		SDL_RenderClear(m_context->renderer);
		m_context->render(m_textTest);

		SDL_SetRenderDrawColor(m_context->renderer, r, g, b, a);
		SDL_SetRenderTarget(m_context->renderer, nullptr);

		m_context->render(m_window);
	}

	// scroll
	if (m_userInterface.scrollBar(2, 204, 154+20, WINDOW_HEIGHT-(155+20)-4-24, m_totalPokemon*15, &m_scroll)) {
		m_textTest.setPosition(2, -m_scroll+2);
	}
	m_userInterface.end();

	SDL_RenderPresent(m_context->renderer);
}
