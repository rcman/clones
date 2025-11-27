@echo off
REM IRIX Desktop Build Script for Windows (Enhanced)
REM Usage: build.bat [options]
REM Options:
REM   --run       Build and run the application
REM   --clean     Clean build (remove all .class files first)
REM   --jar       Create JAR file after compilation
REM   --help      Show this help message

setlocal enabledelayedexpansion

echo ======================================
echo IRIX Desktop Environment - Build Script
echo ======================================
echo.

REM Parse arguments
set RUN_AFTER=false
set CLEAN_BUILD=false
set CREATE_JAR=false

:parse_args
if "%1"=="" goto end_parse
if "%1"=="--run" set RUN_AFTER=true
if "%1"=="--clean" set CLEAN_BUILD=true
if "%1"=="--jar" set CREATE_JAR=true
if "%1"=="--help" goto show_help
shift
goto parse_args
:end_parse

REM Check if Java compiler is available
where javac >nul 2>nul
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: javac not found. Please install JDK
    echo Download from: https://www.oracle.com/java/technologies/downloads/
    exit /b 1
)

REM Display Java version
echo Java compiler version:
javac -version
echo.

REM Clean if requested
if "%CLEAN_BUILD%"=="true" (
    echo Cleaning old class files...
    del /Q *.class 2>nul
    del /Q IRIXDesktop.jar 2>nul
    echo Clean complete.
    echo.
)

REM Compile all Java files
echo Compiling Java files...
echo ----------------------

set COMPILED=0
set FAILED=0

for %%f in (
    IRIXWindow.java
    Calculator.java
    DrawingApp.java
    FileSystemBrowser.java
    MediaPlayer.java
    TerminalEmulator.java
    TextEditor.java
    WebBrowser.java
    WorkspaceManager.java
    StartMenu.java
    IRIXDesktop.java
) do (
    echo Compiling %%f...
    javac -Xlint:none %%f
    if !ERRORLEVEL! NEQ 0 (
        echo FAILED: %%f
        set /a FAILED+=1
    ) else (
        set /a COMPILED+=1
    )
)

echo.

if %FAILED% GTR 0 (
    echo ======================================
    echo Compilation FAILED!
    echo %FAILED% file(s) failed to compile.
    echo ======================================
    exit /b 1
)

echo ======================================
echo Compilation successful!
echo %COMPILED% file(s) compiled.
echo ======================================
echo.

REM Create JAR if requested
if "%CREATE_JAR%"=="true" (
    echo Creating JAR file...
    echo Main-Class: IRIXDesktop > manifest.txt
    jar cfm IRIXDesktop.jar manifest.txt *.class
    del manifest.txt
    echo JAR file created: IRIXDesktop.jar
    echo Run with: java -jar IRIXDesktop.jar
    echo.
)

REM Run if requested
if "%RUN_AFTER%"=="true" (
    echo Starting IRIX Desktop...
    echo.
    java IRIXDesktop
) else (
    echo To run the application:
    echo   java IRIXDesktop
    echo.
    echo Or run this script with --run flag:
    echo   build.bat --run
)

goto :eof

:show_help
echo Usage: build.bat [options]
echo Options:
echo   --run       Build and run the application
echo   --clean     Clean build (remove all .class files first)
echo   --jar       Create JAR file after compilation
echo   --help      Show this help message
exit /b 0
