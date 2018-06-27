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

#define INCL_DOSDEVICES
#define INCL_DOSDEVIOCTL
#define INCL_DOSFILEMGR
#define INCL_DOSMISC
#define INCL_DOSERRORS
#include <os2.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>
#include <unidef.h>


// -------------------------------------------------------------------------
// CONSTANTS
//

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


// -------------------------------------------------------------------------
// IOCtl data for AOSLdr extended memory info
//

#define OEMHLP_GETMEMINFOEX    0x0011

#pragma pack(1)
typedef struct _OEMHLP_MEMINFOEX {  // OEMHLP_GETMEMINFOEX packet
   unsigned long    LoPages;        // # of 4k pages below 4Gb border (except 1st mb)
   unsigned long    HiPages;        // # of 4k pages above 4Gb
   unsigned long    AvailPages;     // # of pages, available for OS/2 (except 1st mb)
} OEMHLP_MEMINFOEX;
#pragma pack()



// -------------------------------------------------------------------------
// getXMemSize
//
// Query the amount of memory above the 4GB barrier, if any.  This
// information is not reported by DosQuerySysInfo and must be queried
// from the loader via IOCtl.  Requires the enhanced loader (otherwise
// -1 will be returned).
//
long long getXMemSize( void )
{
    OEMHLP_MEMINFOEX xmi;
    HFILE     hf;
    ULONG     ulAction,
              cb, cbActual;
    long long llSize = -1;
    APIRET    rc;

    rc = DosOpen("\\DEV\\OEMHLP$", &hf, &ulAction, 0L, FILE_NORMAL,
                 OPEN_ACTION_OPEN_IF_EXISTS, OPEN_SHARE_DENYNONE, NULL );
    if ( rc == NO_ERROR ) {
        cb = sizeof( xmi );
        rc = DosDevIOCtl( hf, IOCTL_OEMHLP, OEMHLP_GETMEMINFOEX,
                          NULL, 0L, NULL, &xmi, cb, &cbActual );
        if (( rc == NO_ERROR ) && ( cb == cbActual ))
            llSize = (long long)xmi.HiPages * 4096;
        DosClose( hf );
    }
    return llSize;
}


// -------------------------------------------------------------------------
// getGroupingCharacter
//
// Determine the numeric thousands-grouping separator for the current locale.
//
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


// -------------------------------------------------------------------------
// sprintGroup
//
// Format a 64-bit integer with the indicated thousands-grouping separator.
//
void sprintGroup( PSZ buf, long long val, PSZ sep )
{
    if ( val < 1000 ) {
        sprintf( buf, "%llu", val );
        return;
    }
    sprintGroup( buf, val / 1000, sep );
    sprintf( buf+strlen(buf), "%s%03llu", sep, val % 1000 );
}


// -------------------------------------------------------------------------
// printFormattedSize
//
// Print a formatted byte value using the configured units.
//
void printFormattedSize( long long llValue, BYTE bMode, PSZ pszSep )
{
    char achBuf[ 40 ] = {0};

    switch ( bMode & 0x0F ) {
        case MODE_BYTES:
            sprintGroup( achBuf, llValue, pszSep );
            if ( bMode & MODE_VERBOSE )
                printf("%15s bytes", achBuf );
            else
                printf("%s bytes", achBuf );
            break;
        case MODE_KBYTES:
            sprintGroup( achBuf, llValue / 1024, pszSep );
            if ( bMode & MODE_VERBOSE )
                printf("%11s KB", achBuf );
            else
                printf("%s KB", achBuf );
            break;
        default:
        case MODE_MBYTES:
            sprintGroup( achBuf, llValue / 1048576, pszSep );
            if ( bMode & MODE_VERBOSE )
                printf("%7s MB", achBuf );
            else
                printf("%s MB", achBuf );
            break;
    }

}


// -------------------------------------------------------------------------
// main program
//
int main( int argc, char *argv[] )
{
    ULONG     aulBuf[ MY_QSV_LAST - MY_QSV_FIRST + 1 ] = {0};
    long long llMemSize     = 0,
              llAvlMemSize  = 0,
              llResMemSize  = 0,
              llPriMemSize  = 0,
              llShdMemSize  = 0,
              llPriHMemSize = 0,
              llShdHMemSize = 0,
              llXMemSize    = 0;
    ULONG     ulRC          = 0;
    BYTE      bMode         = 0;
    BOOL      fCLocale      = FALSE;
    PSZ       pszSep        = NULL,
              psz;
    int       a;

    /* Parse command-line arguments:
     *
     * /? | /H      Show help
     * /L           Force use of C locale (suppresses thousands separators)
     * /U:[G|M|K]   Units to use for display ([G]iB, [M]iB, [K]iB) -- default is M
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

    llMemSize     = aulBuf[ MY_TOTPHYSMEM ];
    llAvlMemSize  = aulBuf[ MY_TOTAVAILMEM ];
    llResMemSize  = aulBuf[ MY_TOTRESMEM ];
    llPriMemSize  = aulBuf[ MY_MAXPRMEM ];
    llShdMemSize  = aulBuf[ MY_MAXSHMEM ];
    llPriHMemSize = aulBuf[ MY_MAXHPRMEM ];
    llShdHMemSize = aulBuf[ MY_MAXHSHMEM ];

    llXMemSize = getXMemSize();

    psz = getenv("LANG") ;
    if ( !psz || fCLocale )
        setlocale( LC_NUMERIC, "C");
    else
        setlocale( LC_NUMERIC, psz );

    if ( !fCLocale )
        getGroupingCharacter( &pszSep );
    if ( pszSep == NULL ) pszSep = strdup("");

    if ( bMode & MODE_VERBOSE ) {
        printf("\nTotal physical memory:    ");
        if ( llXMemSize > 0 ) {
            printFormattedSize( llMemSize + llXMemSize, bMode, pszSep );
            printf("\nAccessible to system:     ");
            printFormattedSize( llMemSize, bMode, pszSep );
            printf("\nAdditional (PAE) memory:  ");
            printFormattedSize( llXMemSize, bMode, pszSep );
        }
        else {
            printFormattedSize( llMemSize, bMode, pszSep );
        }
        printf("\n\n");
        printf("Resident memory:          ");
        printFormattedSize( llResMemSize, bMode, pszSep  );
        printf("\n");
        printf("Available virtual memory: ");
        printFormattedSize( llAvlMemSize, bMode, pszSep  );
        printf("\n\n");

        printf("Available process memory:\n");
        printf("  Private low memory:     ");
        printFormattedSize( llPriMemSize, bMode, pszSep  );
        printf("\n");
        printf("  Private high memory:    ");
        printFormattedSize( llPriHMemSize, bMode, pszSep  );
        printf("\n");
        printf("  Shared low memory:      ");
        printFormattedSize( llShdMemSize, bMode, pszSep  );
        printf("\n");
        printf("  Shared high memory:     ");
        printFormattedSize( llShdHMemSize, bMode, pszSep  );
        printf("\n");
    }
    else {
        if ( llXMemSize > 0 ) {
            printFormattedSize( llMemSize + llXMemSize, bMode, pszSep );
            printf("  (");
            printFormattedSize( llMemSize, bMode, pszSep );
            printf(" accessible to system)");
        }
        else {
            printFormattedSize( llMemSize, bMode, pszSep );
        }
        printf("\n");
    }

    if ( pszSep ) free( pszSep );
    return 0;
}


