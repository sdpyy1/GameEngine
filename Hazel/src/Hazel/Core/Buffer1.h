#pragma once

namespace Hazel {

	struct Buffer1
	{
		void* Data = nullptr;
		uint64_t Size = 0;

		Buffer1() = default;

		Buffer1(const void* data, uint64_t size = 0)
			: Data((void*)data), Size(size) {
		}

		static Buffer1 Copy(const Buffer1& other)
		{
			Buffer1 Buffer1;
			Buffer1.Allocate(other.Size);
			memcpy(Buffer1.Data, other.Data, other.Size);
			return Buffer1;
		}

		static Buffer1 Copy(const void* data, uint64_t size)
		{
			Buffer1 Buffer1;
			Buffer1.Allocate(size);
			memcpy(Buffer1.Data, data, size);
			return Buffer1;
		}

		void Allocate(uint64_t size)
		{
			delete[](byte*)Data;
			Data = nullptr;
			Size = size;

			if (size == 0)
				return;

			Data = new byte[size];
		}

		void Release()
		{
			delete[](byte*)Data;
			Data = nullptr;
			Size = 0;
		}

		void ZeroInitialize()
		{
			if (Data)
				memset(Data, 0, Size);
		}

		template<typename T>
		T& Read(uint64_t offset = 0)
		{
			return *(T*)((byte*)Data + offset);
		}

		template<typename T>
		const T& Read(uint64_t offset = 0) const
		{
			return *(T*)((byte*)Data + offset);
		}

		byte* ReadBytes(uint64_t size, uint64_t offset) const
		{
			HZ_CORE_ASSERT(offset + size <= Size, "Buffer1 overflow!");
			byte* Buffer1 = new byte[size];
			memcpy(Buffer1, (byte*)Data + offset, size);
			return Buffer1;
		}

		void Write(const void* data, uint64_t size, uint64_t offset = 0)
		{
			HZ_CORE_ASSERT(offset + size <= Size, "Buffer1 overflow!");
			memcpy((byte*)Data + offset, data, size);
		}

		operator bool() const
		{
			return (bool)Data;
		}

		byte& operator[](int index)
		{
			return ((byte*)Data)[index];
		}

		byte operator[](int index) const
		{
			return ((byte*)Data)[index];
		}

		template<typename T>
		T* As() const
		{
			return (T*)Data;
		}

		inline uint64_t GetSize() const { return Size; }
	};

	struct Buffer1Safe : public Buffer1
	{
		~Buffer1Safe()
		{
			Release();
		}

		static Buffer1Safe Copy(const void* data, uint64_t size)
		{
			Buffer1Safe Buffer1;
			Buffer1.Allocate(size);
			memcpy(Buffer1.Data, data, size);
			return Buffer1;
		}
	};
}
