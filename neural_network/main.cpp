// neural_network.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "neural_network.h"
#include <iostream>
#include <string>
#include <set>

using namespace std;
using namespace cv;

int main()
{
	string path = "pics/catdog/";
	neural_network n_n = neural_network();
	vector<string> files;

	float trainSplitRatio = .001;
	int network_input_size = 512;


	Mat descriptor_set;
	std::vector<image_data*> descriptors_metadata;
	set<std::string> classes;

	n_n.read_files(path, files);

	n_n.read_images(files.begin(), files.begin() + (size_t)(files.size() * trainSplitRatio), [&](const std::string& classname, const cv::Mat& descriptors)
	{
		classes.insert(classname);
		descriptor_set.push_back(descriptors);
		image_data* data = new image_data;
		data->class_name = classname;
		data->bow_features = Mat::zeros(Size(network_input_size,1), CV_32F);
		for (int j = 0; j < descriptors.rows; j++)
		{
			descriptors_metadata.push_back(data);
		}
	});

	Mat labels;
	Mat voc;
	try { 
	kmeans(descriptor_set, network_input_size, labels, TermCriteria(TermCriteria::EPS + TermCriteria::MAX_ITER, 10, 0.01), 1, KMEANS_PP_CENTERS, voc);
	}
	catch (const std::exception& e)
	{
		string msg = e.what();
	}
	descriptor_set.release();
	
	int* ptr_labels = (int*)(labels.data);
	int size = labels.rows * labels.cols;
	for (int i = 0; i < size; ++i)
	{
		int label = *ptr_labels++;
		image_data* data = descriptors_metadata[i];
		data->bow_features.at<float>(label)++;
	}


	Mat image, gauss;
	image = imread(path);
	GaussianBlur(image, gauss, Size(0, 0), 2, 2);

	waitKey(0);
    return 0;
}

