cmd /c build
diskpart -s mount.s
xcopy .\efi x:\efi /e /y
xcopy .\Tomato x:\Tomato /e /y
diskpart -s unmount.s