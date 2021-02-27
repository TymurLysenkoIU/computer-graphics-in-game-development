#pragma once

#include "utils/error_handler.h"

#include <algorithm>
#include <linalg.h>
#include <vector>


using namespace linalg::aliases;

namespace cg
{
template<typename T>
class resource
{
public:
  resource(size_t size);
  resource(size_t x_size, size_t y_size);
  ~resource();

  const T* get_data();
  T& item(size_t item);
  T& item(size_t x, size_t y);

  size_t get_size_in_bytes() const;
  size_t get_number_of_elements() const;
  size_t get_stride() const;

  typename std::vector<T>::iterator begin() noexcept;
  typename std::vector<T>::iterator end() noexcept;

  typename std::vector<T>::const_iterator begin() const noexcept;
  typename std::vector<T>::const_iterator end() const noexcept;

private:
  std::vector<T> data;
  size_t item_size = sizeof(T);
  size_t stride = 0;
};

template<typename T>
resource<T>::resource(size_t size)
{
  data.resize(size);
  stride = 0;
}

template<typename T>
resource<T>::resource(size_t x_size, size_t y_size)
{
  data.resize(x_size * y_size);
  stride = x_size;
}

template<typename T>
resource<T>::~resource()
{
  data.clear();
}

template<typename T>
const T* resource<T>::get_data()
{
  return data.data();
}

template<typename T>
T& resource<T>::item(size_t item)
{
  return data.at(item);
}

template<typename T>
T& resource<T>::item(size_t x, size_t y)
{
  return data.at(y * stride + x);
}

template<typename T>
size_t resource<T>::get_size_in_bytes() const
{
  return data.size() * item_size;
}

template<typename T>
size_t resource<T>::get_number_of_elements() const
{
  return data.size();
}

template<typename T>
size_t resource<T>::get_stride() const
{
  return stride;
}

template<typename T>
typename std::vector<T>::iterator resource<T>::begin() noexcept
{
  return data.begin();
}

template<typename T>
typename std::vector<T>::iterator resource<T>::end() noexcept
{
  return data.end();
}

template<typename T>
typename std::vector<T>::const_iterator resource<T>::begin() const noexcept
{
  return data.begin();
}

template<typename T>
typename std::vector<T>::const_iterator resource<T>::end() const noexcept
{
  return data.end();
}

struct color
{
  static color from_float3(const float3& in)
  {
    return color{ in.x, in.y, in.z };
  };

  float r;
  float g;
  float b;
};

struct unsigned_color
{
  static unsigned_color from_color(const color& color)
  {
    return unsigned_color{
      static_cast<unsigned char>(std::clamp(255.f * color.r, 0.f, 255.f)),
      static_cast<unsigned char>(std::clamp(255.f * color.g, 0.f, 255.f)),
      static_cast<unsigned char>(std::clamp(255.f * color.b, 0.f, 255.f)),
    };
  }

  float3 to_float3()
  {
    return float3(r, g, b) / 255.f;
  }

  unsigned char r;
  unsigned char g;
  unsigned char b;
};


struct vertex
{
  float x;
  float y;
  float z;

  float nx;
  float ny;
  float nz;

  float ambient_r;
  float ambient_g;
  float ambient_b;

  float diffuse_r;
  float diffuse_g;
  float diffuse_b;

  float emissive_r;
  float emissive_g;
  float emissive_b;
};
} // namespace cg
