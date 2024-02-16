#pragma once
namespace ns {
	struct ShadowMap {
		unsigned int fbo;
		unsigned int depthMap;
		unsigned int width;
		unsigned int height;
	};
	ShadowMap createShadowMap(unsigned int width, unsigned int height);
}