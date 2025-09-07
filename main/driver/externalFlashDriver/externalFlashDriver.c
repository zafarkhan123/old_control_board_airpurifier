/**
 * @file externalFlashDriver.c
 *
 * @brief external flash driver
 *
 * @dir externalFlashDriver
 * @brief 
 *
 * @author matfio
 * @date 2021.12.31
 * @version v1.0
 *
 * @copyright 2021 Fideltronik R&D - all rights reserved.
 */

#include "setting.h"
#include "config.h"

#include "esp_log.h"

#include "esp_flash.h"
#include "esp_flash_spi_init.h"
#include "esp_partition.h"
#include "esp_vfs.h"
#include "esp_vfs_fat.h"
#include "esp_system.h"

/*****************************************************************************
                          PRIVATE DEFINES / MACROS
*****************************************************************************/

/*****************************************************************************
                     PRIVATE STRUCTS / ENUMS / VARIABLES
*****************************************************************************/

static esp_flash_t* sExternalFlash;

static const char *TAG = "extFlashD";

// Mount path for the partition
const char *base_path = "/extflash";

// Handle of the wear levelling library instance
static wl_handle_t s_wl_handle;

/*****************************************************************************
                         PRIVATE FUNCTION DECLARATION
*****************************************************************************/

/** @brief  Adding a new partition to the existing ones
 *  @param ext_flash pointer to esp_flash_t
 *  @param partition_label new partition label
 *  @return return esp_partition_t pointer
 */
static const esp_partition_t* AddNewPartition(esp_flash_t* ext_flash, const char* partition_label);

/** @brief  Lists all data partition
 */
static void ListDataPartitions(void);

/** @brief  Mount fat fs in selected partition
 *  @param partition_label partition label
 *  @return return true if success
 */
static bool MountFatfs(const char* partition_label);

/** @brief  Initialize Flash reset an cip select pin
 *  @return return esp_partition_t pointer
 */
static bool InitFlashRstAndCsPin(void);

/** @brief  Get fat fs usage
 *  @param out_total_bytes total bytes
 *  @param out_free_bytes free bytes
 */
static void GetFatfsUsage(size_t* out_total_bytes, size_t* out_free_bytes);

/*****************************************************************************
                           INTERFACE IMPLEMENTATION
*****************************************************************************/

bool ExternalFlashDriverInit(void)
{
    InitFlashRstAndCsPin();
    vTaskDelay(500U);
    gpio_set_level(CFG_EXTERNAL_FLASH_RST_GPIO, 1);
    vTaskDelay(500U);

    const esp_flash_spi_device_config_t device_config = {
        .host_id = CFG_SPI_HOST_NUMBER,
        .cs_id = 0,
        .cs_io_num = CFG_EXTERNAL_FLASH_CS_GPIO,
        .io_mode = SPI_FLASH_SLOWRD,
        .speed = ESP_FLASH_5MHZ,
    };

    // Add device to the SPI bus
    esp_err_t err = spi_bus_add_flash_device(&sExternalFlash, &device_config);
    if(err != ESP_OK){
        ESP_LOGE(TAG, "Failed to add device to bus: %s (0x%x)", esp_err_to_name(err), err);
        return false;
    }

    // Probe the Flash chip and initialize it
    err = esp_flash_init(sExternalFlash);
    if(err != ESP_OK){
        ESP_LOGE(TAG, "Failed to initialize external Flash: %s (0x%x)", esp_err_to_name(err), err);
        return false;
    }

    // Print out the ID and size
    uint32_t id;
    err = esp_flash_read_id(sExternalFlash, &id);
    if(err != ESP_OK){
        ESP_LOGE(TAG, "Read flash id fail: %s (0x%x)", esp_err_to_name(err), err);
        return false;
    }

    ESP_LOGI(TAG, "Initialized external Flash, size=%d KB, ID=0x%x", sExternalFlash->size / 1024, id);

    return true;
}

bool ExternalFlashEraseChip(void)
{
    esp_err_t err = esp_flash_erase_chip(sExternalFlash);
        if(err != ESP_OK){
        ESP_LOGE(TAG, "Erase chip fail: %s (0x%x)", esp_err_to_name(err), err);
        return false;
    }

    return true;
}

bool ExternalFlashMountFs(void)
{
    const char *partition_label = "storage";
    AddNewPartition(sExternalFlash, partition_label);

    // List the available partitions
    ListDataPartitions();

    // Initialize FAT FS in the partition
    if (!MountFatfs(partition_label)) {
        ESP_LOGE(TAG, "mount fat fs fail");
        return false;
    }
    
    return true;
}

bool ExternalFlashFileTest(void)
{
    // Create a file in FAT FS
    ESP_LOGI(TAG, "Opening file");
    FILE *f = fopen("/extflash/hello.txt", "wb");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file for writing");
        return false;
    }
    fprintf(f, "Written using ESP-IDF %s\n", esp_get_idf_version());
    fclose(f);
    ESP_LOGI(TAG, "File written");

    // Open file for reading
    ESP_LOGI(TAG, "Reading file");
    f = fopen("/extflash/hello.txt", "rb");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file for reading");
        return false;
    }
    char line[128];
    fgets(line, sizeof(line), f);
    fclose(f);
    // strip newline
    char *pos = strchr(line, '\n');
    if (pos) {
        *pos = '\0';
    }
    ESP_LOGI(TAG, "Read from file: '%s'", line);

    return true;
}

/*****************************************************************************
                         PRIVATE FUNCTION IMPLEMENTATION
*****************************************************************************/

static const esp_partition_t* AddNewPartition(esp_flash_t* ext_flash, const char* partition_label)
{
    ESP_LOGI(TAG, "Adding external Flash as a partition, label=\"%s\", size=%d KB", partition_label, ext_flash->size / 1024);
    const esp_partition_t* fat_partition;
    ESP_ERROR_CHECK(esp_partition_register_external(ext_flash, 0, ext_flash->size, partition_label, ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_FAT, &fat_partition));
    return fat_partition;
}

static void ListDataPartitions(void)
{
    ESP_LOGI(TAG, "Listing data partitions:");
    esp_partition_iterator_t it = esp_partition_find(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_ANY, NULL);

    for (; it != NULL; it = esp_partition_next(it)) {
        const esp_partition_t *part = esp_partition_get(it);
        ESP_LOGI(TAG, "- partition '%s', subtype %d, offset 0x%x, size %d kB",
        part->label, part->subtype, part->address, part->size / 1024);
    }

    esp_partition_iterator_release(it);
}

static bool MountFatfs(const char* partition_label)
{
    ESP_LOGI(TAG, "Mounting FAT filesystem");
    const esp_vfs_fat_mount_config_t mount_config = {
            .max_files = 4,
            .format_if_mount_failed = true,
            .allocation_unit_size = 0
    };
    esp_err_t err = esp_vfs_fat_spiflash_mount(base_path, partition_label, &mount_config, &s_wl_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to mount FATFS (%s)", esp_err_to_name(err));
        return false;
    }

    // Print FAT FS size information
    size_t bytesTotal = 0;
    size_t bytesFree = 0;
    GetFatfsUsage(&bytesTotal, &bytesFree);
    ESP_LOGI(TAG, "FAT FS: %d kB total, %d kB free", bytesTotal / 1024, bytesFree / 1024);

    return true;
}

static bool InitFlashRstAndCsPin(void)
{
    bool res = true;
    gpio_config_t io_conf;

    //interrupt disable
    io_conf.intr_type = GPIO_INTR_DISABLE;

    //bit mask of the pins
    io_conf.pin_bit_mask = ((1ULL << CFG_EXTERNAL_FLASH_RST_GPIO) || (1ULL << CFG_EXTERNAL_FLASH_CS_GPIO));

    //set as input mode
    io_conf.mode = GPIO_MODE_OUTPUT;

    //disable pull-down mode
    io_conf.pull_down_en = 0;
    //enable pull-up mode
    io_conf.pull_up_en = 1;
    
    esp_err_t esp_res = gpio_config(&io_conf);
    if(esp_res != ESP_OK){
        res = false;
    }

    esp_res = gpio_set_level(CFG_EXTERNAL_FLASH_RST_GPIO, 0);
    if(esp_res != ESP_OK){
        res = false;
    }

    esp_res = gpio_set_level(CFG_EXTERNAL_FLASH_CS_GPIO, 1);
    if(esp_res != ESP_OK){
        res = false;
    }

    return res;
}

static void GetFatfsUsage(size_t* out_total_bytes, size_t* out_free_bytes)
{
    FATFS *fs;
    size_t free_clusters;
    int res = f_getfree("0:", &free_clusters, &fs);
    assert(res == FR_OK);
    size_t total_sectors = (fs->n_fatent - 2) * fs->csize;
    size_t free_sectors = free_clusters * fs->csize;

    // assuming the total size is < 4GiB, should be true for SPI Flash
    if (out_total_bytes != NULL) {
        *out_total_bytes = total_sectors * fs->ssize;
    }
    if (out_free_bytes != NULL) {
        *out_free_bytes = free_sectors * fs->ssize;
    }
}