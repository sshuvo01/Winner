#include "Lights.h"

Light::Light()
	: Color { 1.f, 1.f, 1.f }
{
}

Light::~Light()
{
}

DirectionalLight::DirectionalLight()
	: Direction{ 0.f, 1.f, 0.f }
{
}

DirectionalLight::~DirectionalLight()
{
}
