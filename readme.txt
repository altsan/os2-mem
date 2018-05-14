## OS/2 MEM (MEM.EXE) ##

MEM.EXE is a simple memory reporting command for OS/2, similar in concept 
(though necessarily different in behaviour) to the equivalently-named DOS
command.

By default, total physical memory is displayed, in binary megabytes.  Numbers
are formatted according to current locale conventions, with the appropriate
thousands-grouping character.  The /V parameter shows additional information,
including shared and private memory in both the upper and lower arenas.

Supported parameters:

/H or /?    Show help
/L          Disable locale formatting (no thousands-grouping character)
/U:B or /B  Report memory amounts in bytes
/U:K or /K  Report memory amounts in binary kilobytes (2^10 bytes)
/U:M or /M  Report memory amounts in binary megabytes (2^20 bytes) - default
/V          Verbose reporting, show additional memory information


## Notices ##

    OS/2 MEM
    Copyright (C) 2018 Alexander Taylor

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
