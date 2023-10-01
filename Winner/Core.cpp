#include "Core.h"

UINT Useful::GetConstantBufferByteSize(UINT ByteSize)
{
	return (ByteSize + 255) & ~255;
}
