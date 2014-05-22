/*
 * node-id-tag.h
 *
 *  Created on: May 21, 2014
 *      Author: czhe
 */

#ifndef NODE_ID_TAG_H_
#define NODE_ID_TAG_H_

#include "ns3/tag.h"
//#include "ns3/uinteger.h"
#include <iostream>

using namespace ns3;

//extern std::map<Ipv4Address, Node> IpServerMap;
extern std::map<int, int> IpServerMap;

namespace ns3 {

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

TypeId NodeIdTag::GetTypeId(void) {
	static TypeId tid =
			TypeId("scratch::NodeIdTag").SetParent<Tag>().AddConstructor<
					NodeIdTag>();
	return tid;
}
TypeId NodeIdTag::GetInstanceTypeId(void) const {
	return GetTypeId();
}
uint32_t NodeIdTag::GetSerializedSize(void) const {
	return sizeof(uint16_t) * 3;
}
void NodeIdTag::Serialize(TagBuffer i) const {
	i.WriteU16(id_pod);
	i.WriteU16(id_switch);
	i.WriteU16(id_level);
}
void NodeIdTag::Deserialize(TagBuffer i) {
	id_pod = i.ReadU16();
	id_switch = i.ReadU16();
	id_level = i.ReadU16();
}
void NodeIdTag::Print(std::ostream &os) const {
	os << "node id = " << id_pod << "," << id_switch << "," << id_level;
}
}

#endif /* NODE_ID_TAG_H_ */
