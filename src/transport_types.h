/**
 * @file transport_types.h
 *
 * @brief Generic definitions required to support multiple fabrics.
 *
 * This stuff cannot go in transport.h. Several classes (e.g., Session and
 * HugeAlloc) require these generic definitions, and these classes are in turn
 * required by the Transport class.
 */

#ifndef ERPC_TRANSPORT_TYPE_H
#define ERPC_TRANSPORT_TYPE_H

#include <strings.h>
#include <functional>
#include <string>
#include "common.h"

namespace ERpc {

static const size_t kMaxRoutingInfoSize = 128;  ///< Space for routing info
static const size_t kMaxMemRegInfoSize = 64;  ///< Space for memory registration

/// Generic struct to store routing info for any transport.
struct RoutingInfo {
  uint8_t buf[kMaxRoutingInfoSize];
};

/// Generic struct to store memory registration info for any transport.
struct MemRegInfo {
  void *transport_mr;
  uint32_t lkey;

  MemRegInfo(void *transport_mr, uint32_t lkey)
      : transport_mr(transport_mr), lkey(lkey) {}
};

/// Generic types for memory registration and deregistration functions.
typedef std::function<MemRegInfo(void *, size_t)> reg_mr_func_t;
typedef std::function<void(MemRegInfo)> dereg_mr_func_t;

enum class TransportType { kInfiniBand, kRoCE, kOmniPath, kInvalidTransport };

static std::string get_transport_name(TransportType transport_type) {
  switch (transport_type) {
    case TransportType::kInfiniBand:
      return std::string("[InfiniBand]");
    case TransportType::kRoCE:
      return std::string("[RoCE]");
    case TransportType::kOmniPath:
      return std::string("[OmniPath]");
    default:
      return std::string("[Invalid transport]");
  }
}
}  // End ERpc

#endif  // ERPC_TRANSPORT_TYPE_H
