<p align="center">
  <img src="images/marlin-old-250.png" height="100"/>
  <img src="images/GRBL.png" height="100"/>
  <img src="images/RepRap.png" height="100"/>
  <img src="images/Open-source-hardware-logo.png" height="100"/>
  <img src="images/Am0k-logo.png" height="100"/>
</p>

<h1 align="center">STM32F401CCU6 UNI CNC board</h1>

<p align="center">
    <a href="/LICENSE"><img alt="GPL-V3.0 License" src="https://img.shields.io/github/license/marlinfirmware/marlin.svg"></a>
</p>

Проект открытой управляющей платы ЧПУ на основе МК STM32F401CCU6. Разработана для работы под управление прошивок Marlin и GRBL.

## Marlin 2.1.2

Плата поддерживает и протестированна с прошивкой на базе <a href="https://marlinfw.org/meta/download/Marlin">Marlin</a> 2.1.2. В репозитории вы можете найти как сами исходники прошивки под самосборную кинематику Core-XY, так и инструкцию по модицикации оригинальных исходников <a href="https://marlinfw.org/meta/download/Marlin">Marlin</a>. Для компиляции использовался <a href="https://code.visualstudio.com">Visual Studio Code</a> с установленным PlatformIO, перед сборкой выбираем окружение [env:blackpill_f401cc_env].

## GRBL 1.1f

Плата поддерживает и протестированна с прошивкой на базе <a href="https://github.com/grblHAL/STM32F4xx">GRBL HAL Driver</a>. В репозитории вы можете найти как сами исходники прошивки под самосборную 4-осевую кинематику (3 осевой фрезер + поворотная ось), так и инструкцию по модицикации оригинальных исходников <a href="https://github.com/grblHAL/STM32F4xx">GRBL HAL Driver</a>. Для компиляции использовался <a href="https://www.st.com/en/development-tools/stm32cubeide.html">STM32 Cube IDE</a> в режиме [Build:Release F401 Blackpill]. 

## Дополнительное ПО

Так же вам могут быть полезны следующие программы для заливки прошивки по DFU:
 * <a href="https://www.st.com/en/development-tools/stsw-stm32080.html">DfuSe USB device firmware upgrade</a>
 * <a href="https://www.st.com/en/development-tools/stm32cubeprog.html">STM32 Cube Programmer</a>

<p align="center">
  <img src="images/board_top.svg" height="300"/>
  <img src="images/board_bot.svg" height="300"/>
</p>

## Особенности

Схему печатной платы, а так же гербер-файлы, необходимые для ее производства, вы найдете в репозитории.
<img src="images/Schematic_Marlin_STM30F401CCU6_main.png"/>
Из особенностей следует отметить:
- Это минималистичная плата на основе современного и дешевого 32-битного МК STM32F401CCU6, для интеграции в нее подойдет
отладочная плата от WeAct, известная как BlackPill. Она дешевле и производительнее отладочных плат, основанных на
8-битном МК AtMega2560, рассчитаных на Ramps под управлением Marlin, и тем более намного производительнее плат на базе МК AtMega328
для работы с прошивкой GRBL.
- Питание платы осуществляется от блока питания 24 В, никаких дополнительных линий питания не требуется. 
- Питание нагревателя экструдера и нагревателя стола или небольшого коллекторного шпинделя осуществляется напряжением первичного питания. 
Вы можете подключить нагреваемый стол с потребляемым постоянным током до 15 А, заменив SMD предохранитель на плате.
По умолчанию установливается предохранитель на 12 А. В случае необходимости использовать внешнюю коммутацию 
(твердотельное реле нагревательного стола, мощный внешний драйвер шпинделя, драйвер лазера) на плате выведен
разъем TTL (3.3 В). Для управления бесколлекторными шпинделя, в драйвере которых предусмотрена аналоговая установка скорости вращения, на плате
установлен DAC с возможностью подстройки (R605) напряжения максимальных оборотов и соответствующий аналоговому уровню 0-10 В разъем RPM.
- Путем выбора установленных перемычек вы можете выбрать, подключать к МК цепочки измерения температуры для экструдера и стола (для Marlin - 
перемычки J00, J10), либо цепи опторазвязки концевика дополнительной 4 оси и Z-щупа (для GRBL - перемычки J01, J11). 
- Питание вентиляторов обдува нагревателя экструдера и обдува модели можно выбрать: 12 В или 24 В, установив джампер в соответствующее положение.
- Дополнительное питания 3.3 В, 5 В, 12В выведено с платы на соответствующие разъемы.
- Вы можете подключить лишь 4 драйвера шаговых двигателей, 3 для осей, 1 для экструдера в случае работы с прошивкой Marlin, 
4 для осей в случае работы с прошивкой GRBL.
- Обеспечена дополнительная защита МК: концевые выключатели установлены через опторазвязку, входы АЦП для термодатчиков экструдера и стола 
защищены от перенапряжения супрессорами, USB интерфейс защищен от электростатических разрядов. В прошивке представлен пример конфигурации 
концевых выключателей, когда выключатель "срабатывает" так же в случае его обрыва или отсоединения от платы, в качестве выключателя по Z 
использован индуктивный выключатель NPN с нормально замкнутым контактом LJ12A3-4-ZAX.
- Изменена входная цепочка измерения температуры. Подтягивающие резисторы установлены с сопротивлением 1kΩ, что обеспечивает большее количество 
отсчетов АЦП в диапазоне температур 80-300 C, а значит и большую точность поддержания заданной температуры.
К сожалению, это снижает количество отсчетов АЦП при температурах, близких к комнатным, однако, они не являются рабочими.

## Лицензия

Все исходники публикуются под [GPL license](/LICENSE). Ответственность за их использование целиком и полностью лежит на вас. Я верю в открытую разработку силами энтузиастов, и прошу вас так же делиться своими наработками. Если же вы собираетесь использовать эти наработки в закрытом виде или защищенном патентом виде, прошу вас выбрать другие источники.