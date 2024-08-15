#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/animation-interface.h"
#include "ns3/bridge-module.h"
#include "ns3/applications-module.h"
#include "ns3/udp-echo-helper.h"
#include "ns3/node.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/flow-monitor-helper.h"

#include <iostream>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("NetworkTopology");

int main(int argc, char *argv[]) {
    CommandLine cmd;
    cmd.Parse(argc, argv);
    bool verbose = false;

    // Enable log components
    if (verbose) {
        LogComponentEnable("NetworkTopology", LOG_LEVEL_INFO);
        LogComponentEnable("CsmaChannel", LOG_LEVEL_INFO);
        LogComponentEnable("Ipv4L3Protocol", LOG_LEVEL_INFO);
        LogComponentEnable("PointToPointHelper", LOG_LEVEL_INFO);
        LogComponentEnable("Ipv4GlobalRouting", LOG_LEVEL_INFO);
        LogComponentEnable("UdpEchoClientApplication", LOG_LEVEL_INFO);
        LogComponentEnable("UdpEchoServerApplication", LOG_LEVEL_INFO);
    }

    // Flow monitor
    Ptr<FlowMonitor> flowMonitor;
    FlowMonitorHelper flowHelper;
    flowMonitor = flowHelper.InstallAll();


    NS_LOG_INFO("Creating nodes.");
    NodeContainer csmaNodes0, csmaNodes1, csmaNodes2, csmaNodes3, routerNode, internetNode;
    csmaNodes0.Create(3);
    csmaNodes1.Create(3);
    csmaNodes2.Create(3);
    csmaNodes3.Create(1);
    routerNode.Create(4);
    internetNode.Create(1);

    NodeContainer switchNode;
    switchNode.Create(3); // This node will not have an IP stack.

    NS_LOG_INFO("Setting up CSMA channel.");
    CsmaHelper csma;
    csma.SetChannelAttribute("DataRate", StringValue("100Mbps"));
    csma.SetChannelAttribute("Delay", TimeValue(NanoSeconds(6560)));

    NetDeviceContainer csmaDevices0, csmaDevices1, csmaDevices2, csmaDevices3, switch1Devices, switch2Devices, switch0Devices, link; // Devices connected to the switch.

    NS_LOG_INFO("Connecting CSMA1 nodes to the switch.");
    for (int i = 0; i < 3; ++i) {
        link = csma.Install(NodeContainer(csmaNodes1.Get(i), switchNode.Get(1)));
        csmaDevices1.Add(link.Get(0));
        switch1Devices.Add(link.Get(1));
    }

    NS_LOG_INFO("Connecting CSMA2 nodes to the switch.");
    for (int i = 0; i < 3; ++i) {
        link = csma.Install(NodeContainer(csmaNodes2.Get(i), switchNode.Get(0)));
        csmaDevices2.Add(link.Get(0));
        switch2Devices.Add(link.Get(1));
    }

    NS_LOG_INFO("Connecting CSMA0 nodes to the switch.");
    for (int i = 0; i < 3; ++i) {
        link = csma.Install(NodeContainer(csmaNodes0.Get(i), switchNode.Get(2)));
        csmaDevices0.Add(link.Get(0));
        switch0Devices.Add(link.Get(1));
    }

    // Connect routers to switch 2 with CSMA
    NetDeviceContainer router1ToSwitch2Link = csma.Install(NodeContainer(routerNode.Get(1), switchNode.Get(0)));
    NetDeviceContainer router2ToSwitch2Link = csma.Install(NodeContainer(routerNode.Get(0), switchNode.Get(0)));

    // Connect router 2 to switch 2 with CSMA
    switch2Devices.Add(router2ToSwitch2Link.Get(1)); // Add the switch-side interface to the bridge
    csmaDevices2.Add(router2ToSwitch2Link.Get(0)); // Add the router-side interface to the bridge

    // Connect router 1 to switch 2 with CSMA
    switch2Devices.Add(router1ToSwitch2Link.Get(1)); // Add the switch-side interface to the bridge
    csmaDevices2.Add(router1ToSwitch2Link.Get(0)); // Add the router-side interface to the bridge

    // Connect router 1 to switch 1 with CSMA
    NetDeviceContainer router1ToSwitch1Link = csma.Install(NodeContainer(routerNode.Get(1), switchNode.Get(1)));
    switch1Devices.Add(router1ToSwitch1Link.Get(1)); // Add the switch-side interface to the bridge
    csmaDevices1.Add(router1ToSwitch1Link.Get(0)); // Add the router-side interface to the bridge

    // Connect router 0 to switch 1 with CSMA
    NetDeviceContainer router0ToSwitch1Link = csma.Install(NodeContainer(routerNode.Get(2), switchNode.Get(1)));
    switch1Devices.Add(router0ToSwitch1Link.Get(1)); // Add the switch-side interface to the bridge
    csmaDevices1.Add(router0ToSwitch1Link.Get(0)); // Add the router-side interface to the bridge

    // Connect router 0 to switch 0 with CSMA
    NetDeviceContainer router0ToSwitch0Link = csma.Install(NodeContainer(routerNode.Get(2), switchNode.Get(2)));
    switch0Devices.Add(router0ToSwitch0Link.Get(1)); // Add the switch-side interface to the bridge
    csmaDevices0.Add(router0ToSwitch0Link.Get(0)); // Add the router-side interface to the bridge

    // Connect router 3 to switch 0
    NetDeviceContainer router3ToSwitch0Link = csma.Install(NodeContainer(routerNode.Get(3), switchNode.Get(2)));
    switch0Devices.Add(router3ToSwitch0Link.Get(1)); // Add the switch-side interface to the bridge
    csmaDevices0.Add(router3ToSwitch0Link.Get(0)); // Add the router-side interface to the bridge

    // Connect router 3 to csma 3
    NetDeviceContainer router3ToCsma3Link = csma.Install(NodeContainer(routerNode.Get(3), csmaNodes3.Get(0)));
    csmaDevices3.Add(router3ToCsma3Link.Get(0));
    csmaDevices3.Add(router3ToCsma3Link.Get(1));

    NS_LOG_INFO("Installing bridge on the switch.");
    BridgeHelper bridge;
    // Add router interfaces to the bridge
    bridge.Install(switchNode.Get(0), switch2Devices);
    bridge.Install(switchNode.Get(1), switch1Devices);
    bridge.Install(switchNode.Get(2), switch0Devices);

    NS_LOG_INFO("Setting up point-to-point links.");
    PointToPointHelper p2p;
    p2p.SetDeviceAttribute("DataRate", StringValue("1Gbps"));
    p2p.SetChannelAttribute("Delay", TimeValue(MilliSeconds(10)));

    // Connect router 2 to the internet node
    NetDeviceContainer routerToInternetDevices = p2p.Install(routerNode.Get(0), internetNode.Get(0));

    NS_LOG_INFO("Installing internet stack.");
    InternetStackHelper stack;
    stack.Install(csmaNodes2);
    stack.Install(routerNode);
    stack.Install(internetNode);
    stack.Install(csmaNodes1);
    stack.Install(csmaNodes0);
    stack.Install(csmaNodes3);

    NS_LOG_INFO("Assigning IP addresses.");
    Ipv4AddressHelper address;
    address.SetBase("192.168.2.0", "255.255.255.0");
    Ipv4InterfaceContainer csma2Interfaces = address.Assign(csmaDevices2);

    address.SetBase("192.168.1.0", "255.255.255.0");
    Ipv4InterfaceContainer csma1Interfaces = address.Assign(csmaDevices1);

    address.SetBase("192.168.0.0", "255.255.255.0");
    Ipv4InterfaceContainer csma0Interfaces = address.Assign(csmaDevices0);

    address.SetBase("10.0.0.0", "255.0.0.0");
    Ipv4InterfaceContainer csma3Interfaces = address.Assign(csmaDevices3);

    address.SetBase("97.108.1.0", "255.255.255.0");
    Ipv4InterfaceContainer routerToInternetInterfaces = address.Assign(routerToInternetDevices);

    NS_LOG_INFO("Populating routing tables.");
    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    NS_LOG_INFO("Creating applications.");
/*
    // Send UDP packets from internet to csma 2 node 1
    UdpEchoServerHelper echoServer5(13);
    ApplicationContainer serverApps5 = echoServer5.Install(csmaNodes0.Get(1));
    serverApps5.Start(Seconds(1.0));
    serverApps5.Stop(Seconds(5.0));

    UdpEchoClientHelper echoClient5(csma0Interfaces.GetAddress(1), 13);
    echoClient5.SetAttribute("MaxPackets", UintegerValue(1));
    echoClient5.SetAttribute("Interval", TimeValue(Seconds(1.0)));
    echoClient5.SetAttribute("PacketSize", UintegerValue(1024));

    ApplicationContainer clientApps5 = echoClient5.Install(internetNode.Get(0));
    clientApps5.Start(Seconds(1.0));
    clientApps5.Stop(Seconds(5.0));
*/
    /*// Send UDP packets from internet node to csma 3 node 0
    UdpEchoServerHelper echoServer6(14);
    ApplicationContainer serverApps6 = echoServer6.Install(csmaNodes3.Get(0));
    serverApps6.Start(Seconds(1.0));
    serverApps6.Stop(Seconds(5.0));

    UdpEchoClientHelper echoClient6(csma3Interfaces.GetAddress(1), 14);
    echoClient6.SetAttribute("MaxPackets", UintegerValue(1));
    echoClient6.SetAttribute("Interval", TimeValue(Seconds(1.0)));
    echoClient6.SetAttribute("PacketSize", UintegerValue(1024));

    ApplicationContainer clientApps6 = echoClient6.Install(internetNode.Get(0));
    clientApps6.Start(Seconds(1.0));
    clientApps6.Stop(Seconds(5.0));
*/
    // Send TCP packets from internet node 0 to csma 3 node 0
    PacketSinkHelper sink("ns3::TcpSocketFactory", InetSocketAddress(csma3Interfaces.GetAddress(1), 15));
    ApplicationContainer serverApps7 = sink.Install(csmaNodes3.Get(0));
    serverApps7.Start(Seconds(1.0));
    serverApps7.Stop(Seconds(5.0));

    OnOffHelper onoff("ns3::TcpSocketFactory", InetSocketAddress(csma3Interfaces.GetAddress(1), 15));
    onoff.SetAttribute("PacketSize", UintegerValue(1024));
    onoff.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1]"));
    onoff.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));
    onoff.SetAttribute("DataRate", StringValue("1Mbps"));

    ApplicationContainer clientApps7 = onoff.Install(internetNode.Get(0));
    clientApps7.Start(Seconds(1.0));
    clientApps7.Stop(Seconds(5.0));

    NS_LOG_INFO("Creating animation interface.");
    AnimationInterface anim("PedagogicalCase.xml");
    anim.EnablePacketMetadata(true);
    anim.EnableIpv4RouteTracking("PedagogicalCase-routing.xml", Seconds(1.0), Seconds(10.0), Seconds(1.0));
    // Set node positions
    anim.SetConstantPosition(csmaNodes2.Get(0), 98.0, 20.0);
    anim.SetConstantPosition(csmaNodes2.Get(1), 98.0, 40.0);
    anim.SetConstantPosition(csmaNodes2.Get(2), 98.0, 60.0);
    anim.SetConstantPosition(csmaNodes1.Get(0), 20.0, 10.0);
    anim.SetConstantPosition(csmaNodes1.Get(1), 40.0, 10.0);
    anim.SetConstantPosition(csmaNodes1.Get(2), 60.0, 10.0);
    anim.SetConstantPosition(csmaNodes0.Get(0), 10.0, 85.0);
    anim.SetConstantPosition(csmaNodes0.Get(1), 30.0, 85.0);
    anim.SetConstantPosition(csmaNodes0.Get(2), 50.0, 85.0);
    anim.SetConstantPosition(switchNode.Get(1), 40.0, 20.0);
    anim.SetConstantPosition(switchNode.Get(0), 80.0, 40.0);
    anim.SetConstantPosition(routerNode.Get(0), 90.0, 75.0);
    anim.SetConstantPosition(routerNode.Get(1), 80.0, 10.0);
    anim.SetConstantPosition(internetNode.Get(0), 90.0, 95.0);
    anim.SetConstantPosition(switchNode.Get(2), 30.0, 75.0);
    anim.SetConstantPosition(routerNode.Get(2), 50.0, 70.0);
    anim.SetConstantPosition(routerNode.Get(3), 15.0, 60.0);
    anim.SetConstantPosition(csmaNodes3.Get(0), 15.0, 50.0);

    // Update node images
    uint32_t router_img = anim.AddResource("/home/tom/repos/ns-3-dev/netanim/images/1200px-Router.svg.png");
    uint32_t switch_img = anim.AddResource("/home/tom/repos/ns-3-dev/netanim/images/core-switch.png");
    uint32_t pc_img = anim.AddResource("/home/tom/repos/ns-3-dev/netanim/images/vecteezy_monitor-icon-sign-symbol-design_10158482.png");

    for (int i = 0; i < 3; ++i) {
        anim.UpdateNodeImage(csmaNodes2.Get(i)->GetId(), pc_img);
        anim.UpdateNodeImage(csmaNodes1.Get(i)->GetId(), pc_img);
        anim.UpdateNodeImage(csmaNodes0.Get(i)->GetId(), pc_img);
    }
    anim.UpdateNodeImage(csmaNodes3.Get(0)->GetId(), pc_img);
    anim.UpdateNodeImage(routerNode.Get(0)->GetId(), router_img);
    anim.UpdateNodeImage(routerNode.Get(1)->GetId(), router_img);
    anim.UpdateNodeImage(routerNode.Get(2)->GetId(), router_img);
    anim.UpdateNodeImage(routerNode.Get(3)->GetId(), router_img);
    anim.UpdateNodeImage(switchNode.Get(2)->GetId(), switch_img);
    anim.UpdateNodeImage(switchNode.Get(0)->GetId(), switch_img);
    anim.UpdateNodeImage(switchNode.Get(1)->GetId(), switch_img);
    anim.UpdateNodeImage(internetNode.Get(0)->GetId(), pc_img);

    // Update node sizes
    for (int i = 0; i < 3; ++i) {
        anim.UpdateNodeSize(csmaNodes1.Get(i), 6.0, 6.0);
        anim.UpdateNodeSize(csmaNodes2.Get(i), 6.0, 6.0);
        anim.UpdateNodeSize(csmaNodes0.Get(i), 6.0, 6.0);
    }
    
    anim.UpdateNodeSize(csmaNodes3.Get(0), 6.0, 6.0);
    anim.UpdateNodeSize(switchNode.Get(0), 6.0, 6.0);
    anim.UpdateNodeSize(switchNode.Get(1), 6.0, 6.0);
    anim.UpdateNodeSize(switchNode.Get(2), 6.0, 6.0);
    anim.UpdateNodeSize(routerNode.Get(0), 6.0, 6.0);
    anim.UpdateNodeSize(routerNode.Get(1), 6.0, 6.0);
    anim.UpdateNodeSize(routerNode.Get(2), 6.0, 6.0);
    anim.UpdateNodeSize(routerNode.Get(3), 6.0, 6.0);
    anim.UpdateNodeSize(internetNode.Get(0), 6.0, 6.0);

    // Update node descriptions
    // Let's start with the routers
    anim.UpdateNodeDescription(routerNode.Get(0), "ROU 2");
    anim.UpdateNodeDescription(routerNode.Get(1), "ROU 1");
    anim.UpdateNodeDescription(routerNode.Get(2), "ROU 0");
    anim.UpdateNodeDescription(routerNode.Get(3), "ROU 3");

    // Now the switches
    anim.UpdateNodeDescription(switchNode.Get(0), "SW 2");
    anim.UpdateNodeDescription(switchNode.Get(1), "SW 1");
    anim.UpdateNodeDescription(switchNode.Get(2), "SW 0");


    //Simulator::Schedule(Seconds(5.0), &PrintArpCache, routerNode.Get(2));
    csma.EnablePcap("Router1", router1ToSwitch1Link.Get(0), true);


    Simulator::Stop(Seconds(10));
    Simulator::Run();

    //flowMonitor->SerializeToXmlFile("pedagogicalCase-flowmon.xml", true, true);


    Simulator::Destroy();

    return 0;
}
