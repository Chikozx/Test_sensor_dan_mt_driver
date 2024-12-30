#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/gpio.h>
#include <esp_log.h>
#include "nvs.h"
#include "nvs_flash.h"
#include "main.h"
#include "spp_config.h"
#include "control.h"


bool is_line_follow = true;
mcpwm_cmpr_handle_t comparator_1 = NULL;
mcpwm_cmpr_handle_t comparator_2 = NULL;

void app_main() {

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    if((ret = esp_bluetooth_stack_init()) != ESP_OK){
        ESP_LOGE("Bluetooth INIT", "%s Bluetooth stacks INIT failed : %s Please restart !", __func__, esp_err_to_name(ret));
        return;
    }

    gpio_config_t sensor = {
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pin_bit_mask = (uint64_t) (0 | (1ULL << SENSOR1) | (1ULL<< SENSOR2) | (1ULL<< SENSOR3) |  (1ULL<< SENSOR4)),
    };

    gpio_config(&sensor);

    gpio_config_t motor = {
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pin_bit_mask = (uint64_t) (0 | (1ULL << MOTORA_EN) | (1ULL<< MOTORA_IN1) | (1ULL<< MOTORA_IN2)
                                      | (1ULL << MOTORB_EN) | (1ULL<< MOTORB_IN3) | (1ULL<< MOTORB_IN4)),
    };

    gpio_config(&motor);

    //MCPWM timer
    mcpwm_timer_handle_t timer = NULL;
    mcpwm_timer_config_t timer_config = {
        .group_id = 0,
        .clk_src = MCPWM_TIMER_CLK_SRC_DEFAULT,
        .resolution_hz = 1000000, //Timer Resolution 1Mhz
        .period_ticks = 20000,    //Timer count every 20000 ticks  ~0.02 Second / 2ms
        .count_mode = MCPWM_TIMER_COUNT_MODE_UP,
    };
    ESP_ERROR_CHECK(mcpwm_new_timer(&timer_config, &timer));

    //MCPWM Operator
    mcpwm_oper_handle_t oper = NULL;
    mcpwm_operator_config_t operator_config = {
        .group_id = 0, 
    };
    ESP_ERROR_CHECK(mcpwm_new_operator(&operator_config, &oper));

    ESP_LOGI("INIT", "Connect timer and operator");
    ESP_ERROR_CHECK(mcpwm_operator_connect_timer(oper, timer));

    ESP_LOGI("INIT", "Create comparator and generator from the operator");
    
    mcpwm_comparator_config_t comparator_config = {
        .flags.update_cmp_on_tez = true,
    };
    ESP_ERROR_CHECK(mcpwm_new_comparator(oper, &comparator_config, &comparator_1));
    ESP_ERROR_CHECK(mcpwm_new_comparator(oper, &comparator_config, &comparator_2));
    
    mcpwm_gen_handle_t generator_1 = NULL;
    mcpwm_gen_handle_t generator_2 = NULL;
    mcpwm_generator_config_t generator_config = {
        .gen_gpio_num = MOTORA_EN,
    };

    ESP_ERROR_CHECK(mcpwm_new_generator(oper, &generator_config, &generator_1));
    generator_config.gen_gpio_num = MOTORB_EN;
    ESP_ERROR_CHECK(mcpwm_new_generator(oper, &generator_config, &generator_2));

    // set the initial compare value, so that the servo will spin to the center position
    ESP_ERROR_CHECK(mcpwm_comparator_set_compare_value(comparator_1, 0));
    ESP_ERROR_CHECK(mcpwm_comparator_set_compare_value(comparator_2, 0));


    ESP_LOGI("INIT", "Set generator action on timer and compare event");
    // go high on counter empty
    ESP_ERROR_CHECK(mcpwm_generator_set_action_on_timer_event(generator_1,
                                                              MCPWM_GEN_TIMER_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, MCPWM_TIMER_EVENT_EMPTY, MCPWM_GEN_ACTION_HIGH)));
    // go low on compare threshold
    ESP_ERROR_CHECK(mcpwm_generator_set_action_on_compare_event(generator_1,
                                                                MCPWM_GEN_COMPARE_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, comparator_1, MCPWM_GEN_ACTION_LOW)));
    
    ESP_ERROR_CHECK(mcpwm_generator_set_action_on_timer_event(generator_2,
                                                              MCPWM_GEN_TIMER_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, MCPWM_TIMER_EVENT_EMPTY, MCPWM_GEN_ACTION_HIGH)));
    // go low on compare threshold
    ESP_ERROR_CHECK(mcpwm_generator_set_action_on_compare_event(generator_2,
                                                                MCPWM_GEN_COMPARE_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, comparator_2, MCPWM_GEN_ACTION_LOW)));

    ESP_LOGI("INIT", "Enable and start timer");
    ESP_ERROR_CHECK(mcpwm_timer_enable(timer));
    ESP_ERROR_CHECK(mcpwm_timer_start_stop(timer, MCPWM_TIMER_START_NO_STOP));

    gpio_set_level(MOTORA_IN1,1);
    gpio_set_level(MOTORA_IN2,0);

    gpio_set_level(MOTORB_IN3,0);
    gpio_set_level(MOTORB_IN4,1);

    while (1)
    {
        if (is_line_follow) // Metode pertama untuk line follow
        {
               // Dibawah untuk debug sensor print ke serial
               // printf("%d %d %d %d \n", gpio_get_level(SENSOR1),gpio_get_level(SENSOR2),gpio_get_level(SENSOR3),gpio_get_level(SENSOR4));

            if((!gpio_get_level(SENSOR3))&(!gpio_get_level(SENSOR2))){
                // Logic stop
                mcpwm_comparator_set_compare_value(comparator_1,0);
                mcpwm_comparator_set_compare_value(comparator_2,0);  
            }else if(gpio_get_level(SENSOR3)&(!gpio_get_level(SENSOR2)))
            {
                // Logic Belok Kanan / Kiri ?
                mcpwm_comparator_set_compare_value(comparator_1,5000);
                mcpwm_comparator_set_compare_value(comparator_2,1500); 

            }else if((!gpio_get_level(SENSOR3))&gpio_get_level(SENSOR2))
            {
                // Logic Belok Kanan / Kiri ?
                mcpwm_comparator_set_compare_value(comparator_1,1500);
                mcpwm_comparator_set_compare_value(comparator_2,5000);  
            }else
            {
                // Logic Maju jika tidak ada sensor yang aktif
                mcpwm_comparator_set_compare_value(comparator_1,3000);
                mcpwm_comparator_set_compare_value(comparator_2,3000);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}