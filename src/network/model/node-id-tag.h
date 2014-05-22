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
#include "ns3/core-module.h"
#include "ns3/ipv4-address.h"

using namespace ns3;

//extern std::map<Ipv4Address, Node> IpServerMap;


namespace ns3 {

class NodeIdTag: public Tag {
public:
	static TypeId GetTypeId(void);
	virtual TypeId GetInstanceTypeId(void) const;
	virtual uint32_t GetSerializedSize(void) const;
	virtual void Serialize(TagBuffer i) const;
	virtual void Deserialize(TagBuffer i);
	virtual void Print(std::ostream &os) const;
	NodeIdTag();
	NodeIdTag(uint32_t id_pod, uint32_t id_switch, uint32_t id_level);

	uint32_t id_pod, id_switch, id_level;

};
NodeIdTag::NodeIdTag(){

	id_pod = 0xffffffff;
	id_switch = 0xffffffff;
	id_level = 0xffffffff;

}
NodeIdTag::NodeIdTag(uint32_t id_pod_, uint32_t id_switch_, uint32_t id_level_){

	id_pod = id_pod_;
	id_switch = id_switch_;
	id_level = id_level_;

}

TypeId NodeIdTag::GetTypeId(void) {
	static TypeId tid =
			TypeId("scratch::NodeIdTag").SetParent<Tag>();
	return tid;
}
TypeId NodeIdTag::GetInstanceTypeId(void) const {
	return GetTypeId();
}
uint32_t NodeIdTag::GetSerializedSize(void) const {
	return sizeof(uint32_t) * 3;
}
void NodeIdTag::Serialize(TagBuffer i) const {
	i.WriteU32(id_pod);
	i.WriteU32(id_switch);
	i.WriteU32(id_level);
}
void NodeIdTag::Deserialize(TagBuffer i) {
	id_pod = i.ReadU32();
	id_switch = i.ReadU32();
	id_level = i.ReadU32();
}
void NodeIdTag::Print(std::ostream &os) const {
	os << "node id = " << id_pod << "," << id_switch << "," << id_level;
}
}
extern std::map<Ipv4Address, NodeIdTag> IpServerMap;
#endif /* NODE_ID_TAG_H_ */
