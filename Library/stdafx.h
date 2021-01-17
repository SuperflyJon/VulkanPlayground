// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "targetver.h"

#pragma warning(disable:26812)  //enum class warning

#include <vulkan/vulkan.h>

#include <string>
#include <sstream>
#include <vector>
#include <map>
#include <memory>
#include <set>
#include <list>
#include <iostream>
#include <algorithm>
#include <functional>
#include <array>
#include <chrono>
#include <fstream>

#pragma warning(push)
#pragma warning(disable:4201 4127 4324 26451 26495)	// Disable some warning in complicated template code
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#pragma warning(pop)
