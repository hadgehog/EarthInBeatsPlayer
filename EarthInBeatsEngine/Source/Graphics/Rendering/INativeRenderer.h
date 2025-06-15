#pragma once

#include <string>

class INativeRenderer
{
public:
	INativeRenderer() = default;
	virtual ~INativeRenderer() = default;

	virtual void LoadModel(const std::string& path) = 0;
	virtual void LoadTexture(const std::string& path) = 0;

	virtual void Start() = 0;
	virtual void Stop() = 0;

	virtual void SetRotation(float yaw, float pitch) = 0;
};

