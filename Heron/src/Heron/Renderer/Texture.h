#pragma once

#include <string>
#include <memory>
#include "Heron/Core/Core.h"

namespace Heron {

	class Texture {
	public:
		virtual ~Texture() = default;

		virtual uint32_t GetWidth() const = 0;
		virtual uint32_t GetHeight() const = 0;

		virtual void Bind(uint32_t slot = 0) const = 0;

		virtual bool operator==(const Texture& other) const = 0;
	};

	class Texture2D : public Texture {
	public:
		static Ref<Texture2D> Create(uint32_t width, uint32_t height);
		static Ref<Texture2D> Create(const std::string& path);

		virtual void SetData(void* data, uint32_t size) {}
	};

	class TextureCube : public Texture {
	public:
		static Ref<TextureCube> Create(const std::vector<std::string>& paths);
	};

}
