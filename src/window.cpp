#include "window.hpp"

#include <chrono>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <iostream>
#include <thread>

GLFWwindow* init() {
	// GLFW Init
	// Print GLFW errors
	glfwSetErrorCallback([](int, const char* description) { fprintf(stderr, "Error: %s\n", description); });

	if (!glfwInit())
		exit(EXIT_FAILURE);

	// I need OpenGL 4.6 Core
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	// This supposedly enables sRGB, but sRGB is enabled without it?
	glfwWindowHint(GLFW_SRGB_CAPABLE, true);

	// MSAA x8 (If I implement post-processing, I'll do AA there, so this could be removed in the future)
	glfwWindowHint(GLFW_SAMPLES, 64);

	// Debug Contexts are helpful but slow; Only enable in debug builds
#ifndef NDEBUG
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, true);
#endif

	GLFWwindow* window = glfwCreateWindow(1280, 720, "CircuiTron", NULL, NULL);
	if (!window) {
		glfwTerminate();
		exit(EXIT_FAILURE);
	}
	glfwMakeContextCurrent(window);
	glfwSwapInterval(1);

	return window;
}

Window::Window() : Window(init()) {}
Window::Window(GLFWwindow* window) : render(glfwGetProcAddress), window(window) {
	if (glfwRawMouseMotionSupported())
		glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);

	// Resize render on window resize
	glfwSetWindowUserPointer(window, this);
	glfwSetFramebufferSizeCallback(window, [](GLFWwindow* window, int width, int height) {
		static_cast<Window*>(glfwGetWindowUserPointer(window))->render.resize(width, height);
	});
	{
		// Render needs to be resized before it can be used
		int width, height;
		glfwGetFramebufferSize(window, &width, &height);
		render.resize(width, height);
	}

	glfwSetScrollCallback(window, [](GLFWwindow* window, double xoffset, double yoffset) {
		static_cast<Window*>(glfwGetWindowUserPointer(window))->scroll.add(xoffset, yoffset);
	});

	// Init imgui
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui::StyleColorsDark();
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init(NULL);
}

void Window::beginFrame() {
	scroll.reset();
	glfwPollEvents(); // This has to happen before ImGui::NewFrame()

	// All imgui commands must happen after here
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	double xpos, ypos;
	glfwGetCursorPos(window, &xpos, &ypos);
	cursor.apply(xpos, ypos);
}

void Window::endFrame() {
	render.run();

	// Draw the framerate counter
	// This is replaced by Profiler if I get it working better
	ImGui::SetNextWindowPos(ImVec2(0, 0));
	ImGui::Begin(
		"Profiler", nullptr,
		ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_AlwaysAutoResize |
			ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoSavedSettings);
	ImGui::Text("%.1f fps", ImGui::GetIO().Framerate);
	ImGui::End();

	// All imgui commands must happen before here
	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

	// Swap the render buffers and wait for the next frame
	// Should probably be the last thing in the game loop
	glfwSwapBuffers(window);
}