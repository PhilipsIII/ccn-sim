/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2012 University of Washington
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
 */

#include <string>
#include <iostream>

//ns3 modules
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include "ns3/internet-module.h"
#include <ns3-dev/ns3/point-to-point-module.h>
//#include <ns3-dev/ns3/simulator.h>
// #include <ns3/lte-module.h>

#include "ns3/ndnSIM-module.h"

#include <boost/lexical_cast.hpp>

using namespace ns3;
using namespace boost;

int main (int argc, char *argv[])
{
    // disable fragmentation
    Config::SetDefault ("ns3::WifiRemoteStationManager::FragmentationThreshold", StringValue ("2200"));
    Config::SetDefault ("ns3::WifiRemoteStationManager::RtsCtsThreshold", StringValue ("2200"));
    Config::SetDefault ("ns3::WifiRemoteStationManager::NonUnicastMode", StringValue ("OfdmRate24Mbps"));	
    
    CommandLine cmd;
	cmd.Parse (argc, argv);

    double sec = 0.0;
	double waitint = 2.0;
	double travelTime = 5.0;

	// One train & three stations
	NodeContainer train;
	train.Create(1);
	NodeContainer sta;
	sta.Create (5);
    NodeContainer nodes;
    nodes.Add(train);
    nodes.Add(sta);

    // Two routers int the area
    NodeContainer routers;
    routers.Create(2);
    // One server in the area
    NodeContainer server;
    server.Create(1);
    
    uint32_t mtId = sta.Get (0)->GetId();

    WifiHelper wifi = WifiHelper::Default ();
    // wifi.SetRemoteStationManager ("ns3::AarfWifiManager");
    wifi.SetStandard (WIFI_PHY_STANDARD_80211a);
    wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
                                "DataMode", StringValue ("OfdmRate24Mbps"));

    YansWifiChannelHelper wifiChannel;// = YansWifiChannelHelper::Default ();
    wifiChannel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
    wifiChannel.AddPropagationLoss ("ns3::ThreeLogDistancePropagationLossModel");
    wifiChannel.AddPropagationLoss ("ns3::NakagamiPropagationLossModel");

    //YansWifiPhy wifiPhy = YansWifiPhy::Default();
    YansWifiPhyHelper wifiPhyHelper = YansWifiPhyHelper::Default ();
    wifiPhyHelper.SetChannel (wifiChannel.Create ());
    wifiPhyHelper.Set("TxPowerStart", DoubleValue(5));
    wifiPhyHelper.Set("TxPowerEnd", DoubleValue(5));


    NqosWifiMacHelper wifiMacHelper = NqosWifiMacHelper::Default ();
    //wifiMacHelper.SetType("ns3::AdhocWifiMac");

    std::vector<Ssid> ssidV;

    	//NS_LOG_INFO ("Creating ssids for wireless cards");

    	for (int i = 0; i < 5; i++)
    	{
    		ssidV.push_back (Ssid ("ap-" + boost::lexical_cast<std::string>(i)));
    	}

    	//NS_LOG_INFO ("Assigning mobile terminal wireless cards");
    	// Create a Wifi station type MAC
    	wifiMacHelper.SetType("ns3::StaWifiMac",
    	//		"Ssid", SsidValue(ssidV[3]),
    			"ActiveProbing", BooleanValue (true));

    	NetDeviceContainer wifiMTNetDevices = wifi.Install (wifiPhyHelper, wifiMacHelper, train);

    	//NS_LOG_INFO ("Assigning AP wireless cards");
    	std::vector<NetDeviceContainer> wifiAPNetDevices;
    	for (int i = 0; i < 5; i++)
    	{
    		wifiMacHelper.SetType ("ns3::ApWifiMac",
    	                       "Ssid", SsidValue (ssidV[i]),
    	                       "BeaconGeneration", BooleanValue (true),
    	                       "BeaconInterval", TimeValue (Seconds (0.1)));

    		wifiAPNetDevices.push_back (wifi.Install (wifiPhyHelper, wifiMacHelper, sta.Get(i)));

    	}
    PointToPointHelper p2p_2gb200ms, p2p_1gb5ms;
    p2p_2gb200ms.SetDeviceAttribute ("DataRate", StringValue ("2Gbps"));
    p2p_2gb200ms.SetChannelAttribute ("Delay", StringValue ("200ms"));
    p2p_1gb5ms.SetDeviceAttribute ("DataRate", StringValue ("1Gbps"));
    p2p_1gb5ms.SetChannelAttribute ("Delay", StringValue ("5ms"));

    p2p_2gb200ms.Install (server.Get(0), routers.Get(0));
    p2p_2gb200ms.Install (server.Get(0), routers.Get(1));
    
    for(int i = 0; i<3; i++)
    {
        p2p_1gb5ms.Install (routers.Get(0), sta.Get(i));
    }
    for(int i = 3; i<=4; i++)
    {
        p2p_1gb5ms.Install (routers.Get(1), sta.Get(i));
    }
    
	ndn::StackHelper ndnHelperRouters;
	ndnHelperRouters.SetForwardingStrategy ("ns3::ndn::fw::BestRoute");
	ndnHelperRouters.SetContentStore ("ns3::ndn::cs::Freshness::Lru", "MaxSize", "1000");
	ndnHelperRouters.SetDefaultRoutes (true);
	ndnHelperRouters.Install (routers);
	ndnHelperRouters.Install (server);
	ndnHelperRouters.Install (nodes);

/*	ndn::StackHelper ndnHelperUsers;
	ndnHelperUsers.SetForwardingStrategy ("ns3::ndn::fw::Flooding");
	ndnHelperUsers.SetContentStore ("ns3::ndn::cs::Nocache");
	ndnHelperUsers.SetDefaultRoutes (true);
	ndnHelperUsers.Install (train);
*/
/*    ndn::GlobalRoutingHelper ndnGlobalRoutingHelper;
	ndnGlobalRoutingHelper.InstallAll ();
    ndnGlobalRoutingHelper.AddOrigins ("/test/prefix", sta.Get(0));
/*


	// How to get/set sta's location and presentate them into the form of Vector(a,b,c)?
	// Ptr<ConstantPositionMobilityModel> Loc =  n->GetObject<ConstantPositionMobilityModel> ();
	// if (Loc == 0)
	//     {
	//    Loc = CreateObject<ConstantPositionMobilityModel> ();
	//    n->AggregateObject (Loc);
	//     }
	// Vector vec (10, 20, 0);
	// Loc->SetPosition (vec);
/*    Ptr<LteHelper> lteHelper = CreateObject<LteHelper> ();
    NetDeviceContainer enbDevs;
    enbDevs = lteHelper->InstallEnbDevice (train);

    NetDeviceContainer ueDevs;
    ueDevs = lteHelper->InstallUeDevice (sta);

    lteHelper->Attach (ueDevs, enbDevs.Get (0));

    enum EpsBearer::Qci q = EpsBearer::GBR_CONV_VOICE;
    EpsBearer bearer (q);
    lteHelper->ActivateDataRadioBearer (ueDevs, bearer);
*/
    
    ndn::AppHelper consumerHelper ("ns3::ndn::ConsumerCbr");
    consumerHelper.SetPrefix ("/waseda/Sato");
    consumerHelper.SetAttribute ("Frequency", DoubleValue (10.0));
	consumerHelper.SetAttribute("StartTime", TimeValue (Seconds(travelTime /2)));
	consumerHelper.SetAttribute("StopTime", TimeValue (Seconds(sec-1)));
    consumerHelper.Install (train);

    ndn::AppHelper producerHelper ("ns3::ndn::Producer");
    producerHelper.SetPrefix ("/waseda/Sato");
    producerHelper.SetAttribute ("PayloadSize", StringValue("1024"));
    producerHelper.SetAttribute("StopTime", TimeValue (Seconds(sec)));
    producerHelper.Install (server.Get(0));

    MobilityHelper mobility;
	MobilityHelper mobilityStations;
    MobilityHelper mobilityRouters;
    MobilityHelper mobilityServer;

	mobilityStations.SetPositionAllocator ("ns3::GridPositionAllocator",
			"MinX", DoubleValue (0.0),
			"MinY", DoubleValue (0.0),
			"DeltaX", DoubleValue (200.0),
			"DeltaY", DoubleValue (0.0),
			"GridWidth", UintegerValue (sta.GetN()),
			"LayoutType", StringValue ("RowFirst"));
	mobilityStations.SetMobilityModel ("ns3::ConstantPositionMobilityModel");

	mobilityStations.Install (sta);


	Ptr<ListPositionAllocator> routerinitialAlloc = 
            CreateObject<ListPositionAllocator>();

    routerinitialAlloc->Add (Vector (60.0, -100.0, 0.0));
	routerinitialAlloc->Add (Vector (140.0,-100.0, 0.0));

	mobilityRouters.SetPositionAllocator(routerinitialAlloc);
    mobilityRouters.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
    mobilityRouters.Install (routers);


	Ptr<ListPositionAllocator> serverinitialAlloc = 
            CreateObject<ListPositionAllocator>();

    serverinitialAlloc->Add (Vector (80.0, -180.0, 0.0));

	mobilityServer.SetPositionAllocator(serverinitialAlloc);
    mobilityServer.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
    mobilityServer.Install (server);


    MobilityHelper mobilityTrain;

	Vector diff = Vector(0.0, 10.0, 0.0);

	Vector pos;

	Ptr<ListPositionAllocator> traininitialAlloc = CreateObject<ListPositionAllocator>();

	Ptr<MobilityModel> mob = sta.Get(0)->GetObject<MobilityModel>();

	pos = mob->GetPosition();

	traininitialAlloc->Add (pos + diff);

	// Put everybody into a line
	//initialAlloc->Add (Vector (45.0, 37, 0.));
	//initialAlloc->Add (Vector (53.0, 8, 0.));
	mobilityTrain.SetPositionAllocator(traininitialAlloc);
	mobilityTrain.SetMobilityModel("ns3::WaypointMobilityModel");
	mobilityTrain.Install(train.Get(0));

	// Set mobility random number streams to fixed values
	//mobility.AssignStreams (sta, 0);

	Ptr<WaypointMobilityModel> staWaypointMobility = DynamicCast<WaypointMobilityModel>(train.Get(0)->GetObject<MobilityModel>());


	for (int j = 0; j < sta.GetN(); j++)
	{
		

        mob = sta.Get(j)->GetObject<MobilityModel>();

		Vector wayP = mob->GetPosition() + diff;

		staWaypointMobility->AddWaypoint(Waypoint(Seconds(sec), wayP));
		staWaypointMobility->AddWaypoint(Waypoint(Seconds(sec + waitint), wayP));
		sec += waitint + travelTime;
	}
	//NS_LOG_INFO ("Scheduling events - Getting objects");

	char configbuf[250];
	// This causes the device in mtId to change the SSID, forcing AP change
	sprintf(configbuf, "/NodeList/%d/DeviceList/0/$ns3::WifiNetDevice/Mac/Ssid", mtId);

	// Schedule AP Changes
	double apsec = 0.0;

	//NS_LOG_INFO ("Scheduling events - Installing events");
	for (int j = 0; j < 5; j++)
	{
		//sprintf(buffer, "Setting mobile node to AP %i at %2f seconds", j, apsec);
		//NS_LOG_INFO (buffer);

		Simulator::Schedule (Seconds(apsec), Config::Set, configbuf, SsidValue (ssidV[j]));

		apsec += waitint + travelTime;
	}


	Simulator::Stop (Seconds (40.0));
	Simulator::Run ();
	Simulator::Destroy ();
    return 0;
}
