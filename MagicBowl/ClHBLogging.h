/*
 * ClHBLogging.h
 *
 *  Copyright (C) 2010 André Borrmann
 *
 * This program is free software;
 * you can redistribute it and/or modify it under the terms of the
 * GNU General Public License as published by the Free Software Foundation;
 * either version 3 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * See the GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License along with this program;
 * if not, see <http://www.gnu.org/licenses/>.
 */

#ifndef CLHBLOGGING_H_
#define CLHBLOGGING_H_

class ClHBLogging {
public:
	static void log(const char *, ...)
	                __attribute__ ((__format__ (__printf__, 1, 2)));
protected:
	static int logFile;
};

#endif /* CLHBLOGGING_H_ */
