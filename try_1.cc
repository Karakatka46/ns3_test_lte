#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/lte-module.h"
#include "ns3/config-store.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-helper.h"
#include "ns3/on-off-helper.h"

using namespace ns3;

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

    OnOffHelper onOffHelper("ns3::TcpSocketFactory", InetSocketAddress(ueIpIface.GetAddress(0), 9));
    onOffHelper.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1.0]"));
    onOffHelper.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0.0]"));
    onOffHelper.SetAttribute("DataRate", StringValue("10Mbps"));

    Ptr<UniformRandomVariable> startTime = CreateObject<UniformRandomVariable>();
    startTime->SetAttribute("Min", DoubleValue(0.1));
    startTime->SetAttribute("Max", DoubleValue(0.5));

    for (uint32_t i = 0; i < ueNodes.GetN(); ++i)
    {
        Ptr<Node> ueNode = ueNodes.Get(i);
        ApplicationContainer clientApps = onOffHelper.Install(ueNode);
        clientApps.Start(Seconds(startTime->GetValue()));
        clientApps.Stop(Seconds(10.0));
    }
    for (uint32_t i = 0; i < ueNodes.GetN(); ++i)
{
    Ptr<Node> ueNode = ueNodes.Get(i);
    ApplicationContainer clientApps = onOffHelper.Install(ueNode);
    clientApps.Start(Seconds(3.5));  // Устанавливаем старт с 3.5 секунд
    clientApps.Stop(Seconds(9.5));   // Устанавливаем стоп на 9.5 секунд
}
    lteHelper->EnablePhyTraces();
    lteHelper->EnableMacTraces();
    lteHelper->EnableRlcTraces();

    Simulator::Stop(Seconds(10.0));
    Simulator::Run();
    Simulator::Destroy();

    return 0;
}
