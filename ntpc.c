/**
 * @file ntpc.c
 *
 * Network time protocol client.
 */

#include <stdbool.h>

#include "esp_log.h"
#include "esp_sntp.h"

#include "ntpc.h"

static const char *TAG = "ntpc";

/**
 * Handle time synchronization events.
 */
static void ntpc_sync_cb(struct timeval *tv)
{
    ESP_LOGI(TAG, "event: time update");
}

void ntpc_init(void)
{
    time_t now;
    struct tm timeinfo;

    ESP_LOGI(TAG, "init: update interval = %d ms", sntp_get_sync_interval());
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, CONFIG_NTPC_SERVER);
    sntp_set_time_sync_notification_cb(ntpc_sync_cb);
    sntp_init();

    time(&now);
    localtime_r(&now, &timeinfo);

    // if system time is not set, tm_year will be (1970 - 1900) = 70
    if (timeinfo.tm_year < 100) {
        ESP_LOGI(TAG, "time is not set");
        ntpc_sync(CONFIG_NTPC_SYNC_TIMEOUT);
    }
}

bool ntpc_sync(uint32_t timeout)
{
    sntp_set_sync_status(SNTP_SYNC_STATUS_RESET);

    for (uint32_t i = 0; i < timeout; ++i) {
        if (sntp_get_sync_status() == SNTP_SYNC_STATUS_COMPLETED) {
            return true;
        }

        ESP_LOGI(TAG, "... waiting for time update");
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }

    return false;
}
