#pragma once

#include "DebugUtils.h"

namespace rdf {

class ThomasTest {
public:
	ThomasTest(const DebugConfig& config);

	void test();
	void testXml();
	void testFeatureCollection();
	void testTraining();
	void testClassification();

private:
	DebugConfig mConfig;
};

}