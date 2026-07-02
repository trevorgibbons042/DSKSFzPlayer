@echo off
cd /d "%~dp0"

:: Desactiva el paginador de Git (less.exe). Sin esto, comandos como
:: "git show --stat" pueden abrir un paginador que se queda esperando
:: una pulsacion de tecla, dando la falsa impresion de que el script
:: se ha quedado colgado justo antes de terminar.
set "GIT_PAGER=cat"
set "PAGER=cat"

echo ============================================
echo   DSK SFz Player - Sincronizar con GitHub
echo ============================================
echo.

where git >nul 2>&1
if errorlevel 1 (
    echo ! ERROR: no se encontro "git" en el PATH. Instala Git para Windows.
    echo.
    pause
    exit /b 1
)

echo [1/3] Comprobando estado del repositorio...
git status
echo.

set "MSG="
set /p "MSG=Mensaje del commit (o enter para fecha): "
if "%MSG%"=="" set "MSG=Update %date% %time%"

echo.
echo [2/3] Anadiendo y confirmando cambios locales...
git add -A
git commit -m "%MSG%"
if errorlevel 1 (
    echo   (sin cambios nuevos que confirmar; continuo con la sincronizacion)
)

echo.
echo [3/3] Sincronizando con el servidor (pull y push)...
git pull --rebase origin main
if errorlevel 1 (
    echo.
    echo ! ERROR: "git pull --rebase" fallo, probablemente por un conflicto.
    echo   Revisa "git status" para ver los archivos en conflicto, resuelvelos
    echo   y luego ejecuta:   git rebase --continue
    echo   o para cancelar:   git rebase --abort
    echo   NO se ha intentado el push.
    pause
    goto :fin
)

git push origin main
if errorlevel 1 (
    echo.
    echo ! ERROR: el push no se pudo completar. Revisa tu conexion o credenciales.
    pause
    goto :fin
)

echo.
echo ============================================
echo   HECHO.
echo ============================================
git --no-pager show --stat HEAD

:fin
echo.
set "OPENACT="
set /p "OPENACT=Abrir GitHub Actions para compilar? (s/N): "
if /i "%OPENACT%"=="S" start "" "https://github.com/dskmusic/DSKSFzPlayer/actions/workflows/build.yml"

echo.
echo Pulsa una tecla para cerrar esta ventana...
pause >nul
