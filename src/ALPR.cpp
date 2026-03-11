#include "ALPR.h"
#include <iostream>
#include <random>
#include <sstream>

cv::Mat ALPR::preprocess(const cv::Mat& src) {
    cv::Mat gray, smooth, edges;
    cv::cvtColor(src, gray, cv::COLOR_BGR2GRAY);

    // Bilateral filter: smooths flat areas but preserves plate edges
    cv::bilateralFilter(gray, smooth, 11, 17, 17);

    // Canny: lower=30 catches weak edges, upper=200 keeps strong ones
    cv::Canny(smooth, edges, 30, 200);
    return edges;
}

std::vector<cv::RotatedRect> ALPR::detectRegions(const cv::Mat& edges) {
    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(edges.clone(), contours, cv::RETR_TREE, cv::CHAIN_APPROX_SIMPLE);

    // Sort largest contours first (plates tend to be large blobs)
    std::sort(contours.begin(), contours.end(), [](const auto& a, const auto& b){
        return cv::contourArea(a) > cv::contourArea(b);
    });

    std::vector<cv::RotatedRect> results;
    int limit = std::min(10, (int)contours.size());
    for (int i = 0; i < limit; ++i) {
        cv::RotatedRect rr = cv::minAreaRect(contours[i]);
        float w = rr.size.width, h = rr.size.height;
        if (h < 1.0f) continue;
        float ratio = (w > h) ? w / h : h / w;
        // Typical plate aspect ratio: 2.0 to 5.5
        if (ratio >= 2.0f && ratio <= 5.5f)
            results.push_back(rr);
    }
    return results;
}

std::string ALPR::readPlate(const std::string& imagePath) {
    cv::Mat img = cv::imread(imagePath);
    if (img.empty()) {
        std::cerr << "[ALPR] Could not load: " << imagePath << " → using mock plate\n";
        return generateMockPlate();
    }

    auto edges   = preprocess(img);
    auto regions = detectRegions(edges);

    if (regions.empty()) {
        std::cout << "[ALPR] No plate region detected → using mock plate\n";
        return generateMockPlate();
    }

    std::cout << "[ALPR] Plate region at ("
              << (int)regions[0].center.x << ","
              << (int)regions[0].center.y << ")\n";

    // ---- Tesseract OCR (uncomment to enable) ----
    // #include <tesseract/baseapi.h>
    // cv::Mat roi;
    // cv::getRectSubPix(img, regions[0].size, regions[0].center, roi);
    // cv::cvtColor(roi, roi, cv::COLOR_BGR2GRAY);
    // tesseract::TessBaseAPI tess;
    // tess.Init(NULL, "eng");
    // tess.SetPageSegMode(tesseract::PSM_SINGLE_LINE);
    // tess.SetImage(roi.data, roi.cols, roi.rows, 1, (int)roi.step);
    // std::string text(tess.GetUTF8Text());
    // text.erase(std::remove_if(text.begin(),text.end(),::isspace), text.end());
    // return text;
    // -------------------------------------------

    return generateMockPlate(); // Remove once Tesseract is wired up
}

std::string ALPR::generateMockPlate() {
    // Indian-style plate: KA-05-AB-1234
    static std::mt19937 rng{std::random_device{}()};
    static std::uniform_int_distribution<int> L(0, 25), D(0, 9);

    std::ostringstream ss;
    ss << (char)('A'+L(rng)) << (char)('A'+L(rng)) << '-'
       << D(rng) << D(rng) << '-'
       << (char)('A'+L(rng)) << (char)('A'+L(rng)) << '-'
       << D(rng) << D(rng) << D(rng) << D(rng);
    return ss.str();
}
