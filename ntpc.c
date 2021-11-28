/**
 * @file ntpc.c
 *
 * Network time protocol client.
 */

#include <stdbool.h>
#include <time.h>

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#include "esp_log.h"
#include "esp_sntp.h"

#include "soc/rtc.h"
#include "driver/rtc_io.h"

#include "ntpc.h"

static const char *TAG = "ntpc";

/** System time and RTC clock synchronization mark */
RTC_DATA_ATTR static struct ntpc_mark {
    struct timeval tv;
    uint64_t rtc_clk;
    uint32_t rtc_period;
} s_mark;

/** Synchronization mark mutex */
static SemaphoreHandle_t s_mark_mutex;

/**
 * Read RTC timer.
 *
 * @return Number of RTC slow clock cycles since the last power-up
 * reset.
 */
static uint64_t read_rtc_timer(void)
{
    uint64_t now;

    SET_PERI_REG_MASK(RTC_CNTL_TIME_UPDATE_REG, RTC_CNTL_TIME_UPDATE);
    now = READ_PERI_REG(RTC_CNTL_TIME_LOW0_REG);
    now |= ((uint64_t)READ_PERI_REG(RTC_CNTL_TIME_HIGH0_REG)) << 32;

    return now;
}

/**
 * Handle time synchronization events.
 */
static void ntpc_sync_cb(struct timeval *tv)
{
    xSemaphoreTake(s_mark_mutex, portMAX_DELAY);

    s_mark.rtc_clk = read_rtc_timer();
    s_mark.tv = *tv;
    s_mark.rtc_period = rtc_clk_cal(RTC_CAL_RTC_MUX, 100);

    xSemaphoreGive(s_mark_mutex);

    ESP_LOGI(TAG, "event: time update");
}

void ntpc_init(const char *timezone, int daylight)
{
    time_t now;
    struct tm timeinfo;

    s_mark_mutex = xSemaphoreCreateMutex();
    if (!s_mark_mutex) {
        ESP_LOGE(TAG, "xSemaphoreCreateMutex: failed to create s_mark_mutex");
    }

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

    // set timezone
    if (timezone) {
	setenv("TZ", timezone, daylight);
	tzset();
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

time_t ntpc_rtctime(uint64_t rtc_clk)
{
    int64_t usec;
    int64_t delta_rtc;
    time_t result;

    xSemaphoreTake(s_mark_mutex, portMAX_DELAY);

    // usually rtc_clk > s_mark.rtc_clk, but if the system clock is
    // synchronized AFTER rtc_clk, the reference time (mark) may be in
    // the future; in this case, delta_rtc is negative
    delta_rtc = (int64_t)rtc_clk - s_mark.rtc_clk;

    usec = s_mark.tv.tv_usec +
           s_mark.rtc_period * delta_rtc / (1 << RTC_CLK_CAL_FRACT);
    result = s_mark.tv.tv_sec + usec / 1000000;

    xSemaphoreGive(s_mark_mutex);

    return result;
}
