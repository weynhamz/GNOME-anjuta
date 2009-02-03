/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor Boston, MA 02110-1301,  USA
 */
 
#ifndef ANJUTA_VCS_STATUS_H
#define ANJUTA_VCS_STATUS_H

/**
 * IAnjutaVcsStatus:
 * @ANJUTA_VCS_STATUS_NONE: File has unknown status
 * @ANJUTA_VCS_STATUS_MODIFIED: File was modified locally
 * @ANJUTA_VCS_STATUS_ADDED: File was added
 * @ANJUTA_VCS_STATUS_DELETED: File was deleted
 * @ANJUTA_VCS_STATUS_CONFLICTED: File has unresolved conflict
 * @ANJUTA_VCS_STATUS_UPTODATE: File is up-to-date
 * @ANJUTA_VCS_STATUS_LOCKED: File is locked
 * @ANJUTA_VCS_STATUS_MISSING: File is missing 
 * @ANJUTA_VCS_STATUS_UNVERSIONED: File is ignored by VCS system
 *
 * This enumeration is used to specify the status of a file. A file can
 * have multiple status flags assigned (MODIFIED and CONFLICT, for example)
 */
typedef enum
{
	/* Unversioned, ignored, or uninteresting items */
	ANJUTA_VCS_STATUS_NONE = 0, /*< skip >*/
	ANJUTA_VCS_STATUS_MODIFIED = 1 << 0,
	ANJUTA_VCS_STATUS_ADDED = 1 << 1,
	ANJUTA_VCS_STATUS_DELETED = 1 << 2,
	ANJUTA_VCS_STATUS_CONFLICTED = 1 << 3,
	ANJUTA_VCS_STATUS_UPTODATE = 1 << 4,
	ANJUTA_VCS_STATUS_LOCKED = 1 << 5,
	ANJUTA_VCS_STATUS_MISSING = 1 << 6,
	ANJUTA_VCS_STATUS_UNVERSIONED = 1 << 7,
	ANJUTA_VCS_STATUS_IGNORED = 1 << 8
} AnjutaVcsStatus;

#endif // ANJUTA_VCS_STATUS_H


 
 
