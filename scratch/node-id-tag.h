/*
 * node-id-tag.h
 *
 *  Created on: May 21, 2014
 *      Author: czhe
 */

#ifndef NODE_ID_TAG_H_
#define NODE_ID_TAG_H_
#include "ns3/tag.h"
#include "ns3/packet.h"
#include "ns3/uinteger.h"
#include <iostream>

using namespace ns3;

class NodeIdTag: public Tag {
public:
	static TypeId GetTypeId(void);
	virtual TypeId GetInstanceTypeId(void) const;
	virtual uint32_t GetSerializedSize(void) const;
	virtual void Serialize(TagBuffer i) const;
	virtual void Deserialize(TagBuffer i);
	virtual void Print(std::ostream &os) const;

	uint16_t id_pod, id_switch, id_level;

};

#endif /* NODE_ID_TAG_H_ */
