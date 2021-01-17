
#include <VulkanPlayground\Includes.h>
#include <VulkanPlayground\PixelData.h>
#include <numeric>
#include <random>

class INoise
{
public:
	INoise(float noiseScale) : noiseScale(noiseScale) {}
	virtual ~INoise() {}
	virtual float Noise(float nx, float ny, float nz) = 0;

protected:
	float noiseScale;
};

// Translation of Ken Perlin's JAVA implementation (http://mrl.nyu.edu/~perlin/noise/)
template <typename T>
class PerlinNoise : public INoise
{
private:
	uint32_t permutations[512]{};
	T fade(T t)
	{
		return t * t * t * (t * (t * (T)6 - (T)15) + (T)10);
	}
	T lerp(T t, T a, T b)
	{
		return a + t * (b - a);
	}
	T grad(int hash, T x, T y, T z)
	{
		// Convert LO 4 bits of hash code into 12 gradient directions
		int h = hash & 15;
		T u = h < 8 ? x : y;
		T v = h < 4 ? y : h == 12 || h == 14 ? x : z;
		return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
	}

public:
	PerlinNoise(float noiseScale) : INoise(noiseScale)
	{
		// Generate random lookup for permutations containing all numbers from 0..255
		std::vector<uint8_t> plookup;
		plookup.resize(256);
		std::iota(plookup.begin(), plookup.end(), '\0');
		std::default_random_engine rndEngine(std::random_device{}());
		std::shuffle(plookup.begin(), plookup.end(), rndEngine);

		for (uint32_t i = 0; i < 256; i++)
		{
			permutations[i] = permutations[256 + i] = plookup[i];
		}
	}
	T noise(T x, T y, T z)
	{
		// Find unit cube that contains point
		int32_t X = (int32_t)floor(x) & 255;
		int32_t Y = (int32_t)floor(y) & 255;
		int32_t Z = (int32_t)floor(z) & 255;
		// Find relative x,y,z of point in cube
		x -= floor(x);
		y -= floor(y);
		z -= floor(z);

		// Compute fade curves for each of x,y,z
		T u = fade(x);
		T v = fade(y);
		T w = fade(z);

		// Hash coordinates of the 8 cube corners
		uint32_t A = permutations[X] + Y;
		uint32_t AA = permutations[A] + Z;
		uint32_t AB = permutations[A + 1] + Z;
		uint32_t B = permutations[X + 1] + Y;
		uint32_t BA = permutations[B] + Z;
		uint32_t BB = permutations[B + 1] + Z;

		// And add blended results for 8 corners of the cube;
		T res = lerp(w, lerp(v,
			lerp(u, grad(permutations[AA], x, y, z), grad(permutations[BA], x - 1, y, z)), lerp(u, grad(permutations[AB], x, y - 1, z), grad(permutations[BB], x - 1, y - 1, z))),
			lerp(v, lerp(u, grad(permutations[AA + 1], x, y, z - 1), grad(permutations[BA + 1], x - 1, y, z - 1)), lerp(u, grad(permutations[AB + 1], x, y - 1, z - 1), grad(permutations[BB + 1], x - 1, y - 1, z - 1))));
		return res;
	}

	float Noise(float nx, float ny, float nz) override
	{
		return noiseScale * noise(nx, ny, nz);
	}
};

// Fractal noise generator based on perlin noise above
template <typename T>
class FractalNoise : public PerlinNoise<T>
{
private:
	uint32_t octaves;
	T frequency{};
	T amplitude{};
	T persistence;

public:
	FractalNoise(float noiseScale) : PerlinNoise<T>(noiseScale)
	{
		octaves = 6;
		persistence = (T)0.5;
	}

	float Noise(float nx, float ny, float nz) override
	{
		return noise(nx * INoise::noiseScale, ny * INoise::noiseScale, nz * INoise::noiseScale);
	}

	T noise(T x, T y, T z)
	{
		T sum = (T)0;
		T max = (T)0;
		frequency = (T)1;
		amplitude = (T)1;
		for (uint32_t i = 0; i < octaves; i++)
		{
			sum += PerlinNoise<T>::noise(x * frequency, y * frequency, z * frequency) * amplitude;
			max += amplitude;
			amplitude *= persistence;
			frequency *= (T)2;
		}

		sum = sum / max;
		return (sum + (T)1.0) / (T)2.0;
	}
};

void GenerateNoise(bool fractal, float noiseScale, PixelData* pd)
{
	uint32_t widthSize = pd->width;
	uint32_t heightSize = pd->height;
	uint32_t depthSize = pd->mipLevels;

	std::unique_ptr<INoise> generator;
	if (fractal)
		generator = std::make_unique<FractalNoise<float>>(noiseScale);
	else
		generator = std::make_unique<PerlinNoise<float>>(noiseScale);

	// Generate perlin based noise
	std::cout << "Generating " << widthSize << " x " << heightSize << " x " << depthSize << " noise texture..." << std::endl;

	auto tStart = std::chrono::high_resolution_clock::now();

	pd->size = (uint64_t)(widthSize * heightSize * depthSize * sizeof(uint8_t));
	pd->pixels = new unsigned char[pd->size];
	uint8_t* pData = (uint8_t*)pd->pixels;

	for (uint32_t z = 0; z < depthSize; z++)
	{
		for (uint32_t y = 0; y < heightSize; y++)
		{
			for (uint32_t x = 0; x < widthSize; x++)
			{
				float nx = (float)x / (float)widthSize;
				float ny = (float)y / (float)heightSize;
				float nz = (float)z / (float)depthSize;

				float n = generator->Noise(nx, ny, nz);

				n = n - floor(n);
				*pData = static_cast<uint8_t>(floor(n * 255));
				pData++;
			}
		}
	}

	auto tEnd = std::chrono::high_resolution_clock::now();
	auto tDiff = std::chrono::duration<double, std::milli>(tEnd - tStart).count();

	std::cout << "Done in " << tDiff << "ms" << std::endl;
}
