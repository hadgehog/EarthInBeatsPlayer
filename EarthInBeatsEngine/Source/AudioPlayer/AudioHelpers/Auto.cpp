#include "Auto.h"

#include <mfapi.h>

Auto::Auto()
{
	MFStartup(MF_VERSION);
}

Auto::~Auto()
{
	MFShutdown();
}

Auto& Auto::getInstance()
{
	static Auto instance;
	return instance;
}
