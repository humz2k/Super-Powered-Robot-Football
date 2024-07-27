#ifndef _SPRF_SHADOW_MAP_TEXTURE_HPP_
#define _SPRF_SHADOW_MAP_TEXTURE_HPP_

#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

namespace SPRF {

/**
 * @brief Load a shadow map render texture.
 *
 * This function creates a framebuffer object (FBO) with a depth texture
 * attachment but without a color texture attachment, which is suitable for
 * shadow mapping.
 *
 * @param width Width of the shadow map.
 * @param height Height of the shadow map.
 * @return RenderTexture2D The loaded shadow map render texture.
 */
static RenderTexture2D LoadShadowmapRenderTexture(int width, int height) {
    RenderTexture2D target = {0};

    target.id = rlLoadFramebuffer();
    target.texture.width = width;
    target.texture.height = height;

    if (target.id > 0) {
        rlEnableFramebuffer(target.id);

        // Create depth texture
        // We don't need a color texture for the shadowmap
        target.depth.id = rlLoadTextureDepth(width, height, false);
        target.depth.width = width;
        target.depth.height = height;
        target.depth.format = 19; // DEPTH_COMPONENT_24BIT?
        target.depth.mipmaps = 1;

        // Attach depth texture to FBO
        rlFramebufferAttach(target.id, target.depth.id, RL_ATTACHMENT_DEPTH,
                            RL_ATTACHMENT_TEXTURE2D, 0);

        // Check if fbo is complete with attachments (valid)
        if (rlFramebufferComplete(target.id))
            TRACELOG(LOG_INFO,
                     "FBO: [ID %i] Framebuffer object created successfully",
                     target.id);

        rlDisableFramebuffer();
    } else
        TRACELOG(LOG_WARNING, "FBO: Framebuffer object can not be created");

    return target;
}

/**
 * @brief Unload a shadow map render texture.
 *
 * This function unloads the framebuffer object (FBO) and its associated depth
 * texture.
 *
 * @param target The shadow map render texture to unload.
 */
static void UnloadShadowmapRenderTexture(RenderTexture2D target) {
    if (target.id > 0) {
        // NOTE: Depth texture/renderbuffer is automatically
        // queried and deleted before deleting framebuffer
        rlUnloadFramebuffer(target.id);
    }
}

} // namespace SPRF

#endif // _SPRF_SHADOW_MAP_TEXTURE_HPP_