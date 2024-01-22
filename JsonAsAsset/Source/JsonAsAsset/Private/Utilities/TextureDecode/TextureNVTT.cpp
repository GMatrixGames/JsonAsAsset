// Copyright JAA Contributors 2023-2024

#include "TextureNVTT.h"

#include "nvcore/Stream.h"

class NVTTStream : public nv::Stream {
public:
	NVTTStream(uint8* mem0, uint size0, uint8* mem1, uint size1, bool isLoading)
		: m_mem0(mem0)
		  , m_mem1(mem1)
		  , m_pos(0)
		  , m_size0(size0)
		  , m_size1(size1)
		  , m_loading(isLoading) {
	}

	virtual uint serialize(void* data, uint len) {
		nvDebugCheck(!isError());

		uint left;
		uint8* ptr;
		if (m_pos < m_size0) {
			left = m_size0 - m_pos;
			ptr = m_mem0 + m_pos;
		} else {
			left = m_size1 - (m_pos - m_size0);
			ptr = m_mem1 + (m_pos - m_size0);
		}

		if (len > left) len = left;
		if (m_loading)
			FMemory::Memcpy(data, ptr, len);
		else
			FMemory::Memcpy(ptr, data, len);
		m_pos += len;

		return len;
	}

	virtual void seek(uint pos) {
		nvDebugCheck(!isError());
		m_pos = pos;
		nvDebugCheck(!isError());
	}

	virtual uint tell() const {
		nvDebugCheck(m_pos < m_size0 + m_size1);
		return m_pos;
	}

	virtual uint size() const {
		return m_size0 + m_size1;
	}

	virtual bool isError() const {
		return m_mem0 == NULL || m_mem1 == NULL || m_pos > m_size0 + m_size1;
	}

	virtual void clearError() {
		// Nothing to do.
	}

	virtual bool isAtEnd() const {
		return m_pos == m_size0 + m_size1;
	}

	virtual bool isSeekable() const {
		return true;
	}

	virtual bool isLoading() const {
		return m_loading;
	}

	virtual bool isSaving() const {
		return !m_loading;
	}

private:
	uint8* m_mem0;
	uint8* m_mem1;
	uint m_pos;
	uint m_size0;
	uint m_size1;
	bool m_loading;
};

void DecodeDDS(const unsigned char* Data, int SizeX, int SizeY, int SizeZ, nv::DDSHeader& header, nv::Image& image) {
	uint8 dummy[128];
	NVTTStream* stream = new NVTTStream(dummy, sizeof(dummy), const_cast<unsigned char*>(Data), SizeX * SizeY * SizeZ * 4, true); // deleted in DirectDrawSurface destructor
	nv::DirectDrawSurface dds(stream); // will try to read DDS header, it's zeroed
	dds.header = header; // set real header contents
	dds.mipmap(&image, 0, 0);
}
