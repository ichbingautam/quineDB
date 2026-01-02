#pragma once

namespace quine {
namespace core {

/// @brief Abstract base class for all async operations.
/// The address of this object is stored in io_uring_sqe::user_data.
struct Operation {
  virtual ~Operation() = default;

  /// @brief Called when the I/O operation completes.
  /// @param res The result of the operation (e.g., number of bytes read, or
  /// -errno).
  virtual void complete(int res) = 0;
};

} // namespace core
} // namespace quine
