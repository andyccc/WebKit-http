//
// Copyright 2016 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// TextureVk.h:
//    Defines the class interface for TextureVk, implementing TextureImpl.
//

#ifndef LIBANGLE_RENDERER_VULKAN_TEXTUREVK_H_
#define LIBANGLE_RENDERER_VULKAN_TEXTUREVK_H_

#include "libANGLE/renderer/TextureImpl.h"
#include "libANGLE/renderer/vulkan/RenderTargetVk.h"
#include "libANGLE/renderer/vulkan/ResourceVk.h"
#include "libANGLE/renderer/vulkan/SamplerVk.h"
#include "libANGLE/renderer/vulkan/vk_helpers.h"

namespace rx
{

enum class ImageMipLevels
{
    EnabledLevels = 0,
    FullMipChain  = 1,

    InvalidEnum = 2,
};

// vkCmdCopyBufferToImage buffer offset multiple
constexpr VkDeviceSize kBufferOffsetMultiple = 4;

class TextureVk : public TextureImpl, public angle::ObserverInterface
{
  public:
    TextureVk(const gl::TextureState &state, RendererVk *renderer);
    ~TextureVk() override;
    void onDestroy(const gl::Context *context) override;

    angle::Result setImage(const gl::Context *context,
                           const gl::ImageIndex &index,
                           GLenum internalFormat,
                           const gl::Extents &size,
                           GLenum format,
                           GLenum type,
                           const gl::PixelUnpackState &unpack,
                           gl::Buffer *unpackBuffer,
                           const uint8_t *pixels) override;
    angle::Result setSubImage(const gl::Context *context,
                              const gl::ImageIndex &index,
                              const gl::Box &area,
                              GLenum format,
                              GLenum type,
                              const gl::PixelUnpackState &unpack,
                              gl::Buffer *unpackBuffer,
                              const uint8_t *pixels) override;

    angle::Result setCompressedImage(const gl::Context *context,
                                     const gl::ImageIndex &index,
                                     GLenum internalFormat,
                                     const gl::Extents &size,
                                     const gl::PixelUnpackState &unpack,
                                     size_t imageSize,
                                     const uint8_t *pixels) override;
    angle::Result setCompressedSubImage(const gl::Context *context,
                                        const gl::ImageIndex &index,
                                        const gl::Box &area,
                                        GLenum format,
                                        const gl::PixelUnpackState &unpack,
                                        size_t imageSize,
                                        const uint8_t *pixels) override;

    angle::Result copyImage(const gl::Context *context,
                            const gl::ImageIndex &index,
                            const gl::Rectangle &sourceArea,
                            GLenum internalFormat,
                            gl::Framebuffer *source) override;
    angle::Result copySubImage(const gl::Context *context,
                               const gl::ImageIndex &index,
                               const gl::Offset &destOffset,
                               const gl::Rectangle &sourceArea,
                               gl::Framebuffer *source) override;

    angle::Result copyTexture(const gl::Context *context,
                              const gl::ImageIndex &index,
                              GLenum internalFormat,
                              GLenum type,
                              size_t sourceLevelGL,
                              bool unpackFlipY,
                              bool unpackPremultiplyAlpha,
                              bool unpackUnmultiplyAlpha,
                              const gl::Texture *source) override;
    angle::Result copySubTexture(const gl::Context *context,
                                 const gl::ImageIndex &index,
                                 const gl::Offset &destOffset,
                                 size_t sourceLevelGL,
                                 const gl::Box &sourceBox,
                                 bool unpackFlipY,
                                 bool unpackPremultiplyAlpha,
                                 bool unpackUnmultiplyAlpha,
                                 const gl::Texture *source) override;

    angle::Result copyCompressedTexture(const gl::Context *context,
                                        const gl::Texture *source) override;

    angle::Result setStorage(const gl::Context *context,
                             gl::TextureType type,
                             size_t levels,
                             GLenum internalFormat,
                             const gl::Extents &size) override;

    angle::Result setStorageExternalMemory(const gl::Context *context,
                                           gl::TextureType type,
                                           size_t levels,
                                           GLenum internalFormat,
                                           const gl::Extents &size,
                                           gl::MemoryObject *memoryObject,
                                           GLuint64 offset) override;

    angle::Result setEGLImageTarget(const gl::Context *context,
                                    gl::TextureType type,
                                    egl::Image *image) override;

    angle::Result setImageExternal(const gl::Context *context,
                                   gl::TextureType type,
                                   egl::Stream *stream,
                                   const egl::Stream::GLTextureDescription &desc) override;

    angle::Result generateMipmap(const gl::Context *context) override;

    angle::Result setBaseLevel(const gl::Context *context, GLuint baseLevel) override;

    angle::Result bindTexImage(const gl::Context *context, egl::Surface *surface) override;
    angle::Result releaseTexImage(const gl::Context *context) override;

    angle::Result getAttachmentRenderTarget(const gl::Context *context,
                                            GLenum binding,
                                            const gl::ImageIndex &imageIndex,
                                            GLsizei samples,
                                            FramebufferAttachmentRenderTarget **rtOut) override;

    angle::Result syncState(const gl::Context *context,
                            const gl::Texture::DirtyBits &dirtyBits,
                            gl::TextureCommand source) override;

    angle::Result setStorageMultisample(const gl::Context *context,
                                        gl::TextureType type,
                                        GLsizei samples,
                                        GLint internalformat,
                                        const gl::Extents &size,
                                        bool fixedSampleLocations) override;

    angle::Result initializeContents(const gl::Context *context,
                                     const gl::ImageIndex &imageIndex) override;

    const vk::ImageHelper &getImage() const
    {
        ASSERT(mImage && mImage->valid());
        return *mImage;
    }

    vk::ImageHelper &getImage()
    {
        ASSERT(mImage && mImage->valid());
        return *mImage;
    }

    void retainImageViews(vk::ResourceUseList *resourceUseList)
    {
        mImageViews.retain(resourceUseList);
    }

    void releaseOwnershipOfImage(const gl::Context *context);

    const vk::ImageView &getReadImageViewAndRecordUse(ContextVk *contextVk) const;
    // A special view for cube maps as a 2D array, used with shaders that do texelFetch() and for
    // seamful cube map emulation.
    const vk::ImageView &getFetchImageViewAndRecordUse(ContextVk *contextVk) const;
    // A special view used for texture copies that shouldn't perform swizzle.
    const vk::ImageView &getCopyImageViewAndRecordUse(ContextVk *contextVk) const;
    angle::Result getStorageImageView(ContextVk *contextVk,
                                      const gl::ImageUnit &binding,
                                      const vk::ImageView **imageViewOut);

    const vk::Sampler &getSampler() const
    {
        ASSERT(mSampler.valid());
        return mSampler.get();
    }

    // Normally, initialize the image with enabled mipmap level counts.
    angle::Result ensureImageInitialized(ContextVk *contextVk, ImageMipLevels mipLevels);

    TextureSerial getSerial() const { return mSerial; }

    void overrideStagingBufferSizeForTesting(size_t initialSizeForTesting)
    {
        mStagingBufferInitialSize = initialSizeForTesting;
    }

    GLenum getColorReadFormat(const gl::Context *context) override;
    GLenum getColorReadType(const gl::Context *context) override;

    angle::Result getTexImage(const gl::Context *context,
                              const gl::PixelPackState &packState,
                              gl::Buffer *packBuffer,
                              gl::TextureTarget target,
                              GLint level,
                              GLenum format,
                              GLenum type,
                              void *pixels) override;

    ANGLE_INLINE bool isBoundAsImageTexture(gl::ContextID contextID) const
    {
        return mState.isBoundAsImageTexture(contextID);
    }

  private:
    // Transform an image index from the frontend into one that can be used on the backing
    // ImageHelper, taking into account mipmap or cube face offsets
    gl::ImageIndex getNativeImageIndex(const gl::ImageIndex &inputImageIndex) const;
    uint32_t getNativeImageLevel(uint32_t frontendLevel) const;
    uint32_t getNativeImageLayer(uint32_t frontendLayer) const;

    void releaseAndDeleteImage(ContextVk *contextVk);
    angle::Result ensureImageAllocated(ContextVk *contextVk, const vk::Format &format);
    void setImageHelper(ContextVk *contextVk,
                        vk::ImageHelper *imageHelper,
                        gl::TextureType imageType,
                        const vk::Format &format,
                        uint32_t imageLevelOffset,
                        uint32_t imageLayerOffset,
                        uint32_t imageBaseLevel,
                        bool selfOwned);
    void updateImageHelper(ContextVk *contextVk, size_t imageCopyBufferAlignment);

    // Redefine a mip level of the texture.  If the new size and format don't match the allocated
    // image, the image may be released.  When redefining a mip of a multi-level image, updates are
    // forced to be staged, as another mip of the image may be bound to a framebuffer.  For example,
    // assume texture has two mips, and framebuffer is bound to mip 0.  Redefining mip 1 to an
    // incompatible size shouldn't affect the framebuffer, especially if the redefinition comes from
    // something like glCopyTexSubImage2D() (which simultaneously is reading from said framebuffer,
    // i.e. mip 0 of the texture).
    angle::Result redefineLevel(const gl::Context *context,
                                const gl::ImageIndex &index,
                                const vk::Format &format,
                                const gl::Extents &size);

    angle::Result setImageImpl(const gl::Context *context,
                               const gl::ImageIndex &index,
                               const gl::InternalFormat &formatInfo,
                               const gl::Extents &size,
                               GLenum type,
                               const gl::PixelUnpackState &unpack,
                               gl::Buffer *unpackBuffer,
                               const uint8_t *pixels);
    angle::Result setSubImageImpl(const gl::Context *context,
                                  const gl::ImageIndex &index,
                                  const gl::Box &area,
                                  const gl::InternalFormat &formatInfo,
                                  GLenum type,
                                  const gl::PixelUnpackState &unpack,
                                  gl::Buffer *unpackBuffer,
                                  const uint8_t *pixels,
                                  const vk::Format &vkFormat);

    angle::Result copyImageDataToBufferAndGetData(ContextVk *contextVk,
                                                  size_t sourceLevelGL,
                                                  uint32_t layerCount,
                                                  const gl::Box &sourceArea,
                                                  uint8_t **outDataPtr);

    angle::Result copyBufferDataToImage(ContextVk *contextVk,
                                        vk::BufferHelper *srcBuffer,
                                        const gl::ImageIndex index,
                                        uint32_t rowLength,
                                        uint32_t imageHeight,
                                        const gl::Box &sourceArea,
                                        size_t offset);

    // Called from syncState to prepare the image for mipmap generation.
    void prepareForGenerateMipmap(ContextVk *contextVk);

    // Generate mipmaps from level 0 into the rest of the mips.  This requires the image to have
    // STORAGE usage.
    angle::Result generateMipmapsWithCompute(ContextVk *contextVk);

    angle::Result generateMipmapsWithCPU(const gl::Context *context);

    angle::Result generateMipmapLevelsWithCPU(ContextVk *contextVk,
                                              const angle::Format &sourceFormat,
                                              GLuint layer,
                                              GLuint firstMipLevel,
                                              GLuint maxMipLevel,
                                              const size_t sourceWidth,
                                              const size_t sourceHeight,
                                              const size_t sourceDepth,
                                              const size_t sourceRowPitch,
                                              const size_t sourceDepthPitch,
                                              uint8_t *sourceData);

    angle::Result copySubImageImpl(const gl::Context *context,
                                   const gl::ImageIndex &index,
                                   const gl::Offset &destOffset,
                                   const gl::Rectangle &sourceArea,
                                   const gl::InternalFormat &internalFormat,
                                   gl::Framebuffer *source);

    angle::Result copySubTextureImpl(ContextVk *contextVk,
                                     const gl::ImageIndex &index,
                                     const gl::Offset &destOffset,
                                     const gl::InternalFormat &destFormat,
                                     size_t sourceLevelGL,
                                     const gl::Box &sourceBox,
                                     bool unpackFlipY,
                                     bool unpackPremultiplyAlpha,
                                     bool unpackUnmultiplyAlpha,
                                     TextureVk *source);

    angle::Result copySubImageImplWithTransfer(ContextVk *contextVk,
                                               const gl::ImageIndex &index,
                                               const gl::Offset &destOffset,
                                               const vk::Format &destFormat,
                                               size_t sourceLevelGL,
                                               size_t sourceLayer,
                                               const gl::Box &sourceBox,
                                               vk::ImageHelper *srcImage);

    angle::Result copySubImageImplWithDraw(ContextVk *contextVk,
                                           const gl::ImageIndex &index,
                                           const gl::Offset &destOffset,
                                           const vk::Format &destFormat,
                                           size_t sourceLevelGL,
                                           const gl::Box &sourceBox,
                                           bool isSrcFlipY,
                                           bool unpackFlipY,
                                           bool unpackPremultiplyAlpha,
                                           bool unpackUnmultiplyAlpha,
                                           vk::ImageHelper *srcImage,
                                           const vk::ImageView *srcView,
                                           SurfaceRotation srcFramebufferRotation);

    angle::Result initImage(ContextVk *contextVk,
                            const vk::Format &format,
                            const bool sized,
                            const gl::Extents &extents,
                            const uint32_t levelCount);
    void releaseImage(ContextVk *contextVk);
    void releaseStagingBuffer(ContextVk *contextVk);
    uint32_t getMipLevelCount(ImageMipLevels mipLevels) const;
    uint32_t getMaxLevelCount() const;
    // Used when the image is being redefined (for example to add mips or change base level) to copy
    // each subresource of the image and stage it for another subresource.  When all subresources
    // are taken care of, the image is recreated.
    angle::Result copyAndStageImageSubresource(ContextVk *contextVk,
                                               bool ignoreLayerCount,
                                               uint32_t currentLayer,
                                               uint32_t srcLevelVk,
                                               uint32_t dstLevelGL);
    angle::Result initImageViews(ContextVk *contextVk,
                                 const vk::Format &format,
                                 const bool sized,
                                 uint32_t levelCount,
                                 uint32_t layerCount);
    angle::Result initRenderTargets(ContextVk *contextVk, GLuint layerCount, GLuint levelIndex);
    angle::Result getLevelLayerImageView(ContextVk *contextVk,
                                         size_t level,
                                         size_t layer,
                                         const vk::ImageView **imageViewOut);

    // Flush image's staged updates for all levels and layers.
    angle::Result flushImageStagedUpdates(ContextVk *contextVk);

    const gl::InternalFormat &getImplementationSizedFormat(const gl::Context *context) const;
    const vk::Format &getBaseLevelFormat(RendererVk *renderer) const;
    // Queues a flush of any modified image attributes. The image will be reallocated with its new
    // attributes at the next opportunity.
    angle::Result respecifyImageAttributes(ContextVk *contextVk);
    angle::Result respecifyImageAttributesAndLevels(ContextVk *contextVk,
                                                    GLuint previousBaseLevel,
                                                    GLuint baseLevel,
                                                    GLuint maxLevel);

    // Update base and max levels, and re-create image if needed.
    angle::Result updateBaseMaxLevels(ContextVk *contextVk, GLuint baseLevel, GLuint maxLevel);

    bool isFastUnpackPossible(const vk::Format &vkFormat, size_t offset) const;

    bool shouldUpdateBeStaged(uint32_t textureLevelIndexGL) const;

    // We monitor the staging buffer and set dirty bits if the staging buffer changes. Note that we
    // support changes in the staging buffer even outside the TextureVk class.
    void onSubjectStateChange(angle::SubjectIndex index, angle::SubjectMessage message) override;

    ANGLE_INLINE VkImageTiling getTilingMode()
    {
        return (mImage->valid()) ? mImage->getTilingMode() : VK_IMAGE_TILING_OPTIMAL;
    }

    bool mOwnsImage;
    bool mRequiresSRGBViews;

    gl::TextureType mImageNativeType;

    // The layer offset to apply when converting from a frontend texture layer to a texture layer in
    // mImage. Used when this texture sources a cube map face or 3D texture layer from an EGL image.
    uint32_t mImageLayerOffset;

    // The level offset to apply when converting from a frontend texture level to texture level in
    // mImage.
    uint32_t mImageLevelOffset;

    // |mImage| wraps a VkImage and VkDeviceMemory that represents the gl::Texture. |mOwnsImage|
    // indicates that |TextureVk| owns the image. Otherwise it is a weak pointer shared with another
    // class.
    vk::ImageHelper *mImage;

    // |mImageViews| contains all the current views for the Texture. The views are always owned by
    // the Texture and are not shared like |mImage|. They also have different lifetimes and can be
    // reallocated independently of |mImage| on state changes.
    vk::ImageViewHelper mImageViews;

    // |mSampler| contains the relevant Vulkan sampler states representing the OpenGL Texture
    // sampling states for the Texture.
    vk::BindingPointer<vk::Sampler> mSampler;

    // Render targets stored as vector of vectors
    // Level is first dimension, layer is second
    std::vector<RenderTargetVector> mRenderTargets;

    // The unique object id is used for cache indexing.
    TextureSerial mSerial;

    // Overridden in some tests.
    size_t mStagingBufferInitialSize;

    // The created vkImage usage flag.
    VkImageUsageFlags mImageUsageFlags;

    // Additional image create flags
    VkImageCreateFlags mImageCreateFlags;

    // If an image level is incompatibly redefined, the image lives through the call that did this
    // (i.e. set and copy levels), because the image may be used by the framebuffer in the very same
    // call.  As a result, updates to this redefined level are staged (in both the call that
    // redefines it, and any future calls such as subimage updates).  This bitset flags redefined
    // levels so that their updates will be force-staged until image is recreated.
    //
    // In common cases with mipmapped textures, the base/max level would need adjusting as the
    // texture is no longer mip-complete.  However, if every level is redefined such that at the end
    // the image becomes mip-complete again, no reinitialization of the image is done.  This bitset
    // is additionally used to ensure the image is recreated in the next syncState, if not already.
    gl::TexLevelMask mRedefinedLevels;

    angle::ObserverBinding mImageObserverBinding;
};

}  // namespace rx

#endif  // LIBANGLE_RENDERER_VULKAN_TEXTUREVK_H_
