/*
 * @file FlashManager.cpp
 * @brief Global Flash Management Controller Implementation
 * 
 * Implements centralized flash storage management for STM32WL platforms.
 * Handles automatic selection, switching, and coordination between
 * internal and external flash storage.
 * 
 * @author Meshtastic Team
 * @date 2026-03-03
 */

#include "FlashManager.h"

#if defined(ARCH_STM32WL)

#if CFG_DEBUG
#define FLASH_MGR_LOG LOG_DEBUG
#define FLASH_MGR_LOG_INFO LOG_INFO
#define FLASH_MGR_LOG_ERROR LOG_ERROR
#else
#define FLASH_MGR_LOG(...)
#define FLASH_MGR_LOG_INFO(...)
#define FLASH_MGR_LOG_ERROR(...)
#endif

// Singleton instance
static FlashManager* _instance = nullptr;

FlashManager::FlashManager()
    : _initialized(false), _internalMounted(false), _externalMounted(false),
      _currentType(FlashType::INTERNAL), _configuredType(FlashType::AUTO)
#ifdef USE_EXTERNAL_FLASH
      ,
      _externalAvailable(false)
#endif
{
}

FlashManager::~FlashManager()
{
    // Cleanup handled by filesystem destructors
}

FlashManager& FlashManager::getInstance()
{
    if (!_instance) {
        _instance = new FlashManager();
    }
    return *_instance;
}

bool FlashManager::init(FlashType type)
{
    if (_initialized) {
        FLASH_MGR_LOG("FlashManager already initialized");
        return true;
    }

    _configuredType = type;
    FLASH_MGR_LOG_INFO("FlashManager initializing with type: %d", (int)type);

    bool success = false;

    switch (type) {
    case FlashType::AUTO:
        success = _autoSelect();
        break;

    case FlashType::INTERNAL:
        success = _initInternal();
        break;

    case FlashType::EXTERNAL:
#ifdef USE_EXTERNAL_FLASH
        success = _initExternal();
#else
        FLASH_MGR_LOG_ERROR("External flash not enabled in build");
        success = _initInternal();  // Fallback
#endif
        break;

    case FlashType::DUAL:
        // Initialize both, prefer external for data
#ifdef USE_EXTERNAL_FLASH
        _initExternal();
#endif
        _initInternal();
        _currentType = _externalMounted ? FlashType::EXTERNAL : FlashType::INTERNAL;
        success = true;
        break;

    default:
        FLASH_MGR_LOG_ERROR("Unknown flash type: %d", (int)type);
        success = _initInternal();
        break;
    }

    if (success) {
        _initialized = true;
        FLASH_MGR_LOG_INFO("FlashManager initialized, using: %s",
                          _currentType == FlashType::EXTERNAL ? "EXTERNAL" : "INTERNAL");
    } else {
        FLASH_MGR_LOG_ERROR("FlashManager initialization failed");
    }

    return success;
}

bool FlashManager::_autoSelect()
{
    FLASH_MGR_LOG("Auto-selecting flash type");

#ifdef USE_EXTERNAL_FLASH
    // Try external flash first (preferred for larger storage)
    if (_initExternal()) {
        FLASH_MGR_LOG_INFO("External flash detected, using EXTERNAL");
        _currentType = FlashType::EXTERNAL;
        return true;
    }
#endif

    // Fallback to internal flash
    if (_initInternal()) {
        FLASH_MGR_LOG_INFO("Using INTERNAL flash (external not available)");
        _currentType = FlashType::INTERNAL;
        return true;
    }

    FLASH_MGR_LOG_ERROR("No flash storage available");
    return false;
}

bool FlashManager::_initInternal()
{
    FLASH_MGR_LOG("Initializing internal flash");

    // Acquire SPI lock (internal flash may use SPI on some configurations)
    concurrency::LockGuard g(spiLock);

    if (InternalFS.begin()) {
        _internalMounted = true;
        FLASH_MGR_LOG("Internal flash mounted successfully");
        return true;
    }

    FLASH_MGR_LOG_ERROR("Internal flash mount failed");
    return false;
}

bool FlashManager::_initExternal()
{
#ifndef USE_EXTERNAL_FLASH
    return false;
#endif

    FLASH_MGR_LOG("Initializing external flash");

    // ExternalFS.begin() handles its own SPI locking
    if (ExternalFS.begin()) {
        _externalMounted = true;
        _externalAvailable = true;
        FLASH_MGR_LOG("External flash mounted successfully");
        FLASH_MGR_LOG("External flash size: %u bytes", ExternalFS.getFlashSize());
        return true;
    }

    FLASH_MGR_LOG_ERROR("External flash mount failed");
    return false;
}

bool FlashManager::isExternalAvailable() const
{
#ifdef USE_EXTERNAL_FLASH
    return _externalMounted;
#else
    return false;
#endif
}

FlashStatus FlashManager::getStatus()
{
    FlashStatus status = {
        .mounted = false,
        .type = _currentType,
        .totalBytes = 0,
        .usedBytes = 0,
        .healthy = false,
        .lastCheckTime = millis()
    };

    switch (_currentType) {
    case FlashType::INTERNAL:
        status.mounted = _internalMounted;
        // Internal flash size would need platform-specific code
        status.healthy = _internalMounted;
        break;

    case FlashType::EXTERNAL:
#ifdef USE_EXTERNAL_FLASH
        status.mounted = _externalMounted;
        if (_externalMounted) {
            status.totalBytes = ExternalFS.getFlashSize();
            // LittleFS doesn't provide easy used/total API, would need custom implementation
        }
        status.healthy = _externalMounted;
#endif
        break;

    case FlashType::DUAL:
        status.mounted = _internalMounted || _externalMounted;
        status.healthy = _internalMounted && _externalMounted;
        break;

    default:
        break;
    }

    return status;
}

STM32_LittleFS* FlashManager::getActiveFS()
{
    switch (_currentType) {
    case FlashType::INTERNAL:
        return _internalMounted ? &InternalFS : nullptr;

    case FlashType::EXTERNAL:
#ifdef USE_EXTERNAL_FLASH
        return _externalMounted ? &ExternalFS : nullptr;
#else
        return nullptr;
#endif

    case FlashType::DUAL:
        // Prefer external for data operations
#ifdef USE_EXTERNAL_FLASH
        if (_externalMounted) {
            return &ExternalFS;
        }
#endif
        return _internalMounted ? &InternalFS : nullptr;

    default:
        return nullptr;
    }
}

bool FlashManager::switchFlashType(FlashType newType, bool migrateData)
{
    if (!_initialized) {
        FLASH_MGR_LOG_ERROR("FlashManager not initialized");
        return false;
    }

    if (newType == _currentType) {
        FLASH_MGR_LOG("Already using requested flash type");
        return true;
    }

    FLASH_MGR_LOG_INFO("Switching flash type from %d to %d", (int)_currentType, (int)newType);

    FlashType oldType = _currentType;

    // Unmount current flash
    switch (oldType) {
    case FlashType::INTERNAL:
        {
            concurrency::LockGuard g(spiLock);
            InternalFS.end();
            _internalMounted = false;
        }
        break;

    case FlashType::EXTERNAL:
#ifdef USE_EXTERNAL_FLASH
        ExternalFS.end();
        _externalMounted = false;
#endif
        break;

    default:
        break;
    }

    // Initialize new flash
    bool success = false;
    switch (newType) {
    case FlashType::INTERNAL:
        success = _initInternal();
        break;

    case FlashType::EXTERNAL:
#ifdef USE_EXTERNAL_FLASH
        success = _initExternal();
#else
        FLASH_MGR_LOG_ERROR("External flash not enabled");
        success = false;
#endif
        break;

    default:
        success = false;
        break;
    }

    if (success) {
        _currentType = newType;

        if (migrateData) {
            FLASH_MGR_LOG("Migrating data from old flash type");
            _migrateData(oldType, newType);
        }

        FLASH_MGR_LOG_INFO("Flash type switched successfully");
    } else {
        // Restore old type on failure
        FLASH_MGR_LOG_ERROR("Flash switch failed, restoring previous type");
        switch (oldType) {
        case FlashType::INTERNAL:
            _initInternal();
            break;
        case FlashType::EXTERNAL:
#ifdef USE_EXTERNAL_FLASH
            _initExternal();
#endif
            break;
        default:
            break;
        }
        _currentType = oldType;
    }

    return success;
}

void FlashManager::_migrateData(FlashType from, FlashType to)
{
    // This would require implementing file copy between filesystems
    // For now, just log the request
    FLASH_MGR_LOG("Data migration requested from %d to %d (not yet implemented)",
                 (int)from, (int)to);
    
    // TODO: Implement migration logic
    // 1. List all files on source filesystem
    // 2. Copy each file to destination filesystem
    // 3. Verify copy integrity
    // 4. Delete source files after verification
}

bool FlashManager::backupConfigToExternal()
{
#ifndef USE_EXTERNAL_FLASH
    FLASH_MGR_LOG_ERROR("External flash not available");
    return false;
#endif

    if (!_externalMounted) {
        FLASH_MGR_LOG_ERROR("External flash not mounted");
        return false;
    }

    if (!_internalMounted) {
        FLASH_MGR_LOG_ERROR("Internal flash not mounted");
        return false;
    }

    FLASH_MGR_LOG_INFO("Backing up config from internal to external flash");

    // TODO: Implement config file backup
    // Copy configuration files from InternalFS to ExternalFS

    return true;
}

bool FlashManager::restoreConfigFromExternal()
{
#ifndef USE_EXTERNAL_FLASH
    return false;
#endif

    if (!_externalMounted) {
        FLASH_MGR_LOG_ERROR("External flash not mounted");
        return false;
    }

    if (!_internalMounted) {
        FLASH_MGR_LOG_ERROR("Internal flash not mounted");
        return false;
    }

    FLASH_MGR_LOG_INFO("Restoring config from external to internal flash");

    // TODO: Implement config file restore
    // Copy configuration files from ExternalFS to InternalFS

    return true;
}

bool FlashManager::formatActiveFlash()
{
    FLASH_MGR_LOG_WARN("Formatting active flash - ALL DATA WILL BE LOST");

    switch (_currentType) {
    case FlashType::INTERNAL:
        {
            concurrency::LockGuard g(spiLock);
            InternalFS.end();
            bool result = InternalFS.begin();
            _internalMounted = result;
            return result;
        }

    case FlashType::EXTERNAL:
#ifdef USE_EXTERNAL_FLASH
        ExternalFS.end();
        ExternalFS.eraseChip();  // Full chip erase
        bool result = ExternalFS.begin();
        _externalMounted = result;
        return result;
#else
        return false;
#endif

    default:
        return false;
    }
}

size_t FlashManager::getFreeSpace()
{
    switch (_currentType) {
    case FlashType::INTERNAL:
        // Would need platform-specific implementation
        return 0;

    case FlashType::EXTERNAL:
#ifdef USE_EXTERNAL_FLASH
        if (_externalMounted) {
            // TODO: Implement proper free space calculation
            return ExternalFS.getFlashSize();  // Placeholder
        }
#endif
        return 0;

    default:
        return 0;
    }
}

size_t FlashManager::getTotalSpace()
{
    switch (_currentType) {
    case FlashType::INTERNAL:
        // Would need platform-specific implementation
        return 0;

    case FlashType::EXTERNAL:
#ifdef USE_EXTERNAL_FLASH
        if (_externalMounted) {
            return ExternalFS.getFlashSize();
        }
#endif
        return 0;

    default:
        return 0;
    }
}

bool FlashManager::enableExternal()
{
#ifdef USE_EXTERNAL_FLASH
    if (_externalMounted) {
        return true;  // Already enabled
    }

    if (_initExternal()) {
        _currentType = FlashType::EXTERNAL;
        return true;
    }
#endif
    return false;
}

bool FlashManager::disableExternal()
{
#ifdef USE_EXTERNAL_FLASH
    if (_currentType == FlashType::EXTERNAL) {
        // Switch back to internal
        return switchFlashType(FlashType::INTERNAL, false);
    }
#endif
    return true;  // Already using internal
}

bool FlashManager::checkHealth()
{
    FlashStatus status = getStatus();
    
    if (!status.mounted) {
        FLASH_MGR_LOG_ERROR("Flash not mounted - health check failed");
        return false;
    }

    // TODO: Implement more comprehensive health checks
    // - Check for read/write errors
    // - Verify filesystem integrity
    // - Check wear leveling (if applicable)

    return status.healthy;
}

void FlashManager::enterLowPower()
{
#ifdef USE_EXTERNAL_FLASH
    if (_currentType == FlashType::EXTERNAL && _externalMounted) {
        ExternalFS.enterDeepPowerDown();
        FLASH_MGR_LOG("External flash entered low-power mode");
    }
#endif
}

void FlashManager::exitLowPower()
{
#ifdef USE_EXTERNAL_FLASH
    if (_currentType == FlashType::EXTERNAL && _externalMounted) {
        ExternalFS.wakeFromDeepPowerDown();
        FLASH_MGR_LOG("External flash exited low-power mode");
    }
#endif
}

#endif // ARCH_STM32WL
