@echo off
SET size=80

otf2bdf -r 72 -p %size% -o %1%size%.bdf %1

