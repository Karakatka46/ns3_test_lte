#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/lte-module.h"
#include "ns3/config-store.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-helper.h"
#include "ns3/socket.h"
#include "ns3/packet-sink.h"
#include "ns3/packet-sink-helper.h"
#include "ns3/on-off-helper.h"

using namespace ns3;

void SendPacket(Ptr<Socket> socket, Ptr<Packet> packet)
{
    socket->Send(packet);
}

int main(int argc, char *argv[])
{
    LogComponentEnable("LteHelper", LOG_LEVEL_INFO);
    LogComponentEnable("EpcHelper", LOG_LEVEL_INFO);
    LogComponentEnable("LteEnbRrc", LOG_LEVEL_INFO);
    LogComponentEnable("LteUeRrc", LOG_LEVEL_INFO);

    Ptr<LteHelper> lteHelper = CreateObject<LteHelper>();
    Ptr<PointToPointEpcHelper> epcHelper = CreateObject<PointToPointEpcHelper>();
    lteHelper->SetEpcHelper(epcHelper);

    lteHelper->SetSchedulerType("ns3::PfFfMacScheduler");

    NodeContainer enbNodes;
    enbNodes.Create(1);
    NodeContainer ueNodes;
    ueNodes.Create(2);

    MobilityHelper mobility;
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(enbNodes);
    mobility.Install(ueNodes);

    NetDeviceContainer enbLteDevs = lteHelper->InstallEnbDevice(enbNodes);
    NetDeviceContainer ueLteDevs = lteHelper->InstallUeDevice(ueNodes);

    InternetStackHelper internet;
    internet.Install(ueNodes);

    Ipv4InterfaceContainer ueIpIface;
    ueIpIface = epcHelper->AssignUeIpv4Address(NetDeviceContainer(ueLteDevs));

    lteHelper->Attach(ueLteDevs, enbLteDevs.Get(0));
lteHelper->EnablePhyTraces();
    lteHelper->EnableMacTraces();
    lteHelper->EnableRlcTraces();
    // Traffic generation in DL
    OnOffHelper onOffDl("ns3::UdpSocketFactory", InetSocketAddress(ueIpIface.GetAddress(0), 9));
    onOffDl.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1.0]"));
    onOffDl.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0.0]"));
    onOffDl.SetAttribute("DataRate", StringValue("10Mbps"));
    onOffDl.SetAttribute("PacketSize", UintegerValue(1024));
    ApplicationContainer dlApps = onOffDl.Install(enbNodes.Get(0));
    dlApps.Start(Seconds(0.1));
    dlApps.Stop(Seconds(10.0));

    // Traffic generation in UL
    OnOffHelper onOffUl("ns3::UdpSocketFactory", InetSocketAddress(epcHelper->GetUeDefaultGatewayAddress(), 9));
    onOffUl.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1.0]"));
    onOffUl.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0.0]"));
    onOffUl.SetAttribute("DataRate", StringValue("10Mbps"));
    onOffUl.SetAttribute("PacketSize", UintegerValue(1024));
    ApplicationContainer ulApps = onOffUl.Install(ueNodes.Get(0));
    ulApps.Start(Seconds(0.1));
    ulApps.Stop(Seconds(10.0));

    Ptr<Socket> ns3Socket = Socket::CreateSocket(ueNodes.Get(0), TypeId::LookupByName("ns3::UdpSocketFactory"));
    InetSocketAddress remoteAddress = InetSocketAddress(ueIpIface.GetAddress(0), 9);
    ns3Socket->Connect(remoteAddress);

    Ptr<Packet> packet = Create<Packet>(1024);

    Ptr<UniformRandomVariable> startTime = CreateObject<UniformRandomVariable>();
    startTime->SetAttribute("Min", DoubleValue(0.1));
    startTime->SetAttribute("Max", DoubleValue(0.5));

    Simulator::Schedule(Seconds(startTime->GetValue()), &SendPacket, ns3Socket, packet);

    PacketSinkHelper packetSinkHelper("ns3::UdpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), 9));
    ApplicationContainer sinkApps = packetSinkHelper.Install(ueNodes.Get(1));
    sinkApps.Start(Seconds(0.0));
    sinkApps.Stop(Seconds(10.0));

    Ptr<PacketSink> packetSinkApp = DynamicCast<PacketSink>(sinkApps.Get(0));
    Simulator::Schedule(Seconds(1.0), &PacketSink::GetTotalRx, packetSinkApp);

    

    Simulator::Stop(Seconds(10.0));
    Simulator::Run();
    Simulator::Destroy();

    return 0;
}