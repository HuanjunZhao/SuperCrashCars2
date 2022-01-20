#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "Time.h"

#include "Geometry.h"
#include "GLDebug.h"
#include "Log.h"
#include "Window.h"

#include "glm/glm.hpp"
#include "glm/gtc/type_ptr.hpp"

#include "InputManager.h"
#include "Camera.h"
#include "GLMesh.h"

#include "PVehicle.h"
#include "PDynamic.h"
#include "PStatic.h"


/*
	TODO:
		- Calculate vehicle acceleration.
		- Create better player camera. (find a way to get the car's angle).
		- Learn about convex hulls.
		- Create differnt colliders for each new car. (after learning convex hulls).
*/

int main(int argc, char** argv) {
	Log::info("Starting Game...");

	// OpenGL
	glfwInit();
	Window window(Utils::instance().SCREEN_WIDTH, Utils::instance().SCREEN_HEIGHT, "Super Crash Cars 2");
	Utils::instance().shader = std::make_shared<ShaderProgram>("shaders/shader_vertex.vert", "shaders/shader_fragment.frag");
	std::shared_ptr<InputManager> inputManager = std::make_shared<InputManager>(Utils::instance().SCREEN_WIDTH, Utils::instance().SCREEN_HEIGHT);
	window.setCallbacks(inputManager);

	Camera editorCamera = Camera(Utils::instance().SCREEN_WIDTH, Utils::instance().SCREEN_HEIGHT, glm::vec3(-2.0f, 4.0f, 10.0f));

	GLMesh obstacleMesh(GL_FILL), ball(GL_FILL);
	obstacleMesh.createCube(1.0f, glm::vec3(0.0f, 1.0f, 0.0f));
	ball.createSphere(1.0f, 25, glm::vec3(1.0f, 0.0f, 0.0f));
	Model ground = Model("models/ground/ground.obj");
	ground.translate(glm::vec3(0.0f, -1.2f, 0.0f));
	ground.scale(glm::vec3(2.0f, 1.0f, 2.0f));

	// Physx
	float throttle = 1.0f;
	PhysicsManager pm = PhysicsManager(1.0f/60.0f);
	PVehicle player = PVehicle(pm, VehicleType::eTOYOTA);
	PVehicle enemy = PVehicle(pm, VehicleType::eJEEP, PxVec3(5.0f, 0.0f, 0.0f));
	PDyanmic obstacle_d = PDyanmic(pm, PxSphereGeometry(1), PxVec3(-20.0f, 20.0f, -10.0f));
	PStatic obstacle_s = PStatic(pm, PxBoxGeometry(1.0f, 1.0f, 1.0f), PxVec3(-20.0f, 0.0f, -20.0f));

	Camera playerCamera = Camera(Utils::instance().SCREEN_WIDTH, Utils::instance().SCREEN_HEIGHT);
	playerCamera.setPitch(-30.0f);

	bool cameraToggle = false;

	while (!window.shouldClose()) {

		Time::update();

		pm.simulate();
		player.update();
		enemy.update();
		glfwPollEvents();

		Utils::instance().shader->use();

		if (inputManager->onMouseButtonAction(GLFW_MOUSE_BUTTON_RIGHT, GLFW_REPEAT))
			editorCamera.handleRotation(inputManager->getMousePosition().x, inputManager->getMousePosition().y);
		else if (inputManager->onMouseButtonAction(GLFW_MOUSE_BUTTON_RIGHT, GLFW_RELEASE))
			editorCamera.resetLastPos();

		if (inputManager->onKeyAction(GLFW_KEY_W, GLFW_PRESS)) editorCamera.handleTranslation(GLFW_KEY_W);
		if (inputManager->onKeyAction(GLFW_KEY_A, GLFW_PRESS)) editorCamera.handleTranslation(GLFW_KEY_A);
		if (inputManager->onKeyAction(GLFW_KEY_S, GLFW_PRESS)) editorCamera.handleTranslation(GLFW_KEY_S);
		if (inputManager->onKeyAction(GLFW_KEY_D, GLFW_PRESS)) editorCamera.handleTranslation(GLFW_KEY_D);
		if (inputManager->onKeyAction(GLFW_KEY_LEFT_SHIFT, GLFW_PRESS)) editorCamera.handleTranslation(GLFW_KEY_LEFT_SHIFT);
		if (inputManager->onKeyAction(GLFW_KEY_LEFT_CONTROL, GLFW_PRESS)) editorCamera.handleTranslation(GLFW_KEY_LEFT_CONTROL);

		if (inputManager->onKeyAction(GLFW_KEY_UP, GLFW_PRESS)) player.accelerate(throttle);
		if (inputManager->onKeyAction(GLFW_KEY_DOWN, GLFW_PRESS)) player.reverse(throttle * 0.5f);
		if (inputManager->onKeyAction(GLFW_KEY_LEFT, GLFW_PRESS)) player.turnLeft(throttle * 0.5f);
		if (inputManager->onKeyAction(GLFW_KEY_RIGHT, GLFW_PRESS)) player.turnRight(throttle * 0.5f);
		if (inputManager->onKeyAction(GLFW_KEY_SPACE, GLFW_PRESS)) player.handbrake();

		if (inputManager->onKeyAction(GLFW_KEY_E, GLFW_PRESS) && Time::interval(2.0f)) 
			player.getRigidDynamic()->addForce(PxVec3(0.0, 15000.0, 0.0), PxForceMode::eIMPULSE);

		if (inputManager->onKeyAction(GLFW_KEY_C, GLFW_PRESS) && Time::interval(1.0f))
			cameraToggle = !cameraToggle;

		if (abs(player.getPosition().z) >= 101.0f || abs(player.getPosition().x) >= 101.0) player.removePhysics();

		glEnable(GL_DEPTH_TEST);
		glEnable(GL_LINE_SMOOTH);
		glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


		if (cameraToggle) 
			editorCamera.render();
		else {
			playerCamera.setPosition(glm::vec3(player.getPosition().x, player.getPosition().y + 6.0f, player.getPosition().z + 12.0f));
			playerCamera.render();
		}

		ground.draw();
		player.render();
		enemy.render();
		obstacle_d.render(ball);
		obstacle_s.render(obstacleMesh);

		glDisable(GL_FRAMEBUFFER_SRGB);
		window.swapBuffers();

	}

	player.free();
	enemy.free();
	obstacle_d.free();
	obstacle_s.free();
	pm.free();

	glfwTerminate();
	return 0;
}
