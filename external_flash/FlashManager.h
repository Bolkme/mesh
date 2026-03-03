/*
 * @file FlashManager.h
 * @brief Global Flash Management Controller for STM32WL
 * 
 * This header provides a unified interface to manage both internal and external
 * flash storage. It handles:
 * - Automatic selection between InternalFS and ExternalFS
 * - Global SPI lock management
 * - Flash health monitoring
 * - Configuration backup/restore between flash types
 * - Power management coordination
 * 
 * @author Meshtastic Team
 * @date 2026-03-03
 */

#pragma once

#include "configuration.h"
#include "concurrency.h"
#include "SPILock.h"

#if defined(ARCH_STM32WL)

// Include flash filesystem implementations
#include "platform/stm32wl/LittleFS.h"

#ifdef USE_EXTERNAL_FLASH
#include "ExternalFlashFS.h"
#endif

// Flash type enumeration
enum class FlashType {
    INTERNAL,      // Use internal STM32 flash
    EXTERNAL,      // Use external SPI flash
    AUTO,          // Auto-select based on availability
    DUAL           // Use both (internal for config, external for data)
};

// Flash status structure
struct FlashStatus {
    bool mounted;
    FlashType type;
    size_t totalBytes;
    size_t usedBytes;
    bool healthy;
    uint32_t lastCheckTime;
};

/**
 * @brief Global Flash Manager Class
 * 
 * Provides centralized control over flash storage on STM32WL platforms.
 * Singleton pattern ensures single point of control.
 */
class FlashManager
{
  public:
    /**
     * @brief Get singleton instance
     * @return Reference to FlashManager instance
     */
    static FlashManager& getInstance();

    /**
     * @brief Initialize flash manager with specified type
     * @param type Flash type to use (INTERNAL, EXTERNAL, AUTO, DUAL)
     * @return true if initialization successful
     */
    bool init(FlashType type = FlashType::AUTO);

    /**
     * @brief Get current flash type in use
     * @return Current FlashType
     */
    FlashType getFlashType() const { return _currentType; }

    /**
     * @brief Check if external flash is available
     * @return true if external flash detected and mounted
     */
    bool isExternalAvailable() const;

    /**
     * @brief Check if internal flash is available
     * @return true if internal flash mounted
     */
    bool isInternalAvailable() const { return _internalMounted; }

    /**
     * @brief Get current flash status
     * @return FlashStatus structure with current state
     */
    FlashStatus getStatus();

    /**
     * @brief Get the active filesystem pointer
     * @return Pointer to active STM32_LittleFS instance
     */
    STM32_LittleFS* getActiveFS();

    /**
     * @brief Switch to a different flash type (runtime)
     * @param newType Target flash type
     * @param migrateData Whether to migrate data from old flash
     * @return true if switch successful
     */
    bool switchFlashType(FlashType newType, bool migrateData = false);

    /**
     * @brief Backup configuration from internal to external flash
     * @return true if backup successful
     */
    bool backupConfigToExternal();

    /**
     * @brief Restore configuration from external to internal flash
     * @return true if restore successful
     */
    bool restoreConfigFromExternal();

    /**
     * @brief Format the active flash
     * @return true if format successful
     */
    bool formatActiveFlash();

    /**
     * @brief Get free space in bytes
     * @return Free space in bytes
     */
    size_t getFreeSpace();

    /**
     * @brief Get total space in bytes
     * @return Total space in bytes
     */
    size_t getTotalSpace();

    /**
     * @brief Enable external flash (if available)
     * @return true if enabled or already active
     */
    bool enableExternal();

    /**
     * @brief Disable external flash and switch to internal
     * @return true if disabled or already using internal
     */
    bool disableExternal();

    /**
     * @brief Check flash health (call periodically)
     * @return true if flash is healthy
     */
    bool checkHealth();

    /**
     * @brief Enter low-power mode (for external flash)
     */
    void enterLowPower();

    /**
     * @brief Exit low-power mode
     */
    void exitLowPower();

  private:
    FlashManager();
    ~FlashManager();
    
    // Prevent copying
    FlashManager(const FlashManager&) = delete;
    FlashManager& operator=(const FlashManager&) = delete;

    bool _initialized;
    bool _internalMounted;
    bool _externalMounted;
    FlashType _currentType;
    FlashType _configuredType;
    
#ifdef USE_EXTERNAL_FLASH
    bool _externalAvailable;
#endif

    // Internal methods
    bool _initInternal();
    bool _initExternal();
    bool _autoSelect();
    void _migrateData(FlashType from, FlashType to);
};

// Global instance accessor macro
#define FlashMgr FlashManager::getInstance()

// Convenience macros for common operations
#define FLASH_INIT(type) FlashManager::getInstance().init(type)
#define FLASH_GET_FS() FlashManager::getInstance().getActiveFS()
#define FLASH_STATUS() FlashManager::getInstance().getStatus()

#endif // ARCH_STM32WL
