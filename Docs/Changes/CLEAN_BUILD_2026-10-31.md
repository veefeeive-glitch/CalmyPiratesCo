# Чистая сборка публикационной копии — 31 октября 2026

Попытка Development Editor build из новой папки CalmyPiratesCo-GitHub остановилась до компиляции C++. UnrealBuildTool вернул код 6:

```text
Platform Win64 is not a valid platform to build. SDK validation failed:
  Sdk: not found. Required version 10.0.19041.0.
Result: Failed (OtherCompilationError)
```

На диске C: обнаружены Windows SDK 10.0.22621.0 и 10.0.26100.0; в обоих проверены Windows.h, kernel32.lib и rc.exe. При этом зарегистрированный KitsRoot10 указывает на F:/Program Files (x86)/Windows Kits/10/. Локальный код UE MicrosoftPlatformSDK.cs получает SDK root из этого реестрового ключа. Это указывает на проблему обнаружения установленного SDK после переноса среды; сама компиляция игры ещё не проверена.

Реестр, установка SDK и общие настройки Windows не менялись в задаче публикации. Следующий шаг для чистой сборки — восстановить корректную регистрацию Windows SDK через штатное обслуживание среды, затем повторить Scripts/Project.ps1 -Action Build и automation на новой DLL. Устанавливать SDK 19041 только по тексту нижней границы версии без проверки существующих SDK не требуется.

Полный журнал сохранён локально: E:/Game/Audit/2026-10-30/publish/clean-build.txt. Последние 61/61 automation относятся к существующей DLL от 16 сентября и прогону 30 октября. Успешная отправка Git/LFS не объявляется PASS чистой компиляции или новым готовым релизом.
