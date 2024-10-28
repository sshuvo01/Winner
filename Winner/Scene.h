#pragma once
#include "Core.h"
#include "Renderable.h"

class Scene : public NonCopyable
{
public:
	using RablePtr = std::unique_ptr<Renderable>;
	~Scene();

	void AddRenderable(RablePtr&& Rable);
	FORCEINLINE UINT GetRenderableCount() const { return Renderables.size(); }
	FORCEINLINE Renderable* GetRenderable(UINT Index) { return Renderables[Index].get(); }

	static Scene* Get();
private:
	Scene();
	std::vector<RablePtr> Renderables;
};
