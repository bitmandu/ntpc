# ntpc

This is an [ESP-IDF][1] component to synchronize time with a NTP
server. It is based on the [ESP-IDF/examples/protocols/sntp][2]
example.

## Features

* Time synchronization as you would expect
* Converts clock values from deep sleep (from the RTC slow clock) to seconds

## Installation

From the `components/` directory of your ESP-IDF project:

    $ git submodule add https://github.com/bitmandu/ntpc.git

This component depends on [myfi][3] to connect to the Internet.

## Configuration

    $ idf.py menuconfig

Check the parameters in the `NTPC Configuration` menu. The defaults
are probably fine.

### Time Update Frequency

Time is synchronized with the NTP server based on
`CONFIG_LWIP_SNTP_UPDATE_DELAY` (default: 1 hour), which can be
changed from the `Component config` ⇨ `LWIP` ⇨ `SNTP` menu. When a
time synchronization event occurs, a log message is written.

    I (7773) ntpc: event: time update

This component also provides `ntpc_sync()` to explicitly request that
the time be synchronized.

## Usage

    void app_main(void)
    {
        // initialize non-volatile storage
        esp_err_t ret = nvs_flash_init();
        if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
            ESP_ERROR_CHECK(nvs_flash_erase());
            ret = nvs_flash_init();
        }
        ESP_ERROR_CHECK(ret);

        myfi_connect();

        nptc_init();

        /* ... the rest of your project ... */
    }

## Output

    ...
    I (2873) ntpc: init: update interval = 300000 ms
    I (2883) ntpc: time is not set
    I (2893) ntpc: ... waiting for time update
    I (3893) ntpc: ... waiting for time update
    I (7773) ntpc: event: time update

## API

### nptc_init()

Initialize NTP client.

    void nptc_init(void)

### ntpc_sync()

Synchronize time with NTP server.

    bool ntpc_sync(uint32_t timeout)

#### Parameters

- `timeout`: Timeout (s)

#### Returns

True if time is synchronized, false otherwise.

### ntpc_rtctime()

Convert RTC clock value to time in seconds

    time_t ntpc_rtctime(uint64_t rtc_clk)

#### Parameters

- `rtc_clk`: RTC slow clock (ticks)

#### Returns

Time in seconds.

## Time in Deep Sleep

To determine the time of an event in deep sleep — for example,
recorded by the [ESP32-S2 ULP-RISC-V coprocessor][4] — the RTC slow
clock can be used.

The following function, which would run on the ULP-RISC-V, returns the
value of the RTC slow clock.

    #include "ulp_riscv/ulp_riscv.h"
    #include "ulp_riscv/ulp_riscv_utils.h"

    static uint64_t read_slow_clock(void)
    {
        uint64_t now;

        SET_PERI_REG_MASK(RTC_CNTL_TIME_UPDATE_REG, RTC_CNTL_TIME_UPDATE);
        now = READ_PERI_REG(RTC_CNTL_TIME_LOW0_REG);
        now |= ((uint64_t)READ_PERI_REG(RTC_CNTL_TIME_HIGH0_REG)) << 32;

        return now;
    }

This component provides `ntpc_rtctime()` to convert these clock values
to the time in seconds.

## Contributing

[Pull requests][pulls] and [issue/bug reports][issues] are very much
encouraged!

## License

[MIT](LICENSE)


[issues]: https://github.com/bitmandu/ntpc/issues
[pulls]: https://github.com/bitmandu/ntpc/pulls
[1]: https://docs.espressif.com/projects/esp-idf/en/latest/esp32/index.html
[2]: https://github.com/espressif/esp-idf/tree/master/examples/protocols/sntp
[3]: https://github.com/bitmandu/myfi
[4]: https://docs.espressif.com/projects/esp-idf/en/latest/esp32s2/api-guides/ulp-risc-v.html
