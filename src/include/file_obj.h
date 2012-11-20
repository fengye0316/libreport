/*
    Copyright (C) 2010  ABRT team
    Copyright (C) 2010  RedHat Inc

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

typedef struct file_obj
{
    /* just the filename without the path and extension
     * so it can be used as event name
     * e.g:
     *    if fullpath is: /foo/bar/report_Bugzilla.xml
     *    then filename is: report_Bugzilla
     * in case of symlink the filename is created from the symlink name
     * and the fullpath is the symlink target
     */
    char *filename;
    char *fullpath; //the full path with extension
} file_obj_t;
