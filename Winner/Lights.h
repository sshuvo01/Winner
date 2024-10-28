#pragma once
#include "Core.h"

class Light : public NonCopyable
{
public:
	Light();
	~Light();

	FORCEINLINE void SetColor(CRef<DirectX::XMFLOAT3> InColor) { Color = InColor; }
	FORCEINLINE DirectX::XMFLOAT3 GetColor() const { return Color; }
private:
	DirectX::XMFLOAT3 Color;
};

//-----------------------------------------------------------//
class DirectionalLight : public Light
{
public: 
	DirectionalLight();
	~DirectionalLight();
	
	FORCEINLINE void SetDirection(CRef<DirectX::XMFLOAT3> InDirection) { Direction = InDirection; }
	FORCEINLINE DirectX::XMFLOAT3 GetDirection() const { return Direction; }

private:
	DirectX::XMFLOAT3 Direction;
};
