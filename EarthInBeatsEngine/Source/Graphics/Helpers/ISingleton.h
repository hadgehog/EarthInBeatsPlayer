#pragma once

template <class T>
class ISingleton
{
public:
	static T& GetInstance()
	{
		static T instance;
		return instance;
	}

protected:
	ISingleton() = default;
	virtual ~ISingleton() = default;
	ISingleton(ISingleton&&) = default;
	ISingleton& operator=(ISingleton&&) = default;

	ISingleton(const ISingleton&) = delete;
	ISingleton& operator=(const ISingleton&) = delete;
};