/* Build MEM.EXE with OpenWatcom C */

bldate = LEFT( DATE('N'), 11 )
bltime = LEFT( TIME(), 10 )
blhost = RIGHT( VALUE('HOSTNAME',,'OS2ENVIRONMENT'), 11 )
blvend = 'Alexander Taylor'
blver  = '1.1'
blsig  = '@#'blvend':'blver'#@##1##' bldate bltime blhost'::::::@@MEM.EXE - report physical memory'

'wcl386 -bc -bm -wx -"option de '''blsig'''" mem.c libuls.lib'

RETURN rc
