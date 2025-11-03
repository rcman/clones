@echo off
REM IRIX Desktop Build Script for Windows
REM This script compiles all Java files and runs the application

echo ======================================
echo IRIX Desktop Environment - Build Script
echo ======================================
echo.

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

REM Clean old class files
echo Cleaning old class files...
del /Q *.class 2>nul
echo.

REM Compile all Java files
echo Compiling Java files...
echo ----------------------

echo Compiling IRIXWindow.java...
javac IRIXWindow.java
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: Failed to compile IRIXWindow.java
    exit /b 1
)

echo Compiling Calculator.java...
javac Calculator.java
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: Failed to compile Calculator.java
    exit /b 1
)

echo Compiling DrawingApp.java...
javac DrawingApp.java
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: Failed to compile DrawingApp.java
    exit /b 1
)

echo Compiling FileSystemBrowser.java...
javac FileSystemBrowser.java
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: Failed to compile FileSystemBrowser.java
    exit /b 1
)

echo Compiling MediaPlayer.java...
javac MediaPlayer.java
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: Failed to compile MediaPlayer.java
    exit /b 1
)

echo Compiling TerminalEmulator.java...
javac TerminalEmulator.java
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: Failed to compile TerminalEmulator.java
    exit /b 1
)

echo Compiling TextEditor.java...
javac TextEditor.java
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: Failed to compile TextEditor.java
    exit /b 1
)

echo Compiling WebBrowser.java...
javac WebBrowser.java
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: Failed to compile WebBrowser.java
    exit /b 1
)

echo Compiling WorkspaceManager.java...
javac WorkspaceManager.java
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: Failed to compile WorkspaceManager.java
    exit /b 1
)

echo Compiling StartMenu.java...
javac StartMenu.java
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: Failed to compile StartMenu.java
    exit /b 1
)

echo Compiling IRIXDesktop.java...
javac IRIXDesktop.java
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: Failed to compile IRIXDesktop.java
    exit /b 1
)

echo.
echo ======================================
echo Compilation successful!
echo ======================================
echo.
echo To run the application, use:
echo   java IRIXDesktop
echo.
echo Or run this script with --run flag:
echo   build.bat --run
echo.

REM Run if --run flag is provided
if "%1"=="--run" (
    echo Starting IRIX Desktop...
    echo.
    java IRIXDesktop
)
