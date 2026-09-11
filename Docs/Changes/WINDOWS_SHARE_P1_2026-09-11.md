# Windows share P1 — отдельный ZIP для друга

Подготовлена самостоятельная Windows x64 Development-сборка текущей игры на UE 5.8.2. Архив содержит программу, cooked карты/ресурсы, зависимости, Visual C++ x64 redistributable, русскую инструкцию, запуск сразу на корабль и attribution/licensing notices. Unreal Editor другу не нужен. Пользователь сам отправляет ZIP; загрузка в облако и сообщения другим людям не выполнялись.

## Артефакт

- [CalmyPiratesCo-Windows-2026-09-11.zip](../../../CalmyPiratesCo-Windows-2026-09-11.zip) — отдельный файл в `E:/Game`, вне папки проекта. 468759551 байт (447.0 MiB), SHA256 `1ac1dc405401b21ba077f4d7530fafeb82ef5445ed59c150b376c07a7fe59d63`. Рядом записан `.zip.sha256`.
- Распаковать целиком и запустить `CalmyPiratesCo.exe`. Обычный старт: WaterLab → обучение → OceanFreighterR7. `Play-Ship.cmd` пропускает обучение. Настройки — Esc → Настройки.
- Кандидат: `Builds/Share-Windows-20260911-172821/Windows`. Это новая папка; `Windows-Current` и ярлык editor-game не заменялись.
- Проверяемая распакованная копия: `Saved/Verification/Package-Share-Extracted-20260911-172821`.
- Основной EXE: SHA256 `5239b2ea6db992b9e5b26e8bcb570b3f393c70dff2e971043e629c4e3ad9995a`. Editor DLL S1 build7 осталась `dd75bb3120d62900cdb9e2b1c0d3cf0505dc0229a620b98f45b9c7f646e3cf92`.

Версия включает настройки/оптимизацию S1, автономный рой R6, память/мозг и взаимодействия R4/R5. Карты/модели не изменялись: OceanFreighterR7 и WaterLab прежние, персонажи CrewR6B. Blender R8 не импортирован. Это текущий standalone playtest, не Shipping-релиз и не новый human acceptance.

## Исправление cook и проверки

Первый `BuildCookRun` успешно собрал C++ и упаковал карты, однако просмотр GPU обнаружил отсутствующие модели персонажей/грузов и исходный экран PROPULSION вместо меню рубки. Эти объекты загружаются `LoadObject` из строк и не входят в обычный граф ссылок карты.

В [DefaultGame.ini](../../Config/DefaultGame.ini) добавлены `DirectoriesToAlwaysCook` для `/Game/Characters/CrewR6B`, `/Game/Cargo/R3`, `/Game/UI/BridgeMenu`, а также `MapsToCook` для OceanFreighterR7. Проверены все `LoadObject`/soft loading вызовы игрового кода. Повторный cook использовал тот же standalone EXE; C++/игровые механики не менялись. Первый неполный ZIP перемещён в `Builds/Share-Windows-20260911-171546/Rejected-incomplete-assets.zip`, не предназначен для отправки.

Новый [run_packaged_share_review.ps1](../../Scripts/run_packaged_share_review.ps1) запускает именно распакованный EXE, GPU обучение/переход и настройки; игровые `SkipPackage`/missing-object warnings теперь делают проверку неуспешной. [run_network_smoke.ps1](../../Scripts/run_network_smoke.ps1) для `PackagedExe` использует отдельный `UserDir` каждой роли и также отвергает отсутствующий игровой контент. Пользовательские настройки, память роя и тестовые сохранения не кладутся в ZIP.

| Проверка | Результат |
|---|---|
| Standalone C++ build | PASS, [первый build/cook log](../../Saved/Logs/Package-Share-20260911-171546.log), 69 compile/link actions, 274.11 с build |
| Исправленный cook/stage/archive | PASS, [лог](../../Saved/Logs/Package-Share-20260911-172821.log), 49.31 с BuildCookRun |
| ZIP integrity / extraction | **PASS**, [7-Zip test](../../Builds/Share-Windows-20260911-172821/Zip-test.log); все 50 распакованных файлов совпали с исходным кандидатом по SHA256 |
| GPU из распакованного ZIP | **PASS**, [логи и 13 кадров](../../Saved/Verification/Package-Share-Review-20260911-173019). Tutorial 121.6 см после resume, 4 груза/1 helper, без rescue. Settings — все пять вкладок, remapping/сохранение/rollback/Keep/resume. Просмотрены старт с моделями, настоящий экран меню, ходьба в рубке, экран настроек и сохранённый видеорежим; `material=1`, отсутствующих /Game assets нет |
| Tutorial Host/Client/LateJoin из ZIP | **Host/Client/LateJoin PASS**, [логи](../../Saved/Verification/20260911-173114-Tutorial). Командный переход, resume, ходьба 121.3/121.3/121.4 см, 4 груза/1 helper/0 resets; missing-assets warnings отсутствуют |

Первые логические Tutorial/Settings/network маркеры PASS не означали корректную комплектацию — screenshot и missing-assets warnings их опровергли. Первый GPU runner также имел PowerShell parser error до запуска; исправлен перенос `-or`, затем сценарий запущен. Эти ранние результаты сохраняются отдельно от финальных. Новая engine automation не запускалась: gameplay C++ не менялся; S1 build7 42/42 — предыдущее доказательство логики, а cook-комплект проверяется новыми packaged прогонами.

Проверка выполнена на компьютере разработчика, сетевые процессы — loopback. Другой ПК, интернет-подключение, Steam invitations, Shipping и полный игровой рейс не проверялись. Правила Windows Firewall и MCP не изменялись. Содержимое `Windows-Current` по-прежнему старое, текущий файл для отправки — отдельный ZIP выше.

## Комплект и происхождение

В корне ZIP: `Readme-RU.txt`, `Play-Ship.cmd`, `Credits.txt`, `Licenses`. Фактически UE 5.8 пакует `Engine/Extras/Redist/en-us/vc_redist.x64.exe`; инструкция указывает этот существующий файл. Установщик не запускался на компьютере пользователя.

Сохранены MIT notice DesktopFly и CC BY-NC 4.0 attribution FlyWire (668/18968, upstream commit из локального provenance). Добавлены credits Kenney/Poly Haven/ambientCG и локальный [Poly Haven license record](../../SourceAssets/ThirdParty/PolyHaven/LICENSE-PROVENANCE-2026-09-11.md), проверенный по официальной странице. Исходные данные/редактор/Blender/личные профили/логи и `.pdb` в ZIP не входят; граф встроен в игровой EXE.

Следующий безопасный шаг — пользователь отправляет архив другу; друг распаковывает всю папку и запускает EXE. Для ошибок первого запуска нужен лог именно с компьютера друга, не перенос прежних локальных PASS.

Cook нормализовал формат editor GameUserSettings.ini без изменения строк/значений (Compare-Object пуст). Исходные байты восстановлены после сохранения диагностической копии; итоговый SHA256 пользовательского ini снова a16b2c2a4730eee0200049754e1d572710bdf3338e8314e5a7ed9f830eef0973.

Frozen manifest / логи / кадры / конфигурация: [Windows-Share-P1-20260911](../../Evidence/Gameplay/Windows-Share-P1-20260911). Сам ZIP и полный кандидат хранятся по путям выше, их хеши записаны в manifest; Evidence не является второй полной копией ZIP.
