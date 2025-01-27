#include "ns3/mobility-module.h"
#include "ns3/lte-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-helper.h"
#include "ns3/socket.h"
#include "ns3/packet-sink-helper.h"

using namespace ns3;

void GenerateFullBufferTraffic(Ptr<Socket> socket, uint32_t packetSize, Time interval) {
    // Функция для постоянной отправки пакетов
    socket->Send(Create<Packet>(packetSize));
    Simulator::Schedule(interval, &GenerateFullBufferTraffic, socket, packetSize, interval);
}

int main(int argc, char *argv[]) {
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

    Ipv4InterfaceContainer ueIpIface = epcHelper->AssignUeIpv4Address(NetDeviceContainer(ueLteDevs));

    lteHelper->Attach(ueLteDevs, enbLteDevs.Get(0));

    lteHelper->EnablePhyTraces();
    lteHelper->EnableMacTraces();
    lteHelper->EnableRlcTraces();

    Ptr<Socket> dlSocket = Socket::CreateSocket(enbNodes.Get(0), TypeId::LookupByName("ns3::UdpSocketFactory"));
    InetSocketAddress dlAddress(ueIpIface.GetAddress(0), 1234);
    dlSocket->Connect(dlAddress);
    Simulator::Schedule(Seconds(0.1), &GenerateFullBufferTraffic, dlSocket, 1024, MilliSeconds(1));

    // Настройка Full Buffer трафика (UL)
    Ptr<Socket> ulSocket = Socket::CreateSocket(ueNodes.Get(0), TypeId::LookupByName("ns3::UdpSocketFactory"));
    InetSocketAddress ulAddress(epcHelper->GetUeDefaultGatewayAddress(), 1234);
    ulSocket->Connect(ulAddress);
    Simulator::Schedule(Seconds(0.1), &GenerateFullBufferTraffic, ulSocket, 1024, MilliSeconds(1));

    // Установка приложения для приема пакетов (DL)
    PacketSinkHelper packetSinkDlHelper("ns3::UdpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), 1234));
    ApplicationContainer dlSinkApps = packetSinkDlHelper.Install(ueNodes.Get(0));
    dlSinkApps.Start(Seconds(0.0));
    dlSinkApps.Stop(Seconds(10.0));

    // Установка приложения для приема пакетов (UL)
    PacketSinkHelper packetSinkUlHelper("ns3::UdpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), 1234));
    ApplicationContainer ulSinkApps = packetSinkUlHelper.Install(enbNodes.Get(0));
    ulSinkApps.Start(Seconds(0.0));
    ulSinkApps.Stop(Seconds(10.0));

    Simulator::Stop(Seconds(10.0));
    Simulator::Run();
    Simulator::Destroy();

    return 0;
}
