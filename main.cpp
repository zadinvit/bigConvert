#include <iostream>
#include <fstream>
#include <cstring>

#include "big_core_write.hpp"
#include "big_core_read.hpp"

#include "TBIG.h"

int main()
{
	// loading old BIG --------------
	TBIG T;
	std::string filename = "F:/vita_skola/BIGFiles/old/fabric106_uniform_V2.big";
	int status = T.load(filename.c_str(), true); // true = memory, false = disk
	if (status < 0)
		exit(0);
	int NoOfImages = T.get_images();
	int nr = T.get_height();
	int nc = T.get_width();
	printf("height,width: %d %d\n", nr, nc);
	//T.report();
	printf("loading bigBTF...done\n");

	// creating new BIG -------------
	{
		big::BigCoreWrite bigW("fabric106_uniform_V2.big", nr, nc, 3);
		uint64_t n = nr*nc * 3;
		std::shared_ptr<float> data{ new float[n], [](float *p) {delete[] p; } };
		// filling new BIG from old BIG --------------------
		float RGB[3];
		for (int i = 0; i < NoOfImages; i++)
		{
			for (int y = 0; y < nr; y++)
				for (int x = 0; x < nc; x++)
				{
					T.get_pixel(0, y, x, i, RGB);
					for (int isp = 0; isp < 3; isp++)
						data.get()[y * nc * 3 + x * 3 + isp] = RGB[isp];
				}
			bigW.pushEntity(data, big::DataTypes::FLOAT);
		}
		/*bigW.AddAttributeToXmlInt("method", 1, "uniform");
		bigW.AddAttributeToXmlBool("isDark", false, "is this material dark");
		bigW.AddAttributeToXmlDouble("opacity", 21.15, "opacity of this material");
		bigW.AddAttributeToXmlString("data", "uniform", "Data are uniform");*/

	}

	// control reading ------------------------------------
	{
		big::BigCoreRead bigR("fabric106_uniform_V2.big", false);
		printf("height,width: %d %d\n", bigR.getImageHeight(), bigR.getImageWidth());
		printf("entities,images,planes: %d %d %d\n", bigR.getNumberOfEntities(), bigR.getNumberOfImages(), bigR.getNumberOfPlanes());
		/*std::string str = bigR.readXMLString("data");
		printf("XML: method %d opacity %f data %s\n", bigR.readXMLInt("method"), (float)bigR.readXMLDouble("opacity"), str.c_str());*/

	}
}
