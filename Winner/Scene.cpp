#include "Scene.h"

Scene* Scene::Get()
{
	static Scene Singleton;
	return &Singleton;
}

Scene::Scene()
{
}

Scene::~Scene()
{
}

void Scene::AddRenderable(RablePtr&& Rable)
{
	// TODO: Add some validation if needed
	Renderables.push_back(std::move(Rable));
}
