#include "control.h"


bool is_right = false;
bool is_left = false;

void receive_control(uint8_t* data, uint16_t len ){
    //Bluetooth spp serial handler function

    if(strncmp("Maju",(char *)data,strlen("Maju"))==0){
        ESP_LOGI(CONTROL_TAG,"Maju");\
        gpio_set_level(MOTORA_IN1,1);
        gpio_set_level(MOTORA_IN2,0);
        gpio_set_level(MOTORB_IN3,0);
        gpio_set_level(MOTORB_IN4,1);
        mcpwm_comparator_set_compare_value(comparator_1,5000); // Kecepatan Motor 1 Maju
        mcpwm_comparator_set_compare_value(comparator_2,5000); // Kecepatan Motor 2 Maju
    }
    else if (strncmp("Mundur",(char *)data,strlen("Mundur"))==0){ 
        ESP_LOGI(CONTROL_TAG,"Mundur");
        gpio_set_level(MOTORA_IN1,0);
        gpio_set_level(MOTORA_IN2,1);
        gpio_set_level(MOTORB_IN3,1);
        gpio_set_level(MOTORB_IN4,0);
        mcpwm_comparator_set_compare_value(comparator_1,5000); // Kecepatan Motor 1 Mundur
        mcpwm_comparator_set_compare_value(comparator_2,5000); // Kecepatan Motor 2 Mundur
    }
    else if (strncmp("Kiri",(char *)data,strlen("Kiri"))==0){
        if (!is_left | is_right)
        {
            mcpwm_comparator_set_compare_value(comparator_1,5000); // Kecepatan Motor 1 Belok Kiri
            mcpwm_comparator_set_compare_value(comparator_2,2000); // Kecepatan Motor 2 Belok Kiri
            is_left = true;
        }else
        {
            mcpwm_comparator_set_compare_value(comparator_1,5000); // Kecepatan Motor 1 Maju setelah belok 
            mcpwm_comparator_set_compare_value(comparator_2,5000); // Kecepatan Motor 2 Maju setelah belok 
            is_left = false;
        }
        
    }
    else if (strncmp("Kanan",(char *)data,strlen("Kanan"))==0){ 
        if (is_left | !is_right)
        {
             ESP_LOGI(CONTROL_TAG,"Kanan");
            mcpwm_comparator_set_compare_value(comparator_1,2000); // Kecepatan Motor 1 Belok Kanan
            mcpwm_comparator_set_compare_value(comparator_2,5000); // Kecepatan Motor 2 Belok Kanan
            is_right = true;
        }else
        {
            mcpwm_comparator_set_compare_value(comparator_1,5000); // Kecepatan Motor 1 Maju setelah Belok
            mcpwm_comparator_set_compare_value(comparator_2,5000); // Kecepatan Motor 1 Maju setelah Belok
            is_right = false;
        }
    }
    else if (strncmp("Stop",(char *)data,strlen("Stop"))==0){ 
        //Break by motor driver comparator untuk pwm tetap menyala (agar lebih cepat untuk maju atau mundur kembali)
        gpio_set_level(MOTORA_IN1,1);
        gpio_set_level(MOTORA_IN2,1);
        gpio_set_level(MOTORB_IN3,1);
        gpio_set_level(MOTORB_IN4,1);
        mcpwm_comparator_set_compare_value(comparator_1,5000); 
        mcpwm_comparator_set_compare_value(comparator_2,5000);
    }
    else if (strncmp("Mode",(char *)data,strlen("Mode"))==0){ 
        // Handle switching mode antara control by bluetooth dan line follow
        is_line_follow = !is_line_follow;
        gpio_set_level(MOTORA_IN1,1);
        gpio_set_level(MOTORA_IN2,0);
        gpio_set_level(MOTORB_IN3,0);
        gpio_set_level(MOTORB_IN4,1);
    }
    else
    {
        ESP_LOGI(CONTROL_TAG,"Command Not found");
        ESP_LOGI(CONTROL_TAG,"get string |%s", (char *)data );
    }
    
}