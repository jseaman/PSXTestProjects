@ECHO OFF

ECHO Compiling and generating executable...
psymake

ECHO Building ISO file...
MKPSXISO -y CDLAYOUT.XML

ECHO ISO file built successfully!

