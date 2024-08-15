#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/animation-interface.h"
#include "ns3/flow-monitor-module.h"

using namespace ns3;

int
main(int argc, char* argv[])
{

    bool verbose = true;
    uint32_t nCsma = 4;
    bool tracing = false;

    CommandLine cmd(__FILE__);
    cmd.AddValue("nCsma", "Number of \"extra\" CSMA nodes/devices", nCsma);
    cmd.AddValue("verbose", "Tell echo applications to log if true", verbose);
    cmd.AddValue("tracing", "Enable pcap tracing", tracing);

    cmd.Parse(argc, argv);

    //Activation des logs
    if(verbose)
    {
        LogComponentEnable("UdpEchoClientApplication", LOG_LEVEL_INFO);
        LogComponentEnable("UdpEchoServerApplication", LOG_LEVEL_INFO);
    }

    //Pour le premier réseau CSMA
    NodeContainer csmaNodes0;

    //Pour le deuxième réseau CSMA
    NodeContainer csmaNodes1;

    //Pour le troisième réseau CSMA
    NodeContainer csmaNodes2;

    //Pour le quatrième réseau CSMA
    NodeContainer csmaNodes3;

    //Création de deux noeuds point à point
    NodeContainer p2pNodes;

    //création des noeuds
    p2pNodes.Create(2);
    csmaNodes0.Create(nCsma);
    csmaNodes1.Create(nCsma);
    csmaNodes2.Create(nCsma);
    csmaNodes3.Create(1);

    //caractéristiques du lien csma2
    CsmaHelper csma2;
    csma2.SetChannelAttribute("DataRate", StringValue("100Mbps"));
    csma2.SetChannelAttribute("Delay", TimeValue(NanoSeconds(6560)));
    csmaNodes2.Add(p2pNodes.Get(1));
   
    
    //caractéristiques du lien csma 1
    CsmaHelper csma1;
    csma1.SetChannelAttribute("DataRate", StringValue("100Mbps"));
    csma1.SetChannelAttribute("Delay", TimeValue(NanoSeconds(6560)));
    csmaNodes1.Add(csmaNodes2.Get(3));
    

    //caractéristiques du lien csma 0
    CsmaHelper csma0;
    csma0.SetChannelAttribute("DataRate", StringValue("100Mbps"));
    csma0.SetChannelAttribute("Delay", TimeValue(NanoSeconds(6560)));
    csmaNodes0.Add(csmaNodes1.Get(0));

    //caractéristiques du lien csma 3
    CsmaHelper csma3;
    csma3.SetChannelAttribute("DataRate", StringValue("100Mbps"));
    csma3.SetChannelAttribute("Delay", TimeValue(NanoSeconds(6560)));
    csmaNodes3.Add(csmaNodes0.Get(3));

    NodeContainer RouterNodes;
    RouterNodes.Add(csmaNodes2.Get(3));
    RouterNodes.Add(csmaNodes1.Get(0));
    RouterNodes.Add(csmaNodes0.Get(3));
    RouterNodes.Add(p2pNodes.Get(1));
    

    //Installation des dispositifs CSMA
    NetDeviceContainer csmaDevices0;
    csmaDevices0 = csma0.Install(csmaNodes0);
 
    NetDeviceContainer csmaDevices1;
    csmaDevices1 = csma1.Install(csmaNodes1);

    NetDeviceContainer csmaDevices2;
    csmaDevices2 = csma2.Install(csmaNodes2);

    NetDeviceContainer csmaDevices3;
    csmaDevices3 = csma3.Install(csmaNodes3);

    //caractéristiques du lien point à point
    PointToPointHelper pointToPoint;
    pointToPoint.SetDeviceAttribute("DataRate", StringValue("5Mbps"));
    pointToPoint.SetChannelAttribute("Delay", StringValue("2ms"));

    //Routeur 2 vers internet
    NetDeviceContainer p2pDevices;
    p2pDevices = pointToPoint.Install(p2pNodes);

    //Configuration des adresses IP
    InternetStackHelper stack;
    stack.Install(csmaNodes0);
    stack.Install(csmaNodes1);
    stack.Install(csmaNodes2);
    stack.Install(csmaNodes3);
    stack.Install(p2pNodes);

    //Definition de l'ojet d'adresse IP 
    Ipv4AddressHelper address;

    //Création des interfaces pour P2P
    
    address.SetBase("14.14.14.0", "255.255.255.0");
    Ipv4InterfaceContainer p2pInterfaces;
    p2pInterfaces = address.Assign(p2pDevices);
    

    //Création des interfaces pour LAN 2
    
    address.SetBase("192.168.2.0" , "255.255.255.0");
    Ipv4InterfaceContainer lan2interfaces;
    lan2interfaces = address.Assign(csmaDevices2);

    //Création des interfaces pour LAN 1
    
    address.SetBase("192.168.1.0" , "255.255.255.0");
    Ipv4InterfaceContainer lan1interfaces;
    lan1interfaces = address.Assign(csmaDevices1);

    //Création des interfaces pour LAN 0
    
    address.SetBase("192.168.0.0" , "255.255.255.0");
    Ipv4InterfaceContainer lan0interfaces;
    lan0interfaces = address.Assign(csmaDevices0);

    //Création des interfaces pour LAN 3
    
    address.SetBase("10.0.0.0" , "255.255.255.0");
    Ipv4InterfaceContainer lan3interfaces;
    lan3interfaces = address.Assign(csmaDevices3);


    //Creation du serveur et du client UDP
    UdpEchoServerHelper echoServer(13);
    ApplicationContainer serverApps = echoServer.Install(csmaNodes3.Get(0));
    serverApps.Start(Seconds(0));
    serverApps.Stop(Seconds(10.0));

    //creation du client
    UdpEchoClientHelper echoClient(lan3interfaces.GetAddress(0), 13);
    echoClient.SetAttribute("MaxPackets", UintegerValue(100));
    echoClient.SetAttribute("Interval", TimeValue(MilliSeconds(200)));
    echoClient.SetAttribute("PacketSize", UintegerValue(1024));

    NodeContainer clientNodes(p2pNodes.Get(0));

    ApplicationContainer clientApps = echoClient.Install(clientNodes);
    clientApps.Start(Seconds(1));
    clientApps.Stop(Seconds(10));


    //Activation du routage
    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    //Activation de la capture de paquets sur tous les noeuds reliés à deux réseaux en spécifiant le nom du fichier de capture

    if(tracing)
    {
        pointToPoint.EnablePcapAll("TFE-topology-UDP");
        csma0.EnablePcap("lan0", csmaDevices0);
        csma1.EnablePcap("lan1", csmaDevices1);
        csma2.EnablePcap("lan2", csmaDevices2);
        csma3.EnablePcap("lan3", csmaDevices3);
        pointToPoint.EnableAscii("TFE-topology-UDP", p2pNodes);
    }



    //Configure la position des noeuds dans une animation
    AnimationInterface anim("TFE-topology-UDP.xml");
    anim.EnablePacketMetadata(true);
    anim.EnableIpv4RouteTracking("routingtable-topology.xml", Seconds(1), Seconds(10), Seconds(1));
    //anim.AddNodeCounter(RouterNodes, "Router");
    
    anim.UpdateNodeDescription(RouterNodes.Get(0), "Routeur 0");
    anim.UpdateNodeDescription(RouterNodes.Get(1), "Routeur 1");
    anim.UpdateNodeDescription(RouterNodes.Get(2), "Routeur 2");
    anim.UpdateNodeDescription(RouterNodes.Get(3), "Routeur 3");
    anim.SetConstantPosition(p2pNodes.Get(0), 43.5, 85.5);
    anim.SetConstantPosition(p2pNodes.Get(1), 43.5, 73);
    csma3.EnablePcapAll("TFE-topology-UDP-csma3");

    int nodeSize = 3;
    //Taille des noeuds
    anim.UpdateNodeSize(p2pNodes.Get(0), nodeSize, nodeSize);
    anim.UpdateNodeSize(p2pNodes.Get(1), nodeSize, nodeSize);
    anim.UpdateNodeSize(csmaNodes3.Get(0), nodeSize, nodeSize);

    for(uint32_t i = 0; i < nCsma; i++)
    {
        anim.UpdateNodeSize(csmaNodes0.Get(i), nodeSize, nodeSize);
        anim.UpdateNodeColor(csmaNodes0.Get(i), 255, 0, 0);
        anim.UpdateNodeSize(csmaNodes1.Get(i), nodeSize, nodeSize);
        anim.UpdateNodeColor(csmaNodes1.Get(i), 0, 255, 0);
        anim.UpdateNodeSize(csmaNodes2.Get(i), nodeSize, nodeSize);
        anim.UpdateNodeColor(csmaNodes2.Get(i), 0, 0, 255);
    }


    int center=53.5;
    int spacing=13;

    //Positionnement des noeuds du LAN 0 à la verticale à gauche
    for(uint32_t i = 0; i < nCsma; i++)
    {
        anim.SetConstantPosition(csmaNodes0.Get(i), center - spacing*i, 63);
    }

    //Positionnement des noeuds du LAN 2 à la verticale à droite des noeuds du LAN 0

    for(uint32_t i = 0; i < nCsma; i++)
    {
        anim.SetConstantPosition(csmaNodes2.Get(i) , 70.0, 63 - spacing*i);
    }

    //Positionnement des noeuds du LAN 1 à l'horizontale au dessus des noeuds du LAN 0 et à gauche des noeuds du LAN 2
    
    for(uint32_t i = 0; i < nCsma; i++)
    {
        anim.SetConstantPosition(csmaNodes1.Get(i), center - spacing*i, 18.0);
    }

    //Positionnement des noeuds du LAN 3 à l'horizontale en dessous des noeuds du LAN 0
    anim.SetConstantPosition(csmaNodes3.Get(0), 33.5, 47.0);

    //Lancement de la simulation
    Simulator::Run();
    Simulator::Destroy();
    return 0;
}