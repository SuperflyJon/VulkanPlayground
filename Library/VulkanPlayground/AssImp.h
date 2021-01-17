#pragma once

#include <string>
#include <vector>

class Model;
namespace Attribs { struct Attrib; }
struct aiScene;

namespace AssImp
{
	const aiScene* ImportFile(const std::string& modelpath, bool correctDodgyModel);
	void ProcessScene(const aiScene& scene, Model& model, const std::vector<Attribs::Attrib>& attribs, const std::array<double, 3>& translation);
	std::string GetErrorString();
	void Tidy(const aiScene* scene);
}
