#include <SDL.h>
#include <lua.hpp>
#include <stdio.h>
#include <hamjet.hpp>
#include <png.h>

#include <HamMath.hpp>

#define WINDOW_WIDTH 350
#define WINDOW_HEIGHT 350

class TraceObject {
public:
	Hamjet::FSurface* surface;
	Hamjet::FVector3 color;

	TraceObject() : color(Hamjet::FVector3(0, 0, 0)) {}
};

class TraceLight {
public:
	Hamjet::FVector3 position;
	Hamjet::FVector3 color;

	TraceLight() : color(Hamjet::FVector3(0, 0, 0)), position(Hamjet::FVector3(0, 0, 0)) {}
};

class Tracer {
public:
	SDL_Renderer* renderer;
	SDL_Texture* texture;

	TraceObject* objects;
	TraceLight light;

	int objCount;

	Hamjet::FVector3 ambient;

public:
	Tracer(SDL_Renderer* r, int objs) : ambient(0.1, 0.2, 0.1) {
		renderer = r;

		texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, WINDOW_WIDTH, WINDOW_HEIGHT);

		objCount = objs;
		objects = new TraceObject[objs];

		light.position = Hamjet::FVector3(0, 5, 3);
	}

	~Tracer() {
		delete[] objects;
	}

	void display() {
		uint32_t* pixels;
		int pitch;

		SDL_LockTexture(texture, NULL, (void**)&pixels, &pitch);

		Hamjet::FSphere sphere = Hamjet::FSphere(Hamjet::FVector3(0, 0, -3), 1);
		Hamjet::FPlane plane = Hamjet::FPlane(Hamjet::FPlane(Hamjet::FVector3::axisy(), -3.0f));

		float aspect = (float)WINDOW_WIDTH / (float)WINDOW_HEIGHT;

		Hamjet::FVector3 camera = Hamjet::FVector3::zero();
		for (int y = 0; y < WINDOW_HEIGHT; y++) {
			float ydir = 1.0f - ((2.0f * y) / (float)WINDOW_HEIGHT);
			for (int x = 0; x < WINDOW_WIDTH; x++) {
				if (y == 141 && x == 160) {
					pitch = pitch;
				}

				float xdir = ((2.0f * aspect * x) / (float)WINDOW_WIDTH) - aspect;

				Hamjet::FVector3 rayDir = Hamjet::FVector3(xdir, ydir, -1);
				Hamjet::FRay ray = Hamjet::FRay(camera, rayDir);
				uint32_t color = 0xFF000000;

				float distance = 100;
				Hamjet::FVector3 normal(0, 0, 0);
				Hamjet::FVector3 hitPoint(0, 0, 0);
				TraceObject* obj = cast(ray, NULL, &distance, &hitPoint, &normal);

				if (obj != NULL) {

					Hamjet::FVector3 h_to_light = light.position.sub(hitPoint);
					Hamjet::FVector3 h_to_light_norm = h_to_light.normalize();

					Hamjet::FRay lightRay = Hamjet::FRay(hitPoint, h_to_light_norm);

					float lightDistance = 100;
					Hamjet::FVector3 lightNormal(0, 0, 0);
					Hamjet::FVector3 lightHitPoint(0, 0, 0);
					TraceObject* lightObj = cast(lightRay, obj, &lightDistance, &lightHitPoint, &lightNormal);

					if (lightObj == NULL || lightDistance > h_to_light.magnitude()) {
						float lightStrength = fmax(h_to_light_norm.dot(normal), 0);
						color = vecToARGB32(obj->color.mul(lightStrength).add(obj->color.mul(ambient)));
					}
				}


				uint32_t* pixel = pixels + (y * (pitch / 4)) + x;
				*pixel = color;
			}
		}

		SDL_UnlockTexture(texture);

		SDL_RenderCopy(renderer, texture, NULL, NULL);
	}

	TraceObject* cast(Hamjet::FRay& ray, TraceObject* exclude, float* d, Hamjet::FVector3* hitPoint, Hamjet::FVector3* normal) {
		float distance = 100;
		TraceObject* obj = NULL;
		Hamjet::FVector3 norm(0, 0, 0);

		for (int i = 0; i < objCount; i++) {
			float newDistance;
			Hamjet::FVector3 newNorm(0, 0, 0);
			if (&objects[i] != exclude && objects[i].surface->intersect(ray, &newDistance, &newNorm)) {
				if (newDistance < distance) {
					distance = newDistance;
					norm = newNorm;
					obj = &objects[i];
				}
			}
		}

		if (obj != NULL) {
			*d = distance;
			*hitPoint = ray.origin.add(ray.direction.mul(distance));
			*normal = norm;
		}

		return obj;
	}

	static inline uint32_t vecToARGB32(Hamjet::FVector3& vec) {
		return 0xFF000000 |
			((int)(fmax(fmin(vec.x, 1), 0) * 255) % 256) << 16 |
			((int)(fmax(fmin(vec.y, 1), 0) * 255) % 256) << 8 |
			((int)(fmax(fmin(vec.z, 1), 0) * 255) % 256);
	}

	Hamjet::FRay rayForPixel(int x, int y) {
		float aspect = (float)WINDOW_WIDTH / (float)WINDOW_HEIGHT;
		float ydir = 1.0f - ((2.0f * y) / (float)WINDOW_HEIGHT);
		float xdir = ((2.0f * aspect * x) / (float)WINDOW_WIDTH) - aspect;

		return Hamjet::FRay(Hamjet::FVector3::zero(), Hamjet::FVector3(xdir, ydir, -1));
	}
};

class BallState {
public:
	float decayRate;
	float intensity;
	Hamjet::FVector3 baseColor;

public:
	BallState(Hamjet::FVector3& color) : baseColor(color) {
		reset();
	}

	bool update(float dt) {
		intensity -= decayRate * dt;

		if (intensity < 0) {
			return false;
		}
		else {
			return true;
		}
	}

	Hamjet::FVector3 currentColor() {
		return baseColor.mul(intensity);
	}

	void click() {
		intensity = 1;
		decayRate += (((double)rand() / (RAND_MAX)) / 10);
	}

	void reset() {
		decayRate = (((double)rand() / (RAND_MAX)) / 5) + 0.01;
		intensity = 1;
	}
};

int main(int argc, char** argv) {
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
		printf("Error %s\n", SDL_GetError());
		return 1;
	}

	SDL_Window *win = SDL_CreateWindow("Rays!", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
		WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_SHOWN);
	if (win == 0) {
		SDL_Quit();
		return 1;
	}

	SDL_Renderer *ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED);
	if (ren == 0) {
		SDL_DestroyWindow(win);
		SDL_Quit();
		return 1;
	}

	Tracer t(ren, 4);

	BallState bs1(Hamjet::FVector3(1, 0, 0));
	BallState bs2(Hamjet::FVector3(0, 1, 0));
	BallState bs3(Hamjet::FVector3(0, 0, 1));

	Hamjet::FSphere s1 = Hamjet::FSphere(Hamjet::FVector3(0, 0, -3), 1);
	Hamjet::FSphere s2 = Hamjet::FSphere(Hamjet::FVector3(0, 0, -5), 1);
	Hamjet::FSphere s3 = Hamjet::FSphere(Hamjet::FVector3(-2, 0, -3), 1);

	t.objects[0].surface = new Hamjet::FPlane(Hamjet::FPlane(Hamjet::FVector3::axisy(), -1.0f));
	t.objects[0].color = Hamjet::FVector3(0.5, 0.5, 0.5);
	t.objects[1].surface = &s1;
	t.objects[1].color = Hamjet::FVector3(1, 0, 0);
	t.objects[2].surface = &s2;
	t.objects[2].color = Hamjet::FVector3(0, 0, 1);
	t.objects[3].surface = &s3;
	t.objects[3].color = Hamjet::FVector3(0, 1, 0);

	bool cont = true;

	uint32_t lastTime = SDL_GetTicks();
	int frameCount = 0;


	uint32_t lastFrameTime = SDL_GetTicks();
	float angle = 0;

	while (cont) {
		uint32_t thisTime = SDL_GetTicks();
		uint32_t timeDelta = thisTime - lastFrameTime;
		lastFrameTime = thisTime;

		float dt = (fmax(timeDelta, 1) / 1000.0f);

		if (!bs1.update(dt) || !bs2.update(dt) || !bs3.update(dt)) {
			bs1.reset();
			bs2.reset();
			bs3.reset();
			printf("Reseting\n");
		}

		t.objects[1].color = bs1.currentColor();
		t.objects[2].color = bs2.currentColor();
		t.objects[3].color = bs3.currentColor();

		s1.origin.x = 1.5f * cos(angle);
		s1.origin.z = 1.5f * sin(angle) - 3;

		s2.origin.x = 1.5f * cos(angle + (M_PI * 2.0f / 3.0f));
		s2.origin.z = 1.5f * sin(angle + (M_PI * 2.0f / 3.0f)) - 3;

		s3.origin.x = 1.5f * cos(angle + 2 * (M_PI * 2.0f / 3.0f));
		s3.origin.z = 1.5f * sin(angle + 2 * (M_PI * 2.0f / 3.0f)) - 3;


		t.display();
		SDL_RenderPresent(ren);

		frameCount++;
		if (frameCount == 100) {
			frameCount = 0;
			uint32_t newTime = SDL_GetTicks();
			printf("Frame time: %d ms\n", (newTime - lastTime) / 100);
			lastTime = newTime;
		}

		SDL_Event e;
		while (SDL_PollEvent(&e)) {
			if (e.type == SDL_QUIT) {
				cont = false;
				break;
			}
			else if (e.type == SDL_MOUSEBUTTONDOWN) {
				Hamjet::FRay pixelRay = t.rayForPixel(e.button.x, e.button.y);
				float clickDistance = 100;
				Hamjet::FVector3 clickNormal(0, 0, 0);
				Hamjet::FVector3 clickHitPoint(0, 0, 0);
				TraceObject* lightObj = t.cast(pixelRay, NULL, &clickDistance, &clickHitPoint, &clickNormal);

				if (lightObj == &t.objects[1]) {
					bs1.click();
				}
				else if (lightObj == &t.objects[2]) {
					bs2.click();
				}
				else if (lightObj == &t.objects[3]) {
					bs3.click();
				}
			}
		}

		float rotate_speed = 0.6f * dt;
		const Uint8 *state = SDL_GetKeyboardState(NULL);
		if (state[SDL_SCANCODE_LEFT]) {
			angle += rotate_speed;
		}
		else if (state[SDL_SCANCODE_RIGHT]) {
			angle -= rotate_speed;
		}
	}

	SDL_DestroyRenderer(ren);
	SDL_DestroyWindow(win);
	SDL_Quit();

	SDL_Quit();
	return 0;
}
