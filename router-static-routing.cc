/*
 * Three-site WAN with redundant connectivity
 * Triangular topology: HQ, Branch Office, and Data Center
 *
 * Network Topology:
 *
 *           HQ (n0)
 *          /      \
 *    10.1.1.0/30  10.1.3.0/30
 *        /          \
 *   Branch(n1)----DC(n2)
 *      10.1.2.0/30
 *
 * IP Address Allocation:
 * - Link HQ-Branch (10.1.1.0/30):
 *     HQ: 10.1.1.1, Branch: 10.1.1.2
 * - Link Branch-DC (10.1.2.0/30):
 *     Branch: 10.1.2.1, DC: 10.1.2.2
 * - Link HQ-DC (10.1.3.0/30):
 *     HQ: 10.1.3.1, DC: 10.1.3.2
 */

#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/netanim-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include <iostream>

using namespace ns3;
using namespace std;

NS_LOG_COMPONENT_DEFINE("RedundantWAN");


void
DisableLink(Ptr<NetDevice> device)
{
    Ptr<PointToPointNetDevice> p2pDevice = DynamicCast<PointToPointNetDevice>(device);
    if (p2pDevice)
    {
        cout << "\n!!! LINK FAILURE at " << Simulator::Now().GetSeconds() << "s !!!" << endl;
        
        // Get the channel and disable it
        Ptr<PointToPointChannel> channel = p2pDevice->GetChannel()->GetObject<PointToPointChannel>();
        if (channel)
        {
            // Set both devices on the channel to down
            Ptr<PointToPointNetDevice> dev1 = channel->GetPointToPointDevice(0);
            Ptr<PointToPointNetDevice> dev2 = channel->GetPointToPointDevice(1);
            
            if (dev1) dev1->SetMtu(0); // Effectively disable
            if (dev2) dev2->SetMtu(0);
        }
        
        cout << "Primary path (HQ-DC) is DOWN\n" << endl;
    }
}

void
EnableLink(Ptr<NetDevice> device)
{
    Ptr<PointToPointNetDevice> p2pDevice = DynamicCast<PointToPointNetDevice>(device);
    if (p2pDevice)
    {
        cout << "\n*** LINK RESTORED at " << Simulator::Now().GetSeconds() << "s ***" << endl;
        
        // Get the channel and re-enable it
        Ptr<PointToPointChannel> channel = p2pDevice->GetChannel()->GetObject<PointToPointChannel>();
        if (channel)
        {
            // Restore both devices on the channel
            Ptr<PointToPointNetDevice> dev1 = channel->GetPointToPointDevice(0);
            Ptr<PointToPointNetDevice> dev2 = channel->GetPointToPointDevice(1);
            
            if (dev1) dev1->SetMtu(1500); // Restore normal MTU
            if (dev2) dev2->SetMtu(1500);
        }
        
        cout << "Primary path (HQ-DC) is UP\n" << endl;
    }
}


int
main(int argc, char* argv[])
{
    // Enable logging
    LogComponentEnable("UdpEchoClientApplication", LOG_LEVEL_INFO);
    LogComponentEnable("UdpEchoServerApplication", LOG_LEVEL_INFO);

    // Create three nodes: n0 (HQ), n1 (Branch), n2 (DC)
    NodeContainer nodes;
    nodes.Create(3);

    Ptr<Node> n0 = nodes.Get(0); // HQ (Headquarters)
    Ptr<Node> n1 = nodes.Get(1); // Branch Office
    Ptr<Node> n2 = nodes.Get(2); // Data Center

    // Create point-to-point links
    PointToPointHelper p2p;
    p2p.SetDeviceAttribute("DataRate", StringValue("5Mbps"));
    p2p.SetChannelAttribute("Delay", StringValue("2ms"));

    // Link 1: n0 (HQ) <-> n1 (Branch) - Network 1
    NodeContainer link1Nodes(n0, n1);
    NetDeviceContainer link1Devices = p2p.Install(link1Nodes);

    // Link 2: n1 (Branch) <-> n2 (DC) - Network 2
    NodeContainer link2Nodes(n1, n2);
    NetDeviceContainer link2Devices = p2p.Install(link2Nodes);

    // Link 3: n0 (HQ) <-> n2 (DC) - Network 3 (REDUNDANT PATH)
    NodeContainer link3Nodes(n0, n2);
    NetDeviceContainer link3Devices = p2p.Install(link3Nodes);

    // Install mobility model to keep nodes at fixed positions
    MobilityHelper mobility;
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(nodes);

    // Set the positions for each node
    Ptr<MobilityModel> mob0 = n0->GetObject<MobilityModel>();
    Ptr<MobilityModel> mob1 = n1->GetObject<MobilityModel>();
    Ptr<MobilityModel> mob2 = n2->GetObject<MobilityModel>();

    // Triangle layout: HQ at top, Branch bottom-left, DC bottom-right
    mob0->SetPosition(Vector(10.0, 2.0, 0.0));  // HQ at top center
    mob1->SetPosition(Vector(5.0, 15.0, 0.0));  // Branch at bottom-left
    mob2->SetPosition(Vector(15.0, 15.0, 0.0)); // DC at bottom-right

    // Install Internet stack on all nodes
    InternetStackHelper stack;
    stack.Install(nodes);

    // *** IP Address Assignment ***
    
    // Assign IP addresses to Network 1 (10.1.1.0/30) - HQ <-> Branch
    Ipv4AddressHelper address1;
    address1.SetBase("10.1.1.0", "255.255.255.252"); // /30 subnet
    Ipv4InterfaceContainer interfaces1 = address1.Assign(link1Devices);
    // interfaces1.GetAddress(0) = 10.1.1.1 (n0 - HQ)
    // interfaces1.GetAddress(1) = 10.1.1.2 (n1 - Branch)

    // Assign IP addresses to Network 2 (10.1.2.0/30) - Branch <-> DC
    Ipv4AddressHelper address2;
    address2.SetBase("10.1.2.0", "255.255.255.252"); // /30 subnet
    Ipv4InterfaceContainer interfaces2 = address2.Assign(link2Devices);
    // interfaces2.GetAddress(0) = 10.1.2.1 (n1 - Branch)
    // interfaces2.GetAddress(1) = 10.1.2.2 (n2 - DC)

    // Assign IP addresses to Network 3 (10.1.3.0/30) - HQ <-> DC
    Ipv4AddressHelper address3;
    address3.SetBase("10.1.3.0", "255.255.255.252"); // /30 subnet
    Ipv4InterfaceContainer interfaces3 = address3.Assign(link3Devices);
    // interfaces3.GetAddress(0) = 10.1.3.1 (n0 - HQ)
    // interfaces3.GetAddress(1) = 10.1.3.2 (n2 - DC)

    // *** Configure Static Routing ***

    // Enable IP forwarding on ALL nodes (they all act as routers)
    Ptr<Ipv4> ipv4HQ = n0->GetObject<Ipv4>();
    ipv4HQ->SetAttribute("IpForward", BooleanValue(true));

    Ptr<Ipv4> ipv4Branch = n1->GetObject<Ipv4>();
    ipv4Branch->SetAttribute("IpForward", BooleanValue(true));

    Ptr<Ipv4> ipv4DC = n2->GetObject<Ipv4>();
    ipv4DC->SetAttribute("IpForward", BooleanValue(true));

    // Get static routing protocol helper
    Ipv4StaticRoutingHelper staticRoutingHelper;

    // ============================================
    // Configure routing on n0 (HQ)
    // ============================================
    // HQ is directly connected to:
    //   - Branch via 10.1.1.0/30 (interface 1)
    //   - DC via 10.1.3.0/30 (interface 2)
    // HQ needs a route to reach Branch-DC network (10.1.2.0/30)
    
    Ptr<Ipv4StaticRouting> staticRoutingHQ =
        staticRoutingHelper.GetStaticRouting(n0->GetObject<Ipv4>());
    
    // Route to 10.1.2.0/30 (Branch-DC network) via DC
    staticRoutingHQ->AddNetworkRouteTo(
        Ipv4Address("10.1.2.0"),      // Destination network
        Ipv4Mask("255.255.255.252"),  // Network mask (/30)
        Ipv4Address("10.1.3.2"),      // Next hop: DC's IP on HQ-DC link
        2                             // Interface index: HQ's interface to DC
    );

    // ============================================
    // Configure routing on n1 (Branch)
    // ============================================
    // Branch is directly connected to:
    //   - HQ via 10.1.1.0/30 (interface 1)
    //   - DC via 10.1.2.0/30 (interface 2)
    // Branch needs a route to reach HQ-DC network (10.1.3.0/30)
    
    Ptr<Ipv4StaticRouting> staticRoutingBranch =
        staticRoutingHelper.GetStaticRouting(n1->GetObject<Ipv4>());
    
    // Route to 10.1.3.0/30 (HQ-DC network) via HQ
    staticRoutingBranch->AddNetworkRouteTo(
        Ipv4Address("10.1.3.0"),      // Destination network
        Ipv4Mask("255.255.255.252"),  // Network mask (/30)
        Ipv4Address("10.1.1.1"),      // Next hop: HQ's IP on HQ-Branch link
        1                             // Interface index: Branch's interface to HQ
    );

    // ============================================
    // Configure routing on n2 (DC)
    // ============================================
    // DC is directly connected to:
    //   - Branch via 10.1.2.0/30 (interface 1)
    //   - HQ via 10.1.3.0/30 (interface 2)
    // DC needs a route to reach HQ-Branch network (10.1.1.0/30)
    
    Ptr<Ipv4StaticRouting> staticRoutingDC =
        staticRoutingHelper.GetStaticRouting(n2->GetObject<Ipv4>());
    
    // Route to 10.1.1.0/30 (HQ-Branch network) via HQ
    staticRoutingDC->AddNetworkRouteTo(
        Ipv4Address("10.1.1.0"),      // Destination network
        Ipv4Mask("255.255.255.252"),  // Network mask (/30)
        Ipv4Address("10.1.3.1"),      // Next hop: HQ's IP on HQ-DC link
        2                             // Interface index: DC's interface to HQ
    );

    // Print routing tables for verification
    Ptr<OutputStreamWrapper> routingStream =
        Create<OutputStreamWrapper>("router-static-routing.routes", std::ios::out);
    staticRoutingHelper.PrintRoutingTableAllAt(Seconds(1.0), routingStream);

    // *** Display Network Configuration ***
    
    cout << "\n========================================" << endl;
    cout << "Network Configuration Summary" << endl;
    cout << "========================================\n" << endl;

    cout << "HQ (n0) Interfaces:" << endl;
    cout << "  - To Branch (n1): " << interfaces1.GetAddress(0) << " (Network 10.1.1.0/30)" << endl;
    cout << "  - To DC (n2):     " << interfaces3.GetAddress(0) << " (Network 10.1.3.0/30)" << endl;

    cout << "\nBranch (n1) Interfaces:" << endl;
    cout << "  - To HQ (n0): " << interfaces1.GetAddress(1) << " (Network 10.1.1.0/30)" << endl;
    cout << "  - To DC (n2): " << interfaces2.GetAddress(0) << " (Network 10.1.2.0/30)" << endl;

    cout << "\nDC (n2) Interfaces:" << endl;
    cout << "  - To Branch (n1): " << interfaces2.GetAddress(1) << " (Network 10.1.2.0/30)" << endl;
    cout << "  - To HQ (n0):     " << interfaces3.GetAddress(1) << " (Network 10.1.3.0/30)" << endl;

    cout << "\n========================================" << endl;
    cout << "Redundant Paths Available" << endl;
    cout << "========================================" << endl;
    cout << "HQ -> DC:" << endl;
    cout << "  Primary: HQ -> DC (direct via 10.1.3.0/30)" << endl;
    cout << "  Backup:  HQ -> Branch -> DC" << endl;

    cout << "\nHQ -> Branch:" << endl;
    cout << "  Primary: HQ -> Branch (direct via 10.1.1.0/30)" << endl;
    cout << "  Backup:  HQ -> DC -> Branch" << endl;

    cout << "\nBranch -> DC:" << endl;
    cout << "  Primary: Branch -> DC (direct via 10.1.2.0/30)" << endl;
    cout << "  Backup:  Branch -> HQ -> DC" << endl;
    cout << "========================================\n" << endl;

    // *** Application Layer - UDP Echo ***

    cout << "Installing Applications..." << endl;

    // Server 1: UDP Echo Server on Branch (n1)
    uint16_t port1 = 9;
    UdpEchoServerHelper echoServer1(port1);
    ApplicationContainer serverApps1 = echoServer1.Install(n1);
    serverApps1.Start(Seconds(1.0));
    serverApps1.Stop(Seconds(11.0));
    cout << "  - Echo Server on Branch: " << interfaces1.GetAddress(1) << ":" << port1 << endl;

    // Server 2: UDP Echo Server on DC (n2)
    uint16_t port2 = 10;
    UdpEchoServerHelper echoServer2(port2);
    ApplicationContainer serverApps2 = echoServer2.Install(n2);
    serverApps2.Start(Seconds(1.0));
    serverApps2.Stop(Seconds(11.0));
    cout << "  - Echo Server on DC: " << interfaces2.GetAddress(1) << ":" << port2 << endl;

    // Client 1: HQ sends to Branch (testing direct HQ-Branch link)
    cout << "\nClient Applications:" << endl;
    UdpEchoClientHelper echoClient1(interfaces1.GetAddress(1), port1);
    echoClient1.SetAttribute("MaxPackets", UintegerValue(4));
    echoClient1.SetAttribute("Interval", TimeValue(Seconds(2.0)));
    echoClient1.SetAttribute("PacketSize", UintegerValue(1024));

    ApplicationContainer clientApps1 = echoClient1.Install(n0);
    clientApps1.Start(Seconds(2.0));
    clientApps1.Stop(Seconds(11.0));
    cout << "  - HQ -> Branch (direct path via 10.1.1.0/30)" << endl;

    // Client 2: HQ sends to DC (testing direct HQ-DC link)
    UdpEchoClientHelper echoClient2(interfaces3.GetAddress(1), port2);
    echoClient2.SetAttribute("MaxPackets", UintegerValue(4));
    echoClient2.SetAttribute("Interval", TimeValue(Seconds(2.0)));
    echoClient2.SetAttribute("PacketSize", UintegerValue(1024));

    ApplicationContainer clientApps2 = echoClient2.Install(n0);
    clientApps2.Start(Seconds(3.0));
    clientApps2.Stop(Seconds(11.0));
    cout << "  - HQ -> DC (direct path via 10.1.3.0/30)" << endl;

    // Client 3: Branch sends to DC (testing Branch-DC link)
    UdpEchoClientHelper echoClient3(interfaces2.GetAddress(1), port2);
    echoClient3.SetAttribute("MaxPackets", UintegerValue(4));
    echoClient3.SetAttribute("Interval", TimeValue(Seconds(2.5)));
    echoClient3.SetAttribute("PacketSize", UintegerValue(512));

    ApplicationContainer clientApps3 = echoClient3.Install(n1);
    clientApps3.Start(Seconds(4.0));
    clientApps3.Stop(Seconds(11.0));
    cout << "  - Branch -> DC (direct path via 10.1.2.0/30)" << endl;



    // ============================================
    // SIMULATE LINK FAILURE FOR TESTING BACKUP PATH
    // ============================================

    cout << "\n========================================" << endl;
    cout << "Link Failure Simulation Configuration" << endl;
    cout << "========================================" << endl;
    cout << "Timeline:" << endl;
    cout << "  t=0-4s:   Normal operation (primary path active)" << endl;
    cout << "  t=4s:     HQ-DC link FAILS" << endl;
    cout << "  t=4-8s:   Traffic uses backup path (HQ->Branch->DC)" << endl;
    cout << "  t=8s:     HQ-DC link RESTORED" << endl;
    cout << "  t=8-12s:  Traffic returns to primary path" << endl;
    cout << "========================================\n" << endl;

    // Schedule link failure at t=4 seconds
    // Disable BOTH ends of the link to simulate complete failure
    Simulator::Schedule(Seconds(4.0), &DisableLink, link3Devices.Get(0)); // HQ's end
    Simulator::Schedule(Seconds(4.0), &DisableLink, link3Devices.Get(1)); // DC's end

    // Schedule link restoration at t=8 seconds (optional - to test recovery)
    Simulator::Schedule(Seconds(8.0), &EnableLink, link3Devices.Get(0)); // HQ's end
    Simulator::Schedule(Seconds(8.0), &EnableLink, link3Devices.Get(1)); // DC's end


    // *** NetAnim Configuration ***
    AnimationInterface anim("router-static-routing.xml");

    // Node positions are already set via MobilityModel above
    // NetAnim will automatically use the mobility model positions

    // Set node descriptions
    anim.UpdateNodeDescription(n0, "HQ\n10.1.1.1 | 10.1.3.1");
    anim.UpdateNodeDescription(n1, "Branch\n10.1.1.2 | 10.1.2.1");
    anim.UpdateNodeDescription(n2, "DC\n10.1.2.2 | 10.1.3.2");

    // Set node colors
    anim.UpdateNodeColor(n0, 0, 255, 0);   // Green for HQ
    anim.UpdateNodeColor(n1, 255, 255, 0); // Yellow for Branch
    anim.UpdateNodeColor(n2, 0, 0, 255);   // Blue for DC

    // Enable PCAP tracing on all devices for Wireshark analysis
    p2p.EnablePcapAll("router-static-routing");

    cout << "\n========================================" << endl;
    cout << "Starting Simulation..." << endl;
    cout << "========================================\n" << endl;

    // Run simulation
    Simulator::Stop(Seconds(12.0));
    Simulator::Run();
    Simulator::Destroy();

    cout << "\n========================================" << endl;
    cout << "Simulation Complete!" << endl;
    cout << "========================================" << endl;
    cout << "Output files saved in current directory:" << endl;
    cout << "  - router-static-routing.xml (NetAnim)" << endl;
    cout << "  - router-static-routing.routes (Routing tables)" << endl;
    cout << "  - router-static-routing-*.pcap (Packet captures)" << endl;
    cout << "\nTo visualize:" << endl;
    cout << "  netanim router-static-routing.xml" << endl;
    cout << "========================================\n" << endl;

    return 0;
}
