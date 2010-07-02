/*
 * ClEventManager.h
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

#ifndef CLEVENTMANAGER_H_
#define CLEVENTMANAGER_H_

#include <vector>

using namespace std;

typedef unsigned int EventId;
typedef void (*EventCallback)(void* object, void* parameter);
typedef struct Event{
	EventId id;
	EventCallback callback;
	void* cbTrigger;
	void* cbObject;
	void* cbParam;
	bool triggered;
} Event;

class ClEventManager {
public:
	static EventId registerEvent(void* trigger, void* reciever, EventCallback evtCallback, void* cbParameter = 0);
	static bool triggerEvent(EventId evId);
	static bool triggerEvent(void* triggerObject);

protected:

	static std::vector<Event> events;
};

#endif /* CLEVENTMANAGER_H_ */
