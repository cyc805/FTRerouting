/*
 * NewFatTree.cc
 *
 *  Created on: Mar 20, 2014
 *      Author: chris
 */

#include<iostream>
#include<fstream>
#include<string>
#include<cassert>
#include<malloc.h>

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-helper.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/csma-module.h"
#include "ns3/ipv4-nix-vector-helper.h"
#include "ns3/netanim-module.h"
#include "ns3/constant-position-mobility-model.h"
#include "ns3/gnuplot.h"

#include "ns3/node.h"

using namespace ns3;
using namespace std;

std::map<Ptr<Node>, Ipv4Address> ServerIpMap;
std::map<Ipv4Address, Ptr<Node> > IpServerMap;

std::map<std::string, uint> serverLabel_id_map;
std::map<std::string, Ipv4Address> serverLabel_address_map;

//std::map<std::string, std::string> server_turning_map; // serverLabel-> turningSwitchLabel
std::vector<std::pair<std::string, std::string> > server_turning_pairs; // serverLabel-> turningSwitchLabel

std::map<std::string, uint32_t> FailNode_oif_map;
extern double startTimeFatTree;
extern double stopTimeFatTree;
extern uint32_t selectedNode;
extern double collectTime;
//extern double failTimeFatTree;
const int Port_num = 8; // set the switch number

void setFailure(void) {
	std::string failNodeId1 = "001";
	uint32_t failOif1 = 2;
	std::string failNodeId2 = "012";
	uint32_t failOif2 = 5;
	FailNode_oif_map[failNodeId1] = failOif1;
	FailNode_oif_map[failNodeId2] = failOif2;
}
void clearFailure(NodeContainer switchAll) {
	FailNode_oif_map.clear();
	for (uint i = 0; i < switchAll.GetN(); i++) {
		switchAll.Get(i)->reRoutingMap.clear();
	}

}
NS_LOG_COMPONENT_DEFINE("first");

std::string i2s(uint i) {
	std::string s;
	std::stringstream out;
	out << i;
	s = out.str();
	return s;
}

int main(int argc, char *argv[]) {
	/*-------------------parameter setting----------------------*/
	startTimeFatTree = 0.0;
	stopTimeFatTree = 10.0;
	collectTime = 5.0;
	selectedNode = 129; //Id 167 is the aggregate switch for flow(18->5) and flow(20->113)
						//Id 171 is the aggregate switch for flow(35->5) and flow(40->100)
						//Id 129 is the edge switch for flow(18->5) and flow(35->5)
	double failTimeFatTree = collectTime;
//Changes seed from default of 1 to a specific number

	SeedManager::SetSeed(6);
//Changes run number (scenario number) from default of 1 to a specific runNumber
	SeedManager::SetRun(5);

//	Config::SetDefault("ns3::OnOffApplication::PacketSize", UintegerValue(210));
//	Config::SetDefault("ns3::OnOffApplication::DataRate",
//			StringValue("448kb/s"));
	Config::SetDefault("ns3::DropTailQueue::Mode",
			EnumValue(ns3::Queue::QUEUE_MODE_BYTES));
//
//	Config::SetDefault("ns3::DropTailQueue::Mode",
//			EnumValue(ns3::Queue::QUEUE_MODE_PACKETS));

	// output-queued switch buffer size (max bytes of each fifo queue)
	Config::SetDefault("ns3::DropTailQueue::MaxBytes", UintegerValue(15000));
//	Config::SetDefault("ns3::DropTailQueue::MaxPackets", UintegerValue(25));
//	Config::SetDefault("ns3::RedQueue::Mode",
//			EnumValue(ns3::Queue::QUEUE_MODE_BYTES));
//
////	 output-queued switch buffer size (max bytes of each fifo queue)
//	Config::SetDefault("ns3::RedQueue::MinTh", DoubleValue(5000.0));
//	Config::SetDefault("ns3::RedQueue::MaxTh", DoubleValue(15000.0));
//	Config::SetDefault("ns3::RedQueue::QueueLimit", UintegerValue(15000));
	/*-------------------------------------------------------------------------*/
	CommandLine cmd;
	bool enableMonitor = true;
	cmd.AddValue("EnableMonitor", "Enable Flow Monitor", enableMonitor);

	cmd.Parse(argc, argv);

	Ipv4InterfaceContainer p2p_ip;
	InternetStackHelper internet;
	Ipv4InterfaceContainer server_ip;
	Ipv4AddressHelper address;
	NS_LOG_INFO("Create Nodes");
	// Create nodes for servers
	NodeContainer node_server;
	node_server.Create(Port_num * Port_num * Port_num / 4);
	internet.Install(node_server);
	// Create nodes for Level-2 switches
	NodeContainer node_l2switch;
	node_l2switch.Create(Port_num * Port_num / 2);
	internet.Install(node_l2switch);
	// Create nodes for Level-1 switches
	NodeContainer node_l1switch;
	node_l1switch.Create(Port_num * Port_num / 2);
	internet.Install(node_l1switch);
	// Create nodes for Level-0 switches
	NodeContainer node_l0switch;
	node_l0switch.Create(Port_num * Port_num / 4);
	internet.Install(node_l0switch);
	// Point to Point Helper to all the links in fat-tree include the server subnets
	PointToPointHelper p2p;
	p2p.SetDeviceAttribute("DataRate", StringValue("5Mbps"));
	p2p.SetChannelAttribute("Delay", StringValue("1ms"));
//	p2p.SetQueue(
//			"ns3::RedQueue");
//		p2p.SetDeviceAttribute("DataRate", StringValue("5Mbps"));
//		p2p.SetChannelAttribute("Delay", StringValue("1ms"));
//	// First create four set of Level-0 subnets
	uint32_t ip1;

	for (int i = 0; i < Port_num; i++) {     // cycling with pods
		for (int j = 0; j < Port_num / 2; j++) { // cycling within every pod among Level 2 switches
			node_l2switch.Get(i * (Port_num / 2) + j)->SetId_FatTree(i, j, 2); // labeling the switch
			for (int k = 0; k < Port_num / 2; k++) { // cycling within every level 2 switch

				NetDeviceContainer link;
				NodeContainer node;
				Ptr<Node> serverNode = node_server.Get(
						i * (Port_num * Port_num / 4) + j * (Port_num / 2) + k);
				node.Add(serverNode);
				//node_server.Get(
//						i * (Port_num * Port_num / 4) + j * (Port_num / 2) + k)->SetId_FatTree(
//						i, j, k); // labling the host server

				serverNode->nodeId_FatTree = NodeId(i, j, k); // labeling the host server

				std::string serverTag = i2s(i) + i2s(j) + i2s(k);
				serverLabel_id_map[serverTag] = serverNode->GetId();

				node.Add(node_l2switch.Get(i * (Port_num / 2) + j));
				link = p2p.Install(node);
				// assign the ip address of the two device.
				ip1 = (10 << 24) + (i << 16) + (3 << 14) + (j << 8) + (k << 4);
				//std::cout<< "assign ip is "<< Ipv4Address(ip1)<<std::endl;
				address.SetBase(Ipv4Address(ip1), "255.255.255.248");
				server_ip = address.Assign(link);

				ServerIpMap[serverNode] = server_ip.GetAddress(0);
				IpServerMap[server_ip.GetAddress(0)] = serverNode;
				serverLabel_address_map[serverTag] = server_ip.GetAddress(0);
			}

		}
	} // End of creating servers at level-0 and connecting it to first layer switch
	  // Now connect first layer switch with second layer
	for (int i = 0; i < Port_num; i++) { //cycling among pods
		for (int j = 0; j < Port_num / 2; j++) {  //cycling among switches
			for (int k = 0; k < Port_num / 2; k++) { //cycling among ports on one switch
				NetDeviceContainer link;
				NodeContainer node;
				node.Add(node_l2switch.Get(i * (Port_num / 2) + j));
				node.Add(node_l1switch.Get(i * (Port_num / 2) + k));
				node_l1switch.Get(i * (Port_num / 2) + k)->SetId_FatTree(i, k,
						1);
				link = p2p.Install(node);
				ip1 = (10 << 24) + (i << 16) + (2 << 14) + (j << 8) + (k << 4);
				address.SetBase(Ipv4Address(ip1), "255.255.255.248");
				p2p_ip = address.Assign(link);
			}
		}
	}  //end of the connections construct between l2 switches and l1 switches
	   // Now connect the core switches and the l2 switches

	   // Efficient fat-tree (k=8) construction (shuffle pattern between core and aggregation layers)
	/*int Pod[Port_num][Port_num * Port_num / 4] = { { 1, 2, 3, 4, 5, 6, 7, 8, 9,
	 10, 11, 12, 13, 14, 15, 16 }, { 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12,
	 13, 14, 15, 16, 1 }, { 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
	 16, 1, 2 },
	 { 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 1, 2, 3 }, { 1, 5,
	 9, 13, 2, 6, 10, 14, 3, 7, 11, 15, 4, 8, 12, 16 }, { 5, 9,
	 13, 2, 6, 10, 14, 3, 7, 11, 15, 4, 8, 12, 16, 1 }, { 9, 13,
	 2, 6, 10, 14, 3, 7, 11, 15, 4, 8, 12, 16, 1, 5 }, { 13, 2,
	 6, 10, 14, 3, 7, 11, 15, 4, 8, 12, 16, 1, 5, 9 } };*/

	// Regular fat-tree (k=8) construction (shuffle pattern between core and aggregation layers)
	int Pod[Port_num][Port_num * Port_num / 4] = { { 1, 2, 3, 4, 5, 6, 7, 8, 9,
			10, 11, 12, 13, 14, 15, 16 }, { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11,
			12, 13, 14, 15, 16 }, { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13,
			14, 15, 16 }, { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
			16 }, { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16 }, {
			1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16 }, { 1, 2, 3,
			4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16 }, { 1, 2, 3, 4, 5, 6,
			7, 8, 9, 10, 11, 12, 13, 14, 15, 16 } };

	int temp = 0;
	for (int i = 0; i < Port_num; i++) {	//cycling among the pods
		for (int j = 0; j < Port_num / 2; j++) {//cycling among the pod switches
			for (int k = 0; k < Port_num / 2; k++) {//cycling among the ports on each switch
				temp = Pod[i][j * (Port_num / 2) + k];
				NetDeviceContainer link;
				NodeContainer node;
				node.Add(node_l1switch.Get(i * (Port_num / 2) + j));
				node.Add(node_l0switch.Get(temp - 1));
				link = p2p.Install(node);
				ip1 = (10 << 24) + (i << 16) + (1 << 14) + (j << 8) + (k << 4);
				address.SetBase(Ipv4Address(ip1), "255.255.255.248");
				p2p_ip = address.Assign(link);
			}

		}
	}

	for (int i = 0; i < Port_num / 2; i++)    	// labeling the level 0 switches
			{
		for (int j = 0; j < Port_num / 2; j++) {
			node_l0switch.Get(i * (Port_num / 2) + j)->SetId_FatTree(i, j, 0);
		}
	}
	/*--------------------------verify the ID---------------------------------------*/
//	for (int i = 0; i < (Port_num * Port_num * Port_num / 4); i++) //show the server ID
//			{
//		std::cout << "the ID for the server is";
//		node_server.Get(i)->nodeId_FatTree.Print(std::cout);
//	}
//	for (int i = 0; i < (Port_num * Port_num / 2); i++) //show the l1switch and l2switch ID
//			{
//		std::cout << "the ID for the l1switch is";
//		node_l1switch.Get(i)->nodeId_FatTree.Print(std::cout);
//
//		std::cout << "the ID for the l2switch is";
//		node_l2switch.Get(i)->nodeId_FatTree.Print(std::cout);
//	}
//	for (int i = 0; i < (Port_num * Port_num / 4); i++) {
//		std::cout << "the ID for the l0switch is";
//		node_l0switch.Get(i)->nodeId_FatTree.Print(std::cout);
//	}
	Ipv4GlobalRoutingHelper::PopulateRoutingTables();
// set fail link//

// set application
//	Time::SetResolution(Time::NS);
//	LogComponentEnable("UdpEchoClientApplication", LOG_LEVEL_INFO);
//	LogComponentEnable("UdpEchoServerApplication", LOG_LEVEL_INFO);
//	LogComponentEnable("ns3::TcpSocketFactory", LOG_LEVEL_INFO);
//	UdpEchoServerHelper echoServer(9);
//	ApplicationContainer serverApps = echoServer.Install(node_server.Get(5));
//	ApplicationContainer serverApps2 = echoServer.Install(node_server.Get(113));
//	serverApps.Start(Seconds(1.0));
//	serverApps.Stop(Seconds(15.0));
//	serverApps2.Start(Seconds(1.0));
//	serverApps2.Stop(Seconds(15.0));
//	std::cout << "server id is " << node_server.Get(5)->GetId() << std::endl;
//	UdpEchoClientHelper echoClient(Ipv4Address("10.0.193.17"), 9);
//	UdpEchoClientHelper echoClient2(Ipv4Address("10.7.192.17"), 9);
//	10.0.193.17  ip of node 5
//	10.0.192.1 ip of node 0
//	10.0.193.33 ip of node 6
//	echoClient.SetAttribute("MaxPackets", UintegerValue(1));
//	echoClient.SetAttribute("Interval", TimeValue(Seconds(1.0)));
//	echoClient.SetAttribute("PacketSize", UintegerValue(1024));
//	ApplicationContainer clientApps = echoClient.Install(node_server.Get(18));
//	ApplicationContainer clientApps2 = echoClient2.Install(node_server.Get(20));
//	ApplicationContainer clientApps3 = echoClient.Install(node_server.Get(32));
//	clientApps.Start(Seconds(2.0));
//	clientApps.Stop(Seconds(13.0));
//	clientApps2.Start(Seconds(6.0));
//	clientApps2.Stop(Seconds(13.0));
//	clientApps3.Start(Seconds(10.0));
//	clientApps3.Stop(Seconds(13.0));

	uint16_t dst_port;
	dst_port = 20;
//	Config::Set("/NodeList/171/DeviceList/5/TxQueue/TraceDrop",
//			UintegerValue(1));
//	Config::Set("/NodeList/167/DeviceList/5/TxQueue/TraceDrop",
//			UintegerValue(1));
	/*	Config::Set("/NodeList/129/DeviceList/2/TxQueue/TraceDrop",
	 UintegerValue(1));*/// by Chunzhi
//	Config::Set("/NodeList/129/DeviceList/2/TxQueue/TraceDrop",
//			UintegerValue(1));
//set failure schedule
	NodeContainer switchAll;
	switchAll.Add(node_l2switch);
	switchAll.Add(node_l1switch);
	switchAll.Add(node_l0switch);
	Simulator::Schedule(Seconds(failTimeFatTree), setFailure);
//	Simulator::Schedule(Seconds(10.0), clearFailure, switchAll);
//	Simulator::Schedule(Seconds(50.0), setFailure);
//---------------------------------Bulk and Sink Application-------------------------------//
	BulkSendHelper sourceTarget("ns3::TcpSocketFactory",
			(InetSocketAddress(serverLabel_address_map["011"], dst_port))); // Node(5), NodeTag("011")
	BulkSendHelper source1("ns3::TcpSocketFactory",
			(InetSocketAddress(serverLabel_address_map["011"], dst_port + 1)));
//	BulkSendHelper source2("ns3::TcpSocketFactory",
//			(InetSocketAddress(serverTag_address_map["012"], port))); // Node(6), NodeTag("012")
	BulkSendHelper source3("ns3::TcpSocketFactory",
			(InetSocketAddress(serverLabel_address_map["701"], dst_port))); // Node(113), NodeTag("701")
	BulkSendHelper source4("ns3::TcpSocketFactory",
			(InetSocketAddress(serverLabel_address_map["610"], dst_port))); //Node(100), NodeTag("610")
//	BulkSendHelper source5("ns3::TcpSocketFactory",
//			(InetSocketAddress(serverTag_address_map["302"], port + 5))); // Node(50), NodeTag("302")

	/*----------------------------------Test---------------------------------*/
//	BulkSendHelper sourceTest("ns3::TcpSocketFactory",
//			(InetSocketAddress(Ipv4Address("10.3.192.33"), port + 5))); // Node(50)
//	sourceTest.SetAttribute("MaxBytes", UintegerValue(0));
//	ApplicationContainer sourceAppsTest = sourceTest.Install(node_server.Get(78)); //NodeId(220)
//	sourceAppsTest.Start(Seconds(startTimeFatTree));
//	sourceAppsTest.Stop(Seconds(stopTimeFatTree));
//
//	BulkSendHelper sourceTest2("ns3::TcpSocketFactory",
//			(InetSocketAddress(Ipv4Address("10.5.194.17"), port + 5))); // Node(50)
//	sourceTest2.SetAttribute("MaxBytes", UintegerValue(0));
//	ApplicationContainer sourceAppsTest2 = sourceTest2.Install(node_server.Get(122)); //NodeId(220)
//	sourceAppsTest2.Start(Seconds(startTimeFatTree));
//	sourceAppsTest2.Stop(Seconds(stopTimeFatTree));
//
//	PacketSinkHelper sinkTest2("ns3::TcpSocketFactory",
//			(InetSocketAddress(Ipv4Address("10.5.194.17"), port + 5)));
//	ApplicationContainer sinkAppsTest2 = sinkTest2.Install(node_server.Get(89));
//	sinkAppsTest2.Start(Seconds(startTimeFatTree));
//	sinkAppsTest2.Stop(Seconds(stopTimeFatTree));
	/*------------------------------------------------------------------------*/

	sourceTarget.SetAttribute("MaxBytes", UintegerValue(0));
	source1.SetAttribute("MaxBytes", UintegerValue(0));
//	source2.SetAttribute("MaxBytes", UintegerValue(0));
	source3.SetAttribute("MaxBytes", UintegerValue(0));
	source4.SetAttribute("MaxBytes", UintegerValue(0));
//	source5.SetAttribute("MaxBytes", UintegerValue(0));
	ApplicationContainer sourceAppsTarget = sourceTarget.Install(
			node_server.Get(serverLabel_id_map["203"])); // "203" -> "011"
	sourceAppsTarget.Start(Seconds(startTimeFatTree));
	sourceAppsTarget.Stop(Seconds(stopTimeFatTree));
	ApplicationContainer sourceApps1 = source1.Install(
			node_server.Get(serverLabel_id_map["102"])); // "102" -> "011"
	sourceApps1.Start(Seconds(startTimeFatTree));
	sourceApps1.Stop(Seconds(stopTimeFatTree));
//	ApplicationContainer sourceApps2 = source2.Install(node_server.Get(serverTag_id_map["113"])); // NodeTag("113")
//	sourceApps2.Start(Seconds(startTimeFatTree));
//	sourceApps2.Stop(Seconds(stopTimeFatTree));
	ApplicationContainer sourceApps3 = source3.Install(
			node_server.Get(serverLabel_id_map["110"])); // "110" -> "701"
	sourceApps3.Start(Seconds(startTimeFatTree));
	sourceApps3.Stop(Seconds(stopTimeFatTree));
	ApplicationContainer sourceApps4 = source4.Install(
			node_server.Get(serverLabel_id_map["220"])); // "220" -> "610"
	sourceApps4.Start(Seconds(startTimeFatTree));
	sourceApps4.Stop(Seconds(stopTimeFatTree));
//	ApplicationContainer sourceApps5 = source5.Install(node_server.Get(serverTag_id_map["111"])); // NodeTag("111")
//	sourceApps5.Start(Seconds(startTimeFatTree));
//	sourceApps5.Stop(Seconds(stopTimeFatTree));

	// server_label -> turning_switch_label
	// Note that the adding order is important, i.e. the first matching pair is used.
//	server_turning_pairs.push_back(std::make_pair("203", "32x"));
//	server_turning_pairs.push_back(std::make_pair("102", "31x"));
	server_turning_pairs.push_back(std::make_pair("110", "31x"));
	server_turning_pairs.push_back(std::make_pair("220", "32x"));
	server_turning_pairs.push_back(std::make_pair("011", "00x"));
	server_turning_pairs.push_back(std::make_pair("111", "31x"));

//	BulkSendApplication* sendTarget =
//			dynamic_cast<BulkSendApplication*>(&(*sourceAppsTarget.Get(0)));
//	BulkSendApplication* send1 =
//			dynamic_cast<BulkSendApplication*>(&(*sourceApps1.Get(0)));
//	BulkSendApplication* send3 =
//			dynamic_cast<BulkSendApplication*>(&(*sourceApps3.Get(0)));
//	BulkSendApplication* send4 =
//			dynamic_cast<BulkSendApplication*>(&(*sourceApps4.Get(0)));
////	BulkSendApplication* send5 =
////			dynamic_cast<BulkSendApplication*>(&(*sourceApps5.Get(0)));
//	sendTarget->SetAttribute("TraceCwndOutputFile", StringValue("cwnd35.txt"));
//	send1->SetAttribute("TraceCwndOutputFile", StringValue("cwnd18.txt"));
//	send3->SetAttribute("TraceCwndOutputFile", StringValue("cwnd20.txt"));
//	send4->SetAttribute("TraceCwndOutputFile", StringValue("cwnd40.txt"));
////	send5->SetAttribute("TraceCwndOutputFile", StringValue("cwnd21.txt"));

	PacketSinkHelper sinkTarget("ns3::TcpSocketFactory",
			(InetSocketAddress(serverLabel_address_map["011"], dst_port))); // Node(5), NodeTag("011")
	PacketSinkHelper sink1("ns3::TcpSocketFactory",
			(InetSocketAddress(serverLabel_address_map["011"], dst_port + 1)));
	PacketSinkHelper sink2("ns3::TcpSocketFactory",
			(InetSocketAddress(serverLabel_address_map["012"], dst_port))); // Node(6), NodeTag("012")
	PacketSinkHelper sink3("ns3::TcpSocketFactory",
			(InetSocketAddress(serverLabel_address_map["701"], dst_port))); // Node(113), NodeTag("701")
	PacketSinkHelper sink4("ns3::TcpSocketFactory",
			(InetSocketAddress(serverLabel_address_map["610"], dst_port))); //Node(100), NodeTag("610")
	PacketSinkHelper sink5("ns3::TcpSocketFactory",
			(InetSocketAddress(serverLabel_address_map["302"], dst_port + 5))); // Node(50), NodeTag("302")

	ApplicationContainer sinkAppsTarget = sinkTarget.Install(
			node_server.Get(serverLabel_id_map["011"]));
	sinkAppsTarget.Start(Seconds(startTimeFatTree));
	sinkAppsTarget.Stop(Seconds(stopTimeFatTree));
	ApplicationContainer sinkApps1 = sink1.Install(
			node_server.Get(serverLabel_id_map["011"])); // NodeTag("011")
	sinkApps1.Start(Seconds(startTimeFatTree));
	sinkApps1.Stop(Seconds(stopTimeFatTree));
	ApplicationContainer sinkApps2 = sink2.Install(
			node_server.Get(serverLabel_id_map["012"])); // NodeTag("012")
	sinkApps2.Start(Seconds(startTimeFatTree));
	sinkApps2.Stop(Seconds(stopTimeFatTree));
	ApplicationContainer sinkApps3 = sink3.Install(
			node_server.Get(serverLabel_id_map["701"])); // NodeTag("701")
	sinkApps3.Start(Seconds(startTimeFatTree));
	sinkApps3.Stop(Seconds(stopTimeFatTree));
	ApplicationContainer sinkApps4 = sink4.Install(
			node_server.Get(serverLabel_id_map["610"])); // NodeTag("610")
	sinkApps4.Start(Seconds(startTimeFatTree));
	sinkApps4.Stop(Seconds(stopTimeFatTree));
	ApplicationContainer sinkApps5 = sink5.Install(
			node_server.Get(serverLabel_id_map["302"])); // NodeTag("302")
	sinkApps5.Start(Seconds(startTimeFatTree));
	sinkApps5.Stop(Seconds(stopTimeFatTree));

	std::cout << serverLabel_id_map["011"] << std::endl;

//	p2p.EnablePcap("server tracing 35", node_server.Get(35)->GetDevice(1),
//			true);
//	p2p.EnablePcap("server tracing 18", node_server.Get(18)->GetDevice(1),
//			true);
//	p2p.EnablePcap("server tracing 20", node_server.Get(20)->GetDevice(1),
//			true);
//	p2p.EnablePcap("server tracing 40", node_server.Get(40)->GetDevice(1),
//			true);
//	p2p.EnablePcap("server tracing 23", node_server.Get(23)->GetDevice(1),
//			true);
//	p2p.EnablePcap("server tracing 21", node_server.Get(21)->GetDevice(1),
//			true);

//----------------------------------Simulation result-----------------------------------------------//
	Simulator::Stop(Seconds(stopTimeFatTree + 0.01));
	Simulator::Run();
	Simulator::Destroy();
	PacketSink* sinkStatisticTarget =
			dynamic_cast<PacketSink*>(&(*sinkAppsTarget.Get(0)));
	PacketSink* sinkStatisticCompare1 =
			dynamic_cast<PacketSink*>(&(*sinkApps1.Get(0)));
	PacketSink* sinkStatisticCompare2 =
			dynamic_cast<PacketSink*>(&(*sinkApps2.Get(0)));
	PacketSink* sinkStatisticCompare3 =
			dynamic_cast<PacketSink*>(&(*sinkApps3.Get(0)));
	PacketSink* sinkStatisticCompare4 =
			dynamic_cast<PacketSink*>(&(*sinkApps4.Get(0)));
	PacketSink* sinkStatisticCompare5 =
			dynamic_cast<PacketSink*>(&(*sinkApps5.Get(0)));
	std::cout << "the target one(35->5): --------------------------------"
			<< std::endl;
	std::cout << "total size : " << sinkStatisticTarget->GetTotalRx()
			<< std::endl;
	std::cout << "total number of packets : " << sinkStatisticTarget->packetN
			<< std::endl;
	std::cout << "total delay is : " << sinkStatisticTarget->totalDelay
			<< std::endl;
	std::cout << "average delay is : "
			<< sinkStatisticTarget->totalDelay / sinkStatisticTarget->packetN
			<< std::endl;
	std::cout << "Goodput is : "
			<< (sinkStatisticTarget->GetTotalRx()
					/ (stopTimeFatTree - startTimeFatTree - collectTime)) * 8.0
					/ 1000.0 << " kbps" << std::endl;
	/*--------------------------------Compare 1------------------------------------------*/
	std::cout
			<< "the Target two (same Dst with Target one)(18->5) : --------------------------------"
			<< std::endl;
	std::cout << "total size : " << sinkStatisticCompare1->GetTotalRx()
			<< std::endl;
	std::cout << "total number of packets : " << sinkStatisticCompare1->packetN
			<< std::endl;
	std::cout << "total delay is : " << sinkStatisticCompare1->totalDelay
			<< std::endl;
	std::cout << "average delay is : "
			<< sinkStatisticCompare1->totalDelay
					/ sinkStatisticCompare1->packetN << std::endl;
	std::cout << "Goodput is : "
			<< (sinkStatisticCompare1->GetTotalRx()
					/ (stopTimeFatTree - startTimeFatTree - collectTime)) * 8.0
					/ 1000.0 << " kbps" << std::endl;
	/*--------------------------------Compare 2------------------------------------------*/
	std::cout
			<< "the compare one (share same path in the Dst Pod(23->6), used in LRD Algorithm) : -----------------------"
			<< std::endl;
	std::cout << "total size : " << sinkStatisticCompare2->GetTotalRx()
			<< std::endl;
	std::cout << "total number of packets : " << sinkStatisticCompare2->packetN
			<< std::endl;
	std::cout << "total delay is : " << sinkStatisticCompare2->totalDelay
			<< std::endl;
	std::cout << "average delay is : "
			<< sinkStatisticCompare2->totalDelay
					/ sinkStatisticCompare2->packetN << std::endl;
	std::cout << "Goodput is : "
			<< (sinkStatisticCompare2->GetTotalRx()
					/ (stopTimeFatTree - startTimeFatTree - collectTime)) * 8.0
					/ 1000.0 << " kbps" << std::endl;
	/*--------------------------------Compare 3------------------------------------------*/
	std::cout
			<< "the compare two (share same path in the Src Pod(20->113), used in LRB Algorithm) : -----------------------"
			<< std::endl;
	std::cout << "total size : " << sinkStatisticCompare3->GetTotalRx()
			<< std::endl;
	std::cout << "total number of packets : " << sinkStatisticCompare3->packetN
			<< std::endl;
	std::cout << "total delay is : " << sinkStatisticCompare3->totalDelay
			<< std::endl;
	std::cout << "average delay is : "
			<< sinkStatisticCompare3->totalDelay
					/ sinkStatisticCompare3->packetN << std::endl;
	std::cout << "Goodput is : "
			<< (sinkStatisticCompare3->GetTotalRx()
					/ (stopTimeFatTree - startTimeFatTree - collectTime)) * 8.0
					/ 1000.0 << " kbps" << std::endl;
	/*--------------------------------Compare 4------------------------------------------*/
	std::cout
			<< "the compare two (share same path in the Src Pod(21->50), used in LRB Algorithm) : -----------------------"
			<< std::endl;
	std::cout << "total size : " << sinkStatisticCompare5->GetTotalRx()
			<< std::endl;
	std::cout << "total number of packets : " << sinkStatisticCompare5->packetN
			<< std::endl;
	std::cout << "total delay is : " << sinkStatisticCompare5->totalDelay
			<< std::endl;
	std::cout << "average delay is : "
			<< sinkStatisticCompare5->totalDelay
					/ sinkStatisticCompare5->packetN << std::endl;
	std::cout << "Goodput is : "
			<< (sinkStatisticCompare5->GetTotalRx()
					/ (stopTimeFatTree - startTimeFatTree - collectTime)) * 8.0
					/ 1000.0 << " kbps" << std::endl;
	/*--------------------------------Compare 5------------------------------------------*/
	std::cout
			<< "the compare three (share same path in the Src Pod(40->100), used in LRB Algorithm) : -----------------------"
			<< std::endl;
	std::cout << "total size : " << sinkStatisticCompare4->GetTotalRx()
			<< std::endl;
	std::cout << "total number of packets : " << sinkStatisticCompare4->packetN
			<< std::endl;
	std::cout << "total delay is : " << sinkStatisticCompare4->totalDelay
			<< std::endl;
	std::cout << "average delay is : "
			<< sinkStatisticCompare4->totalDelay
					/ sinkStatisticCompare4->packetN << std::endl;
	std::cout << "Goodput is : "
			<< (sinkStatisticCompare4->GetTotalRx()
					/ (stopTimeFatTree - startTimeFatTree - collectTime)) * 8.0
					/ 1000.0 << " kbps" << std::endl;
//	delete []Pod;
//	//Pod = NULL;
	NS_LOG_INFO("Done");
	std::cout << "done" << std::endl;
	return 0;
}        // End of program

