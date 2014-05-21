
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
//#include "ns3/ipv4-address-generator.h";

using namespace ns3;
using namespace std;
const int Port_num = 8; // set the switch number

uint32_t construct_ip(int pod, int pswitch, int num, int level, int port_level)
{
	uint32_t ip;
	ip = 10;
	ip = ip<<24;

	if (level==0)
	{
		ip+= pod<<16;
		ip+= ((pswitch<<4)+0)<<8;
		ip+= 2;
		return ip;
	}
	else
	{
		ip+= ((pswitch<<4)+num)<<16;
		if (level==1)
		{
			if(port_level == 0)
			{
				ip += ((num<<4)+1)<<8;
				ip += 4;
				return ip;
			}
			else
			{
				ip += (((num+Port_num/2)<<4)+1)<<8;
				ip += 4;
				return ip;
			}
		}
		if (level==2)
		{
			if(port_level == 0)
			{
				ip += ((num<<4)+2)<<8;
				ip += 6;
				return ip;
			}
			else
			{
				ip += (((num+Port_num/2)<<4)+2)<<8;
				ip += 6;
				return ip;
			}
		}
		if (level==3)
		{
			ip += ((num<<4)+3)<<8;
			ip += 8;
			return ip;
		}
	}
	return 0;
}
NS_LOG_COMPONENT_DEFINE("first");

int main(int argc, char *argv[])
{
      Config::SetDefault ("ns3::OnOffApplication::PacketSize", UintegerValue(210));
      Config::SetDefault ("ns3::OnOffApplication::DataRate", StringValue("448kb/s"));

       int temp;
      //int k=16,i,j,temp,l1_index,p;
      CommandLine cmd;
      bool enableMonitor = true;

      cmd.AddValue("EnableMonitor", "Enable Flow Monitor", enableMonitor);

      cmd.Parse(argc, argv);
      //Ipv4AddressGenerator address;

      //char *str1, *str2;
      //str1 = (char *) malloc(sizeof(char) * 30);
      //str2 = (char *) malloc(sizeof(char) * 30);
      //str3 = (char *) malloc(sizeof(char) * 30);

      //For BulkSend
      //uint32_t maxBytes = 52428800;/////////////

      NS_LOG_INFO("Create Nodes");
      // servers and level 2 switches
      NodeContainer* c_server = new NodeContainer[Port_num*Port_num*Port_num/4];
      NetDeviceContainer *p2p_server = new NetDeviceContainer[Port_num*Port_num*Port_num/4];
      // Aggregate level-lower and the aggregate level-upper
      NodeContainer* l2 = new NodeContainer[Port_num];
      NodeContainer* Nodelink_l1_l2 = new NodeContainer[Port_num*Port_num*Port_num/4];
      NetDeviceContainer* Devicelink_l1_l2 = new NetDeviceContainer[Port_num*Port_num*Port_num/4];
      // Aggregate level-upper and core switches
      NodeContainer* l1 = new NodeContainer[Port_num];
      NodeContainer* Nodelink_l0_l1 = new NodeContainer[Port_num*Port_num*Port_num/4];
      NetDeviceContainer* Devicelink_l0_l1 = new NetDeviceContainer[Port_num*Port_num*Port_num/4];
      //core_switches
      NodeContainer l0;
      // Point to Point Helper to all the links in fat-tree include the server subnets
      PointToPointHelper p2p;
      p2p.SetDeviceAttribute("DataRate", StringValue("5Mbps"));
      p2p.SetChannelAttribute("Delay", StringValue("10ms"));
      // NetDeviceContainer for p2p connections
      //NetDeviceContainer *p2p_d;
      Ipv4InterfaceContainer p2p_ip;
      InternetStackHelper internet;
      Ipv4AddressHelper address1;
      // First create four set of Level-0 subnets
      temp = 0;
      uint32_t ip1, ip2;
      //ip1 = (char *) malloc(8 * sizeof(char));
      //ip2 = (char *) malloc(8 * sizeof(char));
      //ip3 = (char *) malloc(8 * sizeof(char));

      //strcpy(ip1,"00");
      //strcpy(ip2,"00");
      for(int i=0;i < Port_num; i++)// cycling with pods
      {
           l2[i].Create(Port_num/2);
           std::cout<<"here1"<<std::endl;
           internet.Install(l2[i]);
           std::cout<<"here2"<<std::endl;
           temp = i * Port_num/2;
           for(int j=0;j < Port_num/2; j++)// cycling within every pod among Level 2 switches
           {
        	   for(int k=0; k<Port_num/2; k++)// cycling within every level 2 switch
        	   {
                  c_server[temp+j*(Port_num/2)+k].Create(1);
                  internet.Install(c_server[temp+j*(Port_num/2)+k].Get(0));
                  c_server[temp+j*(Port_num/2)+k].Add(l2[i].Get(j));

                  //
                  //printf("c[%d] is added to l1[%d].Get(%d)\n",temp+j,i,j);
                  p2p_server[temp+j*(Port_num/2)+k] = p2p.Install(c_server[temp+j*(Port_num/2)+k]);
                  //internet.Install(c_server[temp+j*(Port_num/2)+k]);
                  ip1 = construct_ip(i,j,k,3,0);
                  ip2 = construct_ip(i,j,k,2,0);
                  // assign the ip address of the two device.
                  //address1.SetBase("10.0.0.2","255.255.0.0");
                  p2p_ip = address1.Assign_FatTree(p2p_server[temp+j*(Port_num/2)+k],ip1, ip2,"255.255.0.0");
        	   }
           }
      } // End of creating servers at level-0 and connecting it to first layer switch
  // Now connect first layer switch with second layer
      for (int i=0; i<Port_num; i++)//cycling among pods
      {
    	  l1[i].Create(Port_num/2);
    	  internet.Install(l1[i]);
    	  temp = i* Port_num/2;
    	  for (int j=0; j<Port_num/2; j++)//cycling among switches
    	  {
    		  for(int k=0; k<Port_num/2;k++)//cycling among ports on one switch
    		  {
    			  Nodelink_l1_l2[temp+j*(Port_num/2)+k].Add(l2[i].Get(j));
    			  Nodelink_l1_l2[temp+j*(Port_num/2)+k].Add(l1[i].Get(k));
    			  Devicelink_l1_l2[temp+j*(Port_num/2)+k] = p2p.Install(Nodelink_l1_l2[temp+j*(Port_num/2)+k]);

    			  ip1 = construct_ip(i,j,k,2,1);
    			  ip2 = construct_ip(i,j,k,1,0);
    			  // assign the ip address of the two device.
    			  //address1.SetBase("10.0.0.2","255.255.0.0");
    			  p2p_ip = address1.Assign_FatTree(Devicelink_l1_l2[temp+j*(Port_num/2)+k],ip1,ip2,"255.255.0.0");


      			  //p2p_ip = address.InitAddress(str1,"255.255.0.0");

    			  //Devicelink_l1_l2[temp+j*(Port_num/2)+k].Get(0) = address.InitAddress(str1,"255.0.0.0");
    			  //Devicelink_l1_l2[temp+j*(Port_num/2)+k].Get(1) = address.InitAddress(str2,"255.0.0.0");

    		  }
    	  }
      }//end of the connections construct between l2 switches and l1 switches
	// Now connect the core switches and the l2 switches

    	  int Pod0[16] = {1,2.3,4,5,6,7,8,9,10,11,12,13,14,15,16};
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
    	l0.Create(Port_num*Port_num/4);
    	internet.Install(l0);
    	int a;
    	for (int i =0; i <Port_num; i++)//cycling among the pods
    	{
    		temp = i*Port_num;
    		for(int j =0; j< Port_num/2; j++)//cycling among the pod switches
    		{
    			for(int k = 0; k < Port_num/2; k++)//cycling among the ports on each switch
    			{
    				a =Pod[i][j*k];
    				Nodelink_l0_l1[temp+j*(Port_num/2)+k].Add(l0.Get(a-1));
    				Nodelink_l0_l1[temp+j*(Port_num/2)+k].Add(l1[i].Get(k));
    				Devicelink_l0_l1[temp+j*(Port_num/2)+k] = p2p.Install(Nodelink_l0_l1[temp+j*(Port_num/2)+k]);
    				ip2= construct_ip(i,j,k,1,1);
    				ip1 = construct_ip(i,j,k,0,0);
    				// assign the ip address of the two device.
    				//address1.SetBase("10.0.0.2","255.0.0.0");
    				p2p_ip =address1.Assign_FatTree(Devicelink_l0_l1[temp+j*(Port_num/2)+k],ip1,ip2,"255.0.0.0");
    				//bzero(str1,30);
    				//bzero(str2,30);
    				//strcpy(str1, construct_address_l1switch_top(i, j, k));
    				//strcpy(str2, construct_address_l0switch(a,i));
    				//address1.NewAddress() = str1;
    				//p2p_ip = address1.Assign(Devicelink_l0_l1[temp+j*(Port_num/2)+k].Get(1));
    				//address1.NewAddress() = str2;
    				//p2p_ip = address1.Assign(Devicelink_l0_l1[temp+j*(Port_num/2)+k].Get(0));
    				//Devicelink_l0_l1[temp+j*(Port_num/2)+k].Get(1) = address.InitAddress(str1,"255.0.0.0");
    				//Devicelink_l0_l1[temp+j*(Port_num/2)+k].Get(0) = address.InitAddress(str2,"255.0.0.0");
    			}

    		}
    	}

    	  Simulator::Run ();
    	  Simulator::Destroy ();
    delete Pod;

	return 0;
    	NS_LOG_INFO("Done");
}// End of program

