#pragma once

#include "DebugUtils.h"

namespace rdf {

class ThomasTest {
public:
	ThomasTest(const DebugConfig& config);

	void test();
	void testLayout();
	void testXml();

private:
	DebugConfig mConfig;
};

}