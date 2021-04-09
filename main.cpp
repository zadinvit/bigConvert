#include <iostream>
#include <fstream>
#include <cstring>

#include "MIFbtf.hpp"
#include "Image.hpp"

#include "opencv2/core.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/imgproc.hpp"
#include "pugixml.hpp"

using namespace mif;
using namespace mif::directions;
using namespace mif::mipmap;
#include "TBIG.h"


using namespace cv;
pugi::xml_document doc;



void createMipMapWithInfo(shared_ptr<float>& data, const int& rows, const int& cols, shared_ptr<float>& output, Mipmap & mip) {
	Mat img = Mat(rows, cols, CV_32FC3, data.get());
	img.convertTo(img, CV_32FC3, 1 / 255.0);
	Mat mipmap = Mat::zeros(Size(((img.cols) * 3 / 2)+1, img.rows), CV_32FC3);
	img.copyTo(mipmap(cv::Rect(0, 0, img.cols, img.rows)));
	int row = 0;
	int level = 1;
	while (true) {
		Mat img2;
		resize(img, img2, Size(), pow(0.5,level), pow(0.5, level), INTER_AREA);
		mip.isotropic.push_back(Item(img.cols, row, img2.cols, img2.rows));
		img2.copyTo(mipmap(cv::Rect(img.cols, row, img2.cols, img2.rows)));
		//handle rectangle object special cases
		if (img2.rows <= 1 || img2.cols <= 1) {
			if (img2.cols > 1) {
				row += img2.rows;
				resize(img2, img2, Size(), 0.5, 1, INTER_AREA);
				mip.isotropic.push_back(Item(img.cols, row, img2.cols, img2.rows));
				img2.copyTo(mipmap(cv::Rect(img.cols, row, img2.cols, img2.rows)));
			} else if (img2.rows > 1) {
				row += img2.rows;
				resize(img2, img2, Size(), 1, 0.5, INTER_AREA);
				mip.isotropic.push_back(Item(img.cols, row, img2.cols, img2.rows));
				img2.copyTo(mipmap(cv::Rect(img.cols, row, img2.cols, img2.rows)));
			}
			break;
		}
		row += img2.rows;
		level++;

	}
	for (int i = 0; i < mipmap.rows; i++)
	{
		const float* Mi = mipmap.ptr<float>(i);
		for (int j = 0; j < mipmap.cols; j++) {
			float test = mipmap.at<Vec3f>(i, j)[0];
			output.get()[i * mipmap.cols * 3 + j * 3] = mipmap.at<Vec3f>(i, j)[0];
			output.get()[i * mipmap.cols *3 + j * 3 + 1] = mipmap.at<Vec3f>(i, j)[1];
			output.get()[i * mipmap.cols* 3 + j * 3 + 2] = mipmap.at<Vec3f>(i, j)[2];
		}
	}
}
//create mip map image using OpenCV resize with INTER_AREA algorithm which is clever and it gives moire-free results
void createMipMap(shared_ptr<float>& data, const int& rows, const int& cols, shared_ptr<float>& output) {
	Mat img = Mat(rows, cols, CV_32FC3, data.get());
	img.convertTo(img, CV_32FC3, 1 / 255.0);
	Mat mipmap = Mat::zeros(Size(((img.cols) * 3 / 2) + 1, img.rows), CV_32FC3);
	img.copyTo(mipmap(cv::Rect(0, 0, img.cols, img.rows)));
	int row = 0;
	int level = 1;
	while (true) {
		Mat img2;
		resize(img, img2, Size(), pow(0.5, level), pow(0.5, level), INTER_AREA);
		img2.copyTo(mipmap(cv::Rect(img.cols, row, img2.cols, img2.rows)));
		//handle rectangle object special cases
		if (img2.rows <= 1 || img2.cols <= 1) {
			if (img2.cols > 1) {
				row += img2.rows;
				resize(img2, img2, Size(), 0.5, 1, INTER_AREA);
				img2.copyTo(mipmap(cv::Rect(img.cols, row, img2.cols, img2.rows)));
			} else if (img2.rows > 1) {
				row += img2.rows;
				resize(img2, img2, Size(), 1, 0.5, INTER_AREA);
				img2.copyTo(mipmap(cv::Rect(img.cols, row, img2.cols, img2.rows)));
			}
			break;
		}
		row += img2.rows;
		level++;
	}
	
	for (int i = 0; i < mipmap.rows; i++)
	{
		const float* Mi = mipmap.ptr<float>(i);
		for (int j = 0; j < mipmap.cols; j++) {
			float test = mipmap.at<Vec3f>(i, j)[0];
			output.get()[i * mipmap.cols * 3 + j * 3] = mipmap.at<Vec3f>(i, j)[0];
			output.get()[i * mipmap.cols * 3 + j * 3 + 1] = mipmap.at<Vec3f>(i, j)[1];
			output.get()[i * mipmap.cols * 3 + j * 3 + 2] = mipmap.at<Vec3f>(i, j)[2];
		}
	}
}


void checkFilename(std::string filename) {
	FILE* fp = fopen(filename.c_str(), "r");
	if (fp) {
		fclose(fp);
		remove(filename.c_str());
	}
}


void controlReading(std::string filename) {
	MIFbtf mif(filename,true);
	std::cout<<"BTF name" <<mif.getBtfName("btf0")<<std::endl;
	printf("height,width: %d %d\n", mif.getImageHeight(0), mif.getImageWidth(0));
	printf("images,planes: %d %d\n", mif.getNumberOfImages(), mif.getImageNumberOfPlanes(0));
	string xmlFilename = filename.substr(0, filename.size() - 3) + "xml";
	mif.saveXMLtoSeparateFile(xmlFilename);
	std::cout << "control XML save to: " << xmlFilename<< std::endl;
}


int main()
{
	std::ifstream configfile("config.txt");
	if (configfile.fail()) {
		std::cout << "Config file missing" << std::endl;
		return -1;
	}

	auto declarationNode = doc.append_child(pugi::node_declaration);
	declarationNode.append_attribute("version") = "1.0";
	declarationNode.append_attribute("encoding") = "ISO-8859-1";
	declarationNode.append_attribute("standalone") = "yes";
	auto root = doc.append_child("BigFile");

	std::string line;
	std::getline(configfile, line);
	std::istringstream iss(line);
	std::string text;
	iss = std::istringstream(line);
	bool ubo;
	if (!(iss >> text >> ubo)) {
		std::cout << "UBO parse error" << std::endl;
		return -1;
	}
	auto type = root.append_child("type");
	if (ubo) {
		type.append_child(pugi::node_pcdata).set_value("UBO");
	} else {
		type.append_child(pugi::node_pcdata).set_value("uniform");
	}
	std::getline(configfile, line);
	bool mipmap;
	iss = std::istringstream(line);
	if (!(iss >> text >> mipmap)) {
		std::cout << "mipmap parse error" << std::endl;
		return -1;
	}
	
	while (std::getline(configfile, line)) {
		iss = std::istringstream(line);
		std::string filename;
		if (!(iss >> filename)) {
			std::cout << "Filename parse error" << std::endl;
			return -1;
		}


		if (!ubo) {
			// loading old BIG --------------
			TBIG T;
			//printf("start\n");
			std::string lFilename = filename + ".big";
			int status = T.load(lFilename.c_str(), true, 10000000000000); // true = memory, false = disk
			if (status < 0) {
				std::cout << "Open BIG file failed. File name: " << filename<< std::endl;
				return -5;
			}
			int NoOfImages = T.get_images();
			int nr = T.get_height();
			int nc = T.get_width();
			printf("height,width: %d %d\n", nr, nc);
			//T.report();
			printf("loading bigBTF...done\n");
			filename += ".mif";
			// creating new BIG -------------
			{
				uint64_t cols = nc;
				if (mipmap) {
					cols = (nc * 3 / 2)+1;
				}
				checkFilename(filename);
				MIFbtf mif(filename);
				mif.addBtfElement("btf0", "uniform");
				Directions directions;
				directions.type = Directions::Type::list;
				directions.name = "uniform";
				directions.attributes = { { "stepTheta", "15" },{"stepPhi","30" } };
				Mipmap mip;
				mip.type = Mipmap::Type::isotropic;

				uint64_t n = nr * nc * 3;
				std::shared_ptr<float> data{ new float[n], [](float* p) {delete[] p; } };
				// filling new BIG from old BIG --------------------
				float RGB[3];
				for (int i = 0; i < NoOfImages; i++)
				{
					directions.listRecords.push_back({ Direction(Type::source, 0, 0), Direction(Type::sensor, 0, 0) });
					for (int y = 0; y < nr; y++)
						for (int x = 0; x < nc; x++)
						{
							T.get_pixel(0, y, x, i, RGB);
							for (int isp = 0; isp < 3; isp++)
								data.get()[y * nc * 3 + x * 3 + isp] = RGB[isp];
						}

					if (mipmap) {
						uint64_t n = nr * cols * 3;
						std::shared_ptr<float> mipmap{ new float[n], [](float* p) {delete[] p; } };
						if (i == 0)
							createMipMapWithInfo(data, nr, nc, mipmap, mip);
						else
							createMipMap(data, nr, nc, mipmap);
						image::Image<float> img(mipmap.get(), nr, cols, 3);
						mif.addImage(i, img);
						//bigW.pushEntity(mipmap, big::DataTypes::FLOAT);
					} else {
						//bigW.pushEntity(data, big::DataTypes::FLOAT);
						image::Image<float> img(data.get(), nr, nc, 3);
						mif.addImage(i, img);
					}

					
				}
				if (mipmap)
					mif.addBtfMipmap("btf0", mip);
				mif.addBtfDirectionsReference("btf0", "directions0", directions);
				mif.addDirections("directions0", directions);
				mif.saveXML();
				
			}

			// control reading ------------------------------------
			controlReading(filename);
			
		} else // UBO sampling
		{
			std::string lFilename = filename + ".big";
			// loading old BIG --------------
			TBIG T;
			int status = T.load(lFilename.c_str(), true, 10000000000000); // true = memory, false = disk
			if (status < 0) {
				std::cout << "Open BIG file failed. File name: " << filename << std::endl;
				return -5;
			}
			int NoOfImages = T.get_images();
			int nr = T.get_height();
			int nc = T.get_width();
			printf("height,width: %d %d\n", nr, nc);
			//T.report();
			printf("loading bigBTF...done\n");
			filename += ".mif";
			// creating new BIG -------------
			{
				int cols = nc;
				if (mipmap) 
					cols = (nc * 3 / 2) + 1;
				checkFilename(filename);
				MIFbtf mif(filename);
				mif.addBtfElement("btf0", "uniform");
				Directions directions;
				directions.type = Directions::Type::list;
				directions.name = "UBO81x81";
				
				Mipmap mip;
				mip.type = Mipmap::Type::isotropic;
				std::vector<std::pair<int, int>> levels;
				std::vector<std::pair<int, int>> sizes;
				uint64_t n = nr * nc * 3;
				
				std::shared_ptr<float> data{ new float[n], [](float* p) {delete[] p; } };
				// filling new BIG from old BIG --------------------
				int nillu = 81;
				int nview = 81;
				float RGB[3];
				for (int v = 0; v < nview; v++)
					for (int i = 0; i < nillu; i++)
					{
						int idx = v * nillu + i;
						for (int y = 0; y < nr; y++)
							for (int x = 0; x < nc; x++)
							{
								T.get_pixel(0, y, x, idx, RGB);
								for (int isp = 0; isp < 3; isp++)
									data.get()[y * nc * 3 + x * 3 + isp] = RGB[isp];
							}

						if(mipmap) {
							uint64_t n = nr * cols * 3;
							std::shared_ptr<float> mipmap{ new float[n], [](float* p) {delete[] p; } };
							if (i == 0)
								createMipMapWithInfo(data, nr, nc, mipmap, mip);
							else
								createMipMap(data, nr, nc, mipmap);
							image::Image<float> img(mipmap.get(), nr, nc, 3);
							mif.addImage(i, img);
							//bigW.pushEntity(mipmap, big::DataTypes::FLOAT);
						} else {
							//bigW.pushEntity(data, big::DataTypes::FLOAT);
							image::Image<float> img(data.get(), nr, nc, 3);
							mif.addImage(nview*v+i, img);
						}
						directions.listRecords.push_back({ Direction(Type::source, 0, 0), Direction(Type::sensor, 0, 0) });
					}

				mif.addBtfDirectionsReference("btf0", "directions0", directions);
				mif.addDirections("directions0", directions);
				if (mipmap)
					mif.addBtfMipmap("btf0", mip);

				//save xml to MIF
				mif.saveXML();
				std::cout << mif.getXML() << std::endl;
			}
			// control reading ------------------------------------
			controlReading(filename);

		}
		//std::cout << "Saving result: " << doc.save_file("output.xml") << std::endl;
	}
}
