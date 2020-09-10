#include <iostream>
#include <vector>
#include <queue>
#include <cmath>

#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glut.h>

int AMPLITUDE = 24000;
const int FREQUENCY = 44100;

const double semitone = 1.059463077981413;
const double C = 16.351598, D = 18.354048, E = 20.601722,
			 F = 21.826765, G = 24.499715, A = 27.5, B = 30.867706;

struct BeepObject {
	std::vector<double> freqs;
	int samplesLeft;
};

class Beeper {
	private:
		double v;
		std::queue<BeepObject> beeps;
	public:
		Beeper();
		~Beeper();
		void beep(std::vector<double>& freqs, double duration);
		void generateSamples(Sint16 *stream, int length);
		void clearSamples();
};

void audio_callback(void*, Uint8*, int);

Beeper::Beeper() {
	SDL_AudioSpec desiredSpec;

	desiredSpec.freq = FREQUENCY;
	desiredSpec.format = AUDIO_S16SYS;
	desiredSpec.channels = 1;
	desiredSpec.samples = 2048;
	desiredSpec.callback = audio_callback;
	desiredSpec.userdata = this;

	SDL_AudioSpec obtainedSpec;
	SDL_OpenAudio(&desiredSpec, &obtainedSpec);
	SDL_PauseAudio(0);

	v = 0;
}

Beeper::~Beeper() {
	SDL_CloseAudio();
}

void Beeper::generateSamples(Sint16 *stream, int length) {
	int i = 0;

	while (i < length) {
		if (beeps.empty()) {
			while (i < length) {
				stream[i] = 0;
				i++;
			} return;
		}
		BeepObject& bo = beeps.front();

		int samplesToDo = std::min(i + bo.samplesLeft, length);
		bo.samplesLeft -= samplesToDo - i;

		int frequencies = bo.freqs.size();
		while (i < samplesToDo) {
			for (int n = 0; n < frequencies; n++) {
				stream[i++] = AMPLITUDE * (std::sin(v * 2 * std::acos(-1.0) / FREQUENCY * (bo.freqs[n] / bo.freqs[0])) <= 0 ? -1 : 1);
				v += bo.freqs[0];
			}
		}

		if (bo.samplesLeft == 0) {
			beeps.pop();
		}
	}
}

void Beeper::clearSamples() {
	if (beeps.size() > 0)
		beeps.pop();
}

void Beeper::beep(std::vector<double>& freqs, double duration) {
	BeepObject bo;
	bo.freqs = freqs;
	bo.samplesLeft = duration * FREQUENCY;

	beeps.push(bo);
}

void audio_callback(void *_beeper, Uint8 *_stream, int _length) {
	Sint16 *stream = (Sint16*) _stream;
	int length = _length / 2;
	Beeper* beeper = (Beeper*) _beeper;

	beeper->generateSamples(stream, length);
}

bool keys[256];
bool keypress = false, quit = false, toggle_fullscreen = false;
int octave_modifier = 0;
SDL_Window *window = nullptr;

void event_handler(Beeper *beeper)
{
	SDL_Event event;

	while (SDL_PollEvent(&event) && !quit) {
		switch (event.type) {
			case SDL_QUIT:
				quit = true;
				break;
			case SDL_KEYDOWN:
				switch (event.key.keysym.sym) {
					case SDLK_ESCAPE:
						quit = true;
						break;
					case SDLK_F12:
						glClearDepth(1.0);
						glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);					
						glBegin(GL_POINTS);
						
						glEnd();
						SDL_GL_SwapWindow(window);
						break;
					case SDLK_F11:
						toggle_fullscreen = true;
						break;
					case SDLK_F10:
						SDL_SetWindowSize(window, 1920, 1080);
						break;
					case SDLK_F9:
						SDL_SetWindowSize(window, 640, 480);
						break;
					case SDLK_UP:
						octave_modifier += 1;
						break;
					case SDLK_DOWN:
						octave_modifier -= 1;
						break;
				}

				if (event.key.keysym.sym < 256)
					keys[event.key.keysym.sym] = true;
				keypress = true;
				break;
			case SDL_KEYUP:
				if (event.key.keysym.sym < 256)
					keys[event.key.keysym.sym] = false;
				keypress = false;
				beeper->clearSamples();
				break;
		}
	}
}

std::vector<double> get_tones(std::vector<std::string>& note_infos) {
	double tone, semitone = 1.059463077981413, C = 16.351598, D = 18.354048,
		E = 20.601722, F = 21.826765, G = 24.499715, A = 27.5, B = 30.867706;
	std::vector<double> tones;
	tones.clear();

	char note, accidental;
	int octave;
	for (int i = 0; i < note_infos.size(); i++) {
		note = note_infos[i][0];
		accidental = note_infos[i][1];
		octave = note_infos[i][2] - '0';

		switch (note) {
			case 'C':
				tone = C * (1 << octave);
				break;
			case 'D':
				tone = D * (1 << octave);
				break;
			case 'E':
				tone = E * (1 << octave);
				break;
			case 'F':
				tone = F * (1 << octave);
				break;
			case 'G':
				tone = G * (1 << octave);
				break;
			case 'A':
				tone = A * (1 << octave);
				break;
			case 'B':
				tone = B * (1 << octave);
				break;
			default:
				tone = 0;
				break;
		}
			
		if (accidental == 'S' || accidental == 's')
			tone *= semitone;

		tones.push_back(tone);
	}
	
	return tones;
}

void play_tone(Beeper *beeper)
{
	std::vector<std::string> tone_strs;
	std::string tmp;

	tone_strs.clear();
	for (int key = 32; key < 127; key++) {
		if (!(key == ',' || key == '.' || key >= '1' && key <= '9' || key >= 'a' && key <= 'z' && key != 'p'))
			continue;

		if (keys['1'] && key == '1')
			tmp = "Cn3";
		else if (keys['q'] && key == 'q')
			tmp = "Cs3";
		else if (keys['a'] && key == 'a')
			tmp = "Dn3";
		else if (keys['z'] && key == 'z')
			tmp = "Ds3";
		else if (keys['2'] && key == '2')
			tmp = "En3";
		else if (keys['w'] && key == 'w')
			tmp = "Fn3";
		else if (keys['s'] && key == 's')
			tmp = "Fs3";
		else if (keys['x'] && key == 'x')
			tmp = "Gn3";
		else if (keys['3'] && key == '3')
			tmp = "Gs3";
		else if (keys['e'] && key == 'e')
			tmp = "An3";
		else if (keys['d'] && key == 'd')
			tmp = "As3";
		else if (keys['c'] && key == 'c')
			tmp = "Bn3";
		else if (keys['4'] && key == '4')
			tmp = "Cn4";
		else if (keys['r'] && key == 'r')
			tmp = "Cs4";
		else if (keys['f'] && key == 'f')
			tmp = "Dn4";
		else if (keys['v'] && key == 'v')
			tmp = "Ds4";
		else if (keys['5'] && key == '5')
			tmp = "En4";
		else if (keys['t'] && key == 't')
			tmp = "Fn4";
		else if (keys['g'] && key == 'g')
			tmp = "Fs4";
		else if (keys['b'] && key == 'b')
			tmp = "Gn4";
		else if (keys['6'] && key == '6')
			tmp = "Gs4";
		else if (keys['y'] && key == 'y')
			tmp = "An4";
		else if (keys['h'] && key == 'h')
			tmp = "As4";
		else if (keys['n'] && key == 'n')
			tmp = "Bn4";
		else if (keys['7'] && key == '7')
			tmp = "Cn5";
		else if (keys['u'] && key == 'u')
			tmp = "Cs5";
		else if (keys['j'] && key == 'j')
			tmp = "Dn5";
		else if (keys['m'] && key == 'm')
			tmp = "Ds5";
		else if (keys['8'] && key == '8')
			tmp = "En5";
		else if (keys['i'] && key == 'i')
			tmp = "Fn5";
		else if (keys['k'] && key == 'k')
			tmp = "Fs5";
		else if (keys[','] && key == ',')
			tmp = "Gn5";
		else if (keys['9'] && key == '9')
			tmp = "Gs5";
		else if (keys['o'] && key == 'o')
			tmp = "An5";
		else if (keys['l'] && key == 'l')
			tmp = "As5";
		else if (keys['.'] && key == '.')
			tmp = "Bn5";
		
		if (!tmp.empty()) {
			tmp[2] += octave_modifier;
			tone_strs.push_back(tmp);
			tmp = std::string();
		}
	}

	if (tone_strs.size() > 0) {
		auto tones = get_tones(tone_strs);
		SDL_LockAudio();
		beeper->clearSamples();
		beeper->beep(tones, 0.1);
		SDL_UnlockAudio();
	}
}

int main(int argc, char **argv)
{
	std::cout << "F12 - fullscreen\nKeys:\n\t1 - C3, q - C#3, a - D3, z - D#3, 2 - E3, etc.\n";
	std::cout << "\tThe notes go up to B5 on the '.' key\nPress Esc to exit the piano\n";

	for(int i = 0; i < 322; i++)
		keys[i] = false;

	SDL_Init(SDL_INIT_EVERYTHING);
	window = SDL_CreateWindow("smoki window", 0, 0, 640, 480, SDL_WINDOW_OPENGL);

	SDL_GLContext ctx = SDL_GL_CreateContext(window);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	glClearColor(0, 0, 0, 1.0);

	glClearDepth(1.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	SDL_GL_SwapWindow(window);

	Beeper *beeper = new Beeper;

	while (!quit) {
		event_handler(beeper);
		play_tone(beeper);

		if (toggle_fullscreen) {
			if (!(SDL_GetWindowFlags(window) & SDL_WINDOW_FULLSCREEN))
				SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN);
			else 
				SDL_SetWindowFullscreen(window, SDL_WINDOW_MAXIMIZED);
			
			toggle_fullscreen = false;
		}
			
	}

	delete beeper;
	SDL_DestroyWindow(window);
	SDL_Quit();
	return 0;
}
