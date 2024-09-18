/*
Emile Belanger
GPL3
*/
#include "renderer/tr_local.h"
#include "renderer/VertexCache.h"

#define FRAMEBUFFER_POOL_SIZE 5

static GLuint m_framebuffer[FRAMEBUFFER_POOL_SIZE];
static GLuint m_depthbuffer[FRAMEBUFFER_POOL_SIZE];

static int m_framebuffer_width, m_framebuffer_height;
static GLuint m_framebuffer_texture[FRAMEBUFFER_POOL_SIZE];

static GLint drawFboId = 0;
static int currentFramebufferIndex = 0;


void R_InitFrameBuffer()
{
	m_framebuffer_width = glConfig.vidWidth;
	m_framebuffer_height = glConfig.vidHeight;

	for (int i = 0; i < FRAMEBUFFER_POOL_SIZE; ++i) {
        // Create texture
        glGenTextures(1, &m_framebuffer_texture[i]);
        glBindTexture(GL_TEXTURE_2D, m_framebuffer_texture[i]);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_framebuffer_width, m_framebuffer_height, 0,
                     GL_RGBA, GL_UNSIGNED_BYTE, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        // Create framebuffer
        glGenFramebuffers(1, &m_framebuffer[i]);

        // Create renderbuffer
        glGenRenderbuffers(1, &m_depthbuffer[i]);
        glBindRenderbuffer(GL_RENDERBUFFER, m_depthbuffer[i]);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8_OES, m_framebuffer_width,
                              m_framebuffer_height);
    }
}

void R_FrameBufferStart()
{
	if (currentFramebufferIndex == 0) {
        glGetIntegerv(GL_FRAMEBUFFER_BINDING, &drawFboId);
    }

	// Render to framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, m_framebuffer[currentFramebufferIndex]);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_framebuffer_texture[currentFramebufferIndex], 0);

    // Attach combined depth+stencil
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_depthbuffer[currentFramebufferIndex]);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_depthbuffer[currentFramebufferIndex]);

    GLenum result = glCheckFramebufferStatus(GL_FRAMEBUFFER);
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
        glBindFramebuffer(GL_FRAMEBUFFER, m_framebuffer[currentFramebufferIndex - 1]);
    }
}


