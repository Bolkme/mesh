/*
 * @file FSCommon_ExternalFlash.h
 * @brief Wrapper header to use External SPI Flash with automatic SPI locking
 * 
 * This header redefines FSCom to use ExternalFS and ensures all filesystem
 * operations are protected by spiLock, preventing conflicts with other SPI
 * devices (e.g., SX126x radio transceivers).
 * 
 * Usage:
 * 1. Include this file AFTER configuration.h in your main code
 * 2. Define USE_EXTERNAL_FLASH in platformio.ini or variant.h
 * 
 * Example platformio.ini:
 * [env:your_board]
 * build_flags =
 *     -D USE_EXTERNAL_FLASH
 *     -D EXTERNAL_FLASH_CS=PA4
 * 
 * @author Meshtastic Team
 * @date 2026-03-03
 */

#pragma once

#if defined(ARCH_STM32WL) && defined(USE_EXTERNAL_FLASH)

// Include the ExternalFlashFS implementation
#include "ExternalFlashFS.h"

// Undefine existing FSCom macros if present
#ifdef FSCom
#undef FSCom
#endif

#ifdef FSBegin
#undef FSBegin
#endif

// Redefine FSCom to use ExternalFS
#define FSCom ExternalFS
#define FSBegin() ExternalFS.begin()

// Include concurrency for LockGuard
#include "concurrency.h"
#include "SPILock.h"

// Helper macro to ensure SPI lock is held during FS operations
// This is automatically used by FSCommon.cpp functions
#define EXTERNAL_FS_LOCK() concurrency::LockGuard external_fs_lock_guard(spiLock)

// Log the switch (only if debug is enabled)
#if CFG_DEBUG
#include "Logger.h"
#define EXTERNAL_FLASH_LOG LOG_DEBUG
#else
#define EXTERNAL_FLASH_LOG(...)
#endif

EXTERNAL_FLASH_LOG("Using External SPI Flash for filesystem with SPI locking");

/**
 * @brief Helper function to safely initialize ExternalFS with SPI lock
 * Call this instead of ExternalFS.begin() directly
 * @return true if mount successful, false otherwise
 */
inline bool ExternalFS_BeginSafe()
{
    concurrency::LockGuard g(spiLock);
    // Use beginInternal to avoid double-locking
    return ExternalFS.beginInternal(SPI, EXTERNAL_FLASH_CS, 10000000);
}

/**
 * @brief Helper function to safely initialize ExternalFS with custom parameters and SPI lock
 * @param spi SPI instance to use
 * @param cs_pin Chip Select pin
 * @param frequency SPI frequency in Hz
 * @return true if mount successful, false otherwise
 */
inline bool ExternalFS_BeginSafeCustom(SPIClass &spi, uint8_t cs_pin, uint32_t frequency)
{
    concurrency::LockGuard g(spiLock);
    return ExternalFS.beginInternal(spi, cs_pin, frequency);
}

/**
 * @brief Helper function to safely end ExternalFS with SPI lock
 * Call this instead of ExternalFS.end() directly
 */
inline void ExternalFS_EndSafe()
{
    concurrency::LockGuard g(spiLock);
    ExternalFS.endInternal();
}

/**
 * @brief Helper function to safely detect flash with SPI lock
 * @return true if flash detected, false otherwise
 */
inline bool ExternalFS_DetectSafe()
{
    concurrency::LockGuard g(spiLock);
    return ExternalFS.detectFlash();
}

/**
 * @brief Helper function to safely get flash ID with SPI lock
 * @return JEDEC ID or 0 if error
 */
inline uint32_t ExternalFS_GetFlashIdSafe()
{
    concurrency::LockGuard g(spiLock);
    return ExternalFS.getFlashId();
}

/**
 * @brief Helper function to safely erase chip with SPI lock
 * WARNING: This destroys all data!
 * @return true if erase successful, false otherwise
 */
inline bool ExternalFS_EraseChipSafe()
{
    concurrency::LockGuard g(spiLock);
    return ExternalFS.eraseChip();
}

#endif // ARCH_STM32WL && USE_EXTERNAL_FLASH
