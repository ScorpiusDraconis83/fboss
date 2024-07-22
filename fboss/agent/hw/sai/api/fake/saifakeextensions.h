// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include <sai.h>

typedef enum _sai_port_serdes_extensions_attr_t {
  SAI_PORT_SERDES_ATTR_EXT_FAKE_RX_CTLE_CODE =
      SAI_PORT_SERDES_ATTR_CUSTOM_RANGE_START,
  SAI_PORT_SERDES_ATTR_EXT_FAKE_RX_DSP_MODE,
  SAI_PORT_SERDES_ATTR_EXT_FAKE_RX_AFE_TRIM,
  SAI_PORT_SERDES_ATTR_EXT_FAKE_RX_AC_COUPLING_BYPASS,
  SAI_PORT_SERDES_ATTR_EXT_FAKE_RX_AFE_ADAPTIVE_ENABLE,
  SAI_PORT_SERDES_ATTR_EXT_RX_CHANNEL_REACH,
  SAI_PORT_SERDES_ATTR_EXT_RX_DIFF_ENCODER_EN,
  SAI_PORT_SERDES_ATTR_EXT_RX_FBF_COEF_INIT_VAL,
  SAI_PORT_SERDES_ATTR_EXT_RX_FBF_LMS_ENABLE,
  SAI_PORT_SERDES_ATTR_EXT_RX_INSTG_SCAN_OPTIMIZE,
  SAI_PORT_SERDES_ATTR_EXT_RX_INSTG_TABLE_END_ROW,
  SAI_PORT_SERDES_ATTR_EXT_RX_INSTG_TABLE_START_ROW,
  SAI_PORT_SERDES_ATTR_EXT_RX_PARITY_ENCODER_EN,
  SAI_PORT_SERDES_ATTR_EXT_RX_THP_EN,
  SAI_PORT_SERDES_ATTR_EXT_TX_DIFF_ENCODER_EN,
  SAI_PORT_SERDES_ATTR_EXT_TX_DIG_GAIN,
  SAI_PORT_SERDES_ATTR_EXT_TX_FFE_COEFF_0,
  SAI_PORT_SERDES_ATTR_EXT_TX_FFE_COEFF_1,
  SAI_PORT_SERDES_ATTR_EXT_TX_FFE_COEFF_2,
  SAI_PORT_SERDES_ATTR_EXT_TX_FFE_COEFF_3,
  SAI_PORT_SERDES_ATTR_EXT_TX_FFE_COEFF_4,
  SAI_PORT_SERDES_ATTR_EXT_TX_PARITY_ENCODER_EN,
  SAI_PORT_SERDES_ATTR_EXT_TX_THP_EN,
  SAI_PORT_SERDES_ATTR_EXT_TX_LUT_MODE,
} sai_port_serdes_extensions_attr_t;

typedef enum _sai_switch_extensions_attr_t {
  SAI_SWITCH_ATTR_EXT_FAKE_LED = SAI_SWITCH_ATTR_END,
  SAI_SWITCH_ATTR_EXT_FAKE_LED_RESET,
  SAI_SWITCH_ATTR_EXT_FAKE_ACL_FIELD_LIST,
  SAI_SWITCH_ATTR_DEFAULT_EGRESS_BUFFER_POOL_SHARED_SIZE,
  SAI_SWITCH_ATTR_EXT_FAKE_HW_ECC_ERROR_INITIATE,
  SAI_SWITCH_ATTR_ISSU_CUSTOM_DLL_PATH,
  SAI_SWITCH_ATTR_EXT_RESTART_ISSU,
  SAI_SWITCH_ATTR_FORCE_TRAFFIC_OVER_FABRIC,
  SAI_SWITCH_ATTR_EXT_WARM_BOOT_TARGET_VERSION,
  SAI_SWITCH_ATTR_FABRIC_REMOTE_REACHABLE_PORT_LIST,
  SAI_SWITCH_ATTR_VOQ_LATENCY_MIN_LOCAL,
  SAI_SWITCH_ATTR_VOQ_LATENCY_MAX_LOCAL,
  SAI_SWITCH_ATTR_VOQ_LATENCY_MIN_LEVEL_1,
  SAI_SWITCH_ATTR_VOQ_LATENCY_MAX_LEVEL_1,
  SAI_SWITCH_ATTR_VOQ_LATENCY_MIN_LEVEL_2,
  SAI_SWITCH_ATTR_VOQ_LATENCY_MAX_LEVEL_2,
} sai_switch_extensions_attr_t;

typedef enum _sai_tam_event_extensions_attr_t {
  SAI_TAM_EVENT_ATTR_FAKE_SWITCH_EVENT_TYPE = SAI_TAM_EVENT_ATTR_END,
  SAI_TAM_EVENT_ATTR_FAKE_SWITCH_EVENT_ID,
} sai_tam_event_extensions_attr_t;

typedef enum _sai_port_extensions_attr_t {
  SAI_PORT_ATTR_EXT_FAKE_SYSTEM_PORT_ID = SAI_PORT_ATTR_CUSTOM_RANGE_START,
  SAI_PORT_ATTR_SERDES_LANE_LIST,
  SAI_PORT_ATTR_DIAGNOSTICS_MODE_ENABLE,
  SAI_PORT_ATTR_RX_LANE_SQUELCH_ENABLE,
  SAI_PORT_ATTR_FDR_ENABLE,
  SAI_PORT_ATTR_CRC_ERROR_TOKEN_DETECT,
} sai_port_extensions_attr_t;
