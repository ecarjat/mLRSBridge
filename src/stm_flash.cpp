#include "stm_flash.h"
#include "esp_task_wdt.h"

static const char *TAG_STM_FLASH = "stm_flash";

esp_err_t writeTask(uint8_t nb_uart, File *flash_file)
{
    logI(TAG_STM_FLASH, "%s", "Write Task");

    char loadAddress[4] = {0x08, 0x00, 0x00, 0x00};
    char block[256] = {0};
    int curr_block = 0, bytes_read = 0;

    flash_file->seek(0, fs::SeekSet);
    esp_task_wdt_reset();
    setupSTM(nb_uart);
    while ((bytes_read = flash_file->read((uint8_t *)block, 256)) > 0)
    {
        curr_block++;
        logI(TAG_STM_FLASH, "Writing block: %d", curr_block);
        //ESP_LOG_BUFFER_HEXDUMP("Block:  ", block, sizeof(block), ESP_LOG_DEBUG);

        esp_err_t ret = flashPage(nb_uart, loadAddress, block);
        if (ret == ESP_FAIL)
        {
            logE(TAG_STM_FLASH, "Failed flashing page: %x", flashPage);
            return ESP_FAIL;
        }
        esp_task_wdt_reset();
        incrementLoadAddress(loadAddress);
        printf("\n");

        memset(block, 0xff, 256);
    }

    return ESP_OK;
}

esp_err_t readTask(uint8_t nb_uart,File *flash_file)
{
    logI(TAG_STM_FLASH, "%s", "Read & Verification Task");
    char readAddress[4] = {0x08, 0x00, 0x00, 0x00};

    char block[257] = {0};
    int curr_block = 0, bytes_read = 0;

    flash_file->seek(0, fs::SeekSet);

    while ((bytes_read = flash_file->read((uint8_t *)block, 256)) > 0)
    {
        curr_block++;
        logI(TAG_STM_FLASH, "Reading block: %d", curr_block);
        //ESP_LOG_BUFFER_HEXDUMP("Block:  ", block, sizeof(block), ESP_LOG_DEBUG);

        esp_err_t ret = readPage(nb_uart, readAddress, block);
        if (ret == ESP_FAIL)
        {
            return ESP_FAIL;
        }
        esp_task_wdt_reset();
        incrementLoadAddress(readAddress);
        printf("\n");

        memset(block, 0xff, 256);
    }

    return ESP_OK;
}
