#ifndef __IS_MSG_CV_HPP__
#define __IS_MSG_CV_HPP__

#include <opencv2/core.hpp>
#include "../packer.hpp"

namespace msgpack {

MSGPACK_API_VERSION_NAMESPACE(MSGPACK_DEFAULT_API_NS) {
  namespace adaptor {

  template <>
  struct convert<cv::Mat> {
    msgpack::object const& operator()(msgpack::object const& o, cv::Mat& mat) const {
      if (o.type != msgpack::type::ARRAY)
        throw msgpack::type_error();

      if (o.via.array.size != 4)
        throw msgpack::type_error();

      auto rows = o.via.array.ptr[0].as<int>();
      auto cols = o.via.array.ptr[1].as<int>();
      auto type = o.via.array.ptr[2].as<int>();
      auto data = o.via.array.ptr[3].as<std::vector<unsigned char>>();
      auto size = data.size();

      cv::Mat matref(rows, cols, type, data.data(), size / rows);
      mat = matref.clone();

      return o;
    }
  };

  template <>
  struct pack<cv::Mat> {
    template <typename Stream>
    packer<Stream>& operator()(msgpack::packer<Stream>& o, cv::Mat const& mat) const {
      o.pack_array(4);
      o.pack(mat.rows);
      o.pack(mat.cols);
      o.pack(mat.type());
      const std::vector<unsigned char> data(mat.datastart, mat.dataend);
      o.pack(data);
      return o;
    }
  };

  }  // ::adaptor

}  // MSGPACK_API_VERSION_NAMESPACE(MSGPACK_DEFAULT_API_NS)

}  // ::msgpack

#endif  // __IS_MSG_CV_HPP__