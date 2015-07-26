//
// Tomato OS - C 运行时库
// 内存相关
// (c) 2015 SunnyCase
// 创建日期：2015-4-23
#include "stdafx.h"
#include "ExternVariables.h"
#include "crtinit.h"

namespace {
	struct _heapheader
	{
		void* heap_ptr;		// 系统堆指针
		size_t page_count;
		size_t obj_count;
	};

	struct _heapentry : public _heapinfo
	{
		_heapheader* header;
		_heapentry* prev;
		_heapentry* next;

		void free()
		{
			if (_useflag != _USEDENTRY) return;

			_useflag = _FREEENTRY;
			header->obj_count--;
			// 同一个堆中的可以合并
			if (next && next->_useflag == _FREEENTRY && header == next->header)
			{
				_size += next->_size + sizeof(_heapentry);
				next = next->next;
			}
			if (!header->obj_count)
			{

#ifdef CRT_KERNEL
				memoryManager->FreePages(header->heap_ptr, header->page_count);
#endif
			}
		}
	};

	_heapentry* _heapHead = nullptr;
	enum : size_t
	{
		DefaultHeapPages = 16,
		DefaultHeapSize = DefaultHeapPages * 4096
	};
}

using namespace Tomato::CRT;

// 初始化堆
void Tomato::CRT::_crt_init_heap()
{
#ifdef CRT_KERNEL
	// 内核模式未分配堆，需自行分配
	uint8_t* heapData = (uint8_t*)memoryManager->AllocatePages(DefaultHeapPages);
#else
	uint8_t* heapData = nullptr;
#endif
	auto header = (_heapheader*)(heapData);
	header->heap_ptr = heapData;
	header->page_count = DefaultHeapPages;
	header->obj_count = 1;
	auto heap = (_heapentry*)(heapData + sizeof(_heapheader));
	heap->_pentry = (int*)((uint8_t*)heap + sizeof(_heapentry));
	heap->_size = DefaultHeapSize - sizeof(_heapheader) - sizeof(_heapentry);
	heap->_useflag = _FREEENTRY;
	heap->header = header;
	heap->prev = nullptr;
	heap->next = nullptr;
	_heapHead = heap;
}

void* __cdecl malloc(
	_In_ _CRT_GUARDOVERFLOW size_t _Size
	)
{
	// 包含控制块的大小
	const auto size_with_cb = _Size + sizeof(_heapentry);
	// 在堆链表中寻找
	for (auto heap = _heapHead; heap; heap = heap->next)
	{
#ifdef CRT_KERNEL
		bootVideo->PutFormat(L"Heap at %x\r\n", heap);
#endif
		if (heap->_useflag == _FREEENTRY)
		{
			// 大小相等则直接修改为使用中
			if (heap->_size == _Size)
			{
				heap->_useflag = _USEDENTRY;
				return heap->_pentry;
			}
			// 否则分离出来
			else if (heap->_size >= size_with_cb)
			{
				heap->_size -= size_with_cb;
				auto newheap = (_heapentry*)((uint8_t*)(heap->_pentry) + heap->_size);
				newheap->_size = _Size;
				newheap->header = heap->header;
				newheap->prev = heap;
				newheap->next = heap->next;
				newheap->_useflag = _USEDENTRY;
				newheap->_pentry = (int*)((uint8_t*)(newheap)+sizeof(_heapentry));
				heap->header->obj_count++;
				heap->next = newheap;
				return newheap->_pentry;
			}
		}
	}
	return nullptr;
}

void __cdecl free(
	_Pre_maybenull_ _Post_invalid_ void* _Block
	)
{
	if (!_Block)return;

	auto heap = (_heapentry*)((uint8_t*)_Block - sizeof(_heapentry));
	heap->free();
}