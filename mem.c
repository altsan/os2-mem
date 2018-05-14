/* mem.c (C) 2018 Alex Taylor
 *
 * Query the amount of memory in the system.
 *
 *  Copyright (C) 2018 Alexander Taylor
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#define INCL_DOSMISC
#define INCL_DOSERRORS
#include <os2.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>
#include <unidef.h>


#define MODE_BYTES      1
#define MODE_KBYTES     2
#define MODE_MBYTES     4
#define MODE_HELP       8
#define MODE_VERBOSE    0xF0

#define MY_QSV_FIRST    QSV_TOTPHYSMEM
#define MY_QSV_LAST     QSV_MAXHSHMEM

#define MY_TOTPHYSMEM   (QSV_TOTPHYSMEM - MY_QSV_FIRST)
#define MY_TOTAVAILMEM  (QSV_TOTAVAILMEM - MY_QSV_FIRST)
#define MY_TOTRESMEM    (QSV_TOTRESMEM - MY_QSV_FIRST)
#define MY_MAXPRMEM     (QSV_MAXPRMEM - MY_QSV_FIRST)
#define MY_MAXSHMEM     (QSV_MAXSHMEM - MY_QSV_FIRST)
#define MY_MAXHPRMEM    (QSV_MAXHPRMEM - MY_QSV_FIRST)
#define MY_MAXHSHMEM    (QSV_MAXHSHMEM - MY_QSV_FIRST)


#ifdef LEGACY_C_LOCALE
void printFormattedSize( ULONG ulValue, BYTE bMode, ... )
{
    switch ( bMode & 0x0F ) {
        case MODE_BYTES:
            if ( bMode & MODE_VERBOSE )
                printf("%13'u bytes\n", ulValue );
            else
                printf("%'u bytes\n", ulValue );
            break;
        case MODE_KBYTES:
            if ( bMode & MODE_VERBOSE )
                printf("%9'u KB\n", ulValue / 1024 );
            else
                printf("%'u KB\n", ulValue / 1024 );
            break;
        default:
        case MODE_MBYTES:
            if ( bMode & MODE_VERBOSE )
                printf("%5'u MB\n", ulValue / 1048576 );
            else
                printf("%'u MB\n", ulValue / 1048576 );
            break;
    }
}
#else

void getGroupingCharacter( PSZ *ppszGrouping )
{
    LocaleObject locale = NULL;
    UniChar     *puzSep;
    int          rc = 0;
    int          buf_size;
    PSZ          pszBuffer;

    rc = UniCreateLocaleObject( UNI_MBS_STRING_POINTER, "", &locale );
    if ( rc != ULS_SUCCESS )
        return;
    if ( UniQueryLocaleItem( locale, LOCI_sThousand, &puzSep ) == ULS_SUCCESS ) {
            buf_size = UniStrlen( puzSep );
            pszBuffer = (PSZ) calloc( 1, buf_size + 1 );
            if ( pszBuffer ) {
                sprintf( pszBuffer, "%ls", puzSep );
                *ppszGrouping = pszBuffer;
            }
        UniFreeMem( puzSep );
    }
    UniFreeLocaleObject( locale );
}


void sprintGroup( PSZ buf, ULONG val, PSZ sep )
{
    if ( val < 1000 ) {
        sprintf( buf, "%u", val );
        return;
    }
    sprintGroup( buf, val / 1000, sep );
    sprintf( buf+strlen(buf), "%s%03u", sep, val % 1000 );
}


void printFormattedSize( ULONG ulValue, BYTE bMode, PSZ pszSep )
{
    char achBuf[ 20 ] = {0};
    int  rc = 0;

    switch ( bMode & 0x0F ) {
        case MODE_BYTES:
            sprintGroup( achBuf, ulValue, pszSep );
            if ( bMode & MODE_VERBOSE )
                printf("%13s bytes\n", achBuf );
            else
                printf("%s bytes\n", achBuf );
            break;
        case MODE_KBYTES:
            sprintGroup( achBuf, ulValue / 1024, pszSep );
            if ( bMode & MODE_VERBOSE )
                printf("%9s KB\n", achBuf );
            else
                printf("%s KB\n", achBuf );
            break;
        default:
        case MODE_MBYTES:
            sprintGroup( achBuf, ulValue / 1048576, pszSep );
            if ( bMode & MODE_VERBOSE )
                printf("%5s MB\n", achBuf );
            else
                printf("%s MB\n", achBuf );
            break;
    }
}
#endif  // LEGACY_C_LOCALE


int main( int argc, char *argv[] )
{
    ULONG aulBuf[ MY_QSV_LAST - MY_QSV_FIRST + 1 ] = {0},
          ulMemSize     = 0,
          ulAvlMemSize  = 0,
          ulResMemSize  = 0,
          ulPriMemSize  = 0,
          ulShdMemSize  = 0,
          ulPriHMemSize = 0,
          ulShdHMemSize = 0,
          ulRC          = 0;
    BYTE  bMode         = 0;
    BOOL  fCLocale      = FALSE;
    PSZ   pszSep        = NULL,
          psz;
    int   a;

    /* Parse command-line arguments:
     *
     * /? | /H      Show help
     * /L           Force use of C locale (suppresses thousands separators)
     * /U:[M|K|B]   Units to use for display ([M]iB, [K]iB, B) -- default is M
     * /V           Verbose reporting
     *
     */
    for ( a = 1; a < argc; a++ ) {
        psz = argv[ a ];
        if ( *psz == '/' || *psz == '-') {
            psz++;
            if ( !(*psz) ) continue;
            strupr( psz );
            switch ( *psz ) {
                case 'U':
                    if ( !( bMode & 0x0F ) && ( strlen( psz ) > 2 )) {
                        char unit = *(psz+2);
                        switch ( unit ) {
                            case 'M': bMode |= MODE_MBYTES; break;
                            case 'K': bMode |= MODE_KBYTES; break;
                            case 'B': bMode |= MODE_BYTES;  break;
                        }
                    }
                    break;

                case 'K':
                    if ( !(bMode & 0x0F)) bMode |= MODE_KBYTES;
                    break;

                case 'B':
                    if ( !(bMode & 0x0F)) bMode |= MODE_BYTES;
                    break;

                case 'V':
                    bMode |= MODE_VERBOSE;
                    break;

                case 'L':
                    fCLocale = TRUE;
                    break;

                case 'H':
                case '?':
                    bMode = MODE_HELP;
                    break;
            }
        }
    }

    if ( bMode == MODE_HELP ) {
        PSZ pszCommand = strdup( argv[0] ),
            p;

        if ( pszCommand ) {
            if (( p = strchr( pszCommand, '.')) != NULL )
                *p = 0;
            else
                p = pszCommand;
            strupr( p );
            printf("%s - Report system memory.\n", p );
            printf("\nOptions:\n");
            printf("  /H or /?    Show this help\n");
            printf("  /L          Disable locale-specific number formatting (no grouping character)\n");
            printf("  /U:<unit>   Use <unit> for output, where <unit> is one of:\n");
            printf("     or /B      B   Bytes\n");
            printf("     or /K      K   Binary kilobytes\n");
            printf("                M   Binary megabytes (default)\n");
            printf("  /V          Verbose reporting\n");
            free( pszCommand );
        }
        return 0;
    }

    if ( !(bMode & 0x0F)) bMode |= MODE_MBYTES;

    ulRC = DosQuerySysInfo( MY_QSV_FIRST, MY_QSV_LAST, &aulBuf, sizeof(aulBuf) );
    if ( ulRC != NO_ERROR ) {
        return ulRC;
    }

    ulMemSize     = aulBuf[ MY_TOTPHYSMEM ];
    ulAvlMemSize  = aulBuf[ MY_TOTAVAILMEM ];
    ulResMemSize  = aulBuf[ MY_TOTRESMEM ];
    ulPriMemSize  = aulBuf[ MY_MAXPRMEM ];
    ulShdMemSize  = aulBuf[ MY_MAXSHMEM ];
    ulPriHMemSize = aulBuf[ MY_MAXHPRMEM ];
    ulShdHMemSize = aulBuf[ MY_MAXHSHMEM ];

    psz = getenv("LANG") ;
    if ( !psz || fCLocale )
        setlocale( LC_NUMERIC, "C");
    else
        setlocale( LC_NUMERIC, psz );

#ifndef LEGACY_C_LOCALE
    if ( !fCLocale )
        getGroupingCharacter( &pszSep );
    if ( pszSep == NULL ) pszSep = strdup("");
#endif

    if ( bMode & MODE_VERBOSE ) {
        printf("\nTotal physical memory:  ");
        printFormattedSize( ulMemSize, bMode, pszSep );
        printf("\n");
        printf("  Available memory:     ");
        printFormattedSize( ulAvlMemSize, bMode, pszSep  );
        printf("  Resident memory:      ");
        printFormattedSize( ulResMemSize, bMode, pszSep  );
        printf("\n");
        printf("Available process memory:\n");
        printf("\n");
        printf("  Private low memory:   ");
        printFormattedSize( ulPriMemSize, bMode, pszSep  );
        printf("  Private high memory:  ");
        printFormattedSize( ulPriHMemSize, bMode, pszSep  );
//        printf("\n");
        printf("  Shared low memory:    ");
        printFormattedSize( ulShdMemSize, bMode, pszSep  );
        printf("  Shared high memory:   ");
        printFormattedSize( ulShdHMemSize, bMode, pszSep  );
//        printf("\n");
    }
    else {
        printFormattedSize( ulMemSize, bMode, pszSep );
    }

    if ( pszSep ) free( pszSep );
    return 0;
}



