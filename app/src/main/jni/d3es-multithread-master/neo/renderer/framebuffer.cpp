/*
Emile Belanger
GPL3
*/
#include "renderer/tr_local.h"
#include "renderer/VertexCache.h"

#include <GLES3/gl3.h>
#include <EGL/egl.h>

#define FRAMEBUFFER_POOL_SIZE 5

// Every shader is compiled with layout(num_views=2), so the offscreen buffers used for the
// HUD and PDA are two-layer multiview framebuffers like the eye buffers. Mesa (Steam Frame)
// rejects multiview draws into a single-layer framebuffer, which left the HUD and PDA black.
// Copies and reads can't use a multiview framebuffer, so each buffer also has a read
// framebuffer that exposes layer 0 only.
static GLuint m_framebuffer[FRAMEBUFFER_POOL_SIZE];
static GLuint m_readFramebuffer[FRAMEBUFFER_POOL_SIZE];
static GLuint m_depthbuffer[FRAMEBUFFER_POOL_SIZE];

static int m_framebuffer_width, m_framebuffer_height;
static GLuint m_framebuffer_texture[FRAMEBUFFER_POOL_SIZE];

static GLint drawFboId = 0;
static int currentFramebufferIndex = 0;

static PFNGLFRAMEBUFFERTEXTUREMULTIVIEWOVRPROC glFramebufferTextureMultiviewOVR_ = NULL;


void R_InitFrameBuffer()
{
	m_framebuffer_width = glConfig.vidWidth;
	m_framebuffer_height = glConfig.vidHeight;

	glFramebufferTextureMultiviewOVR_ = (PFNGLFRAMEBUFFERTEXTUREMULTIVIEWOVRPROC)eglGetProcAddress("glFramebufferTextureMultiviewOVR");

	for (int i = 0; i < FRAMEBUFFER_POOL_SIZE; ++i) {
        glGenFramebuffers(1, &m_framebuffer[i]);
        glGenTextures(1, &m_framebuffer_texture[i]);

        if (glFramebufferTextureMultiviewOVR_) {
            // Two-layer color and depth/stencil textures, one layer per view
            glBindTexture(GL_TEXTURE_2D_ARRAY, m_framebuffer_texture[i]);
            glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, GL_RGBA8, m_framebuffer_width, m_framebuffer_height, 2);
            glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            glGenTextures(1, &m_depthbuffer[i]);
            glBindTexture(GL_TEXTURE_2D_ARRAY, m_depthbuffer[i]);
            glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, GL_DEPTH24_STENCIL8, m_framebuffer_width, m_framebuffer_height, 2);
            glBindTexture(GL_TEXTURE_2D_ARRAY, 0);

            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_framebuffer[i]);
            glFramebufferTextureMultiviewOVR_(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, m_framebuffer_texture[i], 0, 0, 2);
            glFramebufferTextureMultiviewOVR_(GL_DRAW_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, m_depthbuffer[i], 0, 0, 2);

            glGenFramebuffers(1, &m_readFramebuffer[i]);
            glBindFramebuffer(GL_READ_FRAMEBUFFER, m_readFramebuffer[i]);
            glFramebufferTextureLayer(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, m_framebuffer_texture[i], 0, 0);
        } else {
            glBindTexture(GL_TEXTURE_2D, m_framebuffer_texture[i]);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_framebuffer_width, m_framebuffer_height, 0,
                         GL_RGBA, GL_UNSIGNED_BYTE, NULL);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            glGenRenderbuffers(1, &m_depthbuffer[i]);
            glBindRenderbuffer(GL_RENDERBUFFER, m_depthbuffer[i]);
            glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8_OES, m_framebuffer_width,
                                  m_framebuffer_height);

            glBindFramebuffer(GL_FRAMEBUFFER, m_framebuffer[i]);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_framebuffer_texture[i], 0);
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_depthbuffer[i]);
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_depthbuffer[i]);
            m_readFramebuffer[i] = m_framebuffer[i];
        }
    }

	glBindRenderbuffer(GL_RENDERBUFFER, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

static void R_BindPoolFramebuffer(int index)
{
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_framebuffer[index]);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, m_readFramebuffer[index]);
}

void R_FrameBufferStart()
{
	if (currentFramebufferIndex == 0) {
        glGetIntegerv(GL_FRAMEBUFFER_BINDING, &drawFboId);
    }

	// Render to framebuffer
    R_BindPoolFramebuffer(currentFramebufferIndex);

    GLenum result = glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER);
	if(result != GL_FRAMEBUFFER_COMPLETE)
	{
	    common->Error( "Error binding Framebuffer: %i\n", result );
	}

    glClearColor( 0.0f, 0.0f,  0.0f, 1.0f );
    qglClear( GL_COLOR_BUFFER_BIT );

    //Increment index in case this gets called again
    currentFramebufferIndex++;
}


void R_FrameBufferEnd()
{
    currentFramebufferIndex--;

    if (currentFramebufferIndex == 0) {
        glBindFramebuffer(GL_FRAMEBUFFER, drawFboId);
    } else {
        R_BindPoolFramebuffer(currentFramebufferIndex - 1);
    }
}


