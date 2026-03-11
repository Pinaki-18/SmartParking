#pragma once
#include <string>
#include <opencv2/opencv.hpp>

/**
 * Automatic License Plate Recognition (ALPR)
 *
 * Pipeline:
 *   1. Load image with cv::imread
 *   2. Grayscale + bilateral filter (noise reduction, edge preservation)
 *   3. Canny edge detection
 *   4. Contour extraction + aspect-ratio filtering (plates are ~2-5x wide)
 *   5. [Production] Extract ROI and run Tesseract OCR
 *   6. [Demo]       Return a procedurally generated plate string
 *
 * To enable real OCR: install tesseract-ocr + libtesseract-dev
 * and uncomment the Tesseract block in ALPR.cpp.
 */
class ALPR {
public:
    static std::string readPlate(const std::string& imagePath);
    static std::string generateMockPlate();   // For simulation

private:
    static cv::Mat                    preprocess(const cv::Mat& src);
    static std::vector<cv::RotatedRect> detectRegions(const cv::Mat& edges);
};
