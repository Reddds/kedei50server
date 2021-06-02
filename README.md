based on https://github.com/FREEWING-JP/RaspberryPi_KeDei_35_lcd_v50

# Запуск
`sudo ./dist/lcdout`
`sudo "./dist/lcdout" -r 180 -b "./dist/111.bmp"`


# Компиляция
`make`

# Контролы

## Общая структура 
Длина: 17

|       |Заголовок| ID | ID родителя | X | Y | Ширина | Высота | Видимость |
|-------|:-------:|:--:|:-----------:|:-:|:-:|:------:|:------:|:---------:|
| Длина |    4    | 2  |      2      | 2 | 2 |    2   |    2   |     1     |
| Смещение |  0   | 4  |      6      | 8 |10 |   12   |   14   |    16     |

## dpan - Панель
Signature: **dpan**

