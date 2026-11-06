/*
* Laboratorio de Computación Gráfica e Interacción Humano-Computadora
* 14 - Modelo de Fresnel
*/

#include <iostream>
#include <stdlib.h>
#include <unordered_map>

// GLAD: Multi-Language GL/GLES/EGL/GLX/WGL Loader-Generator
// https://glad.dav1d.de/
#include <glad/glad.h>

// GLFW: https://www.glfw.org/
#include <GLFW/glfw3.h>

// GLM: OpenGL Math library
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// Model loading classes
#include <shader_m.h>
#include <camera.h>
#include <model.h>
#include <cubemap.h>

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

Shader *dynamicShader;
Shader *earthShader;

// Modelos
Model	*basemodel;
Model	*scene;

// Por Keyframes
AnimatedModel *alien;

// Cubemap
CubeMap *mainCubeMap;

// character movement
bool movement = false;




glm::mat4 thirdPOV() {
	glm::vec3 target = charaPosition + glm::vec3(0.0f, 1.5f, 0.0f);
	glm::vec3 cameraPosition = charaPosition - forwardView * 6.5f + glm::vec3(0.0f, 2.0f, 0.0f);

	return glm::lookAt(cameraPosition, target, glm::vec3(0.0f, 2.2f, 0.0f));
}

bool keyPressed(GLFWwindow* window, int key) {
	static std::unordered_map<int, bool> previousState;

	bool currentState = glfwGetKey(window, key) == GLFW_PRESS;
	bool pressed = currentState && !previousState[key];

	previousState[key] = currentState;

	return pressed;
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
	earthShader = new Shader("shaders/10_texturesPhong.vs", "shaders/10_texturesPhong.fs");


	// Máximo número de huesos: 100 por defecto
	dynamicShader->setBonesIDs(MAX_RIGGING_BONES);



	// Dibujar en malla de alambre
	// glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);

	basemodel = new Model("models/IllumModels/moon.fbx");
	scene = new Model("models/earth.fbx");

	// ANIMADOS
	
	alien = new AnimatedModel("models/alien.fbx");




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
		view = glm::lookAt(fixeds[currentFixed].position, fixeds[currentFixed].target,
			glm::vec3(0.0f, 1.0f, 0.0f)
		);
	}


	// Cubemap (fondo)
	{
		mainCubeMap->drawCubeMap(*cubemapShader, projection, view);
	}
	
	// Dibujamos un objeto cualquiera
	{
		// Activamos el shader de Phong
		fresnelShader->use();

		// Activamos para objetos transparentes
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		// Aplicamos transformaciones de proyección y cámara (si las hubiera)
		fresnelShader->setMat4("projection", projection);
		fresnelShader->setMat4("view", view);

		// Aplicamos transformaciones del modelo
		glm::mat4 model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(-2.5f, 0.0f, 0.0f));
		model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::scale(model, glm::vec3(1.0f, 1.0f, 1.0f));
		fresnelShader->setMat4("model", model);
		fresnelShader->setVec3("cameraPosition", camera.Position);
		fresnelShader->setFloat("mRefractionRatio", 1.0f / 1.003f); // 1.003 Aire
		fresnelShader->setFloat("_Bias", 0.1f); // Inclinación de la refracción/reflexión (distorsión)
                                                // Inclinación = 0 (material transparente)
                                                // Inclinación = 1 (material metálico)
		fresnelShader->setFloat("_Scale", 0.1f);
		fresnelShader->setFloat("_Power", 1.0f);

		basemodel->Draw(*fresnelShader);


		model = glm::translate(model, glm::vec3(2.5f, 0.0f, 0.0f));

		fresnelShader->setMat4("model", model);
		fresnelShader->setVec3("cameraPosition", camera.Position);
		fresnelShader->setFloat("mRefractionRatio", 1.0f / 1.333f); // 1.333 Agua
		fresnelShader->setFloat("_Bias", 0.1f); 
		fresnelShader->setFloat("_Scale", 0.1f);
		fresnelShader->setFloat("_Power", 1.0f);

		basemodel->Draw(*fresnelShader);

		model = glm::translate(model, glm::vec3(2.5f, 0.0f, 0.0f));

		fresnelShader->setMat4("model", model);
		fresnelShader->setVec3("cameraPosition", camera.Position);
		fresnelShader->setFloat("mRefractionRatio", 1.0f / 1.7f); // 2.417 Diamond
		fresnelShader->setFloat("_Bias", 0.1f); 
		fresnelShader->setFloat("_Scale", 0.1f);
		fresnelShader->setFloat("_Power", 1.0f);

		basemodel->Draw(*fresnelShader);

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

		// Desactivamos el shader actual
		glUseProgram(0);

	}









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