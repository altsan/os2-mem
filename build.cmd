/* */

bldate = RIGHT( DATE('N'), 11 )
bltime = TIME()
blhost = RIGHT( VALUE('HOSTNAME',,'OS2ENVIRONMENT'), 14 )
blvend = 'Alexander Taylor'
blver  = '1.0'
blsig  = '@#'blvend':'blver'#@##1##'bldate bltime blhost'::::::@@MEM.EXE - report physical memory'

'icc /G5 /Gm /Ss /Q /V"'blsig'" /B"/MAP libuls.lib" mem.c'
IF rc == 0 THEN
    'dllrname.exe mem.exe CPPOM30=OS2OM30 /Q /R'

RETURN rc
