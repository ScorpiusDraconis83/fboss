// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <array>
#include <chrono>
#include <mutex>
#include <vector>

#include "fboss/lib/usb/TransceiverAccessParameter.h"

#include "fboss/qsfp_service/if/gen-cpp2/qsfp_service_config_types.h"

namespace facebook::fboss {

/*
 * Class to log I2c transactions to a buffer. An instance of this class can
 * be created per transceiver to track latest <size_> transactions written
 * to the transceiver.
 * This class is designed as a circular buffer with the following properties:
 *     * Can hold up to <size_> entries.
 *     * The buffer is full when the head_ == tail_ and there are elements
 *       inserted.
 *     * Tail will track the oldest element still in buffer.
 *     * When dumping the contents of the buffer, the buffer is cleared.
 *     * The buffer is thread safe (logging/dumping)
 *     * The size of valid data in each buffer slot is:
 *       min(kMaxI2clogDataSize, param.len)
 */

constexpr int kMaxI2clogDataSize = 32;

class I2cLogBuffer {
  using TimePointSteady = std::chrono::steady_clock::time_point;
  using TimePointSystem = std::chrono::system_clock::time_point;

 public:
  enum class Operation {
    Read = 0,
    Write = 1,
  };

  // Log Entry.
  // Steady time required for replaying the sequence of transactions.
  // System time required to correlate to other qsfp_service logs
  struct I2cLogEntry {
    TimePointSteady steadyTime;
    TimePointSystem systemTime;
    TransceiverAccessParameter param;
    std::array<uint8_t, kMaxI2clogDataSize> data;
    Operation op;
    // Using default constructor for TransceiverAccessParameter when
    // initializing the buffer.
    I2cLogEntry() : param(TransceiverAccessParameter(0, 0, 0)) {}
  };

  struct I2cReplayEntry {
    TransceiverAccessParameter param;
    Operation op;
    std::array<uint8_t, kMaxI2clogDataSize> data;
    uint64_t delay;
    I2cReplayEntry(
        TransceiverAccessParameter param,
        Operation op,
        std::array<uint8_t, kMaxI2clogDataSize> data,
        uint64_t delay)
        : param(param), op(op), data(std::move(data)), delay(delay) {}
  };

  explicit I2cLogBuffer(cfg::TransceiverI2cLogging config, std::string logFile);

  // Insert a log entry into the buffer.
  void log(
      const TransceiverAccessParameter& param,
      const uint8_t* data,
      Operation op);

  // Dump the buffer contents into a vector of I2cLogEntry.
  // The vector (enriesOut) will be resized to the size of buffers,
  // and return value represents the number of valid entries dumped.
  // The log entries in the vector are from oldest to newest time.
  // Once the buffer is dumped, the contents of the circular buffer
  // will be cleared.
  // It is recommended that entriesOut is created with capacity <size_>
  // on the stack before calling this function (reduce allocation latency).
  size_t dump(std::vector<I2cLogEntry>& entriesOut);

  // Get the number of entries logged to the buffer. The size of the
  // buffer can be smaller than total entries logged.
  size_t getTotalEntries() const {
    return totalEntries_;
  }

  // Disable Logging for both read/write operations if
  // disableOnError is set in config.
  void transactionError();

  // Dumps the buffer contents into logFile_.
  // Format: Each log entry will be dumped into a single line.
  //         Month D HH:MM:SS.uuuuuu  <i2c_address  offset  len  page  bank  op>
  //         [data] steadyclock_ns
  // Returns a pair: header lines and number of log entries
  std::pair<size_t, size_t> dumpToFile();

  // Translate from a log file back to a vector of entries. Can be used
  // to replay the sequence of transactions or test the logging.
  static std::vector<I2cReplayEntry> loadFromLog(std::string logFile);

 private:
  std::vector<I2cLogEntry> buffer_;
  const size_t size_;
  cfg::TransceiverI2cLogging config_;
  size_t head_{0};
  size_t tail_{0};
  size_t totalEntries_{0};
  std::string logFile_;
  std::mutex mutex_;

  void getEntryTime(std::stringstream& ss, const TimePointSystem& time_point);

  // Operations to re-construct I2cReplayEntry from a log file.
  static size_t
  getHeader(std::stringstream& ss, size_t entries, size_t numContents);
  static std::string getField(const std::string& line, char left, char right);
  static TransceiverAccessParameter getParam(std::stringstream& ss);
  static I2cLogBuffer::Operation getOp(std::stringstream& ss);
  static std::array<uint8_t, kMaxI2clogDataSize> getData(std::string str);
  static uint64_t getDelay(const std::string& str);
};

} // namespace facebook::fboss
