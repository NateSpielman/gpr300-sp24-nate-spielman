#include "framebuffer.h"
#include "../ew/external/glad.h"

namespace ns {
	Framebuffer createFramebuffer(unsigned int width, unsigned int height, int colorFormat) {
		Framebuffer framebuffer;
		framebuffer.width = width;
		framebuffer.height = height;

		glGenFramebuffers(1, &framebuffer.fbo);
		glBindFramebuffer(GL_FRAMEBUFFER, framebuffer.fbo);
		//create and bind color buffer
		glGenTextures(1, &framebuffer.colorBuffer[0]);
		glBindTexture(GL_TEXTURE_2D, framebuffer.colorBuffer[0]);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, framebuffer.colorBuffer[0], 0);
		//renderbuffer object for depth
		glGenRenderbuffers(1, &framebuffer.depthBuffer);
		glBindRenderbuffer(GL_RENDERBUFFER, framebuffer.depthBuffer);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, framebuffer.depthBuffer);

		return framebuffer;
	}
}