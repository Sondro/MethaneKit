/******************************************************************************

Copyright 2019-2020 Evgeny Gorodetskiy

Licensed under the Apache License, Version 2.0 (the "License"),
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

*******************************************************************************

FILE: Methane/Graphics/ImageLoader.cpp
Image Loader creates textures from images loaded via data provider and
by decoding them from popular image formats.

******************************************************************************/

#include <Methane/Graphics/ImageLoader.h>
#include <Methane/Platform/Utils.h>
#include <Methane/Data/Math.hpp>
#include <Methane/Instrumentation.h>
#include <Methane/Checks.hpp>

#include <taskflow/taskflow.hpp>

#ifdef USE_OPEN_IMAGE_IO

#include <OpenImageIO/imagebuf.h>
#include <OpenImageIO/filesystem.h>

#else

#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_STDIO
#include <stb_image.h>

#endif

namespace Methane::Graphics
{

static PixelFormat GetDefaultImageFormat(bool srgb)
{
    return srgb ? PixelFormat::RGBA8Unorm_sRGB : PixelFormat::RGBA8Unorm;
}

ImageLoader::ImageData::ImageData(const Dimensions& in_dimensions, uint32_t in_channels_count, Data::Chunk&& in_pixels)
    : dimensions(in_dimensions)
    , channels_count(in_channels_count)
    , pixels(std::move(in_pixels))
{
    META_FUNCTION_TASK();
}

ImageLoader::ImageData::ImageData(ImageData&& other)
    : dimensions(std::move(other.dimensions))
    , channels_count(other.channels_count)
    , pixels(std::move(other.pixels))
{
    META_FUNCTION_TASK();
}

ImageLoader::ImageData::~ImageData()
{
    META_FUNCTION_TASK();

#ifndef USE_OPEN_IMAGE_IO
    if (pixels.data.empty() && pixels.p_data)
    {
        // We assume that image data was loaded with STB load call and was not copied to container, so it must be freed
        stbi_image_free(const_cast<Data::RawPtr>(pixels.p_data));
    }
#endif
}

ImageLoader::ImageLoader(Data::Provider& data_provider)
    : m_data_provider(data_provider)
{
    META_FUNCTION_TASK();
}

ImageLoader::ImageData ImageLoader::LoadImage(const std::string& image_path, size_t channels_count, bool create_copy)
{
    META_FUNCTION_TASK();

    Data::Chunk raw_image_data = m_data_provider.GetData(image_path);

#ifdef USE_OPEN_IMAGE_IO

#if 0
    OIIO::Filesystem::IOMemReader image_reader(const_cast<char*>(raw_image_data.p_data), raw_image_data.size);
    OIIO::ImageSpec init_spec;
    init_spec.attribute("oiio:ioproxy", OIIO::TypeDesc::PTR, &image_reader);
    OIIO::ImageBuf image_buf(init_spec);
#else
    const std::string image_file_path = Platform::GetResourceDir() + "/" + image_path;
    OIIO::ImageBuf image_buf(image_file_path.c_str());
#endif

    // Read image format with general information
    const OIIO::ImageSpec& image_spec = image_buf.spec();
    META_CHECK_ARG_DESCR(image_path, !image_spec.undefined(), "failed to load image specification");

    const bool read_success = image_buf.read();
    META_CHECK_ARG_DESCR(image_path, read_success, "failed to read image data from file, error: {}", image_buf.geterror());

    // Convert image pixels data to the target texture format RGBA8 Unorm
    OIIO::ROI image_roi = OIIO::get_roi(image_spec);
    Data::Bytes texture_data(channels_count * image_roi.npixels(), 255);
    const OIIO::TypeDesc texture_format(OIIO::TypeDesc::BASETYPE::UCHAR);
    const decode_success = image_buf.get_pixels(image_roi, texture_format, texture_data.data(), channels_count * sizeof(texture_data[0]));
    META_CHECK_ARG_DESCR(image_path, decode_success, "failed to decode image pixels, error: {}", image_buf.geterror());

    return ImageData(Dimensions(static_cast<uint32_t>(image_spec.width), static_cast<uint32_t>(image_spec.height)),
                                static_cast<uint32_t>(channels_count),
                                Data::Chunk(std::move(texture_data)));

#else
    int image_width = 0, image_height = 0, image_channels_count = 0;
    stbi_uc* p_image_data = stbi_load_from_memory(reinterpret_cast<stbi_uc const*>(raw_image_data.p_data),
                                                  static_cast<int>(raw_image_data.size),
                                                  &image_width, &image_height, &image_channels_count,
                                                  static_cast<int>(channels_count));

    META_CHECK_ARG_NOT_NULL_DESCR(p_image_data, "failed to load image data from memory");
    META_CHECK_ARG_GREATER_OR_EQUAL_DESCR(image_width, 1, "invalid image width");
    META_CHECK_ARG_GREATER_OR_EQUAL_DESCR(image_height, 1, "invalid image height");
    META_CHECK_ARG_GREATER_OR_EQUAL_DESCR(image_channels_count, 1, "invalid image channels count");

    const Dimensions image_dimensions(static_cast<uint32_t>(image_width), static_cast<uint32_t>(image_height));
    const Data::Size image_data_size = static_cast<Data::Size>(image_width * image_height * channels_count * sizeof(stbi_uc));

    if (create_copy)
    {
        Data::RawPtr p_image_raw_data = reinterpret_cast<Data::RawPtr>(p_image_data);
        Data::Bytes image_data_copy(p_image_raw_data, p_image_raw_data + image_data_size);
        ImageData image_data(image_dimensions, static_cast<uint32_t>(image_channels_count), Data::Chunk(std::move(image_data_copy)));
        stbi_image_free(p_image_data);
        return image_data;
    }
    else
    {
        return ImageData(image_dimensions, static_cast<uint32_t>(image_channels_count),
                         Data::Chunk(reinterpret_cast<Data::ConstRawPtr>(p_image_data), image_data_size));
    }

#endif
}

Ptr<Texture> ImageLoader::LoadImageToTexture2D(Context& context, const std::string& image_path, Options::Mask options)
{
    META_FUNCTION_TASK();

    const ImageData   image_data   = LoadImage(image_path, 4, false);
    const PixelFormat image_format = GetDefaultImageFormat(options & Options::SrgbColorSpace);
    Ptr<Texture> texture_ptr = Texture::CreateImage(context, image_data.dimensions, 1, image_format, options & Options::Mipmapped);
    texture_ptr->SetData({ { image_data.pixels.p_data, image_data.pixels.size } });

    return texture_ptr;
}

Ptr<Texture> ImageLoader::LoadImagesToTextureCube(Context& context, const CubeFaceResources& image_paths, Options::Mask options)
{
    META_FUNCTION_TASK();

    const uint32_t desired_channels_count = 4;

    // Load face image data in parallel
    TracyLockable(std::mutex, data_mutex);
    std::vector<std::pair<Data::Index, ImageData>> face_images_data;
    face_images_data.reserve(image_paths.size());

    tf::Taskflow load_task_flow;
    load_task_flow.for_each_index_guided(0, static_cast<int>(image_paths.size()), 1,
        [&](const int face_index)
        {
            META_FUNCTION_TASK();
            // NOTE:
            //  we create a copy of the loaded image data (via 3-rd argument of LoadImage)
            //  to resolve a problem of STB image loader which requires an image data to be freed before next image is loaded
            const std::string& face_image_path = image_paths[face_index];
            ImageLoader::ImageData image_data = LoadImage(face_image_path, desired_channels_count, true);

            std::lock_guard<LockableBase(std::mutex)> data_lock(data_mutex);
            face_images_data.emplace_back(face_index, std::move(image_data));
        }
    );
    context.GetParallelExecutor().run(load_task_flow).get();

    // Verify cube textures

    META_CHECK_ARG_EQUAL_DESCR(face_images_data.size(), image_paths.size(), "some faces of cube texture have failed to load");
    const Dimensions face_dimensions     = face_images_data.front().second.dimensions;
    const uint32_t   face_channels_count = face_images_data.front().second.channels_count;
    META_CHECK_ARG_EQUAL_DESCR(face_dimensions.width, face_dimensions.height, "all images of cube texture faces must have equal width and height");

    Resource::SubResources face_resources;
    face_resources.reserve(face_images_data.size());
    for(const std::pair<Data::Index, ImageData>& face_image_data : face_images_data)
    {
        META_CHECK_ARG_EQUAL_DESCR(face_dimensions, face_image_data.second.dimensions, "all face image of cube texture must have equal dimensions");
        META_CHECK_ARG_EQUAL_DESCR(face_channels_count, face_image_data.second.channels_count, "all face image of cube texture must have equal channels count");
        face_resources.emplace_back(face_image_data.second.pixels.p_data, face_image_data.second.pixels.size, Resource::SubResource::Index(face_image_data.first));
    }

    // Load face images to cube texture

    const PixelFormat image_format = GetDefaultImageFormat(options & Options::SrgbColorSpace);
    Ptr<Texture> texture_ptr = Texture::CreateCube(context, face_dimensions.width, 1, image_format, options & Options::Mipmapped);
    texture_ptr->SetData(face_resources);

    return texture_ptr;
}

} // namespace Methane::Graphics
