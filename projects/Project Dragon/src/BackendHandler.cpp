#include "BackendHandler.h"
#include "RenderingManager.h"
#include <InputHelpers.h>
#include <IBehaviour.h>
#include <Camera.h>
#include <bullet/LinearMath/btVector3.h>
#include <GreyscaleEffect.h>
#include <PhysicsBody.h>
#define GLM_ENABLE_EXPERIMENTAL
#include<glm/gtx/rotate_vector.hpp>
#include <BtToGlm.h>
#include <ColorCorrection.h>
#include <WorldBuilderV2.h>
#include <AudioEngine.h>
#include <Bloom.h>
GLFWwindow* BackendHandler::window = nullptr;
std::vector<std::function<void()>> BackendHandler::imGuiCallbacks;

void mouse_callback(GLFWwindow* window, double xpos, double ypos);

void BackendHandler::GlDebugMessage(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam)
{
	{
		std::string sourceTxt;
		switch (source) {
		case GL_DEBUG_SOURCE_API: sourceTxt = "DEBUG"; break;
		case GL_DEBUG_SOURCE_WINDOW_SYSTEM: sourceTxt = "WINDOW"; break;
		case GL_DEBUG_SOURCE_SHADER_COMPILER: sourceTxt = "SHADER"; break;
		case GL_DEBUG_SOURCE_THIRD_PARTY: sourceTxt = "THIRD PARTY"; break;
		case GL_DEBUG_SOURCE_APPLICATION: sourceTxt = "APP"; break;
		case GL_DEBUG_SOURCE_OTHER: default: sourceTxt = "OTHER"; break;
		}
		switch (severity) {
		case GL_DEBUG_SEVERITY_LOW:          LOG_INFO("[{}] {}", sourceTxt, message); break;
		case GL_DEBUG_SEVERITY_MEDIUM:       LOG_WARN("[{}] {}", sourceTxt, message); break;
		case GL_DEBUG_SEVERITY_HIGH:         LOG_ERROR("[{}] {}", sourceTxt, message); break;
#ifdef LOG_GL_NOTIFICATIONS
		case GL_DEBUG_SEVERITY_NOTIFICATION: LOG_INFO("[{}] {}", sourceTxt, message); break;
#endif
		default: break;
		}
	}
}

void BackendHandler::UpdateAudio()
{
	AudioEngine& engine = AudioEngine::Instance();
	engine.Update();
}

bool BackendHandler::InitAll()
{
	Logger::Init();
	Util::Init();

	if (!InitGLFW())
		return 1;
	if (!InitGLAD())
		return 1;
	Framebuffer::InitFullscreenQuad();
	RenderingManager::Init();

	//Init Audio
	AudioEngine& engine = AudioEngine::Instance();
	engine.Init();
	//start the music
	
	engine.LoadBank("sound/music");
	engine.LoadBank("sound/Sound Effects");
	engine.LoadBank("sound/Music.strings");
	engine.LoadBus("MusicBus", "{137cb40e-93aa-4837-8626-c2445824f974}");
	AudioEvent& music = engine.CreateNewEvent("Ambient Music 1", "{eac6c8f2-dc46-4acb-96a2-d4271dbe8072}");
	engine.CreateNewEvent("Element Swap", "{aa3a7bc0-fe97-48a1-8ce7-4680087fe66d}");
	engine.CreateNewEvent("Enemy Jump", "{8ef856c1-a3f5-4313-8266-74b56a655319}");
	engine.CreateNewEvent("Level Complete", "{7148fbe2-c4ee-4e3a-a254-5bb351cbcbf8}");
	
	music.Play();

	InitImGui();
}

void BackendHandler::GlfwWindowResizedCallback(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
	RenderingManager::activeScene->Registry().view<Camera>().each([=](Camera& cam)
		{
			cam.ResizeWindow(width, height);
		});
	RenderingManager::activeScene->Registry().view<Framebuffer>().each([=](Framebuffer& buf)
		{
			buf.Reshape(width, height);
		});
	RenderingManager::activeScene->Registry().view<PostEffect>().each([=](PostEffect& buf)
		{
			buf.Reshape(width, height);
		});
	RenderingManager::activeScene->Registry().view<GreyscaleEffect>().each([=](GreyscaleEffect& buf)
		{
			buf.Reshape(width, height);
		});
	RenderingManager::activeScene->Registry().view<ColorCorrectionEffect>().each([=](ColorCorrectionEffect& buf)
		{
			buf.Reshape(width, height);
		});
	RenderingManager::activeScene->Registry().view<BloomEffect>().each([=](BloomEffect& buf)
		{
			buf.Reshape(width, height);
		});
}



#include <Player.h>
#include <Enemy.h>
bool AudioInit = 0;
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
	//Placeholder shoot sfx
	AudioEngine& engine = AudioEngine::Instance();
	
	AudioEvent& tempShoot = engine.GetEvent("Element Swap");

	Player& p = RenderingManager::activeScene->FindFirst("Camera").get<Player>();
	
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
	{
		p.LeftHandShoot();
	}
	if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS)
	{
		p.RightHandShoot();
	}
	
}

void BackendHandler::UpdateInput()
{
	//creates a single camera object to call

	GameObject cameraObj = RenderingManager::activeScene->FindFirst("Camera");
	//loads the LUTS to switch them

	Camera cam = cameraObj.get<Camera>();
	Transform t = cameraObj.get<Transform>();
	PhysicsBody phys = cameraObj.get<PhysicsBody>();
	//get a forward vector using fancy maths
	glm::vec3 forward(0, 1, 0);
	forward = glm::rotate(forward, glm::radians(t.GetLocalRotation().z), glm::vec3(0, 0, 1));
	glm::normalize(forward);

	btVector3 movement = btVector3(0, 0, 0);
	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
	{
		movement += BtToGlm::GLMTOBTV3(forward);
	}
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
	{
		movement -= BtToGlm::GLMTOBTV3(forward);
	}

	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
	{
		glm::vec3 direction = glm::normalize(glm::cross(forward, cam.GetUp()));
		movement.setX(movement.getX() - direction.x * 1.8);
		movement.setY(movement.getY() - direction.y * 1.8);
	}

	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
	{
		glm::vec3 direction = -glm::normalize(glm::cross(forward, cam.GetUp()));
		movement.setX(movement.getX() - direction.x * 1.8);
		movement.setY(movement.getY() - direction.y * 1.8);
	}
	Player& p = RenderingManager::activeScene->FindFirst("Camera").get<Player>();
	if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
	{
	//	
		//p.CheckJump();

		//if (p.GetPlayerData().m_CanJump) //To infinite jump remove this if statement
		//{
			//Placeholder shoot sfx
			AudioEngine& engine = AudioEngine::Instance();
			AudioEvent& tempJump = engine.GetEvent("Enemy Jump");
			tempJump.Play();
			movement.setZ(1.0f);
		//}
	}

	//temporary
	if (glfwGetKey(window, GLFW_KEY_G) == GLFW_PRESS)
	{
		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
	}
	if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS)
	{
		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	}


	if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
	{
		p.SwitchLeftHand();
	}

	if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
	{
		p.SwitchRightHand();
	}
	phys.ApplyForce(movement);

	RenderingManager::activeScene->Registry().view<BehaviourBinding>().each([&](entt::entity entity, BehaviourBinding& binding) {
		// Iterate over all the behaviour scripts attached to the entity, and update them in sequence (if enabled)
		for (const auto& behaviour : binding.Behaviours) {
			if (behaviour->Enabled) {
				behaviour->Update(entt::handle(RenderingManager::activeScene->Registry(), entity));
			}
		}
		}
	);
}

bool BackendHandler::InitGLFW()
{
	if (glfwInit() == GLFW_FALSE) {
		LOG_ERROR("Failed to initialize GLFW");
		return false;
	}

#ifdef _DEBUG
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, true);
#endif

	//Create a new GLFW window
	window = glfwCreateWindow(1280, 720, "Project Dragon", nullptr, nullptr);
	glfwMakeContextCurrent(window);

	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

	// Set our window resized callback
	glfwSetWindowSizeCallback(window, GlfwWindowResizedCallback);

	glfwSetMouseButtonCallback(window, mouse_button_callback);

	// Store the window in the application singleton
	Application::Instance().Window = window;

	return true;
}

bool BackendHandler::InitGLAD()
{
	if (gladLoadGLLoader((GLADloadproc)glfwGetProcAddress) == 0) {
		LOG_ERROR("Failed to initialize Glad");
		return false;
	}
	return true;
}

void BackendHandler::InitImGui()
{
	// Creates a new ImGUI context
	ImGui::CreateContext();
	// Gets our ImGUI input/output
	ImGuiIO& io = ImGui::GetIO();
	// Enable keyboard navigation
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	// Allow docking to our window
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	// Allow multiple viewports (so we can drag ImGui off our window)
	io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
	// Allow our viewports to use transparent backbuffers
	io.ConfigFlags |= ImGuiConfigFlags_TransparentBackbuffers;

	// Set up the ImGui implementation for OpenGL
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 410");

	// Dark mode FTW
	ImGui::StyleColorsDark();

	// Get our imgui style
	ImGuiStyle& style = ImGui::GetStyle();
	//style.Alpha = 1.0f;
	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
		style.WindowRounding = 0.0f;
		style.Colors[ImGuiCol_WindowBg].w = 0.8f;
	}
}

void BackendHandler::ShutdownImGui()
{
	// Cleanup the ImGui implementation
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	// Destroy our ImGui context
	ImGui::DestroyContext();
}

void BackendHandler::RenderImGui()
{
	// Implementation new frame
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	// ImGui context new frame
	ImGui::NewFrame();

	if (ImGui::Begin("Debug")) {
		// Render our GUI stuff
		for (auto& func : imGuiCallbacks) {
			func();
		}
		ImGui::End();
	}

	// Make sure ImGui knows how big our window is
	ImGuiIO& io = ImGui::GetIO();
	int width{ 0 }, height{ 0 };
	glfwGetWindowSize(window, &width, &height);
	io.DisplaySize = ImVec2((float)width, (float)height);

	// Render all of our ImGui elements
	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

	// If we have multiple viewports enabled (can drag into a new window)
	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
		// Update the windows that ImGui is using
		ImGui::UpdatePlatformWindows();
		ImGui::RenderPlatformWindowsDefault();
		// Restore our gl context
		glfwMakeContextCurrent(window);
	}
}

void BackendHandler::RenderVAO(const Shader::sptr& shader, const VertexArrayObject::sptr& vao, const glm::mat4& viewProjection, const Transform& transform)
{
	shader->SetUniformMatrix("u_ModelViewProjection", viewProjection * transform.WorldTransform());
	shader->SetUniformMatrix("u_Model", transform.WorldTransform());
	shader->SetUniformMatrix("u_NormalMatrix", transform.WorldNormalMatrix());
	vao->Render();
}

void BackendHandler::SetupShaderForFrame(const Shader::sptr& shader, const glm::mat4& view, const glm::mat4& projection)
{
	shader->Bind();
	// These are the uniforms that update only once per frame
	shader->SetUniformMatrix("u_View", view);
	shader->SetUniformMatrix("u_ViewProjection", projection * view);
	shader->SetUniformMatrix("u_SkyboxMatrix", projection * glm::mat4(glm::mat3(view)));
	glm::vec3 camPos = glm::inverse(view) * glm::vec4(0, 0, 0, 1);
	shader->SetUniform("u_CamPos", camPos);
}

//for camera forward
bool firstMouse = true;
float yaw = -90.0f;	// yaw is initialized to -90.0 degrees since a yaw of 0.0 results in a direction vector pointing to the right so we initially rotate a bit to the left.
float pitch = 0.0f;
float lastX = 1280 / 2.0;
float lastY = 720.0 / 2.0;