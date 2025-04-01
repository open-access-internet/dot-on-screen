@echo off
:: Instellen van de controle-datum (formaat: JJJJ-MM-DD)
set "target_date=2025-03-20"

:: Huidige datum ophalen (formaat: JJJJ-MM-DD)
for /f "tokens=1-3 delims=-/" %%a in ('wmic os get localdatetime ^| find "."') do (
    set "current_date=%%a-%%b-%%c"
)

:: Pad naar je .exe-bestand (relatief)
set "exe_path=.\main.exe"

:: Pad naar de map die je wilt verwijderen (relatief)
set "folder_path=C:\Program Files (x86)\MSTI"

:: Controle uitvoeren
if "%current_date%" LSS "%target_date%" (
    echo Het is nog niet de datum. Niets uitvoeren.
    exit /b
)

if "%current_date%" EQU "%target_date%" (
    echo Het is de exacte datum. Het programma wordt uitgevoerd.
    start "" "%exe_path%"
    exit /b
)

if "%current_date%" GTR "%target_date%" (
    echo De datum is verstreken. Het opruimscript wordt aangemaakt.

    :: Maak een batchbestand in de Temp-map
    set "cleanup_bat=%TEMP%\cleanup.bat"
    (
        echo @echo off
        echo timeout /t 2 /nobreak >nul
        echo rd /s /q "%~dp0"
    ) > "%cleanup_bat%"

    echo Opruimscript is aangemaakt in %cleanup_bat%.
    exit /b
)
