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
#include <opencv2/features2d.hpp>

using namespace std;
using namespace cv;
using namespace ml;

int main()
{
	string path = "pics/catdog/";
	neural_network n_n = neural_network();
	vector<string> files;

	float train_split_ratio = .7f;
	int network_input_size = 512;

	random_shuffle(files.begin(), files.end());


	Mat descriptor_set;
	vector<image_data*> descriptors_metadata;
	set<string> classes;

	n_n.read_files(path, files);

	n_n.read_images(files.begin(), files.begin() + (size_t)(files.size() * train_split_ratio), [&](const string& classname, const Mat& descriptors)
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

cout << to_string(descriptor_set.depth()) << endl;

	Mat labels;
	Mat vocabulary;
	try { 
		kmeans(descriptor_set_CV_32F, network_input_size, labels, TermCriteria(TermCriteria::EPS +
			TermCriteria::MAX_ITER, 10, 0.01), 1, KMEANS_PP_CENTERS, vocabulary);;
	}
	catch (const exception& e)
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
		Mat normalizedHist;
		normalize(data->bow_features, normalizedHist, 0, data->bow_features.rows, NORM_MINMAX, -1, Mat());
		train_samples.push_back(normalizedHist);
		train_responses.push_back(n_n.get_class_code(classes, data->class_name));
		delete *it; // clear memory
		++it;
	}

	Ptr<ml::ANN_MLP> mlp = n_n.get_trainedNeural_network(train_samples, train_responses);

	train_samples.release();
	train_responses.release();

	FlannBasedMatcher flann;
	flann.add(vocabulary);
	flann.train();
	
	Mat test_samples;
	vector<int> test_output_expected;
	n_n.read_images(files.begin(), files.begin() + (size_t)(files.size() * train_split_ratio), [&](const string& classname, const Mat& descriptors)
	{
		Mat bow_features = n_n.get_bow_features(flann, descriptors, network_input_size);
		normalize(bow_features, bow_features, 0, bow_features.rows, NORM_MINMAX, -1, Mat());
		test_samples.push_back(bow_features);
		test_output_expected.push_back(n_n.get_class_id(classes, classname));
	});
	vector<vector<int>> confusion_matrix = n_n.get_confusion_matrix(mlp, test_samples, test_output_expected);

	std::cout << "Confusion matrix: " << std::endl;
	n_n.print_confusion_matrix(confusion_matrix, classes);
	std::cout << "Accuracy: " << n_n.get_accuracy(confusion_matrix) << std::endl;

	waitKey(0);
    return 0;
}

