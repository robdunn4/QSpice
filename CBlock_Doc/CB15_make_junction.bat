@echo off
setlocal EnableDelayedExpansion

:: ============================================================
::  make_junction.bat
::
::  PURPOSE:
::    This is a utility for the QSpice circuit simulator. It
::    replaces QSpice's built-in Repository folder with an NTFS
::    junction link pointing to a user-defined folder. This
::    simplifies development of custom symbols that use the
::    LOOKUP Programmable Attribute, by allowing the developer
::    to maintain their Repository content in a convenient
::    location (such as a source-controlled project folder)
::    while QSpice continues to find it at its expected path.
::
::    A junction link is a transparent NTFS redirect: QSpice and
::    any other application that accesses LINK_PATH will
::    seamlessly read and write from TARGET_PATH instead, with
::    no awareness of the redirection.
::
::  USAGE:
::    1. Edit the CONFIGURATION section below to set LINK_PATH,
::       TARGET_PATH, and KEEP_BACKUP.
::    2. Double-click the script, or run it from a command prompt.
::       It will request elevation via UAC if not already running
::       as Administrator.
::    3. Review the output when complete.
::
::  NOTES:
::    - The TARGET_PATH folder must already exist before running.
::    - If LINK_PATH is already a junction, the script exits
::      cleanly without making any changes.
::    - Deleting a junction with 'rmdir' removes only the link;
::      the TARGET_PATH folder and its contents are unaffected.
::
:: ============================================================
::  DISCLAIMER / WARNING
:: ============================================================
::
::  This batch file was generated with the assistance of
::  Claude.AI (https://claude.ai) on March 26, 2026
::  by @RDunn.
::
::  IT SHOULD BE CONSIDERED SUSPECT AND DANGEROUS.
::  AI-generated code may contain errors, unintended behavior,
::  or security vulnerabilities. It has not been independently
::  audited or formally tested.
::
::  USE ENTIRELY AT YOUR OWN RISK. No guarantees, warranties,
::  or assurances of any kind are made regarding the correctness,
::  safety, or fitness for purpose of this script.
::
::  BEFORE RUNNING:
::    - Read and understand every line of this script.
::    - Verify that LINK_PATH and TARGET_PATH are correct.
::    - Ensure you have a backup of any data in LINK_PATH.
::    - Test in a non-production environment first.
::
:: ============================================================


:: ── CONFIGURATION ───────────────────────────────────────────
::
::  LINK_PATH:
::    The full path of the QSpice Repository folder to replace
::    with a junction. This must currently exist as a plain
::    folder. After the script runs, this path will transparently
::    redirect all access to TARGET_PATH.
::
::  TARGET_PATH:
::    The full path of your user-defined folder that the junction
::    will point to. This folder must already exist. Its contents
::    are never modified by this script.
::
::  KEEP_BACKUP:
::    1 = rename the original LINK_PATH folder to
::        LINK_PATH_backup_YYYYMMDD_HHMMSS before creating the
::        junction. Recommended -- preserves the original content.
::    0 = permanently delete the original folder. Use only if
::        you are certain you no longer need its contents.

set "LINK_PATH=C:\Program Files\QSpice\Repository"
set "TARGET_PATH=C:\Dev\QSpice\TAE_Libs\Repository"
set "KEEP_BACKUP=1"

:: ────────────────────────────────────────────────────────────


:: ── SELF-ELEVATION ───────────────────────────────────────────
::
::  Creating an NTFS junction and operating on protected folders
::  such as C:\Program Files requires Administrator rights.
::  This block checks whether the script is already elevated and,
::  if not, re-launches it elevated via a UAC prompt.
::
::  ADMIN CHECK:
::    PowerShell queries the current process's Windows identity
::    and checks whether it includes the built-in Administrator
::    role. It exits with code 0 (admin) or 1 (not admin).
::    This is more reliable than older methods such as
::    'net session' or 'fsutil dirty query', both of which behave
::    inconsistently across Windows 10/11 configurations.

powershell -NoProfile -Command ^
    "if(([Security.Principal.WindowsPrincipal][Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)){exit 0}else{exit 1}"

if %errorlevel% equ 0 goto :elevated

::  Not running as Administrator. Re-launch elevated.
echo Requesting administrator privileges...

::  LAUNCHER SCRIPT STRATEGY:
::    We cannot pass this script's path directly to PowerShell's
::    Start-Process -ArgumentList, because the script path may
::    contain characters that PowerShell treats specially (such as
::    @, which PowerShell interprets as its array-splatting operator).
::    Such characters corrupt the argument before cmd.exe ever
::    receives it, causing the elevated instance to silently fail.
::
::    The fix is to write a minimal one-line "launcher" batch file
::    to %PUBLIC%\Documents -- a safe path guaranteed to contain
::    no special characters, accessible to all user accounts.
::    PowerShell elevates and runs the launcher (safe, plain path).
::    The launcher uses cmd.exe's 'call' command to invoke this
::    script -- at which point special characters in the path are
::    harmless, because cmd.exe does not interpret them inside a
::    quoted path string.
::
::  RESULT FILE:
::    Because the elevated window runs hidden (-WindowStyle Hidden),
::    the user would otherwise see no output. To surface the
::    results, the elevated instance is passed the path to a result
::    file as its first argument (%1). All output from the elevated
::    instance is redirected into that file. Once the elevated
::    window exits, this (visible) non-elevated window reads and
::    displays the result file contents, then deletes it.
::
::    Both files are placed in %PUBLIC%\Documents, which is
::    writable by standard users and accessible to the elevated
::    Administrator context, so both instances agree on the paths.

set "SAFE_DIR=%PUBLIC%\Documents"
set "LAUNCHER=%SAFE_DIR%\_junc_launcher.bat"
set "RESULT_FILE=%SAFE_DIR%\_junc_result.txt"

::  Remove any leftover result file from a previous run.
if exist "%RESULT_FILE%" del "%RESULT_FILE%" >nul 2>&1

::  Write the launcher. It contains a single 'call' that invokes
::  this script (%~f0 = full path of this script) and passes the
::  result file path as argument %1.
(
    echo @echo off
    echo call "%~f0" "%RESULT_FILE%"
) > "%LAUNCHER%"

::  Run the launcher elevated and wait for it to finish.
::  -WindowStyle Hidden keeps the elevated window invisible;
::  all its output is captured into RESULT_FILE instead.
powershell -NoProfile -ExecutionPolicy Bypass -Command ^
    "Start-Process cmd.exe -ArgumentList '/c ""%LAUNCHER%""' -Verb RunAs -Wait -WindowStyle Hidden"

::  Display whatever the elevated instance wrote to the result file.
echo.
if exist "%RESULT_FILE%" (
    type "%RESULT_FILE%"
    del "%RESULT_FILE%" >nul 2>&1
) else (
    rem  No result file means the UAC prompt was cancelled, or the
    rem  elevated process failed before writing any output.
    echo [CANCELLED] UAC prompt was cancelled or elevation failed.
)

::  Clean up the temporary launcher script.
del "%LAUNCHER%" >nul 2>&1

echo.
pause
exit /b

:: ────────────────────────────────────────────────────────────


:elevated
:: ── ELEVATED ENTRY POINT ─────────────────────────────────────
::
::  Execution reaches here when the script is confirmed to be
::  running as Administrator, either because it was launched
::  elevated directly, or after the UAC re-launch above.
::
::  If re-launched via the launcher, %1 contains the result file
::  path. We redirect all output from :RUN into that file so the
::  non-elevated window can display it.
::
::  If already elevated with no re-launch, %1 is empty and output
::  goes directly to the console as normal.

set "RESULT_FILE=%~1"
if defined RESULT_FILE (
    call :RUN > "%RESULT_FILE%" 2>&1
) else (
    call :RUN
)
exit /b


:: ════════════════════════════════════════════════════════════
:RUN
::  All junction-creation logic lives here as a subroutine so
::  that its stdout/stderr can be cleanly redirected to the
::  result file when called from the re-launched elevated instance.
:: ════════════════════════════════════════════════════════════

echo.
echo ============================================================
echo  make_junction.bat  (running as Administrator)
echo ============================================================
echo.
echo  Link path   : %LINK_PATH%
echo  Target path : %TARGET_PATH%
echo.


:: ── VALIDATE TARGET ─────────────────────────────────────────
::
::  Confirm TARGET_PATH exists before doing anything destructive.
::  There is no point renaming or deleting LINK_PATH if the
::  junction would point to a non-existent location.

if not exist "%TARGET_PATH%\" (
    echo [ERROR] Target folder does not exist:
    echo         %TARGET_PATH%
    echo         Create the target folder first, then re-run.
    exit /b 1
)


:: ── HANDLE EXISTING LINK PATH ───────────────────────────────
::
::  LINK_PATH can be in one of three states:
::
::  1. Does not exist:
::       Nothing to rename or delete. Proceed directly to mklink.
::
::  2. Already a junction / reparse point:
::       The job is already done. Report and exit cleanly without
::       making any changes.
::
::  3. A plain folder:
::       Must be moved out of the way (backup or delete) before
::       mklink can create the junction at that path.

if exist "%LINK_PATH%" (

    rem  JUNCTION DETECTION:
    rem    PowerShell's Get-Item with -LiteralPath (handles spaces
    rem    and special characters correctly) and -Force (reveals
    rem    hidden/system items) returns an object whose .LinkType
    rem    property is the string "Junction" for a junction, or
    rem    empty for a plain folder.
    rem
    rem    Earlier attempts used 'dir /a:l' and 'fsutil reparsepoint
    rem    query', both of which proved unreliable on paths containing
    rem    spaces or when called from within a for /f subshell.

    for /f "usebackq" %%A in (`powershell -NoProfile -Command ^
        "(Get-Item -LiteralPath '%LINK_PATH%' -Force).LinkType -eq 'Junction'"`) do set "IS_JUNCTION=%%A"

    if /i "!IS_JUNCTION!"=="True" (
        echo [INFO] "%LINK_PATH%" is already a junction / reparse point.
        echo        Nothing to do.
        exit /b 0
    )

    rem  LINK_PATH is a plain folder. Rename or delete it.
    if "%KEEP_BACKUP%"=="1" (

        rem  BACKUP BY RENAME:
        rem    Append a YYYYMMDD_HHMMSS timestamp to the folder name.
        rem
        rem    The 'rename' command requires its second argument to be
        rem    a bare folder name with NO path prefix -- it cannot move
        rem    items between directories. So we extract the name-only
        rem    portion (%%~nxF) and parent path (%%~dpF) separately
        rem    using for-variable modifiers, then pass only the name
        rem    to rename.
        rem
        rem    %TIME% can contain a leading space for single-digit
        rem    hours (e.g. " 9:05:00"). The space-to-zero substitution
        rem    (!TIMESTAMP: =0!) replaces spaces with zeros for a valid name.

        for %%F in ("%LINK_PATH%") do set "LINK_NAME=%%~nxF"
        for %%D in ("%LINK_PATH%") do set "LINK_PARENT=%%~dpF"
        set "TIMESTAMP=%DATE:~-4%%DATE:~4,2%%DATE:~7,2%_%TIME:~0,2%%TIME:~3,2%%TIME:~6,2%"
        set "TIMESTAMP=!TIMESTAMP: =0!"
        set "NEW_NAME=!LINK_NAME!_backup_!TIMESTAMP!"

        echo [INFO] Renaming existing folder to:
        echo        !LINK_PARENT!!NEW_NAME!
        rename "%LINK_PATH%" "!NEW_NAME!"
        if errorlevel 1 (
            echo [ERROR] Could not rename the existing folder.
            echo         Make sure no files inside it are open/locked.
            goto :fail
        )

    ) else (

        rem  DELETE (no backup):
        rem    'rd /s' removes the folder and all contents recursively.
        rem    '/q' suppresses the confirmation prompt.
        rem    This is permanent and unrecoverable -- use with caution.

        echo [WARN] Permanently deleting existing folder:
        echo        %LINK_PATH%
        rd /s /q "%LINK_PATH%"
        if errorlevel 1 (
            echo [ERROR] Could not delete the existing folder.
            echo         Make sure no files inside it are open/locked.
            goto :fail
        )
    )
)


:: ── CREATE THE JUNCTION ─────────────────────────────────────
::
::  'mklink /j' creates an NTFS directory junction.
::  Syntax: mklink /j "<link_path>" "<target_path>"
::
::  The junction is stored as a reparse point in the NTFS
::  metadata of the parent directory. It is fully transparent
::  to applications: they see LINK_PATH as a normal folder
::  containing TARGET_PATH's contents.
::
::  DRIVE REQUIREMENTS:
::    LINK_PATH and TARGET_PATH do NOT need to be on the same drive.
::    A junction on C: can point to a target on D:, E:, or any other
::    local drive. However, both volumes must meet these conditions:
::      - Both must be local NTFS volumes. Junctions cannot target
::        network paths (UNC paths such as \\server\share), FAT32,
::        or exFAT volumes.
::      - The target must be a directory, not a file.
::    If you need to target a network path, use a symbolic link
::    (mklink /d) instead -- but note that symbolic links have
::    additional privilege requirements (SeCreateSymbolicLinkPrivilege).
::
::  Junctions vs symbolic links (/d):
::    - Junctions work on local NTFS volumes only (not UNC/network)
::    - Junctions do not require the SeCreateSymbolicLinkPrivilege
::    - Junctions are supported on all Windows versions since XP
::
::  CAUTION: To remove the junction later without affecting the
::  target, use 'rmdir "%LINK_PATH%"' (no /s flag). Using
::  'rmdir /s' or deleting via Explorer will follow the junction
::  and delete TARGET_PATH's contents.

echo [INFO] Creating junction link...
mklink /j "%LINK_PATH%" "%TARGET_PATH%"
if errorlevel 1 (
    echo [ERROR] mklink failed. Junction was NOT created.
    exit /b 1
)

echo.
echo [SUCCESS] Junction created:
echo           %LINK_PATH%  --^>  %TARGET_PATH%
exit /b 0

:fail
exit /b 1
