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

using namespace ns3;
using namespace std;
const int Port_num = 8; // set the switch number
/**
 *This function is used to construct the ip address
 *of the nodes in the FatTree Topology;
 *Param pod , pswitch, num indicate the pod number, the pswitch num in every pod
 *and the port num in every switch; param level indicate the node level in the whole
 *topology, core switch is in level 0, and servers are in level 3.
 *param port_level, indicate the port level in every aggregate switch. the upper half is 1;
 */
uint32_t construct_ip(int pod, int pswitch, int num, int level, int port_level)
{
	uint32_t ip;
	ip = 10;
	ip = ip<<24;

	if (level==0)
	{
		ip+= (0<<16);
		ip+= (pod)<<8;
		ip+= (num<<4);
		return ip;
	}
	else
	{
		//ip+= ((pod<<4)+pswitch)<<16;
		if (level==1)
		{
			if(port_level == 0)
			{
				ip += 1<<16;
				ip+= ((pod<<4)+pswitch)<<8;
				ip += (num<<4);
				return ip;
			}
			else
			{
				ip += 1<<16;
				ip+= ((pod<<4)+pswitch)<<8;
				ip += ((num+Port_num/2)<<4);
				return ip;
			}
		}
		if (level==2)
		{
			if(port_level == 0)
			{
				ip += 2<<16;
				ip+= ((pod<<4)+pswitch)<<8;
				ip += (num<<4);
				return ip;
			}
			else
			{
				ip += 2<<16;
				ip+= ((pod<<4)+pswitch)<<8;
				ip += ((num+Port_num/2)<<4);
				return ip;
			}
		}
		if (level==3)
		{
			ip += 3<<16;
			ip+= ((pod<<4)+pswitch)<<8;
			ip += (num<<4);
			return ip;
		}
	}
	return 0;
}
void printRoutingTable(Ptr<Node> node)
{
    Ipv4StaticRoutingHelper helper;
	//Ipv4GlobalRoutingHelper helper;
    Ptr<Ipv4> stack = node->GetObject<Ipv4>();
    Ptr<Ipv4StaticRouting> staticRouting = helper.GetStaticRouting(stack);
    //Ptr<Ipv4GlobalRouting> staticRouting = helper.
    uint32_t numroutes = staticRouting->GetNRoutes();
    Ipv4RoutingTableEntry entry;
    std::cout << "Routing table for device: " << Names::FindName(node) << "\n";
    std::cout << "Destination\t Mask \t\t Gateway \t\t Iface \n";
    for(uint32_t i=0; i < numroutes; i++)
    {
         entry = staticRouting->GetRoute(i);
	 std::cout << entry.GetDestNetwork() << "\t" << entry.GetDestNetworkMask() << "\t" << entry.GetGateway() << "\t\t" << entry.GetInterface() << "\n";
    }
    return;
}
NS_LOG_COMPONENT_DEFINE("first");

int main(int argc, char *argv[])
{
      Config::SetDefault ("ns3::OnOffApplication::PacketSize", UintegerValue(210));
      Config::SetDefault ("ns3::OnOffApplication::DataRate", StringValue("448kb/s"));

      CommandLine cmd;
      bool enableMonitor = true;
      cmd.AddValue("EnableMonitor", "Enable Flow Monitor", enableMonitor);

      cmd.Parse(argc, argv);

      std::map<Ipv4Address, Ptr<Node> > IpServerMap;
      Ipv4InterfaceContainer p2p_ip;
      InternetStackHelper internet;
      Ipv4InterfaceContainer server_ip;
      Ipv4AddressHelper address;
      NS_LOG_INFO("Create Nodes");
      // Create nodes for servers
      NodeContainer node_server;
      node_server.Create(Port_num*Port_num*Port_num/4);
      internet.Install(node_server);
      // Create nodes for Level-2 switches
      NodeContainer node_l2switch;
      node_l2switch.Create(Port_num*Port_num/2);
      internet.Install(node_l2switch);
      // Create nodes for Level-1 switches
      NodeContainer node_l1switch;
      node_l1switch.Create(Port_num*Port_num/2);
      internet.Install(node_l1switch);
      // Create nodes for Level-0 switches
      NodeContainer node_l0switch;
      node_l0switch.Create(Port_num*Port_num/4);
      internet.Install(node_l0switch);
      // Point to Point Helper to all the links in fat-tree include the server subnets
      PointToPointHelper p2p;
      p2p.SetDeviceAttribute("DataRate", StringValue("5Mbps"));
      p2p.SetChannelAttribute("Delay", StringValue("10ms"));

      // First create four set of Level-0 subnets
      //uint32_t ip1, ip2;
      uint32_t ip1;

      for(int i=0;i < Port_num; i++)// cycling with pods
      {
           for(int j=0;j < Port_num/2; j++)// cycling within every pod among Level 2 switches
           {
        	   node_l2switch.Get(i*(Port_num/2)+j)->SetId_FatTree(i,j,2);// labling the switch
        	   for(int k=0; k<Port_num/2; k++)// cycling within every level 2 switch
        	   {
                  NetDeviceContainer link;
                  NodeContainer node;
                  Ptr<Node> serverNode = node_server.Get(i*(Port_num*Port_num/4)+ j*(Port_num/2) + k);
                  node.Add(serverNode);
                  node_server.Get(i*(Port_num*Port_num/4)+ j*(Port_num/2) + k)->SetId_FatTree(i,j,k); // labling the host server
                  node.Add(node_l2switch.Get(i*(Port_num/2)+j));
                  link = p2p.Install(node);
                  // assign the ip address of the two device.
                  ip1 = (10<<24)+(i<<16)+(3<<14)+(j<<8)+(k<<4);
                  //std::cout<< "assign ip is "<< Ipv4Address(ip1)<<std::endl;
                  address.SetBase (Ipv4Address(ip1), "255.255.255.248");
                  server_ip = address.Assign(link);
                  IpServerMap[server_ip.GetAddress(0)] = serverNode;
        	   }

           }
      } // End of creating servers at level-0 and connecting it to first layer switch
  // Now connect first layer switch with second layer
      for (int i=0; i<Port_num; i++)//cycling among pods
      {
    	  for (int j=0; j<Port_num/2; j++)//cycling among switches
    	  {
    		  for(int k=0; k<Port_num/2;k++)//cycling among ports on one switch
    		  {
    			  NetDeviceContainer link;
    			  NodeContainer node;
    			  node.Add(node_l2switch.Get(i*(Port_num/2)+j));
    			  node.Add(node_l1switch.Get(i*(Port_num/2)+k));
    			  node_l1switch.Get(i*(Port_num/2)+k)->SetId_FatTree(i,k,1);
    			  link = p2p.Install(node);
    			  ip1 = (10<<24)+(i<<16)+(2<<14)+(j<<8)+(k<<4);
    			  address.SetBase (Ipv4Address(ip1), "255.255.255.248");
    			  p2p_ip = address.Assign(link);
    		  }
    	  }
      }//end of the connections construct between l2 switches and l1 switches
	// Now connect the core switches and the l2 switches

    	  int Pod0[16] = {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16};
    	  int Pod1[16] = {1,5,9,13,2,6,10,14,3,7,11,15,4,8,12,16};
    	  int Pod2[16] = {1,6,12,15,2,5,11,16,3,8,10,13,4,7,9,14};
    	  int Pod3[16] = {1,7,10,16,2,8,9,15,3,5,12,14,4,6,11,13};
    	  int Pod4[16] = {1,8,11,14,2,7,12,13,3,6,9,16,4,5,10,15};
    	  int Pod5[16] = {1,5,10,13,2,6,9,14,3,7,12,15,4,8,11,16};
    	  int Pod6[16] = {1,5,11,13,2,6,12,14,3,7,9,15,4,8,10,16};
    	  int Pod7[16] = {1,5,12,13,2,6,11,14,3,7,10,15,4,8,9,16};
    	  int **Pod = new int *[Port_num];
    	  for (int i=0; i<Port_num; i++)
    	  {
    		  Pod[i] = new int[Port_num*Port_num/4];
    	  }
    	  Pod[0] = Pod0;
    	  Pod[1] = Pod1;
    	  Pod[2] = Pod2;
    	  Pod[3] = Pod3;
    	  Pod[4] = Pod4;
    	  Pod[5] = Pod5;
    	  Pod[6] = Pod6;
    	  Pod[7] = Pod7;
          int temp = 0;
    	for (int i =0; i <Port_num; i++)//cycling among the pods
    	{
    		for(int j =0; j< Port_num/2; j++)//cycling among the pod switches
    		{
    			for(int k = 0; k < Port_num/2; k++)//cycling among the ports on each switch
    			{
    				temp =Pod[i][j*(Port_num/2)+k];
    				NetDeviceContainer link;
    				NodeContainer node;
    				node.Add(node_l1switch.Get(i*(Port_num/2)+j));
    				node.Add(node_l0switch.Get(temp-1));
    				link = p2p.Install(node);
    				ip1 = (10<<24)+(i<<16)+(1<<14)+(j<<8)+(k<<4);
    				//ip2 = construct_ip(temp,k,j,0,0);
    				// assign the ip address of the two device.
    				//p2p_ip =address.Assign_FatTree(link,ip1,ip2,"255.0.0.0");
    				//ip1 = construct_ip(temp,k,0,0,0);
    				address.SetBase (Ipv4Address(ip1), "255.255.255.248");
    				p2p_ip = address.Assign(link);
    			}

    		}
    	}

    	for (int i = 0; i< Port_num/2; i++)// labling the l0switch
    	{
    		for(int j =0; j<Port_num/2; j++)
    		{
    			node_l0switch.Get(i*(Port_num/2)+j)->SetId_FatTree(i,j,0);
    		}
    	}

    	/*for(int i =0; i<(Port_num*Port_num*Port_num/4); i++ )//show the server ID
    	{
    		std::cout<< "the ID for the server is"<<node_server.Get(i)->GetId0_FatTree() <<node_server.Get(i)->GetId1_FatTree()<<node_server.Get(i)->GetIdlevel_FatTree()<<std::endl;
    	}
    	for(int i = 0; i<(Port_num*Port_num/2); i++)//show the l1switch and l2switch ID
    	{
    		std::cout<< "the ID for the l1switch is"<<node_l1switch.Get(i)->GetId0_FatTree() <<node_l1switch.Get(i)->GetId1_FatTree()<<node_l1switch.Get(i)->GetIdlevel_FatTree()<<std::endl;
    		std::cout<< "the ID for the l2switch is"<<node_l2switch.Get(i)->GetId0_FatTree() <<node_l2switch.Get(i)->GetId1_FatTree()<<node_l2switch.Get(i)->GetIdlevel_FatTree()<<std::endl;
    	}
    	for(int i = 0; i<(Port_num*Port_num/4); i++)
    	{
    		std::cout<< "the ID for the l0switch is"<<node_l0switch.Get(i)->GetId0_FatTree() <<node_l0switch.Get(i)->GetId1_FatTree()<<node_l0switch.Get(i)->GetIdlevel_FatTree()<<std::endl;
    	}
    	*/
    	//uint32_t id = node_l2switch.Get(5)->GetNDevices();
    	///std::cout << "id = " << id <<std::endl;
    	Ipv4GlobalRoutingHelper::PopulateRoutingTables();
        //printRoutingTable(node_l1switch.Get(0));
        //printRoutingTable(node_l2switch.Get(5));
        //printRoutingTable(node_server.Get(1));
        //printRoutingTable(node_l0switch.Get(14));
    	Time::SetResolution (Time::NS);
    	LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
    	LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);
    	UdpEchoServerHelper echoServer(9);
    	ApplicationContainer serverApps = echoServer.Install(node_server.Get(5));
    	serverApps.Start(Seconds(1.0));
    	serverApps.Stop(Seconds(15.0));
    	std::cout<<"server id is "<<node_server.Get(5)->GetId()<<std::endl;
    	UdpEchoClientHelper echoClient(Ipv4Address("10.0.193.17"),9);
    	echoClient.SetAttribute("MaxPackets", UintegerValue(1));
    	echoClient.SetAttribute("Interval", TimeValue(Seconds(1.0)));
    	echoClient.SetAttribute("PacketSize", UintegerValue(1024));
    	ApplicationContainer clientApps = echoClient.Install(node_server.Get(18));
    	clientApps.Start(Seconds(2.0));
    	clientApps.Stop(Seconds(10.0));

    	Simulator::Stop(Seconds(20.0));
    	  Simulator::Run ();
    	  Simulator::Destroy ();
    delete Pod;

	NS_LOG_INFO("Done");
	return 0;
}// End of program

