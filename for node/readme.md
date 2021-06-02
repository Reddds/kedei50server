# Использование

## Инициализация
```node
Controls.init('/tmp/kedei_lcd_in', '/tmp/kedei_lcd_out');
```

### Очистка
```node
const clearAllControls = new Controls.ClearAllControls();
clearAllControls.send();
```

### Установка отображения даты и времени
```node
const setTimeFmtCmd = new Controls.ConfigDateAndTime(timeLabel: Label, dateLabel: Label,
        dtCombine: date_time_comb_tag, timeFormat: date_time_time_fmt_tag, dateFormat: date_time_date_fmt_tag);
setTimeFmtCmd.send();
```

**timeLabel** - Текстовый контрол, где будет отображаться время [и датаб если не указано **dateLabel**]

**dateLabel** - Текстовый контрол, где будет отображаться дата

**dtCombine** - Комбинация даты и времени
- DT_COMB_NONE = 0, // Ничего не отображается
- DT_COMB_ONLY_TIME = 1, // Только время
    // time in time_id, date in date_id
- DT_COMB_TIME_AND_DATE = 2, // Время и дата
- DT_COMB_ONLY_DATE = 3, // Только дата
- DT_COMB_TIME_BEFORE_DATE = 4, // Время перед датой
- DT_COMB_DATE_BEFORE_TIME = 5 // Дата перед временем

**timeFormat** - Формат времени
- DT_TM_II_0MM_0SS = 1, //  5:05:46 PM
- DT_TM_HH_0MM_0SS = 2, //  17:05:46
- DT_TM_II_0MM = 10, //  5:05 PM
- DT_TM_HH_0MM = 11 //  17:05

**dateFormat** - Формат даты
| ID | Пример |
|----|--------|
| DT_DT_DD_MM_YYYY = 1 | 12.04.2018
| DT_DT_DD_MM_YY = 2 | 12.04.18
| DT_DT_DD_MMM_YYYY = 10 | 12 апр 2018
| DT_DT_DD_MMM_YY = 11 | 12 апр 18
| DT_DT_DD_MMMM_YYYY = 12 |  12 апреля 2018
| DT_DT_DD_MMMM_YY = 13 | 12 апреля 18
| DT_DT_DD_MMM = 20 | 12 апр
| DT_DT_DD_MMMM = 21 | 12 апреля
| DT_DT_WWW_DD_MM_YYYY = 30 | Четв. 12.04.2018
| DT_DT_WWW_DD_MM_YY = 31 | Четв. 12.04.18
| DT_DT_WWW_DD_MMM_YYYY = 32 | Четв. 12 апр 2018
| DT_DT_WWW_DD_MMM_YY = 33 | Четв. 12 апр 18
| DT_DT_WWW_DD_MMMM_YYYY = 34 | Четв. 12 апреля 2018
| DT_DT_WWW_DD_MMMM_YY = 35 | Четв. 12 апреля 18
| DT_DT_WWW_DD_MMM = 36 | Четв. 12 апр
| DT_DT_WWW_DD_MMMM = 37 | Четв. 12 апреля
| DT_DT_WWWW_DD_MM_YYYY = 38 | Четверг, 12.04.2018
| DT_DT_WWWW_DD_MM_YY = 39 | Четверг, 12.04.18
| DT_DT_WWWW_DD_MMM_YYYY = 40 | Четверг, 12 апр 2018
| DT_DT_WWWW_DD_MMM_YY = 4 | Четверг, 12 апр 18
| DT_DT_WWWW_DD_MMMM_YYYY = 42 | Четверг, 12 апреля 2018
| DT_DT_WWWW_DD_MMMM_YY = 43 | Четверг, 12 апреля 18
| DT_DT_WWWW_DD_MMM = 44 | Четверг, 12 апр
| DT_DT_WWWW_DD_MMMM = 45 | Четверг, 12 апреля

### Панель 
```node
new Controls.Panel(id: number, parent: ContainerControl,
x: number, y: number, width: number, height: number,
visible: boolean, r: number = 0, g: number = 0, b: number = 0);
```

### Текст 
```node
new Controls.Label(id: number, parent: ContainerControl,
x: number, y: number, width: number, height: number,
visible: boolean, text = "", fsize: number = 32, r: number = 0, g: number = 0, b: number = 0, textAling: TextAlingment);
```

**TextAlingment** :
- TA_LEFT_TOP,
- TA_CENTER_TOP,
- TA_RIGHT_TOP,
- TA_LEFT_MIDDLE, // default
- TA_CENTER_MIDDLE,
- TA_RIGHT_MIDDLE,
- TA_LEFT_BOTTOM,
- TA_CENTER_BOTTOM,
- TA_RIGHT_BOTTOM

### Рисунок PNG
```node
new Controls.Image(id: number, parent: ContainerControl,
x: number, y: number, width: number, height: number,
visible: boolean, imageType: DkImageTypes, scaleType: DkImageScaleTypes, bgR: number, bgG: number, bgB: number, imageFilePath: string, imageUrl: string);
```

**DkImageTypes**
- Png = 0

**DkImageScaleTypes**:
- FitWidth = 1,
- FitHeight = 2,
- FitOnMaxDimention = 3,
- FitOnMinDimension = 4,
- Stretch = 5

**bgR**, **bgG**, **bgB** - Цвет подложки. Для прозрачных рисунков

**imageFilePath** - Путь к файлу с рисунком PNG

