#include "Renderable.h"

//Renderable::Renderable(UINT64 InCBIndex)
//	: CBIndex{InCBIndex}
//{
//}

Renderable::~Renderable()
{
}

void Renderable::Update(const UpdateEventArgs& EventArgs)
{
	if (UpdateCallback)
	{
		UpdateCallback(EventArgs);
	}
}
