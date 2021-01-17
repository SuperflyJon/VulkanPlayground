#pragma once

#include <vector>

class VulkanApplication;
class GLFW;

class AppGroups
{
public:
	typedef std::vector<VulkanApplication*> AppList;

	AppGroups(bool fontsEnabled);

	VulkanApplication& GetCurrentApp()
	{
		return *groups[currentGroup][currentSubgroup];
	}

	bool MoveToNextGroup()
	{
		if (currentGroup + 1 < (int)groups.size())
		{
			groupPosition[currentGroup] = currentSubgroup;
			currentGroup++;
			currentSubgroup = groupPosition[currentGroup];
			return true;
		}
		else
			return false;
	}
	bool MoveToPreviousGroup()
	{
		if (currentGroup > 0)
		{
			groupPosition[currentGroup] = currentSubgroup;
			currentGroup--;
			currentSubgroup = groupPosition[currentGroup];
			return true;
		}
		else
			return false;
	}
	bool MoveToNextSubgroup()
	{
		if (currentSubgroup + 1 < (int)groups[currentGroup].size())
		{
			currentSubgroup++;
			return true;
		}
		else
			return false;
	}
	bool MoveToPreviousSubgroup()
	{
		if (currentSubgroup > 0)
		{
			currentSubgroup--;
			return true;
		}
		else
			return false;
	}

	bool CheckKeys(GLFW& window);

	bool SceneChanged()
	{
		if (newScene)
		{
			newScene = false;
			return true;
		}
		else
			return false;
	}

	uint32_t GetCurrentGroupNum() { return currentGroup + 1; }
	uint32_t GetNumGroups() { return (uint32_t)groups.size(); }
	uint32_t GetCurrentSubgroupNum() { return currentSubgroup + 1; }
	uint32_t GetNumSubGroups() { return (uint32_t)groups[currentGroup].size(); }

protected:
	void AddGroup(AppList apps)
	{
		groups.push_back(apps);
		groupPosition.push_back(0);
	}

private:
	std::vector<AppList> groups;
	std::vector<uint32_t> groupPosition;
	uint32_t currentGroup;
	uint32_t currentSubgroup;
	bool newScene = false;
};
