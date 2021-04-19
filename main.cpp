#include <iostream>
#include <fstream>
#include <cstring>
#include <unordered_map>

#include "MIFbtf.hpp"

#include "opencv2/core.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/imgproc.hpp"
#include "pugixml.hpp"

using namespace mif;

#include "TBIG.h"


using namespace cv;

//enum class diffrent types of interpolating, diffrent angels, important in renderer
enum class Distribution { uniform, UBO, BTFthtd, BTFthph, none };
static std::unordered_map<std::string, Distribution > const table = { {"Uniform",Distribution::uniform}, {"UBO81x81",Distribution::UBO}, {"CoatingRegular", Distribution::BTFthtd}, {"CoatingSpecial", Distribution::BTFthph} };
Distribution parse(std::string str) {
	auto it = table.find(str);
	if (it != table.end())
		return it->second;
	else
		return Distribution::none;
}

string distToString(Distribution d) {
	for (auto& i : table) {
		if (i.second == d)
		{
			return i.first;
		}
	}
}

/*enum class diffrent types of filtering -
isotropy- standart mipmapping size of file 1,5*old
anisotropy - anisotropy mipmap size of file 4*old
not - no filtering, just read image for old file and save to new size of file old = new 
none - bad format 
*/
enum class Filtering { isotropy, anisotropy, not, none };
static std::unordered_map<std::string, Filtering > const table2 = { {"mipmap",Filtering::isotropy}, {"anisotropy",Filtering::anisotropy}, {"none",Filtering::not} };
Filtering parseFiltering(std::string str) {
	auto it = table2.find(str);
	if (it != table2.end())
		return it->second;
	else
		return Filtering::none;
}

string filtToString(Filtering d) {
	for (auto& i : table2) {
		if (i.second == d)
		{
			return i.first;
		}
	}
}

void createMipMapWithInfo(shared_ptr<float>& data, const int& rows, const int& cols, shared_ptr<float>& output, std::vector<ElementMipmapItem>& mip) {
	Mat img = Mat(rows, cols, CV_32FC3, data.get());
	Mat mipmap = Mat::zeros(Size(((img.cols) * 3 / 2)+1, img.rows), CV_32FC3);
	img.copyTo(mipmap(cv::Rect(0, 0, img.cols, img.rows)));
	int row = 0;
	mip.push_back(ElementMipmapItem(0,0, 0, img.cols, img.rows));
	int level = 1;
	
	while (true) {
		Mat img2;
		resize(img, img2, Size(), pow(0.5,level), pow(0.5, level), INTER_AREA);
		mip.push_back(ElementMipmapItem(level,img.cols, row, img2.cols, img2.rows));
		img2.copyTo(mipmap(cv::Rect(img.cols, row, img2.cols, img2.rows)));
		//handle rectangle object special cases
		if (img2.rows <= 1 || img2.cols <= 1) {
			if (img2.cols > 1) {
				row += img2.rows;
				resize(img2, img2, Size(), 0.5, 1, INTER_AREA);
				mip.push_back(ElementMipmapItem(level+1,img.cols, row, img2.cols, img2.rows));
				img2.copyTo(mipmap(cv::Rect(img.cols, row, img2.cols, img2.rows)));
			} else if (img2.rows > 1) {
				row += img2.rows;
				resize(img2, img2, Size(), 1, 0.5, INTER_AREA);
				mip.push_back(ElementMipmapItem(level + 1,img.cols, row, img2.cols, img2.rows));
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


void createAnisoMapWithInfo(shared_ptr<float>& data, const int& rows, const int& cols, shared_ptr<float>& output, image::Image<ElementMipmapItem>& mip) {
	Mat img = Mat(rows, cols, CV_32FC3, data.get());
	Mat mipmap = Mat::zeros(Size(2 * img.cols + 1, 2 * img.rows + 1), CV_32FC3);
	mip.resize(ceil(log2f(rows)), ceil(log2f(cols)));
	int levelxCnt = ceil(log2f(cols));
	bool xdone = false;
	int col = 0;
	for (int levelx = 0; !xdone; levelx++) {
		bool ydone = false;
		int row = 0;
		int tmpcol = 0;
		for (int levely = 0; !ydone; levely++) {
			Mat img2;
			resize(img, img2, Size(), pow(0.5, levelx), pow(0.5, levely), INTER_AREA);
			img2.copyTo(mipmap(cv::Rect(col, row, img2.cols, img2.rows)));
			if ((levelx + levely * levelxCnt) >= mip.size()) {
				mip.clear();
				mip.resize(ceil(log2f(rows)+1), ceil(log2f(cols)+1));
				levelxCnt = ceil(log2f(cols)+1);
				levely = -1;
				levelx = 0;
				col = 0;
				row = 0;
				continue;
			}
			mip(levely,levelx) = ElementMipmapItem(levelx, levely,col, row, img2.cols, img2.rows);

			//handle rectangle object special cases
			if (img2.rows <= 1 || img2.cols <= 1) {
				if(levelx != levelxCnt-1)
					ydone = true;
				else {
					row += img2.rows;
					tmpcol = img2.cols;
				}
				if (img2.rows <= 1 && img2.cols <= 1) {
					ydone = true;
					xdone = true;
				}
			} else {
				row += img2.rows;
				tmpcol = img2.cols;
			}
		}
		col += tmpcol;
	}
	/*mipmap.convertTo(img, CV_32FC3, 1 / 255.0);
	imwrite("anisotropy.png", mipmap);*/
	//convert to shared_ptr<float>
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

void createAnisoMap(shared_ptr<float>& data, const int& rows, const int& cols, shared_ptr<float>& output) {
	Mat img = Mat(rows, cols, CV_32FC3, data.get());
	Mat mipmap = Mat::zeros(Size(2*img.cols + 1, 2*img.rows +1), CV_32FC3);
	
	bool xdone = false;
	int col = 0;
	for (int levelx = 0; !xdone; levelx++) {
		bool ydone = false;
		int row = 0;
		int tmpcol=0;
		for (int levely = 0; !ydone; levely++) {
			Mat img2;
			resize(img, img2, Size(), pow(0.5, levelx), pow(0.5, levely), INTER_AREA);
			img2.copyTo(mipmap(cv::Rect(col, row, img2.cols, img2.rows)));
			//handle rectangle object special cases
			if (img2.rows <= 1 ) {
				ydone = true;
				if (img2.rows <= 1 && img2.cols <= 1) {
					xdone = true;
				}
			} else if (img2.rows <= 1 && img2.cols <= 1) {
				xdone = true;
			} else {
				row += img2.rows;
				tmpcol = img2.cols;
			}
		}
		col += tmpcol;
	}
	/*mipmap.convertTo(img, CV_32FC3, 1 / 255.0);
	imwrite("anisotropy.png", mipmap);*/
	
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

//if mif file exist delete it, before creating new
void checkFilename(std::string filename) {
	FILE* fp = fopen(filename.c_str(), "r");
	if (fp) {
		fclose(fp);
		remove(filename.c_str());
	}
}

//read data from new format and write into console
void controlReading(std::string filename) {
	MIFbtf mif(filename,true);
	auto btf = mif.getBtf("btf0");
	std::cout<<"BTF name" <<btf.getName()<<std::endl;
	printf("height,width: %d %d\n", mif.getImageHeight(0), mif.getImageWidth(0));
	printf("images,planes: %d %d\n", mif.getNumberOfImages(), mif.getImageNumberOfPlanes(0));
	string xmlFilename = filename.substr(0, filename.size() - 3) + "xml";
	mif.saveXMLtoSeparateFile(xmlFilename);
	std::cout << "control XML save to: " << xmlFilename<< std::endl;
}

void loadOldBIG(TBIG &T, std::string& filename, int &nr,int & nc, int &NoOfImages) {
	std::string lFilename = filename + ".big";
	int status = T.load(lFilename.c_str(), true, 10000000000000); // true = memory, false = disk
	if (status < 0) {
		std::cout << "Open BIG file failed. File name: " << filename << std::endl;
		return;
	}
	NoOfImages = T.get_images();
	nr = T.get_height();
	nc = T.get_width();
	printf("height,width: %d %d\n", nr, nc);
	printf("loading bigBTF...done\n");
}

void initMipColsAndRows(uint64_t& cols, uint64_t& rows, int& nc, int& nr, Filtering& filt) {
	if (filt == Filtering::isotropy) {
		cols = (nc * 3 / 2) + 1;
	}
	if (filt == Filtering::anisotropy) {
		rows = 2 * nr + 1;
		cols = 2 * nc + 1;
	}
}

//convert old big file to mif file with uniform indexing
void convertUniform(std::string & filename, Filtering& filt) {
	TBIG T;
	int nr, nc, NoOfImages;
	loadOldBIG(T, filename, nr, nc, NoOfImages);
	
	filename += ".mif";
	// creating new MIF -------------
	{
		checkFilename(filename);
		MIFbtf mif(filename);
		ElementBtf btf("btf0", distToString(Distribution::uniform), "", "", "", "");
		std::vector<ElementDirectionsItem> dirItems;
		uint64_t cols = nc;
		uint64_t rows = nr;
		initMipColsAndRows(cols, rows, nc,nr, filt);
		uint64_t n = nr * nc * 3;
		std::shared_ptr<float> data{ new float[n], [](float* p) {delete[] p; } };
		// filling new MIF from old BIG --------------------
		float RGB[3];
		float angles[12];
		for (int i = 0; i < NoOfImages; i++)
		{
			T.get_angles(i, angles);
			dirItems.push_back(ElementDirectionsItem(i, { ElementDirection(0, DirectionType::source, static_cast<double>(angles[0]), static_cast<double>(angles[1])),
				ElementDirection(1, DirectionType::sensor, static_cast<double>(angles[2]), static_cast<double>(angles[3])) }));
			for (int y = 0; y < nr; y++)
				for (int x = 0; x < nc; x++)
				{
					T.get_pixel(0, y, x, i, RGB);
					for (int isp = 0; isp < 3; isp++)
						data.get()[y * nc * 3 + x * 3 + isp] = RGB[isp];
				}

			if (filt == Filtering::isotropy) {
				uint64_t n = nr * cols * 3;
				std::shared_ptr<float> mipmap{ new float[n], [](float* p) {delete[] p; } };
				if (i == 0) {
					std::vector<ElementMipmapItem> mip;
					createMipMapWithInfo(data, nr, nc, mipmap, mip);
					btf.setMipmap(ElementMipmap(mip));
				}
				else
					createMipMap(data, nr, nc, mipmap);
				image::Image<float> img(mipmap.get(), nr, cols, 3);
				mif.addImage(i, img);
			} else if (filt == Filtering::anisotropy) {
				uint64_t n = rows * cols * 3;
				std::shared_ptr<float> arrmip{ new float[n], [](float* p) {delete[] p; } };
				if (i == 0) {
					image::Image<ElementMipmapItem> mip;
					createAnisoMapWithInfo(data, nr, nc, arrmip, mip);
					btf.setMipmap(ElementMipmap(mip));
				}
				else
					createAnisoMap(data, nr, nc, arrmip);
				image::Image<float> img(arrmip.get(), rows, cols, 3);
				mif.addImage(i, img);
			} else {
				image::Image<float> img(data.get(), nr, nc, 3);
				mif.addImage(i, img);
			}
		}
		ElementDirections directions("directions0", dirItems);
		directions.setNameUniform(15.0, 30.0);
		btf.setDirections(directions);
		mif.setBtf(btf);
		mif.setDirections(directions);
		mif.saveXML();
	}

	// control reading ------------------------------------
	controlReading(filename);
}

//convert old big file to mif file with UBO indexing
void convertUBO(std::string& filename, Filtering& filt) {
	std::string lFilename = filename + ".big";
	// loading old BIG --------------
	TBIG T;
	int nr, nc, NoOfImages;
	loadOldBIG(T,filename, nr, nc, NoOfImages);
	filename += ".mif";
	// creating new BIG -------------
	{
		checkFilename(filename);
		MIFbtf mif(filename);
		ElementBtf btf("btf0", distToString(Distribution::UBO), "", "", "", "");
		std::vector<ElementDirectionsItem> dirItems;
		uint64_t cols = nc;
		uint64_t rows = nr;
		initMipColsAndRows(cols, rows, nc, nr, filt);

		uint64_t n = nr * nc * 3;

		std::shared_ptr<float> data{ new float[n], [](float* p) {delete[] p; } };
		// filling new MIF from old BIG --------------------
		int nillu = 81;
		int nview = 81;
		float angles[12];
		float RGB[3];
		int k = 0;
		for (int v = 0; v < nview; v++)
			for (int i = 0; i < nillu; i++)
			{
				T.get_angles(k, angles);
				dirItems.push_back(ElementDirectionsItem(k, { ElementDirection(0, DirectionType::source, static_cast<double>(angles[0]), static_cast<double>(angles[1])),
				ElementDirection(1, DirectionType::sensor, static_cast<double>(angles[2]), static_cast<double>(angles[3])) }));
				int idx = v * nillu + i;
				for (int y = 0; y < nr; y++)
					for (int x = 0; x < nc; x++)
					{
						T.get_pixel(0, y, x, idx, RGB);
						for (int isp = 0; isp < 3; isp++)
							data.get()[y * nc * 3 + x * 3 + isp] = RGB[isp];
					}

				if (filt == Filtering::isotropy) {
					uint64_t n = nr * cols * 3;
					std::shared_ptr<float> arrmip{ new float[n], [](float* p) {delete[] p; } };
					if (k == 0) {
						std::vector<ElementMipmapItem> mip;
						createMipMapWithInfo(data, nr, nc, arrmip, mip);
						btf.setMipmap(ElementMipmap(mip));
					}
					else
						createMipMap(data, nr, nc, arrmip);
					image::Image<float> img(arrmip.get(), nr, cols, 3);
					mif.addImage(k, img);
				} else if (filt == Filtering::anisotropy) {
					uint64_t n = rows * cols * 3;
					std::shared_ptr<float> arrmip{ new float[n], [](float* p) {delete[] p; } };
					if (k == 0) {
						image::Image<ElementMipmapItem> mip;
						createAnisoMapWithInfo(data, nr, nc, arrmip, mip);
						btf.setMipmap(ElementMipmap(mip));
					}
					else
						createAnisoMap(data, nr, nc, arrmip);
					image::Image<float> img(arrmip.get(),rows, cols, 3);
					mif.addImage(k, img);
				} else {
					image::Image<float> img(data.get(), nr, nc, 3);
					mif.addImage(k, img);
				}
				k++;
			}

		ElementDirections directions("directions0", dirItems);
		directions.setNameUBO81x81();
		btf.setDirections(directions);
		mif.setBtf(btf);
		mif.setDirections(directions);
		//save xml to MIF
		mif.saveXML();
		std::cout << mif.getXML() << std::endl;
	}
	// control reading ------------------------------------
	controlReading(filename);
}
//convert old big file to mif file with thtd indexing
void convertthtd(std::string &filename, Filtering& filt) {
	TBIG T;
	int nr, nc, NoOfImages;
	loadOldBIG(T, filename, nr, nc, NoOfImages);
	filename += ".mif";
	// creating new BIG -------------
	{
		checkFilename(filename);
		MIFbtf mif(filename);
		ElementBtf btf("btf0", distToString(Distribution::BTFthtd), "", "", "", "");
		std::vector<ElementDirectionsItem> dirItems;
		uint64_t cols = nc;
		uint64_t rows = nr;
		initMipColsAndRows(cols, rows, nc, nr, filt);
		uint64_t n = nr * nc * 3;
		std::shared_ptr<float> data{ new float[n], [](float* p) {delete[] p; } };
		// filling new MIF from old BIG --------------------
		float angles[12];
		float RGB[3];
		for (int i = 0; i < NoOfImages; i++)
		{
			T.get_angles(i, angles);
			dirItems.push_back(ElementDirectionsItem(i, { ElementDirection(0, DirectionType::source, static_cast<double>(angles[0]), static_cast<double>(angles[1])),
				ElementDirection(1, DirectionType::sensor, static_cast<double>(angles[2]), static_cast<double>(angles[3])) }));
			for (int y = 0; y < nr; y++)
				for (int x = 0; x < nc; x++)
				{
					T.get_pixel(0, y, x, i, RGB);
					for (int isp = 0; isp < 3; isp++)
						data.get()[y * nc * 3 + x * 3 + isp] = RGB[isp];
				}
			if (filt == Filtering::isotropy) {
				uint64_t n = nr * cols * 3;
				std::shared_ptr<float> mipmap{ new float[n], [](float* p) {delete[] p; } };
				if (i == 0) {
					std::vector<ElementMipmapItem> mip;
					createMipMapWithInfo(data, nr, nc, mipmap, mip);
					btf.setMipmap(ElementMipmap(mip));
				}
				else
					createMipMap(data, nr, nc, mipmap);
				image::Image<float> img(mipmap.get(), nr, cols, 3);
				mif.addImage(i, img);
			} else if (filt == Filtering::anisotropy) {
				uint64_t n = rows * cols * 3;
				std::shared_ptr<float> arrmip{ new float[n], [](float* p) {delete[] p; } };
				if (i == 0) {
					image::Image<ElementMipmapItem> mip;
					createAnisoMapWithInfo(data, nr, nc, arrmip, mip);
					btf.setMipmap(ElementMipmap(mip));
				}
				else
					createAnisoMap(data, nr, nc, arrmip);
				image::Image<float> img(arrmip.get(), rows, cols, 3);
				mif.addImage(i, img);
			} else {
				image::Image<float> img(data.get(), nr, nc, 3);
				mif.addImage(i, img);
			}
		}

		ElementDirections directions("directions0", dirItems);
		directions.setNameCoatingRegular(5.0, 15.0);
		btf.setDirections(directions);
		mif.setBtf(btf);
		mif.setDirections(directions);
		//save xml to MIF
		mif.saveXML();
		std::cout << mif.getXML() << std::endl;
		controlReading(filename);
	}

}

//convert old big file to mif file with thph indexing
void convertthph(std::string &filename, Filtering& filt) {
	TBIG T;
	std::string lFilename = filename + ".big";
	int nr, nc, NoOfImages;
	loadOldBIG(T, filename, nr, nc, NoOfImages);
	filename += ".mif";
	// creating new BIG -------------
	{
		checkFilename(filename);
		MIFbtf mif(filename);
		ElementBtf btf("btf0", distToString(Distribution::BTFthtd), "", "", "", "");
		std::vector<ElementDirectionsItem> dirItems;
		uint64_t cols = nc;
		uint64_t rows = nr;
		initMipColsAndRows(cols, rows, nc, nr, filt);
		uint64_t n = nr * nc * 3;
		std::shared_ptr<float> data{ new float[n], [](float* p) {delete[] p; } };
		// filling new BIG from old BIG --------------------
		float angles[12];
		float RGB[3];
		for (int i = 0; i < NoOfImages; i++)
		{
			T.get_angles(i, angles);
			dirItems.push_back(ElementDirectionsItem(i, { ElementDirection(0, DirectionType::source, static_cast<double>(angles[0]), static_cast<double>(angles[1])),
				ElementDirection(1, DirectionType::sensor, static_cast<double>(angles[2]), static_cast<double>(angles[3])) }));
			for (int y = 0; y < nr; y++)
				for (int x = 0; x < nc; x++)
				{
					T.get_pixel(0, y, x, i, RGB);
					for (int isp = 0; isp < 3; isp++)
						data.get()[y * nc * 3 + x * 3 + isp] = RGB[isp];
				}
			if (filt == Filtering::isotropy) { //create mipmap 1,5x bigger size of image 
				uint64_t n = nr * cols * 3;
				std::shared_ptr<float> mipmap{ new float[n], [](float* p) {delete[] p; } };
				if (i == 0) {
					std::vector<ElementMipmapItem> mip;
					createMipMapWithInfo(data, nr, nc, mipmap, mip);
					btf.setMipmap(ElementMipmap(mip));
				}
				else
					createMipMap(data, nr, nc, mipmap);
				image::Image<float> img(mipmap.get(), nr, cols, 3);
				mif.addImage(i, img);
			} else if (filt == Filtering::anisotropy) { //create anisotropic map 4x bigger image 
				uint64_t n = rows * cols * 3;
				std::shared_ptr<float> arrmip{ new float[n], [](float* p) {delete[] p; } };
				if (i == 0) {
					image::Image<ElementMipmapItem> mip;
					createAnisoMapWithInfo(data, nr, nc, arrmip, mip);
					btf.setMipmap(ElementMipmap(mip));
				}
				else
					createAnisoMap(data, nr, nc, arrmip);
				image::Image<float> img(arrmip.get(), rows, cols, 3);
				mif.addImage(i, img);
			} else {
				image::Image<float> img(data.get(), nr, nc, 3);
				mif.addImage(i, img);
			}
		}
		ElementDirections directions("directions0", dirItems);
		directions.setNameCoatingSpecial(5.0,30.0);
		btf.setDirections(directions);
		mif.setBtf(btf);
		mif.setDirections(directions);

		//save xml to MIF
		mif.saveXML();
		std::cout << mif.getXML() << std::endl;
		controlReading(filename);
	}

}

int main()
{
	std::ifstream configfile("config.txt");
	if (configfile.fail()) {
		std::cout << "Config file missing" << std::endl;
		return -1;
	}
	std::string line;
	std::getline(configfile, line);
	std::istringstream iss(line);
	std::string text;
	iss = std::istringstream(line);
	string distStr;
	if (!(iss >> text >> distStr)) {
		std::cout << "UBO parse error" << std::endl;
		return -1;
	}
	Distribution dist = parse(distStr);
	std::getline(configfile, line);
	string filter;
	iss = std::istringstream(line);
	if (!(iss >> text >> filter)) {
		std::cout << "mipmap parse error" << std::endl;
		return -1;
	}
	Filtering filtering = parseFiltering(filter);
	if (filtering == Filtering::none) {
		std::cout << " Bad filtering name, filtering: choice -> choices are: " << std::endl;
		for (auto& item : table2) {
			std::cout << item.first << std::endl;
		}
		std::cout << std::endl;
		return -1;
	}
	
	while (std::getline(configfile, line)) {
		iss = std::istringstream(line);
		std::string distStr;
		std::string filename;
		if (!(iss >> filename)) {
			std::cout << "Filename parse error" << std::endl;
			return -1;
		}
		if ((iss >> distStr)) {
			dist = parse(distStr);
		}
		switch (dist)
		{
		case Distribution::uniform:
			convertUniform(filename, filtering);
			break;
		case Distribution::UBO:
			convertUBO(filename, filtering);
			break;
		case Distribution::BTFthtd:
			convertthtd(filename, filtering);
			break;
		case Distribution::BTFthph:
			convertthph(filename, filtering);
			break;
		case Distribution::none:
			std::cout << filename << " Bad distribution name, choices are: " << std::endl;
			for (auto& item : table) {
				std::cout << item.first << std::endl;
			}
			std::cout << std::endl;
			break;
		default:
			break;
		}
	}
}
