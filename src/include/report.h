/*
    Copyright (C) 2011  ABRT team.
    Copyright (C) 2011  RedHat inc.

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
#ifndef LIBREPORT_REPORT_H_
#define LIBREPORT_REPORT_H_

#include "problem_data.h"

#ifdef __cplusplus
extern "C" {
#endif

enum {
    LIBREPORT_NOWAIT      = 0,
    LIBREPORT_WAIT        = (1 << 0), /* wait for report to finish */
    LIBREPORT_GETPID      = (1 << 1), /* return pid of child. Use with LIBREPORT_NOWAIT. */
                                      /* Note: without LIBREPORT_GETPID, child will be detached */
                                      /* (reparented to init) */
    LIBREPORT_ANALYZE     = (1 << 2), /* run analyzers? */
                                      /* ("run reporters" is always on, has no flag (for now?)) */
    LIBREPORT_RELOAD_DATA = (1 << 5), /* reload problem data after run (needs WAIT) */
    LIBREPORT_DEL_DIR     = (1 << 6), /* delete directory after reporting (passes --delete to child) */
    LIBREPORT_RUN_CLI     = (1 << 7), /* run 'cli' instead of 'gui' */
    LIBREPORT_RUN_NEWT    = (1 << 8), /* run 'report-newt' */
};

int report_problem_in_dir(const char *dirname, int flags);

/* Reports a problem stored in problem_data_t.
 * It's first saved to /tmp and then processed as a dump dir.
 */
int report_problem_in_memory(problem_data_t *pd, int flags);

/* Simple wrapper for trivial uses */
int report_problem(problem_data_t *pd);

#ifdef __cplusplus
}
#endif

#endif
