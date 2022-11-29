#include "UnitempViews.h"
#include "unitemp_icons.h"
static View* view;

static uint8_t sensor_index = 0;
static char buff[7];

static void _draw_temperature(Canvas* canvas, Sensor* sensor, uint8_t x, uint8_t y, Color color) {
    //Рисование рамки
    canvas_draw_rframe(canvas, x, y, 54, 20, 3);

    if(color == ColorBlack) {
        canvas_draw_rbox(canvas, x, y, 54, 19, 3);
        canvas_invert_color(canvas);
    } else {
        canvas_draw_rframe(canvas, x, y, 54, 19, 3);
    }

    int16_t temp_int = sensor->temp;
    int8_t temp_dec = abs((int16_t)(sensor->temp * 10) % 10);

    //Рисование иконки
    canvas_draw_icon(
        canvas, x + 3, y + 3, (app->settings.unit == CELSIUS ? &I_temp_C_11x14 : &I_temp_F_11x14));

    if((int16_t)sensor->temp == -128 || sensor->status == UT_TIMEOUT) {
        snprintf(buff, 5, "--");
        canvas_set_font(canvas, FontBigNumbers);
        canvas_draw_str_aligned(canvas, x + 27, y + 10, AlignCenter, AlignCenter, buff);
        snprintf(buff, 4, ". -");
        canvas_set_font(canvas, FontPrimary);
        canvas_draw_str_aligned(canvas, x + 50, y + 10 + 3, AlignRight, AlignCenter, buff);
        if(color == ColorBlack) canvas_invert_color(canvas);
        return;
    }

    //Целая часть температуры
    snprintf(buff, 7, "%d", temp_int);
    canvas_set_font(canvas, FontBigNumbers);
    canvas_draw_str_aligned(
        canvas, x + 27 + ((temp_int <= -10) ? 5 : 0), y + 10, AlignCenter, AlignCenter, buff);
    //Печать дробной части температуры в диапазоне от -9 до 99 (когда два знака в числе)
    if(temp_int > -10 && temp_int <= 99) {
        uint8_t int_len = canvas_string_width(canvas, buff);
        snprintf(buff, 4, ".%d", temp_dec);
        canvas_set_font(canvas, FontPrimary);
        canvas_draw_str(canvas, x + 27 + int_len / 2 + 2, y + 10 + 7, buff);
    }
    if(color == ColorBlack) canvas_invert_color(canvas);
}

static void _draw_humidity(Canvas* canvas, Sensor* sensor, const uint8_t pos[2]) {
    //Рисование рамки
    canvas_draw_rframe(canvas, pos[0], pos[1], 54, 20, 3);
    canvas_draw_rframe(canvas, pos[0], pos[1], 54, 19, 3);

    //Рисование иконки
    canvas_draw_icon(canvas, pos[0] + 3, pos[1] + 2, &I_hum_9x15);

    if((int8_t)sensor->hum == -128 || sensor->status == UT_TIMEOUT) {
        snprintf(buff, 5, "--");
        canvas_set_font(canvas, FontBigNumbers);
        canvas_draw_str_aligned(canvas, pos[0] + 27, pos[1] + 10, AlignCenter, AlignCenter, buff);
        snprintf(buff, 4, ". -");
        canvas_set_font(canvas, FontPrimary);
        canvas_draw_str_aligned(
            canvas, pos[0] + 50, pos[1] + 10 + 3, AlignRight, AlignCenter, buff);
        return;
    }

    //Целая часть влажности
    snprintf(buff, 5, "%d", (uint8_t)sensor->hum);
    canvas_set_font(canvas, FontBigNumbers);
    canvas_draw_str_aligned(canvas, pos[0] + 27, pos[1] + 10, AlignCenter, AlignCenter, buff);
    uint8_t int_len = canvas_string_width(canvas, buff);
    //Единица измерения
    canvas_set_font(canvas, FontPrimary);
    canvas_draw_str(canvas, pos[0] + 27 + int_len / 2 + 4, pos[1] + 10 + 7, "%");
}

static void _draw_pressure(Canvas* canvas, Sensor* sensor) {
    const uint8_t x = 29, y = 39;
    //Рисование рамки
    canvas_draw_rframe(canvas, x, y, 69, 20, 3);
    canvas_draw_rframe(canvas, x, y, 69, 19, 3);

    //Рисование иконки
    canvas_draw_icon(canvas, x + 3, y + 4, &I_pressure_7x13);

    //Давление
    snprintf(buff, 6, "%d", (uint16_t)sensor->pressure);
    canvas_set_font(canvas, FontBigNumbers);
    canvas_draw_str_aligned(canvas, x + 30, y + 10, AlignCenter, AlignCenter, buff);
    //Единица измерения
    canvas_draw_icon(canvas, x + 50, y + 3, &I_mm_hg_17x15);
}

static void _draw_singleSensor(Canvas* canvas, Sensor* sensor, const uint8_t pos[2], Color color) {
    canvas_set_font(canvas, FontPrimary);

    const uint8_t max_width = 61;

    char sensor_name[12] = {0};
    memcpy(sensor_name, sensor->name, 10);

    if(canvas_string_width(canvas, sensor_name) > max_width) {
        uint8_t i = 10;
        while((canvas_string_width(canvas, sensor_name) > max_width - 6) && (i != 0)) {
            sensor_name[i--] = '\0';
        }
        sensor_name[++i] = '.';
        sensor_name[++i] = '.';
    }

    canvas_draw_str_aligned(
        canvas, pos[0] + 27, pos[1] + 3, AlignCenter, AlignCenter, sensor_name);
    _draw_temperature(canvas, sensor, pos[0], pos[1] + 8, color);
}

static void _draw_view_noSensors(Canvas* canvas) {
    canvas_draw_str(canvas, 0, 24, "Sensors not found");
}

static void _draw_view_sensorsList(Canvas* canvas) {
    //Текущая страница
    uint8_t page = sensor_index / 4;
    //Количество датчиков, которые будут отображаться на странице
    uint8_t page_sensors_count;
    if((app->sensors_count - page * 4) / 4) {
        page_sensors_count = 4;
    } else {
        page_sensors_count = (app->sensors_count - page * 4) % 4;
    }

    //Количество страниц
    uint8_t pages = app->sensors_count / 4 + (app->sensors_count % 4 ? 1 : 0);

    //Стрелка вверх
    if(page > 0) {
        canvas_draw_icon(canvas, 60, 2, &I_arrow_up_5x9);
    }
    //Стрелка вниз
    if(pages > 0 && page < pages - 1) {
        canvas_draw_icon(canvas, 60, 56, &I_arrow_down_5x9);
    }

    const uint8_t value_positions[][4][2] = {
        {{36, 18}}, //1 датчик
        {{4, 18}, {70, 18}}, //2 датчика
        {{4, 3}, {70, 3}, {37, 33}}, //3 датчика
        {{4, 3}, {70, 3}, {4, 33}, {70, 33}}}; //4 датчика
    //Рисование рамки
    canvas_draw_rframe(canvas, 0, 0, 128, 63, 7);
    canvas_draw_rframe(canvas, 0, 0, 128, 64, 7);
    for(uint8_t i = 0; i < page_sensors_count; i++) {
        _draw_singleSensor(
            canvas,
            app->sensors[page * 4 + i],
            value_positions[page_sensors_count - 1][i],
            (i == sensor_index % 4 ? ColorBlack : ColorWhite));
    }
}

static void _draw_view_sensorsCarousel(Canvas* canvas) {
    static const uint8_t temp_positions[3][2] = {{37, 23}, {37, 16}, {9, 16}};
    static const uint8_t hum_positions[2][2] = {{37, 38}, {65, 16}};
    //Рисование рамки
    canvas_draw_rframe(canvas, 3, 0, 122, 63, 7);
    canvas_draw_rframe(canvas, 3, 0, 122, 64, 7);

    //Печать имени
    canvas_set_font(canvas, FontPrimary);
    canvas_draw_str_aligned(
        canvas, 64, 7, AlignCenter, AlignCenter, app->sensors[sensor_index]->name);
    //Подчёркивание
    uint8_t line_len = canvas_string_width(canvas, app->sensors[sensor_index]->name) + 2;
    canvas_draw_line(canvas, 64 - line_len / 2, 12, 64 + line_len / 2, 12);

    //Стрелка вправо
    if(unitemp_sensors_getTypesCount() > 0 && sensor_index < unitemp_sensors_getCount() - 1) {
        canvas_draw_icon(canvas, 117, 28, &I_arrow_right_5x9);
    }
    //Стрелка влево
    if(sensor_index > 0) {
        canvas_draw_icon(canvas, 6, 28, &I_arrow_left_5x9);
    }

    if(app->sensors[sensor_index]->status == UT_TIMEOUT) {
        const Icon* frames[] = {&I_happy_2_78x46, &I_happy_78x46, &I_sad_78x46};
        canvas_draw_icon(canvas, 24, 15, frames[furi_get_tick() % 2250 / 750]);
        return;
    }

    //Селектор значений для отображения
    switch(app->sensors[sensor_index]->type->datatype) {
    case UT_DATA_TYPE_TEMP:
        _draw_temperature(
            canvas,
            app->sensors[sensor_index],
            temp_positions[0][0],
            temp_positions[0][1],
            ColorWhite);
        break;
    case UT_DATA_TYPE_TEMP_HUM:
        _draw_temperature(
            canvas,
            app->sensors[sensor_index],
            temp_positions[1][0],
            temp_positions[1][1],
            ColorWhite);
        _draw_humidity(canvas, app->sensors[sensor_index], hum_positions[0]);
        break;
    case UT_DATA_TYPE_TEMP_PRESS:
        _draw_temperature(
            canvas,
            app->sensors[sensor_index],
            temp_positions[1][0],
            temp_positions[1][1],
            ColorWhite);
        _draw_pressure(canvas, app->sensors[sensor_index]);
        break;
    case UT_DATA_TYPE_TEMP_HUM_PRESS:
        _draw_temperature(
            canvas,
            app->sensors[sensor_index],
            temp_positions[2][0],
            temp_positions[2][1],
            ColorWhite);
        _draw_humidity(canvas, app->sensors[sensor_index], hum_positions[1]);
        _draw_pressure(canvas, app->sensors[sensor_index]);
        break;
    }
}

static void _draw_callback(Canvas* canvas, void* _model) {
    UNUSED(_model);

    app->sensors_ready = true;

    uint8_t sensors_count = unitemp_sensors_getCount();

    if(sensors_count == 0) {
        _draw_view_noSensors(canvas);
    }
    if(sensors_count > 0) {
        _draw_view_sensorsList(canvas);
    }
    if(0) _draw_view_sensorsCarousel(canvas);
}

static bool _input_callback(InputEvent* event, void* context) {
    Unitemp* app = context;

    //Выход по короткому нажатию "назад"
    if(event->key == InputKeyBack && event->type == InputTypeShort) {
        app->processing = false;
    }

    //Пролистывание списка по короткому нажатию "вниз"
    if(event->key == InputKeyDown && event->type == InputTypeShort) {
        if(++sensor_index >= unitemp_sensors_getCount()) sensor_index = 0;
    }
    //Пролистывание списка по короткому нажатию "вверх"
    if(event->key == InputKeyUp && event->type == InputTypeShort) {
        if(--sensor_index >= unitemp_sensors_getCount())
            sensor_index = unitemp_sensors_getCount() - 1;
    }

    //Пролистывание карусели по короткому нажатию "право"
    if(event->key == InputKeyRight && event->type == InputTypeShort) {
        if(++sensor_index >= unitemp_sensors_getCount()) sensor_index = 0;
    }
    //Пролистывание карусели по короткому нажатию "лево"
    if(event->key == InputKeyLeft && event->type == InputTypeShort) {
        if(--sensor_index >= unitemp_sensors_getCount())
            sensor_index = unitemp_sensors_getCount() - 1;
    }
    //Вход в главное меню по короткому нажатию "ок"
    if(event->key == InputKeyOk && event->type == InputTypeShort) {
        app->sensors_ready = false;
        unitemp_MainMenu_switch();
    }
    //Редактирование датчика при длинном нажатии "ок"
    if(event->key == InputKeyOk && event->type == InputTypeLong) {
        app->sensors_ready = false;
        unitemp_SensorEdit_switch(app->sensors[sensor_index]);
    }

    return true;
}

void unitemp_General_alloc(void) {
    view = view_alloc();
    view_set_context(view, app);
    view_set_draw_callback(view, _draw_callback);
    view_set_input_callback(view, _input_callback);

    view_dispatcher_add_view(app->view_dispatcher, GENERAL_VIEW, view);
}

void unitemp_General_switch(void) {
    app->sensors_ready = true;
    view_dispatcher_switch_to_view(app->view_dispatcher, GENERAL_VIEW);
}

void unitemp_General_free(void) {
    view_free(view);
}