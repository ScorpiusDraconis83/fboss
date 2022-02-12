// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/qsfp_service/module/tests/FakeTransceiverImpl.h"
#include "fboss/lib/usb/TransceiverI2CApi.h"
#include "fboss/qsfp_service/module/QsfpModule.h"

#include <gtest/gtest.h>

namespace {
// Create a copy of the lower page that's passed in, and set the module ID byte
template <typename ArrayT, size_t MemberCount = std::extent<ArrayT>::value>
ArrayT customizeModuleIdentifier(ArrayT base, uint8_t newIdentifier) {
  base[0] = newIdentifier;
  return base;
}
} // namespace

namespace facebook {
namespace fboss {

bool FakeTransceiverImpl::detectTransceiver() {
  return true;
}

int FakeTransceiverImpl::readTransceiver(
    int dataAddress,
    int offset,
    int len,
    uint8_t* fieldValue) {
  int read = 0;
  EXPECT_TRUE(dataAddress == 0x50 || dataAddress == 0x51);

  if (offset < QsfpModule::MAX_QSFP_PAGE_SIZE) {
    read = len;
    if (QsfpModule::MAX_QSFP_PAGE_SIZE - offset < len) {
      read = QsfpModule::MAX_QSFP_PAGE_SIZE - offset;
    }
    std::copy(
        lowerPages_[dataAddress].begin() + offset,
        lowerPages_[dataAddress].begin() + offset + read,
        fieldValue);
    len -= read;
    offset = QsfpModule::MAX_QSFP_PAGE_SIZE;
  }
  if (len > 0 && offset >= QsfpModule::MAX_QSFP_PAGE_SIZE) {
    offset -= QsfpModule::MAX_QSFP_PAGE_SIZE;
    EXPECT_LE(len + offset, QsfpModule::MAX_QSFP_PAGE_SIZE);
    assert(
        upperPages_[dataAddress].find(page_) != upperPages_[dataAddress].end());
    std::copy(
        upperPages_[dataAddress][page_].begin() + offset,
        upperPages_[dataAddress][page_].begin() + offset + len,
        fieldValue + read);
    read += len;
  }
  return read;
}

int FakeTransceiverImpl::writeTransceiver(
    int dataAddress,
    int offset,
    int len,
    uint8_t* fieldValue) {
  if (offset == 127) {
    page_ = *fieldValue;
  }
  if (offset < QsfpModule::MAX_QSFP_PAGE_SIZE) {
    EXPECT_LE(offset + len, QsfpModule::MAX_QSFP_PAGE_SIZE);
    std::copy(
        fieldValue,
        fieldValue + len,
        lowerPages_[dataAddress].begin() + offset);
  } else {
    EXPECT_LE(offset + len, 2 * QsfpModule::MAX_QSFP_PAGE_SIZE);
    offset -= QsfpModule::MAX_QSFP_PAGE_SIZE;
    std::copy(
        fieldValue,
        fieldValue + len,
        upperPages_[dataAddress][page_].begin() + offset);
  }

  return len;
}

folly::StringPiece FakeTransceiverImpl::getName() {
  return moduleName_;
}

int FakeTransceiverImpl::getNum() const {
  return module_;
}

// Below are randomly generated eeprom maps for testing purpose and doesn't
// accurately reflect an actual module

std::array<uint8_t, 128> kSffDacPageLowerA0 = {
    0x11, 0x07, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};
std::map<uint8_t, std::array<uint8_t, 128>> kSffDacPageLower = {
    {TransceiverI2CApi::ADDR_QSFP, kSffDacPageLowerA0},
};

std::array<uint8_t, 128> kSffDacPage0 = {
    0x11, 0x00, 0x23, 0x88, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05,
    0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0xa0, 0x46, 0x41, 0x43, 0x45,
    0x54, 0x45, 0x53, 0x54, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
    0x00, 0x78, 0xa7, 0x14, 0x4e, 0x44, 0x41, 0x51, 0x47, 0x46, 0x2d, 0x46,
    0x33, 0x30, 0x32, 0x20, 0x20, 0x20, 0x20, 0x20, 0x41, 0x20, 0x05, 0x07,
    0x09, 0x0d, 0xff, 0xff, 0x0b, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0x32, 0x30, 0x32, 0x33, 0x4a, 0x20, 0x20,
    0x31, 0x37, 0x30, 0x37, 0x32, 0x33, 0x20, 0x20, 0x00, 0x00, 0x67, 0x44,
    0x02, 0x00, 0x30, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
};

std::array<uint8_t, 128> kSffCwdm4PageLowerA0 = {
    0x0d, 0x00, 0x02, 0xab, 0xbc, 0xcd, 0xc0, 0x30, 0x00, 0xc5, 0xf3, 0xc5,
    0x3c, 0xc5, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1f, 0x04,
    0x00, 0x00, 0x80, 0xdd, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x12, 0x23,
    0x23, 0x34, 0x13, 0x31, 0x12, 0x12, 0x14, 0x14, 0x11, 0x11, 0x22, 0x22,
    0x33, 0x33, 0x41, 0x41, 0x42, 0x42, 0x43, 0x43, 0x44, 0x44, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x0e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00,
};
std::map<uint8_t, std::array<uint8_t, 128>> kSffCwdm4PageLower{
    {TransceiverI2CApi::ADDR_QSFP, kSffCwdm4PageLowerA0},
};

// Miniphoton OBO are SFF CWDM4 optics w/ a module identifier of 0x91
std::array<uint8_t, 128> kMiniphotonOBOPageLowerA0 =
    customizeModuleIdentifier(kSffCwdm4PageLowerA0, 0x91);
std::map<uint8_t, std::array<uint8_t, 128>> kMiniphotonOBOPageLower = {
    {TransceiverI2CApi::ADDR_QSFP, kMiniphotonOBOPageLowerA0},
};

// use an invalid module ID (not defined in TransceiverModuleIdentifier)
std::array<uint8_t, 128> kUnknownSffModuleIdentifierPageLowerA0 =
    customizeModuleIdentifier(kSffCwdm4PageLowerA0, 0xFF);
std::map<uint8_t, std::array<uint8_t, 128>>
    kUnknownSffModuleIdentifierPageLower = {
        {TransceiverI2CApi::ADDR_QSFP, kUnknownSffModuleIdentifierPageLowerA0},
};

std::array<uint8_t, 128> kSffCwdm4BadPageLowerA0 = {
    0x0d, 0xff, 0xff, 0x0c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1f, 0x04,
    0x00, 0x00, 0x80, 0xdd, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00,
};
std::map<uint8_t, std::array<uint8_t, 128>> kSffCwdm4BadPageLower = {
    {TransceiverI2CApi::ADDR_QSFP, kSffCwdm4BadPageLowerA0},
};

std::array<uint8_t, 128> kSffCwdm4Page0 = {
    0x0d, 0x10, 0x0c, 0x04, 0x00, 0x00, 0x00, 0x40, 0x40, 0x02, 0x00, 0x05,
    0x67, 0x00, 0x00, 0x32, 0x00, 0x00, 0x00, 0x00, 0x46, 0x41, 0x43, 0x45,
    0x54, 0x45, 0x53, 0x54, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
    0x07, 0x00, 0x00, 0x00, 0x46, 0x54, 0x4c, 0x34, 0x31, 0x30, 0x51, 0x45,
    0x32, 0x43, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x41, 0x20, 0x42, 0x68,
    0x07, 0xd0, 0x46, 0x97, 0x06, 0x01, 0x04, 0xd0, 0x4d, 0x52, 0x45, 0x30,
    0x31, 0x42, 0x30, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
    0x31, 0x34, 0x30, 0x35, 0x30, 0x32, 0x20, 0x20, 0x0a, 0x00, 0x00, 0x22,
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
    0x20, 0x20, 0x20, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

std::array<uint8_t, 128> kSffCwdm4Page3 = {
    0x4b, 0x00, 0xfb, 0x00, 0x46, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x94, 0x70, 0x6e, 0xf0, 0x86, 0xc4, 0x7b, 0x0c,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x11, 0x12, 0x00, 0x11, 0x22, 0x33,
    0x44, 0x55, 0x66, 0x77, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x11, 0x11, 0x00, 0x00, 0x00, 0x05, 0x00, 0x00, 0x12, 0x34,
    0x21, 0x13, 0x13, 0x23, 0x2c, 0x3d, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

// Using the same dummy eeprom map for 100G-FR1 as 100G-CWDM4 except the
// extended specification compliance change in byte 192 in upper page 0
std::array<uint8_t, 128> kSffFr1PageLowerA0 = kSffCwdm4PageLowerA0;
std::map<uint8_t, std::array<uint8_t, 128>> kSffFr1PageLower = {
    {TransceiverI2CApi::ADDR_QSFP, kSffFr1PageLowerA0},
};
std::array<uint8_t, 128> kSffFr1Page0 = {
    0x0d, 0x10, 0x0c, 0x04, 0x00, 0x00, 0x00, 0x40, 0x40, 0x02, 0x00, 0x05,
    0x67, 0x00, 0x00, 0x32, 0x00, 0x00, 0x00, 0x00, 0x46, 0x41, 0x43, 0x45,
    0x54, 0x45, 0x53, 0x54, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
    0x07, 0x00, 0x00, 0x00, 0x46, 0x54, 0x4c, 0x34, 0x31, 0x30, 0x51, 0x45,
    0x32, 0x43, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x41, 0x20, 0x42, 0x68,
    0x07, 0xd0, 0x46, 0x97, 0x26, 0x01, 0x04, 0xd0, 0x4d, 0x52, 0x45, 0x30,
    0x31, 0x42, 0x30, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
    0x31, 0x34, 0x30, 0x35, 0x30, 0x32, 0x20, 0x20, 0x0a, 0x00, 0x00, 0x22,
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
    0x20, 0x20, 0x20, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};
std::array<uint8_t, 128> kSffFr1Page3 = {
    0x4b, 0x00, 0xfb, 0x00, 0x46, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x94, 0x70, 0x6e, 0xf0, 0x86, 0xc4, 0x7b, 0x0c,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x11, 0x12, 0x00, 0x11, 0x22, 0x33,
    0x44, 0x55, 0x66, 0x77, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x11, 0x11, 0x00, 0x00, 0x00, 0x7f, 0x00, 0x00, 0x12, 0x34,
    0x21, 0x13, 0x13, 0x23, 0x2c, 0x3d, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

std::array<uint8_t, 128> kCmis200GFr4LowerA0 = {
    0x1e, 0x40, 0x00, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x5b, 0x00, 0x00,
    0x00, 0x00, 0x28, 0x45, 0x80, 0xfc, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x02, 0x07, 0x00, 0x00, 0x00, 0x01, 0x6f, 0x00, 0x00,
    0x00, 0x66, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x02, 0x0f, 0x18, 0x44, 0x01, 0x0b, 0x10, 0x44, 0x01, 0xff, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x14};
std::map<uint8_t, std::array<uint8_t, 128>> kCmis200GFr4Lower = {
    {TransceiverI2CApi::ADDR_QSFP, kCmis200GFr4LowerA0},
};

std::array<uint8_t, 128> kCmis200GFr4Page0 = {
    0x1e, 0x46, 0x41, 0x43, 0x45, 0x54, 0x45, 0x53, 0x54, 0x20, 0x20, 0x20,
    0x20, 0x20, 0x20, 0x20, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

std::array<uint8_t, 128> kCmis200GFr4Page1 = {
    0x00, 0x00, 0x03, 0x00, 0x1e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x65, 0xa4,
    0x1b, 0xee, 0x20, 0xdf, 0x57, 0x00, 0x41, 0x0f, 0x00, 0x00, 0x9d, 0x38,
    0x00, 0xf0, 0x77, 0x2f, 0x07, 0x07, 0x06, 0x03, 0x07, 0x09, 0x3d, 0x60,
    0x0f, 0x9f, 0x80, 0x37, 0x37, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x0a, 0x03, 0x65, 0x01, 0x0a,
    0x03, 0x1a, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x41};

std::array<uint8_t, 128> kCmis200GFr4Page2 = {
    0x46, 0x00, 0x0a, 0x00, 0x46, 0x00, 0x0a, 0x00, 0x8b, 0x42, 0x76, 0x8e,
    0x8b, 0x42, 0x76, 0x8e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xb6, 0xb6, 0x07, 0x71, 0xb6, 0xb6, 0x07, 0x71, 0x92, 0x7c, 0x1d, 0x4c,
    0x92, 0x7c, 0x1d, 0x4c, 0xb6, 0xb6, 0x00, 0x01, 0xb6, 0xb6, 0x00, 0x01,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xd2};

std::array<uint8_t, 128> kCmis200GFr4Page10 = {
    0x06, 0x00, 0x0a, 0x0b, 0x0c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x0c,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

std::array<uint8_t, 128> kCmis200GFr4Page11 = {
    0x24, 0x35, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0a, 0x0b, 0x0c, 0x0d, 0x02,
    0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x05, 0x06, 0x0a, 0x0b, 0x0c,
    0x0d, 0x00, 0x53, 0x34, 0x51, 0x0c, 0x50, 0x20, 0x52, 0x23, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x5e, 0x9d, 0x61, 0xd1, 0x68, 0x86,
    0x61, 0xb6, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0xc0,
    0x0f, 0x81, 0x16, 0xb4, 0x14, 0x38, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x10, 0x10, 0x10, 0x08, 0x08,
    0x08, 0x08, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0x22,
    0x22, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x33, 0x33, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x11, 0x21, 0x31, 0x41, 0x00, 0x00, 0x00, 0x00,
    0x11, 0x21, 0x31, 0x41, 0x00, 0x00, 0x00, 0x00};

std::array<uint8_t, 128> kCmis200GFr4Page13 = {
    0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

std::array<uint8_t, 128> kCmis400GLr4LowerA0 = {
    0x18, 0x40, 0x04, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x2d, 0x14, 0x7f, 0x42, 0x00, 0x0b, 0x37, 0x20, 0x00, 0x00,
    0x00, 0x00, 0x20, 0x00, 0x00, 0x02, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x02, 0x0a, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff,
    0xff, 0x02, 0x11, 0x1e, 0x84, 0x01, 0x0f, 0x18, 0x44, 0x01, 0x11, 0xc0,
    0x84, 0x01, 0x11, 0xc1, 0x84, 0x01, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x10, 0x11, 0x00, 0x00, 0x00, 0x00, 0x00, 0x25};
std::map<uint8_t, std::array<uint8_t, 128>> kCmis400GLr4Lower = {
    {TransceiverI2CApi::ADDR_QSFP, kCmis400GLr4LowerA0},
};

std::array<uint8_t, 128> kCmis400GLr4Page0 = {
    0x18, 0x46, 0x41, 0x43, 0x45, 0x54, 0x45, 0x53, 0x54, 0x20, 0x20, 0x20,
    0x20, 0x20, 0x20, 0x20, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xa0, 0x30, 0x00, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf0, 0x00,
    0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x11, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

std::array<uint8_t, 128> kCmis400GLr4Page1 = {
    0x00, 0x00, 0x00, 0x01, 0x4a, 0x00, 0x00, 0x00, 0x00, 0x00, 0x65, 0xa4,
    0x04, 0x7e, 0x64, 0xf0, 0x27, 0x81, 0x46, 0x00, 0x00, 0x00, 0x9d, 0x18,
    0x00, 0xf9, 0x77, 0x3f, 0x07, 0x07, 0x06, 0x0f, 0x07, 0x0d, 0x3d, 0x60,
    0x0f, 0x9f, 0x80, 0x26, 0x11, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x0a, 0x03, 0xf5, 0x01, 0x0a,
    0x03, 0x93, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x5d};

std::array<uint8_t, 128> kCmis400GLr4Page2 = {
    0x4b, 0x00, 0x0a, 0x00, 0x4b, 0x00, 0x0a, 0x00, 0x8b, 0x42, 0x76, 0x8e,
    0x8b, 0x42, 0x76, 0x8e, 0x3f, 0xff, 0xc0, 0x00, 0x3f, 0xff, 0xc0, 0x00,
    0x41, 0x00, 0x23, 0x00, 0x3c, 0x00, 0x28, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xae, 0x7c, 0x0e, 0x2f, 0xae, 0x7c, 0x0e, 0x2f, 0xfd, 0xe8, 0x3a, 0x98,
    0xd6, 0xd8, 0x44, 0x5c, 0xae, 0x7c, 0x00, 0x01, 0xae, 0x7c, 0x00, 0x01,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x39};

std::array<uint8_t, 128> kCmis400GLr4Page10 = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
    0x11, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0x44, 0x44,
    0x44, 0x44, 0x00, 0x00, 0x00, 0x00, 0x33, 0x33, 0x33, 0x33, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
    0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0x22, 0x22, 0x22,
    0x22, 0x00, 0x00, 0x00, 0x00, 0x33, 0x33, 0x33, 0x33, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

std::array<uint8_t, 128> kCmis400GLr4Page11 = {
    0x44, 0x44, 0x44, 0x44, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x4b, 0x35, 0x4e, 0x72, 0x51, 0x25, 0x4d, 0x5d, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x88, 0xd0, 0x84, 0xcb, 0x7f, 0x12,
    0x84, 0x84, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x42, 0xe5,
    0x43, 0x2f, 0x46, 0xee, 0x53, 0x32, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
    0x11, 0x11, 0xff, 0x00, 0x00, 0x34, 0x36, 0x74, 0x54, 0xff, 0xff, 0x44,
    0x44, 0x44, 0x44, 0x00, 0x00, 0x00, 0x00, 0x33, 0x33, 0x33, 0x33, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x11, 0x21, 0x31, 0x41, 0x00, 0x00, 0x00, 0x00,
    0x11, 0x21, 0x31, 0x41, 0x00, 0x00, 0x00, 0x00};

std::array<uint8_t, 128> kCmis400GLr4Page13 = {
    0x0a, 0xfc, 0x23, 0x66, 0x55, 0x15, 0x55, 0x15, 0x55, 0x05, 0x55, 0x05,
    0x00, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

std::array<uint8_t, 128> kCmis400GLr4Page14 = {
    0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x1b, 0x24, 0x93, 0x40, 0x69, 0x40, 0x84, 0x36, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x89, 0x15, 0xa0, 0x16, 0x0c, 0x16, 0x6a, 0x15,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

std::array<uint8_t, 128> kCmis400GLr4Page20 = {
    0x00, 0x05, 0x01, 0x05, 0x02, 0x05, 0x03, 0x05, 0x10, 0x07, 0x11, 0x07,
    0x12, 0x07, 0x13, 0x07, 0x2f, 0x09, 0x2f, 0x0b, 0x2f, 0x0d, 0x2f, 0x0f,
    0x3f, 0x11, 0x3f, 0x13, 0x3f, 0x15, 0x3f, 0x17, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

std::array<uint8_t, 128> kCmis400GLr4Page21 = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x2f, 0x0a, 0x2f, 0x0c,
    0x2f, 0x0e, 0x2f, 0x10, 0x3f, 0x12, 0x3f, 0x14, 0x3f, 0x16, 0x3f, 0x18,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

std::array<uint8_t, 128> kCmis400GLr4Page24 = {
    0x15, 0x30, 0x16, 0xf3, 0x16, 0x0c, 0x15, 0x6a, 0x27, 0xe7, 0x40, 0x96,
    0xff, 0xff, 0x38, 0x1a, 0x71, 0xef, 0x74, 0x24, 0x72, 0xe1, 0x72, 0xe6,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

std::array<uint8_t, 128> kCmis400GLr4Page25 = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x8a, 0x37,
    0x6b, 0x54, 0x6e, 0xcd, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

std::array<uint8_t, 128> kCmisFlatMemLowerA0 = {
    0x18, 0x40, 0x80, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x40, 0x01, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
std::map<uint8_t, std::array<uint8_t, 128>> kCmisFlatMemLower = {
    {TransceiverI2CApi::ADDR_QSFP, kCmisFlatMemLowerA0},
};

std::array<uint8_t, 128> kCmisFlatMemPage0 = {
    0x18, 0x46, 0x41, 0x43, 0x45, 0x54, 0x45, 0x53, 0x54, 0x20, 0x20, 0x20,
    0x20, 0x20, 0x20, 0x20, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
    0x00, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0x00,
    0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x82, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x70, 0x7b, 0x93, 0x0a};

std::map<int, std::array<uint8_t, 128>> kSffCwdm4UpperPagesA0 = {
    {0, kSffCwdm4Page0},
    {3, kSffCwdm4Page3}};
std::map<uint8_t, std::map<int, std::array<uint8_t, 128>>> kSffCwdm4UpperPages =
    {{TransceiverI2CApi::ADDR_QSFP, kSffCwdm4UpperPagesA0}};

std::map<int, std::array<uint8_t, 128>> kSffFr1UpperPagesA0 = {
    {0, kSffFr1Page0},
    {3, kSffFr1Page3}};
std::map<uint8_t, std::map<int, std::array<uint8_t, 128>>> kSffFr1UpperPages = {
    {TransceiverI2CApi::ADDR_QSFP, kSffFr1UpperPagesA0}};

std::map<int, std::array<uint8_t, 128>> kSffDacUpperPagesA0 = {
    {0, kSffDacPage0}};
std::map<uint8_t, std::map<int, std::array<uint8_t, 128>>> kSffDacUpperPages = {
    {TransceiverI2CApi::ADDR_QSFP, kSffDacUpperPagesA0}};

std::map<int, std::array<uint8_t, 128>> kCmis200GFr4UpperPagesA0 = {
    {0, kCmis200GFr4Page0},
    {1, kCmis200GFr4Page1},
    {2, kCmis200GFr4Page2},
    {0x10, kCmis200GFr4Page10},
    {0x11, kCmis200GFr4Page11},
    {0x13, kCmis200GFr4Page13}};
std::map<uint8_t, std::map<int, std::array<uint8_t, 128>>>
    kCmis200GFr4UpperPages = {
        {TransceiverI2CApi::ADDR_QSFP, kCmis200GFr4UpperPagesA0}};

std::map<int, std::array<uint8_t, 128>> kCmis400GLr4UpperPagesA0 = {
    {0, kCmis400GLr4Page0},
    {1, kCmis400GLr4Page1},
    {2, kCmis400GLr4Page2},
    {0x10, kCmis400GLr4Page10},
    {0x11, kCmis400GLr4Page11},
    {0x13, kCmis400GLr4Page13},
    {0x14, kCmis400GLr4Page14},
    {0x20, kCmis400GLr4Page20},
    {0x21, kCmis400GLr4Page21},
    {0x24, kCmis400GLr4Page24},
    {0x25, kCmis400GLr4Page25},
};
std::map<uint8_t, std::map<int, std::array<uint8_t, 128>>>
    kCmis400GLr4UpperPages = {
        {TransceiverI2CApi::ADDR_QSFP, kCmis400GLr4UpperPagesA0}};

std::map<int, std::array<uint8_t, 128>> kCmisFlatMemUpperPagesA0 = {
    {0, kCmisFlatMemPage0},
};
std::map<uint8_t, std::map<int, std::array<uint8_t, 128>>>
    kCmisFlatMemUpperPages = {
        {TransceiverI2CApi::ADDR_QSFP, kCmisFlatMemUpperPagesA0}};

std::array<uint8_t, 128> kSfpLowerPageA0 = {
    0x03, 0x04, 0x07, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06,
    0x67, 0x00, 0x00, 0x00, 0x08, 0x03, 0x00, 0x1e, 0x46, 0x41, 0x43, 0x45,
    0x54, 0x45, 0x53, 0x54, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
    0x00, 0x00, 0x90, 0x65, 0x46, 0x54, 0x4c, 0x58, 0x38, 0x35, 0x37, 0x31,
    0x44, 0x33, 0x42, 0x43, 0x4c, 0x20, 0x20, 0x20, 0x41, 0x20, 0x20, 0x20,
    0x03, 0x52, 0x00, 0x48, 0x00, 0x1a, 0x00, 0x00, 0x41, 0x52, 0x4c, 0x30,
    0x4b, 0x58, 0x36, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
    0x31, 0x34, 0x30, 0x35, 0x31, 0x37, 0x20, 0x20, 0x68, 0xf0, 0x03, 0xef,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

std::array<uint8_t, 128> kSfpLowerPageA2 = {
    0x03, 0x04, 0x07, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06,
    0x67, 0x00, 0x00, 0x00, 0x08, 0x03, 0x00, 0x1e, 0x46, 0x49, 0x4e, 0x49,
    0x53, 0x41, 0x52, 0x20, 0x43, 0x4f, 0x52, 0x50, 0x2e, 0x20, 0x20, 0x20,
    0x00, 0x00, 0x90, 0x65, 0x46, 0x54, 0x4c, 0x58, 0x38, 0x35, 0x37, 0x31,
    0x44, 0x33, 0x42, 0x43, 0x4c, 0x20, 0x20, 0x20, 0x41, 0x20, 0x20, 0x20,
    0x03, 0x52, 0x00, 0x48, 0x00, 0x1a, 0x00, 0x00, 0x41, 0x52, 0x4c, 0x30,
    0x4b, 0x58, 0x36, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
    0x31, 0x34, 0x30, 0x35, 0x31, 0x37, 0x20, 0x20, 0x68, 0xf0, 0x03, 0xef,
    0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0x11, 0x22, 0x33, 0x00, 0x00,
    0x00, 0x00, 0x86, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

std::map<uint8_t, std::array<uint8_t, 128>> kSfpLowerPages = {
    {TransceiverI2CApi::ADDR_QSFP, kSfpLowerPageA0},
    {TransceiverI2CApi::ADDR_QSFP_A2, kSfpLowerPageA2},
};
std::map<uint8_t, std::map<int, std::array<uint8_t, 128>>> kSfpUpperPages = {};

SffDacTransceiver::SffDacTransceiver(int module)
    : FakeTransceiverImpl(module, kSffDacPageLower, kSffDacUpperPages) {}

SffCwdm4Transceiver::SffCwdm4Transceiver(int module)
    : FakeTransceiverImpl(module, kSffCwdm4PageLower, kSffCwdm4UpperPages) {}

SffFr1Transceiver::SffFr1Transceiver(int module)
    : FakeTransceiverImpl(module, kSffFr1PageLower, kSffFr1UpperPages) {}

BadSffCwdm4Transceiver::BadSffCwdm4Transceiver(int module)
    : FakeTransceiverImpl(module, kSffCwdm4BadPageLower, kSffCwdm4UpperPages) {}

MiniphotonOBOTransceiver::MiniphotonOBOTransceiver(int module)
    : FakeTransceiverImpl(
          module,
          kMiniphotonOBOPageLower,
          kSffCwdm4UpperPages) {}

UnknownModuleIdentifierTransceiver::UnknownModuleIdentifierTransceiver(
    int module)
    : FakeTransceiverImpl(
          module,
          kUnknownSffModuleIdentifierPageLower,
          kSffCwdm4UpperPages) {}

Cmis200GTransceiver::Cmis200GTransceiver(int module)
    : FakeTransceiverImpl(module, kCmis200GFr4Lower, kCmis200GFr4UpperPages) {}

Cmis400GLr4Transceiver::Cmis400GLr4Transceiver(int module)
    : FakeTransceiverImpl(module, kCmis400GLr4Lower, kCmis400GLr4UpperPages) {}

CmisFlatMemTransceiver::CmisFlatMemTransceiver(int module)
    : FakeTransceiverImpl(module, kCmisFlatMemLower, kCmisFlatMemUpperPages) {}

Sfp10GTransceiver::Sfp10GTransceiver(int module)
    : FakeTransceiverImpl(module, kSfpLowerPages, kSfpUpperPages) {}
} // namespace fboss
} // namespace facebook
