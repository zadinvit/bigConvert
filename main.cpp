#include <iostream>
#include <fstream>
#include <cstring>

#include "big_core_write.hpp"
#include "big_core_read.hpp"

#include "opencv2/core.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/imgproc.hpp"
#include "pugixml.hpp"

#include "TBIG.h"

#define UBO 1
#define UNIFORM 0 

using namespace cv;
pugi::xml_document doc;

template <typename T>
std::string ToString(T const& in_val)
{
	return std::to_string(in_val);
}
// Specialization for boolean type to force "true"/"false"
template<>
std::string ToString(bool const& in_val)
{
	std::ostringstream oss;
	oss << std::boolalpha << in_val;
	return oss.str();
}

//write xml to string write
struct xml_string_writer : pugi::xml_writer
{
	std::string result;

	virtual void write(const void* data, size_t size)
	{
		result.append(static_cast<const char*>(data), size);
	}
};

//xml node to string
std::string node_to_string(pugi::xml_node node)
{
	xml_string_writer writer;
	node.print(writer);

	return writer.result;
}

void createMipMapWithInfo(shared_ptr<float>& data, const int& rows, const int& cols, shared_ptr<float>& output, std::vector<std::pair<int, int>> &levels, std::vector<std::pair<int, int>> &sizes) {
	Mat img = Mat(rows, cols, CV_32FC3, data.get());
	img.convertTo(img, CV_32FC3, 1 / 255.0);
	Mat mipmap = Mat::zeros(Size((img.cols) * 3 / 2, img.rows), CV_32FC3);
	img.copyTo(mipmap(cv::Rect(0, 0, img.cols, img.rows)));
	int row = 0;
	int level = 1;
	while (true) {
		Mat img2;
		resize(img, img2, Size(), pow(0.5,level), pow(0.5, level), INTER_AREA);
		levels.push_back(std::make_pair(img.cols, row));
		sizes.push_back(std::make_pair(img2.cols, img2.rows));
		img2.copyTo(mipmap(cv::Rect(img.cols, row, img2.cols, img2.rows)));
		//handle rectangle object special cases
		if (img2.rows <= 1 || img2.cols <= 1) {
			if (img2.cols > 1) {
				row += img2.rows;
				resize(img2, img2, Size(), 0.5, 1, INTER_AREA);
				levels.push_back(std::make_pair(img.cols, row));
				sizes.push_back(std::make_pair(img2.cols, img2.rows));
				img2.copyTo(mipmap(cv::Rect(img.cols, row, img2.cols, img2.rows)));
			} else if (img2.rows > 1) {
				row += img2.rows;
				resize(img2, img2, Size(), 1, 0.5, INTER_AREA);
				levels.push_back(std::make_pair(img.cols, row));
				sizes.push_back(std::make_pair(img2.cols, img2.rows));
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
	Mat mipmap = Mat::zeros(Size((img.cols) * 3 / 2, img.rows), CV_32FC3);
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


void addLevelsToXML(std::vector<std::pair<int, int>>& lvls, std::vector<std::pair<int, int>>& sizes) {
	pugi::xml_node root = doc.document_element();
	pugi::xml_node mipmap = root.append_child("mipmap");
	pugi::xml_node levels = mipmap.append_child("levels");
	levels.append_attribute("count") = lvls.size();
	for(int i =0; i<lvls.size(); i++){
		pugi::xml_node level = levels.append_child("level");
		pugi::xml_node size = level.append_child("size");
		size.append_attribute("cols") = sizes[i].first;
		size.append_attribute("rows") = sizes[i].second;
		pugi::xml_node origin = level.append_child("origin");
		origin.append_attribute("x") = lvls[i].first;
		origin.append_attribute("y") = lvls[i].second;
	}

}

//save xml to BIG
void writeXMLtoBig(big::BigCoreWrite &bigW) {

	std::string xmlString = node_to_string(doc.document_element());
	bigW.writeXML(xmlString);
	//std::cout << xmlString << std::endl;
}


void controlReading(std::string filename) {
	big::BigCoreRead bigR(filename, false);
	printf("height,width: %d %d\n", bigR.getImageHeight(), bigR.getImageWidth());
	printf("entities,images,planes: %d %d %d\n", bigR.getNumberOfEntities(), bigR.getNumberOfImages(), bigR.getNumberOfPlanes());
	std::cout << bigR.getXMLFile() << std::endl;
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
			filename += "_V2.big";
			// creating new BIG -------------
			{
				uint64_t cols = nc;
				if (mipmap) {
					cols = nc * 3 / 2;
				}
				std::vector<std::pair<int, int>> levels;
				std::vector<std::pair<int, int>> sizes;
				big::BigCoreWrite bigW(filename, nr, cols, 3);
				uint64_t n = nr * nc * 3;
				std::shared_ptr<float> data{ new float[n], [](float* p) {delete[] p; } };
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

					if (mipmap) {
						uint64_t n = nr * cols * 3;
						std::shared_ptr<float> mipmap{ new float[n], [](float* p) {delete[] p; } };
						if (i == 0)
							createMipMapWithInfo(data, nr, nc, mipmap, levels, sizes);
						else
							createMipMap(data, nr, nc, mipmap);
						bigW.pushEntity(mipmap, big::DataTypes::FLOAT);
					} else {
						bigW.pushEntity(data, big::DataTypes::FLOAT);
					}

					
				}
				if (mipmap) {
					addLevelsToXML(levels, sizes);
				}
				writeXMLtoBig(bigW);
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
			filename += "_V2.big";
			// creating new BIG -------------
			{
				int cols = nc;
				if (mipmap) 
					cols = nc * 3 / 2;

				big::BigCoreWrite bigW(filename.c_str(), nr, cols, 3);
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

						uint64_t n = nr * cols * 3;
						std::shared_ptr<float> mipmap{ new float[n], [](float* p) {delete[] p; } };
						if (i == 0 && v == 0)
							createMipMapWithInfo(data, nr, nc, mipmap, levels, sizes);
						else
							createMipMap(data, nr, nc, mipmap);
						bigW.pushEntity(mipmap, big::DataTypes::FLOAT);
					}

				if (mipmap)
					addLevelsToXML(levels, sizes);
				//save xml to BIG
				writeXMLtoBig(bigW);
			}
			// control reading ------------------------------------
			controlReading(filename);

		}
		std::cout << "Saving result: " << doc.save_file("output.xml") << std::endl;
	}
}
