#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/csma-module.h"
#include "ns3/animation-interface.h"

#include <string>

using namespace ns3;

int main(int argc, char *argv[]) {
    CommandLine cmd;
    cmd.Parse(argc, argv);
    bool verbose = true;
    std::string filename = "TFE-topology-TCP.xml";
    //bool tracing = true;   
    bool animation = true;

    if (verbose)
    {
        LogComponentEnable("TcpSocketBase", LOG_LEVEL_INFO);
        LogComponentEnable("TcpL4Protocol", LOG_LEVEL_INFO);
        LogComponentEnable("BulkSendApplication", LOG_LEVEL_INFO);
        LogComponentEnable("PacketSink", LOG_LEVEL_INFO);
        LogComponentEnable("OnOffApplication", LOG_LEVEL_INFO);
    }

    // Nodes
    NodeContainer internetNodes;
    internetNodes.Create(1);

    NodeContainer routerNodes;
    routerNodes.Create(1);

    NodeContainer csma0Nodes;
    csma0Nodes.Create(3);

    NodeContainer csma1Nodes;
    csma1Nodes.Create(4);

    NodeContainer csma2Nodes;
    csma2Nodes.Create(2); 

    NodeContainer csma3Nodes;
    csma3Nodes.Create(3);

    // Point-to-Point Internet to Router
    PointToPointHelper p2p;
    p2p.SetDeviceAttribute("DataRate", StringValue("5Mbps"));
    p2p.SetChannelAttribute("Delay", StringValue("2ms"));
    NetDeviceContainer internetDevices = p2p.Install(internetNodes.Get(0), routerNodes.Get(0));

    // CSMA networks
    CsmaHelper csma;
    csma.SetChannelAttribute("DataRate", StringValue("100Mbps"));
    csma.SetChannelAttribute("Delay", TimeValue(MilliSeconds(2)));



    

    // Connect CSMA networks in a chain (based on your description)

    NetDeviceContainer csma0Devices = csma.Install(NodeContainer(routerNodes,csma0Nodes, csma3Nodes.Get(0))); // CSMA0 to CSMA3
    NetDeviceContainer csma3Devices = csma.Install(NodeContainer(csma3Nodes, csma1Nodes.Get(0))); // CSMA3 to CSMA1
    NetDeviceContainer csma1Devices = csma.Install(NodeContainer(csma1Nodes, csma2Nodes.Get(1))); // CSMA1 to CSMA2
    NetDeviceContainer csma2Devices = csma.Install(csma2Nodes); // CSMA2

    // Internet stack
    InternetStackHelper stack;
    stack.Install(internetNodes);
    stack.Install(routerNodes);
    stack.Install(csma0Nodes);
    stack.Install(csma1Nodes);
    stack.Install(csma2Nodes);
    stack.Install(csma3Nodes);

    // IP addresses
    Ipv4AddressHelper address;
    address.SetBase("14.14.14.0", "255.255.255.0");
    Ipv4InterfaceContainer internetInterfaces = address.Assign(internetDevices);
    
    address.SetBase("192.168.0.0", "255.255.255.0");
    Ipv4InterfaceContainer routerInterfaces = address.Assign(csma0Devices);
    
    address.SetBase("192.168.1.0", "255.255.255.0");
    Ipv4InterfaceContainer csma1Interfaces = address.Assign(csma1Devices);

    address.SetBase("192.168.2.0", "255.255.255.0");
    Ipv4InterfaceContainer csma2Interfaces = address.Assign(csma2Devices);

    address.SetBase("10.0.0.0", "255.255.255.0");
    Ipv4InterfaceContainer csma3Interfaces = address.Assign(csma3Devices);

    // TCP Application to send data from Internet node to N2.1
    uint16_t sinkPort = 8080;
    Address sinkAddress(InetSocketAddress(csma2Interfaces.GetAddress(1), sinkPort));
    PacketSinkHelper packetSinkHelper("ns3::TcpSocketFactory", sinkAddress);
    ApplicationContainer sinkApps = packetSinkHelper.Install(csma2Nodes.Get(1)); // N2.1
    sinkApps.Start(Seconds(1.0));
    sinkApps.Stop(Seconds(8.0));

    BulkSendHelper bulkSend("ns3::TcpSocketFactory", sinkAddress);
    bulkSend.SetAttribute("MaxBytes", UintegerValue(0));
    bulkSend.SetAttribute("SendSize", UintegerValue(1024));
    ApplicationContainer sourceApps = bulkSend.Install(internetNodes.Get(0));
    sourceApps.Start(Seconds(2.0));
    sourceApps.Stop(Seconds(6.0));

    // Routing
    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    // Animation
    if (animation)
    {
        AnimationInterface anim("TFE-topology-TCP.xml");
        anim.EnablePacketMetadata(true);

        anim.UpdateNodeDescription(internetNodes.Get(0), "Internet");
        anim.UpdateNodeDescription(routerNodes.Get(0), "Router");
        anim.UpdateNodeDescription(csma0Nodes.Get(0), "N1.1");
        anim.UpdateNodeDescription(csma0Nodes.Get(1), "N1.2");
        anim.UpdateNodeDescription(csma0Nodes.Get(2), "N1.3");
        anim.UpdateNodeDescription(csma1Nodes.Get(0), "N2.1");
        anim.UpdateNodeDescription(csma1Nodes.Get(1), "N2.2");
        anim.UpdateNodeDescription(csma1Nodes.Get(2), "N2.3");
        anim.UpdateNodeDescription(csma2Nodes.Get(0), "N3.1");
        anim.UpdateNodeDescription(csma3Nodes.Get(0), "N0.1");
        anim.UpdateNodeDescription(csma3Nodes.Get(1), "N0.2");
        anim.UpdateNodeDescription(csma3Nodes.Get(2), "N0.3");

        //update the size of the nodes with a loop
        anim.UpdateNodeSize(internetNodes.Get(0), 2.0, 2.0);
        anim.UpdateNodeSize(routerNodes.Get(0), 2.0, 2.0);
        anim.UpdateNodeSize(csma2Nodes.Get(0), 2.0, 2.0);

        for(uint32_t i = 0; i < csma0Nodes.GetN(); i++)
        {
            anim.UpdateNodeSize(csma0Nodes.Get(i), 2.0, 2.0);
        }

        for(uint32_t i = 0; i < csma1Nodes.GetN(); i++)
        {
            anim.UpdateNodeSize(csma1Nodes.Get(i), 2.0, 2.0);
        }

        for(uint32_t i = 0; i < csma3Nodes.GetN(); i++)
        {
            anim.UpdateNodeSize(csma3Nodes.Get(i), 2.0, 2.0);
        }


        //update the color of the nodes with a loop
        anim.UpdateNodeColor(internetNodes.Get(0), 255, 0, 0);
        anim.UpdateNodeColor(routerNodes.Get(0), 255, 0, 0);
        anim.UpdateNodeColor(csma2Nodes.Get(0), 0, 255, 0);

        for(uint32_t i = 0; i < csma0Nodes.GetN(); i++)
        {
            anim.UpdateNodeColor(csma0Nodes.Get(i), 0, 255, 0);
        }

        for(uint32_t i = 0; i < csma1Nodes.GetN(); i++)
        {
            anim.UpdateNodeColor(csma1Nodes.Get(i), 0, 0, 255);
        }

        for(uint32_t i = 0; i < csma3Nodes.GetN(); i++)
        {
            anim.UpdateNodeColor(csma3Nodes.Get(i), 255, 255, 0);
        }
        
        int spacing=13;

        //update the position of the nodes
        anim.SetConstantPosition(internetNodes.Get(0), 43.5, 85.5);
        anim.SetConstantPosition(routerNodes.Get(0), 43.5, 73);

        for(uint32_t i = 0; i < csma0Nodes.GetN(); i++)
        {
            anim.SetConstantPosition(csma0Nodes.Get(i) , 70.0, 63 - spacing*i);
        }

        for(uint32_t i = 0; i < csma1Nodes.GetN(); i++)
        {
            anim.SetConstantPosition(csma1Nodes.Get(i), 43.5 - spacing*i, 63);
        }

        for(uint32_t i = 0; i < csma2Nodes.GetN(); i++)
        {
            anim.SetConstantPosition(csma2Nodes.Get(i), 17.5, 63 - spacing*i);
        }

        for(uint32_t i = 0; i < csma3Nodes.GetN(); i++)
        {
            anim.SetConstantPosition(csma3Nodes.Get(i), 43.5 - spacing*i, 18.0);
        }
    }
    

    // Enable packet capture
    p2p.EnablePcapAll("TFE-topology-TCP");
    //csma.EnablePcap("TCP-lan0", csma0Devices);
    //csma.EnablePcap("TCP-lan1", csma1Devices);
    csma.EnablePcap("TCP-lan2", csma2Devices);
    //csma.EnablePcap("TCP-lan3", csma3Devices);

    // Simulation
    Simulator::Run();
    Simulator::Destroy();

    return 0;
}
