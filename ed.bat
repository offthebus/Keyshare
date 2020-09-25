@echo off
if exist Session.vim ( start gvim -S Session.vim ) else ( start gvim *.cpp *.h ) 
