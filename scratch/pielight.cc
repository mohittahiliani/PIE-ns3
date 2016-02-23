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

std::stringstream filePlotQueueDel;
uint32_t i=0;


void CheckQueueDel (Ptr<Queue> queue)
{
  double qDel = StaticCast<PieQueue> (queue)->GetQueueDelay ().GetSeconds();
  // check queue size every 1/50 of a second
  Simulator::Schedule (Seconds (0.01), &CheckQueueDel, queue);
  std::ofstream fPlotQueueDel (filePlotQueueDel.str ().c_str (), std::ios::out|std::ios::app);
  fPlotQueueDel << Simulator::Now ().GetSeconds () << " " << (qDel*1000) << std::endl;
  fPlotQueueDel.close ();
}

int main (int argc, char *argv[])
{

bool printPieStats=true;
bool  isPcapEnabled=true;
float startTime = 0.0;
float simDuration = 100;        //in seconds
std::string  pathOut = "."; 
bool writeForPlot=true;
std::string pcapFileName = "pcapFilePieQueue.pcap";

float stopTime = startTime + simDuration;

 CommandLine cmd;  
 cmd.Parse(argc,argv);

LogComponentEnable ("PieQueue", LOG_LEVEL_INFO);
 
std::string bottleneckBandwidth = "10Mbps";
std::string bottleneckDelay = "50ms";

std::string accessBandwidth ="10Mbps";
std::string accessDelay = "5ms";


NodeContainer source;
source.Create(5);

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


  Config::SetDefault ("ns3::PieQueue::MeanPktSize", UintegerValue (1000));
  Config::SetDefault ("ns3::PieQueue::QueueDelayReference", TimeValue (Seconds(0.02)));
   
 
  InternetStackHelper stack;
  stack.InstallAll ();

  Ipv4AddressHelper address;
  address.SetBase ("10.0.0.0", "255.255.255.0");

  // Configure the source and sink net devices
  // and the channels between the source/sink and the gateway
  Ipv4InterfaceContainer sink_Interfaces;
  Ipv4InterfaceContainer interfaces[5];
  Ipv4InterfaceContainer interfaces_sink;
  Ipv4InterfaceContainer interfaces_gateway;

  NetDeviceContainer devices[5];
  NetDeviceContainer devices_sink;
  NetDeviceContainer devices_gateway;
  


for(i=0;i<5;i++)
{
   devices[i] = accessLink.Install (source.Get (i), gateway.Get (0));
   address.NewNetwork ();
   interfaces[i]=address.Assign(devices[i]);
}


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
  Address sinkLocalAddress (InetSocketAddress (Ipv4Address::GetAny (), port));
  PacketSinkHelper sinkHelper ("ns3::TcpSocketFactory", sinkLocalAddress);

   // Configure application
   AddressValue remoteAddress (InetSocketAddress (sink_Interfaces.GetAddress (0, 0), port));
      for (uint16_t i = 0; i < source.GetN (); i++)
   {
          Config::SetDefault ("ns3::TcpSocket::SegmentSize", UintegerValue (1000));
          BulkSendHelper ftp ("ns3::TcpSocketFactory", Address ());
          ftp.SetAttribute ("Remote", remoteAddress);
          ftp.SetAttribute ("SendSize", UintegerValue (1000));
         

          ApplicationContainer sourceApp = ftp.Install (source.Get (i));
          sourceApp.Start (Seconds (0));
          sourceApp.Stop (Seconds (stopTime - 1));
         
          sinkHelper.SetAttribute ("Protocol", TypeIdValue (TcpSocketFactory::GetTypeId ()));
          ApplicationContainer sinkApp = sinkHelper.Install (sink);
          sinkApp.Start (Seconds (0));
          sinkApp.Stop (Seconds (stopTime));
         
     }
     



   if (writeForPlot)
    {
     
      filePlotQueueDel << pathOut << "/" << "delaylight.plotme";               
      remove (filePlotQueueDel.str ().c_str ());                               
     
      Ptr<PointToPointNetDevice> nd = StaticCast<PointToPointNetDevice> (devices_gateway.Get (0));
      Ptr<Queue> queue = nd->GetQueue ();
      Simulator::ScheduleNow (&CheckQueueDel, queue);  
            
    }


   if (isPcapEnabled)
    {
      bottleneckLink.EnablePcap (pcapFileName,gateway,false);
    }

   
   FlowMonitorHelper flowmon;
   Ptr<FlowMonitor> allMon ;
   allMon= flowmon.InstallAll(); 
   flowmon.SerializeToXmlFile("PieQueue.xml", true, true);

  Simulator::Stop (Seconds (stopTime));
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





 





