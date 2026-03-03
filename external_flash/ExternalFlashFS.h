/*
 * @file ExternalFlashFS.h
 * @brief External SPI Flash Filesystem Support for STM32WL
 * 
 * This file provides LittleFS filesystem support for external SPI Flash chips
 * (e.g., W25Q series, MX25 series) on STM32WL platforms.
 * 
 * Use this instead of InternalFS when you need to store configuration data
 * on external flash to preserve internal flash space or increase storage capacity.
 * 
 * @author Meshtastic Team
 * @date 2026-03-03
 */

#ifndef EXTERNAL_FLASH_FS_H_
#define EXTERNAL_FLASH_FS_H_

#include "STM32_LittleFS.h"
#include <SPI.h>
#include "concurrency.h"
#include "SPILock.h"

// Default SPI Flash configuration
// These can be overridden in variant.h or platformio.ini
#ifndef EXTERNAL_FLASH_CS
#define EXTERNAL_FLASH_CS PA4
#endif

#ifndef EXTERNAL_FLASH_SIZE_BYTES
// Default 8MB (64 Mbit) - adjust for your flash chip
#define EXTERNAL_FLASH_SIZE_BYTES (8 * 1024 * 1024)
#endif

#ifndef EXTERNAL_FLASH_BLOCK_SIZE
#define EXTERNAL_FLASH_BLOCK_SIZE 4096
#endif

#ifndef EXTERNAL_FLASH_PAGE_SIZE
#define EXTERNAL_FLASH_PAGE_SIZE 256
#endif

#ifndef EXTERNAL_FLASH_SECTOR_SIZE
#define EXTERNAL_FLASH_SECTOR_SIZE 4096
#endif

// LittleFS configuration for external flash
#define EXTERNAL_FLASH_LFS_READ_SIZE 256
#define EXTERNAL_FLASH_LFS_PROG_SIZE 256
#define EXTERNAL_FLASH_LFS_BLOCK_SIZE EXTERNAL_FLASH_BLOCK_SIZE
#define EXTERNAL_FLASH_LFS_BLOCK_COUNT (EXTERNAL_FLASH_SIZE_BYTES / EXTERNAL_FLASH_BLOCK_SIZE)
#define EXTERNAL_FLASH_LFS_LOOKAHEAD 32
#define EXTERNAL_FLASH_LFS_NAME_MAX 255
#define EXTERNAL_FLASH_LFS_FILE_MAX 256
#define EXTERNAL_FLASH_LFS_ATTR_MAX 1022

class ExternalFlashFS : public STM32_LittleFS
{
  public:
    ExternalFlashFS(void);
    virtual ~ExternalFlashFS();

    /**
     * @brief Initialize and mount the external flash filesystem (with SPI lock)
     * @param spi SPI instance to use (default: SPI)
     * @param cs_pin Chip Select pin (default: EXTERNAL_FLASH_CS)
     * @param frequency SPI frequency in Hz (default: 10MHz)
     * @return true if mount successful, false otherwise
     */
    bool begin(SPIClass &spi = SPI, uint8_t cs_pin = EXTERNAL_FLASH_CS, uint32_t frequency = 10000000);
    
    /**
     * @brief Initialize and mount with custom lfs_config (with SPI lock)
     * @param cfg LittleFS configuration structure
     * @return true if mount successful, false otherwise
     */
    bool begin(struct lfs_config *cfg) override;

    /**
     * @brief Unmount and cleanup external flash filesystem (with SPI lock)
     */
    void end(void) override;

    /**
     * @brief Internal begin without SPI lock (for use when lock is already held)
     * @param spi SPI instance to use
     * @param cs_pin Chip Select pin
     * @param frequency SPI frequency in Hz
     * @return true if mount successful, false otherwise
     */
    bool beginInternal(SPIClass &spi, uint8_t cs_pin, uint32_t frequency);
    
    /**
     * @brief Internal end without SPI lock (for use when lock is already held)
     */
    void endInternal(void);

    /**
     * @brief Check if external flash is connected and responsive
     * @return true if flash chip is detected, false otherwise
     */
    bool detectFlash(void);

    /**
     * @brief Get flash chip JEDEC ID
     * @return 3-byte JEDEC ID as uint32_t (lower 24 bits)
     */
    uint32_t getFlashId(void);

    /**
     * @brief Get flash chip capacity in bytes
     * @return Flash size in bytes
     */
    size_t getFlashSize(void) const { return _flashSize; }

    /**
     * @brief Erase entire flash chip (WARNING: destroys all data)
     * @return true if erase successful, false otherwise
     */
    bool eraseChip(void);

    /**
     * @brief Put flash chip in deep power-down mode (saves power)
     */
    void enterDeepPowerDown(void);

    /**
     * @brief Wake flash chip from deep power-down mode
     */
    void wakeFromDeepPowerDown(void);

  private:
    SPIClass *_spi;
    uint8_t _csPin;
    size_t _flashSize;
    bool _initialized;
    
    struct lfs_config _lfs_cfg;
    uint8_t *_readBuffer;
    uint8_t *_progBuffer;

    // LittleFS block device operations
    static int _block_init(const struct lfs_config *c);
    static int _block_read(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, void *buffer, lfs_size_t size);
    static int _block_prog(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, const void *buffer, lfs_size_t size);
    static int _block_erase(const struct lfs_config *c, lfs_block_t block);
    static int _block_sync(const struct lfs_config *c);

    // SPI low-level operations
    bool _writeEnable(void);
    bool _waitForReady(uint32_t timeout_ms = 5000);
    void _select(void);
    void _deselect(void);
    uint8_t _readStatusRegister(void);
    void _writeStatusRegister(uint8_t status);
};

// Global instance for external flash filesystem
extern ExternalFlashFS ExternalFS;

#endif /* EXTERNAL_FLASH_FS_H_ */
