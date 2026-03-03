# WaveShare_DisplayDriver_esp32_S3 480X800 RGB display
a driver and graphics library meant to be shared for people to learn how to make a library specifically with esp32 in the RGB protocol so we don't have to relay on LVGL for embbed systems and for people to fork and make their own

##notes:
- In the menuCofig you need to set the PSRAM speed to 80MHz or else you will get pixel miss alinement (the project currently work with only those examples witout changing anything)
- setting the bounce buffer size to [800*#] will reduce flicking and the timing should be played with
- if you remove the second frame buffer you will need to use a draw buffer to make drawing to the screen easier

Doument research scources for learning:
- (ESP32-S3-Touch-LCD-4.3 Device and device documention links [https://www.waveshare.com/wiki/ESP32-S3-Touch-LCD-4.3#Demo_3])
- (ESP-IDF LCD documentation [https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-reference/peripherals/lcd/index.html#_CPPv425esp_lcd_panel_draw_bitmap22esp_lcd_panel_handle_tiiiiPKv])
- (ESP-IDF RGB Interfaced LCD documentation[https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-reference/peripherals/lcd/rgb_lcd.html#_CPPv4N35esp_lcd_rgb_panel_event_callbacks_t15on_bounce_emptyE]
- (general firmware devolpment [https://www.youtube.com/watch?v=TzhG82MGNaY&t=1165s])
- (createing a frame buffer [https://www.youtube.com/watch?v=5cp2iPGWmUY&list=PLFP-9JHwUqPE13Wl8PemhzE9FtoqSrX4g&index=8])
- (open gl How to courses *great for learning how graphic drivers work and potentually writing your own* [https://www.youtube.com/@uofmintroductiontocomputer5167/playlists])

# Disclaimer:
  this code is being used for a personal project but i wantged to make the graphics driver open scource so other people code learn how to make their own and use this code as a study guide/ example
