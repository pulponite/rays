#include <Hamjet/Math.hpp>
#include <Hamjet/Engine.hpp>
#include <Hamjet/Raytracer.hpp>

#include <stdio.h>

#define WINDOW_WIDTH 350
#define WINDOW_HEIGHT 350

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

class RayApp : public Hamjet::Application {
private:
	Hamjet::Engine* engine;

	Hamjet::Tracer tracer;

	BallState bs1;
	BallState bs2;
	BallState bs3;

	Hamjet::FSphere s1;
	Hamjet::FSphere s2;
	Hamjet::FSphere s3; 
	
	float angle = 0;

	SDL_Texture* texture;

public:
	RayApp(Hamjet::Engine* e) : engine(e),
		tracer(4),
		bs1(Hamjet::FVector3(1, 0, 0)),
		bs2(Hamjet::FVector3(0, 1, 0)),
		bs3(Hamjet::FVector3(0, 0, 1)), 
		s1(Hamjet::FVector3::zero(), 1),
		s2(Hamjet::FVector3::zero(), 1), 
		s3(Hamjet::FVector3::zero(), 1) {

		tracer.objects[0].surface = new Hamjet::FPlane(Hamjet::FPlane(Hamjet::FVector3::axisy(), -1.0f));
		tracer.objects[0].color = Hamjet::FVector3(0.5, 0.5, 0.5);
		tracer.objects[1].surface = &s1;
		tracer.objects[1].color = Hamjet::FVector3(1, 0, 0);
		tracer.objects[2].surface = &s2;
		tracer.objects[2].color = Hamjet::FVector3(0, 0, 1);
		tracer.objects[3].surface = &s3;
		tracer.objects[3].color = Hamjet::FVector3(0, 1, 0);

		texture = SDL_CreateTexture(engine->windowRenderer, 
			SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, 
			WINDOW_WIDTH, WINDOW_HEIGHT);
	}

	virtual bool update(float dt) {
		if (!bs1.update(dt) || !bs2.update(dt) || !bs3.update(dt)) {
			bs1.reset();
			bs2.reset();
			bs3.reset();
			printf("Reseting\n");
		}

		tracer.objects[1].color = bs1.currentColor();
		tracer.objects[2].color = bs2.currentColor();
		tracer.objects[3].color = bs3.currentColor();

		s1.origin.x = 1.5f * cos(angle);
		s1.origin.z = 1.5f * sin(angle) - 3;

		s2.origin.x = 1.5f * cos(angle + (M_PI * 2.0f / 3.0f));
		s2.origin.z = 1.5f * sin(angle + (M_PI * 2.0f / 3.0f)) - 3;

		s3.origin.x = 1.5f * cos(angle + 2 * (M_PI * 2.0f / 3.0f));
		s3.origin.z = 1.5f * sin(angle + 2 * (M_PI * 2.0f / 3.0f)) - 3;

		float rotate_speed = 0.6f * dt;
		const Uint8 *state = SDL_GetKeyboardState(NULL);
		if (state[SDL_SCANCODE_LEFT]) {
			angle += rotate_speed;
		}
		else if (state[SDL_SCANCODE_RIGHT]) {
			angle -= rotate_speed;
		}

		return true;
	}

	virtual void draw() {
		uint32_t* pixels;
		int pitch;

		SDL_LockTexture(texture, NULL, (void**)&pixels, &pitch);
		tracer.display(pixels, WINDOW_WIDTH, WINDOW_HEIGHT, pitch);
		SDL_UnlockTexture(texture);

		SDL_RenderCopy(engine->windowRenderer, texture, NULL, NULL);
		SDL_RenderPresent(engine->windowRenderer);
	}

	virtual void onClick(int x, int y) {
		Hamjet::FRay pixelRay = tracer.rayForPixel(x, y, WINDOW_WIDTH, WINDOW_HEIGHT);
		float clickDistance = 100;
		Hamjet::FVector3 clickNormal(0, 0, 0);
		Hamjet::FVector3 clickHitPoint(0, 0, 0);
		Hamjet::TraceObject* lightObj = tracer.cast(pixelRay, NULL, &clickDistance, &clickHitPoint, &clickNormal);

		if (lightObj == &tracer.objects[1]) {
			bs1.click();
		}
		else if (lightObj == &tracer.objects[2]) {
			bs2.click();
		}
		else if (lightObj == &tracer.objects[3]) {
			bs3.click();
		}
	}
};

int main(int argc, char** argv) {
	Hamjet::Engine e;
	if (!e.init(WINDOW_WIDTH, WINDOW_HEIGHT)) {
		e.term();
		return 1;
	}

	RayApp app(&e);
	e.run(&app);

	e.term();
	return 0;
}
