/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2007 INRIA
 *               2009,2010 Contributors
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
 * Author: Martín Giachino <martin.giachino@gmail.com>
 *
 *
 * This example demonstrates the use of Ns2MobilityHelper class to work with mobility.
 *
 * Detailed example description.
 *
 *  - intended usage: this should be used in order to load ns2 movement trace files into ns3.
 *  - behavior:
 *      - Ns2MobilityHelper object is created, associated to the specified trace file. 
 *      - A log file is created, using the log file name argument.
 *      - A node container is created with the number of nodes specified in the command line.  For the default ns-2 trace, specify the value 2 for this argument.
 *      - the program calls the Install() method of Ns2MobilityHelper to set mobility to nodes. At this moment, the file is read line by line, and the movement is scheduled in the simulator.
 *      - A callback is configured, so each time a node changes its course a log message is printed.
 *  - expected output: example prints out messages generated by each read line from the ns2 movement trace file.
 *                     For each line, it shows if the line is correct, or of it has errors and in this case it will
 *                     be ignored.
 *
 * Usage of ns2-mobility-trace:
 *
 *  ./waf --run "ns2-mobility-trace \
 *        --traceFile=src/mobility/examples/default.ns_movements
 *        --nodeNum=2  --duration=100.0 --logFile=ns2-mobility-trace.log"
 *
 *  NOTE: ns2-traces-file could be an absolute or relative path. You could use the file default.ns_movements
 *        included in the same directory as the example file.
 *  NOTE 2: Number of nodes present in the trace file must match with the command line argument.
 *          Note that you must know it before to be able to load it.
 *  NOTE 3: Duration must be a positive number and should match the trace file. Note that you must know it before to be able to load it.
 */

#define TX_POWER_START 21.0206
#define TX_POWER_END 21.0206
#define TX_POWER_LEVELS 1
#define TX_GAIN 0
#define RX_GAIN 0
#define ENERGY_DETECTION_THRESHOLD -96.0
#define CCA_MODEL_THRESHOLD -99
#define PROPAGATION_LOSS_MODEL "ns3::NakagamiPropagationLossModel"
#define PROPAGATION_DELAY_MODEL "ns3::ConstantSpeedPropagationDelayModel"
#define WIFI_PHY_STANDARD WIFI_PHY_STANDARD_80211_10MHZ
#define PHY_MODE "OfdmRate6MbpsBW10MHz"
#define REMOTE_STATION_MANAGER "ns3::ConstantRateWifiManager"
#define MAC_TYPE "ns3::BeaconingAdhocWifiMac"
#define BASE_NETWORK_ADDRESS "10.0.0.0"
#define NETWORK_MASK "255.0.0.0"

#define SIMULATION_STEP 1

#include <iostream>
#include <fstream>
#include <sstream>
#include <map>
#include <string>
#include <stdlib.h>

#include "ns3/core-module.h"
#include "ns3/nqos-wifi-mac-helper.h"
#include "ns3/application.h"
#include "ns3/ipv4-address-helper.h"
#include "ns3/internet-stack-helper.h"
#include "ns3/wifi-helper.h"
#include "ns3/wifi-phy.h"
#include "ns3/wifi-net-device.h"
#include "ns3/wifi-helper.h"
#include "ns3/wifi-channel.h"
#include "ns3/yans-wifi-phy.h"
#include "ns3/yans-wifi-channel.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/mobility-module.h"
#include "ns3/wifi-net-device.h"
#include "ns3/node-list.h"
#include "ns3/node.h"
#include "ns3/simulator.h"
#include "ns3/ns2-mobility-helper.h"
#include "ns3/beaconing-adhoc-wifi-mac.h"
#include "ns3/netanim-module.h"
#include "ns3/names.h"

using namespace ns3;
using namespace std;

YansWifiPhyHelper phyHelper;
YansWifiChannelHelper channelHelper;
WifiHelper wifi;
Ipv4AddressHelper address;
Ns2MobilityHelper ns2MobilityHelper;
NqosWifiMacHelper mac;
vector<string> runningVehicles;
double currentTime;
double duration;
ofstream outfile;
ofstream nodesfile;
ofstream edgesfile;

void PrintNodeDetails(NodeContainer node_container, int nodeId) {
	Ptr<Node> node = node_container.Get(nodeId);
	Ptr<ConstantVelocityMobilityModel> model = node->GetObject<ConstantVelocityMobilityModel> ();
	if (model != 0) {
		Vector position;
		position.x = model->GetPosition ().x;
		position.y = model->GetPosition ().y;
		position.z = model->GetPosition ().z;
//		Ptr<NetDevice> dev = node->GetDevice(0);
//		Ptr<WifiNetDevice> wifidev = DynamicCast<WifiNetDevice>(dev);
//		Ptr<WifiMac> mac = wifidev->GetMac();
		Ptr<BeaconingAdhocWifiMac> beaconingmac = node->GetObject<BeaconingAdhocWifiMac>();
//		Ptr<BeaconingAdhocWifiMac> beaconingmac = DynamicCast<BeaconingAdhocWifiMac>(mac);

		if (beaconingmac->GetBeaconGeneration()) {
			outfile << Simulator::Now().GetSeconds() << " " << currentTime << " " << node->GetId() << " " << beaconingmac->GetAddress() << " " << position.x << " " << position.y << " " << node->GetDevice(0)->GetAddress() << " ";
			outfile << beaconingmac->PrintNeighbors();
		}
	}
}

void PrintNode(string nodeId) {
	Ptr<Node> node = Names::Find<Node>(nodeId);
	Ptr<ConstantVelocityMobilityModel> model = node->GetObject<ConstantVelocityMobilityModel> ();
	if (model != 0) {
		Vector position;
		position.x = model->GetPosition ().x;
		position.y = model->GetPosition ().y;
		position.z = model->GetPosition ().z;
		Ptr<NetDevice> dev = node->GetDevice(0);
		Ptr<WifiNetDevice> wifidev = DynamicCast<WifiNetDevice>(dev);
		Ptr<WifiMac> mac = wifidev->GetMac();
		Ptr<BeaconingAdhocWifiMac> beaconingmac = DynamicCast<BeaconingAdhocWifiMac>(mac);
		if (beaconingmac->GetBeaconGeneration()) {
			Ptr<NetDevice> d = node->GetDevice(0);
			outfile << Simulator::Now().GetSeconds() << " " << currentTime << " " << node->GetId() << " " << position.x << " " << position.y << " " << beaconingmac->GetAddress() << " ";
			outfile << beaconingmac->PrintNeighbors();
		}
	}
}

void PrintVehicles(vector<string> vehicles) {
	for (std::vector<string>::iterator i = vehicles.begin(); i != vehicles.end(); ++i) {
		PrintNode(*i);
	}
}

void PrintNodeContainer(NodeContainer node_container) {
	int n = node_container.GetN();
	for (int i = 0; i < n; ++i) {
		PrintNodeDetails(node_container, i);
	}
}

void InitializeNetwork() {
	// phy layer
	phyHelper =  YansWifiPhyHelper::Default ();
	phyHelper.Set("TxPowerStart",DoubleValue(TX_POWER_START));
	phyHelper.Set("TxPowerEnd",DoubleValue(TX_POWER_END));
	phyHelper.Set("TxPowerLevels",UintegerValue(TX_POWER_LEVELS));
	phyHelper.Set("TxGain",DoubleValue(TX_GAIN));
	phyHelper.Set("RxGain",DoubleValue(RX_GAIN));
	phyHelper.Set("EnergyDetectionThreshold", DoubleValue((double)ENERGY_DETECTION_THRESHOLD));
	phyHelper.Set("CcaMode1Threshold", DoubleValue(CCA_MODEL_THRESHOLD));
	// channel
	channelHelper = YansWifiChannelHelper::Default ();
	channelHelper.AddPropagationLoss(PROPAGATION_LOSS_MODEL);
	channelHelper.SetPropagationDelay(PROPAGATION_DELAY_MODEL);
	phyHelper.SetChannel(channelHelper.Create ());
	wifi = WifiHelper::Default();
	wifi.SetStandard(WIFI_PHY_STANDARD);
	wifi.SetRemoteStationManager (REMOTE_STATION_MANAGER, "DataMode", StringValue(PHY_MODE), "ControlMode", StringValue(PHY_MODE));
	address.SetBase(BASE_NETWORK_ADDRESS, NETWORK_MASK); // initial address it defaults to "0.0.0.1"
	mac = NqosWifiMacHelper::Default();
	mac.SetType(MAC_TYPE);
//	// to re-build BeaconingAdhocWifiMac
//    BeaconingAdhocWifiMac * bm = new BeaconingAdhocWifiMac();
//    bm = NULL;
	cout << "Network initialized. " << endl;
}

/**
 * Remove vehicles from collection
 */
void RemoveVehicles(std::vector<string> toRemove, std::vector<string> & vehicles) {
	for (std::vector<std::string>::iterator i = toRemove.begin(); i != toRemove.end(); ++i) {
		vector<string>::iterator it;
		it = std::find(vehicles.begin(), vehicles.end(), (*i));
		if (it != vehicles.end()) {
//			cout << "removing from running vehicles: " << *it << endl;
			vehicles.erase(it);
		}
	}
}

/**
 * Switch off interfaces on nodes who didn't move last timestep and return them as a list
 */
vector<string> SwitchOffArrivedNodes(std::vector<int> movingVehicles, std::vector<string> & runningVehicles) {
	vector<string> toSwitchOff;

	for (std::vector<string>::iterator i = runningVehicles.begin(); i != runningVehicles.end(); ++i) {
		Ptr<Node> node = Names::Find<Node>((*i));
		if (node != 0) {
			int id = node->GetId();
			bool doSwitchOff = true;
			for (vector<int>::iterator it = movingVehicles.begin(); it != movingVehicles.end(); ++it) {
				if (*it == id) {
					doSwitchOff = false;
					break;
				}
			}
			if (doSwitchOff) {
				toSwitchOff.push_back(*i);
				Ptr<Ipv4> ipv4 = node->GetObject<Ipv4>();
				int32_t ifIndex = ipv4->GetInterfaceForDevice(node->GetDevice(0));
				if (!ipv4->IsUp(ifIndex)) {
					break;
				}
//					cout << "Switching off node " << currentTime << " " << node->GetNDevices() << " " << id << " " <<  node->GetId() << endl;
				Ipv4InterfaceAddress address = ipv4->GetAddress(ifIndex, 0);
				ipv4->RemoveAddress(ifIndex, 0);
				ipv4->SetDown(ifIndex);
				for (uint32_t j = 0; j < node->GetNApplications(); ++j) {
					ns3::Time stopTime = Simulator::Now();
					node->GetApplication(j)->SetStopTime(stopTime);
				}
				Ptr<NetDevice> d = node->GetDevice(0);
				Ptr<WifiNetDevice> wd = DynamicCast<WifiNetDevice>(d);
				Ptr<WifiPhy> wp = wd->GetPhy();
				Ptr<WifiMac> wm = wd->GetMac();
				Ptr<BeaconingAdhocWifiMac> bwm = DynamicCast<BeaconingAdhocWifiMac>(wm);
				bwm->SetBeaconGeneration(false);
			}
		}
	}
	return toSwitchOff;
}

/**
 * Switch on beaconing on nodes who have active interface and do not sending beacons yet
 */
void SwitchOnBeaconing(std::vector<string> runningVehicles) {
	// switch on those from old_container
	for (std::vector<string>::iterator i = runningVehicles.begin(); i != runningVehicles.end(); ++i) {
		Ptr<Node> node = Names::Find<Node>((*i));
		if (node != 0) {
			if (node == 0) {
				return;
			}
			Ptr<Ipv4> ipv4 = node->GetObject<Ipv4>();
			bool doSwitchOn = true;
			int32_t ifIndex = ipv4->GetInterfaceForDevice(node->GetDevice(0));
			if (!ipv4->IsUp(ifIndex)) {
				doSwitchOn = false;
				break;
			}
			if (doSwitchOn) {
				Ptr<MobilityModel> mobility = node->GetObject<ConstantVelocityMobilityModel> ();
				Ptr<NetDevice> d = node->GetDevice(0);
				Ptr<WifiNetDevice> wd = DynamicCast<WifiNetDevice>(d);
				Ptr<WifiMac> wm = wd->GetMac();
				Ptr<BeaconingAdhocWifiMac> bwm = DynamicCast<BeaconingAdhocWifiMac>(wm);
				if (bwm->GetBeaconGeneration() == false) {
//					int id = node->GetId();
//					cout << "Switching on beaconing on node " << currentTime << " " << id << " " << node->GetId() << " if bwm->GetBeaconGeneration() " << bwm->GetBeaconGeneration() << endl;
					bwm->SetBeaconGeneration(true);
				}
			}
		}
	}
}

void CreateNodes(double time) {
  	int nodeNum	= ns2MobilityHelper.GetNewNodes(time);
  	NodeContainer node_container;
  	node_container.Create(nodeNum);
//  	// RandomDirection2dMobilityModel
//    MobilityHelper mobility;
//    mobility.SetMobilityModel("ns3::RandomDirection2dMobilityModel", "Bounds", RectangleValue (Rectangle (0.0, 0.0, 100.0, 100.0)));
//    mobility.Install(node_container);
  	ns2MobilityHelper.Install();
	NetDeviceContainer devices = wifi.Install(phyHelper, mac, node_container);
	InternetStackHelper stack;
	stack.Install(node_container);
	Ipv4InterfaceContainer wifiInterfaces;
	wifiInterfaces = address.Assign(devices);
	int n = node_container.GetN();
	// add nodes who were installed in node_container to global namespace
	for (int i = 0; i < n; ++i) {
		Ptr<Node> node = node_container.Get(i);
		stringstream ss;
		ss << node->GetId();
		string strId = ss.str();
		Names::Add("Nodes", strId, node);
	}

	std::vector<int> movingVehicles = ns2MobilityHelper.GetMovedVehicles(time);
	// add to running vehicles those who are moving for the first time
	if (runningVehicles.size() == 0 && movingVehicles.size() > 0) {
		for (vector<int>::iterator it = movingVehicles.begin(); it != movingVehicles.end(); ++it) {
			stringstream ss;
			ss << *it;
			string strId = ss.str();
			runningVehicles.push_back(strId);
		}
	}
	// modify running nodes by adding those who are running at the current step
	else if (runningVehicles.size() > 0 && movingVehicles.size() > 0) {
		vector<string>::iterator isRunning;
		for (vector<int>::iterator it = movingVehicles.begin(); it != movingVehicles.end(); ++it) {
			stringstream ss;
			ss << *it;
			string strId = ss.str();
			isRunning = std::find(runningVehicles.begin(), runningVehicles.end(), strId);
			if (isRunning == runningVehicles.end()) {
				runningVehicles.push_back(strId);
			}
		}
	}

	SwitchOnBeaconing(runningVehicles);

	vector<string> toRemove =SwitchOffArrivedNodes(movingVehicles, runningVehicles);
	RemoveVehicles(toRemove, runningVehicles);

	cout << "CreateNodes for time " << time << ", movedVehicles " << movingVehicles.size() << ", runningVehicles: " << runningVehicles.size() << ", put to node_container: " << node_container.GetN() << endl;
	PrintVehicles(runningVehicles);

}

void SimulationStep() {
	if (currentTime <= currentTime + duration) {
		currentTime = currentTime + SIMULATION_STEP;
		CreateNodes(currentTime);
		Simulator::Schedule(Seconds(SIMULATION_STEP), &SimulationStep);
	}
	else {
		cout << "Simulation ends at " << currentTime << endl;
		outfile.close();
		nodesfile.close();
		edgesfile.close();
	}
}

void Start() {
	currentTime = ns2MobilityHelper.GetFirstTime();
	std::cout << "Simulation starts at " << currentTime << ", ends at " << currentTime + duration << endl;
	CreateNodes(currentTime);
	Simulator::Schedule(Seconds(SIMULATION_STEP), &SimulationStep);
}


// Example to use ns2 traces file in ns3
int main (int argc, char *argv[])
{
  // Enable logging from the ns2 helper
//  LogComponentEnable ("Ns2MobilityHelper",LOG_LEVEL_DEBUG);

  map<string, string> params;

  // Parse command line attribute
  CommandLine cmd;
  cmd.AddValue ("traceFile", "Ns2 movement trace file", params["traceFile"]);
  cmd.AddValue ("nodeNum", "Number of nodes", params["nodeNum"]);
  cmd.AddValue ("duration", "Duration of Simulation", params["duration"]);
  cmd.AddValue ("logFile", "Log file", params["logFile"]);
  cmd.Parse (argc,argv);

  params["traceFile"] = "mobility.tcl";
  params["dir"] = "./";
  params["logFile"] = "main-ns2-mob.log";
  params["animFile"] = "animation.xml";
  params["gexfFile"] = "graph.gexf";
  params["outFile"] = "out.txt";
  params["nodesFile"] = "nodes.txt";
  params["edgesFile"] = "edges.txt";
//  params["nodeNum"] = "993";
  params["duration"] = "11";
  params["dir"] = "/Users/agatagrzybek/PhD/workshop/graphs/LuxembourgTraces/FCD/";
  params["traceFile"] = params["dir"] + params["traceFile"];
  params["logFile"] = params["dir"] + params["logFile"];
  params["animFile"] = params["dir"] + params["animFile"];
  params["outFile"] = params["dir"] + params["outFile"];
  params["nodesFile"] = params["dir"] + params["nodesFile"];
  params["edgesFile"] = params["dir"] + params["edgesFile"];


  // Check command line arguments
  //int nodeNum = atoi(params["nodeNum"].c_str());
  duration = atof(params["duration"].c_str());

  if (params["traceFile"].empty () || duration <= 0 || params["logFile"].empty ())
    {
      std::cerr << "Usage of " << argv[0] << " :\n\n"
      "./waf --run \"ns2-mobility-trace"
      " --traceFile=src/mobility/examples/default.ns_movements"
      " --nodeNum=2 --duration=100.0 --logFile=ns2-mob.log\" \n\n"
      "NOTE: ns2-traces-file could be an absolute or relative path. You could use the file default.ns_movements\n"
      "      included in the same directory of this example file.\n\n"
      "NOTE 2: Number of nodes present in the trace file must match with the command line argument and must\n"
      "        be a positive number. Note that you must know it before to be able to load it.\n\n"
      "NOTE 3: Duration must be a positive number. Note that you must know it before to be able to load it.\n\n";

      return 0;
    }

	outfile.open (params["outFile"].c_str());

	InitializeNetwork();

	ns2MobilityHelper = Ns2MobilityHelper (params["traceFile"]);

//	AnimationInterface anim(params["animFile"]);
	Simulator::Stop (Seconds (duration));
	Simulator::Schedule(Simulator::Now(), &Start);
	Simulator::Run();
	Simulator::Destroy();


	return 0;

}



