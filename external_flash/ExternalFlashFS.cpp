/*
 * @file ExternalFlashFS.cpp
 * @brief External SPI Flash Filesystem Implementation for STM32WL
 * 
 * This file implements LittleFS filesystem support for external SPI Flash chips
 * on STM32WL platforms. It provides block device operations for LittleFS by
 * translating them to SPI Flash commands (read, program, erase).
 * 
 * Supported flash chips:
 * - W25Q series (W25Q16, W25Q32, W25Q64, W25Q128, etc.)
 * - MX25 series (MX25L64, MX25L128, etc.)
 * - Most SPI NOR flash chips with standard command set
 * 
 * @author Meshtastic Team
 * @date 2026-03-03
 */

#include "ExternalFlashFS.h"
#include <Arduino.h>

// SPI Flash commands
#define FLASH_CMD_READ 0x03
#define FLASH_CMD_FAST_READ 0x0B
#define FLASH_CMD_PAGE_PROG 0x02
#define FLASH_CMD_SECTOR_ERASE 0x20
#define FLASH_CMD_BLOCK_ERASE 0x52
#define FLASH_CMD_CHIP_ERASE 0x60
#define FLASH_CMD_WRITE_ENABLE 0x06
#define FLASH_CMD_WRITE_DISABLE 0x04
#define FLASH_CMD_READ_STATUS 0x05
#define FLASH_CMD_WRITE_STATUS 0x01
#define FLASH_CMD_POWER_DOWN 0xB9
#define FLASH_CMD_RELEASE_POWER_DOWN 0xAB
#define FLASH_CMD_JEDEC_ID 0x9F
#define FLASH_CMD_READ_SFDP 0x5A

// Status register bits
#define FLASH_STATUS_BUSY 0x01
#define FLASH_STATUS_WRITE_ENABLE 0x02

// Timeout for flash operations
#define FLASH_TIMEOUT_MS 5000

// Debug output (only if CFG_DEBUG is enabled)
#if CFG_DEBUG
#define FLASH_DEBUG(...) LOG_DEBUG(__VA_ARGS__)
#else
#define FLASH_DEBUG(...)
#endif

// Global instance
ExternalFlashFS ExternalFS;

ExternalFlashFS::ExternalFlashFS()
    : _spi(nullptr), _csPin(NC), _flashSize(EXTERNAL_FLASH_SIZE_BYTES), _initialized(false), _readBuffer(nullptr),
      _progBuffer(nullptr)
{
    // Initialize lfs_config structure
    _lfs_cfg.read_size = EXTERNAL_FLASH_LFS_READ_SIZE;
    _lfs_cfg.prog_size = EXTERNAL_FLASH_LFS_PROG_SIZE;
    _lfs_cfg.block_size = EXTERNAL_FLASH_LFS_BLOCK_SIZE;
    _lfs_cfg.block_count = EXTERNAL_FLASH_LFS_BLOCK_COUNT;
    _lfs_cfg.lookahead = EXTERNAL_FLASH_LFS_LOOKAHEAD;
    _lfs_cfg.name_max = EXTERNAL_FLASH_LFS_NAME_MAX;
    _lfs_cfg.file_max = EXTERNAL_FLASH_LFS_FILE_MAX;
    _lfs_cfg.attr_max = EXTERNAL_FLASH_LFS_ATTR_MAX;

    // Set block device operations
    _lfs_cfg.read = _block_read;
    _lfs_cfg.prog = _block_prog;
    _lfs_cfg.erase = _block_erase;
    _lfs_cfg.sync = _block_sync;
    _lfs_cfg.context = this;
}

ExternalFlashFS::~ExternalFlashFS()
{
    end();
}

bool ExternalFlashFS::begin(SPIClass &spi, uint8_t cs_pin, uint32_t frequency)
{
    if (_initialized) {
        FLASH_DEBUG("ExternalFlashFS already initialized");
        return _mounted;
    }

    _spi = &spi;
    _csPin = cs_pin;

    // Initialize SPI and CS pin
    pinMode(_csPin, OUTPUT);
    _deselect();

    _spi->begin();
    _spi->beginTransaction(SPISettings(frequency, MSBFIRST, SPI_MODE0));

    // Detect flash chip
    if (!detectFlash()) {
        FLASH_DEBUG("External flash chip not detected");
        _spi->endTransaction();
        return false;
    }

    FLASH_DEBUG("External flash detected, size: %u bytes", _flashSize);

    // Allocate buffers
    _readBuffer = new uint8_t[_lfs_cfg.read_size];
    _progBuffer = new uint8_t[_lfs_cfg.prog_size];

    if (!_readBuffer || !_progBuffer) {
        FLASH_DEBUG("Failed to allocate flash buffers");
        delete[] _readBuffer;
        delete[] _progBuffer;
        _readBuffer = nullptr;
        _progBuffer = nullptr;
        _spi->endTransaction();
        return false;
    }

    _initialized = true;

    // Mount filesystem
    bool result = STM32_LittleFS::begin(&_lfs_cfg);

    _spi->endTransaction();

    if (result) {
        FLASH_DEBUG("ExternalFS mounted successfully");
    } else {
        FLASH_DEBUG("ExternalFS mount failed, attempting format");
        // Try to format and mount
        if (format()) {
            result = STM32_LittleFS::begin(&_lfs_cfg);
            if (result) {
                FLASH_DEBUG("ExternalFS formatted and mounted");
            }
        }
    }

    return result;
}

bool ExternalFlashFS::begin(struct lfs_config *cfg)
{
    if (cfg) {
        // Use provided configuration
        memcpy(&_lfs_cfg, cfg, sizeof(struct lfs_config));
        _lfs_cfg.read = _block_read;
        _lfs_cfg.prog = _block_prog;
        _lfs_cfg.erase = _block_erase;
        _lfs_cfg.sync = _block_sync;
        _lfs_cfg.context = this;
    }

    return STM32_LittleFS::begin(&_lfs_cfg);
}

void ExternalFlashFS::end(void)
{
    if (_mounted) {
        STM32_LittleFS::end();
        _mounted = false;
    }

    if (_readBuffer) {
        delete[] _readBuffer;
        _readBuffer = nullptr;
    }

    if (_progBuffer) {
        delete[] _progBuffer;
        _progBuffer = nullptr;
    }

    if (_spi) {
        _spi->end();
        _spi = nullptr;
    }

    _initialized = false;
    FLASH_DEBUG("ExternalFS stopped");
}

bool ExternalFlashFS::detectFlash(void)
{
    uint32_t jedecId = getFlashId();

    if (jedecId == 0 || jedecId == 0xFFFFFF) {
        return false;
    }

    FLASH_DEBUG("Flash JEDEC ID: 0x%06X", jedecId);

    // Determine flash size from JEDEC ID
    // Format: [Manufacturer ID (8 bits)][Memory Type (8 bits)][Capacity (8 bits)]
    uint8_t capacity = jedecId & 0xFF;

    // Common flash sizes (capacity code -> size in bytes)
    switch (capacity) {
    case 0x14:
        _flashSize = 2 * 1024 * 1024;
        break; // 16 Mbit
    case 0x15:
        _flashSize = 4 * 1024 * 1024;
        break; // 32 Mbit
    case 0x16:
        _flashSize = 8 * 1024 * 1024;
        break; // 64 Mbit
    case 0x17:
        _flashSize = 16 * 1024 * 1024;
        break; // 128 Mbit
    case 0x18:
        _flashSize = 32 * 1024 * 1024;
        break; // 256 Mbit
    case 0x19:
        _flashSize = 64 * 1024 * 1024;
        break; // 512 Mbit
    case 0x20:
        _flashSize = 128 * 1024 * 1024;
        break; // 1 Gbit
    default:
        // Unknown size, use default
        _flashSize = EXTERNAL_FLASH_SIZE_BYTES;
        FLASH_DEBUG("Unknown flash capacity code: 0x%02X, using default size", capacity);
        break;
    }

    // Update lfs_config with actual flash size
    _lfs_cfg.block_count = _flashSize / _lfs_cfg.block_size;

    return true;
}

uint32_t ExternalFlashFS::getFlashId(void)
{
    _select();
    _spi->transfer(FLASH_CMD_JEDEC_ID);

    uint32_t id = 0;
    id |= (_spi->transfer(0) << 16);
    id |= (_spi->transfer(0) << 8);
    id |= _spi->transfer(0);

    _deselect();

    return id;
}

bool ExternalFlashFS::eraseChip(void)
{
    if (!_initialized) {
        return false;
    }

    FLASH_DEBUG("Erasing entire flash chip...");

    _spi->beginTransaction(SPISettings(10000000, MSBFIRST, SPI_MODE0));

    if (!_writeEnable()) {
        _spi->endTransaction();
        return false;
    }

    _select();
    _spi->transfer(FLASH_CMD_CHIP_ERASE);
    _deselect();

    bool result = _waitForReady(FLASH_TIMEOUT_MS);

    _spi->endTransaction();

    if (result) {
        FLASH_DEBUG("Chip erase complete");
    } else {
        FLASH_DEBUG("Chip erase timeout");
    }

    return result;
}

void ExternalFlashFS::enterDeepPowerDown(void)
{
    if (!_initialized) {
        return;
    }

    _spi->beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0));
    _select();
    _spi->transfer(FLASH_CMD_POWER_DOWN);
    _deselect();
    _spi->endTransaction();

    FLASH_DEBUG("External flash entered deep power-down");
}

void ExternalFlashFS::wakeFromDeepPowerDown(void)
{
    if (!_initialized) {
        return;
    }

    _spi->beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0));
    _select();
    _spi->transfer(FLASH_CMD_RELEASE_POWER_DOWN);
    _spi->transfer(0); // Dummy bytes
    _spi->transfer(0);
    _spi->transfer(0);
    _deselect();
    _spi->endTransaction();

    delayMicroseconds(20); // Wakeup time

    FLASH_DEBUG("External flash woke from deep power-down");
}

// ------------------------------------------------------------------
// Private methods - SPI low-level operations
// ------------------------------------------------------------------

bool ExternalFlashFS::_writeEnable(void)
{
    _select();
    _spi->transfer(FLASH_CMD_WRITE_ENABLE);
    _deselect();

    uint8_t status = _readStatusRegister();
    return (status & FLASH_STATUS_WRITE_ENABLE) != 0;
}

bool ExternalFlashFS::_waitForReady(uint32_t timeout_ms)
{
    uint32_t start = millis();

    while (millis() - start < timeout_ms) {
        uint8_t status = _readStatusRegister();
        if (!(status & FLASH_STATUS_BUSY)) {
            return true;
        }
        delayMicroseconds(100);
    }

    return false;
}

void ExternalFlashFS::_select(void)
{
    digitalWrite(_csPin, LOW);
}

void ExternalFlashFS::_deselect(void)
{
    digitalWrite(_csPin, HIGH);
}

uint8_t ExternalFlashFS::_readStatusRegister(void)
{
    _select();
    _spi->transfer(FLASH_CMD_READ_STATUS);
    uint8_t status = _spi->transfer(0);
    _deselect();
    return status;
}

void ExternalFlashFS::_writeStatusRegister(uint8_t status)
{
    if (!_writeEnable()) {
        return;
    }

    _select();
    _spi->transfer(FLASH_CMD_WRITE_STATUS);
    _spi->transfer(status);
    _deselect();

    _waitForReady();
}

// ------------------------------------------------------------------
// LittleFS block device operations
// ------------------------------------------------------------------

int ExternalFlashFS::_block_init(const struct lfs_config *c)
{
    ExternalFlashFS *flash = (ExternalFlashFS *)c->context;
    if (!flash->_initialized) {
        return LFS_ERR_IO;
    }
    return 0;
}

int ExternalFlashFS::_block_read(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, void *buffer, lfs_size_t size)
{
    ExternalFlashFS *flash = (ExternalFlashFS *)c->context;

    if (!flash->_initialized || !flash->_spi) {
        return LFS_ERR_IO;
    }

    uint32_t addr = block * c->block_size + off;

    flash->_spi->beginTransaction(SPISettings(10000000, MSBFIRST, SPI_MODE0));
    flash->_select();

    flash->_spi->transfer(FLASH_CMD_READ);
    flash->_spi->transfer((addr >> 16) & 0xFF);
    flash->_spi->transfer((addr >> 8) & 0xFF);
    flash->_spi->transfer(addr & 0xFF);

    // Read data
    for (lfs_size_t i = 0; i < size; i++) {
        ((uint8_t *)buffer)[i] = flash->_spi->transfer(0);
    }

    flash->_deselect();
    flash->_spi->endTransaction();

    return 0;
}

int ExternalFlashFS::_block_prog(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, const void *buffer,
                                 lfs_size_t size)
{
    ExternalFlashFS *flash = (ExternalFlashFS *)c->context;

    if (!flash->_initialized || !flash->_spi) {
        return LFS_ERR_IO;
    }

    uint32_t addr = block * c->block_size + off;

    flash->_spi->beginTransaction(SPISettings(10000000, MSBFIRST, SPI_MODE0));

    // Enable writing
    flash->_select();
    flash->_spi->transfer(FLASH_CMD_WRITE_ENABLE);
    flash->_deselect();

    // Program page
    flash->_select();
    flash->_spi->transfer(FLASH_CMD_PAGE_PROG);
    flash->_spi->transfer((addr >> 16) & 0xFF);
    flash->_spi->transfer((addr >> 8) & 0xFF);
    flash->_spi->transfer(addr & 0xFF);

    for (lfs_size_t i = 0; i < size; i++) {
        flash->_spi->transfer(((const uint8_t *)buffer)[i]);
    }
    flash->_deselect();

    // Wait for completion
    bool ready = flash->_waitForReady();
    flash->_spi->endTransaction();

    return ready ? 0 : LFS_ERR_IO;
}

int ExternalFlashFS::_block_erase(const struct lfs_config *c, lfs_block_t block)
{
    ExternalFlashFS *flash = (ExternalFlashFS *)c->context;

    if (!flash->_initialized || !flash->_spi) {
        return LFS_ERR_IO;
    }

    uint32_t addr = block * c->block_size;

    flash->_spi->beginTransaction(SPISettings(10000000, MSBFIRST, SPI_MODE0));

    // Enable writing
    flash->_select();
    flash->_spi->transfer(FLASH_CMD_WRITE_ENABLE);
    flash->_deselect();

    // Erase sector
    flash->_select();
    flash->_spi->transfer(FLASH_CMD_SECTOR_ERASE);
    flash->_spi->transfer((addr >> 16) & 0xFF);
    flash->_spi->transfer((addr >> 8) & 0xFF);
    flash->_spi->transfer(addr & 0xFF);
    flash->_deselect();

    // Wait for completion
    bool ready = flash->_waitForReady(FLASH_TIMEOUT_MS);
    flash->_spi->endTransaction();

    if (!ready) {
        FLASH_DEBUG("Block erase timeout for block %u", block);
        return LFS_ERR_IO;
    }

    return 0;
}

int ExternalFlashFS::_block_sync(const struct lfs_config *c)
{
    ExternalFlashFS *flash = (ExternalFlashFS *)c->context;

    if (!flash->_initialized) {
        return LFS_ERR_IO;
    }

    // SPI flash operations are synchronous, no additional sync needed
    return 0;
}
