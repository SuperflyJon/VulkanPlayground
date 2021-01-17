
#include <random>
#include <time.h>

constexpr auto FLAME_PARTICLE = 0;
constexpr auto SMOKE_PARTICLE = 1;

class Particle
{
public:
	struct ParticleVertexData
	{
		glm::vec3 pos;
		float alpha{ 0 };
		float size{ 0 };
		float rotation{ 0 };
		int type{ 0 };
	};

	Particle(std::default_random_engine &rndEngine, int _testBigParticle) : rndEngine(rndEngine), growAmount(0), initialAlpha(0), rotationSpeed(0), smoky(false), testBigParticle(_testBigParticle)
	{}

	float rnd(float range)
	{
		if (range <= 0)
			return 0;

		std::uniform_real_distribution<float> rndDist(0.0f, range);
		return rndDist(rndEngine);
	}

	void Init(ParticleVertexData& data)
	{
constexpr auto FLAME_RADIUS = 10.0f;
		const glm::vec3 emitterPos = glm::vec3(0.0f, -6.0f, 0.0f);
		const glm::vec3 minVel = glm::vec3(-1.0f, 5.5f, -1.0f);
		const glm::vec3 maxVel = glm::vec3(1.0f, 20.0f, 1.0f);

		auto RndRange = [this](float min, float max) { return min + rnd(max - min); };
		vel = glm::vec3(RndRange(minVel.x, maxVel.x), RndRange(minVel.y, maxVel.y), RndRange(minVel.z, maxVel.z));
		initialAlpha = data.alpha = 1.0f;
		data.size = 1.0f + rnd(0.5f);
		data.rotation = rnd(2.0f * float(MathsContants::pi<float>));
		rotationSpeed = rnd(4.0f) - 2;

		// Get random point in a circle, favour the center
		std::normal_distribution<float> rndDist(FLAME_RADIUS, 2.0f);
		float distanceFromCentre = fabs(rndDist(rndEngine) - FLAME_RADIUS);
		float point = (FLAME_RADIUS - distanceFromCentre) * 2;
		float xtraHeight = rnd(point);
		growAmount = point + xtraHeight;	// Taller at top

		smoky = (distanceFromCentre < 3.0f);

		// Rotate point randomly
		float theta = rnd(2.0f * float(MathsContants::pi<float>));

		data.pos.x = distanceFromCentre * 2 * cos(theta);
		data.pos.y = 0;
		data.pos.z = distanceFromCentre * 2 * sin(theta);
		data.pos += emitterPos;

		data.size *= point / (FLAME_RADIUS * 2);
		if (testBigParticle)
		{
			data.size *= 3;
			if (testBigParticle == 2)
			{
				Update(data, 1);
				smoky = true;
			}
		}

		data.type = FLAME_PARTICLE;
	}

	void Update(ParticleVertexData& data, float particleTimer)
	{
		glm::vec3 posChange = (vel * particleTimer * 7.0f) / 10.0f;
		data.pos -= posChange;
		growAmount -= posChange.y;
		if (growAmount < 10)
			data.alpha = initialAlpha * (growAmount / 10.0f);
		if (data.type == FLAME_PARTICLE)
			data.size -= particleTimer * 0.5f;
		else
			data.size -= particleTimer * 0.25f;
		data.rotation += particleTimer * rotationSpeed;

		if (growAmount < 0 || (testBigParticle == 2 && data.type == FLAME_PARTICLE))
		{	// Expired
			Transition(data);
		}
	}

	void Transition(ParticleVertexData& data)
	{
		if (data.type == FLAME_PARTICLE && smoky && testBigParticle != 1)
		{	// Convert to smoke
			growAmount = 8.0f + rnd(5.0f);
			data.size = 0.05f + rnd(0.2f);
			data.alpha = initialAlpha = 0.5f + rnd(0.3f);
			data.pos.y *= 0.7f;	// Move down a bit
			data.type = SMOKE_PARTICLE;
			if (testBigParticle)
			{
				data.size *= 3;
				growAmount *= 1.5f;
				data.alpha *= 2;
			}
		}
		else
		{
			Init(data);	// Recreate as flame
		}
	}

private:
	glm::vec3 vel;
	float rotationSpeed;
	float growAmount;
	bool smoky;
	int testBigParticle;
	float initialAlpha;

	std::default_random_engine &rndEngine;
};

class Particles : public ITidy
{
public:
	void Prepare(VulkanSystem& system, uint32_t numParticles, int testBigParticles = false)
	{
		Tidy(system);
		particleInfo.reserve(numParticles);
		particleData.resize(numParticles);

		rndEngine.seed(std::random_device{}());
		std::uniform_real_distribution<float> rndDist(0.0f, 4);

		for (uint32_t i = 0; i < numParticles; i++)
		{
			particleInfo.emplace_back(rndEngine, testBigParticles);
			particleInfo.back().Init(particleData[i]);
			particleInfo.back().Update(particleData[i], rndDist(rndEngine));	// Move initial position along a bit to make start more realistic
		}

		VkDeviceSize bufferSize = numParticles * sizeof(Particle::ParticleVertexData);
		vertexBuffer.Create(system, bufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, "vertexData");
		vertexMemory = vertexBuffer.Map(system.GetDevice());
		memcpy(vertexMemory, particleData.data(), vertexBuffer.GetBufferSize());
	}

	void Update(float frameTime)
	{
		float particleTimer = frameTime * 0.45f;
		for (uint32_t i = 0; i < particleInfo.size(); i++)
		{
			particleInfo[i].Update(particleData[i], particleTimer);
		}
		memcpy(vertexMemory, particleData.data(), vertexBuffer.GetBufferSize());
	}

	void Draw(VkCommandBuffer commandBuffer)
	{
		vertexBuffer.Bind(commandBuffer);
		vkCmdDraw(commandBuffer, (uint32_t)particleData.size(), 1, 0, 0);
	}

	void Tidy(VulkanSystem& system) override
	{
		particleInfo.clear();
		particleData.clear();
		vertexBuffer.DestroyBuffer(system);
	}

private:
	void* vertexMemory{};
	Buffer vertexBuffer;
	std::vector<Particle> particleInfo;
	std::vector<Particle::ParticleVertexData> particleData;

	std::default_random_engine rndEngine;
};

// Simple class to "jitter" a value around a bit over time
class Jitter
{
	const float JIT_TIME = 0.1f;
public:
	Jitter() : value(0), jitAmount(0), jitTime(0)
	{
		NextJitter();
	}
	void NextJitter()
	{
		jitAmount = (rand() / (RAND_MAX * 5.0f)) - 0.1f;
		float overshoot = abs(value + jitAmount) - 0.5f;
		if (overshoot > 0)
		{
			if (value > 0)
				jitAmount = -overshoot;
			else
				jitAmount = overshoot;
		}
		jitTime = JIT_TIME;
	}
	float Vary(float duration)
	{
		if (duration > jitTime)
			duration = jitTime;

		value += jitAmount / (JIT_TIME / duration);

		jitTime -= duration;
		if (jitTime <= 0)
			NextJitter();

		return value;
	}
private:
	float value;
	float jitTime, jitAmount;
};
