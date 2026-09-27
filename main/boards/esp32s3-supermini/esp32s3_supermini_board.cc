#include "wifi_board.h"
#include "codecs/no_audio_codec.h"
#include "display/oled_display.h"
#include "application.h"
#include "button.h"
#include "led/single_led.h"
#include "config.h"
#include "mcp_server.h"

#include <driver/i2c_master.h>
#include <esp_event.h>
#include <esp_lcd_panel_io.h>
#include <esp_lcd_panel_ops.h>
#include <esp_lcd_panel_vendor.h>
#include <esp_log.h>
#include <mqtt_client.h>

#define TAG "ESP32S3SuperMiniBoard"

// ============================================================
// MQTT configuration
// ============================================================

#define MQTT_BROKER_URI "mqtt://broker.hivemq.com:1883"
#define MQTT_LED_TOPIC  "xiaozhi/test/slave/led"


class Esp32S3SuperMiniBoard : public WifiBoard {
private:
    Button boot_button_;

    i2c_master_bus_handle_t display_i2c_bus_ = nullptr;

    Display* display_ = nullptr;
    esp_lcd_panel_io_handle_t panel_io_ = nullptr;
    esp_lcd_panel_handle_t panel_ = nullptr;

    // MQTT
    esp_mqtt_client_handle_t mqtt_client_ = nullptr;
    bool mqtt_connected_ = false;
    bool mqtt_started_ = false;


    // ========================================================
    // Buttons
    // ========================================================

    void InitializeButtons()
    {
        boot_button_.OnClick([this]() {
            auto& app = Application::GetInstance();

            if (app.GetDeviceState() == kDeviceStateStarting) {
                EnterWifiConfigMode();
                return;
            }

            app.ToggleChatState();
        });

        boot_button_.OnLongPress([this]() {
            EnterWifiConfigMode();
        });
    }


    // ========================================================
    // OLED I2C
    // ========================================================

    void InitializeDisplayI2c()
    {
        i2c_master_bus_config_t bus_config = {
            .i2c_port = I2C_NUM_0,
            .sda_io_num = DISPLAY_SDA_PIN,
            .scl_io_num = DISPLAY_SCL_PIN,
            .clk_source = I2C_CLK_SRC_DEFAULT,
            .glitch_ignore_cnt = 7,
            .intr_priority = 0,
            .trans_queue_depth = 0,
            .flags = {
                .enable_internal_pullup = true,
            },
        };

        ESP_ERROR_CHECK(
            i2c_new_master_bus(
                &bus_config,
                &display_i2c_bus_
            )
        );
    }


    // ========================================================
    // SSD1306 OLED
    // ========================================================

    void InitializeSsd1306Display()
    {
        esp_lcd_panel_io_i2c_config_t io_config = {
            .dev_addr = 0x3C,
            .scl_speed_hz = 400 * 1000,
            .control_phase_bytes = 1,
            .dc_bit_offset = 6,
            .lcd_cmd_bits = 8,
            .lcd_param_bits = 8,
            .on_color_trans_done = nullptr,
            .user_ctx = nullptr,
            .flags = {
                .dc_low_on_data = 0,
                .disable_control_phase = 0,
            },
        };

        ESP_ERROR_CHECK(
            esp_lcd_new_panel_io_i2c(
                display_i2c_bus_,
                &io_config,
                &panel_io_
            )
        );

        ESP_LOGI(TAG, "Installing SSD1306 driver");

        esp_lcd_panel_dev_config_t panel_config = {};
        panel_config.reset_gpio_num = GPIO_NUM_NC;
        panel_config.bits_per_pixel = 1;

        esp_lcd_panel_ssd1306_config_t ssd1306_config = {
            .height = static_cast<uint8_t>(DISPLAY_HEIGHT),
        };

        panel_config.vendor_config = &ssd1306_config;

        ESP_ERROR_CHECK(
            esp_lcd_new_panel_ssd1306(
                panel_io_,
                &panel_config,
                &panel_
            )
        );

        ESP_LOGI(TAG, "SSD1306 driver installed");

        ESP_ERROR_CHECK(
            esp_lcd_panel_reset(panel_)
        );

        ESP_ERROR_CHECK(
            esp_lcd_panel_init(panel_)
        );

        ESP_LOGI(TAG, "Turning OLED display on");

        ESP_ERROR_CHECK(
            esp_lcd_panel_disp_on_off(
                panel_,
                true
            )
        );

        display_ = new OledDisplay(
            panel_io_,
            panel_,
            DISPLAY_WIDTH,
            DISPLAY_HEIGHT,
            DISPLAY_MIRROR_X,
            DISPLAY_MIRROR_Y
        );
    }


    // ========================================================
    // MQTT event handler
    // ========================================================

    static void MqttEventHandler(
        void* handler_args,
        esp_event_base_t base,
        int32_t event_id,
        void* event_data
    )
    {
        auto* board =
            static_cast<Esp32S3SuperMiniBoard*>(handler_args);

        if (event_id == MQTT_EVENT_CONNECTED) {

            board->mqtt_connected_ = true;

            ESP_LOGI(
                TAG,
                "MQTT connected"
            );
        }
        else if (event_id == MQTT_EVENT_DISCONNECTED) {

            board->mqtt_connected_ = false;

            ESP_LOGW(
                TAG,
                "MQTT disconnected"
            );
        }
    }


    // ========================================================
    // MQTT initialization
    // ========================================================

    void InitializeMqtt()
    {
        // Start MQTT only once. The MQTT client handles reconnects itself.
        if (mqtt_started_) {

            ESP_LOGI(
                TAG,
                "MQTT client already initialized"
            );

            return;
        }

        ESP_LOGI(
            TAG,
            "Initializing MQTT client"
        );

        esp_mqtt_client_config_t mqtt_config = {};

        mqtt_config.broker.address.uri =
            MQTT_BROKER_URI;

        mqtt_client_ =
            esp_mqtt_client_init(&mqtt_config);

        if (mqtt_client_ == nullptr) {

            ESP_LOGE(
                TAG,
                "Failed to initialize MQTT client"
            );

            return;
        }

        ESP_ERROR_CHECK(
            esp_mqtt_client_register_event(
                mqtt_client_,
                MQTT_EVENT_ANY,
                MqttEventHandler,
                this
            )
        );

        esp_err_t err =
            esp_mqtt_client_start(
                mqtt_client_
            );

        if (err != ESP_OK) {

            ESP_LOGE(
                TAG,
                "Failed to start MQTT client: %s",
                esp_err_to_name(err)
            );

            esp_mqtt_client_destroy(mqtt_client_);
            mqtt_client_ = nullptr;
            return;
        }

        mqtt_started_ = true;

        ESP_LOGI(
            TAG,
            "MQTT client started"
        );
    }


    // ========================================================
    // Register MCP tools
    // ========================================================

void InitializeMcpTools()
{
    auto& mcp_server =
        McpServer::GetInstance();

    mcp_server.AddTool(
        "turn_on_light",

        "Turn on the LED connected to the remote ESP8266 slave device.",

        PropertyList(),

        [this](const PropertyList&) -> ToolResult {

            if (mqtt_client_ == nullptr ||
                !mqtt_connected_) {

                return std::unexpected(
                    "MQTT is not connected"
                );
            }

            int message_id =
                esp_mqtt_client_publish(
                    mqtt_client_,
                    MQTT_LED_TOPIC,
                    "ON",
                    0,
                    1,
                    0
                );

            if (message_id < 0) {

                return std::unexpected(
                    "Failed to publish MQTT command"
                );
            }

            ESP_LOGI(
                TAG,
                "Published LED ON command, message id=%d",
                message_id
            );

            return std::string(
                "Remote LED turned ON"
            );
        }
    );

    mcp_server.AddTool(
        "turn_off_light",

        "Turn off the LED connected to the remote ESP8266 slave device.",

        PropertyList(),

        [this](const PropertyList&) -> ToolResult {

            if (mqtt_client_ == nullptr ||
                !mqtt_connected_) {

                return std::unexpected(
                    "MQTT is not connected"
                );
            }

            int message_id =
                esp_mqtt_client_publish(
                    mqtt_client_,
                    MQTT_LED_TOPIC,
                    "OFF",
                    0,
                    1,
                    0
                );

            if (message_id < 0) {

                return std::unexpected(
                    "Failed to publish MQTT command"
                );
            }

            ESP_LOGI(
                TAG,
                "Published LED OFF command, message id=%d",
                message_id
            );

            return std::string(
                "Remote LED turned OFF"
            );
        }
    );
}


public:

    // ========================================================
    // Constructor
    // ========================================================

    Esp32S3SuperMiniBoard()
        : boot_button_(
            BOOT_BUTTON_GPIO,
            false,
            5000
        )
    {
        InitializeDisplayI2c();

        InitializeSsd1306Display();

        InitializeButtons();

        // Register MCP tools.
        InitializeMcpTools();

        // Do NOT start MQTT from the constructor.
        // The network/TCP-IP stack must be ready first.
        // MQTT is started from OnNetworkConnected().
    }


    // ========================================================
    // Network connected hook
    // ========================================================

    void OnNetworkConnected() override
    {
        // At this point XiaoZhi has confirmed that the network is ready,
        // so it is safe to start the separate MQTT client.
        InitializeMqtt();
    }


    // ========================================================
    // Built-in LED
    // ========================================================

    virtual Led* GetLed() override
    {
        static SingleLed led(BUILTIN_LED_GPIO);
        return &led;
    }


    // ========================================================
    // Audio
    // ========================================================

    virtual AudioCodec* GetAudioCodec() override
    {
        static NoAudioCodecSimplex audio_codec(
            AUDIO_INPUT_SAMPLE_RATE,
            AUDIO_OUTPUT_SAMPLE_RATE,

            // Speaker
            AUDIO_I2S_SPK_GPIO_BCLK,
            AUDIO_I2S_SPK_GPIO_LRCK,
            AUDIO_I2S_SPK_GPIO_DOUT,
            I2S_STD_SLOT_LEFT,

            // Microphone
            AUDIO_I2S_MIC_GPIO_SCK,
            AUDIO_I2S_MIC_GPIO_WS,
            AUDIO_I2S_MIC_GPIO_DIN,
            I2S_STD_SLOT_LEFT
        );

        return &audio_codec;
    }


    // ========================================================
    // Display
    // ========================================================

    virtual Display* GetDisplay() override
    {
        return display_;
    }
};


DECLARE_BOARD(Esp32S3SuperMiniBoard);