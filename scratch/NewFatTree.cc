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
std::map<std::string, uint32_t> FailNode_oif_map;

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

int main(int argc, char *argv[]) {
	Config::SetDefault("ns3::OnOffApplication::PacketSize", UintegerValue(210));
	Config::SetDefault("ns3::OnOffApplication::DataRate",
			StringValue("448kb/s"));
	Config::SetDefault("ns3::DropTailQueue::Mode",
			EnumValue(ns3::Queue::QUEUE_MODE_BYTES));

	// output-queued switch buffer size (max bytes of each fifo queue)
	Config::SetDefault("ns3::DropTailQueue::MaxBytes", UintegerValue(15000));
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

	// First create four set of Level-0 subnets
	uint32_t ip1;

	for (int i = 0; i < Port_num; i++)      // cycling with pods
			{
		for (int j = 0; j < Port_num / 2; j++) // cycling within every pod among Level 2 switches
				{
			node_l2switch.Get(i * (Port_num / 2) + j)->SetId_FatTree(i, j, 2); // labling the switch
			for (int k = 0; k < Port_num / 2; k++) // cycling within every level 2 switch
					{
				NetDeviceContainer link;
				NodeContainer node;
				Ptr<Node> serverNode = node_server.Get(
						i * (Port_num * Port_num / 4) + j * (Port_num / 2) + k);
				node.Add(serverNode);
				//node_server.Get(
//						i * (Port_num * Port_num / 4) + j * (Port_num / 2) + k)->SetId_FatTree(
//						i, j, k); // labling the host server

				serverNode->nodeId_FatTree = NodeId(i, j, k); // labling the host server
				node.Add(node_l2switch.Get(i * (Port_num / 2) + j));
				link = p2p.Install(node);
				// assign the ip address of the two device.
				ip1 = (10 << 24) + (i << 16) + (3 << 14) + (j << 8) + (k << 4);
				//std::cout<< "assign ip is "<< Ipv4Address(ip1)<<std::endl;
				address.SetBase(Ipv4Address(ip1), "255.255.255.248");
				server_ip = address.Assign(link);

				ServerIpMap[serverNode] = server_ip.GetAddress(0);
				IpServerMap[server_ip.GetAddress(0)] = serverNode;
			}

		}
	} // End of creating servers at level-0 and connecting it to first layer switch
	  // Now connect first layer switch with second layer
	for (int i = 0; i < Port_num; i++)  //cycling among pods
			{
		for (int j = 0; j < Port_num / 2; j++)  //cycling among switches
				{
			for (int k = 0; k < Port_num / 2; k++) //cycling among ports on one switch
					{
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

	int Pod[Port_num][Port_num * Port_num / 4] = { { 1, 2, 3, 4, 5, 6, 7, 8, 9,
			10, 11, 12, 13, 14, 15, 16 }, { 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12,
			13, 14, 15, 16, 1 }, { 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
			16, 1, 2 },
			{ 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 1, 2, 3 }, { 1, 5,
					9, 13, 2, 6, 10, 14, 3, 7, 11, 15, 4, 8, 12, 16 }, { 5, 9,
					13, 2, 6, 10, 14, 3, 7, 11, 15, 4, 8, 12, 16, 1 }, { 9, 13,
					2, 6, 10, 14, 3, 7, 11, 15, 4, 8, 12, 16, 1, 5 }, { 13, 2,
					6, 10, 14, 3, 7, 11, 15, 4, 8, 12, 16, 1, 5, 9 } };

	int temp = 0;
	for (int i = 0; i < Port_num; i++)	//cycling among the pods
			{
		for (int j = 0; j < Port_num / 2; j++)	//cycling among the pod switches
				{
			for (int k = 0; k < Port_num / 2; k++)//cycling among the ports on each switch
					{
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

	for (int i = 0; i < Port_num / 2; i++)    			// labling the l0switch
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
	Time::SetResolution(Time::NS);
	LogComponentEnable("UdpEchoClientApplication", LOG_LEVEL_INFO);
	LogComponentEnable("UdpEchoServerApplication", LOG_LEVEL_INFO);
//	LogComponentEnable("ns3::TcpSocketFactory", LOG_LEVEL_INFO);
//	UdpEchoServerHelper echoServer(9);
//	ApplicationContainer serverApps = echoServer.Install(node_server.Get(5));
//	serverApps.Start(Seconds(1.0));
//	serverApps.Stop(Seconds(15.0));
//	std::cout << "server id is " << node_server.Get(5)->GetId() << std::endl;
//	UdpEchoClientHelper echoClient(Ipv4Address("10.0.193.17"), 9);
//	10.0.193.17  ip of node 5
//	10.0.192.1 ip of node 0
//	10.0.193.33 ip of node 6
//	echoClient.SetAttribute("MaxPackets", UintegerValue(1));
//	echoClient.SetAttribute("Interval", TimeValue(Seconds(1.0)));
//	echoClient.SetAttribute("PacketSize", UintegerValue(1024));
//	ApplicationContainer clientApps = echoClient.Install(node_server.Get(32));
//	ApplicationContainer clientApps2 = echoClient.Install(node_server.Get(32));
//	ApplicationContainer clientApps3 = echoClient.Install(node_server.Get(32));
//	clientApps.Start(Seconds(2.0));
//	clientApps.Stop(Seconds(13.0));
//	clientApps2.Start(Seconds(6.0));
//	clientApps2.Stop(Seconds(13.0));
//	clientApps3.Start(Seconds(10.0));
//	clientApps3.Stop(Seconds(13.0));

	uint16_t port;
	port = 20;
	//---------------------------------Bulk and Sink Application-------------------------------//
	BulkSendHelper sourceTarget("ns3::TcpSocketFactory",
			(InetSocketAddress(Ipv4Address("10.0.193.17"), port)));
	BulkSendHelper source1("ns3::TcpSocketFactory",
			(InetSocketAddress(Ipv4Address("10.0.193.33"), port)));
	double startTime = 0.5;
	double stopTime = 100;
	sourceTarget.SetAttribute("MaxBytes", UintegerValue(0));
	source1.SetAttribute("MaxBytes", UintegerValue(0));
	ApplicationContainer sourceAppsTarget = sourceTarget.Install(node_server.Get(32));
	sourceAppsTarget.Start(Seconds(startTime));
	sourceAppsTarget.Stop(Seconds(stopTime));
	ApplicationContainer sourceApps1 = source1.Install(node_server.Get(18));
	sourceApps1.Start(Seconds(startTime));
	sourceApps1.Stop(Seconds(stopTime));
//	ApplicationContainer sourceApps2 = source1.Install(node_server.Get(23));
//	sourceApps2.Start(Seconds(startTime));
//	sourceApps2.Stop(Seconds(stopTime));


	PacketSinkHelper sinkTarget("ns3::TcpSocketFactory",
			(InetSocketAddress(Ipv4Address("10.0.193.17"), port)));
	ApplicationContainer sinkAppsTarget = sinkTarget.Install(node_server.Get(5));
	sinkAppsTarget.Start(Seconds(startTime));
	sinkAppsTarget.Stop(Seconds(stopTime));
	PacketSinkHelper sink1("ns3::TcpSocketFactory",
			(InetSocketAddress(Ipv4Address("10.0.193.33"), port)));
	ApplicationContainer sinkApps1 = sink1.Install(node_server.Get(6));
	sinkApps1.Start(Seconds(startTime));
	sinkApps1.Stop(Seconds(stopTime));
//	ApplicationContainer sinkApps2 = sink1.Install(node_server.Get(6));
//	sinkApps2.Start(Seconds(startTime));
//	sinkApps2.Stop(Seconds(stopTime));



//	p2p.EnablePcap("server tracing", node_server.Get(5)->GetDevice(1),true);
//	p2p.EnablePcap("server tracing2", node_server.Get(32)->GetDevice(1),true);
	//-------------------------------------------------------------------------------------------//
	//set failure schedule
	NodeContainer switchAll;
	switchAll.Add(node_l2switch);
	switchAll.Add(node_l1switch);
	switchAll.Add(node_l0switch);
	Simulator::Schedule(Seconds(5.0), setFailure);
	Simulator::Schedule(Seconds(10.0), clearFailure, switchAll);
	Simulator::Schedule(Seconds(50.0), setFailure);

	Simulator::Stop(Seconds(stopTime + 1));
	Simulator::Run();
	Simulator::Destroy();
	PacketSink* sinkStatisticTarget = dynamic_cast<PacketSink*>(&(*sinkAppsTarget.Get(0)));
	PacketSink* sinkStatisticCompare1 = dynamic_cast<PacketSink*>(&(*sinkApps1.Get(0)));
//	PacketSink* sinkStatisticCompare2 = dynamic_cast<PacketSink*>(&(*sinkApps2.Get(0)));
	std::cout<<"the target one: --------------------------------"<<std::endl;
	std::cout << "total size : " << sinkStatisticTarget->GetTotalRx() << std::endl;
	std::cout << "total number of packets : " << sinkStatisticTarget->packetN
			<< std::endl;
	std::cout << "total delay is : " << sinkStatisticTarget->totalDelay << std::endl;
	std::cout << "average delay is : "
			<< sinkStatisticTarget->totalDelay / sinkStatisticTarget->packetN << std::endl;
	std::cout << "Goodput is : "
			<< (sinkStatisticTarget->GetTotalRx() / (stopTime - startTime)) * 8.0
					/ 1000.0 << " kbps" << std::endl;
	/*--------------------------------Compare 1------------------------------------------*/
	std::cout<<"the compare 1 : --------------------------------"<<std::endl;
	std::cout << "total size : " << sinkStatisticCompare1->GetTotalRx() << std::endl;
	std::cout << "total number of packets : " << sinkStatisticCompare1->packetN
			<< std::endl;
	std::cout << "total delay is : " << sinkStatisticCompare1->totalDelay << std::endl;
	std::cout << "average delay is : "
			<< sinkStatisticCompare1->totalDelay / sinkStatisticCompare1->packetN << std::endl;
	std::cout << "Goodput is : "
			<< (sinkStatisticCompare1->GetTotalRx() / (stopTime - startTime)) * 8.0
					/ 1000.0 << " kbps" << std::endl;
	/*--------------------------------Compare 2------------------------------------------*/
//	std::cout<<"the compare 2 : --------------------------------"<<std::endl;
//	std::cout << "total size : " << sinkStatisticCompare2->GetTotalRx() << std::endl;
//	std::cout << "total number of packets : " << sinkStatisticCompare2->packetN
//			<< std::endl;
//	std::cout << "total delay is : " << sinkStatisticCompare2->totalDelay << std::endl;
//	std::cout << "average delay is : "
//			<< sinkStatisticCompare2->totalDelay / sinkStatisticCompare2->packetN << std::endl;
//	std::cout << "Goodput is : "
//			<< (sinkStatisticCompare2->GetTotalRx() / (stopTime - startTime)) * 8.0
//					/ 1000.0 << " kbps" << std::endl;
//	delete []Pod;
//	//Pod = NULL;

	NS_LOG_INFO("Done");
	std::cout << "done" << std::endl;
	return 0;
}        // End of program

