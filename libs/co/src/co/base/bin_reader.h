#pragma once

// from github + modified

#include "co/common.h"

#include <string>
#include <cstring>
#include <cstdint>

namespace co {

class BinReader {
public:
  BinReader(const std::string& buf)
    : raw_(buf.c_str()), max_to_read_(buf.length()), cur_pos_(0)
  {
  }
  BinReader(const void* raw, size_t max_to_read)
    : raw_(raw), max_to_read_(max_to_read), cur_pos_(0)
  {
  }

  // Error handling code.
  // Notes:
  //  1) Errors are unrecoverable. New instance of BinReader must be
  // created. If there will be a need of stream parser, a new
  // class should be defined.
  //  2) get_parse_error() must be called only after read_XXX() has failed
  //  3) If read_XXX() is failed, current position isn't moved forward
  //
  enum parse_error {
    eParseErrorIncomplete,
    eParseErrorMalformed
  };

  parse_error GetParseError() const { return parse_err_; }
  // End of error handling code.

  size_t CurPos() const { return cur_pos_; }
  size_t BytesLeft() const { return max_to_read_ - cur_pos_; }

  template <typename PodType>
  bool ReadFixedNumberOfPods(PodType* pods, uint32_t count) {
    size_t need_bytes = count * sizeof(PodType);
    if (BytesLeft() < need_bytes) {
      parse_err_ = eParseErrorIncomplete;
      return false;
    }
    std::memcpy(pods, CurPtr<char>(), need_bytes);
    Advance(need_bytes);
    return true;
  }

  template <typename PodType, typename SizeType>
  bool ReadPodArray(const PodType*& ppods, SizeType* pcount) {
    SizeType count;
    if (!ReadFixedNumberOfPods(&count, 1)) {
      // |parse_stat_| was set by read_fixed_number_of_pods()
      return false;
    }
    size_t need_bytes = count * sizeof(PodType);
    if (BytesLeft() < need_bytes) {
      parse_err_ = eParseErrorIncomplete;
      Rollback(sizeof(SizeType));
      return false;
    }
    *pcount = count;
    ppods = CurPtr<PodType>();
    Advance(need_bytes);
    return true;
  }

  bool ReadInt64(int64_t& v) { return ReadFixedNumberOfPods(&v, 1); }
  bool ReadUint64(uint64_t& v) { return ReadFixedNumberOfPods(&v, 1); }
  bool ReadInt32(int32_t& v) { return ReadFixedNumberOfPods(&v, 1); }
  bool ReadUint32(uint32_t& v) { return ReadFixedNumberOfPods(&v, 1); }
  bool ReadInt16(int16_t& v) { return ReadFixedNumberOfPods(&v, 1); }
  bool ReadUint16(uint16_t& v) { return ReadFixedNumberOfPods(&v, 1); }
  bool ReadInt8(int8_t& v) { return ReadFixedNumberOfPods(&v, 1); }
  bool ReadUint8(uint8_t& v) { return ReadFixedNumberOfPods(&v, 1); }

  bool ReadByteArrayCastSize(const uint8_t*& pptr, size_t* psize) {
    uint32_t sz;
    if (!ReadByteArray(pptr, &sz)) {
      return false;
    }
    *psize = sz;
    return true;
  }
  bool ReadByteArray(const uint8_t*& pptr, uint32_t* psize) {
    return ReadPodArray(pptr, psize);
  }
  bool ReadByteArray16(const uint8_t*& pptr, uint16_t* psize) {
    return ReadPodArray(pptr, psize);
  }
  bool ReadString(std::string& val, bool append = false);
  bool ReadString(std::wstring& val, bool append = false);

private:
  template <typename T>
  const T* CurPtr() {
    return static_cast<const T*>(raw_) + cur_pos_;
  }

  void Advance(size_t num_bytes) {
    cur_pos_ += num_bytes;
  }

  void Rollback(size_t num_bytes) {
    cur_pos_ -= num_bytes;
  }

  bool SetFromData(uint32_t hash, int wfeval, Errcode& e);

private:
  const void* raw_;
  size_t max_to_read_;
  size_t cur_pos_;
  parse_error parse_err_;
};

}
