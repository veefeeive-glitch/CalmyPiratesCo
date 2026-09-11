# Engine Dream Loop R1 — первый ограниченный цикл

2026-09-08. Два визуальных прохода по машинной зоне OceanFreighterR7; не завершение корабля, не новый gameplay и не приёмка игроком.

## Навык и эталон

Создан и проверен `calmy-dream-loop` в `C:/Users/Valerii/.codex/skills/calmy-dream-loop/SKILL.md`. Это собственная адаптация идеи [dream-loop](https://github.com/achimala/dream-loop), а не установка неизменённого upstream. Сохранены Unreal 5.8.2, Blender, first person и low-poly; добавлены границы короткой итерации, независимый критик, сохранение коллизий, реальные игровые кадры и проверка маршрута.

[Эталон](../../SourceAssets/References/EngineDream-R1-20260908/concept.png) создан встроенным imagegen из прежнего реального кадра. [Точный промпт и исходный разбор](../../SourceAssets/References/EngineDream-R1-20260908/CONCEPT.md), [снимок навыка](../../SourceAssets/References/EngineDream-R1-20260908/SKILL.snapshot.md). Эталон — сгенерированная иллюстрация, не screenshot работающей игры; дополнительные швы/болты из неё не реализованы. Использована только световая/материальная цель с сохранением существующей планировки.

## Применено в Unreal

- 18 существующих R7_Engine_Worklight: итог 325 lm вместо 1100, 5000 K, source radius 8 cm, source length 40 cm. Положение, attenuation 350 cm и shadows сохранены.
- Два новых material instances EngineDreamR1: свечение MAT_Screen ×0,12 и MAT_R4_MachineryLight ×0,10 на компоненте machinery. Исходные материалы не изменены.
- Локальный bounded PostProcessVolume, центр (-3950,0,590) cm, extents (1650,800,390) cm, blend radius 80 cm; итог exposure bias -0,25, bloom 0,12. Это зона ниже weather deck, не глобальная экспозиция/отключение отражений.
- Blender raycast подтвердил видимый пол: Deck_Tank_Top / MAT_Deck_Green_PBR, Z=2,08 m. Новый EngineDreamR2 root/parent/instance нейтрализует оттенок только внутри статической машинной зоны: X -5600..-2200, abs(Y)<950, Z<1000 cm. Original basecolor переводится в luminance × (0,48;0,56;0,57); вне области исходный цвет возвращается без изменения. Normal/roughness/текстуры и геометрия сохранены.
- Итого пять новых Unreal material assets, три назначения на существующих компонентах, один локальный PP volume. Никаких новых моделей или внешних скачиваний; Blender использован для read-only проверки master.

Первый проход дал слишком тёмные подходы и тёплое пятно. Независимый критик отметил это; во втором проходе exposure поднят с -0,8 до -0,25, температура с 4500 до 5000 K, свет уменьшен с 450 до 325 lm и исправлен оттенок настила. Предыдущие кадры сохранены, а не выданы за финальные.

## Проверки и результат

- Round 1 GPU: `Saved/Verification/ShipR7-Visual-20260908-095007`, 37 точек, 90,708 m, 642 dry checks, max joint 5,632 cm, resets/teleports=0.
- Round 2 GPU: `Saved/Verification/ShipR7-Visual-20260908-095948`, 37 точек, 90,752 m, 671 dry checks, max joint 5,945 cm при лимите 8 cm, resets/teleports=0. Семь кадров; обзор/мостик/машинная — три отдельные CameraActor-позиции, не физические телепорты игрока.
- Ship network повторён отдельно на обеих ревизиях: `20260908-095106-Ship` и финальный `20260908-100137-Ship`. Точные маркеры и роли сохранены в evidence; LateJoin проверяет scene/dry profile/grounded spawn, не третий обход.
- 575 прежних Content-файлов из integration и 42 material assets MetalR1 сверены: изменений 0, кроме отдельной карты. C++/DLL и Blender master прежние. Automation и полный старый набор маршрутов не повторялись. 60 FPS — настройка ограничения, не измеренный результат производительности.
- Критик после двух проходов: нейтральный настил LANDED визуально, баланс света и блики приборов PARTIAL. Наружные ракурсы без явной новой регрессии. Подходы светлее round 1, но темнее baseline; границы пола/стен читаемы. Только один engine-room camera view: полноценный многоракурсный осмотр внутри ещё открыт.

Остаются локальные hotspots пола/подплощадки, яркие края оранжевого штурвала и манометр, бледный cyan насоса, прежний пересвет мостика. Не объявлять свет законченным или процент готовности судна повышенным.

## Продолжение и сохранность

Следующий ограниченный цикл: несколько близких игровых ракурсов вокруг насоса и шкафов, согласование физических источников с корпусами светильников, затем локальные материалы и приборные детали. Не добавлять clutter, перекрывающий проходы.

Это post-import Unreal overrides. Blender master и GLB не обновлены; при пересоздании карты нужно сохранить назначения MetalR1B/EngineDreamR1/EngineDreamR2. Пространственная окраска EngineDreamR2 рассчитана на нынешний неподвижный R7: перед движением корабля перевести её в локальные координаты или отдельные material slots.

Backup до цикла и карта round1: `SourceAssets/Backups/Before-EngineDream-R1-20260908`. Финальный SHA256 карты: `cc86d82e9304ef95185289a945847a0d9fd92dd89bdc50bd3089c963298ef244`.

Первый технический apply остановился на assert: setter в UE 5.8 MaterialEditingLibrary.cpp меняет vector parameter, но возвращает false. Исправлено проверкой readback; исходная карта при ошибке не сохранялась. Скрипты одноразовые с hash guards, не запускать поверх поздней карты.

[Frozen delta evidence](../../Evidence/Gameplay/EngineDream-R1-20260908/acceptance.json) требует базовые ShipR7-Integration и ShipMetal-R1; это не самостоятельный полный проект.
