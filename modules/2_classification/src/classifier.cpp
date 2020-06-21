#include "classifier.hpp"
#include <vector>
#include <opencv2/opencv.hpp>
#include <opencv2/core/utils/filesystem.hpp>
#include <inference_engine.hpp>
#include <algorithm>
#include <cmath> 

using namespace InferenceEngine;
using namespace cv;
using namespace cv::utils::fs;


bool comp2(float a, float b)
	{
	return (a > b); 
	};

void topK(const std::vector<float>& src, unsigned k,
	std::vector<float>& dst,
	std::vector<unsigned>& indices) {
	dst = src;
	sort(dst.begin(), dst.end(), comp2);
	dst.resize(k);
	for (int i = 0; i < k; i++)
	{
		const float &d = dst.at(i);
		auto it = std::find(src.begin(), src.end(), d);
		auto index = std::distance(src.begin(), it);
		indices.push_back(index);
	}

}
	

void softmax(std::vector<float>& values) {
	std::vector<float> y;
	y.reserve(values.size());
	float sum = 0;
	float max = *std::max_element(values.begin(), values.end());

	for (int i = 0; i < values.size(); i++)
	{
		float x = values.at(i) - max;
		sum += exp(x);
	}

	for (int i = 0; i < values.size(); i++)
	{
		y.push_back(exp(values.at(i) - max) / sum);
	}
	values = y;
}

Blob::Ptr wrapMatToBlob(const Mat& m) {
    CV_Assert(m.depth() == CV_8U);
    std::vector<size_t> dims = {1, (size_t)m.channels(), (size_t)m.rows, (size_t)m.cols};
    return make_shared_blob<uint8_t>(TensorDesc(Precision::U8, dims, Layout::NHWC),
                                     m.data);
}

Classifier::Classifier() {
    Core ie;

    // Load deep learning network into memory
    CNNNetwork net = ie.ReadNetwork(join(DATA_FOLDER, "DenseNet_121.xml"),
                                    join(DATA_FOLDER, "DenseNet_121.bin"));

    // Specify preprocessing procedures
    // (NOTE: this part is different for different models!)
    InputInfo::Ptr inputInfo = net.getInputsInfo()["data"];
    inputInfo->getPreProcess().setResizeAlgorithm(ResizeAlgorithm::RESIZE_BILINEAR);
    inputInfo->setLayout(Layout::NHWC);
    inputInfo->setPrecision(Precision::U8);
    outputName = net.getOutputsInfo().begin()->first;

    // Initialize runnable object on CPU device
    ExecutableNetwork execNet = ie.LoadNetwork(net, "CPU");

    // Create a single processing thread
    req = execNet.CreateInferRequest();
}

void Classifier::classify(const cv::Mat& image, std::vector<float>& probabilities) {
    // Create 4D blob from BGR image
    Blob::Ptr input = wrapMatToBlob(image);

    // Pass blob as network's input. "data" is a name of input from .xml file
    req.SetBlob("data", input);

    // Launch network
    req.Infer();

    // Copy output. "prob" is a name of output from .xml file
    float* output = req.GetBlob(outputName)->buffer();
	int size = req.GetBlob(outputName)->size();
	//probabilities.assign(output, output + sizeof(output));

	/*int result = sizeof(output) / sizeof(float);
	probabilities.reserve(sizeof(output));
	std::copy(output, output + sizeof(output), probabilities.begin());*/

	//int result = sizeof(output[2]) / sizeof(float);

	for (int i = 0; i < size; i++)
	{
		probabilities.push_back(output[i]);
	}
}
