#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
#include <iostream>
#include <opencv2/opencv.hpp>
#include <ELSED.h>

namespace py = pybind11;
using namespace upm;

// Converts C++ descriptors to Numpy
inline py::tuple salient_segments_to_py(const upm::SalientSegments &ssegs) {
  py::array_t<float> scores(ssegs.size());
  py::array_t<float> segments({int(ssegs.size()), 4});
  float *p_scores = scores.mutable_data();
  float *p_segments = segments.mutable_data();
  for (int i = 0; i < ssegs.size(); i++) {
    p_scores[i] = ssegs[i].salience;
    p_segments[i * 4] = ssegs[i].segment[0];
    p_segments[i * 4 + 1] = ssegs[i].segment[1];
    p_segments[i * 4 + 2] = ssegs[i].segment[2];
    p_segments[i * 4 + 3] = ssegs[i].segment[3];
  }
  return pybind11::make_tuple(segments, scores);
}

py::tuple compute_elsed(const py::array &py_img,
                        const py::array &py_dfx,
                        const py::array &py_dfy,
                        float sigma = 1,
                        float gradientThreshold = 30,
                        int minLineLen = 15,
                        double lineFitErrThreshold = 0.2,
                        double pxToSegmentDistTh = 1.5,
                        double validationTh = 0.15,
                        bool validate = true,
                        bool treatJunctions = true,
                        bool givenFields = true
) {

  py::buffer_info info_img = py_img.request();
  py::buffer_info info_dfx = py_dfx.request();
  py::buffer_info info_dfy = py_dfy.request();

  cv::Mat img(info_img.shape[0], info_img.shape[1], CV_8UC1, (uint8_t *) info_img.ptr);
  cv::Mat dfx(info_dfx.shape[0], info_dfx.shape[1], CV_16SC1, (int16_t *) info_dfx.ptr);
  cv::Mat dfy(info_dfy.shape[0], info_dfy.shape[1], CV_16SC1, (int16_t *) info_dfy.ptr);

  ELSEDParams params;

  params.sigma = sigma;
  params.ksize = cvRound(sigma * 3 * 2 + 1) | 1; // Automatic kernel size detection
  params.gradientThreshold = gradientThreshold;
  params.minLineLen = minLineLen;
  params.lineFitErrThreshold = lineFitErrThreshold;
  params.pxToSegmentDistTh = pxToSegmentDistTh;
  params.validationTh = validationTh;
  params.validate = validate;
  params.treatJunctions = treatJunctions;
  params.givenFields = givenFields;

  ELSED elsed(params);
  upm::SalientSegments salient_segs = elsed.detectSalient(img, dfx, dfy);

  return salient_segments_to_py(salient_segs);
}

PYBIND11_MODULE(pyelsed, m) {
  m.def("detect", &compute_elsed, R"pbdoc(
        Computes ELSED: Enhanced Line SEgment Drawing in the input image.
    )pbdoc",
        py::arg("img"),
        py::arg("dfx"),
        py::arg("dfy"),
        py::arg("sigma") = 1,
        py::arg("gradientThreshold") = 30,
        py::arg("minLineLen") = 15,
        py::arg("lineFitErrThreshold") = 0.2,
        py::arg("pxToSegmentDistTh") = 1.5,
        py::arg("validationTh") = 0.15,
        py::arg("validate") = true,
        py::arg("treatJunctions") = true,
        py::arg("givenFields") = true
  );
}
