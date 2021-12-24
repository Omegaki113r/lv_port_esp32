/*
    You can either follow instruction from the https://github.com/lvgl/lv_port_esp32.git to clone the original repo and proceed with that 
    OR
    you can use my fork of the above repo to follow along. THe latest code will be in my repo
        this is the command you need to use to clone my repo
            git clone --recurse-submodules https://github.com/Omegaki113r/lv_port_esp32.git
*/

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_freertos_hooks.h"
#include "freertos/semphr.h"
#include "esp_system.h"
#include "driver/gpio.h"

/* Littlevgl specific */
#ifdef LV_LVGL_H_INCLUDE_SIMPLE
#include "lvgl.h"
#else
#include "lvgl/lvgl.h"
#endif

#include "lvgl_helpers.h"

/*********************
 *      DEFINES
 *********************/
#define TAG "demo"
#define LV_TICK_PERIOD_MS 1

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void lv_tick_task(void *arg);
static void guiTask(void *pvParameter);
static void create_demo_application(void);

/**********************
 *   APPLICATION MAIN
 **********************/
void app_main()
{
    xTaskCreatePinnedToCore(guiTask, "gui", 4096 * 2, NULL, 0, NULL, 1);
}

SemaphoreHandle_t xGuiSemaphore;

static void guiTask(void *pvParameter)
{

    (void)pvParameter;
    xGuiSemaphore = xSemaphoreCreateMutex();

    lv_init();

    lvgl_driver_init();

    lv_color_t *buf1 = heap_caps_malloc(DISP_BUF_SIZE * sizeof(lv_color_t), MALLOC_CAP_DMA);
    assert(buf1 != NULL);

    lv_color_t *buf2 = heap_caps_malloc(DISP_BUF_SIZE * sizeof(lv_color_t), MALLOC_CAP_DMA);
    assert(buf2 != NULL);

    static lv_disp_buf_t disp_buf;

    uint32_t size_in_px = DISP_BUF_SIZE;

    lv_disp_buf_init(&disp_buf, buf1, buf2, size_in_px);

    lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.flush_cb = disp_driver_flush;
    disp_drv.buffer = &disp_buf;
    lv_disp_drv_register(&disp_drv);

    /*
        For now let's comment this.
        below part is responsible for detecting touch events
    */
    // lv_indev_drv_t indev_drv;
    // lv_indev_drv_init(&indev_drv);
    // indev_drv.read_cb = touch_driver_read;
    // indev_drv.type = LV_INDEV_TYPE_POINTER;
    // lv_indev_drv_register(&indev_drv);

    const esp_timer_create_args_t periodic_timer_args = {
        .callback = &lv_tick_task,
        .name = "periodic_gui"};
    esp_timer_handle_t periodic_timer;
    ESP_ERROR_CHECK(esp_timer_create(&periodic_timer_args, &periodic_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(periodic_timer, LV_TICK_PERIOD_MS * 1000));

    create_demo_application();

    while (1)
    {
        vTaskDelay(pdMS_TO_TICKS(10 / portTICK_PERIOD_MS));
        if (pdTRUE == xSemaphoreTake(xGuiSemaphore, portMAX_DELAY))
        {
            lv_task_handler();
            xSemaphoreGive(xGuiSemaphore);
        }
    }

    free(buf1);
    free(buf2);
    vTaskDelete(NULL);
}

lv_obj_t *home_page;
lv_obj_t *home_page_background;

lv_obj_t *emergency_button;
lv_obj_t *emergency_button_label;

lv_obj_t *thermometer_img;
LV_IMG_DECLARE(thermometer_lvgl); // this is where we introduce the C file to the codebase

static void emergency_button_event_handler(lv_obj_t *obj, lv_event_t event)
{
    // let's handle the button press in a different day
    // but in here you need to put the code that needs to be run when the button is pressed
}

static void create_demo_application(void)
{
    // this is where i write code to display widgets like buttons and  sliders ....

    /*
        this custom style ensure the buttons will not have borders.
    */
    static lv_style_t no_border_style;
    lv_style_init(&no_border_style);
    lv_style_set_border_width(&no_border_style, LV_STATE_DEFAULT, 0);

    home_page = lv_obj_create(NULL, NULL);
    lv_obj_add_style(home_page, LV_OBJ_PART_MAIN, &no_border_style);
    lv_obj_set_size(home_page, 250, 330); // 250,330 stands for length and width of the home page. this home page is slightly bigger than the screen itself.

    home_page_background = lv_obj_create(home_page, NULL);
    lv_obj_set_size(home_page_background, 250, 330);
    lv_obj_align(home_page_background, home_page, LV_ALIGN_IN_TOP_LEFT, -5, -5);                                       // this is to ensure that there is no other background colors peek through
    lv_obj_set_style_local_bg_color(home_page_background, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, lv_color_make(0, 0, 0)); // lv_color_make(0, 0, 0) this represent the color black in RGB color space, if you want to make this white it should be changed to lv_color_make(255, 255, 255)
    lv_obj_add_style(home_page_background, LV_OBJ_PART_MAIN, &no_border_style);

    // now let's add a button to middle of the screen.
    emergency_button = lv_btn_create(home_page, NULL);
    lv_obj_set_click(emergency_button, true);
    lv_obj_set_size(emergency_button, 200, 40);
    lv_obj_align(emergency_button, home_page, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_event_cb(emergency_button, emergency_button_event_handler); // this is where put the function that needs to be executed when the button is pressed
    lv_obj_set_style_local_bg_color(emergency_button, LV_BTN_PART_MAIN, LV_STATE_DEFAULT, lv_color_make(0, 0, 255));
    lv_obj_add_style(emergency_button, LV_OBJ_PART_MAIN, &no_border_style);

    emergency_button_label = lv_label_create(emergency_button, NULL);
    lv_label_set_long_mode(emergency_button_label, LV_LABEL_LONG_EXPAND); // this ensure that text don't cut off even when the text is longer than the button
    lv_label_set_align(emergency_button_label, LV_LABEL_ALIGN_CENTER);
    lv_label_set_text(emergency_button_label, "Emergency");
    lv_obj_set_size(emergency_button_label, 84, 16); // this size ensure that the label size is less than the size of the button
    lv_obj_set_style_local_text_color(emergency_button_label, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, lv_color_make(255, 255, 255));
    lv_obj_add_style(emergency_button_label, LV_OBJ_PART_MAIN, &no_border_style);
    lv_obj_align(emergency_button_label, emergency_button, LV_ALIGN_CENTER, 0, 0); // this ensure the label is centered on the button. (0,0) stands for offset from center.

    thermometer_img = lv_img_create(home_page, NULL); // this is where we create the container to hold the image
    lv_img_set_src(thermometer_img, &thermometer_lvgl); // this is where we give the source of the image to the container
    // the below 2 lines are to change the color and the opacity of the image
    // the current background of the screen is set to black.
    // the image i copied from the internet was also black. therefore the image will not show up in the screen if we try to display it
    // therefose the following lines of code will change the color of the image to white. so that we can see it on the screen
    lv_obj_set_style_local_image_recolor(thermometer_img,LV_IMG_PART_MAIN,LV_STATE_DEFAULT,lv_color_make(255,255,255));
    lv_obj_set_style_local_image_recolor_opa(thermometer_img, LV_IMG_PART_MAIN, LV_STATE_DEFAULT, 255);
    lv_obj_set_size(thermometer_img, 50, 50);
    lv_obj_align(thermometer_img, home_page, LV_ALIGN_IN_TOP_LEFT, 10, 10); // this places the image on top left corner with an offset of 10,10


    // let's upload and see :-)
    // like this you will be able to add images to the UI in LVGL

    // please like and subscribe for more LVGL tutorials :-)
    // thanks 

    lv_scr_load(home_page);
}

static void lv_tick_task(void *arg)
{
    (void)arg;
    lv_tick_inc(LV_TICK_PERIOD_MS);
}
