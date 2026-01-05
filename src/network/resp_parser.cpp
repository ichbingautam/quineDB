#include "resp_parser.hpp"

#include <algorithm>
#include <iostream>

namespace quine {
namespace network {

RespParser::RespParser() {
  reset();
}

void RespParser::reset() {
  state_ = State::WaitType;
  args_.clear();
  expected_args_ = 0;
  current_arg_len_ = 0;
  current_arg_.clear();
}

RespParser::Result RespParser::consume(const uint8_t* data, size_t len, size_t& consumed) {
  size_t pos = 0;

  while (pos < len) {
    switch (state_) {
      case State::WaitType: {
        // simple validation: expect '*' for array of bulk strings
        if (data[pos] != '*') {
          return Result::Error;
        }
        pos++;
        state_ = State::WaitSize;
        break;
      }
      case State::WaitSize: {
        // Read integer until \r\n
        // Simple inline integer parsing for now
        size_t start = pos;
        while (pos < len && data[pos] != '\r') pos++;

        if (pos == len) {
          consumed = start;
          return Result::Partial;
        }  // Wait for more

        // We parse the integer (skipping \n check for brevity, assumed valid if
        // \r found)
        std::string size_str(reinterpret_cast<const char*>(data + start), pos - start);
        try {
          expected_args_ = std::stoi(size_str);
        } catch (...) {
          return Result::Error;
        }

        if (pos + 1 < len && data[pos + 1] == '\n')
          pos += 2;  // skip \r\n
        else
          return Result::Partial;  // Split on \r\n? Unlikely but possible

        args_.reserve(expected_args_);
        state_ = State::WaitArgSize;  // Next is '$'
        break;
      }
      case State::WaitArgSize: {
        if (data[pos] != '$') return Result::Error;  // Expected bulk string marker
        pos++;

        size_t start = pos;
        while (pos < len && data[pos] != '\r') pos++;
        if (pos == len) {
          consumed = start - 1;
          return Result::Partial;
        }

        std::string len_str(reinterpret_cast<const char*>(data + start), pos - start);
        try {
          current_arg_len_ = std::stoi(len_str);
        } catch (...) {
          return Result::Error;
        }

        if (pos + 1 < len && data[pos + 1] == '\n')
          pos += 2;
        else
          return Result::Partial;

        current_arg_.clear();
        current_arg_.reserve(current_arg_len_);
        state_ = State::WaitArgData;
        break;
      }
      case State::WaitArgData: {
        size_t needed = current_arg_len_ - current_arg_.size();
        size_t available = len - pos;
        size_t to_copy = std::min(needed, available);

        current_arg_.append(reinterpret_cast<const char*>(data + pos), to_copy);
        pos += to_copy;

        if (current_arg_.size() == static_cast<size_t>(current_arg_len_)) {
          state_ = State::WaitCRLF;
        } else {
          consumed = len;  // Consumed all available data
          return Result::Partial;
        }
        break;
      }
      case State::WaitCRLF: {
        if (pos + 1 < len) {
          if (data[pos] == '\r' && data[pos + 1] == '\n') {
            pos += 2;
            args_.push_back(current_arg_);
            if (args_.size() == static_cast<size_t>(expected_args_)) {
              consumed = pos;
              return Result::Complete;
            } else {
              // More args to come
              state_ = State::WaitArgSize;
            }
          } else {
            return Result::Error;
          }
        } else {
          consumed = pos;
          return Result::Partial;
        }
        break;
      }
    }
  }

  consumed = pos;
  // If we are here, we ran out of data but aren't complete
  return Result::Partial;
}

}  // namespace network
}  // namespace quine
