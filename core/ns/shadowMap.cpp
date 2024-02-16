#include "shadowMap.h"
#include "../ew/external/glad.h"
#include <stdio.h>

namespace ns {
	ShadowMap createShadowMap(unsigned int width, unsigned int height) {
		ShadowMap shadowMap;
		shadowMap.width = width;
		shadowMap.height = height;

		glCreateFramebuffers(1, &shadowMap.fbo);
		glBindFramebuffer(GL_FRAMEBUFFER, shadowMap.fbo);
		//Create and bind depth map (depth texture)
		glGenTextures(1, &shadowMap.depthMap);
		glBindTexture(GL_TEXTURE_2D, shadowMap.depthMap);
		//16 bit depth values, 2k resolution
		glTexStorage2D(GL_TEXTURE_2D, 1, GL_DEPTH_COMPONENT16, width, height);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		//Pixels outside of frustum should have max distance (white)
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
		float borderColor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
		glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

		//Attach depth texture as depth buffer
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadowMap.depthMap, 0);
		//Tell open gl we are not drawing colors
		glDrawBuffer(GL_NONE);
		glReadBuffer(GL_NONE);

		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
			printf("ERROR::FRAMEBUFFER:: Shadow Map Framebuffer is not complete!");
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		return shadowMap;
	}
}
