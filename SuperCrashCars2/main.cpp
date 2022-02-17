﻿#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <stb_image.h>

#include "Time.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include "Geometry.h"
#include "GLDebug.h"
#include "Log.h"
#include "Window.h"

#include "glm/glm.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include "glm/gtc/type_ptr.hpp"

#include "InputManager.h"
#include "InputController.h"

#include "Camera.h"

#include "PVehicle.h"
#include "PDynamic.h"
#include "PStatic.h"

#include "Skybox.h"

int main(int argc, char** argv) {
	Log::info("Starting Game...");

	// OpenGL
	glfwInit();
	Window window(Utils::instance().SCREEN_WIDTH, Utils::instance().SCREEN_HEIGHT, "Super Crash Cars 2");
	
	Utils::instance().shader = std::make_shared<ShaderProgram>("shaders/shader_vertex.vert", "shaders/shader_fragment.frag");

	std::shared_ptr<InputManager> inputManager = std::make_shared<InputManager>(Utils::instance().SCREEN_WIDTH, Utils::instance().SCREEN_HEIGHT);
	window.setCallbacks(inputManager);

	// Camera
	bool cameraToggle = false;
	Camera playerCamera = Camera(Utils::instance().SCREEN_WIDTH, Utils::instance().SCREEN_HEIGHT);
	playerCamera.setPitch(-30.0f);

	// Physx
	PhysicsManager pm = PhysicsManager(1.5f/60.0f);
	PVehicle player = PVehicle(pm, VehicleType::eJEEP, PxVec3(0.0f, 10.0f, 0.0f));
	PVehicle enemy = PVehicle(pm, VehicleType::eTOYOTA, PxVec3(5.0f, 10.0f, 0.0f));
	player.vehicleParams.jumpCoefficient = player.getRigidDynamic()->getMass() * 7;
	player.vehicleParams.boostCoefficient = player.getRigidDynamic()->getMass() / 3;

	// ImGui
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	ImGui::StyleColorsDark();
	ImGui_ImplGlfw_InitForOpenGL(window.getWindow(), true);
	ImGui_ImplOpenGL3_Init("#version 330");

	// Lighting
	glm::vec4 lightColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);

	// Controller
	InputController controller;
	if (glfwJoystickPresent(GLFW_JOYSTICK_1)) controller = InputController(GLFW_JOYSTICK_1);

	// skybox
	Skybox skybox;

	// Anti-Aliasing not sure if this works rn becuase doesn't work for frame buffer, but we are missing some parts of frame buffer if we use it can't tell
	unsigned int samples = 8;
	glfwWindowHint(GLFW_SAMPLES, samples);
	glEnable(GL_DEPTH_TEST);

	while (!window.shouldClose()) {

		glfwPollEvents();
		Time::update();

		if (Time::shouldSimulate) {
			Time::startSimTimer();
			#pragma region inputs

			if (glfwJoystickPresent(GLFW_JOYSTICK_1)) {
				controller.PS4Input(player);
				//controller.XboxInput(player);
			}

			if (inputManager->onKeyAction(GLFW_KEY_UP, GLFW_PRESS)) player.accelerate(player.vehicleParams.k_throttle);
			if (inputManager->onKeyAction(GLFW_KEY_DOWN, GLFW_PRESS)) player.reverse(player.vehicleParams.k_throttle * 0.5f);
			if (inputManager->onKeyAction(GLFW_KEY_LEFT, GLFW_PRESS)) player.turnLeft(player.vehicleParams.k_throttle * 0.5f);
			if (inputManager->onKeyAction(GLFW_KEY_RIGHT, GLFW_PRESS)) player.turnRight(player.vehicleParams.k_throttle * 0.5f);
			if (inputManager->onKeyAction(GLFW_KEY_SPACE, GLFW_PRESS)) {
				player.handbrake();
				Time::resetStats();
			}
			if (inputManager->onKeyAction(GLFW_KEY_E, GLFW_PRESS)) player.jump();
			if (inputManager->onKeyAction(GLFW_KEY_F, GLFW_PRESS)) player.boost();
			if (inputManager->onKeyAction(GLFW_KEY_R, GLFW_PRESS)) player.getRigidDynamic()->setGlobalPose(PxTransform(PxVec3(0.0f, 10.0f, 0.0f), PxQuat(PxPi, PxVec3(0.0f, 1.0f, 0.0f))));

#pragma endregion
			pm.simulate();
			player.update();
			enemy.update();
			Time::simulatePhysics();
		}

		if (Time::shouldRender) {
			Time::startRenderTimer();

			glEnable(GL_CULL_FACE);
			glCullFace(GL_FRONT);
			glFrontFace(GL_CW);		 
			glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			Utils::instance().shader->use();
			Utils::instance().shader->setVector4("lightColor", lightColor);
			Utils::instance().shader->setVector3("lightPos", Utils::instance().pxToGlmVec3(player.getPosition()));
			Utils::instance().shader->setVector3("camPos", playerCamera.getPosition());
			
			playerCamera.updateCamera(Utils::instance().pxToGlmVec3(player.getPosition()), player.getFrontVec());

			pm.drawGround();
			enemy.render();
			player.render();

			skybox.draw(playerCamera.getPerspMat(), glm::mat4(glm::mat3(playerCamera.getViewMat())));

#pragma region imgui
			// imGUI section
			ImGui_ImplOpenGL3_NewFrame();
			ImGui_ImplGlfw_NewFrame();
			ImGui::NewFrame();
			ImGui::Begin("Stats:");
			std::string simTime = ("Average Simulation time: " + std::to_string(Time::averageSimTime) + " microseconds");
			std::string renderTime = ("Average Render Time: " + std::to_string(Time::averageRenderTime) + " microseconds");
			std::string printBoost = ("Boost: " + std::to_string(player.vehicleParams.boost));
			std::string printPos = "Current Position: X: " + std::to_string(player.getPosition().x) + " Y: " + std::to_string(player.getPosition().y) + " Z: " + std::to_string(player.getPosition().z);
			std::string printLinearVelocity = "Current Linear Velocity: X: " + std::to_string(player.getRigidDynamic()->getLinearVelocity().x) + " Y: " + std::to_string(player.getRigidDynamic()->getLinearVelocity().y) + " Z: " + std::to_string(player.getRigidDynamic()->getLinearVelocity().z);
			std::string printAngularVelocity = "Current Angular Velocity: X: " + std::to_string(player.getRigidDynamic()->getAngularVelocity().x) + " Y: " + std::to_string(player.getRigidDynamic()->getAngularVelocity().y) + " Z: " + std::to_string(player.getRigidDynamic()->getAngularVelocity().z);

			ImGui::Text(simTime.c_str());
			ImGui::Text(renderTime.c_str());
			ImGui::Text(printBoost.c_str());
			ImGui::Text(printPos.c_str());
			ImGui::Text(printLinearVelocity.c_str());
			ImGui::Text(printAngularVelocity.c_str());

			ImGui::End();

			ImGui::Render();
			ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
#pragma endregion

			glDisable(GL_FRAMEBUFFER_SRGB);
			window.swapBuffers();

			Time::renderFrame(); // turn off the render flag and stop timer
		}
	}

	player.free();
	enemy.free();
	pm.free();

	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();


	glfwTerminate();
	return 0;
}