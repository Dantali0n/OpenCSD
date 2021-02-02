#ifndef AIRGLOW_ENTRY_HPP
#define AIRGLOW_ENTRY_HPP

#include <iostream>
#include <map>
#include <optional>

#include <GLFW/glfw3.h>
#include <vulkan/vulkan.hpp>

namespace airglow::entry {

	struct QueueFamilyIndices {
		std::optional<uint32_t> graphicsFamily;

		bool isComplete() {
			return graphicsFamily.has_value();
		}
	};

	class EntryTriangle {
	public:
		void run();
	protected:
		static const bool enableValidationLayers;
		static const std::vector<const char*> validationLayers;

		GLFWwindow *window;

		VkInstance instance = VK_NULL_HANDLE;
		VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
		VkDevice device = VK_NULL_HANDLE;
		VkQueue graphicsQueue = VK_NULL_HANDLE;
		VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;
	private:
		void initVulkan();
		void mainLoop();
		void cleanup();

		/**
		 * Initializes the Vulkan instance
		 */
		void createInstance();

		/**
		 * Register a callback for the class member instance, assumes the
		 * VkInstance is initialized.
		 */
		void setupDebugMessenger();

		/**
		 * Find the best physical Vulkan device and select it for the current
		 * VkInstance (member variable). Assumes the instance is initiated.
		 */
		void pickPhysicalDevice();

		/**
		 * Create a logical device based on the current VkPhysicalDevice
		 * (member variable). Assumes variable properly initialized.
		 */
		void createLogicalDevice();

		/**
		 * Determine the suitability of a physical device
		 * @param device, the device to rate the suitability for
		 * @return a score depending on the device features
		 */
		int rateDeviceSuitability(VkPhysicalDevice device);

		/**
		 * Find a suitable queue for the physical device
		 * @param device, the device to find a queue for.
		 * @return The found indices
		 */
		QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);

		/**
		 * enumerate all supported Vulkan extensions
		 */
		void enumerateExtensions();

		/**
		 * Determine the Vulkan extensions required by GLFW3
		 * @return vector containing the name of each required Vulkan extension
		 */
		std::vector<const char*> getRequiredExtensions();

		/**
		 * Determine if all layers defined in validationLayers are supported
		 * @return True if all layers supported, false otherwise
		 */
		bool checkValidationLayerSupport();

		/**
		 * Populate a VkDebugUtilsMessengerCreateInfoEXT struct for very
		 * verbose debugging.
		 * @param createInfo, reference to place populated fields into
		 */
		void populateDebugMessengerCreateInfo(
			VkDebugUtilsMessengerCreateInfoEXT& createInfo);

		/**
		 * Debug callback
		 */
		static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallbackVulkan(
			VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
			VkDebugUtilsMessageTypeFlagsEXT messageType,
			const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
			void* pUserData);

		static void debugCallbackGLFW(int, const char *message);
	};
}

#endif //AIRGLOW_ENTRY_HPP
