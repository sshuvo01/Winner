#pragma once
#include "Core.h"
#include "Events.h"
#include "Mesh.h"
#include <functional>

#define RABLEUPDATECALLBACK(Rable, LambdaBody) Rable->SetUpdateCallback([Obj=Rable.get()](const UpdateEventArgs& EventArgs)LambdaBody);

class Renderable : public NonCopyable
{
public:
	//Renderable(UINT64 InCBIndex);
	Renderable() { }
	virtual ~Renderable();
	void Update(const UpdateEventArgs& EventArgs);
	//FORCEINLINE const DirectX::XMFLOAT4X4* GetWorldMat() const { return &WorldMat; }
	//FORCEINLINE const std::shared_ptr<MeshGeometry> GetMeshGeo() {  }
	DirectX::XMMATRIX WorldMat = Useful::IdentityMatrix();
	UINT64 CBIndex = 0;
	std::shared_ptr<MeshGeometry> MeshGeo;
	std::unordered_map<std::string, UINT64> HeapIndexMap;
	//std::shared_ptr<MeshData> MeshData; we don't really need this

	template<typename LAMBDA>
	void SetUpdateCallback(LAMBDA&& InUpdateCallbackLambda) // TODO: experiment with universal reference
	{
		UpdateCallback = std::forward<LAMBDA>(InUpdateCallbackLambda);
	}

private:
	std::function<void(const UpdateEventArgs& EventArgs)> UpdateCallback;
};

