#pragma once
namespace GameEngine {

    typedef struct IndexRange
    {
        uint32_t begin = 0;
        uint32_t size = 0;
    };

	class IndexAllocator {
    public:
        IndexAllocator(uint32_t maxIndex = UINT32_MAX);

        uint32_t Allocate();
        IndexRange Allocate(uint32_t size);
        void Release(uint32_t index); // 和Pool设计一样，释放时才缓存
        void Release(IndexRange range);

        inline uint32_t GetSize() { return maxIndex; }

    private:
        uint32_t maxIndex;
        uint32_t nextIndex;

        std::list<IndexRange> unusedIndex;  // 双向链表，管理释放的索引，再次分配时会再这里找可用的
	};








}
