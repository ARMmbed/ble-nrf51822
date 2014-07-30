/* Copyright (c) 2011 Nordic Semiconductor. All Rights Reserved.
 *
 * The information contained herein is confidential property of Nordic Semiconductor. The use,
 * copying, transfer or disclosure of such information is prohibited except by express written
 * agreement with Nordic Semiconductor.
 *
 */
/**
  @addtogroup BLE_COMMON
  @{
  @defgroup ble_types Common types and macro definitions
  @{

  @brief Common types and macro definitions for the S110 SoftDevice.
 */

#ifndef BLE_TYPES_H__
#define BLE_TYPES_H__

#include <stdint.h>

/** @addtogroup BLE_COMMON_DEFINES Defines
 * @{ */

/** @defgroup BLE_CONN_HANDLES BLE Connection Handles
 * @{ */
#define BLE_CONN_HANDLE_INVALID 0xFFFF  /**< Invalid Connection Handle. */
#define BLE_CONN_HANDLE_ALL     0xFFFE  /**< Applies to all Connection Handles. */
/** @} */





/** @defgroup BLE_UUID_TYPES Types of UUID
 * @{ */
#define BLE_UUID_TYPE_UNKNOWN       0x00 /**< Invalid UUID type. */
#define BLE_UUID_TYPE_BLE           0x01 /**< Bluetooth SIG UUID (16-bit). */
#define BLE_UUID_TYPE_VENDOR_BEGIN  0x02 /**< Vendor UUID types start at this index (128-bit). */
/** @} */

/** @brief Set .type and .uuid fields of ble_uuid_struct to specified uuid value. */
#define BLE_UUID_BLE_ASSIGN(instance, value) do {\
            instance.type = BLE_UUID_TYPE_BLE; \
            instance.uuid = value;} while(0)

/** @brief Copy type and uuid members from src to dst ble_uuid_t pointer. Both pointers must be valid/non-null. */
#define BLE_UUID_COPY_PTR(dst, src) do {\
            (dst)->type = (src)->type; \
            (dst)->uuid = (src)->uuid;} while(0)

/** @brief Copy type and uuid members from src to dst ble_uuid_t struct. */
#define BLE_UUID_COPY_INST(dst, src) do {\
            (dst).type = (src).type; \
            (dst).uuid = (src).uuid;} while(0)

/** @brief Compare for equality both type and uuid members of two (valid, non-null) ble_uuid_t pointers. */
#define BLE_UUID_EQ(p_uuid1, p_uuid2) \
            (((p_uuid1)->type == (p_uuid2)->type) && ((p_uuid1)->uuid == (p_uuid2)->uuid))

/** @brief Compare for difference both type and uuid members of two (valid, non-null) ble_uuid_t pointers. */
#define BLE_UUID_NEQ(p_uuid1, p_uuid2) \
            (((p_uuid1)->type != (p_uuid2)->type) || ((p_uuid1)->uuid != (p_uuid2)->uuid))

/** @} */

/** @addtogroup BLE_TYPES_STRUCTURES Structures
 * @{ */

/** @brief 128 bit UUID values. */
typedef struct
{
    unsigned char uuid128[16];
} ble_uuid128_t;

/** @brief  Bluetooth Low Energy UUID type, encapsulates both 16-bit and 128-bit UUIDs. */
typedef struct
{
    uint16_t    uuid; /**< 16-bit UUID value or octets 12-13 of 128-bit UUID. */
    uint8_t     type; /**< UUID type, see @ref BLE_UUID_TYPES. If type is BLE_UUID_TYPE_UNKNOWN, the value of uuid is undefined. */
} ble_uuid_t;

/** @} */

#endif /* BLE_TYPES_H__ */

/**
  @}
  @}
*/
