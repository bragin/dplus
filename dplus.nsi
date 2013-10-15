; ---------------------------------------------------------------------------
;
; NSIS install script for D+ Browser
;
; ---------------------------------------------------------------------------

!define VERSION "0.5b"
Name "D+ Browser"
OutFile "dplus-${VERSION}-setup.exe"
InstallDir $PROGRAMFILES\DPlus
InstallDirRegKey HKLM "Software\DPlus" "Install_Dir"
SetCompressor lzma
XPStyle on

!define MULTIUSER_EXECUTIONLEVEL Highest
!include MultiUser.nsh
RequestExecutionLevel highest

!include dist\FileAssociation.nsh

Page components
Page directory
Page instfiles

UninstPage uninstConfirm
UninstPage instfiles

; ----------------------------------------

InstType "Full"
InstType "Minimal"
InstType "Default"

Section "!D+ Browser"
	SectionIn 1 2 3 RO

	SetOutPath $INSTDIR
	File src\dplus.exe
	File dist\unicows.dll  ; needed on Windows 9x; harmless on others

	WriteRegStr HKLM "Software\DPlus" "Install_Dir" "$INSTDIR"

	; Workaround for users upgrading from dplus-0.5 on Windows 9x.
	; Move the profile dir from %windir% to %windir%\Application Data.
	IfFileExists "$windir\DPlus\*.*" 0 +3
	CreateDirectory "$windir\Application Data"
	Rename "$windir\DPlus" "$windir\Application Data\DPlus"

	; Make DPlus a valid browser choice for the Windows XP+ start menu
	; FIXME: This doesn't currently appear to work -- does anyone know why?
	WriteRegStr HKLM "Software\Clients\StartMenuInternet\dplus.exe\DefaultIcon" "" '"$INSTDIR\dplus.exe",0'
	WriteRegStr HKLM "Software\Clients\StartMenuInternet\dplus.exe\InstallInfo" "HideIconsCommand" '"$INSTDIR\dplus.exe"'
	WriteRegStr HKLM "Software\Clients\StartMenuInternet\dplus.exe\InstallInfo" "ReinstallCommand" '"$INSTDIR\dplus.exe"'
	WriteRegStr HKLM "Software\Clients\StartMenuInternet\dplus.exe\InstallInfo" "ShowIconsCommand" '"$INSTDIR\dplus.exe"'
	WriteRegDword HKLM "Software\Clients\StartMenuInternet\dplus.exe\InstallInfo" "IconsVisible" 1
	WriteRegStr HKLM "Software\Clients\StartMenuInternet\dplus.exe\shell\open\command" "" '"$INSTDIR\dplus.exe"'

	; Write uninstall information
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\DPlus" "DisplayName" "D+ Browser"
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\DPlus" "UninstallString" "$INSTDIR\uninst.exe"
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\DPlus" "NoModify" 1
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\DPlus" "NoRepair" 1
	WriteUninstaller "uninst.exe"
SectionEnd

Section "Create Start menu shortcut"
	SectionIn 1 2 3
	CreateShortCut "$SMPROGRAMS\D+ Browser.lnk" "$INSTDIR\dplus.exe" "" "$INSTDIR\dplus.exe" 0 SW_SHOWNORMAL "" ""
SectionEnd

Section "Create desktop shortcut"
	SectionIn 1 3
	CreateShortCut "$DESKTOP\D+ Browser.lnk" "$INSTDIR\dplus.exe" "" "$INSTDIR\dplus.exe" 0 SW_SHOWNORMAL "" ""
SectionEnd

; FIXME: This only works for the current user,
; and is no longer supported as of Windows 7.
Section /o "Create quick launch shortcut"
	SectionIn 1
	CreateShortCut "$QUICKLAUNCH\D+ Browser.lnk" "$INSTDIR\dplus.exe" "" "$INSTDIR\dplus.exe" 0 SW_SHOWNORMAL "" ""
SectionEnd

Section /o "Set as the default browser"
	SectionIn 1
	WriteRegStr HKCR "http\shell\open\command" "" '"$INSTDIR\dplus.exe" "%1"'
	WriteRegStr HKCR "https\shell\open\command" "" '"$INSTDIR\dplus.exe" "%1"'

	${RegisterExtension} "$INSTDIR\dplus.exe" ".htm" "HTML Document"
	${RegisterExtension} "$INSTDIR\dplus.exe" ".html" "HTML Document"
	${RegisterExtension} "$INSTDIR\dplus.exe" ".shtml" "HTML Document"
SectionEnd

Section /o "Associate with image files"
	SectionIn 1
	${RegisterExtension} "$INSTDIR\dplus.exe" ".gif" "GIF Image"
	${RegisterExtension} "$INSTDIR\dplus.exe" ".jpg" "JPEG Image"
	${RegisterExtension} "$INSTDIR\dplus.exe" ".jpeg" "JPEG Image"
	${RegisterExtension} "$INSTDIR\dplus.exe" ".png" "PNG Image"
SectionEnd

Section "Uninstall"
	DeleteRegKey HKLM "Software\DPlus"
	DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\DPlus"
	DeleteRegKey HKLM "Software\Clients\StartMenuInternet\dplus.exe"

	RMDir /r "$INSTDIR"

	Delete "$SMPROGRAMS\D+ Browser.lnk"
	Delete "$DESKTOP\D+ Browser.lnk"
	Delete "$QUICKLAUNCH\D+ Browser.lnk"

	${UnregisterExtension} ".htm" "HTML Document"
	${UnregisterExtension} ".html" "HTML Document"
	${UnregisterExtension} ".shtml" "HTML Document"

	${UnregisterExtension} ".gif" "GIF Image"
	${UnregisterExtension} ".jpg" "JPEG Image"
	${UnregisterExtension} ".jpeg" "JPEG Image"
	${UnregisterExtension} ".png" "PNG Image"
SectionEnd

; ----------------------------------------

Function .onInit
	!insertmacro MULTIUSER_INIT
FunctionEnd

Function un.onInit
	!insertmacro MULTIUSER_UNINIT
FunctionEnd

; vim: set nowrap :
