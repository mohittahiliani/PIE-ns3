#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/flow-monitor-helper.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include <fstream>
#include "ns3/internet-module.h"
#include "ns3/ipv6-static-routing-helper.h"
#include "ns3/ipv6-routing-table-entry.h"
#include "ns3/internet-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/tcp-header.h"
#include  <string>

using namespace ns3;
 
NS_LOG_COMPONENT_DEFINE ("PieTests");
std::stringstream filePlotQueueNoDrop;

uint32_t i=0;

void CheckCumulativeDrops (Ptr<Queue> queue)
{
  uint32_t ndrop = StaticCast<PieQueue> (queue)->GetDropCount ();
 
  // check queue size every 1/50 of a second
  Simulator::Schedule (Seconds (0.001), &CheckCumulativeDrops, queue);
  std::ofstream fPlotQueueNoDrop (filePlotQueueNoDrop.str ().c_str (), std::ios::out|std::ios::app);
  fPlotQueueNoDrop << Simulator::Now ().GetSeconds () << " " << ndrop << std::endl;
  fPlotQueueNoDrop.close ();

}


int main (int argc, char *argv[])
{

bool printPieStats=true;
bool  isPcapEnabled=true;
std::string  pathOut = "."; 
bool writeForPlot=true;
std::string pcapFileName = "pcapFilePieQueue.pcap";



 CommandLine cmd;  
 cmd.Parse(argc,argv);

LogComponentEnable ("PieQueue", LOG_LEVEL_INFO);

std::string bottleneckBandwidth = "10Mbps";
std::string bottleneckDelay = "50ms";

std::string accessBandwidth ="50Mbps";
std::string accessDelay = "5ms";

NodeContainer udpsource;
udpsource.Create(1);

NodeContainer gateway;
gateway.Create (2);

NodeContainer sink;
sink.Create(1);


// Create and configure access link and bottleneck link
  PointToPointHelper accessLink;
  accessLink.SetDeviceAttribute ("DataRate", StringValue (accessBandwidth));
  accessLink.SetChannelAttribute ("Delay", StringValue (accessDelay));
  accessLink.SetQueue ("ns3::DropTailQueue");
 

  PointToPointHelper bottleneckLink;
  bottleneckLink.SetDeviceAttribute ("DataRate", StringValue (bottleneckBandwidth));
  bottleneckLink.SetChannelAttribute ("Delay", StringValue (bottleneckDelay));
  bottleneckLink.SetQueue ("ns3::PieQueue");


  Config::SetDefault ("ns3::PieQueue::MeanPktSize", UintegerValue (500));
  Config::SetDefault ("ns3::PieQueue::QueueLimit", UintegerValue (400));
 
 // Config::SetDefault ("ns3::PieQueue::MeanPktSize", UintegerValue (1000));
  Config::SetDefault ("ns3::PieQueue::QueueDelayReference", TimeValue (Seconds(0.02)));
  Config::SetDefault ("ns3::PieQueue::MaxBurstAllowance", TimeValue (Seconds(0))); 
 
  InternetStackHelper stack;
  stack.InstallAll ();

  Ipv4AddressHelper address;
  address.SetBase ("10.0.0.0", "255.255.255.0");

  // Configure the source and sink net devices
  // and the channels between the source/sink and the gateway
  Ipv4InterfaceContainer sink_Interfaces;
  Ipv4InterfaceContainer interfaces_sink;
  Ipv4InterfaceContainer interfaces_gateway;
  Ipv4InterfaceContainer udpinterfaces;

 
  NetDeviceContainer udpdevices;
  NetDeviceContainer devices_sink;
  NetDeviceContainer devices_gateway;
  
 

  udpdevices = accessLink.Install (udpsource.Get (0), gateway.Get (0));
  address.NewNetwork();
  udpinterfaces=address.Assign(udpdevices);

  devices_gateway= bottleneckLink.Install (gateway.Get (0), gateway.Get (1));
  address.NewNetwork();
  interfaces_gateway = address.Assign (devices_gateway);
  
  devices_sink= accessLink.Install (gateway.Get (1), sink.Get (0));
  address.NewNetwork ();
  interfaces_sink = address.Assign (devices_sink);
  
  sink_Interfaces.Add (interfaces_sink.Get (1));

  NS_LOG_INFO ("Initialize Global Routing.");
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();


  uint16_t port = 50000;

   // Configure application
   AddressValue remoteAddress1 (InetSocketAddress (sink_Interfaces.GetAddress (0, 0), port));
  // AddressValue remoteAddress1 (InetSocketAddress (sink_Interfaces.GetAddress (0, 0), port1));
   
      OnOffHelper clientHelper6 ("ns3::UdpSocketFactory", Address ());
      clientHelper6.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
      clientHelper6.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
      clientHelper6.SetAttribute ("DataRate", DataRateValue (DataRate ("25Mbps")));
      clientHelper6.SetAttribute ("PacketSize", UintegerValue (500));

      ApplicationContainer clientApps6;
     // AddressValue remoteAddress1 (InetSocketAddress (udpinterfaces.GetAddress (0), port));
      clientHelper6.SetAttribute ("Remote", remoteAddress1);
      clientApps6.Add (clientHelper6.Install (udpsource.Get (0)));
      clientApps6.Start (Seconds (0.99));
      clientApps6.Stop (Seconds (1.2));

     

  Address sinkLocalAddress (InetSocketAddress (Ipv4Address::GetAny (), port));
  PacketSinkHelper sinkHelper ("ns3::UdpSocketFactory", sinkLocalAddress);      
  sinkHelper.SetAttribute ("Protocol", TypeIdValue (UdpSocketFactory::GetTypeId ()));
  ApplicationContainer sinkApp = sinkHelper.Install (sink);
  sinkApp.Start (Seconds (0));
  sinkApp.Stop (Seconds (1.2));

     
     



   if (writeForPlot)
    {
      
      filePlotQueueNoDrop<< pathOut << "/" << "number of drop.plotme";
      remove (filePlotQueueNoDrop.str ().c_str ());                                
     

      Ptr<PointToPointNetDevice> nd = StaticCast<PointToPointNetDevice> (devices_gateway.Get (0));
      Ptr<Queue> queue = nd->GetQueue ();
      Simulator::ScheduleNow (&CheckCumulativeDrops, queue); 
         
    }


   if (isPcapEnabled)
    {
      bottleneckLink.EnablePcap (pcapFileName,gateway,false);
    }

   
   FlowMonitorHelper flowmon;
   Ptr<FlowMonitor> allMon ;
   allMon= flowmon.InstallAll(); 
   
  
  flowmon.SerializeToXmlFile("PieQueue.xml", true, true);

  Simulator::Stop (Seconds (1.5));
  Simulator::Run ();

   if (printPieStats)
    {
      Ptr<PointToPointNetDevice> nd1 = StaticCast<PointToPointNetDevice> (devices_gateway.Get (0));
      PieQueue::Stats st1 = StaticCast<PieQueue> (nd1->GetQueue ())->GetStats ();
      std::cout << "*** pie stats from Node 2 queue ***" << std::endl;
      std::cout << "\t " << st1.unforcedDrop << " drops due to probability " << std::endl;
      std::cout << "\t " << st1.forcedDrop << " drops due queue full" << std::endl;
     

      Ptr<PointToPointNetDevice> nd2 = StaticCast<PointToPointNetDevice> (devices_gateway.Get (1));
      PieQueue::Stats st2 = StaticCast<PieQueue> (nd2->GetQueue ())->GetStats ();
      std::cout << "*** pie stats from Node 2 queue ***" << std::endl;
      std::cout << "\t " << st2.unforcedDrop << " drops due to probability " << std::endl;
      std::cout << "\t " << st2.forcedDrop << " drops due queue full" << std::endl;
        
    }

  Simulator::Destroy ();
  return 0;
 
}





 





