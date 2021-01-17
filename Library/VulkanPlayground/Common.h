#pragma once

class VulkanSystem;
class VulkanApplication;
class SwapChain;
class RenderPass;
class Extensions;
class Descriptor;
class Descriptor;
struct QueueIndicies;
class Shader;
class Vulkan2DFont;

#pragma warning(disable:4324)

const uint32_t INVALID_VALUE = (uint32_t)-1;
const size_t INVALID_SIZE = (size_t)-1;

class ITidy
{
public:
	virtual void Tidy(VulkanSystem& system) = 0;
};

namespace MathsContants
{
	template<typename T>
	constexpr T pi = T(3.1415926535897932385);
}

#ifdef _DEBUG
#define CHECK_VULKAN(res, err) VulkanPlayground::CheckVulkan(res, err, __FILE__, __LINE__)
#define CHECK_VULKAN_THROW(res, err) VulkanPlayground::CheckVulkan(res, err, __FILE__, __LINE__, true)
#else
#define CHECK_VULKAN(res, err) (res == VK_SUCCESS)
#define CHECK_VULKAN_THROW(res, err) (res == VK_SUCCESS)
#endif

struct MVP
{
	glm::mat4 model;
	glm::mat4 view;
	glm::mat4 projection;
};

struct UBO_light
{
	UBO_light() : ambientLevel(0.0f), diffuseLevel(0.0f), specularLevel(0.0f), shineness(0.0f)
	{}
	glm::vec3 lightPos;
	alignas(16) glm::vec3 viewPos;

	float ambientLevel;
	float diffuseLevel;
	float specularLevel;
	float shineness;
};

struct UBO_lightToon
{
	float cutoff[4];
	float value[4];
	glm::vec3 lightPos;
};

namespace Attribs
{
	enum class Type { Position, Colour, Normal, Texture, Tangent, BiTangent, Misc };

	struct Attrib
	{
		int pos;
		Type type;
		VkFormat format;
	};

	uint32_t FormatSize(VkFormat format);
	uint32_t GetStride(const std::vector<Attrib>& attribs);

	template <class T> static uint32_t NumVertices(const std::vector<T>& data, const std::vector<Attrib>& attribs)
	{
		return (uint32_t)(data.size() * sizeof(T) / GetStride(attribs));
	}

	static const std::vector<Attrib> Pos {
			{0, Type::Position, VK_FORMAT_R32G32B32_SFLOAT} };

	static const std::vector<Attrib> PosNorm {
		{0, Type::Position, VK_FORMAT_R32G32B32_SFLOAT},
		{1, Type::Normal, VK_FORMAT_R32G32B32_SFLOAT} };

	static const std::vector<Attrib> PosTex {
		{ 0, Type::Position, VK_FORMAT_R32G32B32_SFLOAT },
		{ 1, Type::Texture, VK_FORMAT_R32G32_SFLOAT } };

	static const std::vector<Attrib> PosNormTex {
		{ 0, Type::Position, VK_FORMAT_R32G32B32_SFLOAT },
		{ 1, Type::Normal, VK_FORMAT_R32G32B32_SFLOAT },
		{ 2, Type::Texture, VK_FORMAT_R32G32_SFLOAT } };

	static const std::vector<Attrib> PosNormCol {
		{ 0, Type::Position, VK_FORMAT_R32G32B32_SFLOAT },
		{ 1, Type::Normal, VK_FORMAT_R32G32B32_SFLOAT },
		{ 2, Type::Colour, VK_FORMAT_R32G32B32_SFLOAT } };

	static const std::vector<Attrib> PosNormColTex {
		{ 0, Type::Position, VK_FORMAT_R32G32B32_SFLOAT },
		{ 1, Type::Normal, VK_FORMAT_R32G32B32_SFLOAT },
		{ 2, Type::Colour, VK_FORMAT_R32G32B32_SFLOAT },
		{ 3, Type::Texture, VK_FORMAT_R32G32_SFLOAT } };

	static std::vector<Attrib> PosNormTexTanBitan {
		{0, Type::Position, VK_FORMAT_R32G32B32_SFLOAT},
		{1, Type::Normal, VK_FORMAT_R32G32B32_SFLOAT},
		{2, Type::Texture, VK_FORMAT_R32G32_SFLOAT},
		{3, Type::Tangent, VK_FORMAT_R32G32B32_SFLOAT},
		{4, Type::BiTangent, VK_FORMAT_R32G32B32_SFLOAT}
	};
}

namespace VulkanPlayground
{
	// Helper functions to create Vulkan objects
	VkInstance CreateInstance(const std::string& windowName, const Extensions& extensions);
	VkPhysicalDevice FindPhysicalDevice(VkInstance instance, VkSurfaceKHR surface, QueueIndicies& queueIndicies, const Extensions& extensions);
	VkDevice CreateLogicalDevice(VkPhysicalDevice physicalDevice, QueueIndicies queueIndicies, const Extensions& extensions, const VkPhysicalDeviceFeatures& deviceFeatures);
	VkImageView CreateImageView(const VulkanSystem& system, VkImage image, uint32_t mipLevels, VkFormat format, VkImageAspectFlags aspectFlags, const std::string& debugName, VkImageViewType viewType = VK_IMAGE_VIEW_TYPE_2D, uint32_t numImages = 1);
	VkSampler CreateSampler(const VulkanSystem& system, uint32_t mipLevels, uint32_t maxAnisotropy, VkSamplerAddressMode addressMode, VkBorderColor borderColor, VkCompareOp compareOp, const std::string& debugName);
	VkShaderModule LoadShaderModule(const VulkanSystem& system, const std::string& filename);

	// Helper functions to enable features
	void EnableFillModeNonSolid(const VkPhysicalDeviceFeatures& deviceFeatures, VkPhysicalDeviceFeatures* requiredFeatures);
	void EnableWideLines(const VkPhysicalDeviceFeatures& deviceFeatures, VkPhysicalDeviceFeatures* requiredFeatures);

	std::string GetModelFile(const std::string& projectName, const std::string& modelFile);

	void SetupProjectionMatrix(glm::mat4& projection, float aspectRatio, float fov = 45.0f, float zNear = 0.1f, float zFar = 256.0f);
	glm::mat4 CalcViewMatrix(const glm::vec3& pos, float yaw, float pitch);

	void DebugCallbackFn(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, const std::string& pMessage);
	std::string GetDeviceDetailsString(VkPhysicalDevice physicalDevice);
	std::string GetDeviceDetailsName(VkPhysicalDevice physicalDevice);

	inline void SetViewport(VkCommandBuffer commandBuffer, float x, float y, float width, float height, float minDepth = 0.0f, float maxDepth = 1.0f)
	{
		VkViewport viewport{ x, y, width, height, minDepth, maxDepth };
		vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
	}

	bool CheckVulkan(VkResult result, const char* errString, const char* file, int line, bool throwOnError = false);

	extern int showVulkanValidationMessages;	// Enable specific levels of validation messages
	inline void ShowAllValidationMessages() { showVulkanValidationMessages = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT; }
	extern bool showAvaliableObjects;	// Enable to see avaliable Vulkan resources
	extern bool showObjectCreationMessages;	// Enable to see main object lifetime messages
	extern bool failVulkanCallsOnError;	// Forces Vulkan functions that error to fail
	extern bool nameObjects;	// Name objects (enabled when in GPU debugger)
	extern const VkFormat offscreenColourBufferFormat;

	extern const VkDeviceSize* zeroOffset;
	extern const uint64_t NO_TIMEOUT;
	extern int bufferCount;

	inline void SetFloat4(float* dest, std::array<float, 4> src) { memcpy(dest, src.data(), sizeof(float) * 4); }
}

class UnicodeString
{
public:
	UnicodeString() {}
	UnicodeString(int resourceID);
	UnicodeString(const std::string& utf8String);
	UnicodeString(const std::wstring& utf16String);
	UnicodeString(const std::u32string& utf32String);

	UnicodeString(const char* asciiString) : UnicodeString(std::string(asciiString)) {}
	UnicodeString(const wchar_t* wideString) : UnicodeString(std::wstring(wideString)) {}
	UnicodeString(const char32_t* u32String) : UnicodeString(std::u32string(u32String)) {}

	void Load(int resourceID);

	const std::u32string& GetUtf32String() const { return string; }
	operator std::u32string&() { return string; }
	bool empty() const { return string.empty(); }

private:
	std::u32string string;
	
	static std::map<int, std::u32string> strings;
};
