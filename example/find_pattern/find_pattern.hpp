#ifndef __FIND_PATTERN_HPP__
#define __FIND_PATTERN_HPP__

#include <algorithm>
#include <chrono>
#include <iostream>
#include <limits>
#include <opencv2/calib3d/calib3d.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/features2d/features2d.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <vector>

#include <is/msgs/camera.hpp>
#include <is/msgs/common.hpp>

using namespace is::msg::camera;
using namespace is::msg::common;
using namespace std;

struct Contour {
  std::vector<cv::Point> points;
  cv::Point2d center;
  double distance;
  double area;

  Contour() {}

  Contour(std::vector<cv::Point> points, cv::Point2d center) {
    this->points = points;
    this->center = center;
  }
};

struct PointDist {
  cv::Point2d point;
  double distance;
};

void draw_contours(std::vector<std::vector<cv::Point>> const& contours, cv::Mat& output, cv::Size size) {
  output = cv::Mat::zeros(size, CV_8UC3);
  cv::RNG rng(12345);
  for (int i = 0; i < (int)contours.size(); i++) {
    cv::Scalar color = cv::Scalar(rng.uniform(0, 255), rng.uniform(0, 255), rng.uniform(0, 255));
    cv::drawContours(output, contours, i, color);
  }
}

void draw_contour(std::vector<std::vector<cv::Point>> const& contours, cv::Mat& output) {
  cv::cvtColor(output, output, CV_GRAY2BGR);
  for (int i = 0; i < (int)contours.size(); i++) {
    cv::Scalar color = cv::Scalar(0, 0, 255);
    cv::drawContours(output, contours, i, color, 3);
  }
}

bool compare_distance(PointDist const& x, PointDist const& y) { return x.distance < y.distance; };

void erase_point(std::vector<cv::Point2d>& v, cv::Point2d const& pt) {
  v.erase(std::remove(v.begin(), v.end(), pt), v.end());
}

std::vector<PointDist> distance_to_line(cv::Point2d const& vec_dir, cv::Point2d const& ref,
                                            std::vector<cv::Point2d> const& points) {
  std::vector<PointDist> v;
  PointDist v_aux;
  for (auto& p : points) {
    v.push_back({p, cv::norm(vec_dir.cross(p - ref))});
  }
  return v;
}

std::vector<cv::Point2d> nearest_points(cv::Point2d const& vec_dir, cv::Point2d const& ref,
                                        std::vector<cv::Point2d> const& points, int const& qnt_nearest) {
  std::vector<cv::Point2d> n_pts;
  std::vector<PointDist> v = distance_to_line(vec_dir, ref, points);
  std::sort(v.begin(), v.end(), compare_distance);
  for (int i = 0; i < std::min((int)v.size(), qnt_nearest); i++) {
    n_pts.push_back(v[i].point);
  }
  return n_pts;
}

std::vector<PointDist> distance_points_ref(std::vector<cv::Point2d> const& points, cv::Point2d const& ref) {
  std::vector<PointDist> v;
  for (auto& p : points) {
    v.push_back({p, cv::norm(p - ref)});
  }
  return v;
}

namespace fifi {

cv::Point2d centroid(std::vector<cv::Point2d> const& points) {
  return std::accumulate(points.begin(), points.end(), cv::Point2d(0.0, 0.0)) / static_cast<double>(points.size());
}

}  // ::fifi

bool validate_pattern(std::vector<cv::Point2d>& points, cv::Point2d& reference, int const& m, int const& n) {
  int q = static_cast<int>((0.4 * m * n));
  std::vector<cv::Point2d> nearest_pts;
  std::vector<PointDist> pts_dist, pts_dists_aux, pts_dists_vec_centroid, pts_dists_diag;
  cv::Point2d vec_centroid, vec_diag_1, vec_diag_2, vert_1, vert_2, vec_1, vec_2, vec_aux, vec_sup, vec_inf;

  int min_size = std::min(m, n);
  int max_size = std::max(m, n);

  std::vector<cv::Point2d> pattern(m * n, cv::Point2d(0, 0));
  pattern[0] = reference;
  auto centroid = fifi::centroid(points);
  erase_point(points, reference);

  for (auto& p : points) {
    pts_dist.push_back({p, cv::norm(p - reference)});
  }
  std::sort(pts_dist.begin(), pts_dist.end(), compare_distance);
  vec_centroid = centroid - reference;

  pts_dists_vec_centroid = distance_to_line(vec_centroid, reference, points);
  std::sort(pts_dists_vec_centroid.begin(), pts_dists_vec_centroid.end(), compare_distance);
  // verifica entre os (m+n) pontos mais proximos da reta suporte
  // do vetor vec_centroid, qual é o mais distante da referência
  // para definir qual é o último ponto do padrão e o vetor diagonal 1
  double max_distance = 0;
  double cur_distance;
  for (int i = 0; i < q; i++) {
    cur_distance = cv::norm(pts_dists_vec_centroid[i].point - reference);
    if (cur_distance > max_distance) {
      max_distance = cur_distance;
      vec_diag_1 = pts_dists_vec_centroid[i].point - reference;
    }
  }
  //  com o vetor diagonal, calcula-se as distâncias dos pontos
  //    que restaram até a reta suporte do vetor,
  //    em seguida ordena o vetor por distancia
  pts_dists_diag = distance_to_line(vec_diag_1, reference, points);
  std::sort(pts_dists_diag.begin(), pts_dists_diag.end(), compare_distance);
  //  O ponto mais distante é definido como um dos vértices
  vert_1 = pts_dists_diag.back().point;
  //  define vetor que aponta na outra diagonal a partir do
  // ponto médio de ref e o ponto mais distante
  vec_diag_2 = vert_1 - (reference + cv::Point2d(vec_diag_1.x / 2, vec_diag_1.y / 2));
  //  com o vetor diagonal, calcula-se as distâncias dos pontos
  // que restaram até a reta suporte do vetor, em seguida ordena o vetor por
  // distancia
  pts_dists_vec_centroid = distance_to_line(vec_diag_2, vert_1, points);
  std::sort(pts_dists_vec_centroid.begin(), pts_dists_vec_centroid.end(), compare_distance);
  //  verifica entre os (m + n) pontos mais proximos da reta suporte do
  //  vetor vec_centroid, qual é o mais distante da referência para
  //  definir qual é o outro vértice do padrão
  max_distance = 0;
  for (int i = 0; i < q; i++) {
    cur_distance = norm(pts_dists_vec_centroid[i].point - vert_1);
    if (cur_distance > max_distance) {
      max_distance = cur_distance;
      vert_2 = pts_dists_vec_centroid[i].point;
    }
  }
  vec_1 = vert_1 - reference;
  vec_2 = vert_2 - reference;

  //  avalia qual o vetor superior
  if (((vec_diag_1.x < 0) && (vec_diag_1.cross(vec_1) < 0)) || ((vec_diag_1.x > 0) && (vec_diag_1.cross(vec_1) < 0))) {
    vec_sup = vec_1;
    vec_inf = vec_2;
  } else {
    vec_sup = vec_2;
    vec_inf = vec_1;
  }
  //  coleta os pontos mais próximos da reta do vetor superior
  nearest_pts = nearest_points(vec_sup, reference, points, min_size - 1);
  pts_dists_aux = distance_points_ref(nearest_pts, reference);
  std::sort(pts_dists_aux.begin(), pts_dists_aux.end(), compare_distance);
  // marca da linha {1,2,...,min_size}
  for (int i = 1; i < min_size; i++) {
    pattern[i] = pts_dists_aux[i - 1].point;
    erase_point(points, pattern[i]);
  }

  //  coleta os pontos mais próximos da reta do vetor inferor
  nearest_pts = nearest_points(vec_inf, reference, points, max_size - 1);
  pts_dists_aux = distance_points_ref(nearest_pts, reference);
  std::sort(pts_dists_aux.begin(), pts_dists_aux.end(), compare_distance);

  //  marca a linha 1,5,9,13,17,21...
  for (int i = 1; i < max_size; i++) {
    pattern[i * min_size] = pts_dists_aux[i - 1].point;
    erase_point(points, pattern[i * min_size]);
  }

  //  marca outros pontos
  for (int i = min_size; i < (min_size * max_size + 1); i = i + min_size) {
    nearest_pts = nearest_points(vec_sup, pattern[i], points, min_size - 1);
    if ((int)nearest_pts.size() == (min_size - 1)) {
      pts_dists_aux = distance_points_ref(nearest_pts, pattern[i]);
      std::sort(pts_dists_aux.begin(), pts_dists_aux.end(), compare_distance);
      for (int j = 0; j < (min_size - 1); j++) {
        pattern[j + 1 + i] = pts_dists_aux[j].point;
        erase_point(points, pattern[j + 1 + i]);
      }
    }
  }
  //  verifica se o padrão é válido
  double min_x = std::numeric_limits<double>::max();
  double min_y = std::numeric_limits<double>::max();
  double s_x_norm = 0;
  double s_y_norm = 0;
  double mean_x = 0;
  double mean_y = 0;
  double sum_sqr_x = 0;
  double sum_sqr_y = 0;
  double cur_norm;
  for (int i = 0; i < max_size; i++) {
    for (int j = 1; j < min_size; j++) {
      cur_norm = cv::norm(pattern[j + i * min_size] - pattern[j + i * min_size - 1]);
      mean_x = mean_x + cur_norm;
      sum_sqr_x = sum_sqr_x + cur_norm * cur_norm;

      if (cur_norm < min_x) {
        min_x = cur_norm;
      }
    }
  }
  for (int i = 0; i < min_size; i++) {
    for (int j = 1; j < max_size; j++) {
      cur_norm = cv::norm(pattern[i + j * min_size] - pattern[i + (j - 1) * min_size]);
      mean_y = mean_y + cur_norm;
      sum_sqr_y = sum_sqr_y + cur_norm * cur_norm;

      if (cur_norm < min_y) {
        min_y = cur_norm;
      }
    }
  }

  mean_x = mean_x / ((min_size - 1) * max_size);
  mean_y = mean_y / ((max_size - 1) * min_size);

  s_x_norm = std::sqrt(sum_sqr_x / ((min_size - 1) * max_size) - (mean_x * mean_x)) / min_x;
  s_y_norm = std::sqrt(sum_sqr_y / ((max_size - 1) * min_size) - (mean_y * mean_y)) / min_y;

  if ((s_x_norm < 0.1) && (s_y_norm < 0.1)) {
    points = pattern;
    return true;
  } else {
    points.clear();
    return false;
  }
}

cv::Point2d get_reference(std::vector<Contour> const& contours) {
  return (std::max_element(contours.begin(), contours.end(),
                           [](Contour const& a, Contour const& b) { return a.area < b.area; }))
      ->center;
}

std::vector<cv::Point2d> find_pattern(cv::Mat image, int const& m, int const& n) {
  const double AREA_MIN = 20;
  const double AREA_MAX = 500;
  const double AREA_RATIO_THR = 0.93;
  const double F_THR = 0.6;
  const double BIN_THR = 130.0;

  cv::Mat imbin, imblur;
  std::vector<std::vector<cv::Point>> initial_contours;
  std::vector<Contour> contours;
  cv::Point2d centroid, reference;
  std::vector<cv::Point2d> points;
  int n_circles = m * n;

  // Initial image processing
  if (image.channels() > 1) {
    cv::cvtColor(image, image, CV_BGR2GRAY);
  }
  cv::blur(image, imblur, cv::Size(3, 3));
  cv::threshold(imblur, imbin, BIN_THR, 255, cv::THRESH_BINARY);
  cv::dilate(imbin, imbin, cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(3, 3)));

  cv::findContours(imbin, initial_contours, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_NONE, cv::Point(0, 0));

  double A = 0;
  double A_x = 0;
  double A_y = 0;
  double a, b, f, area_p, area_d, area_ratio;
  cv::RotatedRect fit_e;

  for (auto& contour : initial_contours) {
    double contour_area = contourArea(contour);
    if (!((contour_area < AREA_MIN) || (contour_area > AREA_MAX) || (contour.size() < 5) ||
          (isContourConvex(contour)))) {
      fit_e = fitEllipse(contour);
      if (fit_e.size.height > fit_e.size.width) {
        a = fit_e.size.height / 2;
        b = fit_e.size.width / 2;
      } else {
        a = fit_e.size.width / 2;
        b = fit_e.size.height / 2;
      }
      f = (a - b) / a;
      area_p = contour_area;
      area_d = (CV_PI * a * b);
      area_ratio = std::min(area_p, area_d) / std::max(area_p, area_d);

      if ((area_ratio > AREA_RATIO_THR) && (f < F_THR)) {
        contours.push_back({contour, fit_e.center});

        A = A + area_p;
        A_x = A_x + area_p * (fit_e.center.x);
        A_y = A_y + area_p * (fit_e.center.y);
      }
    }
  }

  centroid.x = A_x / A;
  centroid.y = A_y / A;

  bool valid;
  if (static_cast<int>(contours.size()) >= n_circles) {
    for (auto& c : contours) {
      c.distance = cv::norm(c.center - centroid);
    }

    std::sort(contours.begin(), contours.end(),
              [](Contour const& a, Contour const& b) { return a.distance < b.distance; });

    int N = std::min(static_cast<int>(n_circles * 1.25), static_cast<int>(contours.size()));
    int K = n_circles;

    for (int i = 0; i < N; ++i) {
      cv::Moments m = cv::moments(contours[i].points, false);
      contours[i].area = m.m00;
    }

    std::vector<bool> bitmask(K, true);
    bitmask.resize(N, false);
    std::vector<Contour> cur_contours;
    valid = false;
    do {
      cur_contours.clear();
      for (int i = 0; i < N; ++i) {
        if (bitmask[i]) {
          cur_contours.push_back(contours[i]);
        }
      }

      points.clear();
      for (auto& cc : cur_contours) {
        points.push_back(cc.center);
      }
      reference = get_reference(cur_contours);
      valid = validate_pattern(points, reference, m, n);
    } while (std::prev_permutation(bitmask.begin(), bitmask.end()) && !valid);
  } else {
    valid = false;
  }
  if (!valid) {
    points.clear();
  }
  return points;
}

Pattern find_pattern(CompressedImage image) {
  cv::Mat frame = cv::imdecode(image.data, CV_LOAD_IMAGE_COLOR);
  auto vec_points = find_pattern(frame, 3, 4);
  Pattern pattern;
/*
  if (vec_points.empty()) {
    pattern.found = false;
  } else {
    for (auto& p : vec_points) {
      pattern.points.push_back({p.x, p.y});
    }
    pattern.found = true;
  }
*/
  if (!vec_points.empty()) {
    for (auto& p : vec_points) {
      pattern.points.push_back({p.x, p.y});
    }
  }
  return pattern;
}

#endif  // __FIND_PATTERN_HPP__