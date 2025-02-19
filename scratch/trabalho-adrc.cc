#include "ns3/aodv-helper.h"
#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/dsdv-helper.h"
#include "ns3/flow-monitor-helper.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/flow-monitor.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/network-module.h"
#include "ns3/olsr-helper.h"
#include "ns3/wifi-module.h"

#include <fstream>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("AdHocSimulation");

uint64_t totalControlPackets = 0;
uint64_t totalDataPackets = 0;
uint32_t nodesAtDestination = 0;
uint32_t nNodes = 5;

std::string outputDir = "scratch/resultados/";

void ControlPacketCallback(std::string context, Ptr<const Packet> packet)
{
    uint32_t packetSize = packet->GetSize();
    NS_LOG_INFO("Pacote de Controle Recebido! Tamanho: " << packetSize << " bytes");
    totalControlPackets++;
}

void DataPacketCallback(std::string context, Ptr<const Packet> packet)
{
    uint32_t packetSize = packet->GetSize();
    NS_LOG_INFO("Pacote de Dados Recebido! Tamanho: " << packetSize << " bytes");
    totalDataPackets++;
}

void SetupLeaderGroupMobility(NodeContainer nodes, double speed)
{
    MobilityHelper leaderMobility;
    leaderMobility.SetPositionAllocator("ns3::GridPositionAllocator",
                                        "MinX",
                                        DoubleValue(-50.0),
                                        "MinY",
                                        DoubleValue(-50.0),
                                        "DeltaX",
                                        DoubleValue(10.0),
                                        "DeltaY",
                                        DoubleValue(10.0),
                                        "GridWidth",
                                        UintegerValue(10),
                                        "LayoutType",
                                        StringValue("RowFirst"));

    leaderMobility.SetMobilityModel(
        "ns3::RandomWaypointMobilityModel",
        "Speed",
        StringValue("ns3::ConstantRandomVariable[Constant=" + std::to_string(speed) + "]"),
        "Pause",
        StringValue("ns3::ConstantRandomVariable[Constant=2.0]"),
        "PositionAllocator",
        StringValue("ns3::GridPositionAllocator"));

    leaderMobility.Install(nodes.Get(0));

    MobilityHelper followerMobility;

    Ptr<MobilityModel> leaderMobilityModel = nodes.Get(0)->GetObject<MobilityModel>();
    Vector leaderPosition = leaderMobilityModel->GetPosition();

    Ptr<UniformDiscPositionAllocator> positionAllocator =
        CreateObject<UniformDiscPositionAllocator>();
    positionAllocator->SetAttribute("X", DoubleValue(leaderPosition.x));
    positionAllocator->SetAttribute("Y", DoubleValue(leaderPosition.y));
    positionAllocator->SetAttribute("rho",
                                    DoubleValue(5.0));

    followerMobility.SetPositionAllocator(positionAllocator);
    followerMobility.SetMobilityModel(
        "ns3::RandomWalk2dMobilityModel",
        "Bounds",
        RectangleValue(Rectangle(-100.0, 100.0, -100.0, 100.0)),
        "Speed",
        StringValue("ns3::ConstantRandomVariable[Constant=" + std::to_string(speed) + "]"));

    for (uint32_t i = 1; i < nodes.GetN(); i++)
    {
        followerMobility.Install(nodes.Get(i));
    }
}

void CheckSimulationStatus()
{
    // NS_LOG_UNCOND("Verificando condição: nodesAtDestination = " << nodesAtDestination
    //<< ", nNodes = " << nNodes);

    if (nodesAtDestination == nNodes)
    {
        NS_LOG_UNCOND("Todos os nós chegaram ao destino. Encerrando simulação.");
        Simulator::Stop();
    }
    else
    {
        Simulator::Schedule(Seconds(0.1), &CheckSimulationStatus);
    }
}

void CheckIfNodeReachedDestination(Ptr<Node> node, Vector destination)
{
    Ptr<MobilityModel> mobility = node->GetObject<MobilityModel>();
    Vector currentPosition = mobility->GetPosition();

    if ((currentPosition - destination).GetLength() < 10.0)
    {
        nodesAtDestination++;
        NS_LOG_INFO("Nó " << node->GetId() << " chegou ao destino!");
    }
    else
    {
        Simulator::Schedule(Seconds(0.1), &CheckIfNodeReachedDestination, node, destination);
    }
}

void SetupDynamicGroupMobility(NodeContainer nodes, double speed)
{
    Vector meetingPoints[3] = {
        Vector(0, 0, 0),
        Vector(100, 100, 0),
        Vector(-100, -100, 0)};

    MobilityHelper mobility;

    for (uint32_t i = 0; i < nodes.GetN(); ++i)
    {
        Ptr<Node> node = nodes.Get(i);

        Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator>();
        positionAlloc->Add(meetingPoints[0]);
        mobility.SetPositionAllocator(positionAlloc);

        mobility.SetMobilityModel("ns3::WaypointMobilityModel");
        mobility.Install(node);

        Ptr<WaypointMobilityModel> waypointMobility = node->GetObject<WaypointMobilityModel>();

        std::srand(node->GetId());

        Vector randomWaypoint1(std::rand() % 200 - 100, std::rand() % 200 - 100, 0);

        Vector randomWaypoint2(std::rand() % 200 - 100, std::rand() % 200 - 100, 0);

        Vector currentPosition = meetingPoints[0];
        double currentTime = 0.0;

        waypointMobility->AddWaypoint(Waypoint(Seconds(currentTime), currentPosition));

        double distance = (randomWaypoint1 - currentPosition).GetLength();
        currentTime += distance / speed;
        waypointMobility->AddWaypoint(Waypoint(Seconds(currentTime), randomWaypoint1));
        currentPosition = randomWaypoint1;

        distance = (meetingPoints[1] - currentPosition).GetLength();
        currentTime += distance / speed;
        waypointMobility->AddWaypoint(Waypoint(Seconds(currentTime), meetingPoints[1]));
        currentPosition = meetingPoints[1];

        distance = (randomWaypoint2 - currentPosition).GetLength();
        currentTime += distance / speed;
        waypointMobility->AddWaypoint(Waypoint(Seconds(currentTime), randomWaypoint2));
        currentPosition = randomWaypoint2;

        distance = (meetingPoints[2] - currentPosition).GetLength();
        currentTime += distance / speed;
        waypointMobility->AddWaypoint(Waypoint(Seconds(currentTime), meetingPoints[2]));

        Simulator::Schedule(Seconds(0.1), &CheckIfNodeReachedDestination, node, meetingPoints[2]);
    }
}

void SaveResults(const std::string &filename, double lossRatio, double avgDelay, double controlOverhead)
{
    std::ofstream file;
    file.open(filename, std::ios::app);
    if (file.is_open())
    {
        file << "PacketLossRatio: " << lossRatio << "%, " // ( lost packets / total sent packets )
             << "AvgDelay: " << avgDelay << " s, "        // ( totalDelaySum / totalRxPackets )
             << "ControlOverhead: "                       // ( control packets / total number of packets )
             << controlOverhead * 100 << "%\n"
             << "\n";
        file.close();
    }
    else
    {
        NS_LOG_ERROR("Erro ao abrir o arquivo de saída");
    }
}

int main(int argc, char *argv[])
{
    uint32_t seed = time(nullptr);
    SeedManager::SetSeed(seed);
    SeedManager::SetRun(1);

    uint32_t nNodes = 80;
    std::string mobilityModel = "RandomWalk2d";
    double nodeSpeed = 35;
    std::string routingProtocol = "";

    CommandLine cmd;
    cmd.AddValue("nNodes", "Number of nodes", nNodes);
    cmd.AddValue("mobilityModel",
                 "Mobility model (RandomWalk2d, LeaderGroup, DynamicGroup)",
                 mobilityModel);
    cmd.AddValue("nodeSpeed", "Node speed (1, 15, 35 m/s)", nodeSpeed);
    cmd.AddValue("routingProtocol", "Routing protocol (AODV, OLSR, DSDV)", routingProtocol);
    cmd.Parse(argc, argv);

    std::string outputFile = outputDir + std::to_string(nNodes) + "nodes_" + mobilityModel + "_" +
                             std::to_string(static_cast<int>(nodeSpeed)) + "mps_" +
                             routingProtocol + ".txt";
    NodeContainer nodes;
    nodes.Create(nNodes);

    WifiHelper wifi;
    wifi.SetStandard(WIFI_STANDARD_80211b);

    YansWifiChannelHelper channel;
    channel.SetPropagationDelay("ns3::ConstantSpeedPropagationDelayModel");
    channel.AddPropagationLoss("ns3::FriisPropagationLossModel");
    YansWifiPhyHelper phy;
    phy.SetChannel(channel.Create());

    WifiMacHelper mac;
    mac.SetType("ns3::AdhocWifiMac");

    NetDeviceContainer devices;
    devices = wifi.Install(phy, mac, nodes);

    if (mobilityModel == "RandomWalk2d")
    {
        MobilityHelper mobility;
        mobility.SetMobilityModel(
            "ns3::RandomWalk2dMobilityModel",
            "Bounds",
            RectangleValue(Rectangle(-100, 100, -100, 100)),
            "Speed",
            StringValue("ns3::ConstantRandomVariable[Constant=" + std::to_string(nodeSpeed) + "]"));
        mobility.Install(nodes);
    }
    else if (mobilityModel == "LeaderGroup")
    {
        SetupLeaderGroupMobility(nodes, nodeSpeed);
    }
    else if (mobilityModel == "DynamicGroup")
    {
        SetupDynamicGroupMobility(nodes, nodeSpeed);
    }

    InternetStackHelper internet;
    if (routingProtocol == "AODV")
    {
        AodvHelper aodv;
        internet.SetRoutingHelper(aodv);
        internet.Install(nodes);
    }
    else if (routingProtocol == "OLSR")
    {
        OlsrHelper olsr;
        internet.SetRoutingHelper(olsr);
        internet.Install(nodes);
    }
    else if (routingProtocol == "DSDV")
    {
        DsdvHelper dsdv;
        internet.SetRoutingHelper(dsdv);
        internet.Install(nodes);
    }

    Ipv4AddressHelper address;
    address.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer interfaces = address.Assign(devices);

    uint16_t port = 9;
    for (uint32_t i = 0; i < nNodes; i++)
    {
        UdpEchoServerHelper echoServer(port);
        ApplicationContainer serverApps = echoServer.Install(nodes.Get(i));
        serverApps.Start(Seconds(1.0));
        serverApps.Stop(Seconds(10.0));
    }

    for (uint32_t i = 0; i < nNodes; i++)
    {
        for (uint32_t j = 0; j < nNodes; j++)
        {
            if (i != j)
            {
                UdpEchoClientHelper echoClient(interfaces.GetAddress(j), port);
                echoClient.SetAttribute("MaxPackets", UintegerValue(100));
                echoClient.SetAttribute("PacketSize", UintegerValue(1024));

                ApplicationContainer clientApps = echoClient.Install(nodes.Get(i));
                clientApps.Start(Seconds(2.0));
                clientApps.Stop(Seconds(10.0));
            }
        }
    }

    FlowMonitorHelper flowMonitor;
    Ptr<FlowMonitor> monitor = flowMonitor.InstallAll();

    Config::Connect("/NodeList/*/ApplicationList/*/Tx", MakeCallback(&ControlPacketCallback));
    Config::Connect("/NodeList/*/ApplicationList/*/Rx", MakeCallback(&DataPacketCallback));

    Time startTime = Simulator::Now();

    Simulator::Stop(Seconds(600));

    Simulator::Schedule(Seconds(0.1), &CheckSimulationStatus);

    Simulator::Run();

    Time endTime = Simulator::Now();
    double simulationDuration = (endTime - startTime).GetSeconds();
    std::cout << "Tempo total de simulação: " << simulationDuration << " segundos.\n";

    monitor->CheckForLostPackets();
    Ptr<Ipv4FlowClassifier> classifier =
        DynamicCast<Ipv4FlowClassifier>(flowMonitor.GetClassifier());
    FlowMonitor::FlowStatsContainer stats = monitor->GetFlowStats();

    uint64_t totalPacketsSent = 0;
    uint64_t totalPacketsLost = 0;
    double totalDelaySum = 0.0;
    uint64_t totalRxPackets = 0;

    for (auto it = stats.begin(); it != stats.end(); ++it)
    {
        totalPacketsSent += it->second.txPackets;
        totalPacketsLost += it->second.lostPackets;
        totalDelaySum += it->second.delaySum.GetSeconds();
        totalRxPackets += it->second.rxPackets;
    }

    double totalPacketLossRatio =
        (totalPacketsSent > 0) ? (double)totalPacketsLost / totalPacketsSent * 100 : 0.0;

    double totalAvgDelay = (totalRxPackets > 0) ? totalDelaySum / totalRxPackets : 0.0;

    std::cout << "Total de Pacotes Enviados: " << totalPacketsSent << "\n";
    std::cout << "Total de Pacotes Perdidos: " << totalPacketsLost << "\n";
    std::cout << "Taxa de Perda de Pacotes: " << totalPacketLossRatio << "%\n";
    std::cout << "Atraso Total Médio: " << totalAvgDelay << " s\n";

    double controlOverheadRatio =
        (double)totalControlPackets / (totalControlPackets + totalDataPackets);

    std::cout << totalControlPackets << "," << totalDataPackets << std::endl;
    std::cout << "Sobrecarga de Mensagens de Controle: " << controlOverheadRatio * 100 << "%\n";

    SaveResults(outputFile, totalPacketLossRatio, totalAvgDelay, controlOverheadRatio);

    Simulator::Destroy();

    return 0;
}

// A taxa de perda de pacotes é calculada como a porcentagem de pacotes perdidos em relação ao total
// de pacotes enviados o atraso médio é calculado dividindo a soma dos atrasos (totalDelaySum) pelo
// número total de pacotes recebidos (totalRxPackets): A sobrecarga de controle é calculada como a
// proporção de pacotes de controle em relação ao total de pacotes (controle + dados)
