/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2006,2007 INRIA
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */

#include "node-id-tag.h"
using namespace ns3;

// define this class in a public header

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