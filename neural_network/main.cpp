// neural_network.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/ml/ml.hpp"
#include "neural_network.h"
#include <iostream>
#include <string>
#include <set>

using namespace std;
using namespace cv;
using namespace ml;

int main()
{
	string path = "pics/catdog/";
	neural_network n_n = neural_network();
	vector<string> files;

	float train_split_ratio = .001;
	int network_input_size = 512;


	Mat descriptor_set;
	std::vector<image_data*> descriptors_metadata;
	set<std::string> classes;

	n_n.read_files(path, files);

	n_n.read_images(files.begin(), files.begin() + (size_t)(files.size() * train_split_ratio), [&](const std::string& classname, const cv::Mat& descriptors)
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

	
	Mat descriptor_set_CV_32F;
	descriptor_set.convertTo(descriptor_set_CV_32F, CV_32F);

cout << std::to_string(descriptor_set.depth()) << endl;

	Mat labels;
	Mat vocabulary;
	try { 
		cv::kmeans(descriptor_set_CV_32F, network_input_size, labels, cv::TermCriteria(cv::TermCriteria::EPS +
			cv::TermCriteria::MAX_ITER, 10, 0.01), 1, cv::KMEANS_PP_CENTERS, vocabulary);;
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

	Mat train_samples;
	Mat train_responses;
	set<image_data*> unique_metadata(descriptors_metadata.begin(), descriptors_metadata.end());	
	for (auto it = unique_metadata.begin(); it != unique_metadata.end(); )
	{
		image_data* data = *it;
		cv::Mat normalizedHist;
		cv::normalize(data->bow_features, normalizedHist, 0, data->bow_features.rows, cv::NORM_MINMAX, -1, cv::Mat());
		train_samples.push_back(normalizedHist);
		train_responses.push_back(n_n.get_class_code(classes, data->class_name));
		delete *it; // clear memory
		++it;
	}

	Ptr<ml::ANN_MLP> mlp = n_n.get_trainedNeural_network(train_samples, train_responses);

	train_samples.release();
	train_responses.release();



	Mat image, gauss;
	image = imread(path);
	GaussianBlur(image, gauss, Size(0, 0), 2, 2);

	waitKey(0);
    return 0;
}

