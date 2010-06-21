/*
 * ClEventManager.cpp
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

#include "ClEventManager.h"

std::vector<Event> ClEventManager::events;

bool ClEventManager::triggerEvent(EventId evId){
	events[evId-1].callback(events[evId-1].cbObject, events[evId-1].cbParam);
	return true;
}

bool ClEventManager::triggerEvent(void* triggerObject){
	unsigned int e;
	for (e = 0; e <events.size();e++){
		if (events[e].cbTrigger == triggerObject)
			events[e].callback(events[e].cbObject, events[e].cbParam);
	}
	return true;
}

EventId ClEventManager::registerEvent(void* trigger, void* reciever, EventCallback evtCallback, void* cbParameter){
	Event newEvent;
	newEvent.id = events.size()+1;
	newEvent.callback = evtCallback;
	newEvent.cbTrigger = trigger;
	newEvent.cbObject = reciever;
	newEvent.cbParam = cbParameter;

	events.push_back(newEvent);

	return newEvent.id;
}



