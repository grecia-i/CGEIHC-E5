/*
* Laboratorio de Computación Gráfica e Interacción Humano-Computadora
* 14 - Modelo de Fresnel
*/
// AAA
#include <iostream>
#include <stdlib.h>
#include <unordered_map>
#include <sstream>

// GLAD: Multi-Language GL/GLES/EGL/GLX/WGL Loader-Generator
// https://glad.dav1d.de/
#include <glad/glad.h>

// GLFW: https://www.glfw.org/
#include <GLFW/glfw3.h>

// GLM: OpenGL Math library
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
// aaa
// Model loading classes
#include <shader_m.h>
#include <camera.h>
#include <model.h>
#include <cubemap.h>
#include <light.h>
#include <material.h>
#include <animatedmodel.h>

// Functions
bool Start();
bool Update();

// Definición de callbacks
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow *window);

// Gobals
GLFWwindow* window;

// Tamaño en pixeles de la ventana
const unsigned int SCR_WIDTH = 1024;
const unsigned int SCR_HEIGHT = 768;

// Definición de cámara
Camera camera(glm::vec3(0.0f, 2.0f, 10.0f)); 


// Cams
enum CameraMode {
	thirdMode,
	fixedMode
};

CameraMode currentCam = thirdMode;
int currentFixed = 0;

struct FixedCam {
	glm::vec3 position;
	glm::vec3 target;
};

FixedCam fixeds[] = {
	// cam 1
	{ glm::vec3(0.0f, 5.0f, 8.0f),
	glm::vec3(0.0f, 1.0f, 0.0f) },

	// cam 2
	{ glm::vec3(8.0f, 4.0f, 2.0f),
	glm::vec3(0.0f, 1.0f, 0.0f) },

	// cam 3
	{ glm::vec3(-7.0f, 4.0f, -3.0f),
	glm::vec3(0.0f, 1.0f, 0.0f) },

	// cam 4
	{ glm::vec3(0.0f, 6.0f, -8.0f),
	glm::vec3(0.0f, 1.0f, 0.0f) }
};

const int NUM_FIXED_CAMERAS = sizeof(fixeds) / sizeof(fixeds[0]);


// Controladores para el movimiento del mouse
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// Variables para la velocidad de reproducción
// de la animación
float deltaTime = 0.0f;
float lastFrame = 0.0f;
float elapsedTime = 0.0f;

glm::vec3 charaPosition(0.0f, 0.0f, 0.0f);
glm::vec3 forwardView(0.0f, 0.0f, 1.0f);
float     scaleV = 0.005f;
float     rotateCharacter = 0.0f;
float     rotateTable = 0.0f;


// Shaders
Shader *cubemapShader;
Shader *fresnelShader;
Shader *proceduralShader;
Shader *dynamicShader;

Shader *phongShader;

// Modelos
Model	*base;

// Por Keyframes
AnimatedModel *alien;

// Procedural
Model* sun;
Model* moon; // aa

// Cubemap
CubeMap *mainCubeMap;

// Materiales
Material material;


// Luces
std::vector<Light> gLights;

Light sunlight;
glm::vec3 sunPos(20.0f, 20.0f, 0.0f);

Light moonlight;
glm::vec3 moonPos(-20.0f, -20.0f, 0.0f);

float tradius = 10.0f;
float theta = 0.0f;
float alpha = 0.0f;
float proceduralTime = 0.0f;


// character movement
bool movement = false;




glm::mat4 thirdPOV() {
	//glm::vec3 target = charaPosition + glm::vec3(0.0f, 1.5f, 0.0f);
	//glm::vec3 cameraPosition = charaPosition - forwardView * 6.5f + glm::vec3(0.0f, 2.0f, 0.0f);

	//return glm::lookAt(cameraPosition, target, glm::vec3(0.0f, 2.2f, 0.0f));
    float radius = 6.5f;
    float yaw = glm::radians(camera.Yaw);
    float pitch = glm::radians(camera.Pitch);

    glm::vec3 offset;
    offset.x = radius * cos(pitch) * sin(yaw);
    offset.y = radius * sin(pitch);
    offset.z = radius * cos(pitch) * cos(yaw);

    glm::vec3 target = charaPosition + glm::vec3(0.0f, 1.5f, 0.0f);
    glm::vec3 cameraPosition = target + offset;

    return glm::lookAt(cameraPosition, target, glm::vec3(0.0f, 1.0f, 0.0f));
}

bool keyPressed(GLFWwindow* window, int key) {
	static std::unordered_map<int, bool> previousState;

	bool currentState = glfwGetKey(window, key) == GLFW_PRESS;
	bool pressed = currentState && !previousState[key];

	previousState[key] = currentState;

	return pressed;
}

void SetLightUniformInt(Shader* shader, const char* propertyName, size_t lightIndex, int value) {
	std::ostringstream ss;
	ss << "allLights[" << lightIndex << "]." << propertyName;
	std::string uniformName = ss.str();

	shader->setInt(uniformName.c_str(), value);
}
void SetLightUniformFloat(Shader* shader, const char* propertyName, size_t lightIndex, float value) {
	std::ostringstream ss;
	ss << "allLights[" << lightIndex << "]." << propertyName;
	std::string uniformName = ss.str();

	shader->setFloat(uniformName.c_str(), value);
}
void SetLightUniformVec4(Shader* shader, const char* propertyName, size_t lightIndex, glm::vec4 value) {
	std::ostringstream ss;
	ss << "allLights[" << lightIndex << "]." << propertyName;
	std::string uniformName = ss.str();

	shader->setVec4(uniformName.c_str(), value);
}
void SetLightUniformVec3(Shader* shader, const char* propertyName, size_t lightIndex, glm::vec3 value) {
	std::ostringstream ss;
	ss << "allLights[" << lightIndex << "]." << propertyName;
	std::string uniformName = ss.str();

	shader->setVec3(uniformName.c_str(), value);
}



// Entrada a función principal
int main()
{
	if (!Start())
		return -1;

	/* Loop until the user closes the window */
	while (!glfwWindowShouldClose(window))
	{
		if (!Update())
			break;
	}

	glfwTerminate();
	return 0;

}

bool Start() {
	// Inicialización de GLFW

	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	// Creación de la ventana con GLFW
	window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Illumination Models", NULL, NULL);
	if (window == NULL)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return false;
	}
	glfwMakeContextCurrent(window);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSetCursorPosCallback(window, mouse_callback);
	glfwSetScrollCallback(window, scroll_callback);

	// Ocultar el cursor mientras se rota la escena
	// glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	// glad: Cargar todos los apuntadores
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD" << std::endl;
		return false;
	}

	// Activación de buffer de profundidad
	glEnable(GL_DEPTH_TEST);

	// Compilación y enlace de shaders
	cubemapShader = new Shader("shaders/10_vertex_cubemap.vs", "shaders/10_fragment_cubemap.fs");
	fresnelShader = new Shader("shaders/11_fresnel.vs", "shaders/11_fresnel.fs");

	dynamicShader = new Shader("shaders/09_vertex_skinning.vs", "shaders/09_fragment_skinning.fs");
	phongShader = new Shader("shaders/11_PhongShaderMultLights.vs", "shaders/11_PhongShaderMultLights.fs");
	proceduralShader = new Shader("shaders/12_ProceduralAnimation.vs", "shaders/12_ProceduralAnimation.fs");

	// Máximo número de huesos: 100 por defecto
	dynamicShader->setBonesIDs(MAX_RIGGING_BONES);

	// Dibujar en malla de alambre
	// glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);

	base = new Model("models/flooring.fbx");


	// ANIMADOS
	alien = new AnimatedModel("models/alien.fbx");
	sun = new Model("models/sun.fbx");
	moon = new Model("models/moon.fbx");

	// Cubemap
	vector<std::string> faces
	{
		"textures/cubemap/01/posx.png",
		"textures/cubemap/01/negx.png",
		"textures/cubemap/01/posy.png",
		"textures/cubemap/01/negy.png",
		"textures/cubemap/01/posz.png",
		"textures/cubemap/01/negz.png"
	};
	mainCubeMap = new CubeMap();
	mainCubeMap->loadCubemap(faces);


	Light light01;
	light01.Position = glm::vec3(0.0f, 0.5f, 0.0f);
	light01.Color = glm::vec4(0.2f, 0.0f, 0.0f, 1.0f);
	light01.Power = glm::vec4(60.0f, 60.0f, 60.0f, 1.0f);
	gLights.push_back(light01);

	Light light02;
	light02.Position = glm::vec3(1.0f, 0.5f, 0.0f);
	light02.Color = glm::vec4(0.0f, 0.2f, 0.0f, 1.0f);
	light02.Power = glm::vec4(60.0f, 60.0f, 60.0f, 1.0f);
	gLights.push_back(light02);
	/*
	Light light03;
	light03.Position = glm::vec3(5.0f, 2.0f, -5.0f);
	light03.Color = glm::vec4(0.0f, 0.0f, 0.2f, 1.0f);
	light03.Power = glm::vec4(60.0f, 60.0f, 60.0f, 1.0f);
	gLights.push_back(light03);

	Light light04;
	light04.Position = glm::vec3(-5.0f, 2.0f, -5.0f);
	light04.Color = glm::vec4(0.2f, 0.2f, 0.0f, 1.0f);
	light04.Power = glm::vec4(60.0f, 60.0f, 60.0f, 1.0f);
	gLights.push_back(light04);*/

	sunlight.Position = sunPos;
	sunlight.Direction = glm::vec3(0.0f, -1.0f, 0.0f);
	sunlight.Color = glm::vec4(0.9568f, 0.8274f, 0.3686f, 1.0f); // #f4d35e
	sunlight.Power = glm::vec4(200.0f, 200.0f, 200.0f, 1.0f);
	sunlight.alphaIndex = 80;
	sunlight.distance = 35.0f;
	gLights.push_back(sunlight);

	moonlight.Position = moonPos;
	moonlight.Direction = glm::vec3(0.0f, -1.0f, 0.0f);
	moonlight.Color = glm::vec4(0.8784f, 0.8823f, 0.8666f, 1.0f); // #e0e1dd
	moonlight.Power = glm::vec4(150.0f, 150.0f, 150.0f, 1.0f);
	moonlight.alphaIndex = 80;
	moonlight.distance = 35.0f;
	gLights.push_back(moonlight);

	material.ambient = glm::vec4(0.2f, 0.2f, 0.2f, 1.0f);
	material.diffuse = glm::vec4(0.5f, 0.5f, 0.5f, 1.0f);
	material.specular = glm::vec4(0.5f, 0.5f, 0.5f, 1.0f);
	material.transparency = 1.0f;

	return true;
}

bool Update() {
	// Cálculo del framerate
	float currentFrame = (float)glfwGetTime();
	deltaTime = currentFrame - lastFrame;
	lastFrame = currentFrame;

	// Procesa la entrada del teclado o mouse
	processInput(window);

	// Renderizado R - G - B - A
	glClearColor(1.0f, 1.0f, 1.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 10000.0f);
	// glm::mat4 view = camera.GetViewMatrix();
	glm::mat4 view;
	if (currentCam == thirdMode) {
		view = thirdPOV();
	}
	else {
		view = glm::lookAt(fixeds[currentFixed].position, fixeds[currentFixed].target, glm::vec3(0.0f, 1.0f, 0.0f));
	}


	// Cubemap (fondo)
	{
		mainCubeMap->drawCubeMap(*cubemapShader, projection, view);
	}

	//  Suelo
	{
		// Activamos el shader del plano
		phongShader->use();

		// Activamos para objetos transparentes
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		phongShader->setMat4("projection", projection);
		phongShader->setMat4("view", view);

		// Aplicamos transformaciones del modelo
		glm::mat4 model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(0.0f, 0.5f, 0.0f)); // translate it down so it's at the center of the scene
		model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::scale(model, glm::vec3(1.0f, 1.0f, 1.0f));	// it's a bit too big for our scene, so scale it down
		phongShader->setMat4("model", model);

		// Configuramos propiedades de fuentes de luz
		phongShader->setInt("numLights", (int)gLights.size());
		for (size_t i = 0; i < gLights.size(); ++i) {
			SetLightUniformVec3(phongShader, "Position", i, gLights[i].Position);
			SetLightUniformVec3(phongShader, "Direction", i, gLights[i].Direction);
			SetLightUniformVec4(phongShader, "Color", i, gLights[i].Color);
			SetLightUniformVec4(phongShader, "Power", i, gLights[i].Power);
			SetLightUniformInt(phongShader, "alphaIndex", i, gLights[i].alphaIndex);
			SetLightUniformFloat(phongShader, "distance", i, gLights[i].distance);
		}

		phongShader->setVec3("eye", camera.Position);

		phongShader->setVec4("MaterialAmbientColor", material.ambient);
		phongShader->setVec4("MaterialDiffuseColor", material.diffuse);
		phongShader->setVec4("MaterialSpecularColor", material.specular);
		phongShader->setFloat("transparency", material.transparency);

		base->Draw(*phongShader);
	}

	glUseProgram(0);


	{
		// Dibujar personaje
		{
			// Actualización de la animación
			if (movement) {
				alien->UpdateAnimation(deltaTime);

			}
			// Activación del shader del personaje
			dynamicShader->use();

			// Aplicamos transformaciones de proyección y cámara (si las hubiera)
			dynamicShader->setMat4("projection", projection);
			dynamicShader->setMat4("view", view);

			// Aplicamos transformaciones del modelo
			glm::mat4 model = glm::mat4(1.0f);
			model = glm::translate(model, charaPosition); // translate it down so it's at the center of the scene
			model = glm::rotate(model, glm::radians(rotateCharacter), glm::vec3(0.0f, 1.0f, 0.0f));
			model = glm::scale(model, glm::vec3(0.01f, 0.01f, 0.01f));	// it's a bit too big for our scene, so scale it down

			dynamicShader->setMat4("model", model);

			dynamicShader->setMat4("gBones", MAX_RIGGING_BONES, alien->gBones);

			// Dibujamos el modelo
			alien->Draw(*dynamicShader);
		}

	}

	glUseProgram(0);

	// SOOOOOOOOOOOOOL
	{
		// Activamos el shader de Phong
		proceduralShader->use();

		// Activamos para objetos transparentes
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		// Aplicamos transformaciones de proyección y cámara (si las hubiera)
		proceduralShader->setMat4("projection", projection);
		proceduralShader->setMat4("view", view);

		// Aplicamos transformaciones del modelo
		glm::mat4 model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f));
		model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::scale(model, glm::vec3(2.0f, 2.0f, 2.0f));
		proceduralShader->setMat4("model", model);

		proceduralShader->setFloat("time", proceduralTime);
		proceduralShader->setFloat("radius", 40.0f);
		proceduralShader->setFloat("height", 0.5f);

		sun->Draw(*proceduralShader);
		proceduralTime += 0.001;

	}

	glUseProgram(0);

	// LUNAAAAAAAAAAAAAAAAAAAAA
	{
		// Activamos el shader de Phong
		proceduralShader->use();

		// Activamos para objetos transparentes
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		// Aplicamos transformaciones de proyección y cámara (si las hubiera)
		proceduralShader->setMat4("projection", projection);
		proceduralShader->setMat4("view", view);

		// Aplicamos transformaciones del modelo
		glm::mat4 model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f));
		model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::scale(model, glm::vec3(2.0f, 2.0f, 2.0f));
		proceduralShader->setMat4("model", model);

		proceduralShader->setFloat("time", proceduralTime + glm::pi<float>());
		proceduralShader->setFloat("radius", 30.0f);
		proceduralShader->setFloat("height", 0.0f);

		moon->Draw(*proceduralShader);
		proceduralTime += 0.001;

	}

	glUseProgram(0);

	// glfw: swap buffers 
	glfwSwapBuffers(window);
	glfwPollEvents();

	return true;
}

// Procesamos entradas del teclado
void processInput(GLFWwindow* window)
{
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);

	/*
	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		camera.ProcessKeyboard(FORWARD, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		camera.ProcessKeyboard(BACKWARD, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		camera.ProcessKeyboard(LEFT, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		camera.ProcessKeyboard(RIGHT, deltaTime);
		*/
	if (glfwGetKey(window, GLFW_KEY_M) == GLFW_PRESS)
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	if (glfwGetKey(window, GLFW_KEY_N) == GLFW_PRESS)
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	if (glfwGetKey(window, GLFW_KEY_B) == GLFW_PRESS)
		glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);

	if (keyPressed(window, GLFW_KEY_T)) {
		currentCam = thirdMode;
	}

	if (keyPressed(window, GLFW_KEY_1)) { //CAM1
		currentCam = fixedMode;
		currentFixed = 0;
	}

	if (keyPressed(window, GLFW_KEY_2)) { //CAM2
		currentCam = fixedMode;
		currentFixed = 1;
	}
	if (keyPressed(window, GLFW_KEY_3)) { //CAM3
		currentCam = fixedMode;
		currentFixed = 2;
	}
	if (keyPressed(window, GLFW_KEY_4)) { //CAM3
		currentCam = fixedMode;
		currentFixed = 3;
	}

	// character movement
	float movementSpeed = 2.0f * deltaTime;
	float rotationSpeed = 30.0f * deltaTime;

	if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
		//charaPosition = charaPosition + scaleV * forwardView;
		charaPosition += forwardView * movementSpeed;
		movement = true;
	}
	if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
		// charaPosition = charaPosition - scaleV * forwardView;
		charaPosition -= forwardView * movementSpeed;
		movement = true;
	}
	if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {
		rotateCharacter += rotationSpeed;
		movement = true;
		/*rotateCharacter += 0.5f;

		glm::mat4 model = glm::mat4(1.0f);
		model = glm::rotate(model, glm::radians(rotateCharacter), glm::vec3(0.0f, 1.0f, 0.0f));
		glm::vec4 viewVector = model * glm::vec4(0.0f, 0.0f, 1.0f, 1.0f);
		forwardView = glm::vec3(viewVector);
		forwardView = glm::normalize(forwardView);*/
	}
	if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
		rotateCharacter -= rotationSpeed;
		movement = true;
		/*rotateCharacter -= 0.5f;

		glm::mat4 model = glm::mat4(1.0f);
		model = glm::rotate(model, glm::radians(rotateCharacter), glm::vec3(0.0f, 1.0f, 0.0f));
		glm::vec4 viewVector = model * glm::vec4(0.0f, 0.0f, 1.0f, 1.0f);
		forwardView = glm::vec3(viewVector);
		forwardView = glm::normalize(forwardView);*/
	}

	glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), glm::radians(rotateCharacter), glm::vec3(0.0f, 1.0f, 0.0f));

	forwardView = glm::normalize(glm::vec3(rotation * glm::vec4(0, 0, 1.5, 0)));
	
}

// glfw: Actualizamos el puerto de vista si hay cambios del tamaño
// de la ventana
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
}

// glfw: Callback del movimiento y eventos del mouse
void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
	if (firstMouse)
	{
		lastX = (float)xpos;
		lastY = (float)ypos;
		firstMouse = false;
	}

	float xoffset = (float)xpos - lastX;
	float yoffset = lastY - (float)ypos; 

	lastX = (float)xpos;
	lastY = (float)ypos;

	camera.ProcessMouseMovement(xoffset, yoffset);
}

// glfw: Complemento para el movimiento y eventos del mouse
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	camera.ProcessMouseScroll((float)yoffset);
}