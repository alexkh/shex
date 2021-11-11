#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <iostream>
#include <fstream>
#include <stdexcept>
#include <algorithm>
#include <chrono>
#include <vector>
#include <cstring>
#include <cstdlib>
#include <cstdint>
#include <array>
#include <optional>
#include <set>

#include <unistd.h>

#include "fixed6x13koi.h"

const int WIDTH = 364;
const int HEIGHT = 1065;

const int MAX_FRAMES_IN_FLIGHT = 2;

const std::vector<const char*> validationLayers = {
	"VK_LAYER_KHRONOS_validation"
};

const std::vector<const char*> deviceExtensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

VkResult CreateDebugUtilsMessengerEXT(VkInstance instance,
		const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
		const VkAllocationCallbacks* pAllocator,
		VkDebugUtilsMessengerEXT* pDebugMessenger) {
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT)
	vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
	if(func != nullptr) {
		return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
	} else {
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}

void DestroyDebugUtilsMessengerEXT(VkInstance instance,
		VkDebugUtilsMessengerEXT debugMessenger,
		const VkAllocationCallbacks* pAllocator) {
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)
	vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
	if(func != nullptr) {
		func(instance, debugMessenger, pAllocator);
	}
}

struct QueueFamilyIndices {
	std::optional<uint32_t> graphicsFamily;
	std::optional<uint32_t> presentFamily;

	bool isComplete() {
		return graphicsFamily.has_value() && presentFamily.has_value();
	}
};

struct SwapChainSupportDetails {
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentModes;
};

struct Vertex {
	glm::vec2 pos;
	glm::vec3 color;
	glm::vec2 texCoord;

	static VkVertexInputBindingDescription getBindingDescription() {
		VkVertexInputBindingDescription bindingDescription = {};
		bindingDescription.binding = 0;
		bindingDescription.stride = sizeof(Vertex);
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		return bindingDescription;
	}

	static std::array<VkVertexInputAttributeDescription, 3>
			getAttributeDescriptions() {
		std::array<VkVertexInputAttributeDescription, 3>
		attributeDescriptions = {};

		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
		attributeDescriptions[0].offset = offsetof(Vertex, pos);

		attributeDescriptions[1].binding = 0;
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[1].offset = offsetof(Vertex, color);

		attributeDescriptions[2].binding = 0;
		attributeDescriptions[2].location = 2;
		attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
		attributeDescriptions[2].offset = offsetof(Vertex, texCoord);

		return attributeDescriptions;
	}
};

struct Chunkmeta {
	uint32_t file_size;
	uint32_t chunk_file_offset;
	uint32_t view_chunk_offset;
	uint16_t file_size_upper; // upper bits of file size to make 48 bits
	uint16_t chunk_file_offset_upper; // upper bits of chunk file offset
};

const size_t chunk_size = 2048;

struct UniformBufferObject {
	alignas(16) glm::mat4 model;
	alignas(16) glm::mat4 view;
	alignas(16) glm::mat4 proj;
	alignas(16) uint8_t fontdata[sizeof(fixed6x13)];
	alignas(16) Chunkmeta chunkmeta;
	alignas(16) uint8_t datachunk[chunk_size];
	alignas(16) float param[4]; // scalex, scaley
};

const std::vector<Vertex> vertices = {
	{{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
	{{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
	{{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
	{{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}}
};

const std::vector<uint16_t> indices = {
	0, 1, 2, 2, 3, 0
};

class Application {
public:
	void run() {
		initWindow();
		initVulkan();
		mainLoop();
		cleanup();
	}

	void set_fname(char *fname) {
		input_fname = fname;
	}

private:
	GLFWwindow* window;

	VkInstance instance;
	VkDebugUtilsMessengerEXT debugMessenger;
	VkSurfaceKHR surface;

	VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
	VkDevice device;

	VkQueue graphicsQueue;
	VkQueue presentQueue;

	VkSwapchainKHR swapChain;
	std::vector<VkImage> swapChainImages;
	VkFormat swapChainImageFormat;
	VkExtent2D swapChainExtent;
	std::vector<VkImageView> swapChainImageViews;
	std::vector<VkFramebuffer> swapChainFramebuffers;

	VkRenderPass renderPass;
	VkDescriptorSetLayout descriptorSetLayout;
	VkPipelineLayout pipelineLayout;
	VkPipeline graphicsPipeline;

	VkCommandPool commandPool;

	VkImage textureImage;
	VkDeviceMemory textureImageMemory;
	VkImageView textureImageView;
	VkSampler textureSampler;

	VkBuffer vertexBuffer;
	VkDeviceMemory vertexBufferMemory;
	VkBuffer indexBuffer;
	VkDeviceMemory indexBufferMemory;

	std::vector<VkBuffer> uniformBuffers;
	std::vector<VkDeviceMemory> uniformBuffersMemory;

	VkDescriptorPool descriptorPool;
	std::vector<VkDescriptorSet> descriptorSets;

	std::vector<VkCommandBuffer> commandBuffers;

	std::vector<VkSemaphore> imageAvailableSemaphores;
	std::vector<VkSemaphore> renderFinishedSemaphores;
	std::vector<VkFence> inFlightFences;
//	vkAcquireNextImageKHR never returns an image index, that are already presented or queued to be presented.
//	So there is no need to wait for an image index, just for a concurrent frame index
//	std::vector<VkFence> imagesInFlight;

	size_t currentFrame = 0;

	bool framebufferResized = false;
	bool scrollbar_dragged = false; // mouse pressed the scroll bar

	double scrollbar_initial_y = 0; // mouse y when scrollbar was pressed
	int64_t scrollbar_initial_ifile_view_offset = 0; // offset at start

	char *input_fname = nullptr; // file, which contents will be displayed
	std::vector<uint8_t> ifbuffer; // buffer for the input file
	size_t ifbuffer_size = chunk_size; // how much to load into ifbuffer
	size_t ifbuffer_file_offset = 0; // file offset of ifbuffer
	size_t chunk_file_offset = 0; // file offset of chunk sent to shader
	int64_t ifile_view_offset = 0; // file offset of first line displayed
	size_t ifile_size = 0;
	int64_t current_scroll_step = 0; // scrolling step when a key is pressed
	int64_t next_scroll_ms = 0; // if a key is pressed, when to scroll again

	int64_t autorepeat_delay = 180;
	int64_t autorepeat_rate = 60;

	// loads a chunk of input file: returns size actually loaded
	size_t load_chunk(const char *fname, size_t offset,
			size_t len, uint8_t *dest) {
		size_t bytes_read = 0;
		std::ifstream file(fname, std::ios::ate | std::ios::binary);

		if(!file.is_open()) {
			throw std::runtime_error("failed to open input file!");
		}

		ifile_size = (size_t) file.tellg();
		file.seekg(offset);
		file.read((char *)dest, len);
		bytes_read = file.gcount();

		file.close();

//		std::cout << "Loaded file at offset " << offset << " len "
//				<< len << " file size: " << ifile_size << "\n";

		return bytes_read;
	}

	void initWindow() {
		glfwInit();

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

		window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr,
				nullptr);
		glfwSetWindowUserPointer(window, this);
		glfwSetFramebufferSizeCallback(window,
				framebufferResizeCallback);
		glfwSetScrollCallback(window, scroll_callback);
		glfwSetKeyCallback(window, key_callback);
		glfwSetMouseButtonCallback(window, btn_callback);
	}

	static void framebufferResizeCallback(GLFWwindow* window, int width,
			int height) {
		auto app = reinterpret_cast<Application*>(
				glfwGetWindowUserPointer(window));
		app->framebufferResized = true;
	}

	void initVulkan() {
		createInstance();
		setupDebugMessenger();
		createSurface();
		pickPhysicalDevice();
		createLogicalDevice();
		createSwapChain();
		createImageViews();
		createRenderPass();
		createDescriptorSetLayout();
		createGraphicsPipeline();
		createFramebuffers();
		createCommandPool();
		createTextureImage();
		createTextureImageView();
		createTextureSampler();
		createVertexBuffer();
		createIndexBuffer();
		createUniformBuffers();
		createDescriptorPool();
		createDescriptorSets();
		createCommandBuffers();
		createSyncObjects();
	}

	static void scroll_one_step(GLFWwindow *window) {
		Application *app = reinterpret_cast<Application *> (
				glfwGetWindowUserPointer(window));
		if(app->current_scroll_step < 0) {
			if(app->ifile_view_offset >
					-app->current_scroll_step) {
				app->ifile_view_offset +=
					app->current_scroll_step;
			} else {
				app->ifile_view_offset = 0;
			}
		} else {
			app->ifile_view_offset += app->current_scroll_step;
		}
	}

	static void start_scrolling(GLFWwindow* window, int64_t step) {
		Application *app = static_cast<Application *> (
				glfwGetWindowUserPointer(window));
		app->current_scroll_step = step;
		scroll_one_step(window);
//		Using std::chrono::steady_clock instead of std::chrono::high_resolution_clock for better performance and to ensure the clock to be steady
		app->next_scroll_ms = std::chrono::duration_cast
			<std::chrono::milliseconds>(
			std::chrono::time_point_cast
			<std::chrono::milliseconds>(
			std::chrono::steady_clock::now())
			.time_since_epoch()).count() + app->autorepeat_delay;
	}

	static void stop_scrolling(GLFWwindow *window) {
		Application *app = reinterpret_cast<Application *> (
				glfwGetWindowUserPointer(window));
		app->current_scroll_step = 0;
		app->next_scroll_ms = 0;
	}

	static void key_callback(GLFWwindow *window, int key, int scancode,
			int action, int mods) {
		int neg = mods & GLFW_MOD_SHIFT? -1: 1;
		Application *app = reinterpret_cast<Application *> (
				glfwGetWindowUserPointer(window));
		if(action == GLFW_PRESS) {
			switch(key) {
			case GLFW_KEY_PAGE_DOWN:
				start_scrolling(window, 1024);
				break;
			case GLFW_KEY_PAGE_UP:
				start_scrolling(window, -1024);
				break;
			case GLFW_KEY_F3:
				start_scrolling(window, neg * 8192);
				break;
			case GLFW_KEY_F4:
				start_scrolling(window, neg * 0x10000);
				break;
			case GLFW_KEY_F5:
				start_scrolling(window, neg * 0x100000);
				break;
			case GLFW_KEY_F6:
				start_scrolling(window, neg * 0x1000000);
				break;
			case GLFW_KEY_F7:
				start_scrolling(window, neg * 0x10000000);
				break;
			case GLFW_KEY_F8:
				start_scrolling(window, neg * 0x100000000);
				break;
			case GLFW_KEY_F9:
				start_scrolling(window, neg * 0x1000000000);
				break;
			case GLFW_KEY_HOME:
				app->ifile_view_offset = 0;
				break;
			case GLFW_KEY_END:
				app->ifile_view_offset = app->ifile_size > 1184?
						app->ifile_size - 1184: 0;
				break;
			case GLFW_KEY_UP:
				start_scrolling(window, -16);
				break;
			case GLFW_KEY_DOWN:
				start_scrolling(window, 16);
				break;
			}
			if(app->ifile_view_offset < 0) {
				app->ifile_view_offset = 0;
			}
		} else if(action == GLFW_RELEASE) {
			switch(key) {
			case GLFW_KEY_PAGE_DOWN:
			case GLFW_KEY_PAGE_UP:
			case GLFW_KEY_F3:
			case GLFW_KEY_F4:
			case GLFW_KEY_F5:
			case GLFW_KEY_F6:
			case GLFW_KEY_F7:
			case GLFW_KEY_F8:
			case GLFW_KEY_F9:
			case GLFW_KEY_UP:
			case GLFW_KEY_DOWN:
				stop_scrolling(window);
			}
		}
	}

	static void scroll_callback(GLFWwindow *window, double xoffset,
			double yoffset) {
		Application *app = reinterpret_cast<Application *>(
				glfwGetWindowUserPointer(window));
		app->ifile_view_offset += int(yoffset) * 16
				* MOUSEWHEELYDIR;
		if(app->ifile_view_offset < 0) {
			app->ifile_view_offset = 0;
		}
	}

	static void btn_callback(GLFWwindow *window, int button, int action,
			int mods) {
		Application *app = reinterpret_cast<Application *>(
				glfwGetWindowUserPointer(window));
		if(button == GLFW_MOUSE_BUTTON_LEFT) {
			if(action == GLFW_PRESS) {
				// check if scrollbar is being pressed
				double xpos, ypos;
				glfwGetCursorPos(window, &xpos, &ypos);
				// calculate scrollbar size:
				// window contains 1200 bytes
				// and scrollbar available height is 1048 pixels
				double scrollbar_size = (1200.0 /
					double(app->ifile_size)) * 1048.0;
				// minimum scrollbar size is 40 pixels
				if(scrollbar_size < 40) {
					scrollbar_size = 40;
				}
				// calculate current scrollbar vert. position:
				double scrollbar_pos = ((double(
					app->ifile_view_offset) /
					double(app->ifile_size)) *
					(1048.0 - scrollbar_size)) + 15.0;
				std::cout << scrollbar_pos << "\n";
				if(xpos > 2 && xpos < 11 &&
						ypos > scrollbar_pos
						&& ypos < (scrollbar_pos +
						scrollbar_size)) {
					app->scrollbar_dragged = true;
					app->scrollbar_initial_y = ypos;
					app->
					scrollbar_initial_ifile_view_offset =
					app->ifile_view_offset;
				}
			} else if(action == GLFW_RELEASE) {
				app->scrollbar_dragged = false;
			}
		}
	}

	void mainLoop() {
		while(!glfwWindowShouldClose(window)) {
			glfwPollEvents();
			drawFrame();
//			Use VSYNC instead of usleep
//			usleep(100000);
		}

		vkDeviceWaitIdle(device);
	}

	void cleanupSwapChain() {
		for(auto framebuffer : swapChainFramebuffers) {
			vkDestroyFramebuffer(device, framebuffer, nullptr);
		}

		vkFreeCommandBuffers(device, commandPool,
				static_cast<uint32_t>(commandBuffers.size()),
				commandBuffers.data());

		vkDestroyPipeline(device, graphicsPipeline, nullptr);
		vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
		vkDestroyRenderPass(device, renderPass, nullptr);

		for(auto imageView : swapChainImageViews) {
			vkDestroyImageView(device, imageView, nullptr);
		}

		vkDestroySwapchainKHR(device, swapChain, nullptr);

		for(size_t i = 0; i < swapChainImages.size(); i++) {
			vkDestroyBuffer(device, uniformBuffers[i], nullptr);
			vkFreeMemory(device, uniformBuffersMemory[i], nullptr);
		}

		vkDestroyDescriptorPool(device, descriptorPool, nullptr);
	}

	void cleanup() {
		cleanupSwapChain();

		vkDestroySampler(device, textureSampler, nullptr);
		vkDestroyImageView(device, textureImageView, nullptr);

		vkDestroyImage(device, textureImage, nullptr);
		vkFreeMemory(device, textureImageMemory, nullptr);

		vkDestroyDescriptorSetLayout(device, descriptorSetLayout,
				nullptr);

		vkDestroyBuffer(device, indexBuffer, nullptr);
		vkFreeMemory(device, indexBufferMemory, nullptr);

		vkDestroyBuffer(device, vertexBuffer, nullptr);
		vkFreeMemory(device, vertexBufferMemory, nullptr);

		for(size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			vkDestroySemaphore(device,
					renderFinishedSemaphores[i], nullptr);
			vkDestroySemaphore(device,
					imageAvailableSemaphores[i], nullptr);
			vkDestroyFence(device, inFlightFences[i], nullptr);
		}

		vkDestroyCommandPool(device, commandPool, nullptr);

		vkDestroyDevice(device, nullptr);

		if(enableValidationLayers) {
			DestroyDebugUtilsMessengerEXT(instance, debugMessenger,
					nullptr);
		}

		vkDestroySurfaceKHR(instance, surface, nullptr);
		vkDestroyInstance(instance, nullptr);

		glfwDestroyWindow(window);

		glfwTerminate();
	}

	void recreateSwapChain() {
		int width = 0, height = 0;
		glfwGetFramebufferSize(window, &width, &height);
		while(width == 0 || height == 0) {
			glfwGetFramebufferSize(window, &width, &height);
			glfwWaitEvents();
		}

		vkDeviceWaitIdle(device);

		cleanupSwapChain();

		createSwapChain();
		createImageViews();
		createRenderPass();
		createGraphicsPipeline();
		createFramebuffers();
		createUniformBuffers();
		createDescriptorPool();
		createDescriptorSets();
		createCommandBuffers();
	}

	void createInstance() {
		if(enableValidationLayers && !checkValidationLayerSupport()) {
			throw std::runtime_error(
			"validation layers requested, but not available!");
		}

		VkApplicationInfo appInfo = {};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = "Basic text";
		appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.pEngineName = "No Engine";
		appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.apiVersion = VK_API_VERSION_1_0;

		VkInstanceCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo = &appInfo;

		auto extensions = getRequiredExtensions();
		createInfo.enabledExtensionCount =
				static_cast<uint32_t>(extensions.size());
		createInfo.ppEnabledExtensionNames = extensions.data();

		VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo;
		if(enableValidationLayers) {
			createInfo.enabledLayerCount =
				static_cast<uint32_t>(validationLayers.size());
			createInfo.ppEnabledLayerNames =
					validationLayers.data();

			populateDebugMessengerCreateInfo(debugCreateInfo);
			createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)
					&debugCreateInfo;
		} else {
			createInfo.enabledLayerCount = 0;

			createInfo.pNext = nullptr;
		}

		if(vkCreateInstance(&createInfo, nullptr, &instance) !=
				VK_SUCCESS) {
			throw std::runtime_error("failed to create instance!");
		}
	}

	void populateDebugMessengerCreateInfo(
			VkDebugUtilsMessengerCreateInfoEXT& createInfo) {
		createInfo = {};
		createInfo.sType =
			VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		createInfo.messageSeverity =
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		createInfo.messageType =
			VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		createInfo.pfnUserCallback = debugCallback;
	}

	void setupDebugMessenger() {
		if(!enableValidationLayers) return;

		VkDebugUtilsMessengerCreateInfoEXT createInfo;
		populateDebugMessengerCreateInfo(createInfo);

		if(CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr,
				&debugMessenger) != VK_SUCCESS) {
			throw std::runtime_error(
					"failed to set up debug messenger!");
		}
	}

	void createSurface() {
		if(glfwCreateWindowSurface(instance, window, nullptr, &surface)
				!= VK_SUCCESS) {
			throw std::runtime_error(
					"failed to create window surface!");
		}
	}

	void pickPhysicalDevice() {
		uint32_t deviceCount = 0;
		vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

		if(deviceCount == 0) {
			throw std::runtime_error(
				"failed to find GPUs with Vulkan support!");
		}

		std::vector<VkPhysicalDevice> devices(deviceCount);
		vkEnumeratePhysicalDevices(instance, &deviceCount,
				devices.data());

		for(const auto& device : devices) {
			if(isDeviceSuitable(device)) {
				physicalDevice = device;
				break;
			}
		}

		if(physicalDevice == VK_NULL_HANDLE) {
			throw std::runtime_error(
				"failed to find a suitable GPU!");
		}
	}

	void createLogicalDevice() {
		QueueFamilyIndices indices = findQueueFamilies(physicalDevice);

		std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
		std::set<uint32_t> uniqueQueueFamilies = {
		indices.graphicsFamily.value(), indices.presentFamily.value()};

		float queuePriority = 1.0f;
		for(uint32_t queueFamily : uniqueQueueFamilies) {
			VkDeviceQueueCreateInfo queueCreateInfo = {};
			queueCreateInfo.sType =
				VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueCreateInfo.queueFamilyIndex = queueFamily;
			queueCreateInfo.queueCount = 1;
			queueCreateInfo.pQueuePriorities = &queuePriority;
			queueCreateInfos.push_back(queueCreateInfo);
		}

		VkPhysicalDeviceFeatures deviceFeatures = {};
		deviceFeatures.samplerAnisotropy = VK_TRUE;

		VkDeviceCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

		createInfo.queueCreateInfoCount =
				static_cast<uint32_t>(queueCreateInfos.size());
		createInfo.pQueueCreateInfos = queueCreateInfos.data();

		createInfo.pEnabledFeatures = &deviceFeatures;

		createInfo.enabledExtensionCount =
				static_cast<uint32_t>(deviceExtensions.size());
		createInfo.ppEnabledExtensionNames = deviceExtensions.data();

		if(enableValidationLayers) {
			createInfo.enabledLayerCount =
				static_cast<uint32_t>(validationLayers.size());
			createInfo.ppEnabledLayerNames =
					validationLayers.data();
		} else {
			createInfo.enabledLayerCount = 0;
		}

		if(vkCreateDevice(physicalDevice, &createInfo, nullptr, &device)
				!= VK_SUCCESS) {
			throw std::runtime_error(
					"failed to create logical device!");
		}

		vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0,
				&graphicsQueue);
		vkGetDeviceQueue(device, indices.presentFamily.value(), 0,
				&presentQueue);
	}

	void createSwapChain() {
		SwapChainSupportDetails swapChainSupport =
				querySwapChainSupport(physicalDevice);

		VkSurfaceFormatKHR surfaceFormat =
			chooseSwapSurfaceFormat(swapChainSupport.formats);
		VkPresentModeKHR presentMode =
			chooseSwapPresentMode(swapChainSupport.presentModes);
		VkExtent2D extent =
			chooseSwapExtent(swapChainSupport.capabilities);

		uint32_t imageCount =
				swapChainSupport.capabilities.minImageCount + 1;
		if(swapChainSupport.capabilities.maxImageCount > 0 &&
		imageCount > swapChainSupport.capabilities.maxImageCount) {
		imageCount = swapChainSupport.capabilities.maxImageCount;
		}

		VkSwapchainCreateInfoKHR createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		createInfo.surface = surface;

		createInfo.minImageCount = imageCount;
		createInfo.imageFormat = surfaceFormat.format;
		createInfo.imageColorSpace = surfaceFormat.colorSpace;
		createInfo.imageExtent = extent;
		createInfo.imageArrayLayers = 1;
		createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

		QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
		uint32_t queueFamilyIndices[] =
		{indices.graphicsFamily.value(), indices.presentFamily.value()};

		if(indices.graphicsFamily != indices.presentFamily) {
			createInfo.imageSharingMode =
					VK_SHARING_MODE_CONCURRENT;
			createInfo.queueFamilyIndexCount = 2;
			createInfo.pQueueFamilyIndices = queueFamilyIndices;
		} else {
			createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		}

		createInfo.preTransform =
				swapChainSupport.capabilities.currentTransform;
		createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		createInfo.presentMode = presentMode;
		createInfo.clipped = VK_TRUE;

		if(vkCreateSwapchainKHR(device, &createInfo, nullptr,
				&swapChain) != VK_SUCCESS) {
			throw std::runtime_error(
					"failed to create swap chain!");
		}

		vkGetSwapchainImagesKHR(device, swapChain, &imageCount,
				nullptr);
		swapChainImages.resize(imageCount);
		vkGetSwapchainImagesKHR(device, swapChain, &imageCount,
				swapChainImages.data());

		swapChainImageFormat = surfaceFormat.format;
		swapChainExtent = extent;
	}

	void createImageViews() {
		swapChainImageViews.resize(swapChainImages.size());

		for(size_t i = 0; i < swapChainImages.size(); i++) {
			swapChainImageViews[i] = createImageView(
				swapChainImages[i], swapChainImageFormat);
		}
	}

	void createRenderPass() {
		VkAttachmentDescription colorAttachment = {};
		colorAttachment.format = swapChainImageFormat;
		colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachment.stencilStoreOp =
				VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		VkAttachmentReference colorAttachmentRef = {};
		colorAttachmentRef.attachment = 0;
		colorAttachmentRef.layout =
				VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpass = {};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &colorAttachmentRef;

		VkSubpassDependency dependency = {};
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency.dstSubpass = 0;
		dependency.srcStageMask =
				VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.srcAccessMask = 0;
		dependency.dstStageMask =
				VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.dstAccessMask =
				VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

		VkRenderPassCreateInfo renderPassInfo = {};
		renderPassInfo.sType =
				VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.attachmentCount = 1;
		renderPassInfo.pAttachments = &colorAttachment;
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subpass;
		renderPassInfo.dependencyCount = 1;
		renderPassInfo.pDependencies = &dependency;

		if(vkCreateRenderPass(device, &renderPassInfo, nullptr,
				&renderPass) != VK_SUCCESS) {
			throw std::runtime_error(
					"failed to create render pass!");
		}
	}

	void createDescriptorSetLayout() {
		VkDescriptorSetLayoutBinding uboLayoutBinding = {};
		uboLayoutBinding.binding = 0;
		uboLayoutBinding.descriptorCount = 1;
		uboLayoutBinding.descriptorType =
				VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		uboLayoutBinding.pImmutableSamplers = nullptr;
		uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT |
				VK_SHADER_STAGE_FRAGMENT_BIT;

		VkDescriptorSetLayoutBinding samplerLayoutBinding = {};
		samplerLayoutBinding.binding = 1;
		samplerLayoutBinding.descriptorCount = 1;
		samplerLayoutBinding.descriptorType =
				VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		samplerLayoutBinding.pImmutableSamplers = nullptr;
		samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

		std::array<VkDescriptorSetLayoutBinding, 2> bindings =
				{uboLayoutBinding, samplerLayoutBinding};
		VkDescriptorSetLayoutCreateInfo layoutInfo = {};
		layoutInfo.sType =
			VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.bindingCount =
				static_cast<uint32_t>(bindings.size());
		layoutInfo.pBindings = bindings.data();

		if(vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr,
				&descriptorSetLayout) != VK_SUCCESS) {
			throw std::runtime_error(
				"failed to create descriptor set layout!");
		}
	}

	void createGraphicsPipeline() {
		std::vector<uint8_t> vertShaderCode = {
#include "vert.h"
		};
		std::vector<uint8_t> fragShaderCode = {
#include "frag.h"
		};

		VkShaderModule vertShaderModule =
				createShaderModule(vertShaderCode);
		VkShaderModule fragShaderModule =
				createShaderModule(fragShaderCode);

		VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
		vertShaderStageInfo.sType =
			VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
		vertShaderStageInfo.module = vertShaderModule;
		vertShaderStageInfo.pName = "main";

		VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
		fragShaderStageInfo.sType =
			VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		fragShaderStageInfo.module = fragShaderModule;
		fragShaderStageInfo.pName = "main";

		VkPipelineShaderStageCreateInfo shaderStages[] =
				{vertShaderStageInfo, fragShaderStageInfo};

		VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
		vertexInputInfo.sType =
		VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

		auto bindingDescription = Vertex::getBindingDescription();
		auto attributeDescriptions = Vertex::getAttributeDescriptions();

		vertexInputInfo.vertexBindingDescriptionCount = 1;
		vertexInputInfo.vertexAttributeDescriptionCount =
			static_cast<uint32_t>(attributeDescriptions.size());
		vertexInputInfo.pVertexBindingDescriptions =
				&bindingDescription;
		vertexInputInfo.pVertexAttributeDescriptions =
				attributeDescriptions.data();

		VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
		inputAssembly.sType =
		VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		inputAssembly.primitiveRestartEnable = VK_FALSE;

		VkViewport viewport = {};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = (float) swapChainExtent.width;
		viewport.height = (float) swapChainExtent.height;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		VkRect2D scissor = {};
		scissor.offset = {0, 0};
		scissor.extent = swapChainExtent;

		VkPipelineViewportStateCreateInfo viewportState = {};
		viewportState.sType =
			VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportState.viewportCount = 1;
		viewportState.pViewports = &viewport;
		viewportState.scissorCount = 1;
		viewportState.pScissors = &scissor;

		VkPipelineRasterizationStateCreateInfo rasterizer = {};
		rasterizer.sType =
		VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizer.depthClampEnable = VK_FALSE;
		rasterizer.rasterizerDiscardEnable = VK_FALSE;
		rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
		rasterizer.lineWidth = 1.0f;
		rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
		rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		rasterizer.depthBiasEnable = VK_FALSE;

		VkPipelineMultisampleStateCreateInfo multisampling = {};
		multisampling.sType =
		VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampling.sampleShadingEnable = VK_FALSE;
		multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

		VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
		colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT |
				VK_COLOR_COMPONENT_G_BIT |
				VK_COLOR_COMPONENT_B_BIT |
				VK_COLOR_COMPONENT_A_BIT;
		colorBlendAttachment.blendEnable = VK_FALSE;

		VkPipelineColorBlendStateCreateInfo colorBlending = {};
		colorBlending.sType =
		VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlending.logicOpEnable = VK_FALSE;
		colorBlending.logicOp = VK_LOGIC_OP_COPY;
		colorBlending.attachmentCount = 1;
		colorBlending.pAttachments = &colorBlendAttachment;
		colorBlending.blendConstants[0] = 0.0f;
		colorBlending.blendConstants[1] = 0.0f;
		colorBlending.blendConstants[2] = 0.0f;
		colorBlending.blendConstants[3] = 0.0f;

		VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
		pipelineLayoutInfo.sType =
				VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = 1;
		pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;

		if(vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr,
				&pipelineLayout) != VK_SUCCESS) {
			throw std::runtime_error(
					"failed to create pipeline layout!");
		}

		VkGraphicsPipelineCreateInfo pipelineInfo = {};
		pipelineInfo.sType =
				VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineInfo.stageCount = 2;
		pipelineInfo.pStages = shaderStages;
		pipelineInfo.pVertexInputState = &vertexInputInfo;
		pipelineInfo.pInputAssemblyState = &inputAssembly;
		pipelineInfo.pViewportState = &viewportState;
		pipelineInfo.pRasterizationState = &rasterizer;
		pipelineInfo.pMultisampleState = &multisampling;
		pipelineInfo.pColorBlendState = &colorBlending;
		pipelineInfo.layout = pipelineLayout;
		pipelineInfo.renderPass = renderPass;
		pipelineInfo.subpass = 0;
		pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

		if(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1,
		&pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS) {
			throw std::runtime_error(
					"failed to create graphics pipeline!");
		}

		vkDestroyShaderModule(device, fragShaderModule, nullptr);
		vkDestroyShaderModule(device, vertShaderModule, nullptr);
	}

	void createFramebuffers() {
		swapChainFramebuffers.resize(swapChainImageViews.size());

		for(size_t i = 0; i < swapChainImageViews.size(); i++) {
			VkImageView attachments[] = {
				swapChainImageViews[i]
			};

			VkFramebufferCreateInfo framebufferInfo = {};
			framebufferInfo.sType =
				VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferInfo.renderPass = renderPass;
			framebufferInfo.attachmentCount = 1;
			framebufferInfo.pAttachments = attachments;
			framebufferInfo.width = swapChainExtent.width;
			framebufferInfo.height = swapChainExtent.height;
			framebufferInfo.layers = 1;

			if(vkCreateFramebuffer(device,
					&framebufferInfo, nullptr,
				&swapChainFramebuffers[i]) != VK_SUCCESS) {
				throw std::runtime_error(
					"failed to create framebuffer!");
			}
		}
	}

	void createCommandPool() {
		QueueFamilyIndices queueFamilyIndices =
				findQueueFamilies(physicalDevice);

		VkCommandPoolCreateInfo poolInfo = {};
		poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		poolInfo.queueFamilyIndex =
				queueFamilyIndices.graphicsFamily.value();

		if(vkCreateCommandPool(device, &poolInfo, nullptr,
				&commandPool) != VK_SUCCESS) {
			throw std::runtime_error(
				"failed to create graphics command pool!");
		}
	}

	void createTextureImage() {
		int texWidth = 1, texHeight = 1, texChannels = 4;
		uint8_t pixels[4];
		reinterpret_cast<uint32_t *>(pixels)[0] = 0;
		VkDeviceSize imageSize = texWidth * texHeight * 4;

		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;
		createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
				VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				stagingBuffer, stagingBufferMemory);

		void* data;
		vkMapMemory(device, stagingBufferMemory, 0, imageSize, 0,
				&data);
		memcpy(data, pixels, static_cast<size_t>(imageSize));
		vkUnmapMemory(device, stagingBufferMemory);

		createImage(texWidth, texHeight, VK_FORMAT_R8G8B8A8_SRGB,
				VK_IMAGE_TILING_OPTIMAL,
				VK_IMAGE_USAGE_TRANSFER_DST_BIT |
				VK_IMAGE_USAGE_SAMPLED_BIT,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				textureImage, textureImageMemory);

		transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_SRGB,
				VK_IMAGE_LAYOUT_UNDEFINED,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		copyBufferToImage(stagingBuffer, textureImage,
				static_cast<uint32_t>(texWidth),
				static_cast<uint32_t>(texHeight));
		transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_SRGB,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		vkDestroyBuffer(device, stagingBuffer, nullptr);
		vkFreeMemory(device, stagingBufferMemory, nullptr);
	}

	void createTextureImageView() {
		textureImageView = createImageView(textureImage,
				VK_FORMAT_R8G8B8A8_SRGB);
	}

	void createTextureSampler() {
		VkSamplerCreateInfo samplerInfo = {};
		samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerInfo.magFilter = VK_FILTER_LINEAR;
		samplerInfo.minFilter = VK_FILTER_LINEAR;
		samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.anisotropyEnable = VK_TRUE;
		samplerInfo.maxAnisotropy = 16;
		samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
		samplerInfo.unnormalizedCoordinates = VK_FALSE;
		samplerInfo.compareEnable = VK_FALSE;
		samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
		samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

		if(vkCreateSampler(device, &samplerInfo, nullptr,
				&textureSampler) != VK_SUCCESS) {
			throw std::runtime_error(
					"failed to create texture sampler!");
		}
	}

	VkImageView createImageView(VkImage image, VkFormat format) {
		VkImageViewCreateInfo viewInfo = {};
		viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewInfo.image = image;
		viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewInfo.format = format;
		viewInfo.subresourceRange.aspectMask =
				VK_IMAGE_ASPECT_COLOR_BIT;
		viewInfo.subresourceRange.baseMipLevel = 0;
		viewInfo.subresourceRange.levelCount = 1;
		viewInfo.subresourceRange.baseArrayLayer = 0;
		viewInfo.subresourceRange.layerCount = 1;

		VkImageView imageView;
		if(vkCreateImageView(device, &viewInfo, nullptr, &imageView) !=
				VK_SUCCESS) {
			throw std::runtime_error(
					"failed to create texture image view!");
		}

		return imageView;
	}

	void createImage(uint32_t width, uint32_t height, VkFormat format,
			VkImageTiling tiling, VkImageUsageFlags usage,
			VkMemoryPropertyFlags properties, VkImage& image,
			VkDeviceMemory& imageMemory) {
		VkImageCreateInfo imageInfo = {};
		imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageInfo.imageType = VK_IMAGE_TYPE_2D;
		imageInfo.extent.width = width;
		imageInfo.extent.height = height;
		imageInfo.extent.depth = 1;
		imageInfo.mipLevels = 1;
		imageInfo.arrayLayers = 1;
		imageInfo.format = format;
		imageInfo.tiling = tiling;
		imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageInfo.usage = usage;
		imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		if(vkCreateImage(device, &imageInfo, nullptr, &image) !=
				VK_SUCCESS) {
			throw std::runtime_error("failed to create image!");
		}

		VkMemoryRequirements memRequirements;
		vkGetImageMemoryRequirements(device, image, &memRequirements);

		VkMemoryAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex = findMemoryType(
				memRequirements.memoryTypeBits, properties);

		if(vkAllocateMemory(device, &allocInfo, nullptr, &imageMemory)
				!= VK_SUCCESS) {
			throw std::runtime_error(
					"failed to allocate image memory!");
		}

		vkBindImageMemory(device, image, imageMemory, 0);
	}

	void transitionImageLayout(VkImage image, VkFormat format,
			VkImageLayout oldLayout, VkImageLayout newLayout) {
		VkCommandBuffer commandBuffer = beginSingleTimeCommands();

		VkImageMemoryBarrier barrier = {};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.oldLayout = oldLayout;
		barrier.newLayout = newLayout;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.image = image;
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		barrier.subresourceRange.baseMipLevel = 0;
		barrier.subresourceRange.levelCount = 1;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = 1;

		VkPipelineStageFlags sourceStage;
		VkPipelineStageFlags destinationStage;

		if(oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout ==
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

			sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		} else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
		newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			destinationStage =
					VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		} else {
			throw std::invalid_argument(
					"unsupported layout transition!");
		}

		vkCmdPipelineBarrier(
			commandBuffer,
			sourceStage, destinationStage,
			0,
			0, nullptr,
			0, nullptr,
			1, &barrier
		);

		endSingleTimeCommands(commandBuffer);
	}

	void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width,
			uint32_t height) {
		VkCommandBuffer commandBuffer = beginSingleTimeCommands();

		VkBufferImageCopy region = {};
		region.bufferOffset = 0;
		region.bufferRowLength = 0;
		region.bufferImageHeight = 0;
		region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		region.imageSubresource.mipLevel = 0;
		region.imageSubresource.baseArrayLayer = 0;
		region.imageSubresource.layerCount = 1;
		region.imageOffset = {0, 0, 0};
		region.imageExtent = {
			width,
			height,
			1
		};

		vkCmdCopyBufferToImage(commandBuffer, buffer, image,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

		endSingleTimeCommands(commandBuffer);
	}

	void createVertexBuffer() {
		VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;
		createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
				VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				stagingBuffer, stagingBufferMemory);

		void* data;
		vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0,
				&data);
		memcpy(data, vertices.data(), (size_t) bufferSize);
		vkUnmapMemory(device, stagingBufferMemory);

		createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT |
				VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				vertexBuffer, vertexBufferMemory);

		copyBuffer(stagingBuffer, vertexBuffer, bufferSize);

		vkDestroyBuffer(device, stagingBuffer, nullptr);
		vkFreeMemory(device, stagingBufferMemory, nullptr);
	}

	void createIndexBuffer() {
		VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();

		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;
		createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
				VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				stagingBuffer, stagingBufferMemory);

		void* data;
		vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0,
				&data);
		memcpy(data, indices.data(), (size_t) bufferSize);
		vkUnmapMemory(device, stagingBufferMemory);

		createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT |
				VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				indexBuffer, indexBufferMemory);

		copyBuffer(stagingBuffer, indexBuffer, bufferSize);

		vkDestroyBuffer(device, stagingBuffer, nullptr);
		vkFreeMemory(device, stagingBufferMemory, nullptr);
	}

	void createUniformBuffers() {
		VkDeviceSize bufferSize = sizeof(UniformBufferObject);

		uniformBuffers.resize(swapChainImages.size());
		uniformBuffersMemory.resize(swapChainImages.size());

		for(size_t i = 0; i < swapChainImages.size(); i++) {
			createBuffer(bufferSize,
					VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
					VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
					VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
					uniformBuffers[i],
					uniformBuffersMemory[i]);
		}
	}

	void createDescriptorPool() {
		std::array<VkDescriptorPoolSize, 2> poolSizes = {};
		poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		poolSizes[0].descriptorCount =
				static_cast<uint32_t>(swapChainImages.size());
		poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		poolSizes[1].descriptorCount =
				static_cast<uint32_t>(swapChainImages.size());

		VkDescriptorPoolCreateInfo poolInfo = {};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.poolSizeCount =
				static_cast<uint32_t>(poolSizes.size());
		poolInfo.pPoolSizes = poolSizes.data();
		poolInfo.maxSets =
				static_cast<uint32_t>(swapChainImages.size());

		if(vkCreateDescriptorPool(device, &poolInfo, nullptr,
				&descriptorPool) != VK_SUCCESS) {
			throw std::runtime_error(
					"failed to create descriptor pool!");
		}
	}

	void createDescriptorSets() {
		std::vector<VkDescriptorSetLayout>
			layouts(swapChainImages.size(), descriptorSetLayout);
		VkDescriptorSetAllocateInfo allocInfo = {};
		allocInfo.sType =
				VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = descriptorPool;
		allocInfo.descriptorSetCount =
				static_cast<uint32_t>(swapChainImages.size());
		allocInfo.pSetLayouts = layouts.data();

		descriptorSets.resize(swapChainImages.size());
		if(vkAllocateDescriptorSets(device, &allocInfo,
				descriptorSets.data()) != VK_SUCCESS) {
			throw std::runtime_error(
					"failed to allocate descriptor sets!");
		}

		for(size_t i = 0; i < swapChainImages.size(); i++) {
			VkDescriptorBufferInfo bufferInfo = {};
			bufferInfo.buffer = uniformBuffers[i];
			bufferInfo.offset = 0;
			bufferInfo.range = sizeof(UniformBufferObject);

			VkDescriptorImageInfo imageInfo = {};
			imageInfo.imageLayout =
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			imageInfo.imageView = textureImageView;
			imageInfo.sampler = textureSampler;

			std::array<VkWriteDescriptorSet, 2>
					descriptorWrites = {};

			descriptorWrites[0].sType =
					VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[0].dstSet = descriptorSets[i];
			descriptorWrites[0].dstBinding = 0;
			descriptorWrites[0].dstArrayElement = 0;
			descriptorWrites[0].descriptorType =
					VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			descriptorWrites[0].descriptorCount = 1;
			descriptorWrites[0].pBufferInfo = &bufferInfo;

			descriptorWrites[1].sType =
					VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[1].dstSet = descriptorSets[i];
			descriptorWrites[1].dstBinding = 1;
			descriptorWrites[1].dstArrayElement = 0;
			descriptorWrites[1].descriptorType =
				VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			descriptorWrites[1].descriptorCount = 1;
			descriptorWrites[1].pImageInfo = &imageInfo;

			vkUpdateDescriptorSets(device,
				static_cast<uint32_t>(descriptorWrites.size()),
				descriptorWrites.data(), 0, nullptr);
		}
	}

	void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage,
			VkMemoryPropertyFlags properties, VkBuffer& buffer,
			VkDeviceMemory& bufferMemory) {
		VkBufferCreateInfo bufferInfo = {};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = size;
		bufferInfo.usage = usage;
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		if(vkCreateBuffer(device, &bufferInfo, nullptr, &buffer) !=
				VK_SUCCESS) {
			throw std::runtime_error("failed to create buffer!");
		}

		VkMemoryRequirements memRequirements;
		vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

		VkMemoryAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex = findMemoryType(
				memRequirements.memoryTypeBits, properties);

		if(vkAllocateMemory(device, &allocInfo, nullptr,
				&bufferMemory) != VK_SUCCESS) {
			throw std::runtime_error(
					"failed to allocate buffer memory!");
		}

		vkBindBufferMemory(device, buffer, bufferMemory, 0);
	}

	VkCommandBuffer beginSingleTimeCommands() {
		VkCommandBufferAllocateInfo allocInfo = {};
		allocInfo.sType =
				VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandPool = commandPool;
		allocInfo.commandBufferCount = 1;

		VkCommandBuffer commandBuffer;
		vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		vkBeginCommandBuffer(commandBuffer, &beginInfo);

		return commandBuffer;
	}

	void endSingleTimeCommands(VkCommandBuffer commandBuffer) {
		vkEndCommandBuffer(commandBuffer);

		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer;

		vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
		vkQueueWaitIdle(graphicsQueue);

		vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
	}

	void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer,
			VkDeviceSize size) {
		VkCommandBuffer commandBuffer = beginSingleTimeCommands();

		VkBufferCopy copyRegion = {};
		copyRegion.size = size;
		vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1,
				&copyRegion);

		endSingleTimeCommands(commandBuffer);
	}

	uint32_t findMemoryType(uint32_t typeFilter,
			VkMemoryPropertyFlags properties) {
		VkPhysicalDeviceMemoryProperties memProperties;
		vkGetPhysicalDeviceMemoryProperties(physicalDevice,
				&memProperties);

		for(uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
			if((typeFilter & (1 << i)) &&
				(memProperties.memoryTypes[i].propertyFlags &
					properties) == properties) {
				return i;
			}
		}

		throw std::runtime_error(
				"failed to find suitable memory type!");
	}

	void createCommandBuffers() {
		commandBuffers.resize(swapChainFramebuffers.size());

		VkCommandBufferAllocateInfo allocInfo = {};
		allocInfo.sType =
				VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = commandPool;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandBufferCount = (uint32_t) commandBuffers.size();

		if(vkAllocateCommandBuffers(device, &allocInfo,
				commandBuffers.data()) != VK_SUCCESS) {
			throw std::runtime_error(
					"failed to allocate command buffers!");
		}

		for(size_t i = 0; i < commandBuffers.size(); i++) {
			VkCommandBufferBeginInfo beginInfo = {};
			beginInfo.sType =
				VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

			if(vkBeginCommandBuffer(commandBuffers[i], &beginInfo)
					!= VK_SUCCESS) {
				throw std::runtime_error(
				"failed to begin recording command buffer!");
			}

			VkRenderPassBeginInfo renderPassInfo = {};
			renderPassInfo.sType =
				VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			renderPassInfo.renderPass = renderPass;
			renderPassInfo.framebuffer = swapChainFramebuffers[i];
			renderPassInfo.renderArea.offset = {0, 0};
			renderPassInfo.renderArea.extent = swapChainExtent;

			VkClearValue clearColor = {0.0f, 0.0f, 0.0f, 1.0f};
			renderPassInfo.clearValueCount = 1;
			renderPassInfo.pClearValues = &clearColor;

			vkCmdBeginRenderPass(commandBuffers[i], &renderPassInfo,
					VK_SUBPASS_CONTENTS_INLINE);

			vkCmdBindPipeline(commandBuffers[i],
			VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

			VkBuffer vertexBuffers[] = {vertexBuffer};
			VkDeviceSize offsets[] = {0};
			vkCmdBindVertexBuffers(commandBuffers[i], 0, 1,
					vertexBuffers, offsets);

			vkCmdBindIndexBuffer(commandBuffers[i], indexBuffer, 0,
					VK_INDEX_TYPE_UINT16);

			vkCmdBindDescriptorSets(commandBuffers[i],
					VK_PIPELINE_BIND_POINT_GRAPHICS,
					pipelineLayout, 0, 1,
					&descriptorSets[i], 0, nullptr);

			vkCmdDrawIndexed(commandBuffers[i],
			static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);

			vkCmdEndRenderPass(commandBuffers[i]);

			if(vkEndCommandBuffer(commandBuffers[i]) != VK_SUCCESS){
				throw std::runtime_error(
					"failed to record command buffer!");
			}
		}
	}

	void createSyncObjects() {
		imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
		renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
		inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
//		imagesInFlight.resize(swapChainImages.size(), VK_NULL_HANDLE);

		VkSemaphoreCreateInfo semaphoreInfo = {};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		VkFenceCreateInfo fenceInfo = {};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		for(size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			if(vkCreateSemaphore(device, &semaphoreInfo, nullptr,
				&imageAvailableSemaphores[i]) != VK_SUCCESS ||
				vkCreateSemaphore(device, &semaphoreInfo,
				nullptr, &renderFinishedSemaphores[i]) !=
					VK_SUCCESS ||
				vkCreateFence(device, &fenceInfo, nullptr,
					&inFlightFences[i]) != VK_SUCCESS) {
				throw std::runtime_error(
		"failed to create synchronization objects for a frame!");
			}
		}
	}

	void updateUniformBuffer(uint32_t currentImage) {
		static auto startTime =
				std::chrono::steady_clock::now();

		auto currentTime = std::chrono::steady_clock::now();
		float time = std::chrono::duration<float,
		std::chrono::seconds::period>(currentTime - startTime).count();

		UniformBufferObject ubo = {};
		ubo.model = glm::mat4(1.0f);
		ubo.view = glm::lookAt(glm::vec3(0.0f, 0.0f, 1.0f),
		glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f));
		ubo.proj = glm::ortho(-0.5f, 0.5f, -0.5f, 0.5f, -5.0f, 5.0f);
		ubo.proj[1][1] *= -1;
		memcpy(ubo.fontdata, fixed6x13, sizeof(fixed6x13));

		// check if current view offset is outside of file size
		if(ifile_view_offset > ifile_size) {
			ifile_view_offset = (ifile_size / 16) * 16;
		}

		if(ifile_view_offset < 0) {
			ifile_view_offset = 0;
		}

		// check if current view offset if past the loaded buffer
		if(ifile_view_offset + 1200 > ifbuffer_file_offset +
							ifbuffer_size &&
				ifile_size > ifbuffer_size) {
			ifbuffer_file_offset = chunk_file_offset =
				(int64_t(ifile_view_offset) - 16 * 5) < 0? 0:
					(ifile_view_offset - 16 * 5);
			ifbuffer.resize(0); // this will force ifbuffer reload
		} else if(ifile_view_offset < ifbuffer_file_offset) {
			// check if view is before what's loaded in the buffer
			ifbuffer_file_offset = chunk_file_offset =
				(int64_t(ifile_view_offset) - 16 * 45) < 0? 0:
					(ifile_view_offset - 16 * 45);
			ifbuffer.resize(0); // this will force ifbuffer reload
		}

		ubo.chunkmeta.file_size = ifile_size;
		ubo.chunkmeta.chunk_file_offset = chunk_file_offset;
		ubo.chunkmeta.view_chunk_offset = ifile_view_offset -
				ubo.chunkmeta.chunk_file_offset;
		ubo.chunkmeta.file_size_upper = uint16_t(ifile_size >> 32);
		ubo.chunkmeta.chunk_file_offset_upper =
				uint16_t(chunk_file_offset >> 32);
		ubo.param[0] = 1.0f;
		ubo.param[1] = 1.0f;

		if(!ifbuffer.size()) {
			ifbuffer.resize(ifbuffer_size);
			size_t loaded = load_chunk(input_fname,
					ifbuffer_file_offset,
					ifbuffer_size, ifbuffer.data());
//			ifbuffer.resize(loaded);
		}

		memcpy(ubo.datachunk, ifbuffer.data(), ifbuffer.size());

		void *data;
		vkMapMemory(device, uniformBuffersMemory[currentImage], 0,
				sizeof(ubo), 0, &data);
		memcpy(data, &ubo, sizeof(ubo));
		vkUnmapMemory(device, uniformBuffersMemory[currentImage]);

//		std::cout << "Updated uniform buffers\n";
	}

	void drawFrame() {
		Application *app = static_cast<Application *> (
				glfwGetWindowUserPointer(window));

		vkWaitForFences(device, 1, &inFlightFences[currentFrame],
				VK_TRUE, UINT64_MAX);

		// update the keyboard autorepeat scrolling
		int64_t now_ms = std::chrono::duration_cast
			<std::chrono::milliseconds>(
			std::chrono::time_point_cast
			<std::chrono::milliseconds>(
			std::chrono::steady_clock::now())
			.time_since_epoch()).count();
		if(app->next_scroll_ms && now_ms >= app->next_scroll_ms) {
			scroll_one_step(window);
			app->next_scroll_ms = now_ms + app->autorepeat_rate;
		}

		// update dragging of the scroll bar with left mouse button
		if(app->scrollbar_dragged) {
			double xpos, ypos;
			glfwGetCursorPos(window, &xpos, &ypos);
			double dy = ypos - app->scrollbar_initial_y;
			// one pixel corresponds to ifile_size / 1048
			app->ifile_view_offset =
			double(app->scrollbar_initial_ifile_view_offset) +
				dy * (double(app->ifile_size) / 1008);
			app->ifile_view_offset = (app->ifile_view_offset / 16)
					* 16;
			if(app->ifile_view_offset < 0) {
				app->ifile_view_offset = 0;
			}
		}

		uint32_t imageIndex;
		VkResult result = vkAcquireNextImageKHR(device, swapChain,
				UINT64_MAX, imageAvailableSemaphores[
				currentFrame], VK_NULL_HANDLE, &imageIndex);

		if(result == VK_ERROR_OUT_OF_DATE_KHR) {
			recreateSwapChain();
			return;
		} else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR){
			throw std::runtime_error(
					"failed to acquire swap chain image!");
		}

		updateUniformBuffer(imageIndex);

//		if(imagesInFlight[imageIndex] != VK_NULL_HANDLE) {
//			vkWaitForFences(device, 1, &imagesInFlight[imageIndex],
//					VK_TRUE, UINT64_MAX);
//		}
//		imagesInFlight[imageIndex] = inFlightFences[currentFrame];

		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

		VkSemaphore waitSemaphores[] =
				{imageAvailableSemaphores[currentFrame]};
		VkPipelineStageFlags waitStages[] =
				{VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = waitSemaphores;
		submitInfo.pWaitDstStageMask = waitStages;

		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffers[imageIndex];

		VkSemaphore signalSemaphores[] =
				{renderFinishedSemaphores[currentFrame]};
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = signalSemaphores;

		vkResetFences(device, 1, &inFlightFences[currentFrame]);

		if(vkQueueSubmit(graphicsQueue, 1, &submitInfo,
				inFlightFences[currentFrame]) != VK_SUCCESS) {
			throw std::runtime_error(
				"failed to submit draw command buffer!");
		}

		VkPresentInfoKHR presentInfo = {};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = signalSemaphores;

		VkSwapchainKHR swapChains[] = {swapChain};
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = swapChains;

		presentInfo.pImageIndices = &imageIndex;

		result = vkQueuePresentKHR(presentQueue, &presentInfo);

		if(result == VK_ERROR_OUT_OF_DATE_KHR ||
			result == VK_SUBOPTIMAL_KHR || framebufferResized) {
			framebufferResized = false;
			recreateSwapChain();
		} else if (result != VK_SUCCESS) {
			throw std::runtime_error(
					"failed to present swap chain image!");
		}

		currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
	}

	VkShaderModule createShaderModule(const std::vector<uint8_t>& code) {
		VkShaderModuleCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.codeSize = code.size();
		createInfo.pCode =
				reinterpret_cast<const uint32_t*>(code.data());

		VkShaderModule shaderModule;
		if(vkCreateShaderModule(device, &createInfo, nullptr,
				&shaderModule) != VK_SUCCESS) {
			throw std::runtime_error(
					"failed to create shader module!");
		}

		return shaderModule;
	}

	VkSurfaceFormatKHR chooseSwapSurfaceFormat(
		const std::vector<VkSurfaceFormatKHR>& availableFormats) {
		for(const auto& availableFormat : availableFormats) {
			if(availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB &&
					availableFormat.colorSpace ==
					VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
				return availableFormat;
			}
		}

		return availableFormats[0];
	}

	VkPresentModeKHR chooseSwapPresentMode(
		const std::vector<VkPresentModeKHR>& availablePresentModes) {
//		for(const auto& availablePresentMode : availablePresentModes) {
//			if(availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR){
//				return availablePresentMode;
//			}
//		}
		static_cast<void>(availablePresentModes);
		return VK_PRESENT_MODE_FIFO_KHR; // VK_PRESENT_MODE_FIFO_KHR is always available
	}

	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR&
			capabilities) {
		if(capabilities.currentExtent.width != UINT32_MAX) {
			return capabilities.currentExtent;
		} else {
			int width, height;
			glfwGetFramebufferSize(window, &width, &height);

			VkExtent2D actualExtent = {
				static_cast<uint32_t>(width),
				static_cast<uint32_t>(height)
			};

			actualExtent.width = std::max(
					capabilities.minImageExtent.width,
				std::min(capabilities.maxImageExtent.width,
					actualExtent.width));
			actualExtent.height = std::max(
					capabilities.minImageExtent.height,
				std::min(capabilities.maxImageExtent.height,
					actualExtent.height));

			return actualExtent;
		}
	}

	SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device) {
		SwapChainSupportDetails details;

		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface,
				&details.capabilities);

		uint32_t formatCount;
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface,
				&formatCount, nullptr);

		if(formatCount != 0) {
			details.formats.resize(formatCount);
			vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface,
					&formatCount, details.formats.data());
		}

		uint32_t presentModeCount;
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface,
				&presentModeCount, nullptr);

		if(presentModeCount != 0) {
			details.presentModes.resize(presentModeCount);
			vkGetPhysicalDeviceSurfacePresentModesKHR(device,
		surface, &presentModeCount, details.presentModes.data());
		}

		return details;
	}

	bool isDeviceSuitable(VkPhysicalDevice device) {
		QueueFamilyIndices indices = findQueueFamilies(device);

		bool extensionsSupported = checkDeviceExtensionSupport(device);

		bool swapChainAdequate = false;
		if(extensionsSupported) {
			SwapChainSupportDetails swapChainSupport =
					querySwapChainSupport(device);
			swapChainAdequate = !swapChainSupport.formats.empty()
				&& !swapChainSupport.presentModes.empty();
		}

		VkPhysicalDeviceFeatures supportedFeatures;
		vkGetPhysicalDeviceFeatures(device, &supportedFeatures);

		return indices.isComplete() && extensionsSupported &&
		swapChainAdequate && supportedFeatures.samplerAnisotropy;
	}

	bool checkDeviceExtensionSupport(VkPhysicalDevice device) {
		uint32_t extensionCount;
		vkEnumerateDeviceExtensionProperties(device, nullptr,
				&extensionCount, nullptr);

		std::vector<VkExtensionProperties>
				availableExtensions(extensionCount);
		vkEnumerateDeviceExtensionProperties(device, nullptr,
				&extensionCount, availableExtensions.data());

		std::set<std::string> requiredExtensions(
			deviceExtensions.begin(), deviceExtensions.end());

		for(const auto& extension : availableExtensions) {
			requiredExtensions.erase(extension.extensionName);
		}

		return requiredExtensions.empty();
	}

	QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device) {
		QueueFamilyIndices indices;

		uint32_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(device,
				&queueFamilyCount, nullptr);

		std::vector<VkQueueFamilyProperties>
				queueFamilies(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(device,
				&queueFamilyCount, queueFamilies.data());

		int i = 0;
		for(const auto& queueFamily : queueFamilies) {
			if(queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
				indices.graphicsFamily = i;
			}

			VkBool32 presentSupport = false;
			vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface,
					&presentSupport);

			if(presentSupport) {
				indices.presentFamily = i;
			}

			if(indices.isComplete()) {
				break;
			}

			i++;
		}

		return indices;
	}

	std::vector<const char*> getRequiredExtensions() {
		uint32_t glfwExtensionCount = 0;
		const char** glfwExtensions;
		glfwExtensions = glfwGetRequiredInstanceExtensions(
				&glfwExtensionCount);

		std::vector<const char*> extensions(glfwExtensions,
				glfwExtensions + glfwExtensionCount);

		if(enableValidationLayers) {
			extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		}

		return extensions;
	}

	bool checkValidationLayerSupport() {
		uint32_t layerCount;
		vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

		std::vector<VkLayerProperties> availableLayers(layerCount);
		vkEnumerateInstanceLayerProperties(&layerCount,
				availableLayers.data());

		for(const char* layerName : validationLayers) {
			bool layerFound = false;

			for(const auto& layerProperties : availableLayers) {
				if(strcmp(layerName, layerProperties.layerName)
						== 0) {
					layerFound = true;
					break;
				}
			}

			if (!layerFound) {
				return false;
			}
		}

		return true;
	}

	static std::vector<char> readFile(const std::string& filename) {
		std::ifstream file(filename, std::ios::ate | std::ios::binary);

		if(!file.is_open()) {
			throw std::runtime_error("failed to open file!");
		}

		size_t fileSize = (size_t) file.tellg();
		std::vector<char> buffer(fileSize);

		file.seekg(0);
		file.read(buffer.data(), fileSize);

		file.close();

		return buffer;
	}

	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
			VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
			VkDebugUtilsMessageTypeFlagsEXT messageType,
			const VkDebugUtilsMessengerCallbackDataEXT
				*pCallbackData, void *pUserData) {
		std::cerr << "validation layer: " << pCallbackData->pMessage <<
			std::endl;

		return VK_FALSE;
	}
};

int main(int argc, char **argv) {
	if(argc < 2) {
		std::cerr << "usage: " << argv[0] << " filename\n";
		return -1;
	}

	Application app;

	app.set_fname(argv[1]);

	try {
		app.run();
	} catch (const std::exception& e) {
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	std::cout << sizeof(fixed6x13) << "\n";

	return EXIT_SUCCESS;
}
